#include "App.h" // App.h must be included first
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "HostPlaybackController.h"
#include "ReceiverPlaybackController.h"
#include "Options.h"

bool leftClicked = false;
bool rightClicked = false;
bool upClicked = false;
bool downClicked = false;
bool fClicked = false;
bool spaceClicked = false;
bool num0Clicked = false;
bool num1Clicked = false;
bool num2Clicked = false;
bool num3Clicked = false;
bool num4Clicked = false;
bool num5Clicked = false;
bool num6Clicked = false;
bool num7Clicked = false;
bool num8Clicked = false;
bool num9Clicked = false;

bool ButtonPressed(bool& buttonClicked, int vkCode)
{
    if (GetAsyncKeyState(vkCode) & 0x8000)
    {
        if (!buttonClicked)
        {
            buttonClicked = true;
            return true;
        }
    }
    else
    {
        buttonClicked = false;
    }
    return false;
}

PlaybackScene::PlaybackScene()
{
    SetNotificationPosition(NotificationPosition::TOP_RIGHT);
}

void PlaybackScene::_Init(const SceneOptionsBase* options)
{
    PlaybackSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PlaybackSceneOptions*>(options);
    }

    _playbackMode = opt.playbackMode;
    _startPaused = opt.startPaused;
    _placeholder = opt.placeholder;

    if (opt.dataProvider)
        _dataProvider = opt.dataProvider;
    else if (!_placeholder && _playbackMode == PlaybackMode::OFFLINE)
        _dataProvider = new LocalFileDataProvider(opt.fileName);

    _audioAdapter = nullptr;
    _videoAdapter = nullptr;
    _mediaPlayer = nullptr;
    _controller = nullptr;

    // Set up shortcut handler
    _shortcutHandler = std::make_unique<PlaybackShortcutHandler>();
    _shortcutHandler->AddOnKeyDown([&](BYTE keyCode)
    {
        return _HandleKeyDown(keyCode);
    });

    // Initialize scene
    _controlBar = new zcom::Panel();
    _controlBar->SetParentWidthPercent(1.0f);
    _controlBar->SetBaseHeight(100);
    _controlBar->SetVerticalAlignment(zcom::Alignment::END);
    _controlBar->SetBackgroundColor(D2D1::ColorF::ColorF(0.0f, 0.0f, 0.0f, 0.7f));

    _seekBar = new zcom::SeekBar(1000000); // Placeholder duration until data provider loads
    _seekBar->SetParentWidthPercent(1.0f);
    _seekBar->SetBaseWidth(-20);
    _seekBar->SetBaseHeight(50);
    _seekBar->SetHorizontalOffsetPercent(0.5f);
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
        //_canvas->Resize();
    });
    _seekBar->AddOnHoverEnded([&]()
    {
        _timeHoverPanel->SetVisible(false);
    });

    _timeHoverPanel = std::make_unique<zcom::Panel>();
    _timeHoverPanel->SetBaseSize(120, 20);
    _timeHoverPanel->SetVerticalAlignment(zcom::Alignment::END);
    _timeHoverPanel->SetVerticalOffsetPixels(-90);
    _timeHoverPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _timeHoverPanel->SetBorderVisibility(true);
    _timeHoverPanel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _timeHoverPanel->SetVisible(false);
    zcom::PROP_Shadow timePanelShadow;
    timePanelShadow.blurStandardDeviation = 2.0f;
    timePanelShadow.color = D2D1::ColorF(0);
    _timeHoverPanel->SetProperty(timePanelShadow);

    _timeLabel = std::make_unique<zcom::Label>(L"");
    _timeLabel->SetHorizontalOffsetPixels(5);
    _timeLabel->SetParentHeightPercent(1.0f);
    _timeLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _timeLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _timeLabel->SetFontColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));

    _chapterLabel = std::make_unique<zcom::Label>(L"");
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

    _volumeSlider = new zcom::VolumeSlider(0.0f);
    _volumeSlider->SetBaseWidth(150);
    _volumeSlider->SetBaseHeight(20);
    _volumeSlider->SetHorizontalOffsetPixels(10);
    _volumeSlider->SetVerticalOffsetPixels(50);

    _playButton = new zcom::PlayButton();
    _playButton->SetBaseWidth(40);
    _playButton->SetBaseHeight(40);
    _playButton->SetHorizontalOffsetPercent(0.5f);
    _playButton->SetVerticalOffsetPixels(50);
    _playButton->SetPaused(true);

    _overlayButton = new zcom::Button(L"Open overlay");
    _overlayButton->SetBaseSize(150, 25);
    _overlayButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _overlayButton->SetOffsetPixels(-25, -25);
    _overlayButton->SetBorderVisibility(true);
    _overlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _overlayButton->SetOnActivated([&]()
    {
        App::Instance()->MoveSceneToFront(PlaybackOverlayScene::StaticName());
    });

    _streamButton = std::make_unique<zcom::Button>(L"Streams");
    _streamButton->SetBaseSize(75, 25);
    _streamButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _streamButton->SetOffsetPixels(-200, -25);
    _streamButton->SetBorderVisibility(true);
    _streamButton->SetOnActivated([&]()
    {
        if (!_streamMenuPanel->GetVisible())
        {
            RECT screenRect = { 0, 0, _canvas->GetWidth(), _canvas->GetHeight() };
            RECT buttonRect = {
                _streamButton->GetScreenX(),
                _streamButton->GetScreenY(),
                _streamButton->GetScreenX(),// + _streamButton->GetWidth(),
                _streamButton->GetScreenY()// + _streamButton->GetHeight()
            };
            _streamMenuPanel->Show(screenRect, buttonRect);
        }
        else
        {
            _streamMenuPanel->Hide();
        }
    });

    _streamMenuPanel = std::make_unique<zcom::MenuPanel>(_canvas);
    _videoStreamMenuPanel = std::make_unique<zcom::MenuPanel>(_canvas);
    _audioStreamMenuPanel = std::make_unique<zcom::MenuPanel>(_canvas);
    _subtitleStreamMenuPanel = std::make_unique<zcom::MenuPanel>(_canvas);

    _streamMenuPanel->SetBaseWidth(150);
    _streamMenuPanel->SetZIndex(255);
    _streamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>(_videoStreamMenuPanel.get(), L"Video streams"));
    _streamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>(_audioStreamMenuPanel.get(), L"Audio streams"));
    _streamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>(_subtitleStreamMenuPanel.get(), L"Subtitle streams"));

    _videoStreamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>(L"None"));
    _audioStreamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>(L"None"));
    _subtitleStreamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>(L"None"));

    _loadingCircle = new zcom::LoadingCircle();
    _loadingCircle->SetBaseSize(100, 180);
    _loadingCircle->SetOffsetPercent(0.5f, 0.5f);
    _loadingCircle->SetVerticalOffsetPixels(40);
    _loadingCircle->SetVisible(false);

    _controlBar->AddItem(_seekBar);
    _controlBar->AddItem(_volumeSlider);
    _controlBar->AddItem(_playButton);
    _controlBar->AddItem(_overlayButton);
    _controlBar->AddItem(_streamButton.get());

    _playbackControllerPanel = new zcom::PlaybackControllerPanel(_controlBar);
    _playbackControllerPanel->SetParentSizePercent(1.0f, 1.0f);
    _playbackControllerPanel->AddItem(_controlBar);
    //_playbackControllerPanel->SetZIndex(NOTIF_PANEL_Z_INDEX + 1); // Place above notification panel
    //_bottomControlPanel->SetZIndex(1);

    zcom::PROP_Shadow shadow;
    shadow.color = D2D1::ColorF(0, 0.9f);
    
    _skipBackwardsIcon = std::make_unique<zcom::FastForwardIcon>(zcom::FastForwardDirection::LEFT);
    _skipBackwardsIcon->SetBaseSize(60, 60);
    _skipBackwardsIcon->SetVerticalOffsetPercent(0.5f);
    _skipBackwardsIcon->SetHorizontalOffsetPixels(100);
    _skipBackwardsIcon->SetProperty(shadow);

    _skipForwardsIcon = std::make_unique<zcom::FastForwardIcon>(zcom::FastForwardDirection::RIGHT);
    _skipForwardsIcon->SetBaseSize(60, 60);
    _skipForwardsIcon->SetVerticalOffsetPercent(0.5f);
    _skipForwardsIcon->SetHorizontalOffsetPixels(-100);
    _skipForwardsIcon->SetHorizontalAlignment(zcom::Alignment::END);
    _skipForwardsIcon->SetProperty(shadow);

    _pauseIcon = std::make_unique<zcom::PauseIcon>();
    _pauseIcon->SetBaseSize(60, 60);
    _pauseIcon->SetOffsetPercent(0.5f, 0.5f);
    _pauseIcon->SetProperty(shadow);

    _resumeIcon = std::make_unique<zcom::ResumeIcon>();
    _resumeIcon->SetBaseSize(60, 60);
    _resumeIcon->SetOffsetPercent(0.5f, 0.5f);
    _resumeIcon->SetProperty(shadow);

    _volumeIcon = std::make_unique<zcom::VolumeIcon>();
    _volumeIcon->SetBaseSize(60, 60);
    _volumeIcon->SetHorizontalOffsetPercent(0.5f);
    _volumeIcon->SetVerticalOffsetPixels(60);
    _volumeIcon->SetProperty(shadow);

    _canvas->AddComponent(_skipBackwardsIcon.get());
    _canvas->AddComponent(_skipForwardsIcon.get());
    _canvas->AddComponent(_loadingCircle);
    _canvas->AddComponent(_pauseIcon.get());
    _canvas->AddComponent(_resumeIcon.get());
    _canvas->AddComponent(_volumeIcon.get());
    _canvas->AddComponent(_playbackControllerPanel);
    _canvas->AddComponent(_timeHoverPanel.get());
    _canvas->AddComponent(_streamMenuPanel.get());
    _canvas->AddComponent(_videoStreamMenuPanel.get());
    _canvas->AddComponent(_audioStreamMenuPanel.get());
    _canvas->AddComponent(_subtitleStreamMenuPanel.get());
    //componentCanvas.AddComponent(controlBar);
}

void PlaybackScene::_Uninit()
{
    App::Instance()->window.SetFullscreen(false);

    _canvas->ClearComponents();
    _controlBar->ClearItems();
    delete _playbackControllerPanel;
    delete _controlBar;
    delete _seekBar;
    delete _volumeSlider;
    delete _playButton;
    delete _overlayButton;
    delete _loadingCircle;

    delete _controller;
    delete _mediaPlayer;
    delete _dataProvider;

    _audioAdapter = nullptr;
    _videoAdapter = nullptr;
    _mediaPlayer = nullptr;
    _controller = nullptr;

    _controlBar = nullptr;
    _seekBar = nullptr;
    _volumeSlider = nullptr;
    _playButton = nullptr;
    _playbackControllerPanel = nullptr;
    _overlayButton = nullptr;
    _loadingCircle = nullptr;

    _skipBackwardsIcon = nullptr;
    _skipForwardsIcon = nullptr;
    _pauseIcon = nullptr;
    _resumeIcon = nullptr;
    _volumeIcon = nullptr;
    _timeHoverPanel = nullptr;
    _timeLabel = nullptr;
    _chapterLabel = nullptr;

    _streamButton = nullptr;
    _streamMenuPanel = nullptr;
    _videoStreamMenuPanel = nullptr;
    _audioStreamMenuPanel = nullptr;
    _subtitleStreamMenuPanel = nullptr;

    _shortcutHandler = nullptr;
}

void PlaybackScene::_Focus()
{
    App::Instance()->keyboardManager.AddHandler(_shortcutHandler.get());
    GetKeyboardState(_shortcutHandler->KeyStates());
}

void PlaybackScene::_Unfocus()
{
    App::Instance()->keyboardManager.RemoveHandler(_shortcutHandler.get());
    App::Instance()->window.SetCursorVisibility(true);
}

void PlaybackScene::_Update()
{
    // Keep control panel fixed while stream menu is open
    if (_streamMenuPanel->GetVisible())
        _playbackControllerPanel->SetFixed(true);
    else
        _playbackControllerPanel->SetFixed(false);

    // Show loading circle if needed
    _loadingCircle->SetVisible(false);
    if (!_mediaPlayer)
        _loadingCircle->SetVisible(true);
    if (_controller)
    {
        auto info = _controller->Loading();
        _loadingCircle->SetVisible(info.loading);
        _loadingCircle->SetLoadingText(info.message);
    }

    if (_mediaPlayer)
    {
        // Prevent screen turning off while playing
        if (!_controller->Paused())
            App::Instance()->window.ResetScreenTimer();

        // Check for play button click
        if (_controller->Paused() != _playButton->GetPaused())
        {
            if (_playButton->Clicked())
            {
                if (_playButton->GetPaused())
                {
                    _controller->Pause();
                }
                else
                {
                    _controller->Play();
                }
            }
            else
            {
                _playButton->SetPaused(_controller->Paused());
            }
        }

        // Check for seekbar click
        TimePoint seekTo = _seekBar->SeekTime();
        if (seekTo.GetTicks() != -1)
        {
            _controller->Seek(seekTo);
        }

        // Check for volume slider click
        if (_volumeSlider->GetVolume() != _controller->GetVolume())
        {
            _controller->SetVolume(_volumeSlider->GetVolume());
        }

        // Update seekbar
        Duration bufferedDuration = _controller->GetBufferedDuration();
        if (bufferedDuration.GetTicks() != -1)
        {
            _seekBar->SetBufferedDuration(bufferedDuration);
        }
        _seekBar->SetCurrentTime(_controller->CurrentTime());
    }
    
    if (_controller)
    {
        _controller->Update();
    }
    if (_mediaPlayer)
    {
        _mediaPlayer->Update();
    }

    if (!_mediaPlayer && _dataProvider)
    {
        if (!_dataProvider->Initializing() && !_dataProvider->InitFailed())
        {
            _dataProvider->Start();

            // Create adapters
            _videoAdapter = new IVideoOutputAdapter();
            auto audioStream = _dataProvider->CurrentAudioStream();
            if (audioStream)
                _audioAdapter = new XAudio2_AudioOutputAdapter(audioStream->channels, audioStream->sampleRate);
            else
                _audioAdapter = new XAudio2_AudioOutputAdapter(1, 1);

            // Create media player
            _mediaPlayer = new MediaPlayer(
                _dataProvider,
                std::unique_ptr<IVideoOutputAdapter>(_videoAdapter),
                std::unique_ptr<IAudioOutputAdapter>(_audioAdapter)
            );

            // Create controller
            if (_playbackMode == PlaybackMode::OFFLINE)
                _controller = new BasePlaybackController(_mediaPlayer, _dataProvider);
            else if (_playbackMode == PlaybackMode::SERVER)
                _controller = new HostPlaybackController(_mediaPlayer, (MediaHostDataProvider*)_dataProvider);
            else if (_playbackMode == PlaybackMode::CLIENT)
                _controller = new ReceiverPlaybackController(_mediaPlayer, (MediaReceiverDataProvider*)_dataProvider);

            if (!_startPaused)
                _controller->Play();

            // Configure seekbar
            _seekBar->SetDuration(_dataProvider->MediaDuration());
            _seekBar->SetChapters(_dataProvider->GetChapters());

            // Get volume from saved options
            float volume = 0.2f;
            try { volume = std::stof(Options::Instance()->GetValue("volume")); }
            catch (std::out_of_range) {}
            catch (std::invalid_argument) {}
            _volumeSlider->SetValue(volume);
            _controller->SetVolume(_volumeSlider->GetVolume());

            _SetupStreamMenu();
        }
        if (_dataProvider->InitFailed())
        {
            std::cout << "DATA PROVIDER INIT FAILED" << std::endl;
        }
    }

    // Update UI
    _canvas->Update();
}

ID2D1Bitmap1* PlaybackScene::_Draw(Graphics g)
{
    g.target->Clear(D2D1::ColorF(0.05f, 0.05f, 0.05f));

    if (_mediaPlayer)
    {
        const VideoFrame& videoFrame = _videoAdapter->GetVideoData();
        const VideoFrame& subtitleFrame = _videoAdapter->GetSubtitleData();

        // Update video bitmap
        if (!_videoFrameBitmap || _videoAdapter->VideoDataChanged())
        {
            if (_videoFrameBitmap)
                zcom::SafeFullRelease((IUnknown**)&_videoFrameBitmap);

            bool frameValid = videoFrame.GetWidth() && videoFrame.GetHeight();
            if (frameValid)
            {
                D2D1_BITMAP_PROPERTIES props;
                props.dpiX = 96.0f;
                props.dpiY = 96.0f;
                props.pixelFormat = D2D1::PixelFormat
                (
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_IGNORE
                );
                g.target->CreateBitmap
                (
                    D2D1::SizeU(videoFrame.GetWidth(), videoFrame.GetHeight()),
                    props,
                    &_videoFrameBitmap
                );
                g.refs->push_back((IUnknown**)&_videoFrameBitmap);

                D2D1_RECT_U rect = D2D1::RectU(0, 0, videoFrame.GetWidth(), videoFrame.GetHeight());
                _videoFrameBitmap->CopyFromMemory(&rect, videoFrame.GetBytes(), videoFrame.GetWidth() * 4);
            }
        }

        // Update subtitle bitmap
        if (!_subtitleFrameBitmap || _videoAdapter->SubtitleDataChanged())
        {
            if (_subtitleFrameBitmap)
                zcom::SafeFullRelease((IUnknown**)&_subtitleFrameBitmap);

            bool frameValid = subtitleFrame.GetWidth() && subtitleFrame.GetHeight();
            if (frameValid)
            {
                D2D1_BITMAP_PROPERTIES props;
                props.dpiX = 96.0f;
                props.dpiY = 96.0f;
                props.pixelFormat = D2D1::PixelFormat
                (
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED
                );
                g.target->CreateBitmap
                (
                    D2D1::SizeU(subtitleFrame.GetWidth(), subtitleFrame.GetHeight()),
                    props,
                    &_subtitleFrameBitmap
                );
                g.refs->push_back((IUnknown**)&_subtitleFrameBitmap);

                D2D1_RECT_U rect = D2D1::RectU(0, 0, subtitleFrame.GetWidth(), subtitleFrame.GetHeight());
                _subtitleFrameBitmap->CopyFromMemory(&rect, subtitleFrame.GetBytes(), subtitleFrame.GetWidth() * 4);
            }
        }

        // Determine video destination dimensions
        D2D1_RECT_F destRectVideo;
        D2D1_RECT_F srcRectVideo;
        if (_videoFrameBitmap)
        {
            // Scale frame to preserve aspect ratio
            srcRectVideo = D2D1::RectF(0.0f, 0.0f, videoFrame.GetWidth(), videoFrame.GetHeight());
            float tWidth = g.target->GetSize().width;
            float tHeight = g.target->GetSize().height;
            if (videoFrame.GetWidth() / (float)videoFrame.GetHeight() < tWidth / tHeight)
            {
                float scale = videoFrame.GetHeight() / tHeight;
                float newWidth = videoFrame.GetWidth() / scale;
                destRectVideo = D2D1::Rect
                (
                    (tWidth - newWidth) * 0.5f,
                    0.0f,
                    (tWidth - newWidth) * 0.5f + newWidth,
                    tHeight
                );
            }
            else if (videoFrame.GetWidth() / (float)videoFrame.GetHeight() > tWidth / tHeight)
            {
                float scale = videoFrame.GetWidth() / tWidth;
                float newHeight = videoFrame.GetHeight() / scale;
                destRectVideo = D2D1::Rect
                (
                    0.0f,
                    (tHeight - newHeight) * 0.5f,
                    tWidth,
                    (tHeight - newHeight) * 0.5f + newHeight
                );
            }
            else
            {
                destRectVideo = D2D1::RectF(0.0f, 0.0f, tWidth, tHeight);
            }
        }

        // Determine subtitle destination dimensions
        D2D1_RECT_F destRectSubtitles;
        D2D1_RECT_F srcRectSubtitles = D2D1::RectF(0.0f, 0.0f, subtitleFrame.GetWidth(), subtitleFrame.GetHeight());
        if (_videoFrameBitmap)
        {
            destRectSubtitles = destRectVideo;
        }
        else
        {
            // Scale frame to preserve aspect ratio
            float tWidth = g.target->GetSize().width;
            float tHeight = g.target->GetSize().height;
            if (subtitleFrame.GetWidth() / (float)subtitleFrame.GetHeight() < tWidth / tHeight)
            {
                float scale = subtitleFrame.GetHeight() / tHeight;
                float newWidth = subtitleFrame.GetWidth() / scale;
                destRectSubtitles = D2D1::Rect
                (
                    (tWidth - newWidth) * 0.5f,
                    0.0f,
                    (tWidth - newWidth) * 0.5f + newWidth,
                    tHeight
                );
            }
            else if (subtitleFrame.GetWidth() / (float)subtitleFrame.GetHeight() > tWidth / tHeight)
            {
                float scale = subtitleFrame.GetWidth() / tWidth;
                float newHeight = subtitleFrame.GetHeight() / scale;
                destRectSubtitles = D2D1::Rect
                (
                    0.0f,
                    (tHeight - newHeight) * 0.5f,
                    tWidth,
                    (tHeight - newHeight) * 0.5f + newHeight
                );
            }
            else
            {
                destRectSubtitles = D2D1::RectF(0.0f, 0.0f, tWidth, tHeight);
            }
        }

        // Draw video
        if (_videoFrameBitmap)
            g.target->DrawBitmap(_videoFrameBitmap, destRectVideo, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRectVideo);

        // Draw subtitles
        if (_subtitleFrameBitmap)
            g.target->DrawBitmap(_subtitleFrameBitmap, destRectSubtitles, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRectSubtitles);
    }

    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void PlaybackScene::_Resize(int width, int height)
{
    
}

bool PlaybackScene::Finished() const
{
    if (_controller)
    {
        return _controller->Finished();
    }
    else
    {
        return false;
    }
}

void PlaybackScene::_SetupStreamMenu()
{
    // Add video streams to menu
    _videoStreamMenuPanel->ClearMenuItems();
    auto noVideoStreamItem = std::make_unique<zcom::MenuItem>(L"None", [&](bool) { _controller->SetVideoStream(-1); });
    noVideoStreamItem->SetCheckable(true);
    noVideoStreamItem->SetCheckGroup(0);
    _videoStreamMenuPanel->AddMenuItem(std::move(noVideoStreamItem));
    _videoStreamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>());
    auto videoStreams = _controller->GetAvailableVideoStreams();
    for (int i = 0; i < videoStreams.size(); i++)
    {
        videoStreams[i] = int_to_str(i + 1) + ".  " + videoStreams[i];
        auto streamItem = std::make_unique<zcom::MenuItem>(string_to_wstring(videoStreams[i]), [&, i](bool) { _controller->SetVideoStream(i); });
        streamItem->SetCheckable(true);
        streamItem->SetCheckGroup(0);
        if (i == _controller->CurrentVideoStream())
            streamItem->SetChecked(true);
        _videoStreamMenuPanel->AddMenuItem(std::move(streamItem));
    }

    // Add audio streams to menu
    _audioStreamMenuPanel->ClearMenuItems();
    auto noAudioStreamItem = std::make_unique<zcom::MenuItem>(L"None", [&](bool) { _controller->SetAudioStream(-1); });
    noAudioStreamItem->SetCheckable(true);
    noAudioStreamItem->SetCheckGroup(0);
    _audioStreamMenuPanel->AddMenuItem(std::move(noAudioStreamItem));
    _audioStreamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>());
    auto audioStreams = _controller->GetAvailableAudioStreams();
    for (int i = 0; i < audioStreams.size(); i++)
    {
        audioStreams[i] = int_to_str(i + 1) + ".  " + audioStreams[i];
        auto streamItem = std::make_unique<zcom::MenuItem>(string_to_wstring(audioStreams[i]), [&, i](bool) { _controller->SetAudioStream(i); });
        streamItem->SetCheckable(true);
        streamItem->SetCheckGroup(0);
        if (i == _controller->CurrentAudioStream())
            streamItem->SetChecked(true);
        _audioStreamMenuPanel->AddMenuItem(std::move(streamItem));
    }

    // Add subtitle streams to menu
    _subtitleStreamMenuPanel->ClearMenuItems();
    auto noSubtitleStreamItem = std::make_unique<zcom::MenuItem>(L"None", [&](bool) { _controller->SetSubtitleStream(-1); });
    noSubtitleStreamItem->SetCheckable(true);
    noSubtitleStreamItem->SetCheckGroup(0);
    _subtitleStreamMenuPanel->AddMenuItem(std::move(noSubtitleStreamItem));
    _subtitleStreamMenuPanel->AddMenuItem(std::make_unique<zcom::MenuItem>());
    auto subtitleStreams = _controller->GetAvailableSubtitleStreams();
    for (int i = 0; i < subtitleStreams.size(); i++)
    {
        subtitleStreams[i] = int_to_str(i + 1) + ".  " + subtitleStreams[i];
        auto streamItem = std::make_unique<zcom::MenuItem>(string_to_wstring(subtitleStreams[i]), [&, i](bool) { _controller->SetSubtitleStream(i); });
        streamItem->SetCheckable(true);
        streamItem->SetCheckGroup(0);
        if (i == _controller->CurrentSubtitleStream())
            streamItem->SetChecked(true);
        _subtitleStreamMenuPanel->AddMenuItem(std::move(streamItem));
    }
}

bool PlaybackScene::_HandleKeyDown(BYTE keyCode)
{
    switch (keyCode)
    {
    case 'F': // Fullscreen toggle
    {
        if (App::Instance()->window.GetFullscreen())
        {
            App::Instance()->window.SetFullscreen(false);
        }
        else
        {
            App::Instance()->window.SetFullscreen(true);
        }
        break;
    }
    case VK_ESCAPE: // Fullscreen exit
    {
        if (App::Instance()->window.GetFullscreen())
        {
            App::Instance()->window.SetFullscreen(false);
        }
        break;
    }
    case VK_SPACE: // Pause toggle
    {
        if (!_controller) break;

        if (_controller->Paused())
        {
            _controller->Play();
            _playButton->SetPaused(false);
            _pauseIcon->SetVisible(false);
            _resumeIcon->Show();
        }
        else
        {
            _controller->Pause();
            _playButton->SetPaused(true);
            _resumeIcon->SetVisible(false);
            _pauseIcon->Show();
        }
        break;
    }
    case VK_LEFT: // Seek back
    {
        if (!_controller) break;

        int seekAmount = 15;
        if (_shortcutHandler->KeyState(VK_CONTROL)) seekAmount = 60;
        if (_shortcutHandler->KeyState(VK_SHIFT)) seekAmount = 5;
        _controller->Seek(_controller->CurrentTime() - Duration(seekAmount, SECONDS));
        _skipBackwardsIcon->Show(seekAmount);
        break;
    }
    case VK_RIGHT: // Seek forward
    {
        if (!_controller) break;

        int seekAmount = 15;
        if (_shortcutHandler->KeyState(VK_CONTROL)) seekAmount = 60;
        if (_shortcutHandler->KeyState(VK_SHIFT)) seekAmount = 5;
        _controller->Seek(_controller->CurrentTime() + Duration(seekAmount, SECONDS));
        _skipForwardsIcon->Show(seekAmount);
        break;
    }
    case VK_UP: // Volume up
    {
        float volumeChange = 0.05f;
        if (_shortcutHandler->KeyState(VK_CONTROL)) volumeChange = 0.2f;
        if (_shortcutHandler->KeyState(VK_SHIFT)) volumeChange = 0.01f;
        _volumeSlider->SetValue(_volumeSlider->GetValue() + volumeChange);
        _controller->SetVolume(_volumeSlider->GetVolume());
        _volumeIcon->Show(roundf(_volumeSlider->GetValue() * 100.0f));
        break;
    }
    case VK_DOWN: // Volume down
    {
        if (!_controller) break;

        float volumeChange = 0.05f;
        if (_shortcutHandler->KeyState(VK_CONTROL)) volumeChange = 0.2f;
        if (_shortcutHandler->KeyState(VK_SHIFT)) volumeChange = 0.01f;
        _volumeSlider->SetValue(_volumeSlider->GetValue() - volumeChange);
        _controller->SetVolume(_volumeSlider->GetVolume());
        _volumeIcon->Show(roundf(_volumeSlider->GetValue() * 100.0f));
        break;
    }
    case VK_NUMPAD1: // Video stream 1
    {
        if (!_controller) break;

        _controller->SetVideoStream(0);
        break;
    }
    case VK_NUMPAD2: // Video stream 2
    {
        if (!_controller) break;

        _controller->SetVideoStream(1);
        break;
    }
    case VK_NUMPAD3: // Video stream 3
    {
        if (!_controller) break;

        _controller->SetVideoStream(2);
        break;
    }
    case VK_NUMPAD4: // Audio stream 1
    {
        if (!_controller) break;

        _controller->SetAudioStream(0);
        break;
    }
    case VK_NUMPAD5: // Audio stream 2
    {
        if (!_controller) break;

        _controller->SetAudioStream(1);
        break;
    }
    case VK_NUMPAD6: // Audio stream 3
    {
        if (!_controller) break;

        _controller->SetAudioStream(2);
        break;
    }
    case VK_NUMPAD7: // Subtitle stream 1
    {
        if (!_controller) break;

        _controller->SetSubtitleStream(0);
        break;
    }
    case VK_NUMPAD8: // Subtitle stream 2
    {
        if (!_controller) break;

        _controller->SetSubtitleStream(1);
        break;
    }
    case VK_NUMPAD9: // Subtitle stream 3
    {
        if (!_controller) break;

        _controller->SetSubtitleStream(2);
        break;
    }
    }
    return false;
}