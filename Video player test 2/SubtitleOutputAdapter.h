#pragma once

#include "ISubtitleFrame.h"

class SubtitleOutputAdapter
{
    std::unique_ptr<ISubtitleFrame> _frame;
    std::atomic<bool> _frameChanged{ false };

public:
    SubtitleOutputAdapter() : _frame(nullptr) {}
    ~SubtitleOutputAdapter() {}

    void SetFrame(std::unique_ptr<ISubtitleFrame> frame)
    {
        _frame = std::move(frame);
        _frameChanged.store(true);
    }

    ISubtitleFrame* GetFrameData() const
    {
        return _frame.get();
    }

    bool FrameChanged()
    {
        return _frameChanged.exchange(false);
    }
};
