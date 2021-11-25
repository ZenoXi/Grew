#pragma once

#include "IPlaybackController.h"

#include "MediaPlayer.h"
#include "IMediaDataProvider.h"

class BasePlaybackController : public IPlaybackController
{
    MediaPlayer* _player;
    IMediaDataProvider* _dataProvider;

    bool _paused = false;
    bool _loading = true;

    float _volume = 0.1f;
    float _balance = 0.0f;

    std::vector<std::string> _videoStreams;
    std::vector<std::string> _audioStreams;
    std::vector<std::string> _subtitleStreams;

public:
    BasePlaybackController(MediaPlayer* player, IMediaDataProvider* dataProvider);

    void Update();

    void Play();
    void Pause();
    void SetVolume(float volume);
    void SetBalance(float balance);
    void Seek(TimePoint time);
    void SetVideoStream(int index);
    void SetAudioStream(int index);
    void SetSubtitleStream(int index);

    bool Paused();
    float GetVolume() const;
    float GetBalance() const;
    TimePoint CurrentTime() const;
    int CurrentVideoStream() const;
    int CurrentAudioStream() const;
    int CurrentSubtitleStream() const;
    Duration GetDuration() const;

    std::vector<std::string> GetAvailableVideoStreams() const;
    std::vector<std::string> GetAvailableAudioStreams() const;
    std::vector<std::string> GetAvailableSubtitleStreams() const;
};