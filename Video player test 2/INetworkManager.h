#pragma once

#include "NetBase2.h"

namespace znet
{
    enum class NetworkStatus
    {
        OFFLINE,
        ONLINE,
        INITIALIZING,
        UNINITIALIZING
    };

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

        virtual std::string Name() = 0;
        virtual NetworkStatus Status() = 0;
        virtual std::wstring StatusString() = 0;

        // UI interface

        // The label a button that closes the manager should have (ex. "Disconnect" or "Stop server")
        virtual std::wstring CloseLabel() const = 0;

    protected:
        std::function<void(Packet, int64_t)> _packetReceivedHandler = [](Packet, int64_t){};
    };
}