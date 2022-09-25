#include "App.h" // App.h must be included first
#include "PlaybackScene.h"
#include "OverlayScene.h"
#include "PlaybackOverlayScene.h"
#include "HostPlaybackController.h"
#include "ReceiverPlaybackController.h"
#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"
#include "FloatOptionAdapter.h"
#include "Permissions.h"

#include "TintEffect.h"

#include <iomanip>

PlaybackScene::PlaybackScene(App* app)
    : Scene(app)
{
}

void PlaybackScene::_Init(const SceneOptionsBase* options)
{
    PlaybackSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PlaybackSceneOptions*>(options);
    }

    _playback = &App::Instance()->playback;
    _streamMenuSetup = false;
    _chaptersSet = false;

    // Set up shortcut handler
    _shortcutHandler = std::make_unique<PlaybackShortcutHandler>();
    _shortcutHandler->AddOnKeyDown([&](BYTE keyCode)
    {
        return _HandleKeyDown(keyCode);
    });

    // Initialize scene
    _controlBar = Create<zcom::Panel>();
    _controlBar->SetParentWidthPercent(1.0f);
    _controlBar->SetBaseHeight(80);
    _controlBar->SetVerticalAlignment(zcom::Alignment::END);
    //_controlBar->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.7f));

    _controlBarBackground = Create<zcom::ControlBarBackground>();
    _controlBarBackground->SetParentSizePercent(1.0f, 1.0f);
    _controlBarBackground->SetZIndex(-2);

    _playbackController = Create<zcom::PlaybackController>();
    _playbackController->SetParentSizePercent(1.0f, 1.0f);


    //_seekBar = Create<zcom::SeekBar>();
    //_seekBar->SetParentWidthPercent(1.0f);
    //_seekBar->SetBaseWidth(-20);
    //_seekBar->SetHorizontalOffsetPercent(0.5f);
    //_seekBar->SetBaseHeight(16);
    //_seekBar->SetVerticalAlignment(zcom::Alignment::END);
    //_seekBar->SetVerticalOffsetPixels(-35);
    //_seekBar->AddOnTimeHovered([&](int xpos, TimePoint time, std::wstring chapterName)
    //{
    //    int totalWidth = 0;

    //    // Time label
    //    _timeLabel->SetText(string_to_wstring(TimeToString(time)));
    //    _timeLabel->SetBaseWidth(std::ceil(_timeLabel->GetTextWidth()));
    //    totalWidth += _timeLabel->GetBaseWidth() + 10;
    //    
    //    // Chapter label
    //    _chapterLabel->SetVisible(false);
    //    if (!chapterName.empty())
    //    {
    //        _chapterLabel->SetText(chapterName);
    //        _chapterLabel->SetBaseWidth(std::ceil(_chapterLabel->GetTextWidth()));
    //        totalWidth += _chapterLabel->GetBaseWidth() + 5;
    //        _chapterLabel->SetVisible(true);
    //    }

    //    _timeHoverPanel->SetBaseWidth(totalWidth);
    //    //_timeHoverPanel->SetWidth(totalWidth);
    //    //_timeHoverPanel->Resize();

    //    //int absolutePos = _playbackControllerPanel->GetX() + _controlBar->GetX() + _seekBar->GetX() + xpos;
    //    int absolutePos = _seekBar->GetScreenX() + xpos;
    //    _timeHoverPanel->SetHorizontalOffsetPixels(absolutePos - _timeHoverPanel->GetWidth() / 2);
    //    _timeHoverPanel->SetVisible(true);
    //    //_canvas->Resize();
    //});
    //_seekBar->AddOnHoverEnded([&]()
    //{
    //    _timeHoverPanel->SetVisible(false);
    //});

    //_timeHoverPanel = Create<zcom::Panel>();
    //_timeHoverPanel->SetBaseSize(120, 20);
    //_timeHoverPanel->SetVerticalAlignment(zcom::Alignment::END);
    //_timeHoverPanel->SetVerticalOffsetPixels(-55);
    //_timeHoverPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    //_timeHoverPanel->SetBorderVisibility(true);
    //_timeHoverPanel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    //_timeHoverPanel->SetVisible(false);
    //zcom::PROP_Shadow timePanelShadow;
    //timePanelShadow.blurStandardDeviation = 2.0f;
    //timePanelShadow.color = D2D1::ColorF(0);
    //_timeHoverPanel->SetProperty(timePanelShadow);

    //_timeLabel = Create<zcom::Label>(L"");
    //_timeLabel->SetHorizontalOffsetPixels(5);
    //_timeLabel->SetParentHeightPercent(1.0f);
    //_timeLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    //_timeLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    //_timeLabel->SetFontColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));

    //_chapterLabel = Create<zcom::Label>(L"");
    //_chapterLabel->SetHorizontalOffsetPixels(-5);
    //_chapterLabel->SetParentHeightPercent(1.0f);
    //_chapterLabel->SetHorizontalAlignment(zcom::Alignment::END);
    //_chapterLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    //_chapterLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    //_chapterLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    //_chapterLabel->SetFontColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    //zcom::PROP_Shadow chapterLabelShadow;
    //chapterLabelShadow.blurStandardDeviation = 1.0f;
    //chapterLabelShadow.offsetX = 1.0f;
    //chapterLabelShadow.offsetY = 1.0f;
    //chapterLabelShadow.color = D2D1::ColorF(0);
    //_chapterLabel->SetProperty(chapterLabelShadow);

    //_timeHoverPanel->AddItem(_timeLabel.get());
    //_timeHoverPanel->AddItem(_chapterLabel.get());

    //_volumeSlider = Create<zcom::VolumeSlider>(0.0f);
    //_volumeSlider->SetBaseWidth(30);
    //_volumeSlider->SetBaseHeight(30);
    //_volumeSlider->SetHorizontalOffsetPixels(80);
    //_volumeSlider->SetVerticalAlignment(zcom::Alignment::END);
    //_volumeSlider->SetVerticalOffsetPixels(-5);

    //_playButton = Create<zcom::PlayButton>();
    //_playButton->SetBaseWidth(30);
    //_playButton->SetBaseHeight(30);
    //_playButton->SetHorizontalOffsetPercent(0.5f);
    //_playButton->SetVerticalAlignment(zcom::Alignment::END);
    //_playButton->SetVerticalOffsetPixels(-5);
    //_playButton->SetPaused(true);

    //_playNextButton = Create<zcom::Button>(L"");
    //_playNextButton->SetBaseSize(30, 30);
    //_playNextButton->SetHorizontalOffsetPercent(0.5f);
    //_playNextButton->SetHorizontalOffsetPixels(40);
    //_playNextButton->SetVerticalAlignment(zcom::Alignment::END);
    //_playNextButton->SetVerticalOffsetPixels(-5);
    //_playNextButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    //_playNextButton->SetButtonImage(ResourceManager::GetImage("play_next_dim"));
    //_playNextButton->SetButtonHoverImage(ResourceManager::GetImage("play_next"));
    //_playNextButton->SetButtonClickImage(ResourceManager::GetImage("play_next"));
    //_playNextButton->SetSelectable(false);
    //_playNextButton->SetActivation(zcom::ButtonActivation::RELEASE);
    //_playNextButton->SetOnActivated([&]()
    //{
    //    // Find currently playing item
    //    auto readyItems = App::Instance()->playlist.ReadyItems();
    //    for (int i = 0; i < readyItems.size(); i++)
    //    {
    //        if (readyItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyPlaying())
    //        {
    //            if (i < readyItems.size() - 1)
    //                App::Instance()->playlist.Request_PlayItem(readyItems[i + 1]->GetItemId());
    //            else
    //                App::Instance()->playlist.Request_PlayItem(readyItems[0]->GetItemId());
    //            break;
    //        }
    //    }
    //});

    //_playPreviousButton = Create<zcom::Button>(L"");
    //_playPreviousButton->SetBaseSize(30, 30);
    //_playPreviousButton->SetHorizontalOffsetPercent(0.5f);
    //_playPreviousButton->SetHorizontalOffsetPixels(-40);
    //_playPreviousButton->SetVerticalAlignment(zcom::Alignment::END);
    //_playPreviousButton->SetVerticalOffsetPixels(-5);
    //_playPreviousButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    //_playPreviousButton->SetButtonImage(ResourceManager::GetImage("play_previous_dim"));
    //_playPreviousButton->SetButtonHoverImage(ResourceManager::GetImage("play_previous"));
    //_playPreviousButton->SetButtonClickImage(ResourceManager::GetImage("play_previous"));
    //_playPreviousButton->SetSelectable(false);
    //_playPreviousButton->SetActivation(zcom::ButtonActivation::RELEASE);
    //_playPreviousButton->SetOnActivated([&]()
    //{
    //    // Find currently playing item
    //    auto readyItems = App::Instance()->playlist.ReadyItems();
    //    for (int i = 0; i < readyItems.size(); i++)
    //    {
    //        if (readyItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyPlaying())
    //        {
    //            // Play previous if available
    //            if (i > 0)
    //                App::Instance()->playlist.Request_PlayItem(readyItems[i - 1]->GetItemId());
    //            else
    //                App::Instance()->playlist.Request_PlayItem(readyItems.back()->GetItemId());
    //            break;
    //        }
    //    }
    //});

    //_overlayButton = Create<zcom::Button>(L"");
    //_overlayButton->SetBaseSize(30, 30);
    //_overlayButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    //_overlayButton->SetOffsetPixels(-120, -5);
    //_overlayButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    //_overlayButton->SetButtonImage(ResourceManager::GetImage("playlist_dim"));
    //_overlayButton->SetButtonHoverImage(ResourceManager::GetImage("playlist"));
    //_overlayButton->SetButtonClickImage(ResourceManager::GetImage("playlist"));
    //_overlayButton->SetSelectable(false);
    //_overlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    //_overlayButton->SetOnActivated([&]()
    //{
    //    App::Instance()->MoveSceneToFront(PlaybackOverlayScene::StaticName());
    //});

    //_settingsButton = Create<zcom::Button>(L"");
    //_settingsButton->SetBaseSize(30, 30);
    //_settingsButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    //_settingsButton->SetOffsetPixels(-160, -5);
    //_settingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    //_settingsButton->SetButtonImage(ResourceManager::GetImage("settings_dim"));
    //_settingsButton->SetButtonHoverImage(ResourceManager::GetImage("settings"));
    //_settingsButton->SetButtonClickImage(ResourceManager::GetImage("settings"));
    //_settingsButton->SetSelectable(false);
    //_settingsButton->SetOnActivated([&]()
    //{
    //    RECT buttonRect = {
    //        _settingsButton->GetScreenX(),
    //        _settingsButton->GetScreenY(),
    //        _settingsButton->GetScreenX(),// + _streamButton->GetWidth(),
    //        _settingsButton->GetScreenY()// + _streamButton->GetHeight()
    //    };
    //    GetApp()->Overlay()->ShowMenu(_streamMenuPanel.get(), buttonRect);
    //});

    //_streamMenuPanel = Create<zcom::MenuPanel>();
    //_videoStreamMenuPanel = Create<zcom::MenuPanel>();
    //_audioStreamMenuPanel = Create<zcom::MenuPanel>();
    //_subtitleStreamMenuPanel = Create<zcom::MenuPanel>();

    //_streamMenuPanel->SetBaseWidth(150);
    //_streamMenuPanel->SetZIndex(255);
    //_streamMenuPanel->AddItem(Create<zcom::MenuItem>(_videoStreamMenuPanel.get(), L"Video tracks"));
    //_streamMenuPanel->AddItem(Create<zcom::MenuItem>(_audioStreamMenuPanel.get(), L"Audio tracks"));
    //_streamMenuPanel->AddItem(Create<zcom::MenuItem>(_subtitleStreamMenuPanel.get(), L"Subtitle tracks"));

    //_videoStreamMenuPanel->AddItem(Create<zcom::MenuItem>(L"None"));
    //_audioStreamMenuPanel->AddItem(Create<zcom::MenuItem>(L"None"));
    //_subtitleStreamMenuPanel->AddItem(Create<zcom::MenuItem>(L"None"));

    //_fullscreenButton = Create<zcom::Button>(L"");
    //_fullscreenButton->SetBaseSize(30, 30);
    //_fullscreenButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    //_fullscreenButton->SetOffsetPixels(-80, -5);
    //_fullscreenButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    //_fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_on_dim"));
    //_fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_on"));
    //_fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_on"));
    //_fullscreenButton->SetSelectable(false);
    //_fullscreenButton->SetOnActivated([&]()
    //{
    //    if (_app->Fullscreen())
    //    {
    //        _app->Fullscreen(false);
    //        _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_on_dim"));
    //        _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_on"));
    //        _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_on"));
    //    }
    //    else
    //    {
    //        _app->Fullscreen(true);
    //        _fullscreenButton->SetButtonImage(ResourceManager::GetImage("fullscreen_off_dim"));
    //        _fullscreenButton->SetButtonHoverImage(ResourceManager::GetImage("fullscreen_off"));
    //        _fullscreenButton->SetButtonClickImage(ResourceManager::GetImage("fullscreen_off"));
    //    }
    //});

    _loadingCircle = Create<zcom::LoadingCircle>();
    _loadingCircle->SetBaseSize(100, 180);
    _loadingCircle->SetOffsetPercent(0.5f, 0.5f);
    _loadingCircle->SetVerticalOffsetPixels(40);
    _loadingCircle->SetVisible(false);

    _controlBar->AddItem(_controlBarBackground.get());
    _controlBar->AddItem(_playbackController.get());

    //_controlBar->AddItem(_seekBar.get());
    //_controlBar->AddItem(_volumeSlider.get());
    //_controlBar->AddItem(_playButton.get());
    //_controlBar->AddItem(_playNextButton.get());
    //_controlBar->AddItem(_playPreviousButton.get());
    //_controlBar->AddItem(_overlayButton.get());
    //_controlBar->AddItem(_settingsButton.get());
    //_controlBar->AddItem(_fullscreenButton.get());

    _playbackControllerPanel = Create<zcom::PlaybackControllerPanel>(_controlBar.get());
    _playbackControllerPanel->SetParentSizePercent(1.0f, 1.0f);
    _playbackControllerPanel->AddItem(_controlBar.get());
    //_playbackControllerPanel->SetZIndex(NOTIF_PANEL_Z_INDEX + 1); // Place above notification panel
    //_bottomControlPanel->SetZIndex(1);

    zcom::PROP_Shadow shadow;
    shadow.color = D2D1::ColorF(0, 0.9f);
    
    _skipBackwardsIcon = Create<zcom::FastForwardIcon>(zcom::FastForwardDirection::LEFT);
    _skipBackwardsIcon->SetBaseSize(60, 60);
    _skipBackwardsIcon->SetVerticalOffsetPercent(0.5f);
    _skipBackwardsIcon->SetHorizontalOffsetPixels(100);
    _skipBackwardsIcon->SetProperty(shadow);

    _skipForwardsIcon = Create<zcom::FastForwardIcon>(zcom::FastForwardDirection::RIGHT);
    _skipForwardsIcon->SetBaseSize(60, 60);
    _skipForwardsIcon->SetVerticalOffsetPercent(0.5f);
    _skipForwardsIcon->SetHorizontalOffsetPixels(-100);
    _skipForwardsIcon->SetHorizontalAlignment(zcom::Alignment::END);
    _skipForwardsIcon->SetProperty(shadow);

    _pauseIcon = Create<zcom::PauseIcon>();
    _pauseIcon->SetBaseSize(60, 60);
    _pauseIcon->SetOffsetPercent(0.5f, 0.5f);
    _pauseIcon->SetProperty(shadow);

    _resumeIcon = Create<zcom::ResumeIcon>();
    _resumeIcon->SetBaseSize(60, 60);
    _resumeIcon->SetOffsetPercent(0.5f, 0.5f);
    _resumeIcon->SetProperty(shadow);

    _volumeIcon = Create<zcom::VolumeIcon>();
    _volumeIcon->SetBaseSize(60, 60);
    _volumeIcon->SetHorizontalOffsetPercent(0.5f);
    _volumeIcon->SetVerticalOffsetPixels(60);
    _volumeIcon->SetProperty(shadow);

    _waveform = Create<zcom::Waveform>();
    _waveform->SetBaseSize(1000, 200);
    _waveform->SetHorizontalAlignment(zcom::Alignment::CENTER);
    zcom::PROP_Shadow waveShadow;
    waveShadow.offsetX = 0;
    waveShadow.offsetY = 0;
    waveShadow.blurStandardDeviation = 2.0f;
    waveShadow.color = D2D1::ColorF(0);
    _waveform->SetProperty(waveShadow);
    _waveform->SetVisible(false);

    _canvas->AddComponent(_skipBackwardsIcon.get());
    _canvas->AddComponent(_skipForwardsIcon.get());
    _canvas->AddComponent(_loadingCircle.get());
    _canvas->AddComponent(_pauseIcon.get());
    _canvas->AddComponent(_resumeIcon.get());
    _canvas->AddComponent(_volumeIcon.get());
    _canvas->AddComponent(_playbackControllerPanel.get());
    //_canvas->AddComponent(_timeHoverPanel.get());
    _canvas->AddComponent(_waveform.get());
    //_canvas->AddComponent(_streamMenuPanel.get());
    //_canvas->AddComponent(_videoStreamMenuPanel.get());
    //_canvas->AddComponent(_audioStreamMenuPanel.get());
    //_canvas->AddComponent(_subtitleStreamMenuPanel.get());
    //componentCanvas.AddComponent(controlBar);
}

void PlaybackScene::_Uninit()
{
    zcom::SafeFullRelease((IUnknown**)&_ccanvas);
    zcom::SafeFullRelease((IUnknown**)&_videoFrameBitmap);
    zcom::SafeFullRelease((IUnknown**)&_subtitleFrameBitmap);

    _canvas->ClearComponents();
    _controlBar->ClearItems();
    _playbackControllerPanel->ClearItems();

    _controlBar = nullptr;
    _controlBarBackground = nullptr;
    _playbackController = nullptr;
    _playbackControllerPanel = nullptr;

    _loadingCircle = nullptr;
    _skipBackwardsIcon = nullptr;
    _skipForwardsIcon = nullptr;
    _pauseIcon = nullptr;
    _resumeIcon = nullptr;
    _volumeIcon = nullptr;

    _shortcutHandler = nullptr;
}

void PlaybackScene::_Focus()
{
    // Add playback controller first, so that it handles keys first
    _app->keyboardManager.AddHandler(_playbackController.get());
    GetKeyboardState(_playbackController->KeyStates());
    _app->keyboardManager.AddHandler(_shortcutHandler.get());
    GetKeyboardState(_shortcutHandler->KeyStates());
}

void PlaybackScene::_Unfocus()
{
    _app->keyboardManager.RemoveHandler(_playbackController.get());
    _app->keyboardManager.RemoveHandler(_shortcutHandler.get());
    _app->window.SetCursorVisibility(true);
}

void PlaybackScene::_Update()
{
    // Close playback scene if playback init failed
    if (_playback->InitFailed())
    {
        _app->UninitScene(GetName());
        return;
    }

    // Keep control panel fixed while a menu is open or scene is unfocused
    if (_app->Overlay()->MenuOpened() || !Focused())
        _playbackControllerPanel->SetFixed(true);
    else
        _playbackControllerPanel->SetFixed(false);

    // Show loading circle if needed
    _loadingCircle->SetVisible(false);
    if (_playback->Initializing())
    {
        _loadingCircle->SetVisible(true);
    }
    else
    {
        auto info = _playback->Controller()->Loading();
        _loadingCircle->SetVisible(info.loading);
        _loadingCircle->SetLoadingText(info.message);
    }

    if (!_playback->Initializing())
    {
        // Prevent screen turning off while playing
        if (!_playback->Controller()->Paused())
            _app->window.ResetScreenTimer();
    }

    // Update UI
    _canvas->Update();

    //_UpdatePermissions();

    // Check for redraw
    if (!_playback->Initializing())
    {
        if (_playback->VideoAdapter()->FrameChanged())
        {
            _redraw = true;
            _videoFrameChanged = true;
        }
        if (_playback->SubtitleAdapter()->FrameChanged())
        {
            _redraw = true;
            _subtitleFrameChanged = true;
        }
    }
}

bool PlaybackScene::_Redraw()
{
    return _redraw || !_ccanvas || _canvas->Redraw();
}

ID2D1Bitmap* PlaybackScene::_Draw(Graphics g)
{
    _redraw = false;

    // Create _ccanvas
    if (!_ccanvas)
    {
        g.target->CreateBitmap(
            D2D1::SizeU(_canvas->GetWidth(), _canvas->GetHeight()),
            nullptr,
            0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET,
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
            ),
            &_ccanvas
        );
        g.refs->push_back({ (IUnknown**)&_ccanvas, "Playback scene common canvas" });
    }

    ID2D1Image* stash;
    g.target->GetTarget(&stash);
    g.target->SetTarget(_ccanvas);

    //g.target->Clear(D2D1::ColorF(0.05f, 0.05f, 0.05f));
    g.target->Clear(D2D1::ColorF(0));

    if (!_playback->Initializing())
    {
        //const VideoFrame& videoFrame = _playback->VideoAdapter()->GetVideoData();
        //const VideoFrame& subtitleFrame = _playback->VideoAdapter()->GetSubtitleData();
        if (!_videoFrameBitmap)
            _videoFrameChanged = true;
        if (!_subtitleFrameBitmap)
            _subtitleFrameChanged = true;

        IVideoFrame* videoFrame = _playback->VideoAdapter()->GetFrameData();
        ISubtitleFrame* subtitleFrame = _playback->SubtitleAdapter()->GetFrameData();

        int subtitleDrawAreaWidth = 0;
        int subtitleDrawAreaHeight = 0;

        // Update video bitmap
        if (_videoFrameChanged)
            if (videoFrame)
                videoFrame->DrawFrame(g, &_videoFrameBitmap);

        if (_videoFrameBitmap && videoFrame)
        {
            subtitleDrawAreaWidth = videoFrame->GetWidth();
            subtitleDrawAreaHeight = videoFrame->GetHeight();
        }
        else
        {
            subtitleDrawAreaWidth = _ccanvas->GetSize().width;
            subtitleDrawAreaHeight = _ccanvas->GetSize().height;
        }

        // Update subtitle bitmap
        if (_subtitleFrameChanged)
        {
            _subPos.x = 0;
            _subPos.y = 0;
            if (subtitleFrame)
                subtitleFrame->DrawFrame(g, &_subtitleFrameBitmap, _subPos, subtitleDrawAreaWidth, subtitleDrawAreaHeight);
        }

        // Determine video frame destination dimensions
        D2D1_RECT_F destRectVideo;
        D2D1_RECT_F srcRectVideo;
        if (_videoFrameBitmap)
        {
            // Scale frame to preserve aspect ratio
            srcRectVideo = D2D1::RectF(0.0f, 0.0f, videoFrame->GetWidth(), videoFrame->GetHeight());
            float tWidth = g.target->GetSize().width;
            float tHeight = g.target->GetSize().height;
            if (videoFrame->GetWidth() / (float)videoFrame->GetHeight() < tWidth / tHeight)
            {
                float scale = videoFrame->GetHeight() / tHeight;
                float newWidth = videoFrame->GetWidth() / scale;
                destRectVideo = D2D1::Rect
                (
                    (tWidth - newWidth) * 0.5f,
                    0.0f,
                    (tWidth - newWidth) * 0.5f + newWidth,
                    tHeight
                );
            }
            else if (videoFrame->GetWidth() / (float)videoFrame->GetHeight() > tWidth / tHeight)
            {
                float scale = videoFrame->GetWidth() / tWidth;
                float newHeight = videoFrame->GetHeight() / scale;
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

        // Determine subtitle frame destination dimensions
        D2D1_RECT_F destRectSubtitles;
        D2D1_RECT_F srcRectSubtitles;
        if (_subtitleFrameBitmap)
        {
            srcRectSubtitles = D2D1::RectF(
                0,
                0,
                _subtitleFrameBitmap->GetSize().width,
                _subtitleFrameBitmap->GetSize().height
            );

            if (_videoFrameBitmap)
            {
                float videoScale = (destRectVideo.right - destRectVideo.left) / (srcRectVideo.right - srcRectVideo.left);

                // Calculate destination rect size and scale it to video draw dimensions
                destRectSubtitles.left = _subPos.x * videoScale;
                destRectSubtitles.top = _subPos.y * videoScale;
                destRectSubtitles.right = (_subPos.x + srcRectSubtitles.right) * videoScale;
                destRectSubtitles.bottom = (_subPos.y + srcRectSubtitles.bottom) * videoScale;

                // Position destination rect correctly
                destRectSubtitles.left += destRectVideo.left;
                destRectSubtitles.top += destRectVideo.top;
                destRectSubtitles.right += destRectVideo.left;
                destRectSubtitles.bottom += destRectVideo.top;
            }
            else
            {
                // If no video, use absolute coordinates
                destRectSubtitles.left = _subPos.x;
                destRectSubtitles.top = _subPos.y;
                destRectSubtitles.right = _subPos.x + srcRectSubtitles.right;
                destRectSubtitles.bottom = _subPos.y + srcRectSubtitles.bottom;
            }
        }

        // Draw video
        if (_videoFrameBitmap)
            g.target->DrawBitmap(_videoFrameBitmap, destRectVideo, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRectVideo);

        // Draw subtitles
        if (_subtitleFrameBitmap)
            g.target->DrawBitmap(_subtitleFrameBitmap, destRectSubtitles, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRectSubtitles);

        _videoFrameChanged = false;
        _subtitleFrameChanged = false;
    }

    // Draw UI
    if (_canvas->Redraw())
        _canvas->Draw(g);
    g.target->DrawBitmap(_canvas->Image());

    // Unstash
    g.target->SetTarget(stash);
    stash->Release();

    return _ccanvas;
}

ID2D1Bitmap* PlaybackScene::_Image()
{
    return _ccanvas;
}

void PlaybackScene::_Resize(int width, int height)
{
    zcom::SafeFullRelease((IUnknown**)&_ccanvas);
    _redraw = true;
}

bool PlaybackScene::_HandleKeyDown(BYTE keyCode)
{
    switch (keyCode)
    {
    case VK_ESCAPE: // Fullscreen exit
    {
        if (_app->Fullscreen())
            _app->Fullscreen(false);
        break;
    }
    case VK_SPACE: // Pause toggle
    {
        if (_playback->Initializing())
            break;
        auto user = _app->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        if (_playback->Controller()->Paused())
        {
            _resumeIcon->SetVisible(false);
            _pauseIcon->Show();
        }
        else
        {
            _pauseIcon->SetVisible(false);
            _resumeIcon->Show();
        }
        break;
    }
    case VK_LEFT: // Show seek backward icon
    {
        if (_playback->Initializing())
            break;
        auto user = _app->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        int seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT), 15).Value();
        if (_shortcutHandler->KeyState(VK_CONTROL))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_LARGE), 60).Value();
        if (_shortcutHandler->KeyState(VK_SHIFT))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_SMALL), 5).Value();
        _skipBackwardsIcon->Show(seekAmount);
        break;
    }
    case VK_RIGHT: // Show seek forward icon
    {
        if (_playback->Initializing())
            break;
        auto user = _app->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
            break;

        int seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT), 15).Value();
        if (_shortcutHandler->KeyState(VK_CONTROL))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_LARGE), 60).Value();
        if (_shortcutHandler->KeyState(VK_SHIFT))
            seekAmount = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_SEEK_AMOUNT_SMALL), 5).Value();
        _skipForwardsIcon->Show(seekAmount);
        break;
    }
    case VK_UP: // Volume up
    {
        if (_playback->Initializing()) break;

        _volumeIcon->Show(roundf(_playbackController->GetVolumeSliderValue() * 100.0f));
        break;
    }
    case VK_DOWN: // Volume down
    {
        if (_playback->Initializing()) break;

        _volumeIcon->Show(roundf(_playbackController->GetVolumeSliderValue() * 100.0f));
        break;
    }
    case 'V':
    {
        _waveform->SetVisible(!_waveform->GetVisible());
        break;
    }
    }
    return false;
}