#pragma once

#include "AudioFrame.h"

class IAudioOutputAdapter
{
public:
    IAudioOutputAdapter() {}
    virtual ~IAudioOutputAdapter() {}

    virtual void Reset(int channelCount, int sampleRate) = 0;
    virtual void AddRawData(const AudioFrame& frame) = 0;
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual bool Paused() const = 0;
    virtual void SetVolume(float volume) = 0;
    virtual void SetBalance(float balance) = 0;
    virtual int64_t CurrentTime() = 0;
    virtual void SetTime(int64_t time) = 0;
    virtual int64_t BufferLength() const = 0;
    virtual int64_t BufferEndTime() const = 0;
};