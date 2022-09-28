#pragma once

#include "ISubtitleFrame.h"

class SubtitleOutputAdapter
{
    std::unique_ptr<ISubtitleFrame> _frame;
    std::atomic<bool> _frameChanged{ false };
    uint64_t _frameCounter = 0;

public:
    SubtitleOutputAdapter() : _frame(nullptr) {}
    ~SubtitleOutputAdapter() {}

    void SetFrame(std::unique_ptr<ISubtitleFrame> frame)
    {
        _frame = std::move(frame);
        _frameChanged.store(true);
        _frameCounter++;
    }

    ISubtitleFrame* GetFrameData() const
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
