#pragma once

#include "MouseEventHandler.h"

#include <vector>

class MouseManager : public MouseEventHandler
{
    std::vector<MouseEventHandler*> _handlers;
    MouseEventHandler* _exclusiveHandler = nullptr;

public:
    MouseManager() {};

    void AddHandler(MouseEventHandler* handler)
    {
        _handlers.push_back(handler);
    }

    void RemoveHandler(MouseEventHandler* handler)
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

    void SetExclusiveHandler(MouseEventHandler* handler)
    {
        _exclusiveHandler = handler;
    }

    void ResetExclusiveHandler()
    {
        _exclusiveHandler = nullptr;
    }

private: // Mouse event handling
    void _OnMouseMove(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnMouseMove(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnMouseMove(x, y);
        }
    }

    void _OnMouseLeave()
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnMouseLeave();
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnMouseLeave();
        }
    }

    void _OnMouseEnter()
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnMouseEnter();
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnMouseEnter();
        }
    }

    void _OnLeftPressed(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnLeftPressed(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnLeftPressed(x, y);
        }
    }

    void _OnLeftReleased(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnLeftReleased(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnLeftReleased(x, y);
        }
    }

    void _OnRightPressed(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnRightPressed(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnRightPressed(x, y);
        }
    }

    void _OnRightReleased(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnRightReleased(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnRightReleased(x, y);
        }
    }

    void _OnWheelUp(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnWheelUp(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnWheelUp(x, y);
        }
    }

    void _OnWheelDown(int x, int y)
    {
        if (_exclusiveHandler)
        {
            _exclusiveHandler->OnWheelDown(x, y);
            return;
        }

        for (auto& handler : _handlers)
        {
            handler->OnWheelDown(x, y);
        }
    }
};