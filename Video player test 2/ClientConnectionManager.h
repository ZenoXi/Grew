#pragma once

#include "IConnectionManager.h"

#include "PacketTypes.h"

namespace znet
{
    class ClientConnectionManager : public IConnectionManager
    {
        struct _PacketData
        {
            Packet packet;
            std::vector<int64_t> userIds;
        };

        TCPClient _client;
        std::vector<User> _users;

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
            if (_connectionId != -1)
                return ConnectionStatus::CONNECTED;
            if (_connecting)
                return ConnectionStatus::CONNECTING;
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
            if (_connectionId > 0)
                _managementThread = std::thread(&ClientConnectionManager::_ManageConnections, this);

            _connecting = false;
        }
    public:
        std::vector<User> Users()
        {
            return _users;
        }

        // PACKET INPUT/OUTPUT
    public:
        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0)
        {
            if (priority == 0)
            {
                std::unique_lock<std::mutex> lock(_m_outPackets);
                _PacketData data = { std::move(packet), std::move(userIds) };
                _outPackets.push_back({ std::move(data), priority });
                return;
            }

            std::unique_lock<std::mutex> lock(_m_outPackets);
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

        void AddToQueue(Packet&& packet, std::vector<int64_t> userIds)
        {
            _PacketData data = { std::move(packet), std::move(userIds) };
            _packetQueue.push(std::move(data));
        }

        void SendQueue(std::vector<int64_t> userIds, int priority = 0)
        {
            if (priority == 0)
            {
                LOCK_GUARD(lock, _m_outPackets);
                while (!_packetQueue.empty())
                {
                    _outPackets.push_back({ std::move(_packetQueue.front()), priority });
                    _packetQueue.pop();
                }
                return;
            }

            LOCK_GUARD(lock, _m_outPackets);
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
            size_t maxUnconfirmedBytes = 1000000; // 1 MB

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
                    }
                    else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
                    {
                        _connectionId = -1;
                        _disconnected = true;
                        break;
                    }
                    else if (pack1.id == (int32_t)PacketType::USER_ID)
                    {
                        if (connection->PacketCount() > 0)
                        {
                            Packet pack2 = connection->GetPacket();

                            // Send confirmation packet
                            Packet confirmation = Packet((int32_t)PacketType::BYTE_CONFIRMATION);
                            confirmation.From(pack2.size);
                            connection->Send(std::move(confirmation), 1);

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
                        PacketView view = _outPackets[i].first.packet.View();

                        // Heavy packets will be postponed until unconfirmed byte count is below a certain value
                        bool heavyPacket = (
                            view.id == (int32_t)PacketType::VIDEO_PACKET ||
                            view.id == (int32_t)PacketType::AUDIO_PACKET ||
                            view.id == (int32_t)PacketType::SUBTITLE_PACKET ||
                            view.id == (int32_t)PacketType::STREAM
                        );

                        if (bytesUnconfirmed > maxUnconfirmedBytes && heavyPacket)
                            continue;

                        _PacketData data = std::move(_outPackets[i].first);
                        int priority = _outPackets[i].second;
                        _outPackets.erase(_outPackets.begin() + i);
                        lock.unlock();

                        bytesUnconfirmed += data.packet.size;

                        // Construct packets
                        size_t userIdDataSize = data.userIds.size() * sizeof(int64_t);
                        auto userIdData = std::make_unique<int8_t[]>(userIdDataSize);
                        for (int j = 0; j < data.userIds.size(); j++)
                        {
                            ((int64_t*)userIdData.get())[j] = data.userIds[j];
                        }
                        connection->AddToQueue(Packet(std::move(userIdData), userIdDataSize, (int)PacketType::USER_ID));
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

                // Sleep if no immediate work needs to be done
                if (_inPackets.empty() && _outPackets.empty())
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