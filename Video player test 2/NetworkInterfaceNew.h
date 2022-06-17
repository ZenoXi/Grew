#pragma once

#include <fstream>

#include "NetBase2.h"
#include "GameTime.h"
#include "NetworkMode.h"
#include "IConnectionManager.h"
#include "ClientConnectionManager.h"
#include "ServerConnectionManager.h"

#include "PacketSubscriber.h"

#include <memory>

namespace znet
{
    struct PacketData
    {
        char* data;
        size_t size;
        int packetId;
        int userId;
    };

    class NetworkInterface
    {
        znet::TCPServer _server;
        znet::TCPClient _client;

        std::unique_ptr<IConnectionManager> _connectionManager = nullptr;
        bool _PACKET_THR_STOP = false;
        std::thread _incomingPacketManager;
        std::mutex _m_conMng;

        NetworkMode _mode = NetworkMode::OFFLINE;

        std::wstring _username = L"";

        // Singleton interface
    private:
        NetworkInterface();
        static NetworkInterface* _instance;
    public:
        static void Init();
        static NetworkInterface* Instance();


        // Connection functions
    public:
        void StartServer(USHORT port);
        void StopServer();
        void Connect(std::string ip, USHORT port);
        void Disconnect();
    private:
        void _ManageIncomingPackets();


        // Sending functions
    public:
        std::vector<IConnectionManager::User> Users();
        IConnectionManager::User ThisUser();
        void SetUsername(std::wstring username);
        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0);
        void AddToQueue(Packet&& packet, std::vector<int64_t> userIds);
        void SendQueue(std::vector<int64_t> userIds, int priority = 0);
        void AbortSend(int32_t packetId);

        
        // Packet subscription
    private:
        friend class PacketSubscriber;
        struct _PacketSubscription
        {
            int32_t packetId;
            std::vector<PacketSubscriber*> subs;
        };
        std::vector<_PacketSubscription> _subscriptions;
        std::mutex _m_subscriptions;
        void _Subscribe(PacketSubscriber* sub);
        void _Unsubscribe(PacketSubscriber* sub);
        void _DistributePacket(Packet packet, int64_t userId);


        // Network status
    public:
        NetworkMode Mode();

        std::string StatusString();
        ConnectionStatus Status();

        // Sender functions
        //void SendVideoParams(FFmpeg::Result params);
        //void SendAudioParams(FFmpeg::Result params);
        //void SendVideoPacket(FFmpeg::Result packet);
        //void SendAudioPacket(FFmpeg::Result packet);
        //void SendMetadata(Duration duration, AVRational framerate, AVRational videoTimebase, AVRational audioTimebase);
    };
}