#pragma once

#include "NetBase2.h"

namespace znet
{
    class PacketSubscriber
    {
        friend class NetworkInterface;

        int32_t _packetId = -1;
        virtual void _OnPacketReceived(Packet packet, int64_t userId) = 0;
    public:
        PacketSubscriber(int32_t packetId);
        virtual ~PacketSubscriber();
    };
}