#pragma once

#include "IMediaFrame.h"

#include "Graphics.h"

class IVideoFrame : public IMediaFrame
{
public:
    IVideoFrame(TimePoint timestamp, int width, int height) : IMediaFrame(timestamp)
    {
        _width = width;
        _height = height;
    }
    IVideoFrame() : IMediaFrame(-1)
    {
        _width = -1;
        _height = -1;
    }

    virtual void DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap);

    int GetWidth() const
    {
        return _width;
    }

    int GetHeight() const
    {
        return _height;
    }

protected:
    int _width;
    int _height;
};