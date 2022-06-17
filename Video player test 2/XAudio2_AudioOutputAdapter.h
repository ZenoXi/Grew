#pragma once

#include "IAudioOutputAdapter.h"

#include "GameTime.h"
#include "FixedQueue.h"

#pragma comment( lib,"xaudio2.lib" )
#include <xaudio2.h>

#include <iostream>

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    struct BufferContext
    {
        int64_t timestamp;
        int64_t sampleDuration;
        int64_t correction;
        void* data;
    };

    struct Refs
    {
        int64_t& _audioBufferLength;
        int64_t& _currentSampleTimestamp;
        Clock& _playbackTimer;
        int64_t& _playbackOffset;
        int64_t& _offsetCorrection;
    };

private:
    const size_t _QUEUE_SIZE = 30;
    FixedQueue<int64_t> _offsetQueue;
    int64_t _offsetSum;

    Refs _refs;
public:
    VoiceCallback(Refs refs)
        : _refs(refs),
        _offsetQueue(_QUEUE_SIZE),
        _offsetSum(0)
    {}
    ~VoiceCallback() {}
    void Reset()
    {
        _offsetQueue.Fill(0);
        _offsetSum = 0;
    }

    void OnBufferEnd(void* pBufferContext)
    {
        auto ctx = (BufferContext*)pBufferContext;
        delete ctx->data;
        _refs._audioBufferLength -= ctx->sampleDuration;
        delete ctx;
    }

    void OnBufferStart(void* pBufferContext)
    {
        auto ctx = (BufferContext*)pBufferContext;
        _refs._currentSampleTimestamp = ctx->timestamp;
        _refs._playbackTimer.Update();
        int64_t offset = ctx->timestamp - _refs._playbackTimer.Now().GetTime();
        _offsetSum += offset;
        _offsetSum -= _offsetQueue.Front();
        _offsetQueue.Push(offset);
        _refs._playbackOffset = _offsetSum / (int64_t)_QUEUE_SIZE;
        // Remove correction offset
        _refs._offsetCorrection -= ctx->correction;
    }

    //Unused methods are stubs
    void OnVoiceProcessingPassEnd() {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnStreamEnd() {}
    void OnLoopEnd(void* pBufferContext) {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) {}
};

class XAudio2_AudioOutputAdapter : public IAudioOutputAdapter
{
    IXAudio2* _XAudio2 = nullptr;
    IXAudio2MasteringVoice* _masterVoice = nullptr;
    IXAudio2SourceVoice* _sourceVoice = nullptr;
    VoiceCallback _voiceCallback;
    WAVEFORMATEX _wfx = { 0 };
    int64_t _currentSampleTimestamp = 0;
    int64_t _audioBufferLength = 0;
    int64_t _audioBufferEnd = 0;
    int _audioFramesBuffered = 0;

    Clock _playbackTimer;
    // Time from timer to audio timestamp
    // Positive - audio ahead
    // Negative - audio behind
    int64_t _playbackOffset = 0;
    // Values below ~1500us start causing frequent overcorrections
    // and make the audio sound choppy
    int64_t _offsetTolerance = 5000; // In microseconds
    // Pending offset correction
    // Positive - audio behind (some audio will be cut)
    // Negative - audio ahead (silent padding will be added)
    int64_t _offsetCorrection = 0;
    size_t _cyclesSinceLastCorrection = 0;

    int _channelCount = 0;
    int _sampleRate = 0;

    float _volume = 0.05f;
    float _balance = 0.0f;

    bool _paused = true;

public:
    XAudio2_AudioOutputAdapter(int channelCount = -1, int sampleRate = -1)
        : _voiceCallback({
            _audioBufferLength,
            _currentSampleTimestamp,
            _playbackTimer,
            _playbackOffset,
            _offsetCorrection
        }),
        _channelCount(channelCount),
        _sampleRate(sampleRate)
    {
        // Init XAudio
        HRESULT hr;
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        hr = XAudio2Create(&_XAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
        hr = _XAudio2->CreateMasteringVoice(&_masterVoice);

        Reset(channelCount, sampleRate);
    }
    ~XAudio2_AudioOutputAdapter()
    {
        _XAudio2->Release();
        CoUninitialize();
    }

    void Reset(int channelCount, int sampleRate)
    {
        Pause();

        if (_sourceVoice)
        {
            _sourceVoice->FlushSourceBuffers();
            _sourceVoice->DestroyVoice();
        }

        if (_channelCount == -1 && _sampleRate == -1)
            return;

        _wfx = { 0 };
        _currentSampleTimestamp = 0;
        _audioBufferLength = 0;
        _audioBufferEnd = 0;
        _audioFramesBuffered = 0;

        _playbackOffset = 0;
        _offsetCorrection = 0;
        _cyclesSinceLastCorrection = 0;

        _channelCount = channelCount;
        _sampleRate = sampleRate;

        _wfx.wFormatTag = 1;
        _wfx.nChannels = _channelCount;
        _wfx.nSamplesPerSec = _sampleRate;
        _wfx.nAvgBytesPerSec = _wfx.nChannels * _wfx.nSamplesPerSec * 2;
        _wfx.nBlockAlign = _wfx.nChannels * 2;
        _wfx.wBitsPerSample = 2 * 8;

        HRESULT hr;
        hr = _XAudio2->CreateSourceVoice(&_sourceVoice, &_wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_voiceCallback, NULL, NULL);
        SetVolume(_volume);

        _playbackTimer = Clock();
        _playbackTimer.Stop();
    }

    void AddRawData(const AudioFrame& frame)
    {
        _cyclesSinceLastCorrection++;

        //std::cout << "off: " << _playbackOffset << " | cor: " << _offsetCorrection << std::endl;

        int64_t currentOffset = _playbackOffset + _offsetCorrection;
        int64_t samplesToCut = 0;

        // Audio is ahead
        if (currentOffset > _offsetTolerance && _cyclesSinceLastCorrection >= 60)
        {
            _cyclesSinceLastCorrection = 0;

            // Add a silent chunk (up to 100ms)
            int64_t chunkDuration = currentOffset;
            if (chunkDuration > 100000) chunkDuration = 100000;
            size_t sampleCount = (chunkDuration * (int64_t)frame.GetSampleRate()) / (int64_t)1000000;
            size_t chunkSize = sampleCount * frame.GetChannelCount() * 2;

            //18,446,744,073,709,551,615
            //   614,891,469,123,651,500

            char* dataBytes = new char[chunkSize]();
            XAUDIO2_BUFFER buffer = { 0 };
            buffer.AudioBytes = chunkSize;
            buffer.pAudioData = (BYTE*)dataBytes;
            buffer.Flags = XAUDIO2_END_OF_STREAM;

            // Setup callback
            VoiceCallback::BufferContext* bCtx = new VoiceCallback::BufferContext();
            bCtx->timestamp = frame.GetTimestamp();
            bCtx->sampleDuration = chunkDuration;
            bCtx->data = dataBytes; // Callback will delete this pointer after finishing playback
            int64_t correction = -chunkDuration;
            bCtx->correction = correction;
            // Add correction offset
            _offsetCorrection += correction;
            buffer.pContext = bCtx;

            _sourceVoice->SubmitSourceBuffer(&buffer);

            std::cout << "[XAudio2_AudioOuptutAdapter] SYNC: Added silent chunk of length " << chunkDuration << "us" << std::endl;
        }
        // Audio is behind
        else if (currentOffset < -_offsetTolerance && _cyclesSinceLastCorrection >= 60)
        {
            _cyclesSinceLastCorrection = 0;

            samplesToCut = (-currentOffset * (int64_t)frame.GetSampleRate()) / (int64_t)1000000;
        }

        if (samplesToCut > frame.GetSampleCount())
        {
            samplesToCut = frame.GetSampleCount();
        }
        int64_t sampleDuration = ((int64_t)1000000 * (frame.GetSampleCount() - samplesToCut)) / frame.GetSampleRate();
        size_t cutBytes = samplesToCut * frame.GetChannelCount() * 2;

        WaveHeader header = frame.GetHeader();
        char* dataBytes = new char[header.subChunk2Size - cutBytes];
        memcpy(dataBytes, frame.GetBytes(), header.subChunk2Size - cutBytes);
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = header.subChunk2Size - cutBytes;
        buffer.pAudioData = (BYTE*)dataBytes;
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        // Setup callback
        VoiceCallback::BufferContext* bCtx = new VoiceCallback::BufferContext();
        bCtx->timestamp = frame.GetTimestamp();
        bCtx->sampleDuration = sampleDuration;
        bCtx->data = dataBytes; // Callback will delete this pointer after finishing playback
        int64_t correction = (samplesToCut * (int64_t)1000000) / frame.GetSampleRate();
        bCtx->correction = correction;
        // Add correction offset
        _offsetCorrection += correction;
        buffer.pContext = bCtx;

        if (correction != 0)
        {
            std::cout << "[XAudio2_AudioOuptutAdapter] SYNC: Cut chunk of length " << correction << "us" << std::endl;
        }

        _sourceVoice->SubmitSourceBuffer(&buffer);
        _audioBufferLength += sampleDuration;

        _audioBufferEnd = frame.GetTimestamp() + sampleDuration;
        _audioFramesBuffered++;
    }

    void Play()
    {
        if (_paused)
        {
            _sourceVoice->Start();
            _playbackTimer.Update();
            _playbackTimer.Start();
            _paused = false;
        }
    }

    void Pause()
    {
        if (!_paused)
        {
            _sourceVoice->Stop();
            _playbackTimer.Update();
            _playbackTimer.Stop();
            _paused = true;
        }
    }

    bool Paused() const
    {
        return _paused;
    }

    void SetVolume(float volume)
    {
        if (volume > 1.0f) volume = 1.0f;
        if (volume < 0.0f) volume = 0.0f;
        _volume = volume;

        if (_channelCount == 1)
        {
            _sourceVoice->SetVolume(volume);
            return;
        }
        else if (_channelCount == 2)
        {
            float volumeL = _volume;
            float volumeR = _volume;
            if (_balance < 0.0f)
            {
                volumeR *= 1.0f + _balance;
            }
            if (_balance > 0.0f)
            {
                volumeL *= 1.0f - _balance;
            }
            float volumes[2] = { volumeL, volumeR };
            _sourceVoice->SetChannelVolumes(2, volumes);
            return;
        }
        else if (_channelCount > 2)
        {
            auto volumes = std::make_unique<float[]>(_channelCount);
            for (int i = 0; i < _channelCount; i++)
            {
                volumes[i] = _volume;
            }
            _sourceVoice->SetChannelVolumes(_channelCount, volumes.get());
            return;
        }
    }

    void SetBalance(float balance)
    {
        if (balance > 1.0f) balance = 1.0f;
        if (balance < -1.0f) balance = -1.0f;
        _balance = balance;
        SetVolume(_volume);
    }

    int64_t CurrentTime()
    {
        _playbackTimer.Update();
        return _playbackTimer.Now().GetTime();
    }

    void SetTime(int64_t time)
    {
        _playbackTimer.Update();
        _playbackTimer.SetTime(TimePoint(time, MICROSECONDS));
    }

    int64_t BufferLength() const
    {
        return _audioBufferLength;
    }

    int64_t BufferEndTime() const
    {
        return _audioBufferEnd;
    }
};