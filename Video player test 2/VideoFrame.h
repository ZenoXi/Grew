#pragma once

#include "IMediaFrame.h"

#include <memory>

typedef unsigned char uchar;

struct PixelData
{
    uchar r, g, b, a;
};

class VideoFrame : IMediaFrame
{
    std::unique_ptr<uchar[]> _bytes;
    int _width;
    int _height;
    long long int _timestamp;
    bool _first;
    bool _last;

public:
    VideoFrame(int width, int height, long long int timestamp, bool first = false, bool last = false);
    ~VideoFrame();
    VideoFrame(const VideoFrame& other);
    VideoFrame& operator=(const VideoFrame& other);
    VideoFrame(VideoFrame&& other) noexcept;
    VideoFrame& operator=(VideoFrame&& other) noexcept;

    const uchar* GetBytes() const;
    void SetBytes(const uchar* bytes);
    void ClearBytes();
    PixelData GetPixel(int x, int y) const;
    void SetPixel(int x, int y, PixelData pixel);
    int GetWidth() const;
    int GetHeight() const;
    long long int GetTimestamp() const;
    bool First() const;
    bool Last() const;
};