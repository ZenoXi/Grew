#include "Network.h"

znet::Network* znet::Network::_instance = nullptr;

znet::Network::Network()
{

}

void znet::Network::Init()
{
    if (!_instance)
    {
        _instance = new Network();
    }
}

znet::Network* znet::Network::Instance()
{
    return _instance;
}

void znet::Network::SetManager(std::unique_ptr<INetworkManager> networkManager)
{
    _networkManager = std::move(networkManager);
    _networkManager->SetOnPacketReceived([&](Packet pack, int64_t userId) { _DistributePacket(std::move(pack), userId); });
}

void znet::Network::StartManager()
{
    if (_networkManager)
        _networkManager->Start();
}

void znet::Network::CloseManager()
{
    _networkManager.reset();
}

std::wstring znet::Network::CloseLabel() const
{
    if (_networkManager)
        return _networkManager->CloseLabel();
    return L"";
}

std::string znet::Network::ManagerName()
{
    if (_networkManager)
        return _networkManager->Name();
    return "";
}

znet::NetworkStatus znet::Network::ManagerStatus()
{
    if (_networkManager)
        return _networkManager->Status();
    return NetworkStatus::OFFLINE;
}

std::wstring znet::Network::ManagerStatusString()
{
    if (_networkManager)
        return _networkManager->StatusString();
    return L"";
}

std::vector<znet::INetworkManager::User> znet::Network::Users(bool includeSelf)
{
    if (_networkManager)
        return _networkManager->Users(includeSelf);
    return {};
}

std::vector<int64_t> znet::Network::UserIds(bool includeSelf)
{
    if (_networkManager)
        return _networkManager->UserIds(includeSelf);
    return {};
}

znet::INetworkManager::User znet::Network::ThisUser()
{
    if (_networkManager)
        return _networkManager->ThisUser();
    return { -1 };
}

int64_t znet::Network::ThisUserId()
{
    if (_networkManager)
        return _networkManager->ThisUserId();
    return -1;
}

void znet::Network::Send(Packet&& packet, std::vector<int64_t> userIds, int priority)
{
    if (_networkManager)
        _networkManager->Send(std::move(packet), std::move(userIds), priority);
}

void znet::Network::AddToQueue(Packet&& packet)
{
    if (_networkManager)
        _networkManager->AddToQueue(std::move(packet));
}

void znet::Network::SendQueue(std::vector<int64_t> userIds, int priority)
{
    if (_networkManager)
        _networkManager->SendQueue(userIds, priority);
}

void znet::Network::AbortSend(int32_t packetId)
{
    if (_networkManager)
        _networkManager->AbortSend(packetId);
}

void znet::Network::_Subscribe(PacketSubscriber* sub)
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

void znet::Network::_Unsubscribe(PacketSubscriber* sub)
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

void znet::Network::_DistributePacket(Packet packet, int64_t userId)
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
