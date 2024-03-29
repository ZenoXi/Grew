#include "ClientConnectionManager.h"

#include "App.h"
#include "NetworkEvents.h"

#include "GameTime.h"

znet::ClientConnectionManager::ClientConnectionManager(std::string ip, uint16_t port)
{
    _connecting = true;
    _connectionId = -1;
    _connectionThread = std::thread(&ClientConnectionManager::_Connect, this, ip, port);
}

znet::ClientConnectionManager::~ClientConnectionManager()
{
    _CONN_THR_STOP = true;
    _MANAGEMENT_THR_STOP = true;

    if (_connectionThread.joinable())
        _connectionThread.join();
    if (_managementThread.joinable())
        _managementThread.join();
}

// CONNECTION

bool znet::ClientConnectionManager::Connecting() const
{
    return _connecting;
}

bool znet::ClientConnectionManager::ConnectionSuccess() const
{
    return !_connecting && _connectionId > 0;
}

std::string znet::ClientConnectionManager::StatusString()
{
    switch (Status())
    {
    case ConnectionStatus::CONNECTED:
        return "Connected!";
    case ConnectionStatus::CONNECTING:
        return "Connecting..";
    case ConnectionStatus::DISCONNECTED:
        return "Disconnected.";
    case ConnectionStatus::DISCONNECTING:
        return "Disconnecting..";
    case ConnectionStatus::CONNECTION_LOST:
        return "Connection lost..";
    default:
        return "";
    }
}

znet::ConnectionStatus znet::ClientConnectionManager::Status()
{
    if (_connecting)
        return ConnectionStatus::CONNECTING;
    if (_connectionId != -1)
        return ConnectionStatus::CONNECTED;
    if (_disconnecting)
        return ConnectionStatus::DISCONNECTING;
    if (_disconnected)
        return ConnectionStatus::DISCONNECTED;
    if (_connectionLost)
        return ConnectionStatus::CONNECTION_LOST;

    return ConnectionStatus::UNKNOWN;
}

void znet::ClientConnectionManager::_Connect(std::string ip, uint16_t port)
{
    App::Instance()->events.RaiseEvent(ConnectionStartEvent{ ip, port });

    // Connect the socket
    _client.ConnectAsync(ip, port);
    while (!_client.ConnectFinished())
    {
        if (_CONN_THR_STOP)
        {
            _connecting = false;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Aborted" });
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    _connectionId = _client.ConnectResult();

    if (_connectionId != -1)
    {
        // Wait for assigned id to arrive
        TCPClientRef connection = _client.Connection(_connectionId);
        while (connection->PacketCount() == 0)
        {
            if (_CONN_THR_STOP)
            {
                _connecting = false;
                App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Aborted" });
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Packet idPacket = connection->GetPacket();
        if (idPacket.id != (int)PacketType::ASSIGNED_USER_ID)
        {
            _connecting = false;
            // TODO: correctly disconnect
            connection->Disconnect();
            _connectionId = -1;
            App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "Connection protocol violation" });
            return;
        }
        _thisUser.id = idPacket.Cast<int64_t>();

        // Send self username
        if (!_thisUser.name.empty())
        {
            size_t byteCount = sizeof(int64_t) + sizeof(wchar_t) * _thisUser.name.length();
            auto usernameBytes = std::make_unique<int8_t[]>(byteCount);
            ((int64_t*)usernameBytes.get())[0] = 0;
            for (int j = 0; j < _thisUser.name.length(); j++)
            {
                ((wchar_t*)(usernameBytes.get() + sizeof(int64_t)))[j] = _thisUser.name[j];
            }
            connection->Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), 2);
        }

        // Start management thread
        _managementThread = std::thread(&ClientConnectionManager::_ManageConnections, this);

        App::Instance()->events.RaiseEvent(ConnectionSuccessEvent{});
        App::Instance()->events.RaiseEvent(NetworkModeChangedEvent{ NetworkMode::CLIENT });
    }
    else
    {
        App::Instance()->events.RaiseEvent(ConnectionFailEvent{ "" });
    }

    _connecting = false;
}

std::vector<znet::ClientConnectionManager::User> znet::ClientConnectionManager::Users()
{
    std::lock_guard<std::mutex> lock(_m_users);
    return _users;
}

znet::ClientConnectionManager::User znet::ClientConnectionManager::ThisUser()
{
    return _thisUser;
}

void znet::ClientConnectionManager::SetUsername(std::wstring username)
{
    _thisUser.name = username;

    // Send new username
    size_t byteCount = sizeof(int64_t) + sizeof(wchar_t) * _thisUser.name.length();
    auto usernameBytes = std::make_unique<int8_t[]>(byteCount);
    ((int64_t*)usernameBytes.get())[0] = 0;
    for (int j = 0; j < _thisUser.name.length(); j++)
    {
        ((wchar_t*)(usernameBytes.get() + sizeof(int64_t)))[j] = _thisUser.name[j];
    }
    std::cout << "Username change sent" << std::endl;
    Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), { 0 }, 2);
}

// PACKET INPUT/OUTPUT

void znet::ClientConnectionManager::Send(Packet&& packet, std::vector<int64_t> userIds, int priority)
{
    // Split large packets
    size_t splitThreshold = 50000; // Bytes
    std::vector<Packet> packets;
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
        packets.push_back(Packet((int)PacketType::SPLIT_PACKET_HEAD).From(SplitHead{ splitId, packet.id, (int32_t)partCount, packet.size }));

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
            packets.push_back(Packet(std::move(bytes), packetSize, (int)PacketType::SPLIT_PACKET_PART));
            bytesUsed += dataSize;
            bytesLeft -= dataSize;
        }
    }
    else
    {
        packets.push_back(std::move(packet));
    }

    // Send packets
    std::lock_guard<std::mutex> lock(_m_outPackets);
    if (priority != 0)
    {
        for (int i = 0; i < _outPackets.size(); i++)
        {
            if (_outPackets[i].second < priority)
            {
                for (int j = 0; j < packets.size(); j++)
                {
                    _PacketData data = { std::move(packets[j]), userIds };
                    _outPackets.insert(_outPackets.begin() + i, { std::move(data), priority });
                    i++;
                }
                return;
            }
        }
    }
    for (int j = 0; j < packets.size(); j++)
    {
        _PacketData data = { std::move(packets[j]), userIds };
        _outPackets.push_back({ std::move(data), priority });
    }
    //std::lock_guard<std::mutex> lock(_m_outPackets);
    //if (priority != 0)
    //{
    //    for (int i = 0; i < _outPackets.size(); i++)
    //    {
    //        if (_outPackets[i].second < priority)
    //        {
    //            _PacketData data = { std::move(packet), std::move(userIds) };
    //            _outPackets.insert(_outPackets.begin() + i, { std::move(data), priority });
    //            return;
    //        }
    //    }
    //}
    //_PacketData data = { std::move(packet), std::move(userIds) };
    //_outPackets.push_back({ std::move(data), priority });
}

void znet::ClientConnectionManager::AddToQueue(Packet&& packet, std::vector<int64_t> userIds)
{
    _PacketData data = { std::move(packet), std::move(userIds) };
    _packetQueue.push(std::move(data));
}

void znet::ClientConnectionManager::SendQueue(std::vector<int64_t> userIds, int priority)
{
    LOCK_GUARD(lock, _m_outPackets);
    if (priority != 0)
    {
        for (int i = 0; i < _outPackets.size(); i++)
        {
            if (_outPackets[i].second < priority)
            {
                while (!_packetQueue.empty())
                {
                    _outPackets.insert(_outPackets.begin() + i, { std::move(_packetQueue.front()), priority });
                    _packetQueue.pop();
                    i++;
                }
                return;
            }
        }
    }
    while (!_packetQueue.empty())
    {
        _outPackets.push_back({ std::move(_packetQueue.front()), priority });
        _packetQueue.pop();
    }
}

void znet::ClientConnectionManager::AbortSend(int32_t packetId)
{
    std::unique_lock<std::mutex> lock(_m_outPackets);
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

size_t znet::ClientConnectionManager::PacketCount() const
{
    return _inPackets.size();
}

std::pair<znet::Packet, int64_t> znet::ClientConnectionManager::GetPacket()
{
    std::unique_lock<std::mutex> lock(_m_inPackets);
    _PacketData data = std::move(_inPackets.front());
    _inPackets.pop();
    return { std::move(data.packet), data.userIds[0] };
}

// CONNECTION MANAGEMENT

void znet::ClientConnectionManager::_ManageConnections()
{
    TCPClientRef connection = _client.Connection(_connectionId);

    size_t bytesUnconfirmed = 0;
    struct SplitPacket
    {
        int64_t splitId;
        int32_t packetId;
        int32_t partCount;
        size_t packetSize;
        std::vector<Packet> parts;
    };
    std::vector<SplitPacket> splitPackets;
    size_t maxUnconfirmedBytes = 250000; // 1 MB

    Clock threadTimer = Clock();
    std::queue<TimePoint> packetsInTransmittion;
    FixedQueue<Duration> packetLatencies(16);
    packetLatencies.Fill(Duration(0));

    size_t bytesSentSinceLastPrint = 0;
    size_t bytesReceivedSinceLastPrint = 0;
    TimePoint lastPrintTime = ztime::Main();
    Duration printInterval = Duration(3, SECONDS);
    bool printSpeed = true;

    TimePoint lastKeepAliveReceiveTime = ztime::Main();

    while (!_MANAGEMENT_THR_STOP)
    {
        // Check if connection still exists
        if (!connection->Connected())
        {
            _connectionId = -1;
            _connectionLost = true;
            App::Instance()->events.RaiseEvent(ConnectionLostEvent{});
            break;
        }
        if ((ztime::Main() - lastKeepAliveReceiveTime).GetDuration(MILLISECONDS) > 5000)
        {
            connection->Disconnect();
            _connectionId = -1;
            _connectionLost = true;
            std::cout << "Keep alive packet not received within 5 seconds, closing connection.." << std::endl;
            App::Instance()->events.RaiseEvent(ConnectionLostEvent{});
            break;
        }

        // Incoming packets
        if (connection->PacketCount() > 0)
        {
            Packet pack1 = connection->GetPacket();
            if (pack1.id == (int32_t)PacketType::BYTE_CONFIRMATION)
            {
                bytesUnconfirmed -= pack1.Cast<size_t>();
                // Increment connection speed counter
                bytesSentSinceLastPrint += pack1.Cast<size_t>();
                // Calculate latency
                threadTimer.Update();
                TimePoint returnTime = threadTimer.Now();
                TimePoint sendTime = packetsInTransmittion.front();
                packetsInTransmittion.pop();
                packetLatencies.Push(returnTime - sendTime);
            }
            else if (pack1.id == (int32_t)PacketType::KEEP_ALIVE)
            {
                lastKeepAliveReceiveTime = ztime::Main();
            }
            else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
            {
                connection->Disconnect();
                _connectionId = -1;
                _disconnected = true;
                App::Instance()->events.RaiseEvent(ConnectionClosedEvent{});
                break;
            }
            else if (pack1.id == (int32_t)PacketType::NEW_USER)
            {
                int64_t newUserId = pack1.Cast<int64_t>();
                App::Instance()->events.RaiseEvent(UserConnectedEvent{ newUserId });

                std::lock_guard<std::mutex> lock(_m_users);
                _users.push_back({ L"", newUserId });
            }
            else if (pack1.id == (int32_t)PacketType::USER_DATA)
            {
                int64_t userId = pack1.Cast<int64_t>();
                size_t nameLength = (pack1.size - sizeof(int64_t)) / sizeof(wchar_t);
                std::wstring username;
                username.resize(nameLength);
                for (int i = 0; i < nameLength; i++)
                {
                    username[i] = *((wchar_t*)(pack1.Bytes() + sizeof(int64_t)) + i);
                }

                //App::Instance()->events.RaiseEvent(UserNameChangedEvent{ userId, username });

                std::lock_guard<std::mutex> lock(_m_users);
                for (int i = 0; i < _users.size(); i++)
                {
                    if (_users[i].id == userId)
                    {
                        _users[i].name = username;
                        break;
                    }
                }
            }
            else if (pack1.id == (int32_t)PacketType::DISCONNECTED_USER)
            {
                int64_t disconnectedUserId = pack1.Cast<int32_t>();
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
                size_t userCount = pack1.size / sizeof(int64_t);
                std::lock_guard<std::mutex> lock(_m_users);
                _users.resize(userCount);
                for (int i = 0; i < userCount; i++)
                    _users[i] = { L"", ((int64_t*)pack1.Bytes())[i] };
            }
            else if (pack1.id == (int32_t)PacketType::USER_ID)
            {
                if (connection->PacketCount() > 0)
                {
                    Packet pack2 = connection->GetPacket();

                    // Increment connection speed counter
                    bytesReceivedSinceLastPrint += pack2.size;

                    // Send confirmation packet
                    connection->Send(Packet((int)PacketType::BYTE_CONFIRMATION).From(pack2.size), 1);

                    // Process split packet
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
                        splitPackets.push_back({ headData.splitId, headData.packetId, headData.partCount, headData.packetSize });
                    }
                    else if (pack2.id == (int)PacketType::SPLIT_PACKET_PART)
                    {
                        int64_t splitId = pack2.Cast<int64_t>();
                        int index = -1;
                        for (int i = 0; i < splitPackets.size(); i++)
                        {
                            if (splitPackets[i].splitId == splitId)
                            {
                                splitPackets[i].parts.push_back(pack2.Reference());
                                index = i;
                                break;
                            }
                        }

                        // Check if all parts received
                        if (index != -1)
                        {
                            if (splitPackets[index].parts.size() == splitPackets[index].partCount)
                            {
                                // Combine
                                auto bytes = std::make_unique<int8_t[]>(splitPackets[index].packetSize);
                                size_t bytePos = 0;
                                size_t partDataOffset = sizeof(int64_t) + sizeof(int32_t);
                                for (auto& part : splitPackets[index].parts)
                                {
                                    size_t byteCount = part.size - partDataOffset;
                                    std::copy_n(part.Bytes() + partDataOffset, byteCount, bytes.get() + bytePos);
                                    bytePos += byteCount;
                                }
                                Packet combined = Packet(std::move(bytes), splitPackets[index].packetSize, splitPackets[index].packetId);
                                splitPackets.erase(splitPackets.begin() + index);

                                // Add to queue
                                int64_t from = pack1.Cast<int32_t>();
                                std::unique_lock<std::mutex> lock(_m_inPackets);
                                _PacketData data = { std::move(combined), { from } };
                                _inPackets.push(std::move(data));
                            }
                        }
                    }
                    else if (pack2.id == (int)PacketType::SPLIT_PACKET_ABORT)
                    {
                        int64_t splitId = pack2.Cast<int64_t>();
                        for (int i = 0; i < splitPackets.size(); i++)
                        {
                            if (splitPackets[i].splitId == splitId)
                            {
                                splitPackets.erase(splitPackets.begin() + i);
                                break;
                            }
                        }
                    }
                    else
                    {
                        int64_t from = pack1.Cast<int32_t>();
                        std::unique_lock<std::mutex> lock(_m_inPackets);
                        _PacketData data = { std::move(pack2), { from } };
                        _inPackets.push(std::move(data));
                    }
                }
                else
                {
                    std::cout << "[WARN] User id packet received without accompanying data packet" << std::endl;
                }
            }
        }

        // Outgoing packets
        std::unique_lock<std::mutex> lock(_m_outPackets);
        if (_outPackets.size() > 0)
        {
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

                if (bytesUnconfirmed > maxUnconfirmedBytes && heavyPacket)
                    continue;

                _PacketData data = std::move(_outPackets[i].first);
                int priority = _outPackets[i].second;
                _outPackets.erase(_outPackets.begin() + i);
                lock.unlock();

                bytesUnconfirmed += data.packet.size;
                threadTimer.Update();
                packetsInTransmittion.push(threadTimer.Now());

                // Some packet types don't need a prefix
                bool prefixed = !(
                    view.id == (int32_t)PacketType::USER_DATA
                    );


                // Construct packets
                if (prefixed)
                {
                    size_t userIdDataSize = data.userIds.size() * sizeof(int64_t);
                    auto userIdData = std::make_unique<int8_t[]>(userIdDataSize);
                    for (int j = 0; j < data.userIds.size(); j++)
                    {
                        ((int64_t*)userIdData.get())[j] = data.userIds[j];
                    }
                    connection->AddToQueue(Packet(std::move(userIdData), userIdDataSize, (int)PacketType::USER_ID));
                }
                connection->AddToQueue(std::move(data.packet));
                connection->SendQueue(priority);

                break;
            }
            if (lock)
                lock.unlock();
        }
        else
        {
            lock.unlock();
        }

        // Print connection speed
        Duration timeElapsed = ztime::Main() - lastPrintTime;
        if (printSpeed && timeElapsed >= printInterval)
        {
            int64_t latency = 0;
            for (int i = 0; i < packetLatencies.Size(); i++)
                latency += packetLatencies[i].GetDuration();
            latency /= packetLatencies.Size();

            App::Instance()->events.RaiseEvent(NetworkStatsEvent{ timeElapsed, (int64_t)bytesSentSinceLastPrint, (int64_t)bytesReceivedSinceLastPrint, latency });

            lastPrintTime = ztime::Main();
            std::cout << "[INFO] Avg. send speed: " << bytesSentSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
            std::cout << "[INFO] Avg. receive speed: " << bytesReceivedSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
            std::cout << "[INFO] Avg. latency over " << packetLatencies.Size() << " packets: " << latency << "us" << std::endl;
            bytesSentSinceLastPrint = 0;
            bytesReceivedSinceLastPrint = 0;
        }

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
    App::Instance()->events.RaiseEvent(DisconnectEvent{});
    App::Instance()->events.RaiseEvent(NetworkModeChangedEvent{ NetworkMode::OFFLINE });
}