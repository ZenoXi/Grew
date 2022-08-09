#include "ServerConnectionManager.h"

#include "App.h"
#include "NetworkEvents.h"

znet::ServerConnectionManager::ServerConnectionManager(uint16_t port)
{
    _thisUser = { L"", 0 };
    _generator = std::make_unique<ServerIDGenerator>();
    _server.SetGenerator(_generator.get());
    _server.StartServer(port);
    _server.AllowNewConnections();
    _managementThread = std::thread(&ServerConnectionManager::_ManageConnections, this);

    App::Instance()->events.RaiseEvent(ServerStartEvent{ port });
    App::Instance()->events.RaiseEvent(NetworkStateChangedEvent{ "server" });
}

znet::ServerConnectionManager::~ServerConnectionManager()
{
    _server.SetGenerator(nullptr);

    _MANAGEMENT_THR_STOP = true;
    if (_managementThread.joinable())
        _managementThread.join();

    App::Instance()->events.RaiseEvent(ServerStopEvent{});
    App::Instance()->events.RaiseEvent(NetworkStateChangedEvent{ "offline" });
}

// CONNECTION

std::string znet::ServerConnectionManager::StatusString()
{
    switch (Status())
    {
    case ConnectionStatus::RUNNING:
        return "Server running.";
    case ConnectionStatus::OFFLINE:
        return "Offline.";
    default:
        return "";
    }
}

znet::ConnectionStatus znet::ServerConnectionManager::Status()
{
    if (_server.Running())
        return ConnectionStatus::RUNNING;
    else
        return ConnectionStatus::OFFLINE;
}

std::vector<znet::ServerConnectionManager::User> znet::ServerConnectionManager::Users()
{
    std::lock_guard<std::mutex> lock(_m_users);
    return _users;
}

znet::ServerConnectionManager::User znet::ServerConnectionManager::ThisUser()
{
    return _thisUser;
}

void znet::ServerConnectionManager::SetUsername(std::wstring username)
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

    std::lock_guard<std::mutex> lock(_m_users);
    std::vector<int64_t> destinationUsers;
    for (int i = 0; i < _users.size(); i++)
        destinationUsers.push_back(_users[i].id);
    Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), destinationUsers, 2);
}

// PACKET INPUT/OUTPUT

void znet::ServerConnectionManager::Send(Packet&& packet, std::vector<int64_t> userIds, int priority)
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
    for (int i = 0; i < packets.size(); i++)
    {
        _PacketData data = { std::move(packets[i]), userIds };
        _outPackets.push_back({ std::move(data), priority });
    }
}

void znet::ServerConnectionManager::AddToQueue(Packet&& packet, std::vector<int64_t> userIds)
{
    _PacketData data = { std::move(packet), std::move(userIds) };
    _packetQueue.push(std::move(data));
}

void znet::ServerConnectionManager::SendQueue(std::vector<int64_t> userIds, int priority)
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

void znet::ServerConnectionManager::AbortSend(int32_t packetId)
{
    std::unique_lock<std::mutex> lock(_m_outPackets);
    for (int i = 0; i < _outPackets.size(); i++)
    {
        // Check split packets
        if (_outPackets[i].first.packet.id == (int32_t)PacketType::SPLIT_PACKET_HEAD
            && !_outPackets[i].first.redirected)
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
        else if (_outPackets[i].first.packet.id == (int32_t)PacketType::SPLIT_PACKET_PART
            && !_outPackets[i].first.redirected)
        {
            // Extract split id
            int64_t splitId = _outPackets[i].first.packet.Cast<int64_t>();

            while (true)
            {
                _outPackets.erase(_outPackets.begin() + i);

                if (_outPackets.size() == i)
                    break;
                if (_outPackets[i].first.packet.id != (int32_t)PacketType::SPLIT_PACKET_PART
                    || _outPackets[i].first.redirected)
                    break;
                if (_outPackets[i].first.packet.Cast<int64_t>() != splitId)
                    break;
            }

            i--;
        }
        else if (_outPackets[i].first.packet.id == packetId
            && !_outPackets[i].first.redirected)
        {
            _outPackets.erase(_outPackets.begin() + i);
            i--;
        }
    }
}

size_t znet::ServerConnectionManager::PacketCount() const
{
    return _inPackets.size();
}

std::pair<znet::Packet, int64_t> znet::ServerConnectionManager::GetPacket()
{
    std::unique_lock<std::mutex> lock(_m_inPackets);
    _PacketData data = std::move(_inPackets.front());
    _inPackets.pop();
    return { std::move(data.packet), data.userIds[0] };
}

// CONNECTION MANAGEMENT

void znet::ServerConnectionManager::_ManageConnections()
{
    size_t bytesUnconfirmed = 0;
    size_t maxUnconfirmedBytes = 250000; // 1 MB

    size_t bytesSentSinceLastPrint = 0;
    size_t bytesReceivedSinceLastPrint = 0;
    TimePoint lastPrintTime = ztime::Main();
    Duration printInterval = Duration(3, SECONDS);
    bool printSpeed = true;

    TimePoint lastKeepAliveSendTime = ztime::Main();

    while (!_MANAGEMENT_THR_STOP)
    {
        // Get new connections
        int64_t newUser;
        while ((newUser = _server.GetNewConnection()) != -1)
        {
            App::Instance()->events.RaiseEvent(UserConnectedEvent{ newUser });

            _usersData.push_back({});
            std::lock_guard<std::mutex> lock(_m_users);

            // Send new user id to other users
            for (int i = 0; i < _users.size(); i++)
            {
                TCPClientRef connection = _server.Connection(_users[i].id);
                connection->Send(Packet((int)PacketType::NEW_USER).From(newUser), 2);
            }

            TCPClientRef connection = _server.Connection(newUser);
            // Send the assigned id to the new user (maximum priority since this packet needs to go first)
            connection->Send(Packet((int)PacketType::ASSIGNED_USER_ID).From(newUser), std::numeric_limits<int>::max());
            // Send existing user ids to new user
            {
                size_t byteCount = sizeof(int64_t) * (_users.size() + 1);
                auto userIdsBytes = std::make_unique<int8_t[]>(byteCount);
                ((int64_t*)userIdsBytes.get())[0] = 0;
                for (int i = 0; i < _users.size(); i++)
                {
                    ((int64_t*)userIdsBytes.get())[i + 1] = _users[i].id;
                }
                connection->Send(Packet(std::move(userIdsBytes), byteCount, (int)PacketType::USER_LIST), 2);
            }

            // Send existing usernames to new user
            { // Server username
                size_t byteCount = sizeof(int64_t) + sizeof(wchar_t) * _thisUser.name.length();
                auto usernameBytes = std::make_unique<int8_t[]>(byteCount);
                ((int64_t*)usernameBytes.get())[0] = 0;
                for (int j = 0; j < _thisUser.name.length(); j++)
                {
                    ((wchar_t*)(usernameBytes.get() + sizeof(int64_t)))[j] = _thisUser.name[j];
                }
                connection->Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), 2);
            }
            for (int i = 0; i < _users.size(); i++)
            { // Other users
                size_t byteCount = sizeof(int64_t) + sizeof(wchar_t) * _users[i].name.length();
                auto usernameBytes = std::make_unique<int8_t[]>(byteCount);
                ((int64_t*)usernameBytes.get())[0] = _users[i].id;
                for (int j = 0; j < _users[i].name.length(); j++)
                {
                    ((wchar_t*)(usernameBytes.get() + sizeof(int64_t)))[j] = _users[i].name[j];
                }
                connection->Send(Packet(std::move(usernameBytes), byteCount, (int)PacketType::USER_DATA), 2);
            }

            _users.push_back({ L"", newUser });
        }

        // Get disconnections
        int64_t disconnectedUser;
        std::vector<int64_t> disconnects;
        while ((disconnectedUser = _server.GetDisconnectedUser()) != -1)
        {
            App::Instance()->events.RaiseEvent(UserDisconnectedEvent{ disconnectedUser });

            std::lock_guard<std::mutex> lock(_m_users);
            for (int i = 0; i < _users.size(); i++)
            {
                if (_users[i].id == disconnectedUser)
                {
                    disconnects.push_back(disconnectedUser);
                    _users.erase(_users.begin() + i);
                    _usersData.erase(_usersData.begin() + i);
                    break;
                }
            }

            // Send disconnected user id to other users
            for (int i = 0; i < _users.size(); i++)
            {
                TCPClientRef connection = _server.Connection(_users[i].id);
                connection->Send(Packet((int)PacketType::DISCONNECTED_USER).From(disconnectedUser), 2);
            }
        }

        // Send keep-alive packets
        if ((ztime::Main() - lastKeepAliveSendTime).GetDuration(MILLISECONDS) >= 1000)
        {
            lastKeepAliveSendTime = ztime::Main();
            std::lock_guard<std::mutex> lock(_m_users);
            for (int i = 0; i < _users.size(); i++)
            {
                auto connection = _server.Connection(_users[i].id);
                connection->Send(znet::Packet((int)znet::PacketType::KEEP_ALIVE).From(int8_t(0)), std::numeric_limits<int>::max());
            }
        }

        { // Incoming packets
            std::lock_guard<std::mutex> lock(_m_users);
            for (int i = 0; i < _users.size(); i++)
            {
                TCPClientRef client = _server.Connection(_users[i].id);
                if (!client.Valid())
                    continue;

                if (client->PacketCount() > 0)
                {
                    Packet pack1 = client->GetPacket();
                    if (pack1.id == (int32_t)PacketType::BYTE_CONFIRMATION)
                    {
                        _usersData[i].bytesUnconfirmed -= pack1.Cast<size_t>();
                        // Increment connection speed counter
                        bytesSentSinceLastPrint += pack1.Cast<size_t>();
                    }
                    else if (pack1.id == (int32_t)PacketType::USER_DATA)
                    {
                        size_t nameLength = (pack1.size - sizeof(int64_t)) / sizeof(wchar_t);
                        std::wstring username;
                        username.resize(nameLength);
                        for (int j = 0; j < nameLength; j++)
                        {
                            username[j] = ((wchar_t*)(pack1.Bytes() + sizeof(int64_t)))[j];
                        }

                        _users[i].name = username;
                        //App::Instance()->events.RaiseEvent(UserNameChangedEvent{ _users[i].id, _users[i].name });

                        // Send username change to other users
                        std::vector<int64_t> destinationUsers;
                        for (int j = 0; j < _users.size(); j++)
                        {
                            if (j != i)
                            {
                                destinationUsers.push_back(_users[j].id);
                            }
                        }
                        if (!destinationUsers.empty())
                        {
                            *(int64_t*)pack1.Bytes() = _users[i].id;
                            _PacketData data = { std::move(pack1), destinationUsers };
                            std::unique_lock<std::mutex> lock(_m_outPackets);
                            _outPackets.push_back({ std::move(data), 2 });
                        }
                    }
                    else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
                    {
                        continue;
                    }
                    else if (pack1.id == (int32_t)PacketType::USER_ID)
                    {
                        if (client->PacketCount() > 0)
                        {
                            Packet pack2 = client->GetPacket();

                            // Increment connection speed counter
                            bytesReceivedSinceLastPrint += pack2.size;

                            // Send confirmation packet
                            Packet confirmation = Packet((int32_t)PacketType::BYTE_CONFIRMATION);
                            confirmation.From(pack2.size);
                            client->Send(std::move(confirmation), 1);

                            // Create destination vector
                            if (pack1.size % sizeof(int64_t) != 0)
                                std::cout << "[WARN] User id packet size misaligned" << std::endl;
                            std::vector<int64_t> to;
                            for (int j = 0; j < pack1.size / sizeof(int64_t); j++)
                                to.push_back(((int64_t*)pack1.Bytes())[j]);

                            // If server user id is in destinations, move it to incoming packet queue
                            for (int j = 0; j < to.size(); j++)
                            {
                                if (to[j] == 0)
                                {
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
                                        _usersData[i].splitPackets.push_back({ headData.splitId, headData.packetId, headData.partCount, headData.packetSize });
                                    }
                                    else if (pack2.id == (int)PacketType::SPLIT_PACKET_PART)
                                    {
                                        int64_t splitId = pack2.Cast<int64_t>();
                                        int index = -1;
                                        for (int k = 0; k < _usersData[i].splitPackets.size(); k++)
                                        {
                                            if (_usersData[i].splitPackets[k].splitId == splitId)
                                            {
                                                _usersData[i].splitPackets[k].parts.push_back(pack2.Reference());
                                                index = k;
                                                break;
                                            }
                                        }

                                        // Check if all parts received
                                        if (index != -1)
                                        {
                                            if (_usersData[i].splitPackets[index].parts.size() == _usersData[i].splitPackets[index].partCount)
                                            {
                                                // Combine
                                                auto bytes = std::make_unique<int8_t[]>(_usersData[i].splitPackets[index].packetSize);
                                                size_t bytePos = 0;
                                                size_t partDataOffset = sizeof(int64_t) + sizeof(int32_t);
                                                for (auto& part : _usersData[i].splitPackets[index].parts)
                                                {
                                                    size_t byteCount = part.size - partDataOffset;
                                                    std::copy_n(part.Bytes() + partDataOffset, byteCount, bytes.get() + bytePos);
                                                    bytePos += byteCount;
                                                }
                                                Packet combined = Packet(std::move(bytes), _usersData[i].splitPackets[index].packetSize, _usersData[i].splitPackets[index].packetId);
                                                _usersData[i].splitPackets.erase(_usersData[i].splitPackets.begin() + index);

                                                // Add to queue
                                                std::unique_lock<std::mutex> lock(_m_inPackets);
                                                _PacketData data = { std::move(combined), { _users[i].id } };
                                                _inPackets.push(std::move(data));
                                            }
                                        }
                                    }
                                    else if (pack2.id == (int)PacketType::SPLIT_PACKET_ABORT)
                                    {
                                        int64_t splitId = pack2.Cast<int64_t>();
                                        for (int k = 0; k < _usersData[i].splitPackets.size(); k++)
                                        {
                                            if (_usersData[i].splitPackets[k].splitId == splitId)
                                            {
                                                _usersData[i].splitPackets.erase(_usersData[i].splitPackets.begin() + k);
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        std::unique_lock<std::mutex> lock(_m_inPackets);
                                        _PacketData data = { pack2.Reference(), { _users[i].id } };
                                        _inPackets.push(std::move(data));
                                    }

                                    to.erase(to.begin() + j);
                                    break;
                                }
                            }

                            // Add packet to queue to send to remaining destinations
                            if (!to.empty())
                            {
                                std::unique_lock<std::mutex> lock(_m_outPackets);
                                _PacketData data = { std::move(pack2), to, true, _users[i].id };
                                _outPackets.push_back({ std::move(data), 0 });
                            }
                        }
                        else
                        {
                            std::cout << "[WARN] User id packet received without accompanying data packet" << std::endl;
                        }
                    }
                }
            }
        }

        // Outgoing packets
        std::unique_lock<std::mutex> lock(_m_outPackets);
        // Loop through packets until 1 is sent
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

            bool sent = false;

            // Go through every destination user id and attempt to send
            for (int j = 0; j < _outPackets[i].first.userIds.size(); j++)
            {
                // Find user with matching id
                int userIndex = -1;
                for (int k = 0; k < _users.size(); k++)
                {
                    if (_users[k].id == _outPackets[i].first.userIds[j])
                    {
                        userIndex = k;
                        break;
                    }
                }
                // Remove the invalid destination user id
                if (userIndex == -1)
                {
                    _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                    j--;
                    continue;
                }

                // Get connection object
                TCPClientRef connection = _server.Connection(_users[userIndex].id);
                if (!connection.Valid())
                {
                    // Remove unusable destination user id
                    _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                    j--;
                    continue;
                }

                // Postpone sending heavy packets if unconfirmed byte count exceeds max value
                if (_usersData[userIndex].bytesUnconfirmed > maxUnconfirmedBytes && heavyPacket)
                    continue;

                // Some packet types don't need a prefix
                bool prefixed = !(
                    view.id == (int32_t)PacketType::USER_DATA
                    );

                // Reference and send the packet to the destination user
                if (prefixed)
                {
                    auto userIdData = std::make_unique<int8_t[]>(sizeof(int64_t));
                    if (_outPackets[i].first.redirected)
                        *(int64_t*)userIdData.get() = _outPackets[i].first.sourceUserId;
                    else
                        *(int64_t*)userIdData.get() = 0;
                    connection->AddToQueue(Packet(std::move(userIdData), sizeof(int64_t), (int)PacketType::USER_ID));
                }
                connection->AddToQueue(_outPackets[i].first.packet.Reference());
                connection->SendQueue(_outPackets[i].second);

                _usersData[userIndex].bytesUnconfirmed += _outPackets[i].first.packet.size;
                sent = true;

                // Remove destination from list
                _outPackets[i].first.userIds.erase(_outPackets[i].first.userIds.begin() + j);
                j--;
            }

            // If all destinations have been sent to, remove packet from queue
            if (_outPackets[i].first.userIds.empty())
            {
                _outPackets.erase(_outPackets.begin() + i);
                i--;
            }

            // Send packets one at a time
            if (sent) break;
        }
        lock.unlock();

        // Print connection speed
        Duration timeElapsed = ztime::Main() - lastPrintTime;
        if (timeElapsed >= printInterval)
        {
            App::Instance()->events.RaiseEvent(NetworkStatsEvent{ timeElapsed, (int64_t)bytesSentSinceLastPrint, (int64_t)bytesReceivedSinceLastPrint, -1 });

            lastPrintTime = ztime::Main();
            if (printSpeed)
            {
                std::cout << "[INFO] Avg. send speed: " << bytesSentSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
                std::cout << "[INFO] Avg. receive speed: " << bytesReceivedSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
            }
            bytesSentSinceLastPrint = 0;
            bytesReceivedSinceLastPrint = 0;
        }

        // Sleep if no immediate work needs to be done
        bool packetsIncoming = false;
        for (int i = 0; i < _users.size(); i++)
        {
            TCPClientRef client = _server.Connection(_users[i].id);
            if (!client.Valid())
                continue;
            if (client->PacketCount() > 0)
            {
                packetsIncoming = true;
                break;
            }
        }
        if (!packetsIncoming && _outPackets.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}