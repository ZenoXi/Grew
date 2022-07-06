#pragma once

#include "INetworkManager.h"
#include "PacketTypes.h"

#include "FixedQueue.h"
#include "GameTime.h"

namespace znet
{
    class ServerManager : public INetworkManager
    {
    public:
        ServerManager(uint16_t port);
        ~ServerManager();

        std::vector<User> Users(bool includeSelf);
        std::vector<int64_t> UserIds(bool includeSelf);
        User ThisUser();
        int64_t ThisUserId();
        void SetUsername(std::wstring username);

        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0);
        void AddToQueue(Packet&& packet);
        void SendQueue(std::vector<int64_t> userIds, int priority = 0);
        void AbortSend(int32_t packetId);

        std::string Name() { return StaticName(); }
        static std::string StaticName() { return "server"; }
        NetworkStatus Status();
        std::wstring StatusString();
        std::wstring CloseLabel() const { return L"Close server"; }

    private:
        class _ServerIDGenerator : public TCPServer::IDGenerator
        {
            int64_t _ID_COUNTER = 1;
        public:
            int64_t _GetNewID() { return _ID_COUNTER++; }
        };

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

        TCPServer _server;
        bool _startFailed;
        std::vector<User> _users;
        std::vector<_UserData> _usersData;
        User _thisUser;
        std::mutex _m_users;
        std::unique_ptr<_ServerIDGenerator> _generator;

        std::thread _managementThread;
        bool _MANAGEMENT_THR_STOP = false;
        std::deque<std::pair<_PacketData, int>> _outPackets;
        std::mutex _m_outPackets;
        std::deque<_PacketData> _packetQueue;
        int64_t _splitIdCounter = 0;

        struct _ManagerThreadData
        {
            size_t maxUnconfirmedBytes = 250000; // 1 MB

            size_t bytesSentSinceLastPrint = 0;
            size_t bytesReceivedSinceLastPrint = 0;
            TimePoint lastPrintTime = ztime::Main();
            Duration printInterval = Duration(1, SECONDS);
            bool printStats = true;

            TimePoint lastKeepAliveSendTime = ztime::Main();
        };

        // Places the packets in the outgoing packet queue,
        // taking into account packet priority
        void _AddPacketsToOutQueue(std::vector<_PacketData> packets, int priority);
        // Places a packet in the outgoing packet queue,
        // taking into account packet priority
        void _AddPacketToOutQueue(_PacketData packet, int priority);

        void _ManageConnections();
        void _ProcessNewConnections(_ManagerThreadData& data);
        void _ProcessDisconnects(_ManagerThreadData& data);
        void _ProcessIncomingPackets(_ManagerThreadData& data);
        void _ProcessOutgoingPackets(_ManagerThreadData& data);
        void _PrintNetworkStats(_ManagerThreadData& data);
        bool _ProcessSplitPackets(_ManagerThreadData& data, Packet& packet, int userIndex);
    };
}