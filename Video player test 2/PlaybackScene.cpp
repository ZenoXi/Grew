#include "App.h" // App.h must be included first
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "HostPlaybackController.h"
#include "ReceiverPlaybackController.h"

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

PlaybackScene::PlaybackScene() {}

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

    _audioStreamButton = new zcom::Button(L"Audio 2");
    _audioStreamButton->SetBaseSize(50, 20);
    _audioStreamButton->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _audioStreamButton->SetOffsetPixels(100, 60);
    _audioStreamButton->SetBorderVisibility(true);
    _audioStreamButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _audioStreamButton->SetOnActivated([&]()
    {
        if (_mediaPlayer)
        {
            _controller->SetAudioStream(1);
        }
    });

    _subtitleStreamButton = new zcom::Button(L"Sub 2");
    _subtitleStreamButton->SetBaseSize(50, 20);
    _subtitleStreamButton->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _subtitleStreamButton->SetOffsetPixels(160, 60);
    _subtitleStreamButton->SetBorderVisibility(true);
    _subtitleStreamButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _subtitleStreamButton->SetOnActivated([&]()
    {
        if (_mediaPlayer)
        {
            _controller->SetSubtitleStream(1);
        }
    });

    _controlBar->AddItem(_seekBar);
    _controlBar->AddItem(_volumeSlider);
    _controlBar->AddItem(_playButton);
    _controlBar->AddItem(_overlayButton);
    _controlBar->AddItem(_audioStreamButton);
    _controlBar->AddItem(_subtitleStreamButton);

    _playbackControllerPanel = new zcom::PlaybackControllerPanel(_controlBar);
    _playbackControllerPanel->SetParentSizePercent(1.0f, 1.0f);
    _playbackControllerPanel->AddItem(_controlBar);
    //_bottomControlPanel->SetZIndex(1);

    _canvas->AddComponent(_playbackControllerPanel);
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

    delete _controller;
    delete _mediaPlayer;
    delete _dataProvider;

    _audioAdapter = nullptr;
    _videoAdapter = nullptr;
    _mediaPlayer = nullptr;
    _controller = nullptr;
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
    // Prevent screen turning off
    App::Instance()->window.ResetScreenTimer();

    if (_mediaPlayer)
    {
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

            _videoAdapter = new IVideoOutputAdapter();
            auto audioStream = _dataProvider->CurrentAudioStream();
            if (audioStream)
                _audioAdapter = new XAudio2_AudioOutputAdapter(audioStream->channels, audioStream->sampleRate);
            else
                _audioAdapter = new XAudio2_AudioOutputAdapter(1, 1);

            _mediaPlayer = new MediaPlayer(
                _dataProvider,
                std::unique_ptr<IVideoOutputAdapter>(_videoAdapter),
                std::unique_ptr<IAudioOutputAdapter>(_audioAdapter)
            );

            if (_playbackMode == PlaybackMode::OFFLINE)
                _controller = new BasePlaybackController(_mediaPlayer, _dataProvider);
            else if (_playbackMode == PlaybackMode::SERVER)
                _controller = new HostPlaybackController(_mediaPlayer, (MediaHostDataProvider*)_dataProvider);
            else if (_playbackMode == PlaybackMode::CLIENT)
                _controller = new ReceiverPlaybackController(_mediaPlayer, (MediaReceiverDataProvider*)_dataProvider);

            if (!_startPaused)
                _controller->Play();

            _seekBar->SetDuration(_dataProvider->MediaDuration());
            _volumeSlider->SetValue(0.1f);
            _controller->SetVolume(_volumeSlider->GetVolume());
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
        if (_controller->Paused())
        {
            _controller->Play();
            _playButton->SetPaused(false);
        }
        else
        {
            _controller->Pause();
            _playButton->SetPaused(true);
        }
        break;
    }
    case VK_LEFT: // Seek back
    {
        int seekAmount = 15;
        if (_shortcutHandler->KeyState(VK_CONTROL)) seekAmount = 60;
        if (_shortcutHandler->KeyState(VK_SHIFT)) seekAmount = 5;
        _controller->Seek(_controller->CurrentTime() - Duration(seekAmount, SECONDS));
        break;
    }
    case VK_RIGHT: // Seek forward
    {
        int seekAmount = 15;
        if (_shortcutHandler->KeyState(VK_CONTROL)) seekAmount = 60;
        if (_shortcutHandler->KeyState(VK_SHIFT)) seekAmount = 5;
        _controller->Seek(_controller->CurrentTime() + Duration(seekAmount, SECONDS));
        break;
    }
    case VK_UP: // Volume up
    {
        App::Instance()->window.SetCursorVisibility(true);

        _volumeSlider->SetValue(_volumeSlider->GetValue() + 0.05f);
        _controller->SetVolume(_volumeSlider->GetVolume());
        break;
    }
    case VK_DOWN: // Volume down
    {
        App::Instance()->window.SetCursorVisibility(false);

        _volumeSlider->SetValue(_volumeSlider->GetValue() - 0.05f);
        _controller->SetVolume(_volumeSlider->GetVolume());
        break;
    }
    case VK_NUMPAD1: // Video stream 1
    {
        _controller->SetVideoStream(0);
        break;
    }
    case VK_NUMPAD2: // Video stream 2
    {
        _controller->SetVideoStream(1);
        break;
    }
    case VK_NUMPAD3: // Video stream 3
    {
        _controller->SetVideoStream(2);
        break;
    }
    case VK_NUMPAD4: // Audio stream 1
    {
        _controller->SetAudioStream(0);
        break;
    }
    case VK_NUMPAD5: // Audio stream 2
    {
        _controller->SetAudioStream(1);
        break;
    }
    case VK_NUMPAD6: // Audio stream 3
    {
        _controller->SetAudioStream(2);
        break;
    }
    case VK_NUMPAD7: // Subtitle stream 1
    {
        _controller->SetSubtitleStream(0);
        break;
    }
    case VK_NUMPAD8: // Subtitle stream 2
    {
        _controller->SetSubtitleStream(1);
        break;
    }
    case VK_NUMPAD9: // Subtitle stream 3
    {
        _controller->SetSubtitleStream(2);
        break;
    }
    }
}