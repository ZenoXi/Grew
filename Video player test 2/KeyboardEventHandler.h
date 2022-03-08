#pragma once

#include <vector>
#include <string>

#include "ChiliWin.h"
#include "Event.h"

#define _KEY_COUNT 256
// Used for clarity and consistency
#define EVENT_HANDLED(expr) expr == true

class KeyboardEventHandler
{
protected:
    BYTE _keyStates[_KEY_COUNT]{ 0 };

    virtual bool _OnKeyDown(BYTE vkCode) = 0;
    virtual bool _OnKeyUp(BYTE vkCode) = 0;
    virtual bool _OnChar(wchar_t ch) = 0;
    
    Event<bool, BYTE> _OnKeyDownHandlers;
    Event<bool, BYTE> _OnKeyUpHandlers;
    Event<bool, wchar_t> _OnCharHandlers;

public:
    KeyboardEventHandler() {}

    bool OnKeyDown(BYTE vkCode)
    {
        _keyStates[vkCode] = 0x80;
        for (auto& hnd : _OnKeyDownHandlers)
        {
            if (EVENT_HANDLED(hnd(vkCode)))
            {
                return true;
            }
        }
        return _OnKeyDown(vkCode);
    }

    bool OnKeyUp(BYTE vkCode)
    {
        _keyStates[vkCode] = 0;
        for (auto& hnd : _OnKeyUpHandlers)
        {
            if (EVENT_HANDLED(hnd(vkCode)))
            {
                return true;
            }
        }
        return _OnKeyUp(vkCode);
    }

    bool OnChar(wchar_t ch)
    {
        for (auto& hnd : _OnCharHandlers)
        {
            if (EVENT_HANDLED(hnd(ch)))
            {
                return true;
            }
        }
        return _OnChar(ch);
    }

    void AddOnKeyDown(const std::function<bool(BYTE)>& func)
    {
        _OnKeyDownHandlers.Add(func);
    }

    void AddOnKeyUp(const std::function<bool(BYTE)>& func)
    {
        _OnKeyUpHandlers.Add(func);
    }

    void AddOnChar(const std::function<bool(wchar_t)>& func)
    {
        _OnCharHandlers.Add(func);
    }

    bool KeyState(int vkCode)
    {
        return _keyStates[vkCode] & 0x80;
    }

    BYTE* KeyStates()
    {
        return _keyStates;
    }
};