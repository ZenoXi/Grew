#pragma once

#include "INetworkManager.h"
#include "PacketTypes.h"

#include "FixedQueue.h"
#include "GameTime.h"

namespace znet
{
    class ClientManager : public INetworkManager
    {
    public:
        ClientManager(std::string ip, uint16_t port, std::string password = "");
        ~ClientManager();
        void Start();

        std::vector<User> Users(bool includeSelf);
        std::vector<int64_t> UserIds(bool includeSelf);
        User ThisUser();
        int64_t ThisUserId();

        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0);
        void AddToQueue(Packet&& packet);
        void SendQueue(std::vector<int64_t> userIds, int priority = 0);
        void AbortSend(int32_t packetId);

        std::string Name() { return StaticName(); }
        static std::string StaticName() { return "client"; }
        NetworkStatus Status();
        std::wstring StatusString();
        std::wstring CloseLabel() const { return L"Disconnect"; }

        bool Connecting() const { return _connecting; }
        bool ConnectSuccessful() const { return !_connectFailed; }
        bool PasswordRequired() const { return _passwordRequired; }
        std::wstring FailMessage() const { return _failMessage; }
        int FailCode() const { return _failCode; }

    private:
        struct _PacketData
        {
            Packet packet;
            std::vector<int64_t> userIds;
            bool prefixed = true;
        };

        void _Connect(std::string ip, uint16_t port);

        bool _connectFailed = false;
        std::wstring _failMessage = L"";
        int _failCode = 0;

        bool _passwordRequired;
        std::string _password;

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
        std::deque<std::pair<_PacketData, int>> _outPackets;
        std::mutex _m_outPackets;
        std::deque<_PacketData> _packetQueue;
        int64_t _splitIdCounter = 0;

        struct _ManagerThreadData
        {
            struct SplitPacket
            {
                int64_t splitId;
                int32_t packetId;
                int32_t partCount;
                size_t packetSize;
                std::vector<znet::Packet> parts;
            };

            TCPClientRef& connection;

            Clock threadTimer = Clock();

            std::vector<SplitPacket> splitPackets;
            size_t bytesUnconfirmed = 0;
            size_t maxUnconfirmedBytes = 250000; // 1 MB
            std::queue<TimePoint> packetsInTransmittion;

            TimePoint latencyPacketSendTime = ztime::Main();
            TimePoint latencyPacketReceiveTime = ztime::Main();
            Duration latencyPacketInterval = Duration(100, MILLISECONDS);
            bool latencyPacketInTransmission = false;
            FixedQueue<Duration> packetLatencies;

            size_t bytesSentSinceLastPrint = 0;
            size_t bytesReceivedSinceLastPrint = 0;
            TimePoint lastPrintTime = ztime::Main();
            Duration printInterval = Duration(1, SECONDS);
            bool printStats = true;

            TimePoint lastKeepAliveReceiveTime = ztime::Main();
        };

        // Places the packets in the outgoing packet queue,
        // taking into account packet priority
        void _AddPacketsToOutQueue(std::vector<_PacketData> packets, int priority);
        // Places a packet in the outgoing packet queue,
        // taking into account packet priority
        void _AddPacketToOutQueue(_PacketData packet, int priority);

        void _ManageConnections();
        bool _SendLatencyProbePackets(_ManagerThreadData& data);
        bool _CheckIfConnectionExists(_ManagerThreadData& data);
        bool _ProcessIncomingPackets(_ManagerThreadData& data);
        bool _ProcessOutgoingPackets(_ManagerThreadData& data);
        void _PrintNetworkStats(_ManagerThreadData& data);
    };
}