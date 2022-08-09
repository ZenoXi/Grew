#pragma once

#include "IUsersEventHandler.h"
#include "User.h"

#include "AppEvents.h"
#include "NetworkEvents.h"
#include "UserEvents.h"
#include "PacketSubscriber.h"

class UsersEventHandler_Server : public IUsersEventHandler
{
public:
    void SetSelfUsername(std::wstring newUsername);
    void SetUsername(int64_t id, std::wstring newUsername);
    void SetPermission(int64_t id, std::string permission, bool value);

    void Update();

    UsersEventHandler_Server(Users_Internal* users);

private:
    void _AddUser(int64_t id);
    void _RemoveUser(int64_t id);

    std::unique_ptr<EventReceiver<UserConnectedEvent>> _userConnectedReceiver = nullptr;
    std::unique_ptr<EventReceiver<UserDisconnectedEvent>> _userDisconnectedReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _nameChangeRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _permissionChangeRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _allUserDataRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _permissionNameRequestReceiver = nullptr;

    void _HandleUserConnectedEvents();
    void _HandleUserDisconnectedEvents();
    void _CheckForNameChangeRequests();
    void _CheckForPermissionChangeRequests();
    void _CheckForAllUserDataRequests();
    void _CheckForPermissionNameRequests();
    void _HandleAddedPermissions();

    void _SendPermissions(const User* user, std::vector<int64_t> to);

    std::queue<std::pair<std::wstring, std::string>> _addedPermissions;
};