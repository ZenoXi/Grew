#pragma once

#include <string>

struct UserAddedEvent
{
    static const char* _NAME_() { return "user_added"; }
    int64_t userId;
};

struct UserRemovedEvent
{
    static const char* _NAME_() { return "user_removed"; }
    int64_t userId;
};

struct UserNameChangedEvent
{
    static const char* _NAME_() { return "user_name_changed"; }
    int64_t userId;
    std::wstring newName;
};

struct UserPermissionChangedEvent
{
    static const char* _NAME_() { return "user_permission_changed"; }
    int64_t userId;
    std::string permission;
    bool newValue;
};