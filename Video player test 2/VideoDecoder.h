#pragma once

#include "IMediaDecoder.h"

struct AVCodecContext;

class VideoDecoder : public IMediaDecoder
{
    AVCodecContext* _codecContext;

public:
    VideoDecoder(const MediaStream& stream);
    ~VideoDecoder();

private:
    void _DecoderThread();
};