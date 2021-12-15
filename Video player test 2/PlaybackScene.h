#pragma once

#include "Scene.h"

#include "MediaPlayerOld.h"

#include "MediaPlayer.h"
#include "LocalFileDataProvider.h"
#include "XAudio2_AudioOutputAdapter.h"
#include "IVideoOutputAdapter.h"
#include "BasePlaybackController.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "BottomControlPanel.h"
#include "SeekBar.h"
#include "VolumeSlider.h"
#include "PlayButton.h"
#include "Button.h"

struct PlaybackSceneOptions : public SceneOptionsBase
{
    PlaybackMode mode;

    std::string fileName = "";
    std::string ip = "";
    uint16_t port = 0;
};

class PlaybackScene : public Scene
{
    MediaPlayerOld* _player = nullptr;

    zcom::Panel* _controlBar = nullptr;
    zcom::SeekBar* _seekBar = nullptr;
    zcom::VolumeSlider* _volumeSlider = nullptr;
    zcom::PlayButton* _playButton = nullptr;
    zcom::BottomControlPanel* _bottomControlPanel = nullptr;
    zcom::Button* _overlayButton = nullptr;

    LocalFileDataProvider* _dataProvider = nullptr;
    IAudioOutputAdapter* _audioAdapter = nullptr;
    IVideoOutputAdapter* _videoAdapter = nullptr;
    MediaPlayer* _mediaPlayer = nullptr;
    IPlaybackController* _controller = nullptr;

public:
    PlaybackScene();

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

    const char* GetName() const { return "playback"; }
    static const char* StaticName() { return "playback"; }
};