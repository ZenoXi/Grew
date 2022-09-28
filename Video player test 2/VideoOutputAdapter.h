#pragma once

#include "IVideoFrame.h"

class VideoOutputAdapter
{
    std::unique_ptr<IVideoFrame> _frame;
    std::atomic<bool> _frameChanged{ false };
    uint64_t _frameCounter = 0;

public:
    VideoOutputAdapter() : _frame(nullptr) {}
    ~VideoOutputAdapter() {}

    void SetFrame(std::unique_ptr<IVideoFrame> frame)
    {
        _frame = std::move(frame);
        _frameChanged.store(true);
        _frameCounter++;
    }

    IVideoFrame* GetFrameData() const
    {
        return _frame.get();
    }

    bool FrameChanged()
    {
        return _frameChanged.exchange(false);
    }

    uint64_t FrameNumber() const
    {
        return _frameCounter;
    }
};