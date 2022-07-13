#pragma once

#include "AudioFrame.h"

class IAudioOutputAdapter
{
public:
    struct SampleData
    {
        int16_t data[8]{ 0 };
        int32_t channels = 0;
        int32_t sampleRate = 0;

        SampleData() {}
        SampleData(const SampleData& other)
        {
            channels = other.channels;
            sampleRate = other.sampleRate;
            for (int i = 0; i < 8; i++)
            {
                data[i] = other.data[i];
            }
        }
        SampleData(SampleData&& other) noexcept
        {
            channels = other.channels;
            sampleRate = other.sampleRate;
            other.channels = 0;
            other.sampleRate = 0;
            for (int i = 0; i < 8; i++)
            {
                data[i] = other.data[i];
                other.data[i] = 0;
            }
        }
        SampleData& operator= (const SampleData& other)
        {
            channels = other.channels;
            sampleRate = other.sampleRate;
            for (int i = 0; i < 8; i++)
            {
                data[i] = other.data[i];
            }
            return *this;
        }
        SampleData& operator= (SampleData&& other) noexcept
        {
            channels = other.channels;
            sampleRate = other.sampleRate;
            other.channels = 0;
            other.sampleRate = 0;
            for (int i = 0; i < 8; i++)
            {
                data[i] = other.data[i];
                other.data[i] = 0;
            }
            return *this;
        }
    };

    IAudioOutputAdapter() {}
    virtual ~IAudioOutputAdapter() {}

    virtual void Reset(int channelCount, int sampleRate) = 0;
    virtual void AddRawData(const AudioFrame& frame) = 0;
    virtual std::vector<SampleData> GetRecentSampleData() = 0;
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