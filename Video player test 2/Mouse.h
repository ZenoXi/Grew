#pragma once

#include "GameTime.h"
#include "Navigation.h"
#include "MouseEventHandler.h"

#include <queue>
#include <vector>

class Gesture
{
    friend class Mouse;

    int id;

    std::vector<Pos2D<float>> points;
    std::vector<TimePoint> times;
    Pos2D<float> offset;
    bool finished;
public:
    Gesture()
    {
        points.clear();
        times.clear();
        finished = false;
        id = ztime::Main().GetTime() / 1000;
    }
    int GetId() const
    {
        return id;
    }
    Pos2D<float> GetStart() const
    {
        return (points.size() > 0) ? points[0] : Pos2D<float>(0.0f, 0.0f, true);
    }
    Pos2D<float> GetEnd() const
    {
        return (points.size() > 0) ? points[points.size() - 1] : Pos2D<float>(0.0f, 0.0f, true);
    }
    Pos2D<float> GetPoint(unsigned index) const
    {
        if (points.size() == 0)
            return Pos2D<float>(0.0f, 0.0f, true);

        if (index >= 0 || index < points.size())
            return points[index];
        else
            return Pos2D<float>(0.0f, 0.0f, true);
    }
    TimePoint GetStartTime() const
    {
        return (times.size() > 0) ? times[0] : TimePoint(0);
    }
    TimePoint GetEndTime() const
    {
        return (times.size() > 0) ? times[times.size() - 1] : TimePoint(0);
    }
    TimePoint GetPointTime(unsigned index) const
    {
        if (points.size() == 0)
            return TimePoint(0);

        if (index >= 0 || index < points.size())
            return times[index];
        else
            return TimePoint(0);
    }
    Pos2D<float> GetOffset() const
    {
        return offset;
    }
    unsigned int PointCount() const
    {
        return points.size();
    }
    bool Finished() const
    {
        return finished;
    }
};

class Mouse : public MouseEventHandler
{
    friend class DisplayWindow;
public:
    class Event
    {
    public:
        enum Type
        {
            LPress,
            LRelease,
            RPress,
            RRelease,
            WheelUp,
            WheelDown,
            Move,
            Invalid
        };
    private:
        Type type;
        bool leftIsPressed;
        bool rightIsPressed;
        Pos2D<float> pos;
        Pos2D<float> offset;
    public:
        Event()
            :
            type(Invalid),
            leftIsPressed(false),
            rightIsPressed(false),
            pos(0.0f, 0.0f, false),
            offset(0.0f, 0.0f, false)
        {}
        Event(Type type, const Mouse& parent)
            :
            type(type),
            leftIsPressed(parent.leftIsPressed),
            rightIsPressed(parent.rightIsPressed),
            pos(parent.pos),
            offset(parent.offset)
        {}
        bool IsValid() const
        {
            return type != Invalid;
        }
        Type GetType() const
        {
            return type;
        }
        Pos2D<float> GetPos() const
        {
            return pos;
        }
        int GetPosX() const
        {
            return pos.x;
        }
        int GetPosY() const
        {
            return pos.y;
        }
        Pos2D<float> GetOffset() const
        {
            return offset;
        }
        bool LeftIsPressed() const
        {
            return leftIsPressed;
        }
        bool RightIsPressed() const
        {
            return rightIsPressed;
        }
    };
public:
    Mouse() = default;
    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;
    Pos2D<float> GetPos() const;
    int GetPosX() const;
    int GetPosY() const;
    Pos2D<float> GetOffset() const;
    void SetOffset(Pos2D<float> offset);
    bool LeftIsPressed() const;
    bool RightIsPressed() const;
    bool IsInWindow() const;
    Mouse::Event Read();
    bool IsEmpty() const
    {
        return buffer.empty();
    }
    void Flush();
    void ResetGestureL();
    void ResetGestureR();
public:
    void Update();
    Gesture GetGestureL() const;
    Gesture GetGestureR() const;
private:
    Gesture lastGestureL;
    Gesture lastGestureR;
private:
    void OnMouseMove(int x, int y);
    void OnMouseLeave();
    void OnMouseEnter();
    void OnLeftPressed(int x, int y);
    void OnLeftReleased(int x, int y);
    void OnRightPressed(int x, int y);
    void OnRightReleased(int x, int y);
    void OnWheelUp(int x, int y);
    void OnWheelDown(int x, int y);
    void TrimBuffer();
private:
    static constexpr unsigned int bufferSize = 4u;
    Pos2D<float> pos;
    Pos2D<float> offset;
    bool leftIsPressed = false;
    bool rightIsPressed = false;
    bool isInWindow = false;
    std::queue<Event> buffer;
};