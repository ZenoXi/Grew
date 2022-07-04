#include "Playback.h"

#include "XAudio2_AudioOutputAdapter.h"
#include "Options.h"

Playback::Playback()
{

}

Playback::~Playback()
{

}

void Playback::Start(std::unique_ptr<IMediaDataProvider> dataProvider, std::unique_ptr<BasePlaybackController> controller)
{
    _dataProvider = std::move(dataProvider);
    _controller = std::move(controller);
    _initFailed = false;
}

void Playback::Stop()
{
    _controller.reset();
    _player.reset();
    _dataProvider.reset();

    _videoAdapter.reset();
    _audioAdapter.reset();
}

void Playback::Update()
{
    if (!_player)
    {
        if (_dataProvider && !_dataProvider->Initializing())
        {
            if (!_dataProvider->InitFailed())
            {
                // Start provider
                _dataProvider->Start();

                // Create adapters
                _videoAdapter = std::make_unique<IVideoOutputAdapter>();
                auto audioStream = _dataProvider->CurrentAudioStream();
                if (audioStream)
                    _audioAdapter = std::make_unique<XAudio2_AudioOutputAdapter>(audioStream->channels, audioStream->sampleRate);
                else
                    _audioAdapter = std::make_unique<XAudio2_AudioOutputAdapter>();

                // Create media player
                _player = std::make_unique<MediaPlayer>(_dataProvider.get(), _videoAdapter.get(), _audioAdapter.get());

                // Attach player to controller
                _controller->AttachMediaPlayer(_player.get());

                // Get volume from saved options
                float volume = 0.2f;
                try { volume = std::stof(Options::Instance()->GetValue("volume")); }
                catch (std::out_of_range) {}
                catch (std::invalid_argument) {}
                _controller->SetVolume(volume);

                _controller->Play();
            }
            else
            {
                _initFailed = true;
            }
        }
    }

    if (_player)
    {
        _controller->Update();
        _player->Update();
    }
}

bool Playback::Initializing() const
{
    return !_player;
}

bool Playback::InitFailed() const
{
    return _initFailed;
}

IPlaybackController* Playback::Controller() const
{
    return _controller.get();
}

IMediaDataProvider* Playback::DataProvider() const
{
    return _dataProvider.get();
}

IVideoOutputAdapter* Playback::VideoAdapter() const
{
    return _videoAdapter.get();
}

IAudioOutputAdapter* Playback::AudioAdapter() const
{
    return _audioAdapter.get();
}