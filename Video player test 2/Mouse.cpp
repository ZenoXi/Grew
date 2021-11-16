#include "Mouse.h"

void Mouse::Update()
{
    lastGestureL.offset = offset;
    lastGestureR.offset = offset;

    if (leftIsPressed) {
        lastGestureL.points.push_back(pos);
        lastGestureL.times.push_back(ztime::Game());
    }
    if (rightIsPressed) {
        lastGestureR.points.push_back(pos);
        lastGestureR.times.push_back(ztime::Game());
    }
}

Gesture Mouse::GetGestureL() const
{
    return lastGestureL;
}

Gesture Mouse::GetGestureR() const
{
    return lastGestureR;
}

Pos2D<float> Mouse::GetPos() const
{
    return pos;
}

int Mouse::GetPosX() const
{
    return pos.x;
}

int Mouse::GetPosY() const
{
    return pos.y;
}

Pos2D<float> Mouse::GetOffset() const
{
    return offset;
}

void Mouse::SetOffset(Pos2D<float> offset)
{
    this->offset = offset;
}

bool Mouse::LeftIsPressed() const
{
    return leftIsPressed;
}

bool Mouse::RightIsPressed() const
{
    return rightIsPressed;
}

bool Mouse::IsInWindow() const
{
    return isInWindow;
}

Mouse::Event Mouse::Read()
{
    if (buffer.size() > 0u)
    {
        Mouse::Event e = buffer.front();
        buffer.pop();
        return e;
    }
    else
    {
        return Mouse::Event();
    }
}

void Mouse::Flush()
{
    buffer = std::queue<Event>();
}

void Mouse::ResetGestureL()
{
    lastGestureL = Gesture();
}

void Mouse::ResetGestureR()
{
    lastGestureR = Gesture();
}

void Mouse::OnMouseLeave()
{
    isInWindow = false;
    leftIsPressed = false;
    rightIsPressed = false;
    lastGestureL.finished = true;
    lastGestureR.finished = true;
}

void Mouse::OnMouseEnter()
{
    isInWindow = true;
}

void Mouse::OnMouseMove(int newx, int newy)
{
    pos.x = newx;
    pos.y = newy;

    buffer.push(Mouse::Event(Mouse::Event::Move, *this));
    TrimBuffer();
}

void Mouse::OnLeftPressed(int x, int y)
{
    leftIsPressed = true;
    ResetGestureL();

    buffer.push(Mouse::Event(Mouse::Event::LPress, *this));
    TrimBuffer();
}

void Mouse::OnLeftReleased(int x, int y)
{
    leftIsPressed = false;
    lastGestureL.finished = true;

    buffer.push(Mouse::Event(Mouse::Event::LRelease, *this));
    TrimBuffer();
}

void Mouse::OnRightPressed(int x, int y)
{
    rightIsPressed = true;
    ResetGestureR();

    buffer.push(Mouse::Event(Mouse::Event::RPress, *this));
    TrimBuffer();
}

void Mouse::OnRightReleased(int x, int y)
{
    rightIsPressed = false;
    lastGestureR.finished = true;

    buffer.push(Mouse::Event(Mouse::Event::RRelease, *this));
    TrimBuffer();
}

void Mouse::OnWheelUp(int x, int y)
{
    buffer.push(Mouse::Event(Mouse::Event::WheelUp, *this));
    TrimBuffer();
}

void Mouse::OnWheelDown(int x, int y)
{
    buffer.push(Mouse::Event(Mouse::Event::WheelDown, *this));
    TrimBuffer();
}

void Mouse::TrimBuffer()
{
    while (buffer.size() > bufferSize)
    {
        buffer.pop();
    }
}