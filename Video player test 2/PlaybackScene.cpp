#include "App.h" // App.h must be included first
#include "PlaybackScene.h"

bool leftClicked = false;
bool rightClicked = false;
bool upClicked = false;
bool downClicked = false;
bool fClicked = false;
bool spaceClicked = false;

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

    _dataProvider = new LocalFileDataProvider(opt.fileName);

    // Initialize player
    //_player = new MediaPlayerOld(opt.mode, opt.fileName);

    //if (opt.mode == PlaybackMode::CLIENT)
    //{
    //    _player = new MediaPlayer(opt.ip, opt.port);
    //}
    //else if (opt.mode == PlaybackMode::SERVER)
    //{
    //    _player = new MediaPlayer(opt.port, opt.fileName);
    //}
    //else if (opt.mode == PlaybackMode::OFFLINE)
    //{
    //    _player = new MediaPlayer(0, opt.fileName);
    //}

    // Initialize scene
    //_controlBar = new Panel();
    //_controlBar->SetParentWidthPercent(1.0f);
    //_controlBar->SetParentHeightPercent(1.0f);
    //_controlBar->SetBackgroundColor(D2D1::ColorF::ColorF(0.0f, 0.0f, 0.0f, 0.7f));

    //_seekBar = new SeekBar(_player->GetDuration().GetDuration());
    //_seekBar->SetParentWidthPercent(1.0f);
    //_seekBar->SetBaseWidth(-20);
    //_seekBar->SetBaseHeight(50);
    //_seekBar->SetHorizontalOffsetPercent(0.5f);

    //_volumeSlider = new VolumeSlider(_player->GetVolume());
    //_volumeSlider->SetBaseWidth(150);
    //_volumeSlider->SetBaseHeight(20);
    //_volumeSlider->SetHorizontalOffsetPixels(10);
    //_volumeSlider->SetVerticalOffsetPixels(50);

    //_playButton = new PlayButton();
    //_playButton->SetBaseWidth(40);
    //_playButton->SetBaseHeight(40);
    //_playButton->SetHorizontalOffsetPercent(0.5f);
    //_playButton->SetVerticalOffsetPixels(50);
    //_playButton->SetPaused(true);

    //_controlBar->AddItem(_seekBar);
    //_controlBar->AddItem(_volumeSlider);
    //_controlBar->AddItem(_playButton);

    //_bottomControlPanel = new BottomControlPanel(_controlBar);
    //_bottomControlPanel->SetParentWidthPercent(1.0f);
    //_bottomControlPanel->SetBaseHeight(100);
    //_bottomControlPanel->SetVerticalAlignment(Alignment::END);

    
    
    //_canvas->AddComponent(_bottomControlPanel); // ########################################### UNCOMMENT LATER



    //componentCanvas.AddComponent(controlBar);
    _canvas->Resize(App::Instance()->window.width, App::Instance()->window.height);
}

void PlaybackScene::_Uninit()
{
    //delete _player;
}

void PlaybackScene::_Update()
{
    //_player->Update();

    // Check for fullscreen key
    if (ButtonPressed(fClicked, 'F'))
    {
        if (App::Instance()->window.GetFullscreen())
        {
            App::Instance()->window.SetFullscreen(false);
        }
        else
        {
            App::Instance()->window.SetFullscreen(true);
        }
    }

    // Check for pause key
    //if (ButtonPressed(spaceClicked, VK_SPACE))
    //{
    //    std::cout << "Spacebar clicked.\n";
    //    if (_player->Paused())
    //    {
    //        _player->Play(true);
    //        _playButton->SetPaused(false);
    //    }
    //    else
    //    {
    //        _player->Pause(true);
    //        _playButton->SetPaused(true);
    //    }
    //}
    //if (_player->Paused() != _playButton->GetPaused())
    //{
    //    if (_playButton->Clicked())
    //    {
    //        std::cout << "Pause synced.\n";
    //        if (_playButton->GetPaused())
    //        {
    //            _player->Pause(true);
    //        }
    //        else
    //        {
    //            _player->Play(true);
    //        }
    //    }
    //    else
    //    {
    //        _playButton->SetPaused(_player->Paused());
    //    }
    //}

    // Check for seek commands
    //TimePoint seekTo = _seekBar->SeekTime();
    //if (seekTo.GetTime() != -1)
    //{
    //    _player->Seek(seekTo, true);
    //}
    //else
    //{
    //    bool seekBack = ButtonPressed(leftClicked, VK_LEFT);
    //    bool seekForward = ButtonPressed(rightClicked, VK_RIGHT);
    //    if (seekBack || seekForward)
    //    {
    //        if (seekBack)
    //        {
    //            _player->Seek(ztime::Game() - Duration(5, SECONDS), true);
    //        }
    //        else if (seekForward)
    //        {
    //            _player->Seek(ztime::Game() + Duration(5, SECONDS), true);
    //        }
    //    }
    //}

    // Check for volume commands
    //if (ButtonPressed(upClicked, VK_UP))
    //{
    //    float newVolume = _player->GetVolume() + 0.05f;
    //    _player->SetVolume(newVolume);
    //    _volumeSlider->SetVolume(newVolume);
    //}
    //if (ButtonPressed(downClicked, VK_DOWN))
    //{
    //    float newVolume = _player->GetVolume() - 0.05f;
    //    _player->SetVolume(newVolume);
    //    _volumeSlider->SetVolume(newVolume);
    //}
    //if (_volumeSlider->GetVolume() != _player->GetVolume())
    //{
    //    _player->SetVolume(_volumeSlider->GetVolume());
    //}

    // Update buffered time in seekbar
    //Duration bDuration = _player->GetBufferedDuration();
    //if (bDuration.GetDuration() != -1)
    //{
    //    _seekBar->SetBufferedDuration(bDuration);
    //}

    

    if (_controller)
    {
        _controller->Update();

        if (ButtonPressed(rightClicked, VK_RIGHT))
        {
            _controller->Seek(_controller->CurrentTime() + Duration(5, SECONDS));
        }
    }
    if (_mediaPlayer)
    {
        _mediaPlayer->Update();
    }

    if (!_mediaPlayer)
    {
        if (!_dataProvider->Initializing() && !_dataProvider->InitFailed())
        {
            _dataProvider->Start();

            _videoAdapter = new IVideoOutputAdapter();
            auto audioStream = _dataProvider->CurrentAudioStream();
            _audioAdapter = new XAudio2_AudioOutputAdapter(audioStream->channels, audioStream->sampleRate);
            _mediaPlayer = new MediaPlayer(
                _dataProvider,
                std::unique_ptr<IVideoOutputAdapter>(_videoAdapter),
                std::unique_ptr<IAudioOutputAdapter>(_audioAdapter)
            );
            _controller = new BasePlaybackController(_mediaPlayer, _dataProvider);
        }
    }

    // Update UI
    _canvas->Update();
}

ID2D1Bitmap1* PlaybackScene::_Draw(Graphics g)
{
    if (_mediaPlayer)
    {
        const VideoFrame& videoFrame = _videoAdapter->GetVideoData();
        if (videoFrame.GetWidth() && videoFrame.GetHeight())
        {
            // Get video frame
            //const FrameData* fd = _player->GetCurrentFrame();
            D2D1_BITMAP_PROPERTIES props;
            props.dpiX = 96.0f;
            props.dpiY = 96.0f;
            props.pixelFormat = D2D1::PixelFormat
            (
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE
            );
            ID2D1Bitmap* frame;
            g.target->CreateBitmap
            (
                D2D1::SizeU(videoFrame.GetWidth(), videoFrame.GetHeight()),
                props,
                &frame
            );
            D2D1_RECT_U rect = D2D1::RectU(0, 0, videoFrame.GetWidth(), videoFrame.GetHeight());
            frame->CopyFromMemory(&rect, videoFrame.GetBytes(), videoFrame.GetWidth() * 4);

            // Scale frame to preserve aspect ratio
            D2D1_RECT_F srcRect = D2D1::RectF(0.0f, 0.0f, videoFrame.GetWidth(), videoFrame.GetHeight());
            D2D1_RECT_F destRect;
            float tWidth = g.target->GetSize().width;
            float tHeight = g.target->GetSize().height;
            if (videoFrame.GetWidth() / (float)videoFrame.GetHeight() < tWidth / tHeight)
            {
                float scale = videoFrame.GetHeight() / tHeight;
                float newWidth = videoFrame.GetWidth() / scale;
                destRect = D2D1::Rect
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
                destRect = D2D1::Rect
                (
                    0.0f,
                    (tHeight - newHeight) * 0.5f,
                    tWidth,
                    (tHeight - newHeight) * 0.5f + newHeight
                );
            }
            else
            {
                destRect = D2D1::RectF(0.0f, 0.0f, tWidth, tHeight);
            }

            // Draw frame
            g.target->DrawBitmap(frame, destRect, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRect);
            frame->Release();
        }
    }

    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void PlaybackScene::_Resize(int width, int height)
{
    
}