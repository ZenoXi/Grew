#pragma once

#include "VideoFrame.h"

class IVideoOutputAdapter
{
    VideoFrame _videoData;
    VideoFrame _subtitleData;

public:
    IVideoOutputAdapter() : _videoData(0, 0, -1), _subtitleData(0, 0, -1) {}
    virtual ~IVideoOutputAdapter() {}

    void SetVideoData(const VideoFrame& frame)
    {
        _videoData = frame;
    }

    void SetSubtitleData(const VideoFrame& frame)
    {
        _subtitleData = frame;
    }

    const VideoFrame& GetVideoData() const
    {
        return _videoData;
    }

    const VideoFrame& GetSubtitleData() const
    {
        return _subtitleData;
    }
};