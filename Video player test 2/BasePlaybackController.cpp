#include "BasePlaybackController.h"


BasePlaybackController::BasePlaybackController(MediaPlayer* player, IMediaDataProvider* dataProvider)
    : _player(player), _dataProvider(dataProvider)
{
    _videoStreams = _dataProvider->GetAvailableVideoStreams();
    _audioStreams = _dataProvider->GetAvailableAudioStreams();
    _subtitleStreams = _dataProvider->GetAvailableSubtitleStreams();
}

void BasePlaybackController::Update()
{
    if (_loading)
    {
        if (_player->Recovered())
        {
            _loading = false;
            _player->StartTimer();
        }
    }
}

void BasePlaybackController::Play()
{

}

void BasePlaybackController::Pause()
{

}

void BasePlaybackController::SetVolume(float volume)
{
    _volume = volume;
    _player->SetVolume(volume);
}

void BasePlaybackController::SetBalance(float balance)
{
    _balance = balance;
    _player->SetBalance(balance);
}

void BasePlaybackController::Seek(TimePoint time)
{
    _player->StopTimer();
    _dataProvider->Seek(time);
    _loading = true;
}

void BasePlaybackController::SetVideoStream(int index)
{

}

void BasePlaybackController::SetAudioStream(int index)
{

}

void BasePlaybackController::SetSubtitleStream(int index)
{

}

bool BasePlaybackController::Paused()
{
    return false;
}

float BasePlaybackController::GetVolume() const
{
    return 0.0f;
}

float BasePlaybackController::GetBalance() const
{
    return 0.0f;
}

TimePoint BasePlaybackController::CurrentTime() const
{
    return _player->TimerPosition();
}

int BasePlaybackController::CurrentVideoStream() const
{
    return 0;
}

int BasePlaybackController::CurrentAudioStream() const
{
    return 0;
}

int BasePlaybackController::CurrentSubtitleStream() const
{
    return 0;
}

Duration BasePlaybackController::GetDuration() const
{
    return 0;
}

std::vector<std::string> BasePlaybackController::GetAvailableVideoStreams() const
{
    return std::vector<std::string>();
}

std::vector<std::string> BasePlaybackController::GetAvailableAudioStreams() const
{
    return std::vector<std::string>();
}

std::vector<std::string> BasePlaybackController::GetAvailableSubtitleStreams() const
{
    return std::vector<std::string>();
}