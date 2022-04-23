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
#include "PlaybackControllerPanel.h"
#include "SeekBar.h"
#include "VolumeSlider.h"
#include "PlayButton.h"
#include "Button.h"
#include "LoadingCircle.h"
#include "FastForwardIcon.h"
#include "PauseIcon.h"
#include "ResumeIcon.h"
#include "VolumeIcon.h"
#include "MenuPanel.h"

class PlaybackShortcutHandler : public KeyboardEventHandler
{
    bool _OnKeyDown(BYTE vkCode) { return false; }
    bool _OnKeyUp(BYTE vkCode) { return false; }
    bool _OnChar(wchar_t ch) { return false; }
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
    zcom::PlaybackControllerPanel* _playbackControllerPanel = nullptr;
    zcom::Button* _overlayButton = nullptr;
    zcom::LoadingCircle* _loadingCircle = nullptr;
    std::unique_ptr<zcom::FastForwardIcon> _skipBackwardsIcon = nullptr;
    std::unique_ptr<zcom::FastForwardIcon> _skipForwardsIcon = nullptr;
    std::unique_ptr<zcom::PauseIcon> _pauseIcon = nullptr;
    std::unique_ptr<zcom::ResumeIcon> _resumeIcon = nullptr;
    std::unique_ptr<zcom::VolumeIcon> _volumeIcon = nullptr;
    std::unique_ptr<zcom::Panel> _timeHoverPanel = nullptr;
    std::unique_ptr<zcom::Label> _timeLabel = nullptr;
    std::unique_ptr<zcom::Label> _chapterLabel = nullptr;

    std::unique_ptr<zcom::Button> _streamButton = nullptr;
    std::unique_ptr<zcom::MenuPanel> _streamMenuPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _videoStreamMenuPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _audioStreamMenuPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _subtitleStreamMenuPanel = nullptr;

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
    void _SetupStreamMenu();
    bool _HandleKeyDown(BYTE keyCode);
};