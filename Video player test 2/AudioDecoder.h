#pragma once

#include "IMediaDecoder.h"

struct AVCodecContext;

class AudioDecoder : public IMediaDecoder
{
    AVCodecContext* _codecContext;

public:
    AudioDecoder(const MediaStream& stream);
    ~AudioDecoder();

    TimePoint _lastOptionCheck = -1;
    Duration _optionCheckInterval = Duration(1, SECONDS);

private:
    void _DecoderThread();
    void _LoadOptions();
};