#pragma once

#include "NetBase2.h"
#include "PacketTypes.h"

namespace znet
{
    class PacketSubscriber
    {
        friend class NetworkInterface; // TODO: remove this line
        friend class Network;

        int32_t _packetId = -1;
        virtual void _OnPacketReceived(Packet packet, int64_t userId) = 0;
    public:
        PacketSubscriber(int32_t packetId);
        PacketSubscriber(const PacketSubscriber&) = delete;
        virtual ~PacketSubscriber();
    };

    class PacketReceiver : public PacketSubscriber
    {
        std::queue<std::pair<Packet, int64_t>> _packets;
        std::mutex _m_packets;

        void _OnPacketReceived(Packet packet, int64_t userId)
        {
            std::lock_guard<std::mutex> lock(_m_packets);
            _packets.push({ std::move(packet), userId });
        }
    public:
        PacketReceiver(PacketType packetType) : PacketSubscriber((int32_t)packetType) {}
        size_t PacketCount()
        {
            std::lock_guard<std::mutex> lock(_m_packets);
            return _packets.size();
        }
        std::pair<Packet, int64_t> GetPacket()
        {
            std::lock_guard<std::mutex> lock(_m_packets);
            std::pair<Packet, int64_t> packet = std::move(_packets.front());
            _packets.pop();
            return packet;
        }
    };
}