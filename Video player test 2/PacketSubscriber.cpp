#include "Network.h"
#include "PacketSubscriber.h"

znet::PacketSubscriber::PacketSubscriber(int32_t packetId)
{
    _packetId = packetId;
    APP_NETWORK->_Subscribe(this);
}

znet::PacketSubscriber::~PacketSubscriber()
{
    APP_NETWORK->_Unsubscribe(this);
}