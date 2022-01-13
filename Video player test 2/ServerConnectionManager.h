#pragma once

#include "IConnectionManager.h"

#include "PacketTypes.h"

namespace znet
{
    class ServerConnectionManager : public IConnectionManager
    {
        struct _PacketData
        {
            Packet packet;
            std::vector<int64_t> userIds;
            bool redirected = false;
            int64_t sourceUserId = -1;
        };

        class ServerIDGenerator : public TCPServer::IDGenerator
        {
            int64_t _ID_COUNTER = 1;
        public:
            int64_t _GetNewID()
            {
                return _ID_COUNTER++;
            }
        };

        TCPServer _server;
        std::vector<User> _users;
        std::vector<int64_t> _userBytesUnconfirmed;
        std::mutex _m_users;
        std::unique_ptr<ServerIDGenerator> _generator;

        std::thread _managementThread;
        bool _MANAGEMENT_THR_STOP = false;
        std::queue<_PacketData> _inPackets;
        std::deque<std::pair<_PacketData, int>> _outPackets;
        std::mutex _m_inPackets;
        std::mutex _m_outPackets;
        std::queue<_PacketData> _packetQueue;

    public:
        ServerConnectionManager(uint16_t port)
        {
            _generator = std::make_unique<ServerIDGenerator>();
            _server.SetGenerator(_generator.get());
            _server.StartServer(port);
            _server.AllowNewConnections();
            _managementThread = std::thread(&ServerConnectionManager::_ManageConnections, this);
        }
        ~ServerConnectionManager()
        {
            _server.SetGenerator(nullptr);
        }

        // CONNECTION
    public:
        std::string StatusString()
        {
            switch (Status())
            {
            case ConnectionStatus::RUNNING:
                return "Server running.";
            case ConnectionStatus::OFFLINE:
                return "Offline.";
            default:
                return "";
            }
        }

        ConnectionStatus Status()
        {
            if (_server.Running())
                return ConnectionStatus::RUNNING;
            else
                return ConnectionStatus::OFFLINE;
        }

        std::vector<User> Users()
        {
            std::lock_guard<std::mutex> lock(_m_users);
            return _users;
        }

        User ThisUser()
        {
            return { "", 0 };
        }

        // PACKET INPUT/OUTPUT
    public:
        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0)
        {
            std::lock_guard<std::mutex> lock(_m_outPackets);
            if (priority != 0)
            {
                for (int i = 0; i < _outPackets.size(); i++)
                {
                    if (_outPackets[i].second < priority)
                    {
                        _PacketData data = { std::move(packet), std::move(userIds) };
                        _outPackets.insert(_outPackets.begin() + i, { std::move(data), priority });
                        return;
                    }
                }
            }
            _PacketData data = { std::move(packet), std::move(userIds) };
            _outPackets.push_back({ std::move(data), priority });
        }

        void AddToQueue(Packet&& packet, std::vector<int64_t> userIds)
        {
            _PacketData data = { std::move(packet), std::move(userIds) };
            _packetQueue.push(std::move(data));
        }

        void SendQueue(std::vector<int64_t> userIds, int priority = 0)
        {
            LOCK_GUARD(lock, _m_outPackets);
            if (priority != 0)
            {
                for (int i = 0; i < _outPackets.size(); i++)
                {
                    if (_outPackets[i].second < priority)
                    {
                        while (!_packetQueue.empty())
                        {
                            _outPackets.insert(_outPackets.begin() + i, { std::move(_packetQueue.front()), priority });
                            _packetQueue.pop();
                            i++;
                        }
                        return;
                    }
                }
            }
            while (!_packetQueue.empty())
            {
                _outPackets.push_back({ std::move(_packetQueue.front()), priority });
                _packetQueue.pop();
            }
        }

        void AbortSend(int32_t packetId)
        {
            std::unique_lock<std::mutex> lock(_m_outPackets);
            for (int i = 0; i < _outPackets.size(); i++)
            {
                if (_outPackets[i].first.packet.id == packetId
                    && !_outPackets[i].first.redirected)
                {
                    _outPackets.erase(_outPackets.begin() + i);
                    i--;
                }
            }
        }

        size_t PacketCount() const
        {
            return _inPackets.size();
        }

        std::pair<Packet, int64_t> GetPacket()
        {
            std::unique_lock<std::mutex> lock(_m_inPackets);
            _PacketData data = std::move(_inPackets.front());
            _inPackets.pop();
            return { std::move(data.packet), data.userIds[0] };
        }

        // CONNECTION MANAGEMENT THREAD
    private:
        void _ManageConnections()
        {
            size_t bytesUnconfirmed = 0;
            size_t maxUnconfirmedBytes = 1000000; // 1 MB

            while (!_MANAGEMENT_THR_STOP)
            {
                // Get new connections
                int64_t newUser;
                while ((newUser = _server.GetNewConnection()) != -1)
                {
                    _userBytesUnconfirmed.push_back(0);
                    std::lock_guard<std::mutex> lock(_m_users);

                    // Send new user id to other users
                    for (int i = 0; i < _users.size(); i++)
                    {
                        TCPClientRef connection = _server.Connection(_users[i].id);
                        connection->Send(Packet((int)PacketType::NEW_USER).From(newUser), 2);
                    }

                    TCPClientRef connection = _server.Connection(newUser);
                    // Send the assigned id to the new user (maximum priority since this packet needs to go first)
                    connection->Send(Packet((int)PacketType::ASSIGNED_USER_ID).From(newUser), std::numeric_limits<int>::max());
                    // Send existing user ids to new user
                    size_t byteCount = sizeof(int64_t) * (_users.size() + 1);
                    auto userIdsBytes = std::make_unique<int8_t[]>(byteCount);
                    ((int64_t*)userIdsBytes.get())[0] = 0;
                    for (int i = 0; i < _users.size(); i++)
                    {
                        ((int64_t*)userIdsBytes.get())[i + 1] = _users[i].id;
                    }
                    connection->Send(Packet(std::move(userIdsBytes), byteCount, (int)PacketType::USER_LIST), 2);
                    std::cout << "New list sent" << std::endl;

                    _users.push_back({ "", newUser });
                }

                // Get disconnections
                int64_t disconnectedUser;
                std::vector<int64_t> disconnects;
                while ((disconnectedUser = _server.GetDisconnectedUser()) != -1)
                {
                    std::lock_guard<std::mutex> lock(_m_users);
                    for (int i = 0; i < _users.size(); i++)
                    {
                        if (_users[i].id == disconnectedUser)
                        {
                            disconnects.push_back(disconnectedUser);
                            _users.erase(_users.begin() + i);
                            _userBytesUnconfirmed.erase(_userBytesUnconfirmed.begin() + i);
                            break;
                        }
                    }

                    // Send disconnected user id to other users
                    for (int i = 0; i < _users.size(); i++)
                    {
                        TCPClientRef connection = _server.Connection(_users[i].id);
                        connection->Send(Packet((int)PacketType::DISCONNECTED_USER).From(disconnectedUser), 2);
                    }
                }

                { // Incoming packets
                    for (int i = 0; i < _users.size(); i++)
                    {
                        TCPClientRef client = _server.Connection(_users[i].id);
                        if (!client.Valid())
                            continue;

                        if (client->PacketCount() > 0)
                        {
                            Packet pack1 = client->GetPacket();
                            if (pack1.id == (int32_t)PacketType::BYTE_CONFIRMATION)
                            {
                                _userBytesUnconfirmed[i] -= pack1.Cast<size_t>();
                            }
                            else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
                            {
                                continue;
                            }
                            else if (pack1.id == (int32_t)PacketType::USER_ID)
                            {
                                if (client->PacketCount() > 0)
                                {
                                    Packet pack2 = client->GetPacket();

                                    if (pack2.id == (int)PacketType::PAUSE)
                                    {
                                        ztime::clock[3].Update();
                                        TimePoint now = ztime::clock[3].Now();
                                        std::cout << "[ServerConMgr] " << now.GetTime(SECONDS) << "." << now.GetTime(MICROSECONDS) % 1000000 << std::endl;
                                    }

                                    // Send confirmation packet
                                    Packet confirmation = Packet((int32_t)PacketType::BYTE_CONFIRMATION);
                                    confirmation.From(pack2.size);
                                    client->Send(std::move(confirmation), 1);

                                    // Create destination vector
                                    if (pack1.size % sizeof(int64_t) != 0)
                                        std::cout << "[WARN] User id packet size misaligned" << std::endl;
                                    std::vector<int64_t> to;
                                    for (int j = 0; j < pack1.size / sizeof(int64_t); j++)
                                        to.push_back(((int64_t*)pack1.Bytes())[j]);

                                    // If server user id is in destinations, move it to incoming packet queue
                                    for (int j = 0; j < to.size(); j++)
                                    {
                                        if (to[j] == 0)
                                        {
                                            std::unique_lock<std::mutex> lock(_m_inPackets);
                                            _PacketData data = { pack2.Reference(), { _users[i].id } };
                                            _inPackets.push(std::move(data));
                                            to.erase(to.begin() + j);
                                            break;
                                        }
                                    }

                                    // Add packet to queue to send to remaining destinations
                                    if (!to.empty())
                                    {
                                        std::unique_lock<std::mutex> lock(_m_outPackets);
                                        _PacketData data = { std::move(pack2), to, true, _users[i].id };
                                        _outPackets.push_back({ std::move(data), 0 });
                                    }
                                }
                                else
                                {
                                    std::cout << "[WARN] User id packet received without accompanying data packet" << std::endl;
                                }
                            }
                        }
                    }
                }

                // Outgoing packets
                std::unique_lock<std::mutex> lock(_m_outPackets);
                // Loop through packets until 1 is sent
                for (int i = 0; i < _outPackets.size(); i++)
                {
                    // If the server is sending the packet to itself, no additional processing is necessary
                    std::vector<int64_t> destinationUsers = _outPackets[i].first.userIds;
                    for (int j = 0; j < _outPackets[i].first.userIds.size(); j++)
                    {
                        if (_outPackets[i].first.userIds[j] == ThisUser().id)
                        {
                            _PacketData data = { _outPackets[i].first.packet.Reference(), { ThisUser().id } };
                            std::unique_lock<std::mutex> lock(_m_inPackets);
                            _inPackets.push(std::move(data));
                            _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                            break;
                        }
                    }

                    // Erase packet if no destination is specified
                    if (_outPackets[i].first.userIds.empty())
                    {
                        _outPackets.erase(_outPackets.begin() + i);
                        i--;
                        continue;
                    }

                    PacketView view = _outPackets[i].first.packet.View();

                    // Heavy packets will be postponed until unconfirmed byte count is below a certain value
                    bool heavyPacket = (
                        view.id == (int32_t)PacketType::VIDEO_PACKET ||
                        view.id == (int32_t)PacketType::AUDIO_PACKET ||
                        view.id == (int32_t)PacketType::SUBTITLE_PACKET ||
                        view.id == (int32_t)PacketType::VIDEO_STREAM ||
                        view.id == (int32_t)PacketType::AUDIO_STREAM ||
                        view.id == (int32_t)PacketType::SUBTITLE_STREAM ||
                        view.id == (int32_t)PacketType::ATTACHMENT_STREAM ||
                        view.id == (int32_t)PacketType::DATA_STREAM ||
                        view.id == (int32_t)PacketType::UNKNOWN_STREAM
                    );

                    bool sent = false;

                    // Go through every destination user id and attempt to send
                    for (int j = 0; j < _outPackets[i].first.userIds.size(); j++)
                    {
                        // Find user with matching id
                        int userIndex = -1;
                        for (int k = 0; k < _users.size(); k++)
                        {
                            if (_users[k].id == _outPackets[i].first.userIds[j])
                            {
                                userIndex = k;
                                break;
                            }
                        }
                        // Remove the invalid destination user id
                        if (userIndex == -1)
                        {
                            _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                            j--;
                            continue;
                        }

                        // Get connection object
                        TCPClientRef connection = _server.Connection(_users[userIndex].id);
                        if (!connection.Valid())
                        {
                            // Remove unusable destination user id
                            _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                            j--;
                            continue;
                        }

                        // Postpone sending heavy packets if unconfirmed byte count exceeds max value
                        if (_userBytesUnconfirmed[userIndex] > maxUnconfirmedBytes && heavyPacket)
                            continue;

                        // Reference and send the packet to the destination user
                        auto userIdData = std::make_unique<int8_t[]>(sizeof(int64_t));
                        if (_outPackets[i].first.redirected)
                            *(int64_t*)userIdData.get() = _outPackets[i].first.sourceUserId;
                        else
                            *(int64_t*)userIdData.get() = 0;
                        connection->AddToQueue(Packet(std::move(userIdData), sizeof(int64_t), (int)PacketType::USER_ID));
                        connection->AddToQueue(_outPackets[i].first.packet.Reference());
                        connection->SendQueue(_outPackets[i].second);

                        _userBytesUnconfirmed[userIndex] += _outPackets[i].first.packet.size;
                        sent = true;

                        // Remove destination from list
                        _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                        j--;
                    }

                    // If all destinations have been sent to, remove packet from queue
                    if (_outPackets[i].first.userIds.empty())
                    {
                        _outPackets.erase(_outPackets.begin() + i);
                        i--;
                    }

                    // Send packets one at a time
                    if (sent) break;
                }
                lock.unlock();

                // Sleep if no immediate work needs to be done
                bool packetsIncoming = false;
                for (int i = 0; i < _users.size(); i++)
                {
                    TCPClientRef client = _server.Connection(_users[i].id);
                    if (!client.Valid())
                        continue;
                    if (client->PacketCount() > 0)
                    {
                        packetsIncoming = true;
                        break;
                    }
                }
                if (!packetsIncoming && _outPackets.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
    };
}