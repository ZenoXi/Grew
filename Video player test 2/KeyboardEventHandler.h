#pragma once

#include <vector>
#include <string>

#include "ChiliWin.h"
#include "Event.h"

#define _KEY_COUNT 256
// Used for clarity and consistency
#define EVENT_HANDLED(expr) expr == true

enum KeyModifiers
{
    // Values taken from Microsoft documentation for 'RegisterHotKey()'
    KMOD_ALT = 0x0001,
    KMOD_CONTROL = 0x0002,
    KMOD_NOREPEAT = 0x4000,
    KMOD_SHIFT = 0x0004
};

class KeyboardEventHandler
{
protected:
    BYTE _keyStates[_KEY_COUNT]{ 0 };

    virtual bool _OnHotkey(int id) = 0;
    virtual bool _OnKeyDown(BYTE vkCode) = 0;
    virtual bool _OnKeyUp(BYTE vkCode) = 0;
    virtual bool _OnChar(wchar_t ch) = 0;
    
    Event<bool, int> _OnHotkeyHandlers;
    Event<bool, BYTE> _OnKeyDownHandlers;
    Event<bool, BYTE> _OnKeyUpHandlers;
    Event<bool, wchar_t> _OnCharHandlers;

public:
    KeyboardEventHandler() {}

    bool OnHotkey(int id)
    {
        for (auto& hnd : _OnHotkeyHandlers)
        {
            if (EVENT_HANDLED(hnd(id)))
            {
                return true;
            }
        }
        return _OnHotkey(id);
    }

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

    void AddOnHotkey(const std::function<bool(int)>& func)
    {
        _OnHotkeyHandlers.Add(func);
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

    // 'modifiers' specify which modifiers from 'KeyModifiers' enum must be pressed
    // for the key state to return true. If 'exact' is true, then ONLY the specified
    // modifiers need to be pressed.
    bool KeyState(int vkCode, int modifiers = 0, bool exact = false)
    {
        if (!(_keyStates[vkCode] & 0x80))
            return false;
        if (modifiers == 0 && !exact)
            return true;

        bool altNeeded = modifiers & KMOD_ALT;
        bool altState = _keyStates[VK_MENU] & 0x80;
        bool ctrlNeeded = modifiers & KMOD_CONTROL;
        bool ctrlState = _keyStates[VK_CONTROL] & 0x80;
        bool shiftNeeded = modifiers & KMOD_SHIFT;
        bool shiftState = _keyStates[VK_SHIFT] & 0x80;

        if ((altNeeded && !altState) || (exact && !altNeeded && altState))
            return false;
        if ((ctrlNeeded && !ctrlState) || (exact && !ctrlNeeded && ctrlState))
            return false;
        if ((shiftNeeded && !shiftState) || (exact && !shiftNeeded && shiftState))
            return false;
        return true;
    }

    BYTE* KeyStates()
    {
        return _keyStates;
    }
};