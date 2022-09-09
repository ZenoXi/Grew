#pragma once

#include "ComponentBase.h"
#include "Panel.h"
#include "KeyboardEventHandler.h"
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

#include "Playback.h"

namespace zcom
{
    // Completely barebones component, only contains base component functionality
    class PlaybackController : public Panel, public KeyboardEventHandler
    {
    public:
        float GetVolumeSliderValue() const
        {
            return _volumeSlider->GetValue();
        }

    protected:
        friend class Scene;
        friend class Base;
        PlaybackController(Scene* scene) : Panel(scene) {}
        void Init();
    public:
        ~PlaybackController()
        {

        }
        PlaybackController(PlaybackController&&) = delete;
        PlaybackController& operator=(PlaybackController&&) = delete;
        PlaybackController(const PlaybackController&) = delete;
        PlaybackController& operator=(const PlaybackController&) = delete;

    private:
        std::unique_ptr<zcom::SeekBar> _seekBar = nullptr;
        std::unique_ptr<zcom::VolumeSlider> _volumeSlider = nullptr;
        std::unique_ptr<zcom::PlayButton> _playButton = nullptr;
        std::unique_ptr<zcom::Button> _playNextButton = nullptr;
        std::unique_ptr<zcom::Button> _playPreviousButton = nullptr;
        std::unique_ptr<zcom::PlaybackControllerPanel> _playbackControllerPanel = nullptr;
        std::unique_ptr<zcom::Button> _overlayButton = nullptr;
        std::unique_ptr<zcom::Button> _fullscreenButton = nullptr;
        std::unique_ptr<zcom::Panel> _timeHoverPanel = nullptr;
        std::unique_ptr<zcom::Label> _timeLabel = nullptr;
        std::unique_ptr<zcom::Label> _chapterLabel = nullptr;

        std::unique_ptr<zcom::Button> _settingsButton = nullptr;
        bool _streamChangingAllowed = true;
        std::unique_ptr<zcom::MenuPanel> _streamMenuPanel = nullptr;
        std::unique_ptr<zcom::MenuPanel> _videoStreamMenuPanel = nullptr;
        std::unique_ptr<zcom::MenuPanel> _audioStreamMenuPanel = nullptr;
        std::unique_ptr<zcom::MenuPanel> _subtitleStreamMenuPanel = nullptr;

        Playback* _playback;
        bool _playbackLoaded = false;
        bool _streamMenuSetup = false;
        bool _chaptersSet = false;

        bool _inFullscreen = false;
        void _UpdateFullscreenButton(bool force = false);

        void _UpdatePermissions();
        void _SetupStreamMenu();

#pragma region base_class
    protected:
        void _OnUpdate();
        bool _OnHotkey(int id) { return false; }
        bool _OnKeyDown(BYTE vkCode);
        bool _OnKeyUp(BYTE vkCode) { return false; }
        bool _OnChar(wchar_t ch) { return false; }
    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "playback_controller"; }
#pragma endregion
    };
}