#pragma once

#include "User.h"
#include "IUsersEventHandler.h"

#include <memory>

class Users_Internal
{
public:
    int64_t thisUserId;
    std::vector<std::unique_ptr<User>> users;
    std::unique_ptr<User> defaultUser;

    const User* GetUser(int64_t id) const;
    const User* GetThisUser() const;
    std::vector<const User*> GetUsers(bool includeSelf) const;
    std::vector<int64_t> GetUserIds(bool includeSelf) const;
};

class Users
{
public:
    Users();
    ~Users();

    // The returned pointer SHOULD NOT BE SAVED for later use, as it can get deleted.
    // The pointer is guaranteed to exist for the entire frame of the UI thread.
    const User* GetUser(int64_t id) const { return _users->GetUser(id); }
    const User* GetThisUser() const { return _users->GetThisUser(); }
    std::vector<const User*> GetUsers(bool includeSelf) const { return _users->GetUsers(includeSelf); }
    std::vector<int64_t> GetUserIds(bool includeSelf) const { return _users->GetUserIds(includeSelf); }
    User* DefaultUser();

    void SetSelfUsername(std::wstring newUsername);
    void SetUsername(int64_t id, std::wstring newUsername);
    void SetPermission(int64_t id, std::string permission, bool value);

    void Update();

    template<class _Hnd, class... _Args>
    void SetEventHandler(_Args... args);

private:
    std::unique_ptr<Users_Internal> _users = nullptr;
    std::unique_ptr<IUsersEventHandler> _eventHandler = nullptr;
};

template<class _Hnd, class... _Args>
void Users::SetEventHandler(_Args... args)
{
    // Handler destructor and constructor take care of necessary cleanup
    _eventHandler.reset();
    _eventHandler = std::make_unique<_Hnd>(_users.get(), std::forward<_Args>(args)...);
}