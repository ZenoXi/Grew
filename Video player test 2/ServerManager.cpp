#include "ServerManager.h"

#include "App.h"
#include "NetworkEvents.h"
#include "PacketBuilder.h"

znet::ServerManager::ServerManager(uint16_t port)
{
    _thisUser = { L"", 0 };
    _generator = std::make_unique<_ServerIDGenerator>();
    _server.SetGenerator(_generator.get());
    _server.StartServer(port);
    _startFailed = !_server.AllowNewConnections();
    if (_startFailed)
        return;
    _managementThread = std::thread(&ServerManager::_ManageConnections, this);

    App::Instance()->events.RaiseEvent(ServerStartEvent{ port });
    App::Instance()->events.RaiseEvent(NetworkStateChangedEvent{ Name() });
}

znet::ServerManager::~ServerManager()
{
    _server.SetGenerator(nullptr);

    _MANAGEMENT_THR_STOP = true;
    if (_managementThread.joinable())
        _managementThread.join();

    App::Instance()->events.RaiseEvent(ServerStopEvent{});
    App::Instance()->events.RaiseEvent(NetworkStateChangedEvent{ "offline" });
}

std::vector<znet::ServerManager::User> znet::ServerManager::Users(bool includeSelf)
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

std::vector<int64_t> znet::ServerManager::UserIds(bool includeSelf)
{
    std::lock_guard<std::mutex> lock(_m_users);
    std::vector<int64_t> userIds;
    for (auto& user : _users)
        userIds.push_back(user.id);
    if (includeSelf)
        userIds.push_back(_thisUser.id);
    return userIds;
}

znet::ServerManager::User znet::ServerManager::ThisUser()
{
    return _thisUser;
}

int64_t znet::ServerManager::ThisUserId()
{
    return _thisUser.id;
}

void znet::ServerManager::SetUsername(std::wstring username)
{
    _thisUser.name = username;

    // Send new username
    PacketBuilder builder = PacketBuilder(sizeof(int64_t) + sizeof(wchar_t) * _thisUser.name.length());
    builder.Add(int64_t(0));
    builder.Add(_thisUser.name.data(), _thisUser.name.length());
    Send(Packet(builder.Release(), builder.UsedBytes(), (int)PacketType::USER_DATA), UserIds(false), 2);
}

void znet::ServerManager::Send(Packet&& packet, std::vector<int64_t> userIds, int priority)
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

void znet::ServerManager::_AddPacketsToOutQueue(std::vector<_PacketData> packets, int priority)
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

void znet::ServerManager::_AddPacketToOutQueue(_PacketData packet, int priority)
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

void znet::ServerManager::AddToQueue(Packet&& packet)
{
    _PacketData data = { std::move(packet) };
    _packetQueue.push_back(std::move(data));
}

void znet::ServerManager::SendQueue(std::vector<int64_t> userIds, int priority)
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

void znet::ServerManager::AbortSend(int32_t packetId)
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
            _PacketData packetData = { Packet((int32_t)PacketType::SPLIT_PACKET_ABORT).From(headData.splitId), _outPackets[i].first.userIds };
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
            _PacketData packetData = { Packet((int32_t)PacketType::SPLIT_PACKET_ABORT).From(splitId), _outPackets[i].first.userIds };
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

znet::NetworkStatus znet::ServerManager::Status()
{
    if (!_startFailed)
        return NetworkStatus::ONLINE;

    return NetworkStatus::OFFLINE;
}

std::wstring znet::ServerManager::StatusString()
{
    switch (Status())
    {
    case NetworkStatus::INITIALIZING:
        return L"Starting server..";
    case NetworkStatus::ONLINE:
        return L"Server running";
    case NetworkStatus::UNINITIALIZING:
        return L"Stopping server..";
    case NetworkStatus::OFFLINE:
        if (_startFailed)
            return L"Start failed..";
        else
            return L"";
    default:
        return L"";
    }
}

void znet::ServerManager::_ManageConnections()
{
    _ManagerThreadData data;

    while (!_MANAGEMENT_THR_STOP)
    {
        // Send keep-alive packets
        if ((ztime::Main() - data.lastKeepAliveSendTime).GetDuration(MILLISECONDS) >= 1000)
        {
            data.lastKeepAliveSendTime = ztime::Main();
            std::lock_guard<std::mutex> lock(_m_users);
            for (int i = 0; i < _users.size(); i++)
            {
                auto connection = _server.Connection(_users[i].id);
                connection->Send(znet::Packet((int)znet::PacketType::KEEP_ALIVE).From(int8_t(0)), std::numeric_limits<int>::max());
            }
        }

        _ProcessNewConnections(data);
        _ProcessDisconnects(data);
        _ProcessIncomingPackets(data);
        _ProcessOutgoingPackets(data);
        _PrintNetworkStats(data);

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

void znet::ServerManager::_ProcessNewConnections(_ManagerThreadData& data)
{
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
}

void znet::ServerManager::_ProcessDisconnects(_ManagerThreadData& data)
{
    int64_t disconnectedUser;
    while ((disconnectedUser = _server.GetDisconnectedUser()) != -1)
    {
        App::Instance()->events.RaiseEvent(UserDisconnectedEvent{ disconnectedUser });

        std::lock_guard<std::mutex> lock(_m_users);
        for (int i = 0; i < _users.size(); i++)
        {
            if (_users[i].id == disconnectedUser)
            {
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
}

void znet::ServerManager::_ProcessIncomingPackets(_ManagerThreadData& data)
{
    std::lock_guard<std::mutex> lock(_m_users);
    for (int i = 0; i < _users.size(); i++)
    {
        TCPClientRef client = _server.Connection(_users[i].id);
        if (!client.Valid())
            continue;
        if (client->PacketCount() == 0)
            continue;

        Packet pack1 = client->GetPacket();
        // BYTE CONFIRMATION
        if (pack1.id == (int32_t)PacketType::BYTE_CONFIRMATION)
        {
            _usersData[i].bytesUnconfirmed -= pack1.Cast<size_t>();
            // Increment connection speed counter
            data.bytesSentSinceLastPrint += pack1.Cast<size_t>();
        }
        // USER DATA
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
            App::Instance()->events.RaiseEvent(UserNameChangedEvent{ _users[i].id, _users[i].name });

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
        // DISCONNECT REQUEST
        else if (pack1.id == (int32_t)PacketType::DISCONNECT_REQUEST)
        {
            continue;
        }
        // USER ID
        else if (pack1.id == (int32_t)PacketType::USER_ID)
        {
            if (client->PacketCount() < 1)
            {
                std::cout << "[WARN] User id packet received without accompanying data packet\n";
                continue;
            }

            Packet pack2 = client->GetPacket();

            // Increment connection speed counter
            data.bytesReceivedSinceLastPrint += pack2.size;

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

            // If server user id is in destinations, pass packet to handler
            for (int j = 0; j < to.size(); j++)
            {
                if (to[j] == 0)
                {
                    bool splitPacket = _ProcessSplitPackets(data, pack2, i);
                    // Pass to handler
                    if (!splitPacket)
                        _packetReceivedHandler(std::move(pack2), _users[i].id);

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
    }
}

void znet::ServerManager::_ProcessOutgoingPackets(_ManagerThreadData& data)
{
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

        // 'Heavy' packets will be postponed until unconfirmed byte count is below a certain value
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
            if (heavyPacket && _usersData[userIndex].bytesUnconfirmed > data.maxUnconfirmedBytes)
                continue;

            // Some packet types don't need a prefix
            bool prefixed = !(
                view.id == (int32_t)PacketType::USER_DATA
                );

            // Reference and send the packet to the destination user
            if (prefixed)
            {
                PacketBuilder builder = PacketBuilder(sizeof(int64_t));
                if (_outPackets[i].first.redirected)
                    builder.Add(_outPackets[i].first.sourceUserId);
                else
                    builder.Add(int64_t(0));
                connection->AddToQueue(Packet(builder.Release(), builder.UsedBytes(), (int)PacketType::USER_ID));
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
}

void znet::ServerManager::_PrintNetworkStats(_ManagerThreadData& data)
{
    Duration timeElapsed = ztime::Main() - data.lastPrintTime;
    if (timeElapsed >= data.printInterval)
    {
        App::Instance()->events.RaiseEvent(NetworkStatsEvent{ timeElapsed, (int64_t)data.bytesSentSinceLastPrint, (int64_t)data.bytesReceivedSinceLastPrint, -1 });

        data.lastPrintTime = ztime::Main();
        if (data.printStats)
        {
            std::cout << "[INFO] Avg. send speed: " << data.bytesSentSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
            std::cout << "[INFO] Avg. receive speed: " << data.bytesReceivedSinceLastPrint / timeElapsed.GetDuration(MILLISECONDS) << "kb/s" << std::endl;
        }
        data.bytesSentSinceLastPrint = 0;
        data.bytesReceivedSinceLastPrint = 0;
    }
}

bool znet::ServerManager::_ProcessSplitPackets(_ManagerThreadData& data, Packet& pack, int userIndex)
{
    if (pack.id == (int)PacketType::SPLIT_PACKET_HEAD)
    {
        struct SplitHead
        {
            int64_t splitId;
            int32_t packetId;
            int32_t partCount;
            size_t packetSize;
        };
        auto headData = pack.Cast<SplitHead>();
        _usersData[userIndex].splitPackets.push_back({ headData.splitId, headData.packetId, headData.partCount, headData.packetSize });
    }
    else if (pack.id == (int)PacketType::SPLIT_PACKET_PART)
    {
        int64_t splitId = pack.Cast<int64_t>();
        int index = -1;
        for (int i = 0; i < _usersData[userIndex].splitPackets.size(); i++)
        {
            if (_usersData[userIndex].splitPackets[i].splitId == splitId)
            {
                _usersData[userIndex].splitPackets[i].parts.push_back(pack.Reference());
                index = i;
                break;
            }
        }

        // Check if all parts received
        if (index != -1)
        {
            if (_usersData[userIndex].splitPackets[index].parts.size() == _usersData[userIndex].splitPackets[index].partCount)
            {
                // Combine
                auto bytes = std::make_unique<int8_t[]>(_usersData[userIndex].splitPackets[index].packetSize);
                size_t bytePos = 0;
                size_t partDataOffset = sizeof(int64_t) + sizeof(int32_t);
                for (auto& part : _usersData[userIndex].splitPackets[index].parts)
                {
                    size_t byteCount = part.size - partDataOffset;
                    std::copy_n(part.Bytes() + partDataOffset, byteCount, bytes.get() + bytePos);
                    bytePos += byteCount;
                }
                Packet combined = Packet(std::move(bytes), _usersData[userIndex].splitPackets[index].packetSize, _usersData[userIndex].splitPackets[index].packetId);
                _usersData[userIndex].splitPackets.erase(_usersData[userIndex].splitPackets.begin() + index);

                // Pass to handler
                _packetReceivedHandler(std::move(combined), _users[userIndex].id);
            }
        }
    }
    else if (pack.id == (int)PacketType::SPLIT_PACKET_ABORT)
    {
        int64_t splitId = pack.Cast<int64_t>();
        for (int i = 0; i < _usersData[userIndex].splitPackets.size(); i++)
        {
            if (_usersData[userIndex].splitPackets[i].splitId == splitId)
            {
                _usersData[userIndex].splitPackets.erase(_usersData[userIndex].splitPackets.begin() + i);
                break;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}