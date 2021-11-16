#pragma once

class MouseEventHandler
{
    //friend class DisplayWindow;

protected:
    bool _mouseInWindow = false;
    bool _mouseLeftClicked = false;
    bool _mouseRightClicked = false;
    int _mousePosX = 0;
    int _mousePosY = 0;

    virtual void _OnMouseMove(int x, int y) {}
    virtual void _OnMouseLeave() {}
    virtual void _OnMouseEnter() {}
    virtual void _OnLeftPressed(int x, int y) {}
    virtual void _OnLeftReleased(int x, int y) {}
    virtual void _OnRightPressed(int x, int y) {}
    virtual void _OnRightReleased(int x, int y) {}
    virtual void _OnWheelUp(int x, int y) {}
    virtual void _OnWheelDown(int x, int y) {}

public:
    void OnMouseMove(int x, int y)
    {
        _mousePosX = x;
        _mousePosY = y;
        _OnMouseMove(x, y);
    }
    void OnMouseLeave()
    {
        _mouseInWindow = false;
        _OnMouseLeave();
    }
    void OnMouseEnter()
    {
        _mouseInWindow = true;
        _OnMouseEnter();
    }
    void OnLeftPressed(int x, int y)
    {
        _mouseLeftClicked = true;
        _OnLeftPressed(x, y);
    }
    void OnLeftReleased(int x, int y)
    {
        _mouseLeftClicked = false;
        _OnLeftReleased(x, y);
    }
    void OnRightPressed(int x, int y)
    {
        _mouseRightClicked = true;
        _OnRightPressed(x, y);
    }
    void OnRightReleased(int x, int y)
    {
        _mouseRightClicked = false;
        _OnRightReleased(x, y);
    }
    void OnWheelUp(int x, int y)
    {
        _OnWheelUp(x, y);
    }
    void OnWheelDown(int x, int y)
    {
        _OnWheelDown(x, y);
    }

    bool MouseInWindow() { return _mouseInWindow; }
    bool MouseLeftClicked() { return _mouseLeftClicked; }
    bool MouseRightClicked() { return _mouseRightClicked; }
    int MousePosX() { return _mousePosX; }
    int MousePosY() { return _mousePosY; }
};