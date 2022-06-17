#pragma once

#include "IMediaDataProvider.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "SubtitleDecoder.h"
#include "IVideoOutputAdapter.h"
#include "IAudioOutputAdapter.h"
#include "GameTime.h"

class MediaPlayer
{
    struct MediaData
    {
        IMediaDecoder* decoder = nullptr;
        std::unique_ptr<IMediaFrame> currentFrame = nullptr;
        std::unique_ptr<IMediaFrame> nextFrame = nullptr;

        std::unique_ptr<MediaStream> pendingStream = nullptr;
        bool expectingStream = false;
    };

    IMediaDataProvider* _dataProvider = nullptr;

    MediaData _videoData;
    MediaData _audioData;
    MediaData _subtitleData;

    TimePoint _lastSubtitleRender = 0;

    //IMediaDecoder* _videoDecoder = nullptr;
    //IMediaDecoder* _audioDecoder = nullptr;
    //IMediaDecoder* _subtitleDecoder = nullptr;
    //std::unique_ptr<VideoFrame> _currentVideoFrame = nullptr;
    //std::unique_ptr<AudioFrame> _currentAudioFrame = nullptr;
    //std::unique_ptr<VideoFrame> _nextVideoFrame = nullptr;
    //std::unique_ptr<AudioFrame> _nextAudioFrame = nullptr;
    //std::unique_ptr<VideoFrame> _nextSubtitleFrame = nullptr;

    // Subtitles and video are combined
    IVideoOutputAdapter* _videoOutputAdapter = nullptr;
    IAudioOutputAdapter* _audioOutputAdapter = nullptr;

    //std::unique_ptr<MediaStream> _pendingVideoStream = nullptr;
    //std::unique_ptr<MediaStream> _pendingAudioStream = nullptr;
    //std::unique_ptr<MediaStream> _pendingSubtitleStream = nullptr;
    //bool _expectingVideoStream = false;
    //bool _expectingAudioStream = false;
    //bool _expectingSubtitleStream = false;

    Clock _playbackTimer;

    bool _lagging = false;
    bool _buffering = false;
    bool _skipping = false;
    bool _waiting = false;
    bool _recovering = false;
    bool _recovered = false;

public:
    MediaPlayer(
        IMediaDataProvider* dataProvider,
        IVideoOutputAdapter* videoAdapter,
        IAudioOutputAdapter* audioAdapter
    );
    ~MediaPlayer();

private:
    // 0 - no packet to pass, 1 - packed passed, 2 - flush packet received
    int _PassPacket(MediaData& mediaData, MediaPacket packet);
public:
    void Update(double timeLimit = 0.01666666);

    void StartTimer();
    void StopTimer();
    bool TimerRunning() const;
    void SetTimerPosition(TimePoint time);
    void WaitDiscontinuity();
    TimePoint TimerPosition() const;
    void SetVolume(float volume);
    void SetBalance(float balance);

    void SetVideoStream(std::unique_ptr<MediaStream> stream);
    void SetAudioStream(std::unique_ptr<MediaStream> stream);
    void SetSubtitleStream(std::unique_ptr<MediaStream> stream);
private:
    void _SetStream(MediaData& mediaData, std::unique_ptr<MediaStream> stream);
public:

    // Packets are not being decoded fast enough
    bool Lagging() const;
    // Packets are not being provided fast enough
    bool Buffering() const;
    // Frames are being skipped to catch up to the timer (usually after seeking/changing streams)
    bool Skipping() const;
    // The player is waiting for flush packets; Avoid seeking/changing streams
    bool Waiting() const;

    // After a flush packet is received, the player enters recovery mode until
    // all streams have caught up to the timer. Then this function will return
    // true ONCE.
    bool Recovered();
};