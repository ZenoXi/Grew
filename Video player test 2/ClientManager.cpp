#include "ClientManager.h"

#include "App.h"
#include "NetworkEvents.h"
#include "PacketBuilder.h"

znet::ClientManager::ClientManager(std::string ip, uint16_t port, std::string password)
{
    _connecting = true;
    _connectionId = -1;
    _password = password;
    _connectionThread = std::thread(&ClientManager::_Connect, this, ip, port);
}

znet::ClientManager::~ClientManager()
{
    _CONN_THR_STOP = true;
    _MANAGEMENT_THR_STOP = true;

    if (_connectionThread.joinable())
        _connectionThread.join();
    if (_managementThread.joinable())
        _managementThread.join();
}

void znet::ClientManager::Start()
{
    _managementThread = std::thread(&ClientManager::_ManageConnections, this);
}

void znet::ClientManager::_Connect(std::string ip, uint16_t port)
{
    App::Instance()->events.RaiseEvent(ConnectionStartEvent{ ip, port });

    // Connect the socket
    _client.ConnectAsync(ip, port);
    while (!_client.ConnectFinished())
    {
        if (_CONN_THR_STOP)
        {
            _connectFailed = true;
            _failMessage = L"Connect aborted";
            _failCode = -1;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Aborted" });
            _connecting = false;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    _connectionId = _client.ConnectResult();

    if (_connectionId != -1)
    {
        TCPClientRef connection = _client.Connection(_connectionId);

        // Send password with max priority because it must go first
        if (!_password.empty())
        {
            PacketBuilder builder;
            builder.Add(_password.data(), _password.length());
            connection->Send(Packet(builder.Release(), builder.UsedBytes(), (int)PacketType::PASSWORD), std::numeric_limits<int>::max());
        }
        else
        {
            connection->Send(Packet((int)PacketType::PASSWORD), std::numeric_limits<int>::max());
        }

        // Wait for assigned id to arrive
        while (connection->PacketCount() == 0)
        {
            if (_CONN_THR_STOP)
            {
                _connectFailed = true;
                _failMessage = L"Connect aborted";
                _failCode = -1;
                App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Aborted" });
                _connecting = false;
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Packet idPacket = connection->GetPacket();
        if (idPacket.id != (int)PacketType::ASSIGNED_USER_ID)
        {
            // TODO: correctly disconnect
            connection->Disconnect();
            _connectionId = -1;
            _connectFailed = true;
            _failMessage = L"Connection protocol violated by the server";
            _failCode = -1;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Connection protocol violation" });
            _connecting = false;
            return;
        }

        int64_t assignedId = idPacket.Cast<int64_t>();
        if (assignedId == -1)
        {
            connection->Disconnect();
            _connectFailed = true;
            _failMessage = L"Connection denied";
            _failCode = -1;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Denied" });
            _connecting = false;
            return;
        }
        else if (assignedId == -2)
        {
            connection->Disconnect();
            _connectFailed = true;
            _passwordRequired = true;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Incorrect password" });
            _connecting = false;
            return;
        }
        else if (assignedId == -3)
        {
            connection->Disconnect();
            _connectFailed = true;
            _failMessage = L"Server is full";
            _failCode = -1;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Server full" });
            _connecting = false;
            return;
        }
        else
        {
            _thisUser.id = assignedId;
            _online = true;
        }

        App::Instance()->events.RaiseEvent(ConnectionSuccessEvent{});
        App::Instance()->events.RaiseEvent(NetworkModeChangedEvent{ NetworkMode::CLIENT });
    }
    else
    {
        App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "" });
    }

    _connecting = false;
}

std::vector<znet::ClientManager::User> znet::ClientManager::Users(bool includeSelf)
{
    std::lock_guard<std::mutex> lock(_m_users);
    if (includeSelf)
    {
        auto users = _users;
        users.push_back(_thisUser);
        return users;
    }
    return _users;
}

std::vector<int64_t> znet::ClientManager::UserIds(bool includeSelf)
{
    std::lock_guard<std::mutex> lock(_m_users);
    std::vector<int64_t> userIds;
    for (auto& user : _users)
        userIds.push_back(user.id);
    if (includeSelf)
        userIds.push_back(_thisUser.id);
    return userIds;
}

znet::ClientManager::User znet::ClientManager::ThisUser()
{
    return _thisUser;
}

int64_t znet::ClientManager::ThisUserId()
{
    return _thisUser.id;
}

void znet::ClientManager::Send(Packet&& packet, std::vector<int64_t> userIds, int priority)
{
    // Immediatelly distribute self-addressed packets
    // This avoids unnecessary latency of packet processing threads
    for (int i = 0; i < userIds.size(); i++)
    {
        if (userIds[i] == _thisUser.id)
        {
            _packetReceivedHandler(packet.Reference(), userIds[i]);
            userIds.erase(userIds.begin() + i);
            break;
        }
    }
    if (userIds.empty())
        return;

    // Split large packets
    size_t splitThreshold = 50000; // Bytes
    std::vector<_PacketData> packets;
    if (packet.size > splitThreshold)
    {
        size_t partCount = packet.size / splitThreshold;
        if (packet.size % splitThreshold != 0)
            partCount++;
        int64_t splitId = _splitIdCounter++;

        // Create head packet
        struct SplitHead
        {
            int64_t splitId;
            int32_t packetId;
            int32_t partCount;
            size_t packetSize;
        };
        packets.push_back({ Packet((int)PacketType::SPLIT_PACKET_HEAD).From(SplitHead{ splitId, packet.id, (int32_t)partCount, packet.size }), userIds });

        // Create part packets
        size_t bytesUsed = 0;
        size_t bytesLeft = packet.size;
        while (bytesLeft > 0)
        {
            size_t dataSize = splitThreshold;
            if (bytesLeft < splitThreshold)
                dataSize = bytesLeft;

            size_t packetSize = sizeof(int64_t) + sizeof(int32_t) + dataSize;
            auto bytes = std::make_unique<int8_t[]>(packetSize);
            *(int64_t*)bytes.get() = splitId;
            *(int32_t*)(bytes.get() + sizeof(int64_t)) = packet.id;
            std::copy_n(packet.Bytes() + bytesUsed, dataSize, bytes.get() + sizeof(int64_t) + sizeof(int32_t));
            packets.push_back({ Packet(std::move(bytes), packetSize, (int)PacketType::SPLIT_PACKET_PART), userIds });
            bytesUsed += dataSize;
            bytesLeft -= dataSize;
        }
    }
    else
    {
        packets.push_back({ std::move(packet), userIds });
    }

    LOCK_GUARD(lock, _m_outPackets);
    _AddPacketsToOutQueue(std::move(packets), priority);
}

void znet::ClientManager::_AddPacketsToOutQueue(std::vector<_PacketData> packets, int priority)
{
    if (priority != 0)
    {
        for (int i = 0; i < _outPackets.size(); i++)
        {
            if (_outPackets[i].second < priority)
            {
                for (int j = 0; j < packets.size(); j++)
                {
                    _outPackets.insert(_outPackets.begin() + i, { std::move(packets[j]), priority });
                    i++;
                }
                return;
            }
        }
    }
    for (int j = 0; j < packets.size(); j++)
    {
        _outPackets.push_back({ std::move(packets[j]), priority });
    }
}

void znet::ClientManager::_AddPacketToOutQueue(_PacketData packet, int priority)
{
    if (priority != 0)
    {
        for (int i = 0; i < _outPackets.size(); i++)
        {
            if (_outPackets[i].second < priority)
            {
                _outPackets.insert(_outPackets.begin() + i, { std::move(packet), priority });
                return;
            }
        }
    }
    _outPackets.push_back({ std::move(packet), priority });
}

void znet::ClientManager::AddToQueue(Packet&& packet)
{
    _PacketData data = { std::move(packet) };
    _packetQueue.push_back(std::move(data));
}

void znet::ClientManager::SendQueue(std::vector<int64_t> userIds, int priority)
{
    LOCK_GUARD(lock, _m_outPackets);

    // Immediatelly distribute self-addressed packets
    // This avoids unnecessary latency of packet processing threads
    for (int i = 0; i < userIds.size(); i++)
    {
        if (userIds[i] == _thisUser.id)
        {
            for (auto& packetData : _packetQueue)
                _packetReceivedHandler(packetData.packet.Reference(), userIds[i]);
            userIds.erase(userIds.begin() + i);
            break;
        }
    }
    if (userIds.empty())
    {
        _packetQueue.clear();
        return;
    }

    // Send packets
    if (priority != 0)
    {
        for (int i = 0; i < _outPackets.size(); i++)
        {
            if (_outPackets[i].second < priority)
            {
                while (!_packetQueue.empty())
                {
                    _packetQueue.front().userIds = userIds;
                    _outPackets.insert(_outPackets.begin() + i, { std::move(_packetQueue.front()), priority });
                    _packetQueue.pop_front();
                    i++;
                }
                return;
            }
        }
    }
    while (!_packetQueue.empty())
    {
        _packetQueue.front().userIds = userIds;
        _outPackets.push_back({ std::move(_packetQueue.front()), priority });
        _packetQueue.pop_front();
    }
}

void znet::ClientManager::AbortSend(int32_t packetId)
{
    LOCK_GUARD(lock, _m_outPackets);

    for (int i = 0; i < _outPackets.size(); i++)
    {
        // Check split packets
        if (_outPackets[i].first.packet.id == (int32_t)PacketType::SPLIT_PACKET_HEAD)
        {
            // Extract packet id
            struct SplitHead
            {
                int64_t splitId;
                int32_t packetId;
                int32_t partCount;
                size_t packetSize;
            };
            auto headData = _outPackets[i].first.packet.Cast<SplitHead>();

            // Inform recipients of packet send abort
            // 'Send()' isn't used because it tries to lock _m_outPackets again
            _PacketData packetData{ Packet((int32_t)PacketType::SPLIT_PACKET_ABORT).From(headData.splitId), _outPackets[i].first.userIds };
            _AddPacketToOutQueue(std::move(packetData), _outPackets[i].second);

            if (headData.packetId == packetId)
            {
                _outPackets.erase(_outPackets.begin() + i, _outPackets.begin() + i + headData.partCount + 1);
                i--;
            }

        }
        else if (_outPackets[i].first.packet.id == (int32_t)PacketType::SPLIT_PACKET_PART)
        {
            // Extract split id
            int64_t splitId = _outPackets[i].first.packet.Cast<int64_t>();

            // Inform recipients of packet send abort
            // 'Send()' isn't used because it tries to lock _m_outPackets again
            _PacketData packetData{ Packet((int32_t)PacketType::SPLIT_PACKET_ABORT).From(splitId), _outPackets[i].first.userIds };
            _AddPacketToOutQueue(std::move(packetData), _outPackets[i].second);

            while (true)
            {
                _outPackets.erase(_outPackets.begin() + i);

                if (_outPackets.size() == i)
                    break;
                if (_outPackets[i].first.packet.id != (int32_t)PacketType::SPLIT_PACKET_PART)
                    break;
                if (_outPackets[i].first.packet.Cast<int64_t>() != splitId)
                    break;
            }

            i--;
        }
        else if (_outPackets[i].first.packet.id == packetId)
        {
            _outPackets.erase(_outPackets.begin() + i);
            i--;
        }
    }
}

void znet::ClientManager::_ManageConnections()
{
    TCPClientRef connection = _client.Connection(_connectionId);

    _ManagerThreadData data{ connection };
    data.packetLatencies = FixedQueue<Duration>(16);
    data.packetLatencies.Fill(Duration(0));

    while (!_MANAGEMENT_THR_STOP)
    {
        if (_SendLatencyProbePackets(data))
            break;
        if (_CheckIfConnectionExists(data))
            break;
        if (_ProcessIncomingPackets(data))
            break;
        if (_ProcessOutgoingPackets(data))
            break;

        _PrintNetworkStats(data);

        // Sleep if no immediate work needs to be done
        if (connection->PacketCount() == 0 && _outPackets.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (connection->Connected())
    {
        connection->Disconnect();
    }
    _online = false;
    App::Instance()->events.RaiseEvent(DisconnectEvent{});
    App::Instance()->events.RaiseEvent(NetworkModeChangedEvent{ NetworkMode::OFFLINE });
}

bool znet::ClientManager::_SendLatencyProbePackets(_ManagerThreadData& data)
{
    if (!data.latencyPacketInTransmission)
    {
        if (ztime::Main() - data.latencyPacketReceiveTime >= data.latencyPacketInterval)
        {
            data.connection->Send(znet::Packet((int)znet::PacketType::LATENCY_PROBE).From(0), 1024 /* Arbitrarily large priority */);
            data.latencyPacketInTransmission = true;
            data.latencyPacketSendTime = ztime::Main();
        }
    }

    return false;
}

bool znet::ClientManager::_CheckIfConnectionExists(_ManagerThreadData& data)
{
    if (!data.connection->Connected())
    {
        _connectionId = -1;
        _connectionLost = true;
        App::Instance()->events.RaiseEvent(ConnectionLostEvent{});
        return true;
    }
    if ((ztime::Main() - data.lastKeepAliveReceiveTime).GetDuration(MILLISECONDS) > 5000)
    {
        data.connection->Disconnect();
        _connectionId = -1;
        _connectionLost = true;
        std::cout << "Keep alive packet not received within 5 seconds, closing connection.." << std::endl;
        App::Instance()->events.RaiseEvent(ConnectionLostEvent{});
        return true;
    }
    return false;
}

bool znet::ClientManager::_ProcessIncomingPackets(_ManagerThreadData& data)
{
    if (data.connection->PacketCount() > 0)
    {
        Packet pack1 = data.connection->GetPacket();
        if (pack1.id == (int32_t)PacketType::BYTE_CONFIRMATION)
        {
            data.bytesUnconfirmed -= pack1.Cast<size_t>();
            // Increment connection speed counter
            data.bytesSentSinceLastPrint += pack1.Cast<size_t>();
            // Calculate latency
            data.threadTimer.Update();
            TimePoint returnTime = data.threadTimer.Now();
            TimePoint sendTime = data.packetsInTransmittion.front();
            data.packetsInTransmittion.pop();
        }
        else if (pack1.id == (int32_t)PacketType::KEEP_ALIVE)
        {
            data.lastKeepAliveReceiveTime = ztime::Main();
        }
        else if (pack1.id == (int32_t)PacketType::LATENCY_PROBE)
        {
            data.latencyPacketReceiveTime = ztime::Main();
            data.latencyPacketInTransmission = false;
            data.packetLatencies.Push(data.latencyPacketReceiveTime - data.latencyPacketSendTime);
        }
        else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
        {
            data.connection->Disconnect();
            _connectionId = -1;
            _disconnected = true;
            App::Instance()->events.RaiseEvent(ConnectionClosedEvent{});
            return true;
        }
        else if (pack1.id == (int32_t)PacketType::NEW_USER)
        {
            int64_t newUserId = pack1.Cast<int64_t>();
            App::Instance()->events.RaiseEvent(UserConnectedEvent{ newUserId });

            std::lock_guard<std::mutex> lock(_m_users);
            _users.push_back({ newUserId });
        }
        else if (pack1.id == (int32_t)PacketType::DISCONNECTED_USER)
        {
            int64_t disconnectedUserId = pack1.Cast<int64_t>();
            App::Instance()->events.RaiseEvent(UserDisconnectedEvent{ disconnectedUserId });

            std::lock_guard<std::mutex> lock(_m_users);
            for (int i = 0; i < _users.size(); i++)
            {
                if (_users[i].id == disconnectedUserId)
                {
                    _users.erase(_users.begin() + i);
                    break;
                }
            }
        }
        else if (pack1.id == (int32_t)PacketType::USER_LIST)
        {
            PacketReader reader = PacketReader(pack1.Bytes(), pack1.size);
            std::lock_guard<std::mutex> lock(_m_users);
            _users.clear();
            _users.reserve(reader.RemainingBytes() / sizeof(int64_t));
            while (reader.RemainingBytes() > 0)
                _users.push_back({ reader.Get<int64_t>() });
        }
        else if (pack1.id == (int32_t)PacketType::USER_ID)
        {
            if (data.connection->PacketCount() > 0)
            {
                Packet pack2 = data.connection->GetPacket();

                // Increment connection speed counter
                data.bytesReceivedSinceLastPrint += pack2.size;

                // Send confirmation packet
                data.connection->Send(Packet((int)PacketType::BYTE_CONFIRMATION).From(pack2.size), 1);

                // Process split packets
                if (pack2.id == (int)PacketType::SPLIT_PACKET_HEAD)
                {
                    struct SplitHead
                    {
                        int64_t splitId;
                        int32_t packetId;
                        int32_t partCount;
                        size_t packetSize;
                    };
                    auto headData = pack2.Cast<SplitHead>();
                    data.splitPackets.push_back({ headData.splitId, headData.packetId, headData.partCount, headData.packetSize });
                }
                else if (pack2.id == (int)PacketType::SPLIT_PACKET_PART)
                {
                    int64_t splitId = pack2.Cast<int64_t>();
                    int index = -1;
                    for (int i = 0; i < data.splitPackets.size(); i++)
                    {
                        if (data.splitPackets[i].splitId == splitId)
                        {
                            data.splitPackets[i].parts.push_back(pack2.Reference());
                            index = i;
                            break;
                        }
                    }

                    // Check if all parts received
                    if (index != -1)
                    {
                        if (data.splitPackets[index].parts.size() == data.splitPackets[index].partCount)
                        {
                            // Combine
                            auto bytes = std::make_unique<int8_t[]>(data.splitPackets[index].packetSize);
                            size_t bytePos = 0;
                            size_t partDataOffset = sizeof(int64_t) + sizeof(int32_t);
                            for (auto& part : data.splitPackets[index].parts)
                            {
                                size_t byteCount = part.size - partDataOffset;
                                std::copy_n(part.Bytes() + partDataOffset, byteCount, bytes.get() + bytePos);
                                bytePos += byteCount;
                            }
                            Packet combined = Packet(std::move(bytes), data.splitPackets[index].packetSize, data.splitPackets[index].packetId);
                            data.splitPackets.erase(data.splitPackets.begin() + index);

                            // Pass to handler
                            _packetReceivedHandler(std::move(combined), pack1.Cast<int64_t>());
                        }
                    }
                }
                else if (pack2.id == (int)PacketType::SPLIT_PACKET_ABORT)
                {
                    int64_t splitId = pack2.Cast<int64_t>();
                    for (int i = 0; i < data.splitPackets.size(); i++)
                    {
                        if (data.splitPackets[i].splitId == splitId)
                        {
                            data.splitPackets.erase(data.splitPackets.begin() + i);
                            break;
                        }
                    }
                }
                else
                {
                    // Pass to handler
                    _packetReceivedHandler(std::move(pack2), pack1.Cast<int64_t>());
                }
            }
            else
            {
                std::cout << "[WARN] User id packet received without accompanying data packet" << std::endl;
            }
        }
    }

    return false;
}

bool znet::ClientManager::_ProcessOutgoingPackets(_ManagerThreadData& data)
{
    std::unique_lock<std::mutex> lock(_m_outPackets);
    if (_outPackets.empty())
        return false;

    for (int i = 0; i < _outPackets.size(); i++)
    {
        // Erase packet if no destination is specified
        if (_outPackets[i].first.userIds.empty())
        {
            _outPackets.erase(_outPackets.begin() + i);
            i--;
            continue;
        }

        PacketView view = _outPackets[i].first.packet.View();

        // Heavy packets will be postponed until unconfirmed byte count is below a certain value
        bool heavyPacket = (
            view.id == (int32_t)PacketType::SPLIT_PACKET_PART ||
            view.id == (int32_t)PacketType::VIDEO_PACKET ||
            view.id == (int32_t)PacketType::AUDIO_PACKET ||
            view.id == (int32_t)PacketType::SUBTITLE_PACKET ||
            view.id == (int32_t)PacketType::VIDEO_STREAM ||
            view.id == (int32_t)PacketType::AUDIO_STREAM ||
            view.id == (int32_t)PacketType::SUBTITLE_STREAM ||
            view.id == (int32_t)PacketType::ATTACHMENT_STREAM ||
            view.id == (int32_t)PacketType::DATA_STREAM ||
            view.id == (int32_t)PacketType::UNKNOWN_STREAM
            );

        if (heavyPacket && data.bytesUnconfirmed > data.maxUnconfirmedBytes)
            continue;

        _PacketData pdata = std::move(_outPackets[i].first);
        int priority = _outPackets[i].second;
        _outPackets.erase(_outPackets.begin() + i);
        lock.unlock();

        data.bytesUnconfirmed += pdata.packet.size;
        data.threadTimer.Update();
        data.packetsInTransmittion.push(data.threadTimer.Now());

        // Some packet types don't need a prefix
        bool prefixed = !(
            view.id == (int32_t)PacketType::USER_DATA
            );


        // Construct packets
        if (prefixed)
        {
            PacketBuilder builder = PacketBuilder(pdata.userIds.size() * sizeof(int64_t));
            builder.Add(pdata.userIds.data(), pdata.userIds.size());
            data.connection->AddToQueue(Packet(builder.Release(), builder.UsedBytes(), (int)PacketType::USER_ID));
        }
        data.connection->AddToQueue(std::move(pdata.packet));
        data.connection->SendQueue(priority);

        break;
    }

    return false;
}

void znet::ClientManager::_PrintNetworkStats(_ManagerThreadData& data)
{
    Duration timeElapsed = ztime::Main() - data.lastPrintTime;
    if (timeElapsed >= data.printInterval)
    {
        int64_t latency = 0;
        for (int i = 0; i < data.packetLatencies.Size(); i++)
            latency += data.packetLatencies[i].GetDuration();
        latency /= data.packetLatencies.Size();

        App::Instance()->events.RaiseEvent(NetworkStatsEvent{ timeElapsed, (int64_t)data.bytesSentSinceLastPrint, (int64_t)data.bytesReceivedSinceLastPrint, latency });

        data.lastPrintTime = ztime::Main();
        data.bytesSentSinceLastPrint = 0;
        data.bytesReceivedSinceLastPrint = 0;
    }
}