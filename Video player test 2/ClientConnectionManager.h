#pragma once

#include "IConnectionManager.h"
#include "PacketTypes.h"

#include "FixedQueue.h"

namespace znet
{
    class ClientConnectionManager : public IConnectionManager
    {
        struct _PacketData
        {
            Packet packet;
            std::vector<int64_t> userIds;
            bool prefixed = true;
        };

        TCPClient _client;
        std::vector<User> _users;
        std::mutex _m_users;
        User _thisUser;

        std::thread _connectionThread;
        int64_t _connectionId = -1;
        bool _connecting = false;
        bool _disconnected = false;
        bool _disconnecting = false;
        bool _connectionLost = false;

        std::thread _managementThread;
        bool _MANAGEMENT_THR_STOP = false;
        std::queue<_PacketData> _inPackets;
        std::deque<std::pair<_PacketData, int>> _outPackets;
        std::mutex _m_inPackets;
        std::mutex _m_outPackets;
        std::queue<_PacketData> _packetQueue;

    public:
        ClientConnectionManager(std::string ip, uint16_t port)
        {
            _connecting = true;
            _connectionId = -1;
            _connectionThread = std::thread(&ClientConnectionManager::_Connect, this, ip, port);
        }
        ~ClientConnectionManager()
        {
            _MANAGEMENT_THR_STOP = true;

            if (_connectionThread.joinable())
                _connectionThread.join();
            if (_managementThread.joinable())
                _managementThread.join();
        }

        // CONNECTION
    public:
        bool Connecting() const
        {
            return _connecting;
        }

        bool ConnectionSuccess() const
        {
            return !_connecting && _connectionId > 0;
        }

        std::string StatusString()
        {
            switch (Status())
            {
            case ConnectionStatus::CONNECTED:
                return "Connected!";
            case ConnectionStatus::CONNECTING:
                return "Connecting..";
            case ConnectionStatus::DISCONNECTED:
                return "Disconnected.";
            case ConnectionStatus::DISCONNECTING:
                return "Disconnecting..";
            case ConnectionStatus::CONNECTION_LOST:
                return "Connection lost..";
            default:
                return "";
            }
        }

        ConnectionStatus Status()
        {
            if (_connecting)
                return ConnectionStatus::CONNECTING;
            if (_connectionId != -1)
                return ConnectionStatus::CONNECTED;
            if (_disconnecting)
                return ConnectionStatus::DISCONNECTING;
            if (_disconnected)
                return ConnectionStatus::DISCONNECTED;
            if (_connectionLost)
                return ConnectionStatus::CONNECTION_LOST;

            return ConnectionStatus::UNKNOWN;
        }
    private:
        void _Connect(std::string ip, uint16_t port)
        {
            _connectionId = _client.Connect(ip, port);
            if (_connectionId != -1)
            {
                // Wait for assigned id to arrive
                TCPClientRef connection = _client.Connection(_connectionId);
                while (connection->PacketCount() == 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                Packet idPacket = connection->GetPacket();
                if (idPacket.id != (int)PacketType::ASSIGNED_USER_ID)
                {
                    _connecting = false;
                    // TODO: correctly disconnect
                    connection->Disconnect();
                    _connectionId = -1;
                    return;
                }
                _thisUser.id = idPacket.Cast<int64_t>();

                // Send self username
                if (!_thisUser.name.empty())
                {
                    size_t byteCount = sizeof(int64_t) + sizeof(wchar_t) * _thisUser.name.length();
                    auto usernameBytes = std::make_unique<int8_t[]>(byteCount);
                    ((int64_t*)usernameBytes.get())[0] = 0;
                    for (int j = 0; j < _thisUser.name.length(); j++)
                    {
                        ((wchar_t*)(usernameBytes.get() + sizeof(int64_t)))[j] = _thisUser.name[j];
                    }
                    connection->Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), 2);
                }

                // Start management thread
                _managementThread = std::thread(&ClientConnectionManager::_ManageConnections, this);
            }

            _connecting = false;
        }
    public:
        std::vector<User> Users()
        {
            std::lock_guard<std::mutex> lock(_m_users);
            return _users;
        }

        User ThisUser()
        {
            return _thisUser;
        }

        void SetUsername(std::wstring username)
        {
            _thisUser.name = username;

            // Send new username
            size_t byteCount = sizeof(int64_t) + sizeof(wchar_t) * _thisUser.name.length();
            auto usernameBytes = std::make_unique<int8_t[]>(byteCount);
            ((int64_t*)usernameBytes.get())[0] = 0;
            for (int j = 0; j < _thisUser.name.length(); j++)
            {
                ((wchar_t*)(usernameBytes.get() + sizeof(int64_t)))[j] = _thisUser.name[j];
            }
            std::cout << "Username change sent" << std::endl;
            Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), { 0 }, 2);
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
                if (_outPackets[i].first.packet.id == packetId)
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
            TCPClientRef connection = _client.Connection(_connectionId);

            size_t bytesUnconfirmed = 0;
            size_t maxUnconfirmedBytes = 250000; // 1 MB

            Clock threadTimer = Clock();
            std::queue<TimePoint> packetsInTransmittion;
            FixedQueue<Duration> packetLatencies(16);
            packetLatencies.Fill(Duration(0));

            size_t bytesSentSinceLastPrint = 0;
            size_t bytesReceivedSinceLastPrint = 0;
            TimePoint lastPrintTime = ztime::Main();
            Duration printInterval = Duration(3, SECONDS);
            bool printSpeed = true;

            while (!_MANAGEMENT_THR_STOP)
            {
                // Check if connection still exists
                if (!connection->Connected())
                {
                    _connectionId = -1;
                    _connectionLost = true;
                    break;
                }

                // Incoming packets
                if (connection->PacketCount() > 0)
                {
                    Packet pack1 = connection->GetPacket();
                    if (pack1.id == (int32_t)PacketType::BYTE_CONFIRMATION)
                    {
                        bytesUnconfirmed -= pack1.Cast<size_t>();
                        // Increment connection speed counter
                        bytesSentSinceLastPrint += pack1.Cast<size_t>();
                        // Calculate latency
                        threadTimer.Update();
                        TimePoint returnTime = threadTimer.Now();
                        TimePoint sendTime = packetsInTransmittion.front();
                        packetsInTransmittion.pop();
                        packetLatencies.Push(returnTime - sendTime);
                    }
                    else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
                    {
                        _connectionId = -1;
                        _disconnected = true;
                        break;
                    }
                    else if (pack1.id == (int32_t)PacketType::NEW_USER)
                    {
                        int64_t newUserId = pack1.Cast<int64_t>();
                        std::lock_guard<std::mutex> lock(_m_users);
                        _users.push_back({ L"", newUserId });
                    }
                    else if (pack1.id == (int32_t)PacketType::USER_DATA)
                    {
                        int64_t userId = pack1.Cast<int64_t>();
                        size_t nameLength = (pack1.size - sizeof(int64_t)) / sizeof(wchar_t);
                        std::wstring username;
                        username.resize(nameLength);
                        for (int i = 0; i < nameLength; i++)
                        {
                            username[i] = *((wchar_t*)(pack1.Bytes() + sizeof(int64_t)) + i);
                        }

                        std::lock_guard<std::mutex> lock(_m_users);
                        for (int i = 0; i < _users.size(); i++)
                        {
                            if (_users[i].id == userId)
                            {
                                _users[i].name = username;
                                break;
                            }
                        }
                    }
                    else if (pack1.id == (int32_t)PacketType::DISCONNECTED_USER)
                    {
                        int64_t disconnectedUserId = pack1.Cast<int32_t>();
                        std::lock_guard<std::mutex> lock(_m_users);
                        for (int i = 0; i < _users.size(); i++)
                        {
                            if (_users[i].id == disconnectedUserId)
                            {
                                _users.erase(_users.begin() + i);
                                break;
                            }
                        }
                    }
                    else if (pack1.id == (int32_t)PacketType::USER_LIST)
                    {
                        size_t userCount = pack1.size / sizeof(int64_t);
                        std::lock_guard<std::mutex> lock(_m_users);
                        _users.resize(userCount);
                        for (int i = 0; i < userCount; i++)
                            _users[i] = { L"", ((int64_t*)pack1.Bytes())[i] };
                    }
                    else if (pack1.id == (int32_t)PacketType::USER_ID)
                    {
                        if (connection->PacketCount() > 0)
                        {
                            Packet pack2 = connection->GetPacket();

                            // Increment connection speed counter
                            bytesReceivedSinceLastPrint += pack2.size;

                            // Send confirmation packet
                            connection->Send(Packet((int)PacketType::BYTE_CONFIRMATION).From(pack2.size), 1);

                            int64_t from = pack1.Cast<int32_t>();
                            std::unique_lock<std::mutex> lock(_m_inPackets);
                            _PacketData data = { std::move(pack2), { from } };
                            _inPackets.push(std::move(data));
                        }
                        else
                        {
                            std::cout << "[WARN] User id packet received without accompanying data packet" << std::endl;
                        }
                    }
                }

                // Outgoing packets
                std::unique_lock<std::mutex> lock(_m_outPackets);
                if (_outPackets.size() > 0)
                {
                    for (int i = 0; i < _outPackets.size(); i++)
                    {
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

                        if (bytesUnconfirmed > maxUnconfirmedBytes && heavyPacket)
                            continue;

                        _PacketData data = std::move(_outPackets[i].first);
                        int priority = _outPackets[i].second;
                        _outPackets.erase(_outPackets.begin() + i);
                        lock.unlock();

                        bytesUnconfirmed += data.packet.size;
                        threadTimer.Update();
                        packetsInTransmittion.push(threadTimer.Now());

                        // Some packet types don't need a prefix
                        bool prefixed = !(
                            view.id == (int32_t)PacketType::USER_DATA
                        );


                        // Construct packets
                        if (prefixed)
                        {
                            size_t userIdDataSize = data.userIds.size() * sizeof(int64_t);
                            auto userIdData = std::make_unique<int8_t[]>(userIdDataSize);
                            for (int j = 0; j < data.userIds.size(); j++)
                            {
                                ((int64_t*)userIdData.get())[j] = data.userIds[j];
                            }
                            connection->AddToQueue(Packet(std::move(userIdData), userIdDataSize, (int)PacketType::USER_ID));
                        }
                        connection->AddToQueue(std::move(data.packet));
                        connection->SendQueue(priority);

                        break;
                    }
                    if (lock)
                        lock.unlock();
                }
                else
                {
                    lock.unlock();
                }

                // Print connection speed
                Duration timeElapsed = ztime::Main() - lastPrintTime;
                if (printSpeed && timeElapsed >= printInterval)
                {
                    int64_t latency = 0;
                    for (int i = 0; i < packetLatencies.Size(); i++)
                        latency += packetLatencies[i].GetDuration();
                    latency /= packetLatencies.Size();

                    lastPrintTime = ztime::Main();
                    std::cout << "[INFO] Avg. send speed: " << bytesSentSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
                    std::cout << "[INFO] Avg. receive speed: " << bytesReceivedSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
                    std::cout << "[INFO] Avg. latency over " << packetLatencies.Size() << " packets: " << latency << "us" << std::endl;
                    bytesSentSinceLastPrint = 0;
                    bytesReceivedSinceLastPrint = 0;
                }

                // Sleep if no immediate work needs to be done
                if (connection->PacketCount() == 0 && _outPackets.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        void _ManageIncomingPackets()
        {

        }

        void _ManageOutgoingPackets()
        {

        }
    };
}