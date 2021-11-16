#pragma once

#include "KeyboardEventHandler.h"
#include "ComponentBase.h"

class KeyboardManager : public KeyboardEventHandler
{
    std::vector<KeyboardEventHandler*> _handlers;
    KeyboardEventHandler* _exclusiveHandler = nullptr;

public:
    KeyboardManager() {}

    void AddHandler(KeyboardEventHandler* handler)
    {
        _handlers.push_back(handler);
    }

    void RemoveHandler(KeyboardEventHandler* handler)
    {
        for (auto it = _handlers.begin(); it != _handlers.end(); it++)
        {
            if (*it == handler)
            {
                _handlers.erase(it);
                break;
            }
        }
    }

    void SetExclusiveHandler(KeyboardEventHandler* handler)
    {
        _exclusiveHandler = handler;
    }

    void ResetExclusiveHandler()
    {
        _exclusiveHandler = nullptr;
    }

private:  // Keyboard event handling
    void _OnKeyDown(BYTE vkCode)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnKeyDown(vkCode);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnKeyDown(vkCode);
        }
    }

    void _OnKeyUp(BYTE vkCode)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnKeyUp(vkCode);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnKeyUp(vkCode);
        }
    }

    void _OnChar(wchar_t ch)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnChar(ch);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnChar(ch);
        }
    }
};