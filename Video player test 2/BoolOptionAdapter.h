#pragma once

#include <string>

class BoolOptionAdapter
{
    bool _value = false;

public:
    BoolOptionAdapter(const std::wstring& optStr, bool defaultValue = false)
    {
        if (optStr == L"true")
            _value = true;
        else if (optStr == L"false")
            _value = false;
        else
            _value = defaultValue;
    }

    BoolOptionAdapter(bool value)
    {
        _value = value;
    }

    std::wstring ToOptionString() const
    {
        if (_value)
            return L"true";
        else
            return L"false";
    }

    bool Value() const
    {
        return _value;
    }
};