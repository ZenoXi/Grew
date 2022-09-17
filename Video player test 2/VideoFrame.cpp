//#include "VideoFrame.h"
//
//#include <algorithm>
//
//VideoFrame::VideoFrame(int width, int height, long long int timestamp, bool first, bool last)
//    :
//    _width(width),
//    _height(height),
//    _timestamp(timestamp),
//    _first(first),
//    _last(last),
//    _bytes(std::make_unique<uchar[]>(width * height * 4))
//{}
//
//VideoFrame::~VideoFrame() {}
//
//VideoFrame::VideoFrame(const VideoFrame& other)
//{
//    _width = other._width;
//    _height = other._height;
//    _timestamp = other._timestamp;
//    _first = other._first;
//    _last = other._last;
//    int size = _width * _height * 4;
//    _bytes = std::make_unique<uchar[]>(size);
//    std::copy_n(other._bytes.get(), size, _bytes.get());
//}
//
//VideoFrame& VideoFrame::operator=(const VideoFrame& other)
//{
//    if (this != &other)
//    {
//        _width = other._width;
//        _height = other._height;
//        _timestamp = other._timestamp;
//        _first = other._first;
//        _last = other._last;
//        int size = _width * _height * 4;
//        _bytes = std::make_unique<uchar[]>(size);
//        std::copy_n(other._bytes.get(), size, _bytes.get());
//    }
//    return *this;
//}
//
//VideoFrame::VideoFrame(VideoFrame&& other) noexcept
//{
//    _width = other._width;
//    _height = other._height;
//    _timestamp = other._timestamp;
//    _first = other._first;
//    _last = other._last;
//    _bytes = std::move(other._bytes);
//    other._width = 0;
//    other._height = 0;
//    other._timestamp = 0;
//    other._first = 0;
//    other._last = 0;
//}
//
//VideoFrame& VideoFrame::operator=(VideoFrame&& other) noexcept
//{
//    if (this != &other)
//    {
//        _width = other._width;
//        _height = other._height;
//        _timestamp = other._timestamp;
//        _first = other._first;
//        _last = other._last;
//        _bytes = std::move(other._bytes);
//        other._width = 0;
//        other._height = 0;
//        other._timestamp = 0;
//        other._first = 0;
//        other._last = 0;
//    }
//    return *this;
//}
//
//const uchar* VideoFrame::GetBytes() const
//{
//    return _bytes.get();
//}
//
//void VideoFrame::SetBytes(const uchar* bytes)
//{
//    std::copy_n(bytes, _width * _height * 4, _bytes.get());
//}
//
//void VideoFrame::ClearBytes()
//{
//    std::fill_n(_bytes.get(), _width * _height * 4, 0);
//}
//
//PixelData VideoFrame::GetPixel(int x, int y) const
//{
//    int index = y * _width * 4 + x * 4;
//    return
//    {
//        _bytes[index + 2],
//        _bytes[index + 1],
//        _bytes[index],
//        _bytes[index + 3]
//    };
//}
//
//void VideoFrame::SetPixel(int x, int y, PixelData pixel)
//{
//    int index = y * _width * 4 + x * 4;
//    _bytes[index] = pixel.b;
//    _bytes[index + 1] = pixel.g;
//    _bytes[index + 2] = pixel.r;
//    _bytes[index + 3] = pixel.a;
//}
//
//int VideoFrame::GetWidth() const
//{
//    return _width;
//}
//
//int VideoFrame::GetHeight() const
//{
//    return _height;
//}
//
//long long int VideoFrame::GetTimestamp() const
//{
//    return _timestamp;
//}
//
//bool VideoFrame::First() const
//{
//    return _first;
//}
//
//bool VideoFrame::Last() const
//{
//    return _last;
//}
