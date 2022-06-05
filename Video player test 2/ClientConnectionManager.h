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

    public:
        ClientConnectionManager(std::string ip, uint16_t port);
        ~ClientConnectionManager();

        // CONNECTION
    public:
        bool Connecting() const;
        bool ConnectionSuccess() const;
        std::string StatusString();
        ConnectionStatus Status();
    private:
        void _Connect(std::string ip, uint16_t port);
    public:
        std::vector<User> Users();
        User ThisUser();
        void SetUsername(std::wstring username);

        // PACKET INPUT/OUTPUT
    public:
        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0);
        void AddToQueue(Packet&& packet, std::vector<int64_t> userIds);
        void SendQueue(std::vector<int64_t> userIds, int priority = 0);
        void AbortSend(int32_t packetId);
        size_t PacketCount() const;
        std::pair<Packet, int64_t> GetPacket();

        // CONNECTION MANAGEMENT THREAD
    private:
        void _ManageConnections();


    private:
        TCPClient _client;
        std::vector<User> _users;
        std::mutex _m_users;
        User _thisUser;

        std::thread _connectionThread;
        bool _CONN_THR_STOP = false;
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
        int64_t _splitIdCounter = 0;
    };
}