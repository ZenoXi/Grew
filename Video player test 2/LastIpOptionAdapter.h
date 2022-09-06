#pragma once

#include <string>
#include <vector>

#include "Functions.h"

class LastIpOptionAdapter
{
    std::vector<std::wstring> _ipList;
    int _maxEntries = 10;

public:
    LastIpOptionAdapter(std::wstring optionStr = L"")
    {
        if (!optionStr.empty())
        {
            FromOptionString(optionStr);
        }
    }

    void FromOptionString(std::wstring optionStr)
    {
        _ipList.clear();
        split_wstr(optionStr, _ipList, ',', true);
        // Remove invalid ips
        for (int i = 0; i < _ipList.size(); i++)
        {
            if (!ValidateFullIp(_ipList[i]))
            {
                _ipList.erase(_ipList.begin() + i);
                i--;
            }
        }
        // Remove excess ips
        while (_ipList.size() > _maxEntries)
        {
            _ipList.pop_back();
        }
    }

    std::wstring ToOptionString() const
    {
        std::wstring optionStr = L"";
        for (int i = 0; i < _ipList.size(); i++)
        {
            if (i != 0)
                optionStr += ',';
            optionStr += _ipList[i];
        }
        return optionStr;
    }

    std::vector<std::wstring> GetList() const
    {
        return _ipList;
    }

    bool AddIp(std::wstring fullip)
    {
        if (!ValidateFullIp(fullip))
            return false;

        // Remove previous occurences
        for (int i = 0; i < _ipList.size(); i++)
        {
            if (_ipList[i] == fullip)
            {
                _ipList.erase(_ipList.begin() + i);
                i--;
            }
        }

        if (_ipList.size() < _maxEntries)
        {
            _ipList.insert(_ipList.begin(), fullip);
            return true;
        }
        return false;
    }

    bool AddIp(std::wstring ip, std::wstring port)
    {
        return AddIp(ip + L':' + port);
    }

    bool RemoveIp(std::wstring fullip)
    {
        auto it = std::find(_ipList.begin(), _ipList.end(), fullip);
        if (it != _ipList.end())
        {
            _ipList.erase(it);
            return true;
        }
        return false;
    }

    bool RemoveIp(std::wstring ip, std::wstring port)
    {
        return RemoveIp(ip + L':' + port);
    }

    static bool ValidateFullIp(std::wstring fullip)
    {
        std::vector<std::wstring> ipParts;
        split_wstr(fullip, ipParts, ':');
        if (ipParts.size() != 2)
            return false;

        if (!ValidateIp(ipParts[0]))
            return false;

        if (!ValidatePort(ipParts[1]))
            return false;

        return true;
    }

    static bool ValidateIp(std::wstring ip)
    {
        std::vector<std::wstring> ipParts;
        split_wstr(ip, ipParts, '.');
        if (ipParts.size() != 4)
            return false;

        for (int i = 0; i < ipParts.size(); i++)
        {
            if (ipParts[i].length() > 3 || ipParts[i].length() < 1)
                return false;
            if (!std::all_of(ipParts[i].begin(), ipParts[i].end(), ::isdigit))
                return false;
            if (ipParts[i][0] == '0' && ipParts[i].length() != 1)
                return false;

            int64_t num = str_to_int(wstring_to_string(ipParts[i]));
            if (num > 255 || num < 0)
                return false;
        }
        return true;
    }

    static bool ValidatePort(std::wstring port)
    {
        if (port.length() > 5 || port.length() < 1)
            return false;
        if (!std::all_of(port.begin(), port.end(), ::isdigit))
            return false;
        if (port[0] == '0' && port.length() != 1)
            return false;

        int64_t num = str_to_int(wstring_to_string(port));
        if (num > 65535 || num < 1)
            return false;

        return true;
    }
};