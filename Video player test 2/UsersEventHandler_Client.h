#pragma once

#include "IUsersEventHandler.h"

#include "AppEvents.h"
#include "NetworkEvents.h"
#include "UserEvents.h"
#include "PacketSubscriber.h"

class UsersEventHandler_Client : public IUsersEventHandler
{
public:
    void SetSelfUsername(std::wstring newUsername);
    void SetUsername(int64_t id, std::wstring newUsername) {}
    void SetPermission(int64_t id, std::string permission, bool value);

    void Update();

    UsersEventHandler_Client(Users_Internal* users);

private:
    std::unique_ptr<znet::PacketReceiver> _userConnectedReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _userDisconnectedReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _usernameChangeReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _permissionChangeReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _permissionNameReceiver = nullptr;

    void _CheckForUserConnections();
    void _CheckForUserDisconnections();
    void _CheckForUsernameChanges(int count);
    void _CheckForPermissionChanges(int count);
    void _CheckForPermissionNames();
};