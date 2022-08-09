#include "UsersEventHandler_Client.h"
#include "Users.h"

#include "App.h"
#include "Network.h"
#include "PacketBuilder.h"

UsersEventHandler_Client::UsersEventHandler_Client(Users_Internal* users) : IUsersEventHandler(users)
{
    _users->thisUserId = APP_NETWORK->ThisUserId();
    _users->defaultUser = std::make_unique<User>();
    auto localUser = std::make_unique<User>();
    localUser->id = _users->thisUserId;
    _users->users.push_back(std::move(localUser));

    // Set up packet receivers
    _userConnectedReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::USER_CONNECTED);
    _userDisconnectedReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::USER_DISCONNECTED);
    _usernameChangeReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::USERNAME_CHANGED);
    _permissionChangeReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PERMISSION_CHANGED);
    _permissionNameReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PERMISSION_NAME);

    // Request default permissions
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PERMISSION_NAME_REQUEST), { 0 }, 2);

    // Request all user data
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::ALL_USER_DATA_REQUEST), { 0 }, 2);
}

void UsersEventHandler_Client::SetSelfUsername(std::wstring newUsername)
{
    PacketBuilder builder;
    builder.Add(newUsername.data(), newUsername.length());
    APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::USERNAME_CHANGE_REQUEST), { 0 }, 2);
}

void UsersEventHandler_Client::SetPermission(int64_t id, std::string permission, bool value)
{
    // Send request to server
    PacketBuilder builder;
    builder.Add(id);
    builder.Add(value ? int8_t(1) : int8_t(0));
    builder.Add(permission.data(), permission.length());
    APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_CHANGE_REQUEST), { 0 }, 2);
}

void UsersEventHandler_Client::Update()
{
    int usernameChangeCount = _usernameChangeReceiver->PacketCount();
    int permissionChangeCount = _permissionChangeReceiver->PacketCount();
    _CheckForUserConnections();
    _CheckForUserDisconnections();
    _CheckForUsernameChanges(usernameChangeCount);
    _CheckForPermissionChanges(permissionChangeCount);
    _CheckForPermissionNames();
}

void UsersEventHandler_Client::_CheckForUserConnections()
{
    while (_userConnectedReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _userConnectedReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t senderId = packetPair.second;

        // Ignore non-server packets
        if (senderId != 0)
            continue;

        PacketReader reader(packet.Bytes(), packet.size);
        int64_t userId = reader.Get<int64_t>();

        // Add user if it doesn't exist
        if (_users->GetUser(userId))
            continue;
        auto newUser = std::make_unique<User>(*_users->defaultUser);
        newUser->id = userId;
        _users->users.push_back(std::move(newUser));
        App::Instance()->events.RaiseEvent(UserAddedEvent{ userId });
    }
}

void UsersEventHandler_Client::_CheckForUserDisconnections()
{
    while (_userDisconnectedReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _userDisconnectedReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t senderId = packetPair.second;

        // Ignore non-server packets
        if (senderId != 0)
            continue;

        PacketReader reader(packet.Bytes(), packet.size);
        int64_t userId = reader.Get<int64_t>();

        // Remove user
        for (int i = 0; i < _users->users.size(); i++)
        {
            if (_users->users[i]->id == userId)
            {
                _users->users.erase(_users->users.begin() + i);
                App::Instance()->events.RaiseEvent(UserRemovedEvent{ userId });
                break;
            }
        }
    }
}

void UsersEventHandler_Client::_CheckForUsernameChanges(int count)
{
    // It is possible for a USER_CONNECTED packet to arrive between
    // the call to _CheckForUserConnections and this function, which
    // could also allow the client to receive a username change for
    // a user which it doesn't yet see (it sits in the _userConnectedReceiver).
    // Because of this, 'count' is the limit of how many username
    // changes this function must process to avoid the aforementioned
    // problem.

    for (int i = 0; i < count; i++)
    {
        // Process packet
        auto packetPair = _usernameChangeReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t senderId = packetPair.second;

        // Ignore non-server packets
        if (senderId != 0)
            continue;

        PacketReader reader(packet.Bytes(), packet.size);
        int64_t userId = reader.Get<int64_t>();
        size_t stringLen = reader.RemainingBytes() / sizeof(wchar_t);
        std::wstring newUsername;
        newUsername.resize(stringLen);
        reader.Get(newUsername.data(), stringLen);

        // Casting const away is safe because it is only in place to prevent
        // direct modification from outside.
        User* user = (User*)_users->GetUser(userId);
        if (user)
        {
            user->name = newUsername;
            App::Instance()->events.RaiseEvent(UserNameChangedEvent{ userId, newUsername });
        }
    }
}

void UsersEventHandler_Client::_CheckForPermissionChanges(int count)
{
    // Read '_CheckForUsernameChanges'

    for (int i = 0; i < count; i++)
    {
        // Process packet
        auto packetPair = _permissionChangeReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t senderId = packetPair.second;

        // Ignore non-server packets
        if (senderId != 0)
            continue;

        PacketReader reader(packet.Bytes(), packet.size);
        int64_t userId = reader.Get<int64_t>();
        int8_t value = reader.Get<int8_t>();
        size_t stringLen = reader.RemainingBytes() / sizeof(char);
        std::string permission;
        permission.resize(stringLen);
        reader.Get(permission.data(), stringLen);

        // Casting const away is safe because it is only in place to prevent
        // direct modification from outside.
        User* user = (User*)_users->GetUser(userId);
        if (user)
        {
            if (permission == "ALLOW_ALL")
                user->AllowAll(value);
            else if (permission == "DENY_ALL")
                user->DenyAll(value);
            else
                user->SetPermission(permission, value);
            App::Instance()->events.RaiseEvent(UserPermissionChangedEvent{ userId, permission, (bool)value });
        }
    }
}

void UsersEventHandler_Client::_CheckForPermissionNames()
{
    while (_permissionNameReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _permissionNameReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t senderId = packetPair.second;

        // Ignore non-server packets
        if (senderId != 0)
            continue;

        PacketReader reader(packet.Bytes(), packet.size);
        size_t nameLength = reader.Get<size_t>();
        std::wstring displayName;
        displayName.resize(nameLength);
        reader.Get(displayName.data(), nameLength);
        size_t keyLength = reader.RemainingBytes();
        std::string keyName;
        keyName.resize(keyLength);
        reader.Get(keyName.data(), keyLength);

        // Add to default user
        _users->defaultUser->AddPermission(displayName, keyName);

        // Apply to all users
        for (auto& user : _users->users)
        {
            user->AddPermission(displayName, keyName);
        }
    }
}