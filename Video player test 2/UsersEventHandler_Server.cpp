#include "UsersEventHandler_Server.h"
#include "Users.h"

#include "App.h"
#include "Network.h"
#include "PacketBuilder.h"

#include "Permissions.h"

UsersEventHandler_Server::UsersEventHandler_Server(Users_Internal* users) : IUsersEventHandler(users)
{
    _users->thisUserId = -1;
    _users->defaultUser = std::make_unique<User>();
    _users->defaultUser->AddOnPermissionAdd([&](std::wstring name, std::string key)
    {
        _addedPermissions.push({ name, key });
    });

    // Set up event handlers
    _userConnectedReceiver = std::make_unique<EventReceiver<UserConnectedEvent>>(&App::Instance()->events);
    _userDisconnectedReceiver = std::make_unique<EventReceiver<UserDisconnectedEvent>>(&App::Instance()->events);
    // Set up packet receivers
    _nameChangeRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::USERNAME_CHANGE_REQUEST);
    _permissionChangeRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PERMISSION_CHANGE_REQUEST);
    _allUserDataRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::ALL_USER_DATA_REQUEST);
    _permissionNameRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PERMISSION_NAME_REQUEST);

    // Add local client
    auto localUser = std::make_unique<User>();
    localUser->id = APP_NETWORK->ThisUserId();
    localUser->AllowAll(true);
    _users->users.push_back(std::move(localUser));
}

void UsersEventHandler_Server::SetSelfUsername(std::wstring newUsername)
{
    return;
}

void UsersEventHandler_Server::SetUsername(int64_t id, std::wstring newUsername)
{
    for (auto& user : _users->users)
    {
        if (user->id == id)
        {
            user->name = newUsername;

            // Send change to all clients
            PacketBuilder builder;
            builder.Add(id);
            builder.Add(newUsername.data(), newUsername.length());
            APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::USERNAME_CHANGED), _users->GetUserIds(true), 2);

            App::Instance()->events.RaiseEvent(UserNameChangedEvent{ id, newUsername });
            break;
        }
    }
}

void UsersEventHandler_Server::SetPermission(int64_t id, std::string permission, bool value)
{
    for (auto& user : _users->users)
    {
        if (user->id == id)
        {
            if (user->GetPermission(permission) == value)
                break;
            user->SetPermission(permission, value);

            // Send change to client and local client
            PacketBuilder builder;
            builder.Add(id);
            builder.Add(value ? int8_t(1) : int8_t(0));
            builder.Add(permission.data(), permission.length());
            std::vector<int64_t> ids;
            ids.push_back(id);
            if (id != 0)
                ids.push_back(0);
            APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_CHANGED), ids, 2);

            App::Instance()->events.RaiseEvent(UserPermissionChangedEvent{ id, permission, value });
            break;
        }
    }
}

void UsersEventHandler_Server::Update()
{
    _HandleUserConnectedEvents();
    _HandleUserDisconnectedEvents();
    _CheckForNameChangeRequests();
    _CheckForPermissionChangeRequests();
    _CheckForAllUserDataRequests();
    _CheckForPermissionNameRequests();
    _HandleAddedPermissions();
}

void UsersEventHandler_Server::_AddUser(int64_t id)
{
    // Check if user exists
    if (_users->GetUser(id))
        return;

    auto newUser = std::make_unique<User>(*_users->defaultUser.get());
    newUser->id = id;

    // Notify all clients
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::USER_CONNECTED).From(id), _users->GetUserIds(false), 2);

    // Send permissions to local client
    _SendPermissions(newUser.get(), { 0 });

    _users->users.push_back(std::move(newUser));
}

void UsersEventHandler_Server::_RemoveUser(int64_t id)
{
    // Check if user exists
    for (int i = 0; i < _users->users.size(); i++)
    {
        if (_users->users[i]->id == id)
        {
            _users->users.erase(_users->users.begin() + i);

            // Notify all clients
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::USER_DISCONNECTED).From(id), _users->GetUserIds(false), 2);
            break;
        }
    }
}

void UsersEventHandler_Server::_HandleUserConnectedEvents()
{
    while (_userConnectedReceiver->EventCount() > 0)
    {
        auto ev = _userConnectedReceiver->GetEvent();
        _AddUser(ev.userId);
    }
}

void UsersEventHandler_Server::_HandleUserDisconnectedEvents()
{
    while (_userDisconnectedReceiver->EventCount() > 0)
    {
        auto ev = _userDisconnectedReceiver->GetEvent();
        _RemoveUser(ev.userId);
    }
}

void UsersEventHandler_Server::_CheckForNameChangeRequests()
{
    while (_nameChangeRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _nameChangeRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        PacketReader reader(packet.Bytes(), packet.size);
        size_t stringLen = reader.RemainingBytes() / sizeof(wchar_t);
        std::wstring newUsername;
        newUsername.resize(stringLen);
        reader.Get(newUsername.data(), stringLen);

        // Find user
        bool userFound = false;
        for (auto& user : _users->users)
        {
            if (user->id == userId)
            {
                userFound = true;
                user->name = newUsername;

                // Notify all clients
                PacketBuilder builder;
                builder.Add(userId);
                builder.Add(newUsername.data(), newUsername.length());
                APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::USERNAME_CHANGED), _users->GetUserIds(true), 2);
            }
        }
        if (!userFound)
        {
            // Deny request
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::USERNAME_CHANGE_DENIED), { userId }, 2);
        }
    }
}

void UsersEventHandler_Server::_CheckForPermissionChangeRequests()
{
    while (_permissionChangeRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _permissionChangeRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t senderId = packetPair.second;

        // Check permissions
        if (!_users->GetUser(senderId)->GetPermission(PERMISSION_EDIT_PERMISSIONS))
            continue;

        PacketReader reader(packet.Bytes(), packet.size);
        int64_t userId = reader.Get<int64_t>();
        int8_t value = reader.Get<int8_t>();
        size_t stringLen = reader.RemainingBytes() / sizeof(char);
        std::string permission;
        permission.resize(stringLen);
        reader.Get(permission.data(), stringLen);

        User* user = (User*)_users->GetUser(userId);
        if (user)
        {
            user->SetPermission(permission, value);

            // Send change to client and local client
            std::vector<int64_t> ids;
            ids.push_back(userId);
            if (userId != 0)
                ids.push_back(0);
            znet::Packet copy = packet.Reference();
            copy.id = (int)znet::PacketType::PERMISSION_CHANGED;
            APP_NETWORK->Send(std::move(copy), ids, 2);

            App::Instance()->events.RaiseEvent(UserPermissionChangedEvent{ userId, permission, (bool)value });
        }
    }
}

void UsersEventHandler_Server::_CheckForAllUserDataRequests()
{
    while (_allUserDataRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _allUserDataRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        for (auto& user : _users->users)
        {
            // Send users
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::USER_CONNECTED).From(user->id), { userId }, 2);

            // Send usernames
            PacketBuilder builder;
            builder.Add(user->id);
            builder.Add(user->name.data(), user->name.length());
            APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::USERNAME_CHANGED), { userId }, 2);

            // Send permissions
            _SendPermissions(user.get(), { userId });
        }
    }
}

void UsersEventHandler_Server::_CheckForPermissionNameRequests()
{
    while (_permissionNameRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _permissionNameRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        for (auto& perm : _users->defaultUser->GetPermissionNames())
        {
            // Send permission names
            PacketBuilder builder;
            builder.Add(perm.first.length());
            builder.Add(perm.first.data(), perm.first.length());
            builder.Add(perm.second.data(), perm.second.length());
            APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_NAME), { userId }, 2);
        }
    }
}

void UsersEventHandler_Server::_HandleAddedPermissions()
{
    while (!_addedPermissions.empty())
    {
        auto namePair = _addedPermissions.front();
        _addedPermissions.pop();

        // Send permission names
        PacketBuilder builder;
        builder.Add(namePair.first.length());
        builder.Add(namePair.first.data(), namePair.first.length());
        builder.Add(namePair.second.data(), namePair.second.length());
        APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_NAME), _users->GetUserIds(true), 2);
    }
}

void UsersEventHandler_Server::_SendPermissions(const User* user, std::vector<int64_t> to)
{
    // Send main permissions
    for (auto& perm : user->GetRawPermissions())
    {
        PacketBuilder builder;
        builder.Add(user->id);
        builder.Add(perm.second ? int8_t(1) : int8_t(0));
        builder.Add(perm.first.data(), perm.first.length());
        APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_CHANGED), to, 2);
    }

    // Send allow/deny all
    if (user->AllowAll())
    {
        std::string permission = "ALLOW_ALL";
        PacketBuilder builder;
        builder.Add(user->id);
        builder.Add(int8_t(1));
        builder.Add(permission.data(), permission.length());
        APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_CHANGED), to, 2);
    }
    if (user->DenyAll())
    {
        std::string permission = "DENY_ALL";
        PacketBuilder builder;
        builder.Add(user->id);
        builder.Add(int8_t(1));
        builder.Add(permission.data(), permission.length());
        APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PERMISSION_CHANGED), to, 2);
    }
}