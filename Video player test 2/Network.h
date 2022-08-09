#pragma once

#include "NetBase2.h"
#include "GameTime.h"
#include "NetworkMode.h"
#include "INetworkManager.h"

#include "PacketSubscriber.h"

#define APP_NETWORK znet::Network::Instance()

namespace znet
{
    class Network
    {
        // Singleton interface
    private:
        Network();
        static Network* _instance;
    public:
        static void Init();
        static Network* Instance();

        // Network functions
        void SetManager(std::unique_ptr<INetworkManager> networkManager);
        void StartManager();
        void CloseManager();
        // Returns the text which the close button should have (ex: "Disconnect" or "Close server")
        std::wstring CloseLabel() const;

        // Network status

        std::string ManagerName();
        NetworkStatus ManagerStatus();
        std::wstring ManagerStatusString();
        std::vector<INetworkManager::User> Users(bool includeSelf = false);
        std::vector<int64_t> UserIds(bool includeSelf = false);
        INetworkManager::User ThisUser();
        int64_t ThisUserId();

        // Operation functions

        void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0);
        void AddToQueue(Packet&& packet);
        void SendQueue(std::vector<int64_t> userIds, int priority = 0);
        void AbortSend(int32_t packetId);

        // Manager options interface

        // TODO: functions for retrieving manager options

    private:
        std::unique_ptr<INetworkManager> _networkManager = nullptr;

        // Packet subscription
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
    };
}