#include "PlaybackController.h"

#include "App.h"
#include "OverlayScene.h"
#include "PlaybackOverlayScene.h"

#include "Permissions.h"
#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"

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
    _playNextButton->SetButtonImage(ResourceManager::GetImage("play_next_dim"));
    _playNextButton->SetButtonHoverImage(ResourceManager::GetImage("play_next"));
    _playNextButton->SetButtonClickImage(ResourceManager::GetImage("play_next"));
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
    _playPreviousButton->SetButtonImage(ResourceManager::GetImage("play_previous_dim"));
    _playPreviousButton->SetButtonHoverImage(ResourceManager::GetImage("play_previous"));
    _playPreviousButton->SetButtonClickImage(ResourceManager::GetImage("play_previous"));
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
    _overlayButton->SetButtonImage(ResourceManager::GetImage("playlist_dim"));
    _overlayButton->SetButtonHoverImage(ResourceManager::GetImage("playlist"));
    _overlayButton->SetButtonClickImage(ResourceManager::GetImage("playlist"));
    _overlayButton->SetSelectable(false);
    _overlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _overlayButton->SetOnActivated([&]()
    {
        _scene->GetApp()->MoveSceneToFront(PlaybackOverlayScene::StaticName());
    });

    _settingsButton = Create<zcom::Button>(L"");
    _settingsButton->SetBaseSize(30, 30);
    _settingsButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _settingsButton->SetOffsetPixels(-160, -5);
    _settingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _settingsButton->SetButtonImage(ResourceManager::GetImage("settings_dim"));
    _settingsButton->SetButtonHoverImage(ResourceManager::GetImage("settings"));
    _settingsButton->SetButtonClickImage(ResourceManager::GetImage("settings"));
    _settingsButton->SetSelectable(false);
    _settingsButton->SetOnActivated([&]()
    {
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
    _streamMenuPanel->AddItem(Create<zcom::MenuItem>(_videoStreamMenuPanel.get(), L"Video tracks"));
    _streamMenuPanel->AddItem(Create<zcom::MenuItem>(_audioStreamMenuPanel.get(), L"Audio tracks"));
    _streamMenuPanel->AddItem(Create<zcom::MenuItem>(_subtitleStreamMenuPanel.get(), L"Subtitle tracks"));

    _videoStreamMenuPanel->AddItem(Create<zcom::MenuItem>(L"None"));
    _audioStreamMenuPanel->AddItem(Create<zcom::MenuItem>(L"None"));
    _subtitleStreamMenuPanel->AddItem(Create<zcom::MenuItem>(L"None"));

    _fullscreenButton = Create<zcom::Button>(L"");
    _fullscreenButton->SetBaseSize(30, 30);
    _fullscreenButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _fullscreenButton->SetOffsetPixels(-80, -5);
    _fullscreenButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_on_dim"));
    _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_on"));
    _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_on"));
    _fullscreenButton->SetSelectable(false);
    _fullscreenButton->SetOnActivated([&]()
    {
        if (_scene->GetApp()->Fullscreen())
        {
            _scene->GetApp()->Fullscreen(false);
            _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_on_dim"));
            _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_on"));
            _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_on"));
        }
        else
        {
            _scene->GetApp()->Fullscreen(true);
            _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_off_dim"));
            _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_off"));
            _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_off"));
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

void zcom::PlaybackController::_OnUpdate()
{
    if (!_playback->Initializing())
    {
        _playbackLoaded = true;

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
    }

    _UpdateFullscreenButton();

    Panel::_OnUpdate();
}

void zcom::PlaybackController::_UpdateFullscreenButton(bool force)
{
    if (force || _inFullscreen != _scene->GetApp()->Fullscreen())
    {
        _inFullscreen = _scene->GetApp()->Fullscreen();
        if (_inFullscreen)
        {
            _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_off_dim"));
            _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_off"));
            _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_off"));
        }
        else
        {
            _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_on_dim"));
            _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_on"));
            _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_on"));
        }
    }
}

void zcom::PlaybackController::_UpdatePermissions()
{
    auto user = _scene->GetApp()->users.GetThisUser();
    if (!user)
        return;

    bool allowPlaybackManipulation = user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK);
    _playButton->SetActive(allowPlaybackManipulation);
    bool allowPlaybackStartStop = user->GetPermission(PERMISSION_START_STOP_PLAYBACK);
    _playNextButton->SetVisible(allowPlaybackStartStop);
    _playPreviousButton->SetVisible(allowPlaybackStartStop);
    bool allowStreamChanging = user->GetPermission(PERMISSION_CHANGE_STREAMS);
    for (int i = 0; i < _streamMenuPanel->ItemCount(); i++)
    {
        // Slightly janky way to disable the correct menu items
        zcom::MenuItem* item = _streamMenuPanel->GetItem(i);
        if (item->GetMenuPanel() == _videoStreamMenuPanel.get() ||
            item->GetMenuPanel() == _audioStreamMenuPanel.get() ||
            item->GetMenuPanel() == _subtitleStreamMenuPanel.get())
        {
            item->SetDisabled(!allowStreamChanging);
        }
    }
}

void zcom::PlaybackController::_SetupStreamMenu()
{
    // Add video streams to menu
    _videoStreamMenuPanel->ClearItems();
    auto noVideoStreamItem = Create<zcom::MenuItem>(L"None", [&](bool) { _playback->Controller()->SetVideoStream(-1); });
    noVideoStreamItem->SetCheckable(true);
    noVideoStreamItem->SetCheckGroup(0);
    _videoStreamMenuPanel->AddItem(std::move(noVideoStreamItem));
    _videoStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    auto videoStreams = _playback->Controller()->GetAvailableVideoStreams();
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

    // Add audio streams to menu
    _audioStreamMenuPanel->ClearItems();
    auto noAudioStreamItem = Create<zcom::MenuItem>(L"None", [&](bool) { _playback->Controller()->SetAudioStream(-1); });
    noAudioStreamItem->SetCheckable(true);
    noAudioStreamItem->SetCheckGroup(0);
    _audioStreamMenuPanel->AddItem(std::move(noAudioStreamItem));
    _audioStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    auto audioStreams = _playback->Controller()->GetAvailableAudioStreams();
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

    // Add subtitle streams to menu
    _subtitleStreamMenuPanel->ClearItems();
    auto addSubtitlesItem = Create<zcom::MenuItem>(L"Add subtitles from file", [&](bool) { /* Add subtitles */ });
    addSubtitlesItem->SetIcon(ResourceManager::GetImage("plus"));
    addSubtitlesItem->SetDisabled(true);
    _subtitleStreamMenuPanel->AddItem(std::move(addSubtitlesItem));
    _subtitleStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    auto noSubtitleStreamItem = Create<zcom::MenuItem>(L"None", [&](bool) { _playback->Controller()->SetSubtitleStream(-1); });
    noSubtitleStreamItem->SetCheckable(true);
    noSubtitleStreamItem->SetCheckGroup(0);
    _subtitleStreamMenuPanel->AddItem(std::move(noSubtitleStreamItem));
    _subtitleStreamMenuPanel->AddItem(Create<zcom::MenuItem>());
    auto subtitleStreams = _playback->Controller()->GetAvailableSubtitleStreams();
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
    case VK_NUMPAD1: // Video stream 1
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetVideoStream(0);
        break;
    }
    case VK_NUMPAD2: // Video stream 2
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetVideoStream(1);
        break;
    }
    case VK_NUMPAD3: // Video stream 3
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetVideoStream(2);
        break;
    }
    case VK_NUMPAD4: // Audio stream 1
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetAudioStream(0);
        break;
    }
    case VK_NUMPAD5: // Audio stream 2
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetAudioStream(1);
        break;
    }
    case VK_NUMPAD6: // Audio stream 3
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetAudioStream(2);
        break;
    }
    case VK_NUMPAD7: // Subtitle stream 1
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetSubtitleStream(0);
        break;
    }
    case VK_NUMPAD8: // Subtitle stream 2
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetSubtitleStream(1);
        break;
    }
    case VK_NUMPAD9: // Subtitle stream 3
    {
        if (_playback->Initializing())
            break;
        auto user = _scene->GetApp()->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        _playback->Controller()->SetSubtitleStream(2);
        break;
    }
    }
    return false;
}