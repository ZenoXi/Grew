#include "App.h"
#include "Users.h"

#include "UsersEventHandler_Offline.h"

const User* Users_Internal::GetUser(int64_t id) const
{
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i]->id == id)
        {
            return users[i].get();
        }
    }
    return nullptr;
}

const User* Users_Internal::GetThisUser() const
{
    return GetUser(thisUserId);
}

std::vector<const User*> Users_Internal::GetUsers(bool includeSelf) const
{
    std::vector<const User*> userPtrs;
    userPtrs.reserve(users.size());
    for (int i = 0; i < users.size(); i++)
    {
        if (!includeSelf && users[i]->id == thisUserId)
            continue;
        userPtrs.push_back(users[i].get());
    }
    return userPtrs;
}

std::vector<int64_t> Users_Internal::GetUserIds(bool includeSelf) const
{
    std::vector<int64_t> userIds;
    userIds.reserve(users.size());
    for (int i = 0; i < users.size(); i++)
    {
        if (!includeSelf && users[i]->id == thisUserId)
            continue;
        userIds.push_back(users[i]->id);
    }
    return userIds;
}

Users::Users()
{
    // Alloc internals
    _users = std::make_unique<Users_Internal>();

    // Init offline event handler
    _eventHandler = std::make_unique<UsersEventHandler_Offline>(_users.get());
}

Users::~Users()
{

}

User* Users::DefaultUser()
{
    return _users->defaultUser.get();
}

void Users::SetSelfUsername(std::wstring newUsername)
{
    _eventHandler->SetSelfUsername(newUsername);
}

void Users::SetUsername(int64_t id, std::wstring newUsername)
{
    _eventHandler->SetUsername(id, newUsername);
}

void Users::SetPermission(int64_t id, std::string permission, bool value)
{
    _eventHandler->SetPermission(id, permission, value);
}

void Users::Update()
{
    _eventHandler->Update();
}