#pragma once

#include <string>
#include <vector>

#include "Functions.h"

class LastIpOptionAdapter
{
    std::vector<std::string> _ipList;

public:
    LastIpOptionAdapter(std::string optionStr = "")
    {
        if (!optionStr.empty())
        {
            FromOptionString(optionStr);
        }
    }

    void FromOptionString(std::string optionStr)
    {
        _ipList.clear();
        split_str(optionStr, _ipList, ',', true);
        // Remove invalid ips
        for (int i = 0; i < _ipList.size(); i++)
        {
            if (!ValidateFullIp(_ipList[i]))
            {
                _ipList.erase(_ipList.begin() + i);
                i--;
            }
        }
    }

    std::string ToOptionString() const
    {
        std::string optionStr = "";
        for (int i = 0; i < _ipList.size(); i++)
        {
            if (i != 0)
                optionStr += ',';
            optionStr += _ipList[i];
        }
        return optionStr;
    }

    std::vector<std::string> GetList() const
    {
        return _ipList;
    }

    bool AddIp(std::string fullip)
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

        _ipList.insert(_ipList.begin(), fullip);
        return true;
    }

    bool AddIp(std::string ip, std::string port)
    {
        return AddIp(ip + ':' + port);
    }

    static bool ValidateFullIp(std::string fullip)
    {
        std::vector<std::string> ipParts;
        split_str(fullip, ipParts, ':');
        if (ipParts.size() != 2)
            return false;

        if (!ValidateIp(ipParts[0]))
            return false;

        if (!ValidatePort(ipParts[1]))
            return false;

        return true;
    }

    static bool ValidateIp(std::string ip)
    {
        std::vector<std::string> ipParts;
        split_str(ip, ipParts, '.');
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

            int64_t num = str_to_int(ipParts[i]);
            if (num > 255 || num < 0)
                return false;
        }
        return true;
    }

    static bool ValidatePort(std::string port)
    {
        if (port.length() > 5 || port.length() < 1)
            return false;
        if (!std::all_of(port.begin(), port.end(), ::isdigit))
            return false;
        if (port[0] == '0' && port.length() != 1)
            return false;

        int64_t num = str_to_int(port);
        if (num > 65535 || num < 1)
            return false;

        return true;
    }
};