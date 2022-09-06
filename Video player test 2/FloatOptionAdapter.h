#pragma once

#include <string>
#include <sstream>
#include <stdexcept>

class FloatOptionAdapter
{
    float _value = false;

public:
    FloatOptionAdapter(const std::wstring& optStr, float defaultValue = 0.0f)
    {
        _value = defaultValue;
        try { _value = std::stof(optStr); }
        catch (std::out_of_range) {}
        catch (std::invalid_argument) {}
    }

    FloatOptionAdapter(float value)
    {
        _value = value;
    }

    std::wstring ToOptionString() const
    {
        std::wstringstream ss;
        ss << _value;
        return ss.str();
    }

    float Value() const
    {
        return _value;
    }
};