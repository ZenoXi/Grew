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
    IMediaDataProvider* _dataProvider = nullptr;

    IMediaDecoder* _videoDecoder = nullptr;
    IMediaDecoder* _audioDecoder = nullptr;
    IMediaDecoder* _subtitleDecoder = nullptr;
    std::unique_ptr<VideoFrame> _currentVideoFrame = nullptr;
    std::unique_ptr<AudioFrame> _currentAudioFrame = nullptr;
    std::unique_ptr<VideoFrame> _nextVideoFrame = nullptr;
    std::unique_ptr<AudioFrame> _nextAudioFrame = nullptr;
    //std::unique_ptr<VideoFrame> _nextSubtitleFrame = nullptr;

    // Subtitles and video are combined
    std::unique_ptr<IVideoOutputAdapter> _videoOutputAdapter = nullptr;
    std::unique_ptr<IAudioOutputAdapter> _audioOutputAdapter = nullptr;

    Clock _playbackTimer;

    bool _lagging = false;
    bool _buffering = false;
    bool _skipping = false;

    bool _recovering = false;
    bool _recovered = false;

public:
    MediaPlayer(
        IMediaDataProvider* dataProvider,
        std::unique_ptr<IVideoOutputAdapter> videoAdapter,
        std::unique_ptr<IAudioOutputAdapter> audioAdapter
    );
    ~MediaPlayer();

private:
    // 0 - no packet to pass, 1 - packed passed, 2 - flush packet received
    int _PassPacket(IMediaDecoder* decoder, MediaPacket packet);
public:
    void Update(double timeLimit = 0.01666666);

    void StartTimer();
    void StopTimer();
    bool TimerRunning() const;
    void SetTimerPosition(TimePoint time);
    TimePoint TimerPosition() const;
    void SetVolume(float volume);
    void SetBalance(float balance);

    // Packets are not being decoded fast enough
    bool Lagging() const;
    // Packets are not being provided fast enough
    bool Buffering() const;
    // Frames are being skipped to catch up to the timer (usually after seeking/changing streams)
    bool Skipping() const;

    // After a flush packet is received, the player enters recovery mode until
    // all streams have caught up to the timer. Then this function will return
    // true ONCE.
    bool Recovered();
};