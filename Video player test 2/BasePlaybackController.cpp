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
            if (!_paused)
            {
                _player->StartTimer();
            }
        }
    }
}

void BasePlaybackController::Play()
{
    _paused = false;
    _player->StartTimer();
}

void BasePlaybackController::Pause()
{
    _paused = true;
    _player->StopTimer();
}

void BasePlaybackController::SetVolume(float volume, bool bounded)
{
    if (bounded)
    {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
    }
    _volume = volume;
    _player->SetVolume(volume);
}

void BasePlaybackController::SetBalance(float balance, bool bounded)
{
    if (bounded)
    {
        if (balance < -1.0f) balance = -1.0f;
        if (balance > 1.0f) balance = 1.0f;
    }
    _balance = balance;
    _player->SetBalance(balance);
}

void BasePlaybackController::Seek(TimePoint time)
{
    if (_player->Waiting()) return;

    if (time.GetTime() < 0) time.SetTime(0);
    else
    {
        Duration mediaDuration = _dataProvider->MediaDuration();
        if (time.GetTime() > mediaDuration.GetDuration())
        {
            time.SetTime(mediaDuration.GetDuration());
        }
    }

    _player->StopTimer();
    _player->SetTimerPosition(time);
    _player->WaitDiscontinuity();
    _dataProvider->Seek(time);
    _loading = true;
}

void BasePlaybackController::SetVideoStream(int index)
{
    if (_player->Waiting()) return;

    _player->StopTimer();
    _player->SetVideoStream(_dataProvider->SetVideoStream(index, _player->TimerPosition()));
    _currentVideoStream = index;
    _loading = true;
}

void BasePlaybackController::SetAudioStream(int index)
{
    if (_player->Waiting()) return;

    _player->StopTimer();
    _player->SetAudioStream(_dataProvider->SetAudioStream(index, _player->TimerPosition()));
    _currentAudioStream = index;
    _loading = true;
}

void BasePlaybackController::SetSubtitleStream(int index)
{
    if (_player->Waiting()) return;

    _player->StopTimer();
    _player->SetSubtitleStream(_dataProvider->SetSubtitleStream(index, _player->TimerPosition()));
    _currentSubtitleStream = index;
    _loading = true;
}

bool BasePlaybackController::Paused()
{
    return _paused;
}

float BasePlaybackController::GetVolume() const
{
    return _volume;
}

float BasePlaybackController::GetBalance() const
{
    return _balance;
}

TimePoint BasePlaybackController::CurrentTime() const
{
    return _player->TimerPosition();
}

Duration BasePlaybackController::GetDuration() const
{
    return _dataProvider->MediaDuration();
}

Duration BasePlaybackController::GetBufferedDuration() const
{
    return _dataProvider->BufferedDuration();
}

int BasePlaybackController::CurrentVideoStream() const
{
    return _currentVideoStream;
}

int BasePlaybackController::CurrentAudioStream() const
{
    return _currentAudioStream;
}

int BasePlaybackController::CurrentSubtitleStream() const
{
    return _currentSubtitleStream;
}

std::vector<std::string> BasePlaybackController::GetAvailableVideoStreams() const
{
    return _videoStreams;
}

std::vector<std::string> BasePlaybackController::GetAvailableAudioStreams() const
{
    return _audioStreams;
}

std::vector<std::string> BasePlaybackController::GetAvailableSubtitleStreams() const
{
    return _subtitleStreams;
}