#include "PlaybackController.h"

#include "App.h"
#include "OverlayScene.h"
#include "PlaybackOverlayScene.h"
#include "Network.h"

#include "Permissions.h"
#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"
#include "FileTypes.h"

zcom::PlaybackController::PlaybackController(Scene* scene)
    : Panel(scene)
{
    _inputSourcesChangedReceiver = std::make_unique<EventReceiver<InputSourcesChangedEvent>>(&App::Instance()->events);
}

void zcom::PlaybackController::Init()
{
    _playback = &_scene->GetApp()->playback;

    _seekBar = Create<zcom::SeekBar>();
    _seekBar->SetParentWidthPercent(1.0f);
    _seekBar->SetBaseWidth(-20);
    _seekBar->SetHorizontalOffsetPercent(0.5f);
    _seekBar->SetBaseHeight(16);
    _seekBar->SetVerticalAlignment(zcom::Alignment::END);
    _seekBar->SetVerticalOffsetPixels(-35);
    _seekBar->AddOnTimeHovered([&](int xpos, TimePoint time, std::wstring chapterName)
    {
        int totalWidth = 0;

        // Time label
        _timeLabel->SetText(string_to_wstring(TimeToString(time)));
        _timeLabel->SetBaseWidth(std::ceil(_timeLabel->GetTextWidth()));
        totalWidth += _timeLabel->GetBaseWidth() + 10;

        // Chapter label
        _chapterLabel->SetVisible(false);
        if (!chapterName.empty())
        {
            _chapterLabel->SetText(chapterName);
            _chapterLabel->SetBaseWidth(std::ceil(_chapterLabel->GetTextWidth()));
            totalWidth += _chapterLabel->GetBaseWidth() + 5;
            _chapterLabel->SetVisible(true);
        }

        _timeHoverPanel->SetBaseWidth(totalWidth);
        //_timeHoverPanel->SetWidth(totalWidth);
        //_timeHoverPanel->Resize();

        //int absolutePos = _playbackControllerPanel->GetX() + _controlBar->GetX() + _seekBar->GetX() + xpos;
        int absolutePos = _seekBar->GetScreenX() + xpos;
        _timeHoverPanel->SetHorizontalOffsetPixels(absolutePos - _timeHoverPanel->GetWidth() / 2);
        _timeHoverPanel->SetVisible(true);

        _scene->GetApp()->Overlay()->RemoveItem(_timeHoverPanel.get());
        _scene->GetApp()->Overlay()->AddItem(_timeHoverPanel.get());
        //_canvas->Resize();
    });
    _seekBar->AddOnHoverEnded([&]()
    {
        _scene->GetApp()->Overlay()->RemoveItem(_timeHoverPanel.get());
        _timeHoverPanel->SetVisible(false);
    });

    _timeHoverPanel = Create<zcom::Panel>();
    _timeHoverPanel->SetBaseSize(120, 20);
    _timeHoverPanel->SetVerticalAlignment(zcom::Alignment::END);
    _timeHoverPanel->SetVerticalOffsetPixels(-55);
    _timeHoverPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _timeHoverPanel->SetBorderVisibility(true);
    _timeHoverPanel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _timeHoverPanel->SetVisible(false);
    zcom::PROP_Shadow timePanelShadow;
    timePanelShadow.blurStandardDeviation = 2.0f;
    timePanelShadow.color = D2D1::ColorF(0);
    _timeHoverPanel->SetProperty(timePanelShadow);

    _timeLabel = Create<zcom::Label>(L"");
    _timeLabel->SetHorizontalOffsetPixels(5);
    _timeLabel->SetParentHeightPercent(1.0f);
    _timeLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _timeLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _timeLabel->SetFontColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));

    _chapterLabel = Create<zcom::Label>(L"");
    _chapterLabel->SetHorizontalOffsetPixels(-5);
    _chapterLabel->SetParentHeightPercent(1.0f);
    _chapterLabel->SetHorizontalAlignment(zcom::Alignment::END);
    _chapterLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _chapterLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _chapterLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _chapterLabel->SetFontColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    zcom::PROP_Shadow chapterLabelShadow;
    chapterLabelShadow.blurStandardDeviation = 1.0f;
    chapterLabelShadow.offsetX = 1.0f;
    chapterLabelShadow.offsetY = 1.0f;
    chapterLabelShadow.color = D2D1::ColorF(0);
    _chapterLabel->SetProperty(chapterLabelShadow);

    _timeHoverPanel->AddItem(_timeLabel.get());
    _timeHoverPanel->AddItem(_chapterLabel.get());

    _volumeSlider = Create<zcom::VolumeSlider>(0.0f);
    _volumeSlider->SetBaseWidth(30);
    _volumeSlider->SetBaseHeight(30);
    _volumeSlider->SetHorizontalOffsetPixels(80);
    _volumeSlider->SetVerticalAlignment(zcom::Alignment::END);
    _volumeSlider->SetVerticalOffsetPixels(-5);
    std::wstring optStr = Options::Instance()->GetValue(OPTIONS_VOLUME);
    float volume = FloatOptionAdapter(optStr, 0.2f).Value();
    _volumeSlider->SetVolume(volume);

    _playButton = Create<zcom::PlayButton>();
    _playButton->SetBaseWidth(30);
    _playButton->SetBaseHeight(30);
    _playButton->SetHorizontalOffsetPercent(0.5f);
    _playButton->SetVerticalAlignment(zcom::Alignment::END);
    _playButton->SetVerticalOffsetPixels(-5);
    _playButton->SetPaused(true);

    _playNextButton = Create<zcom::Button>(L"");
    _playNextButton->SetBaseSize(30, 30);
    _playNextButton->SetHorizontalOffsetPercent(0.5f);
    _playNextButton->SetHorizontalOffsetPixels(40);
    _playNextButton->SetVerticalAlignment(zcom::Alignment::END);
    _playNextButton->SetVerticalOffsetPixels(-5);
    _playNextButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _playNextButton->SetButtonImageAll(ResourceManager::GetImage("play_next_20x20"));
    _playNextButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
    _playNextButton->UseImageParamsForAll(_playNextButton->ButtonImage());
    _playNextButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _playNextButton->SetSelectable(false);
    _playNextButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _playNextButton->SetOnActivated([&]()
    {
        // Find currently playing item
        auto readyItems = _scene->GetApp()->playlist.ReadyItems();
        for (int i = 0; i < readyItems.size(); i++)
        {
            if (readyItems[i]->GetItemId() == _scene->GetApp()->playlist.CurrentlyPlaying())
            {
                if (i < readyItems.size() - 1)
                    _scene->GetApp()->playlist.Request_PlayItem(readyItems[i + 1]->GetItemId());
                else
                    _scene->GetApp()->playlist.Request_PlayItem(readyItems[0]->GetItemId());
                break;
            }
        }
    });

    _playPreviousButton = Create<zcom::Button>(L"");
    _playPreviousButton->SetBaseSize(30, 30);
    _playPreviousButton->SetHorizontalOffsetPercent(0.5f);
    _playPreviousButton->SetHorizontalOffsetPixels(-40);
    _playPreviousButton->SetVerticalAlignment(zcom::Alignment::END);
    _playPreviousButton->SetVerticalOffsetPixels(-5);
    _playPreviousButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _playPreviousButton->SetButtonImageAll(ResourceManager::GetImage("play_previous_20x20"));
    _playPreviousButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
    _playPreviousButton->UseImageParamsForAll(_playPreviousButton->ButtonImage());
    _playPreviousButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _playPreviousButton->SetSelectable(false);
    _playPreviousButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _playPreviousButton->SetOnActivated([&]()
    {
        // Find currently playing item
        auto readyItems = _scene->GetApp()->playlist.ReadyItems();
        for (int i = 0; i < readyItems.size(); i++)
        {
            if (readyItems[i]->GetItemId() == _scene->GetApp()->playlist.CurrentlyPlaying())
            {
                // Play previous if available
                if (i > 0)
                    _scene->GetApp()->playlist.Request_PlayItem(readyItems[i - 1]->GetItemId());
                else
                    _scene->GetApp()->playlist.Request_PlayItem(readyItems.back()->GetItemId());
                break;
            }
        }
    });

    _overlayButton = Create<zcom::Button>(L"");
    _overlayButton->SetBaseSize(30, 30);
    _overlayButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _overlayButton->SetOffsetPixels(-120, -5);
    _overlayButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _overlayButton->SetButtonImageAll(ResourceManager::GetImage("playlist_22x22"));
    _overlayButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
    _overlayButton->UseImageParamsForAll(_overlayButton->ButtonImage());
    _overlayButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _overlayButton->SetSelectable(false);
    _overlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _overlayButton->SetOnActivated([&]()
    {
        Scene* scene = _scene->GetApp()->FindActiveScene(PlaybackOverlayScene::StaticName());
        if (!scene->Focused())
            _scene->GetApp()->MoveSceneToFront(PlaybackOverlayScene::StaticName());
        else
            _scene->GetApp()->MoveSceneToBack(PlaybackOverlayScene::StaticName());
    });

    _settingsButton = Create<zcom::Button>(L"");
    _settingsButton->SetBaseSize(30, 30);
    _settingsButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _settingsButton->SetOffsetPixels(-160, -5);
    _settingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _settingsButton->SetButtonImageAll(ResourceManager::GetImage("settings_22x22"));
    _settingsButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
    _settingsButton->UseImageParamsForAll(_settingsButton->ButtonImage());
    _settingsButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _settingsButton->SetSelectable(false);
    _settingsButton->SetOnActivated([&]()
    {
        if (!_playback->Initializing())
        {
            _SetupStreamMenu();
        }
        else
        {
            _videoStreamMenuPanel->ClearItems();
            _audioStreamMenuPanel->ClearItems();
            _subtitleStreamMenuPanel->ClearItems();
        }

        RECT buttonRect = {
            _settingsButton->GetScreenX(),
            _settingsButton->GetScreenY(),
            _settingsButton->GetScreenX(),// + _streamButton->GetWidth(),
            _settingsButton->GetScreenY()// + _streamButton->GetHeight()
        };
        _scene->GetApp()->Overlay()->ShowMenu(_streamMenuPanel.get(), buttonRect);
    });

    _streamMenuPanel = Create<zcom::MenuPanel>();
    _videoStreamMenuPanel = Create<zcom::MenuPanel>();
    _audioStreamMenuPanel = Create<zcom::MenuPanel>();
    _subtitleStreamMenuPanel = Create<zcom::MenuPanel>();

    _streamMenuPanel->SetBaseWidth(150);
    _streamMenuPanel->SetZIndex(255);
    _ResetStreamMenu();

    _fullscreenButton = Create<zcom::Button>(L"");
    _fullscreenButton->SetBaseSize(30, 30);
    _fullscreenButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _fullscreenButton->SetOffsetPixels(-80, -5);
    _fullscreenButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _fullscreenButton->SetButtonImageAll(ResourceManager::GetImage("fullscreen_on_22x22"));
    _fullscreenButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
    _fullscreenButton->UseImageParamsForAll(_fullscreenButton->ButtonImage());
    _fullscreenButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _fullscreenButton->SetSelectable(false);
    _fullscreenButton->SetOnActivated([&]()
    {
        if (_scene->GetApp()->Fullscreen())
        {
            _scene->GetApp()->Fullscreen(false);
            _fullscreenButton->SetButtonImageAll(ResourceManager::GetImage("fullscreen_on_22x22"));
        }
        else
        {
            _scene->GetApp()->Fullscreen(true);
            _fullscreenButton->SetButtonImageAll(ResourceManager::GetImage("fullscreen_off_22x22"));
        }
    });

    _inFullscreen = _scene->GetApp()->Fullscreen();
    _UpdateFullscreenButton(true);

    AddItem(_seekBar.get());
    AddItem(_volumeSlider.get());
    AddItem(_playButton.get());
    AddItem(_playNextButton.get());
    AddItem(_playPreviousButton.get());
    AddItem(_overlayButton.get());
    AddItem(_settingsButton.get());
    AddItem(_fullscreenButton.get());
}

zcom::PlaybackController::~PlaybackController()
{
    ClearItems();

    _scene->GetApp()->Overlay()->RemoveItem(_timeHoverPanel.get());
    _timeHoverPanel->ClearItems();

    _ResetStreamMenu();
}

void zcom::PlaybackController::_OnUpdate()
{
    if (!_playback->Initializing())
    {
        _playbackLoaded = true;

        // Check for file dialog finish
        if (_fileDialog && _fileDialog->Done())
        {
            auto path = _fileDialog->Result();
            if (!path.empty())
            {
                _playback->DataProvider()->AddLocalMedia(wstr_to_utf8(path), STREAM_SELECTION_SUBTITLE);
            }
            _addingSubtitles = false;
            _fileDialog.reset();
        }

        // Check if input sources changed
        if (_inputSourcesChangedReceiver->EventCount() > 0)
        {
            _inputSourcesChangedReceiver->GetEvent();
            _SetupStreamMenu();
            _streamMenuSetup = true;
        }

        // Set up stream menu
        if (!_streamMenuSetup)
        {
            _SetupStreamMenu();
            _streamMenuSetup = true;
        }

        // Prevent screen turning off while playing
        if (!_playback->Controller()->Paused())
            App::Instance()->window.ResetScreenTimer();

        // Check for play button click
        if (_playback->Controller()->Paused() != _playButton->GetPaused())
        {
            if (_playButton->Clicked())
            {
                if (_playButton->GetPaused())
                {
                    _playback->Controller()->Pause();
                }
                else
                {
                    _playback->Controller()->Play();
                }
            }
            else
            {
                _playButton->SetPaused(_playback->Controller()->Paused());
            }
        }

        // Check for seekbar click
        TimePoint seekTo = _seekBar->SeekTime();
        if (seekTo.GetTicks() != -1)
        {
            _playback->Controller()->Seek(seekTo);
        }

        // Check for volume slider click
        // Custom equality function is used because volume->value->volume conversion
        // results in a very small value difference which the float != operator picks up
        if (!is_equal_f(_volumeSlider->GetVolume(), _playback->Controller()->GetVolume()))
        {
            if (_volumeSlider->Moved())
            {
                _playback->Controller()->SetVolume(_volumeSlider->GetVolume());
            }
            else
            {
                _volumeSlider->SetVolume(_playback->Controller()->GetVolume());
            }
        }

        // Update seekbar
        Duration bufferedDuration = _playback->Controller()->GetBufferedDuration();
        if (bufferedDuration.GetTicks() != -1)
        {
            _seekBar->SetBufferedDuration(bufferedDuration);
        }
        _seekBar->SetCurrentTime(_playback->Controller()->CurrentTime());
        _seekBar->SetDuration(_playback->DataProvider()->MediaDuration());
        if (!_chaptersSet)
        {
            _seekBar->SetChapters(_playback->DataProvider()->GetChapters());
            _chaptersSet = true;
        }
    }
    else if (_playbackLoaded)
    {
        _playbackLoaded = false;

        // Reset seekbar
        _seekBar->SetDuration(-1);
        _seekBar->SetBufferedDuration(-1);
        _seekBar->SetCurrentTime(0);
        _seekBar->SetChapters({});

        // Reset stream menu
        _ResetStreamMenu();

        // Set play button state
        _playButton->Clicked();
        _playButton->SetPaused(true);
    }

    if (!_playback->Controller())
    {
        // Use play button to start playback while no playback is in progress
        if (_playButton->Clicked() && !_playButton->GetPaused())
        {
            if (_scene->GetApp()->playlist.CurrentlyStarting() == -1)
            {
                auto readyItems = _scene->GetApp()->playlist.ReadyItems();
                if (!readyItems.empty())
                    _scene->GetApp()->playlist.Request_PlayItem(readyItems[0]->GetItemId());
            }
        }
    }

    _UpdateFullscreenButton();
    _UpdateButtonAppearances();
    _UpdatePermissions();

    Panel::_OnUpdate();
}

void zcom::PlaybackController::_ShowSubtitleOpenDialog()
{
    if (_fileDialog)
        return;

    _addingSubtitles = true;
    _fileDialog = std::make_unique<AsyncFileDialog>();
    FileDialogOptions opt;
    opt.allowedExtensions = {
        { L"Subtitle files", L"" EXTENSIONS_SUBTITLES },
        { L"All files", L"*.*" }
    };
    _fileDialog->Open(opt);
}

void zcom::PlaybackController::_UpdateFullscreenButton(bool force)
{
    if (force || _inFullscreen != _scene->GetApp()->Fullscreen())
    {
        _inFullscreen = _scene->GetApp()->Fullscreen();
        if (_inFullscreen)
            _fullscreenButton->SetButtonImageAll(ResourceManager::GetImage("fullscreen_off_22x22"));
        else
            _fullscreenButton->SetButtonImageAll(ResourceManager::GetImage("fullscreen_on_22x22"));
    }
}

void zcom::PlaybackController::_UpdateButtonAppearances()
{
    // Play button
    bool playButtonActive = true;
    if (!_playback->Controller())
    {
        // Gray out if user has no permission to start playback
        const User* thisUser = _scene->GetApp()->users.GetThisUser();
        if (thisUser && !thisUser->GetPermission(PERMISSION_START_STOP_PLAYBACK))
            playButtonActive = false;

        // Gray out if there are no ready items in playlist
        auto readyItems = _scene->GetApp()->playlist.ReadyItems();
        if (readyItems.empty())
            playButtonActive = false;

        // Gray out if playback is starting
        if (_scene->GetApp()->playlist.CurrentlyStarting() != -1)
            playButtonActive = false;
    }
    else
    {
        // Gray out if user has no permission to play/pause
        const User* thisUser = _scene->GetApp()->users.GetThisUser();
        if (thisUser && !thisUser->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            playButtonActive = false;
    }
    _playButton->SetActive(playButtonActive);

    // Play next/previous buttons
    bool playNextPrevActive = true;
    if (_playback->Initializing())
    {
        // Gray out if no playback is currently in progress
        playNextPrevActive = false;
    }
    else
    {
        // Gray out if user has no permission to start playback
        const User* thisUser = _scene->GetApp()->users.GetThisUser();
        if (thisUser && !thisUser->GetPermission(PERMISSION_START_STOP_PLAYBACK))
            playNextPrevActive = false;
    }
    _playNextButton->SetActive(playNextPrevActive);
    _playPreviousButton->SetActive(playNextPrevActive);

    // Playlist button
    if (_scene->GetApp()->FindActiveScene(PlaybackScene::StaticName()))
        _overlayButton->SetActive(true);
    else
        _overlayButton->SetActive(false);
}

void zcom::PlaybackController::_UpdatePermissions()
{
    auto user = _scene->GetApp()->users.GetThisUser();

    // This function only disables certain buttons, never enables
    // This is because previously called functions can disable certain buttons
    // which cannot be functionally active under any permission level

    // Play button
    bool allowPlaybackManipulation = user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK);
    if (!allowPlaybackManipulation)
        _playButton->SetActive(false);

    // Play next/previous
    bool allowPlaybackStartStop = user->GetPermission(PERMISSION_START_STOP_PLAYBACK);
    if (!allowPlaybackStartStop)
    {
        _playNextButton->SetActive(false);
        _playPreviousButton->SetActive(false);
    }

    // Change streams
    bool allowStreamChanging = user->GetPermission(PERMISSION_CHANGE_STREAMS);
    if (!allowStreamChanging)
    {
        for (int i = 0; i < _streamMenuPanel->ItemCount(); i++)
        {
            // Slightly janky way to disable the correct menu items
            zcom::MenuItem* item = _streamMenuPanel->GetItem(i);
            if (item->GetMenuPanel() == _videoStreamMenuPanel.get() ||
                item->GetMenuPanel() == _audioStreamMenuPanel.get() ||
                item->GetMenuPanel() == _subtitleStreamMenuPanel.get())
            {
                item->SetDisabled(true);
            }
        }
    }
}

void zcom::PlaybackController::_ResetStreamMenu()
{
    _streamMenuPanel->ClearItems();

    _videoStreamMenuPanel->ClearItems();
    _audioStreamMenuPanel->ClearItems();
    _subtitleStreamMenuPanel->ClearItems();
    auto videoStreamMenuItem = Create<zcom::MenuItem>(_videoStreamMenuPanel.get(), L"Video tracks");
    auto audioStreamMenuItem = Create<zcom::MenuItem>(_audioStreamMenuPanel.get(), L"Audio tracks");
    auto subtitleStreamMenuItem = Create<zcom::MenuItem>(_subtitleStreamMenuPanel.get(), L"Subtitle tracks");
    videoStreamMenuItem->SetDisabled(true);
    audioStreamMenuItem->SetDisabled(true);
    subtitleStreamMenuItem->SetDisabled(true);
    _streamMenuPanel->AddItem(std::move(videoStreamMenuItem));
    _streamMenuPanel->AddItem(std::move(audioStreamMenuItem));
    _streamMenuPanel->AddItem(std::move(subtitleStreamMenuItem));
}

void zcom::PlaybackController::_SetupStreamMenu()
{
    _videoStreamMenuPanel->ClearItems();
    _audioStreamMenuPanel->ClearItems();
    _subtitleStreamMenuPanel->ClearItems();

    // Create video stream disable option
    auto noVideoStreamItem = Create<zcom::MenuItem>(L"None", [&](bool) { _playback->Controller()->SetVideoStream(-1); });
    noVideoStreamItem->SetCheckable(true);
    noVideoStreamItem->SetCheckGroup(0);
    if (_playback->Controller()->CurrentVideoStream() == -1)
        noVideoStreamItem->SetChecked(true);
    _videoStreamMenuPanel->AddItem(std::move(noVideoStreamItem));
    // Add video streams to menu
    auto videoStreams = _playback->Controller()->GetAvailableVideoStreams();
    if (!videoStreams.empty())
        _videoStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    for (int i = 0; i < videoStreams.size(); i++)
    {
        videoStreams[i] = int_to_str(i + 1) + ".  " + videoStreams[i];
        auto streamItem = Create<zcom::MenuItem>(string_to_wstring(videoStreams[i]), [&, i](bool) { _playback->Controller()->SetVideoStream(i); });
        streamItem->SetCheckable(true);
        streamItem->SetCheckGroup(0);
        if (i == _playback->Controller()->CurrentVideoStream())
            streamItem->SetChecked(true);
        _videoStreamMenuPanel->AddItem(std::move(streamItem));
    }

    // Create audio stream disable option
    auto noAudioStreamItem = Create<zcom::MenuItem>(L"None", [&](bool) { _playback->Controller()->SetAudioStream(-1); });
    noAudioStreamItem->SetCheckable(true);
    noAudioStreamItem->SetCheckGroup(0);
    if (_playback->Controller()->CurrentAudioStream() == -1)
        noAudioStreamItem->SetChecked(true);
    _audioStreamMenuPanel->AddItem(std::move(noAudioStreamItem));
    // Add audio streams to menu
    auto audioStreams = _playback->Controller()->GetAvailableAudioStreams();
    if (!audioStreams.empty())
        _audioStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    for (int i = 0; i < audioStreams.size(); i++)
    {
        audioStreams[i] = int_to_str(i + 1) + ".  " + audioStreams[i];
        auto streamItem = Create<zcom::MenuItem>(string_to_wstring(audioStreams[i]), [&, i](bool) { _playback->Controller()->SetAudioStream(i); });
        streamItem->SetCheckable(true);
        streamItem->SetCheckGroup(0);
        if (i == _playback->Controller()->CurrentAudioStream())
            streamItem->SetChecked(true);
        _audioStreamMenuPanel->AddItem(std::move(streamItem));
    }

    // Create subtitle add option
    if (!APP_NETWORK->GetManager<void>())
    {
        auto addSubtitlesItem = Create<zcom::MenuItem>(L"Add subtitles from file", [&](bool) { _ShowSubtitleOpenDialog(); });
        addSubtitlesItem->SetIcon(ResourceManager::GetImage("plus_13x13"));
        _subtitleStreamMenuPanel->AddItem(std::move(addSubtitlesItem));
        _subtitleStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    }
    // Create subtitle stream disable option
    auto noSubtitleStreamItem = Create<zcom::MenuItem>(L"None", [&](bool) { _playback->Controller()->SetSubtitleStream(-1); });
    noSubtitleStreamItem->SetCheckable(true);
    noSubtitleStreamItem->SetCheckGroup(0);
    if (_playback->Controller()->CurrentSubtitleStream() == -1)
        noSubtitleStreamItem->SetChecked(true);
    _subtitleStreamMenuPanel->AddItem(std::move(noSubtitleStreamItem));
    // Add subtitle streams to menu
    auto subtitleStreams = _playback->Controller()->GetAvailableSubtitleStreams();
    if (!subtitleStreams.empty())
        _subtitleStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    for (int i = 0; i < subtitleStreams.size(); i++)
    {
        subtitleStreams[i] = int_to_str(i + 1) + ".  " + subtitleStreams[i];
        auto streamItem = Create<zcom::MenuItem>(string_to_wstring(subtitleStreams[i]), [&, i](bool) { _playback->Controller()->SetSubtitleStream(i); });
        streamItem->SetCheckable(true);
        streamItem->SetCheckGroup(0);
        if (i == _playback->Controller()->CurrentSubtitleStream())
            streamItem->SetChecked(true);
        _subtitleStreamMenuPanel->AddItem(std::move(streamItem));
    }

    _streamMenuPanel->ClearItems();
    auto videoStreamMenuItem = Create<zcom::MenuItem>(_videoStreamMenuPanel.get(), L"Video tracks");
    auto audioStreamMenuItem = Create<zcom::MenuItem>(_audioStreamMenuPanel.get(), L"Audio tracks");
    auto subtitleStreamMenuItem = Create<zcom::MenuItem>(_subtitleStreamMenuPanel.get(), L"Subtitle tracks");
    if (_playback->Initializing())
    {
        videoStreamMenuItem->SetDisabled(true);
        audioStreamMenuItem->SetDisabled(true);
        subtitleStreamMenuItem->SetDisabled(true);
    }
    _streamMenuPanel->AddItem(std::move(videoStreamMenuItem));
    _streamMenuPanel->AddItem(std::move(audioStreamMenuItem));
    _streamMenuPanel->AddItem(std::move(subtitleStreamMenuItem));
}

bool zcom::PlaybackController::_OnKeyDown(BYTE keyCode)
{
    switch (keyCode)
    {
    case 'F': // Fullscreen toggle
    {
        if (_scene->GetApp()->Fullscreen())
            _scene->GetApp()->Fullscreen(false);
        else
            _scene->GetApp()->Fullscreen(true);
        break;
    }
    case VK_SPACE: // Pause toggle
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        if (_playback->Controller()->Paused())
        {
            _playback->Controller()->Play();
            //_playButton->SetPaused(false);
        }
        else
        {
            _playback->Controller()->Pause();
            //_playButton->SetPaused(true);
        }
        break;
    }
    case VK_LEFT: // Seek back
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        int seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT), 15).Value();
        if (KeyState(VK_CONTROL))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_LARGE), 60).Value();
        if (KeyState(VK_SHIFT))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_SMALL), 5).Value();
        _playback->Controller()->Seek(_playback->Controller()->CurrentTime() - Duration(seekAmount, SECONDS));
        break;
    }
    case VK_RIGHT: // Seek forward
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        int seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT), 15).Value();
        if (KeyState(VK_CONTROL))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_LARGE), 60).Value();
        if (KeyState(VK_SHIFT))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_SMALL), 5).Value();
        _playback->Controller()->Seek(_playback->Controller()->CurrentTime() + Duration(seekAmount, SECONDS));
        break;
    }
    case VK_UP: // Volume up
    {
        if (_playback->Initializing()) break;

        float volumeChange = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_VOLUME_CHANGE_AMOUNT), 5).Value() * 0.01f;
        if (KeyState(VK_CONTROL))
            volumeChange = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_VOLUME_CHANGE_AMOUNT_LARGE), 20).Value() * 0.01f;
        if (KeyState(VK_SHIFT))
            volumeChange = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_VOLUME_CHANGE_AMOUNT_SMALL), 1).Value() * 0.01f;
        _volumeSlider->SetValue(_volumeSlider->GetValue() + volumeChange);
        _playback->Controller()->SetVolume(_volumeSlider->GetVolume());
        break;
    }
    case VK_DOWN: // Volume down
    {
        if (_playback->Initializing()) break;

        float volumeChange = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_VOLUME_CHANGE_AMOUNT), 5).Value() * 0.01f;
        if (KeyState(VK_CONTROL))
            volumeChange = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_VOLUME_CHANGE_AMOUNT_LARGE), 20).Value() * 0.01f;
        if (KeyState(VK_SHIFT))
            volumeChange = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_VOLUME_CHANGE_AMOUNT_SMALL), 1).Value() * 0.01f;
        _volumeSlider->SetValue(_volumeSlider->GetValue() - volumeChange);
        _playback->Controller()->SetVolume(_volumeSlider->GetVolume());
        break;
    }
    case VK_NUMPAD1: // Previous video Stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        auto streams = _playback->Controller()->GetAvailableVideoStreams();
        if (streams.empty())
            break;
        int currentStream = _playback->Controller()->CurrentVideoStream();
        if (currentStream == -1)
            _playback->Controller()->SetVideoStream(streams.size() - 1);
        else if (currentStream == 0)
            _playback->Controller()->SetVideoStream(streams.size() - 1);
        else
            _playback->Controller()->SetVideoStream(currentStream - 1);
        break;
    }
    case VK_NUMPAD2: // Next video stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        auto streams = _playback->Controller()->GetAvailableVideoStreams();
        if (streams.empty())
            break;
        int currentStream = _playback->Controller()->CurrentVideoStream();
        if (currentStream == -1)
            _playback->Controller()->SetVideoStream(0);
        else if (currentStream == streams.size() - 1)
            _playback->Controller()->SetVideoStream(0);
        else
            _playback->Controller()->SetVideoStream(currentStream + 1);
        break;
    }
    case VK_NUMPAD3: // Disable video stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetVideoStream(-1);
        break;
    }
    case VK_NUMPAD4: // Previous audio stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        auto streams = _playback->Controller()->GetAvailableAudioStreams();
        if (streams.empty())
            break;
        int currentStream = _playback->Controller()->CurrentAudioStream();
        if (currentStream == -1)
            _playback->Controller()->SetAudioStream(streams.size() - 1);
        else if (currentStream == 0)
            _playback->Controller()->SetAudioStream(streams.size() - 1);
        else
            _playback->Controller()->SetAudioStream(currentStream - 1);
        break;
    }
    case VK_NUMPAD5: // Next audio stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        auto streams = _playback->Controller()->GetAvailableAudioStreams();
        if (streams.empty())
            break;
        int currentStream = _playback->Controller()->CurrentAudioStream();
        if (currentStream == -1)
            _playback->Controller()->SetAudioStream(0);
        else if (currentStream == streams.size() - 1)
            _playback->Controller()->SetAudioStream(0);
        else
            _playback->Controller()->SetAudioStream(currentStream + 1);
        break;
    }
    case VK_NUMPAD6: // Disable audio stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetAudioStream(-1);
        break;
    }
    case VK_NUMPAD7: // Previous subtitle stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        auto streams = _playback->Controller()->GetAvailableSubtitleStreams();
        if (streams.empty())
            break;
        int currentStream = _playback->Controller()->CurrentSubtitleStream();
        if (currentStream == -1)
            _playback->Controller()->SetSubtitleStream(streams.size() - 1);
        else if (currentStream == 0)
            _playback->Controller()->SetSubtitleStream(streams.size() - 1);
        else
            _playback->Controller()->SetSubtitleStream(currentStream - 1);
        break;
    }
    case VK_NUMPAD8: // Next subtitle stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        auto streams = _playback->Controller()->GetAvailableSubtitleStreams();
        if (streams.empty())
            break;
        int currentStream = _playback->Controller()->CurrentSubtitleStream();
        if (currentStream == -1)
            _playback->Controller()->SetSubtitleStream(0);
        else if (currentStream == streams.size() - 1)
            _playback->Controller()->SetSubtitleStream(0);
        else
            _playback->Controller()->SetSubtitleStream(currentStream + 1);
        break;
    }
    case VK_NUMPAD9: // Disable subtitle stream
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetSubtitleStream(-1);
        break;
    }
    }
    return false;
}