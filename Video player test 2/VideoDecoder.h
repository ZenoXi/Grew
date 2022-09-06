#pragma once

#include "IMediaDecoder.h"

struct AVCodecContext;

class VideoDecoder : public IMediaDecoder
{
    AVCodecContext* _codecContext;

    TimePoint _lastOptionCheck = -1;
    Duration _optionCheckInterval = Duration(1, SECONDS);

public:
    VideoDecoder(const MediaStream& stream);
    ~VideoDecoder();

private:
    void _DecoderThread();
    void _LoadOptions();
};