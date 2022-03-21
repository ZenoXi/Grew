#include "BasePlaybackController.h"

BasePlaybackController::BasePlaybackController(MediaPlayer* player, IMediaDataProvider* dataProvider)
    : _player(player), _dataProvider(dataProvider), _timerController(player, true)
{
    _timerController.AddStop("loading");
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

    if (_player->TimerRunning())
    {
        Duration duration = _dataProvider->MediaDuration();
        if (_player->TimerPosition().GetTicks() >= duration.GetTicks())
        {
            _player->StopTimer();
            _finished = true;
        }
    }
}

void BasePlaybackController::Play()
{
    _paused = false;
    if (_finished)
    {
        Seek(0);
    }
    if (!Loading().loading)
    {
        _player->StartTimer();
    }
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

    if (time.GetTime() < 0)
    {
        time.SetTime(0);
    }
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
    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    _dataProvider->Seek(seekData);
    _loading = true;
    _finished = false;
}

void BasePlaybackController::SetVideoStream(int index)
{
    if (_player->Waiting()) return;

    _player->StopTimer();
    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.videoStreamIndex = index;
    IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    _player->SetVideoStream(std::move(seekResult.videoStream));
    //_player->SetVideoStream(_dataProvider->SetVideoStream(index, _player->TimerPosition()));
    _player->WaitDiscontinuity();
    _currentVideoStream = index;
    _loading = true;
    _finished = false;
}

void BasePlaybackController::SetAudioStream(int index)
{
    if (_player->Waiting()) return;

    _player->StopTimer();
    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.audioStreamIndex = index;
    IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    _player->SetAudioStream(std::move(seekResult.audioStream));
    //_player->SetAudioStream(_dataProvider->SetAudioStream(index, _player->TimerPosition()));
    _player->WaitDiscontinuity();
    _currentAudioStream = index;
    _loading = true;
    _finished = false;
}

void BasePlaybackController::SetSubtitleStream(int index)
{
    if (_player->Waiting()) return;

    _player->StopTimer();
    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.subtitleStreamIndex = index;
    IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    _player->SetSubtitleStream(std::move(seekResult.subtitleStream));
    //_player->SetSubtitleStream(_dataProvider->SetSubtitleStream(index, _player->TimerPosition()));
    _player->WaitDiscontinuity();
    _currentSubtitleStream = index;
    _loading = true;
    _finished = false;
}

bool BasePlaybackController::Paused() const
{
    return _paused;
}

IPlaybackController::LoadingInfo BasePlaybackController::Loading() const
{
    return { _loading, L"" };
}

bool BasePlaybackController::Finished() const
{
    return _finished;
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