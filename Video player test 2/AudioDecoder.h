#pragma once

#include "IMediaDecoder.h"

struct AVCodecContext;

class AudioDecoder : public IMediaDecoder
{
    AVCodecContext* _codecContext;

public:
    AudioDecoder(const MediaStream& stream);
    ~AudioDecoder();

private:
    void _DecoderThread();
};