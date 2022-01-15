#pragma once

#include "VideoFrame.h"

class IVideoOutputAdapter
{
    VideoFrame _videoData;
    VideoFrame _subtitleData;

    std::atomic<bool> _videoDataChanged{ false };
    std::atomic<bool> _subtitleDataChanged{ false };

public:
    IVideoOutputAdapter() : _videoData(0, 0, -1), _subtitleData(0, 0, -1) {}
    virtual ~IVideoOutputAdapter() {}

    void SetVideoData(const VideoFrame& frame)
    {
        _videoData = frame;
        _videoDataChanged.store(true);
    }

    void SetSubtitleData(const VideoFrame& frame)
    {
        _subtitleData = frame;
        _subtitleDataChanged.store(true);
    }

    const VideoFrame& GetVideoData() const
    {
        return _videoData;
    }

    const VideoFrame& GetSubtitleData() const
    {
        return _subtitleData;
    }

    bool VideoDataChanged()
    {
        return _videoDataChanged.exchange(false);
    }

    bool SubtitleDataChanged()
    {
        return _subtitleDataChanged.exchange(false);
    }
};