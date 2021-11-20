#pragma once

#include "NetworkInterface.h"
#include "MediaDecoder.h"

#pragma comment( lib,"xaudio2.lib" )
#include <xaudio2.h>

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    struct BufferEndContext
    {
        long long sampleDuration;
        void* data;
    };

private:
    long long& _audioBufferLength;
public:
    VoiceCallback(long long& audioBufferLengthRef) : _audioBufferLength(audioBufferLengthRef) {}
    ~VoiceCallback() {}

    void OnBufferEnd(void* pBufferContext)
    {
        auto ctx = (BufferEndContext*)pBufferContext;
        delete ctx->data;
        _audioBufferLength -= ctx->sampleDuration;
        //std::cout << "k " << ctx->byteCount << std::endl;
        delete ctx;
    }

    //Unused methods are stubs
    void OnVoiceProcessingPassEnd() {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnBufferStart(void* pBufferContext) {}
    void OnStreamEnd() {}
    void OnLoopEnd(void* pBufferContext) {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) {}
};

class MediaPlayerOld
{
    MediaDecoder* _decoder;
    NetworkInterface* _network;
    PlaybackMode _mode;

    bool _loading;
    bool _clientBuffering;
    bool _waitingForServer;
    bool _waitingForPackets;
    bool _waiting;
    bool _paused;
    bool _flushing = false;
    float _volume;
    FrameData _currentFrame;
    FrameData _skippedFrame;
    bool _skipFinished = false;

    VideoInfo _vinfo;
    AudioInfo _ainfo;

    IXAudio2* _XAudio2 = nullptr;
    IXAudio2MasteringVoice* _masterVoice = nullptr;
    IXAudio2SourceVoice* _sourceVoice = nullptr;
    VoiceCallback _voiceCallback;
    WAVEFORMATEX _wfx = { 0 };
    long long _audioBufferLength = 0;
    TimePoint _audioBufferEnd = 0;
    int _audioFramesBuffered = 0;

public:
    MediaPlayerOld(std::string ip, USHORT port);
    MediaPlayerOld(USHORT port, std::string filename);
    MediaPlayerOld(PlaybackMode mode, std::string filename = "");
    ~MediaPlayerOld() {};

    // Controls
    void Update(double timeLimit = 0.01666666);
    void Play(bool echo = false);
    void Pause(bool echo = false);
    void Seek(TimePoint position, bool echo = false);
    void SetVolume(float volume);

    bool Paused();
    Duration GetDuration();
    float GetVolume();

    bool Buffering();
    bool Loading();
    Duration GetBufferedDuration();
    const FrameData* GetCurrentFrame();

private:
    TimePoint _lastPositionNotification = 0;
    //std::ofstream cAudioOut;
};