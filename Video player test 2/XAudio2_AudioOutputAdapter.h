#pragma once

#include "IAudioOutputAdapter.h"

#include "GameTime.h"

#pragma comment( lib,"xaudio2.lib" )
#include <xaudio2.h>

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    struct BufferContext
    {
        int64_t timestamp;
        int64_t sampleDuration;
        void* data;
    };

private:
    int64_t& _audioBufferLength;
    int64_t& _currentSampleTimestamp;
    Clock& _playbackTimer;
    int64_t& _playbackOffset;
public:
    VoiceCallback(
        int64_t& audioBufferLengthRef,
        int64_t& currentSampleTimestamp,
        Clock& clock,
        int64_t& playbackOffset
    ) : _audioBufferLength(audioBufferLengthRef),
        _currentSampleTimestamp(currentSampleTimestamp),
        _playbackTimer(clock),
        _playbackOffset(playbackOffset)
    {}
    ~VoiceCallback() {}

    void OnBufferEnd(void* pBufferContext)
    {
        auto ctx = (BufferContext*)pBufferContext;
        delete ctx->data;
        _audioBufferLength -= ctx->sampleDuration;
        delete ctx;
    }

    void OnBufferStart(void* pBufferContext)
    {
        auto ctx = (BufferContext*)pBufferContext;
        _currentSampleTimestamp = ctx->timestamp;
        _playbackTimer.Update();
        _playbackOffset = _currentSampleTimestamp - _playbackTimer.Now().GetTime();
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
    int64_t _playbackOffset = 0;
    int64_t _offsetTolerance = 1000; // In microseconds

    int _channelCount = 0;
    int _sampleRate = 0;

    float _volume = 0.05f;
    float _balance = 0.0f;

    bool _paused = true;

public:
    XAudio2_AudioOutputAdapter(int channelCount, int sampleRate)
      : _voiceCallback(_audioBufferLength, _currentSampleTimestamp, _playbackTimer, _playbackOffset),
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
        // Audio is ahead
        if (_playbackOffset > _offsetTolerance)
        {
            // Yeah no :^)
        }
        // Actually fuck this synchronization altogether.
        // The solutions are too stupidly complex for the likelihood of this problem
        // and the desync can be fixed by just seeking.
        // Maybe later if this becomes a problem. </rant>

        _audioBufferEnd = frame.GetTimestamp() + (1000000LL * frame.GetSampleCount()) / frame.GetSampleRate();
        _audioFramesBuffered++;

        int64_t sampleDuration = (1000000LL * frame.GetSampleCount()) / frame.GetSampleRate();

        WaveHeader header = frame.GetHeader();
        char* dataBytes = new char[header.subChunk2Size];
        memcpy(dataBytes, frame.GetBytes(), header.subChunk2Size);
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = header.subChunk2Size;
        buffer.pAudioData = (BYTE*)dataBytes;
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        // Setup callback
        VoiceCallback::BufferContext* bCtx = new VoiceCallback::BufferContext();
        bCtx->timestamp = frame.GetTimestamp();
        bCtx->sampleDuration = sampleDuration;
        bCtx->data = dataBytes; // Callback will delete this pointer after finishing playback
        buffer.pContext = bCtx;

        _sourceVoice->SubmitSourceBuffer(&buffer);
        _audioBufferLength += sampleDuration;
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
        else if (_channelCount >= 2)
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