#pragma once

#include "IConnectionManager.h"
#include "PacketTypes.h"

#include "GameTime.h"

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

        struct _SplitPacket
        {
            int64_t splitId;
            int32_t packetId;
            int32_t partCount;
            size_t packetSize;
            std::vector<Packet> parts;
        };

        struct _UserData
        {
            int64_t bytesUnconfirmed = 0;
            std::vector<_SplitPacket> splitPackets;
        };

    public:
        ServerConnectionManager(uint16_t port);
        ~ServerConnectionManager();

        // CONNECTION
    public:
        std::string StatusString();
        ConnectionStatus Status();
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
        class ServerIDGenerator : public TCPServer::IDGenerator
        {
            int64_t _ID_COUNTER = 1;
        public:
            int64_t _GetNewID() { return _ID_COUNTER++; }
        };

        TCPServer _server;
        std::vector<User> _users;
        std::vector<_UserData> _usersData;
        User _thisUser;
        std::mutex _m_users;
        std::unique_ptr<ServerIDGenerator> _generator;

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