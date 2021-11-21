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

public:
    MediaPlayer(
        IMediaDataProvider* dataProvider,
        std::unique_ptr<IVideoOutputAdapter> videoAdapter,
        std::unique_ptr<IAudioOutputAdapter> audioAdapter
    );
    ~MediaPlayer();

private:
    bool _PassPacket(IMediaDecoder* decoder, MediaPacket packet);
public:
    void Update(double timeLimit = 0.01666666);

    void StartTimer();
    void StopTimer();
    TimePoint TimerPosition() const;
    void SetVolume(float volume);
    void SetBalance(float balance);

    // Packets are not being decoded fast enough
    bool Lagging() const;
    // Packets are not being provided fast enough
    bool Buffering() const;
};