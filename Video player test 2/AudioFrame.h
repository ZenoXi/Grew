#pragma once

#include "IMediaFrame.h"

#include <cstdint>
#include <memory>

struct WaveHeader
{
    uint32_t chunkID;       // 0x46464952 "RIFF" in little endian
    uint32_t chunkSize;     // 4 + (8 + subChunk1Size) + (8 + subChunk2Size)
    uint32_t format;        // 0x45564157 "WAVE" in little endian

    uint32_t subChunk1ID;   // 0x20746d66 "fmt " in little endian
    uint32_t subChunk1Size; // 16 for PCM
    uint16_t audioFormat;   // 1 for PCM, 3 for EEE floating point , 7 for -law
    uint16_t numChannels;   // 1 for mono, 2 for stereo
    uint32_t sampleRate;    // 8000, 22050, 44100, etc...
    uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;    // numChannels * bitsPerSample/8
    uint16_t bitsPerSample; // number of bits (8 for 8 bits, etc...)

    uint32_t subChunk2ID;   // 0x61746164 "data" in little endian
    uint32_t subChunk2Size; // numSamples * numChannels * bitsPerSample/8 (this is the actual data size in bytes)
};

class AudioFrame : IMediaFrame
{
    std::unique_ptr<char[]> _bytes;
    int _samples;
    int _channels;
    int _sampleRate;
    long long int _timestamp;
    bool _first;
    bool _last;

public:
    AudioFrame(int sampleCount, int channelCount, int sampleRate, long long int timestamp, bool first = false, bool last = false);
    ~AudioFrame();
    AudioFrame(const AudioFrame& fd);
    AudioFrame& operator=(const AudioFrame& fd);
    AudioFrame(AudioFrame&& fd) noexcept;
    AudioFrame& operator=(AudioFrame&& fd) noexcept;

    const char* GetBytes() const;
    std::unique_ptr<char[]> GetBytesWithHeader() const;
    WaveHeader GetHeader() const;
    void SetBytes(const char* bytes);
    void ClearBytes();
    int GetSampleCount() const;
    int GetChannelCount() const;
    int GetSampleRate() const;
    long long int GetTimestamp() const;
    bool First() const;
    bool Last() const;
};