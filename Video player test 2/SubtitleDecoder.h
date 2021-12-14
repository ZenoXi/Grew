#pragma once

#include "IMediaDecoder.h"
#include "VideoFrame.h"
#include "GameTime.h"

extern "C"
{
#include "../libass-0.13.0/libass/ass.h"
}

struct AVCodecContext;

class SubtitleDecoder : public IMediaDecoder
{
    AVCodecContext* _codecContext;
    ASS_Library* _library = nullptr;
    ASS_Renderer* _renderer = nullptr;
    ASS_Track* _track = nullptr;
    std::mutex _m_ass;

    MediaStream _stream;
    int _outputWidth = 0;
    int _outputHeight = 0;

    std::thread _renderingThread;

public:
    struct FontDesc
    {
        char* name;
        char* data;
        size_t dataSize;
    };

    SubtitleDecoder(const MediaStream& stream);
    ~SubtitleDecoder();
    VideoFrame RenderFrame(TimePoint time);
    void AddFonts(const std::vector<FontDesc>& fonts);
    void SetOutputSize(int width, int height);
    int GetOutputWidth() const;
    int GetOutputHeight() const;

private:
    void _DecoderThread();
    void _RenderingThread();

    void _ResetRenderer();
};