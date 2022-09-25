#pragma once

#include "IUsersEventHandler.h"

class UsersEventHandler_Offline : public IUsersEventHandler
{
public:
    void SetSelfUsername(std::wstring newUsername) {}
    void SetUsername(int64_t id, std::wstring newUsername) {}
    void SetPermission(int64_t id, std::string permission, bool value) {}

    void Update() {}

    UsersEventHandler_Offline(Users_Internal* users);
    ~UsersEventHandler_Offline();
};