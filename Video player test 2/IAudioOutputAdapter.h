#pragma once

#include "AudioFrame.h"

class IAudioOutputAdapter
{
public:
    IAudioOutputAdapter() {}
    virtual ~IAudioOutputAdapter() {}

    virtual void AddRawData(const AudioFrame& frame) = 0;
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual bool Paused() const = 0;
    virtual void SetVolume(float volume) = 0;
    virtual void SetBalance(float balance) = 0;
    virtual int64_t CurrentTime() const = 0;
    virtual int64_t BufferLength() const = 0;
    virtual int64_t BufferEndTime() const = 0;
};