#pragma once

#include <string>
#include <memory>
#include <cstdint>

class Users_Internal;

class IUsersEventHandler
{
public:
    virtual void SetSelfUsername(std::wstring newUsername) = 0;
    virtual void SetUsername(int64_t id, std::wstring newUsername) = 0;
    virtual void SetPermission(int64_t id, std::string permission, bool value) = 0;

    virtual void Update() = 0;

    IUsersEventHandler(Users_Internal* users) : _users(users) {};
    virtual ~IUsersEventHandler() {};
    IUsersEventHandler(IUsersEventHandler&&) = delete;
    IUsersEventHandler& operator=(IUsersEventHandler&&) = delete;
    IUsersEventHandler(const IUsersEventHandler&) = delete;
    IUsersEventHandler& operator=(const IUsersEventHandler&) = delete;

protected:
    Users_Internal* _users = nullptr;
};