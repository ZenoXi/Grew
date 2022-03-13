#include "NetworkInterfaceNew.h"

// SINGLETON INTERFACE

znet::NetworkInterface* znet::NetworkInterface::_instance = nullptr;

znet::NetworkInterface::NetworkInterface()
{

}

void znet::NetworkInterface::Init()
{
    if (!_instance)
    {
        _instance = new NetworkInterface();
        _instance->_incomingPacketManager = std::thread(&znet::NetworkInterface::_ManageIncomingPackets, _instance);
    }
}

znet::NetworkInterface* znet::NetworkInterface::Instance()
{
    return _instance;
}

// Connection functions

void znet::NetworkInterface::StartServer(USHORT port)
{
    _connectionManager = new znet::ServerConnectionManager(port);
    _mode = NetworkMode::SERVER;
}

void znet::NetworkInterface::StopServer()
{
    std::lock_guard<std::mutex> lock(_m_conMng);
    delete _connectionManager;
    _connectionManager = nullptr;
    _mode = NetworkMode::OFFLINE;
}

void znet::NetworkInterface::Connect(std::string ip, USHORT port)
{
    _connectionManager = new znet::ClientConnectionManager(ip, port);
    _mode = NetworkMode::CLIENT;
}

void znet::NetworkInterface::Disconnect()
{
    std::lock_guard<std::mutex> lock(_m_conMng);
    delete _connectionManager;
    _connectionManager = nullptr;
    _mode = NetworkMode::OFFLINE;
}

void znet::NetworkInterface::_ManageIncomingPackets()
{
    while (!_PACKET_THR_STOP)
    {
        // Safely acquire incoming packet
        std::unique_lock<std::mutex> lock(_m_conMng);
        if (!_connectionManager)
        {
            lock.unlock();
            // Wait for connection
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::pair<Packet, int64_t> packetData;
        if (_connectionManager->PacketCount() > 0)
        {
            packetData = _connectionManager->GetPacket();
        }
        else
        {
            lock.unlock();
            // Wait for packets
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        lock.unlock();

        _DistributePacket(std::move(packetData.first), packetData.second);
    }
}

// Sending functions

std::vector<znet::IConnectionManager::User> znet::NetworkInterface::Users()
{
    if (_connectionManager)
        return _connectionManager->Users();
    else
        return std::vector<znet::IConnectionManager::User>();
}

znet::IConnectionManager::User znet::NetworkInterface::ThisUser()
{
    if (_connectionManager)
        return _connectionManager->ThisUser();
    else
        return { L"", -1 };
}

void znet::NetworkInterface::SetUsername(std::wstring username)
{
    _username = username;
    if (_connectionManager)
        _connectionManager->SetUsername(username);
}

void znet::NetworkInterface::Send(Packet&& packet, std::vector<int64_t> userIds, int priority)
{
    if (_connectionManager)
    {
        // Immediatelly distribute self-addressed packets
        // This avoids unnecessary latency of packet processing threads
        for (int j = 0; j < userIds.size(); j++)
        {
            if (userIds[j] == ThisUser().id)
            {
                _DistributePacket(packet.Reference(), userIds[j]);
                userIds.erase(userIds.begin() + j);
                break;
            }
        }
        if (userIds.empty())
            return;

        _connectionManager->Send(std::move(packet), userIds, priority);
    }
}

void znet::NetworkInterface::AddToQueue(Packet&& packet, std::vector<int64_t> userIds)
{
    if (_connectionManager)
        _connectionManager->AddToQueue(std::move(packet), userIds);
}

void znet::NetworkInterface::SendQueue(std::vector<int64_t> userIds, int priority)
{
    if (_connectionManager)
        _connectionManager->SendQueue(userIds, priority);
}

void znet::NetworkInterface::AbortSend(int32_t packetId)
{
    if (_connectionManager)
        _connectionManager->AbortSend(packetId);
}

// Packet subscription

void znet::NetworkInterface::_Subscribe(PacketSubscriber* sub)
{
    std::unique_lock<std::mutex> lock(_m_subscriptions);
    for (auto subscription = _subscriptions.begin(); subscription != _subscriptions.end(); subscription++)
    {
        if (subscription->packetId == sub->_packetId)
        {
            auto subscriber = std::find(subscription->subs.begin(), subscription->subs.end(), sub);
            if (subscriber == subscription->subs.end())
            {
                subscription->subs.push_back(sub);
            }
            return;
        }
    }
    _subscriptions.push_back({ sub->_packetId, { sub } });
}

void znet::NetworkInterface::_Unsubscribe(PacketSubscriber* sub)
{
    std::unique_lock<std::mutex> lock(_m_subscriptions);
    for (auto subscription = _subscriptions.begin(); subscription != _subscriptions.end(); subscription++)
    {
        if (subscription->packetId == sub->_packetId)
        {
            auto subscriber = std::find(subscription->subs.begin(), subscription->subs.end(), sub);
            if (subscriber != subscription->subs.end())
            {
                subscription->subs.erase(subscriber);
                if (subscription->subs.empty())
                {
                    _subscriptions.erase(subscription);
                }
            }
            return;
        }
    }
}

void znet::NetworkInterface::_DistributePacket(Packet packet, int64_t userId)
{
    std::unique_lock<std::mutex> lock(_m_subscriptions);
    for (auto& subscription : _subscriptions)
    {
        if (subscription.packetId == packet.id)
        {
            for (int i = 0; i < subscription.subs.size(); i++)
            {
                if (i != subscription.subs.size() - 1)
                    subscription.subs[i]->_OnPacketReceived(packet.Reference(), userId);
                else
                    subscription.subs[i]->_OnPacketReceived(std::move(packet), userId);
            }
            return;
        }
    }
}

// Network status

znet::NetworkMode znet::NetworkInterface::Mode()
{
    return _mode;
}

std::string znet::NetworkInterface::StatusString()
{
    if (_connectionManager)
        return _connectionManager->StatusString();
    else
        return "";
}

znet::ConnectionStatus znet::NetworkInterface::Status()
{
    if (_connectionManager)
        return _connectionManager->Status();
    else
        return znet::ConnectionStatus::UNKNOWN;
}