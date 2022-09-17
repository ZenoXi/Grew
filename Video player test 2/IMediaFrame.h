#pragma once

#include "GameTime.h"

// Base class to house all frame types
class IMediaFrame
{
public:
    TimePoint GetTimestamp() const
    {
        return _timestamp;
    }

    void SetTimeStamp(TimePoint timestamp)
    {
        _timestamp = timestamp;
    }

    IMediaFrame(TimePoint timestamp) : _timestamp(timestamp) {}
    virtual ~IMediaFrame() {}

protected:
    TimePoint _timestamp;
};