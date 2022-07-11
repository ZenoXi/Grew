#pragma once

#include "MouseEventHandler.h"

#include <vector>

class MouseManager : public MouseEventHandler
{
    std::vector<MouseEventHandler*> _handlers;
    MouseEventHandler* _overlayHandler = nullptr;

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

    void SetOverlayHandler(MouseEventHandler* handler)
    {
        _overlayHandler = handler;
    }

private: // Mouse event handling
    bool _OnMouseMove(int x, int y)
    {
        if (_overlayHandler)
        {
            bool clicked = false;
            for (auto& handler : _handlers)
            {
                if (handler->MouseLeftClicked()) { clicked = true; break; }
                if (handler->MouseRightClicked()) { clicked = true; break; }
            }
            if (!clicked)
            {
                if (_overlayHandler->OnMouseMove(x, y))
                {
                    for (auto& handler : _handlers)
                        handler->OnMouseLeave();
                    return true;
                }
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnMouseMove(x, y))
                return true;
        }
        return false;
    }

    void _OnMouseLeave()
    {
        for (auto& handler : _handlers)
        {
            handler->OnMouseLeave();
        }
    }

    void _OnMouseEnter()
    {
        for (auto& handler : _handlers)
        {
            handler->OnMouseEnter();
        }
    }

    bool _OnLeftPressed(int x, int y)
    {
        if (_overlayHandler)
        {
            if (_overlayHandler->OnLeftPressed(x, y))
            {
                return true;
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnLeftPressed(x, y))
                return true;
        }
        return false;
    }

    bool _OnLeftReleased(int x, int y)
    {
        if (_overlayHandler)
        {
            if (_overlayHandler->OnLeftReleased(x, y))
            {
                return true;
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnLeftReleased(x, y))
                return true;
        }
        return false;
    }

    bool _OnRightPressed(int x, int y)
    {
        if (_overlayHandler)
        {
            if (_overlayHandler->OnRightPressed(x, y))
            {
                return true;
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnRightPressed(x, y))
                return true;
        }
        return false;
    }

    bool _OnRightReleased(int x, int y)
    {
        if (_overlayHandler)
        {
            if (_overlayHandler->OnRightReleased(x, y))
            {
                return true;
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnRightReleased(x, y))
                return true;
        }
        return false;
    }

    bool _OnWheelUp(int x, int y)
    {
        if (_overlayHandler)
        {
            if (_overlayHandler->OnWheelUp(x, y))
            {
                return true;
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnWheelUp(x, y))
                return true;
        }
        return false;
    }

    bool _OnWheelDown(int x, int y)
    {
        if (_overlayHandler)
        {
            if (_overlayHandler->OnWheelDown(x, y))
            {
                return true;
            }
        }

        for (auto& handler : _handlers)
        {
            if (handler->OnWheelDown(x, y))
                return true;
        }
        return false;
    }
};