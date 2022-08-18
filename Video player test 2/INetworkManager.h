#pragma once

#include "NetBase2.h"

namespace znet
{
    class INetworkManager
    {
    public:
        struct User
        {
            int64_t id;
        };

        virtual ~INetworkManager() {};
        virtual void Start() = 0;

        virtual std::vector<User> Users(bool includeSelf) = 0;
        virtual std::vector<int64_t> UserIds(bool includeSelf) = 0;
        virtual User ThisUser() = 0;
        virtual int64_t ThisUserId() = 0;

        virtual void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0) = 0;
        virtual void AddToQueue(Packet&& packet) = 0;
        virtual void SendQueue(std::vector<int64_t> userIds, int priority = 0) = 0;
        virtual void AbortSend(int32_t packetId) = 0;
        void SetOnPacketReceived(std::function<void(Packet, int64_t)> handler)
        {
            _packetReceivedHandler = handler;
        }

    protected:
        std::function<void(Packet, int64_t)> _packetReceivedHandler = [](Packet, int64_t){};
    };
}