#include "NetworkInterfaceNew.h"
#include "PacketSubscriber.h"

znet::PacketSubscriber::PacketSubscriber(int32_t packetId)
{
    _packetId = packetId;
    NetworkInterface::Instance()->_Subscribe(this);
}

znet::PacketSubscriber::~PacketSubscriber()
{
    NetworkInterface::Instance()->_Unsubscribe(this);
}