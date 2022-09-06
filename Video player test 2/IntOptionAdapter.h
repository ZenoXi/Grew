#pragma once

#include <string>
#include <sstream>
#include <stdexcept>

class IntOptionAdapter
{
    int64_t _value = false;

public:
    IntOptionAdapter(const std::wstring& optStr, int64_t defaultValue = 0)
    {
        _value = defaultValue;
        try { _value = std::stoll(optStr); }
        catch (std::out_of_range) {}
        catch (std::invalid_argument) {}
    }

    IntOptionAdapter(int64_t value)
    {
        _value = value;
    }

    std::wstring ToOptionString() const
    {
        std::wstringstream ss;
        ss << _value;
        return ss.str();
    }

    int64_t Value() const
    {
        return _value;
    }
};