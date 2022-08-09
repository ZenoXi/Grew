#include "UsersEventHandler_Offline.h"
#include "Users.h"

UsersEventHandler_Offline::UsersEventHandler_Offline(Users_Internal* users) : IUsersEventHandler(users)
{
    _users->users.clear();
}