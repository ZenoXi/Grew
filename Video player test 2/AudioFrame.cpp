#include "AudioFrame.h"

#include <algorithm>

AudioFrame::AudioFrame(int sampleCount, int channelCount, int sampleRate, long long int timestamp, bool first, bool last)
    : _samples(sampleCount),
    _channels(channelCount),
    _sampleRate(sampleRate),
    _timestamp(timestamp),
    _first(first),
    _last(last),
    _bytes(std::make_unique<char[]>(sampleCount * channelCount * 2))
{}

AudioFrame::~AudioFrame() {}

AudioFrame::AudioFrame(const AudioFrame& ad)
{
    _samples = ad._samples;
    _channels = ad._channels;
    _sampleRate = ad._sampleRate;
    _timestamp = ad._timestamp;
    int size = _samples * _channels * 2;
    _bytes = std::make_unique<char[]>(size);
    std::copy(ad._bytes.get(), ad._bytes.get() + size, _bytes.get());
}

AudioFrame& AudioFrame::operator=(const AudioFrame& ad)
{
    if (this != &ad)
    {
        _samples = ad._samples;
        _channels = ad._channels;
        _sampleRate = ad._sampleRate;
        _timestamp = ad._timestamp;
        int size = _samples * _channels * 2;
        _bytes = std::make_unique<char[]>(size);
        std::copy(ad._bytes.get(), ad._bytes.get() + size, _bytes.get());
    }
    return *this;
}

AudioFrame::AudioFrame(AudioFrame&& ad) noexcept
{
    _samples = ad._samples;
    _channels = ad._channels;
    _sampleRate = ad._sampleRate;
    _timestamp = ad._timestamp;
    _bytes = std::move(ad._bytes);
    ad._samples = 0;
    ad._channels = 0;
    ad._sampleRate = 0;
    ad._timestamp = 0;
}

AudioFrame& AudioFrame::operator=(AudioFrame&& ad) noexcept
{
    if (this != &ad)
    {
        _samples = ad._samples;
        _channels = ad._channels;
        _sampleRate = ad._sampleRate;
        _timestamp = ad._timestamp;
        _bytes = std::move(ad._bytes);
        ad._samples = 0;
        ad._channels = 0;
        ad._sampleRate = 0;
        ad._timestamp = 0;
    }
    return *this;
}

const char* AudioFrame::GetBytes() const
{
    return _bytes.get();
}

std::unique_ptr<char[]> AudioFrame::GetBytesWithHeader() const
{
    int dataSize = _samples * _channels * 2;
    auto fullData = std::make_unique<char[]>(dataSize + sizeof(WaveHeader));

    WaveHeader header;
    header.chunkID = 0x46464952;
    header.format = 0x45564157;
    header.subChunk1ID = 0x20746d66;
    header.subChunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = _channels;
    header.sampleRate = _sampleRate;
    header.byteRate = header.sampleRate * header.numChannels * 2;
    header.blockAlign = header.numChannels * 2;
    header.bitsPerSample = 2 * 8;
    header.subChunk2ID = 0x61746164;
    header.subChunk2Size = dataSize;
    header.chunkSize = 4 + (8 + header.subChunk1Size) + (8 + header.subChunk2Size);

    std::copy_n((char*)&header, sizeof(WaveHeader), fullData.get());
    std::copy_n(_bytes.get(), dataSize, fullData.get() + sizeof(WaveHeader));
    return fullData;
}

WaveHeader AudioFrame::GetHeader() const
{
    int dataSize = _samples * _channels * 2;

    WaveHeader header;
    header.chunkID = 0x46464952;
    header.format = 0x45564157;
    header.subChunk1ID = 0x20746d66;
    header.subChunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = _channels;
    header.sampleRate = _sampleRate;
    header.byteRate = header.sampleRate * header.numChannels * 2;
    header.blockAlign = header.numChannels * 2;
    header.bitsPerSample = 2 * 8;
    header.subChunk2ID = 0x61746164;
    header.subChunk2Size = dataSize;
    header.chunkSize = 4 + (8 + header.subChunk1Size) + (8 + header.subChunk2Size);

    return header;
}

void AudioFrame::SetBytes(const char* bytes)
{
    std::copy_n(bytes, _samples * _channels * 2, _bytes.get());
}

void AudioFrame::ClearBytes()
{
    std::fill_n(_bytes.get(), _samples * _channels * 2, 0);
}

int AudioFrame::GetSampleCount() const
{
    return _samples;
}

int AudioFrame::GetChannelCount() const
{
    return _channels;
}

int AudioFrame::GetSampleRate() const
{
    return _sampleRate;
}

long long int AudioFrame::GetTimestamp() const
{
    return _timestamp;
}

bool AudioFrame::First() const
{
    return _first;
}

bool AudioFrame::Last() const
{
    return _last;
}

Duration AudioFrame::CalculateDuration() const
{
    return Duration((_samples * 1000000) / _sampleRate, MICROSECONDS);
}