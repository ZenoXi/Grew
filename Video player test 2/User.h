#pragma once

#include "Event.h"

#include <string>
#include <vector>
#include <unordered_map>

class User
{
public:
    int64_t id;
    std::wstring name;

    bool GetPermission(std::string key) const
    {
        if (_denyAll)
            return false;
        if (_allowAll)
            return true;

        auto it = _permissions.find(key);
        if (it != _permissions.end())
        {
            return it->second;
        }
        return false;
    }
    void SetPermission(std::string key, bool value)
    {
        _permissions[key] = value;
    }
    void AddPermission(std::wstring name, std::string key)
    {
        _permissionNames.push_back({ name, key });
        if (_permissions.find(key) == _permissions.end())
            _permissions[key] = false;
        _onPermissionAdd.InvokeAll(name, key);
    }
    std::vector<std::pair<std::wstring, std::string>> GetPermissionNames() const
    {
        return _permissionNames;
    }
    std::unordered_map<std::string, bool> GetRawPermissions() const
    {
        return _permissions;
    }
    void ClearPermissions()
    {
        _permissions.clear();
        _permissionNames.clear();
    }

    bool AllowAll() const { return _allowAll; }
    void AllowAll(bool value) { _allowAll = value; }
    bool DenyAll() const { return _denyAll; }
    void DenyAll(bool value) { _denyAll = value; }

    void AddOnPermissionAdd(std::function<void(std::wstring, std::string)> handler)
    {
        _onPermissionAdd.Add(handler);
    }

    User() {}
    User(const User& other)
    {
        id = other.id;
        name = other.name;
        _permissions = other._permissions;
        _permissionNames = other._permissionNames;
        _allowAll = other._allowAll;
        _denyAll = other._denyAll;
    }
private:
    std::unordered_map<std::string, bool> _permissions;
    std::vector<std::pair<std::wstring, std::string>> _permissionNames;
    bool _allowAll = false;
    bool _denyAll = false;

    Event<void, std::wstring, std::string> _onPermissionAdd;
};