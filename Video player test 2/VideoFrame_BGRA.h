#pragma once

#include "IVideoFrame.h"

class VideoFrame_BGRA : public IVideoFrame
{
public:
    VideoFrame_BGRA(TimePoint timestamp)
        : IVideoFrame(timestamp, -1, -1)
    {
        _data = nullptr;
    }
    VideoFrame_BGRA(TimePoint timestamp, int width, int height, std::unique_ptr<unsigned char[]> data)
        : IVideoFrame(timestamp, width, height)
    {
        _data = std::move(data);
    }

    void DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap);

protected:
    std::unique_ptr<unsigned char[]> _data;
};