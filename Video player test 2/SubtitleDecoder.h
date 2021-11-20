#pragma once

#include "IMediaDecoder.h"

struct AVCodecContext;

class SubtitleDecoder : public IMediaDecoder
{
    AVCodecContext* _codecContext;

public:
    SubtitleDecoder(const MediaStream& stream);
    ~SubtitleDecoder();

private:
    void _DecoderThread();
};