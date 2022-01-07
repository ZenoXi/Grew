#pragma once

#include "NetBase2.h"

namespace znet
{
    enum class ConnectionStatus
    {
        UNKNOWN,
        CONNECTING,
        CONNECTED,
        DISCONNECTING,
        DISCONNECTED,
        CONNECTION_LOST,
        RUNNING,
        OFFLINE
    };

    class IConnectionManager
    {
    public:
        struct User
        {
            std::string name;
            int64_t id;
        };

        virtual std::vector<User> Users() = 0;
        virtual void Send(Packet&& packet, std::vector<int64_t> userIds, int priority = 0) = 0;
        virtual void AddToQueue(Packet&& packet, std::vector<int64_t> userIds) = 0;
        virtual void SendQueue(std::vector<int64_t> userIds, int priority = 0) = 0;
        virtual void AbortSend(int32_t packetId) = 0;
        virtual size_t PacketCount() const = 0;
        virtual std::pair<Packet, int64_t> GetPacket() = 0;

        virtual std::string StatusString() = 0;
        virtual ConnectionStatus Status() = 0;
    };
}