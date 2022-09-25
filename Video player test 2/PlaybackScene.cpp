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

    _loadingCircle = Create<zcom::LoadingCircle>();
    _loadingCircle->SetBaseSize(100, 180);
    _loadingCircle->SetOffsetPercent(0.5f, 0.5f);
    _loadingCircle->SetVerticalOffsetPixels(40);
    _loadingCircle->SetVisible(false);

    _controlBar->AddItem(_controlBarBackground.get());
    _controlBar->AddItem(_playbackController.get());

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
    _canvas->AddComponent(_waveform.get());
    _canvas->AddOnLeftPressed([&](const zcom::EventTargets* targets)
    {
        // If the click didn't hit the playback controller, pause/resume
        if (!targets->Contains(_playbackController.get()))
        {
            if (_playback->Initializing())
                return false;
            auto user = _app->users.GetThisUser();
            if (user && !user->GetPermission(PERMISSION_MANIPULATE_PLAYBACK))
                return false;

            if (_playback->Controller()->Paused())
            {
                _playback->Controller()->Play();
                _resumeIcon->SetVisible(false);
                _pauseIcon->Show();
            }
            else
            {
                _playback->Controller()->Pause();
                _pauseIcon->SetVisible(false);
                _resumeIcon->Show();
            }
        }
        return false;
    }, { nullptr, "play/pause" });
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