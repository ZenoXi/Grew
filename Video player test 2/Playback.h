#pragma once

#include "IMediaDataProvider.h"
#include "BasePlaybackController.h"
//#include "IVideoOutputAdapter.h"
#include "VideoOutputAdapter.h"
#include "SubtitleOutputAdapter.h"
#include "IAudioOutputAdapter.h"

#include <memory>

class Playback
{
public:
    void Start(std::unique_ptr<IMediaDataProvider> dataProvider, std::unique_ptr<BasePlaybackController> controller);
    void Stop();
    void Update();
    
    bool Initializing() const;
    bool InitFailed() const;
    IPlaybackController* Controller() const;
    IMediaDataProvider* DataProvider() const;
    //IVideoOutputAdapter* VideoAdapter() const;
    VideoOutputAdapter* VideoAdapter() const;
    SubtitleOutputAdapter* SubtitleAdapter() const;
    IAudioOutputAdapter* AudioAdapter() const;

    Playback();
    ~Playback();

private:
    std::unique_ptr<IMediaDataProvider> _dataProvider;
    std::unique_ptr<BasePlaybackController> _controller;
    std::unique_ptr<MediaPlayer> _player;
    bool _initFailed = false;

    std::unique_ptr<VideoOutputAdapter> _videoAdapter = nullptr;
    std::unique_ptr<SubtitleOutputAdapter> _subtitleAdapter = nullptr;
    std::unique_ptr<IAudioOutputAdapter> _audioAdapter = nullptr;
};