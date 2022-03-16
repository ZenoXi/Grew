#include "HostPlaybackController.h"

HostPlaybackController::HostPlaybackController(MediaPlayer* player, MediaHostDataProvider* dataProvider)
    : BasePlaybackController(player, dataProvider)
{
    auto destUsers = dataProvider->GetDestinationUsers();
    for (auto user : destUsers)
        _destinationUsers.push_back({ user, 0 });

    _playRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::RESUME_REQUEST);
    _pauseRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PAUSE_REQUEST);
    _seekRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::SEEK_REQUEST);
    _seekFinishedReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::SEEK_FINISHED);
    _playbackPositionReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_POSITION);
    _syncPauseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::SYNC_PAUSE);

    _StartSeeking();
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::HOST_CONTROLLER_READY), _GetUserIds());
}

void HostPlaybackController::Update()
{
    _SyncPlayback();

    // Process packets
    _CheckForPlayRequest();
    _CheckForPauseRequest();
    _CheckForSeekRequest();
    _CheckForSeekFinished();
    _CheckForPlaybackPosition();
    _CheckForSyncPause();


    if (_loading)
    {
        if (_player->Recovered())
        {
            std::cout << "Loading finished!" << std::endl;
            _loading = false;
            //if (!_paused)
            //{
            //    _player->StartTimer();
            //}
        }
    }

    _waitTimer.Update();
    if (_waiting && !_paused && _waitTimer.Now().GetTicks() > _waitLeft)
    {
        std::cout << "Finished waiting" << std::endl;
        _waiting = false;
    }

    if (!_waiting && !_paused && !_player->TimerRunning())
    {
        // Bypass _Play() check
        _paused = true;
        // _Play() also checks Loading()
        _Play();
    }

    if (_player->TimerRunning())
    {
        Duration duration = _dataProvider->MediaDuration();
        if (_player->TimerPosition().GetTicks() >= duration.GetTicks())
        {
            _finished = true;
            _Pause();
        }
    }
}

void HostPlaybackController::_CheckForPlayRequest()
{
    if (!_playRequestReceiver)
        return;

    while (_playRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _playRequestReceiver->GetPacket();
        Play();
    }
}

void HostPlaybackController::_CheckForPauseRequest()
{
    if (!_pauseRequestReceiver)
        return;

    while (_pauseRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _pauseRequestReceiver->GetPacket();
        Pause();
    }
}

void HostPlaybackController::_CheckForSeekRequest()
{
    if (!_seekRequestReceiver)
        return;

    while (_seekRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _seekRequestReceiver->GetPacket();
        auto seekData = packetPair.first.Cast<IMediaDataProvider::SeekData>();

        // Buffer the seek data
        if (_seeking)
        {
            if (seekData.time.GetTicks() > -1)
                _bufferedSeekData.time = seekData.time;
            if (seekData.videoStreamIndex != std::numeric_limits<int>::min())
                _bufferedSeekData.videoStreamIndex = seekData.videoStreamIndex;
            if (seekData.audioStreamIndex != std::numeric_limits<int>::min())
                _bufferedSeekData.audioStreamIndex = seekData.audioStreamIndex;
            if (seekData.subtitleStreamIndex != std::numeric_limits<int>::min())
                _bufferedSeekData.subtitleStreamIndex = seekData.subtitleStreamIndex;
            continue;
        }

        // Seek
        if (seekData.time.GetTicks() == -1)
            seekData.time = _player->TimerPosition();
        _player->StopTimer();
        _player->SetTimerPosition(seekData.time);
        _player->WaitDiscontinuity();

        IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);

        if (seekData.videoStreamIndex != std::numeric_limits<int>::min())
        {
            _player->SetVideoStream(std::move(seekResult.videoStream));
            _currentVideoStream = seekData.videoStreamIndex;
        }
        if (seekData.audioStreamIndex != std::numeric_limits<int>::min())
        {
            _player->SetAudioStream(std::move(seekResult.audioStream));
            _currentAudioStream = seekData.audioStreamIndex;
        }
        if (seekData.subtitleStreamIndex != std::numeric_limits<int>::min())
        {
            _player->SetSubtitleStream(std::move(seekResult.subtitleStream));
            _currentSubtitleStream = seekData.subtitleStreamIndex;
        }

        _bufferedSeekData = IMediaDataProvider::SeekData();
        _StartSeeking();
        _loading = true;
        _finished = false;
    }
}

void HostPlaybackController::_CheckForSeekFinished()
{
    if (!_seekFinishedReceiver)
        return;

    while (_seekFinishedReceiver->PacketCount() > 0)
    {
        auto packetPair = _seekFinishedReceiver->GetPacket();

        for (auto& user : _destinationUsers)
        {
            if (user.id == packetPair.second)
            {
                user.seekCompleted = true;
                break;
            }
        }
    }

    bool allSeekCompleted = true;
    for (auto& user : _destinationUsers)
    {
        if (!user.seekCompleted)
        {
            allSeekCompleted = false;
            break;
        }
    }
    if (_seeking && allSeekCompleted && !_loading)
    {
        _seeking = false;
        _lastSeek = ztime::Main();

        // Check for buffered seek
        if (!_bufferedSeekData.Default())
        {
            // Seek again
            if (_bufferedSeekData.time.GetTicks() == -1)
                _bufferedSeekData.time = _player->TimerPosition();
            _player->SetTimerPosition(_bufferedSeekData.time);
            _player->WaitDiscontinuity();

            IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(_bufferedSeekData);

            if (_bufferedSeekData.videoStreamIndex != std::numeric_limits<int>::min())
            {
                _player->SetVideoStream(std::move(seekResult.videoStream));
                _currentVideoStream = _bufferedSeekData.videoStreamIndex;
            }
            if (_bufferedSeekData.audioStreamIndex != std::numeric_limits<int>::min())
            {
                _player->SetAudioStream(std::move(seekResult.audioStream));
                _currentAudioStream = _bufferedSeekData.audioStreamIndex;
            }
            if (_bufferedSeekData.subtitleStreamIndex != std::numeric_limits<int>::min())
            {
                _player->SetSubtitleStream(std::move(seekResult.subtitleStream));
                _currentSubtitleStream = _bufferedSeekData.subtitleStreamIndex;
            }

            _bufferedSeekData = IMediaDataProvider::SeekData();
            _StartSeeking();
            _loading = true;
            _finished = false;
        }
        else
        {
            // Resume playback
            std::cout << "Playback Resumed" << std::endl;
            if (!_paused)
                _Play();
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::HOST_SEEK_FINISHED), _GetUserIds(), 1);
            _lastSeek = ztime::Main();
        }
    }
}

void HostPlaybackController::_CheckForPlaybackPosition()
{
    if (!_playbackPositionReceiver)
        return;

    while (_playbackPositionReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackPositionReceiver->GetPacket();

        for (auto& user : _destinationUsers)
        {
            if (user.id == packetPair.second)
            {
                user.currentTime = TimePoint(packetPair.first.Cast<int64_t>());
                user.receiveTime = ztime::Main();
                user.timeOffset = user.currentTime - _player->TimerPosition();
                break;
            }
        }
    }
}

void HostPlaybackController::_CheckForSyncPause()
{
    if (!_syncPauseReceiver)
        return;

    while (_syncPauseReceiver->PacketCount() > 0)
    {
        auto packetPair = _syncPauseReceiver->GetPacket();
        
        _waitTimer.Reset();
        _waitTimer.Stop();
        _waitLeft = packetPair.first.Cast<int64_t>();
        _waiting = true;
        std::cout << "Started waiting" << std::endl;

        if (_player->TimerRunning())
        {
            _waitTimer.Start();
            _player->StopTimer();
        }
    }
}

void HostPlaybackController::Play()
{
    _Play();
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::RESUME), _GetUserIds(), 1);
}

void HostPlaybackController::Pause()
{
    _Pause();
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PAUSE), _GetUserIds(), 1);
}

void HostPlaybackController::_Play()
{
    if (_paused)
    {
        if (_finished)
        {
            Seek(0);
        }
        if (_CanPlay())
        {
            if (_waiting)
            {
                _waitTimer.Reset();
            }
            else
            {
                _player->StartTimer();
            }
        }
        _paused = false;
    }
}

void HostPlaybackController::_Pause()
{
    if (!_paused)
    {
        if (_waiting)
        {
            _waitTimer.Update();
            _waitTimer.Stop();
            _waitLeft -= _waitTimer.Now().GetTicks();
        }
        _player->StopTimer();
        _paused = true;
    }
}

bool HostPlaybackController::_CanPlay() const
{
    return !_loading && !_seeking;
}

void HostPlaybackController::Seek(TimePoint time)
{
    if (_seeking)
    {
        _bufferedSeekData.time = time;
        return;
    }

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
    _StartSeeking();
    _loading = true;
    _finished = false;
}

void HostPlaybackController::SetVideoStream(int index)
{
    if (_seeking)
    {
        _bufferedSeekData.videoStreamIndex = index;
        return;
    }

    _player->StopTimer();
    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.videoStreamIndex = index;
    IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    _player->SetVideoStream(std::move(seekResult.videoStream));
    _player->WaitDiscontinuity();
    _currentVideoStream = index;
    _StartSeeking();
    _loading = true;
    _finished = false;
}

void HostPlaybackController::SetAudioStream(int index)
{
    if (_seeking)
    {
        _bufferedSeekData.audioStreamIndex = index;
        return;
    }

    _player->StopTimer();
    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.audioStreamIndex = index;
    IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    _player->SetAudioStream(std::move(seekResult.audioStream));
    _player->WaitDiscontinuity();
    _currentAudioStream = index;
    _StartSeeking();
    _loading = true;
    _finished = false;
}

void HostPlaybackController::SetSubtitleStream(int index)
{
    if (_seeking)
    {
        _bufferedSeekData.subtitleStreamIndex = index;
        return;
    }

    _player->StopTimer();
    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.subtitleStreamIndex = index;
    IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    _player->SetSubtitleStream(std::move(seekResult.subtitleStream));
    _player->WaitDiscontinuity();
    _currentSubtitleStream = index;
    _StartSeeking();
    _loading = true;
    _finished = false;
}

IPlaybackController::LoadingInfo HostPlaybackController::Loading() const
{
    if (_waiting)
        return { true, L"Syncing" };
    return { !_CanPlay(), L"" };
}

void HostPlaybackController::_StartSeeking()
{
    _waiting = false;
    _seeking = true;
    for (auto& user : _destinationUsers)
        user.seekCompleted = false;
}

void HostPlaybackController::_SyncPlayback()
{
    // Do not sync if playback isn't happening
    if (!_CanPlay())
        return;
    // Do not sync for a couple of seconds after seeking
    if ((ztime::Main() - _lastSeek).GetDuration(SECONDS) < 2)
        return;
    // Do not sync again right after syncing
    if ((ztime::Main() - _lastSync).GetDuration(SECONDS) < 5)
        return;

    // Find largest sync gap between user
    Duration mostAheadOffset = 0;
    Duration mostBehindOffset = 0;
    int indexAhead = -1;
    int indexBehind = -1;
    for (int i = 0; i < _destinationUsers.size(); i++)
    {
        if (_destinationUsers[i].timeOffset > mostAheadOffset)
        {
            mostAheadOffset = _destinationUsers[i].timeOffset;
            indexAhead = i;
        }
        if (_destinationUsers[i].timeOffset < mostBehindOffset)
        {
            mostBehindOffset = _destinationUsers[i].timeOffset;
            indexBehind = i;
        }
    }

    // Fix large offsets
    Duration deltaOffset = mostAheadOffset - mostBehindOffset;
    if (deltaOffset.GetDuration(MILLISECONDS) > 1000)
    {
        std::cout << "Fix offset of " << deltaOffset.GetDuration(MILLISECONDS) << "ms" << std::endl;

        // Pause host itself
        if (indexBehind != -1)
        {
            int64_t thisId = znet::NetworkInterface::Instance()->ThisUser().id;
            int64_t pauseTicks = -mostBehindOffset.GetTicks();
            znet::NetworkInterface::Instance()->Send(
                znet::Packet((int)znet::PacketType::SYNC_PAUSE).From(pauseTicks), { thisId });
        }

        // Pause others
        for (int i = 0; i < _destinationUsers.size(); i++)
        {
            if (i == indexBehind)
                continue;

            int64_t pauseTicks = (_destinationUsers[i].timeOffset - mostBehindOffset).GetTicks();
            znet::NetworkInterface::Instance()->Send(
                znet::Packet((int)znet::PacketType::SYNC_PAUSE).From(pauseTicks), { _destinationUsers[i].id }, 2);
        }

        _lastSync = ztime::Main();
    }
}