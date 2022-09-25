#include "UsersEventHandler_Offline.h"
#include "Users.h"

UsersEventHandler_Offline::UsersEventHandler_Offline(Users_Internal* users) : IUsersEventHandler(users)
{
    _users->users.clear();

    // Add the local user with all permissions
    auto localUser = std::make_unique<User>();
    localUser->id = -1;
    localUser->AllowAll(true);
    _users->thisUserId = -1;
    _users->users.push_back(std::move(localUser));
}

UsersEventHandler_Offline::~UsersEventHandler_Offline()
{
    // Clear the local user
    _users->users.clear();
}