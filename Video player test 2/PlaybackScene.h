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

class PlaybackShortcutHandler : public KeyboardEventHandler
{
    void _OnKeyDown(BYTE vkCode) {}
    void _OnKeyUp(BYTE vkCode) {}
    void _OnChar(wchar_t ch) {}
};

struct PlaybackSceneOptions : public SceneOptionsBase
{
    std::string fileName = "";
    std::string ip = "";
    uint16_t port = 0;
    IMediaDataProvider* dataProvider = nullptr;
    PlaybackMode playbackMode = PlaybackMode::OFFLINE;
    bool startPaused = false;
    bool placeholder = false;
};

class PlaybackScene : public Scene
{
    MediaPlayerOld* _player = nullptr;

    PlaybackMode _playbackMode = PlaybackMode::OFFLINE;

    zcom::Panel* _controlBar = nullptr;
    zcom::SeekBar* _seekBar = nullptr;
    zcom::VolumeSlider* _volumeSlider = nullptr;
    zcom::PlayButton* _playButton = nullptr;
    zcom::BottomControlPanel* _bottomControlPanel = nullptr;
    zcom::Button* _overlayButton = nullptr;
    zcom::Button* _audioStreamButton = nullptr;
    zcom::Button* _subtitleStreamButton = nullptr;

    std::unique_ptr<PlaybackShortcutHandler> _shortcutHandler = nullptr;

    IMediaDataProvider* _dataProvider = nullptr;
    IAudioOutputAdapter* _audioAdapter = nullptr;
    IVideoOutputAdapter* _videoAdapter = nullptr;
    MediaPlayer* _mediaPlayer = nullptr;
    IPlaybackController* _controller = nullptr;
    bool _startPaused = false;
    bool _placeholder = false;

    ID2D1Bitmap* _videoFrameBitmap = nullptr;
    ID2D1Bitmap* _subtitleFrameBitmap = nullptr;

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

public:
    // Playback reached the end
    bool Finished() const;

private:
    bool _HandleKeyDown(BYTE keyCode);
};