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
#include "Image.h"
#include "PlaybackControllerPanel.h"
#include "SeekBar.h"
#include "ControlBarBackground.h"
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

};

class PlaybackScene : public Scene
{
    MediaPlayerOld* _player = nullptr;

    zcom::Panel* _controlBar = nullptr;
    
    std::unique_ptr<zcom::ControlBarBackground> _controlBarBackground = nullptr;
    std::unique_ptr<zcom::SeekBar> _seekBar = nullptr;
    std::unique_ptr<zcom::VolumeSlider> _volumeSlider = nullptr;
    std::unique_ptr<zcom::PlayButton> _playButton = nullptr;
    std::unique_ptr<zcom::Button> _playNextButton = nullptr;
    std::unique_ptr<zcom::Button> _playPreviousButton = nullptr;
    std::unique_ptr<zcom::PlaybackControllerPanel> _playbackControllerPanel = nullptr;
    std::unique_ptr<zcom::Button> _overlayButton = nullptr;
    std::unique_ptr<zcom::Button> _fullscreenButton = nullptr;
    std::unique_ptr<zcom::LoadingCircle> _loadingCircle = nullptr;
    std::unique_ptr<zcom::FastForwardIcon> _skipBackwardsIcon = nullptr;
    std::unique_ptr<zcom::FastForwardIcon> _skipForwardsIcon = nullptr;
    std::unique_ptr<zcom::PauseIcon> _pauseIcon = nullptr;
    std::unique_ptr<zcom::ResumeIcon> _resumeIcon = nullptr;
    std::unique_ptr<zcom::VolumeIcon> _volumeIcon = nullptr;
    std::unique_ptr<zcom::Panel> _timeHoverPanel = nullptr;
    std::unique_ptr<zcom::Label> _timeLabel = nullptr;
    std::unique_ptr<zcom::Label> _chapterLabel = nullptr;

    std::unique_ptr<zcom::Button> _settingsButton = nullptr;
    std::unique_ptr<zcom::MenuPanel> _streamMenuPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _videoStreamMenuPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _audioStreamMenuPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _subtitleStreamMenuPanel = nullptr;

    std::unique_ptr<PlaybackShortcutHandler> _shortcutHandler = nullptr;

    Playback* _playback;
    bool _streamMenuSetup = false;
    bool _chaptersSet = false;

    // Common canvas to which UI and playback is drawn
    ID2D1Bitmap1* _ccanvas = nullptr;
    bool _redraw = false;

    ID2D1Bitmap* _videoFrameBitmap = nullptr;
    ID2D1Bitmap* _subtitleFrameBitmap = nullptr;
    bool _videoFrameChanged = false;
    bool _subtitleFrameChanged = false;

public:
    PlaybackScene();

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    bool _Redraw();
    ID2D1Bitmap* _Draw(Graphics g);
    ID2D1Bitmap* _Image();
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