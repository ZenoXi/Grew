#include "HostPlaybackController.h"

#include "App.h"
#include "PlaybackScene.h"

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
            _timerController.RemoveStop("loading");
            _loading = false;
        }
    }

    if (_player->Lagging())
    {
        if (!_buffering)
        {
            _timerController.AddStop("buffering");
            _timerController.AddTimer(Duration(1, SECONDS));
            _buffering = true;
        }
    }
    else
    {
        if (_buffering)
        {
            _timerController.RemoveStop("buffering");
            _buffering = false;
        }
    }

    if (_player->TimerRunning())
    {
        Duration duration = _dataProvider->MediaDuration();
        if (_player->TimerPosition().GetTicks() >= duration.GetTicks())
        {
            _timerController.AddStop("finished");
            _finished = true;
            _Pause();
        }
    }

    _timerController.Update();
}

void HostPlaybackController::_CheckForPlayRequest()
{
    if (!_playRequestReceiver)
        return;

    while (_playRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _playRequestReceiver->GetPacket();
        _Play();
        znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::RESUME).From(packetPair.second), _GetUserIds(), 1);

        // Show notification
        Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        if (scene)
        {
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(1, SECONDS);
            ninfo.title = L"Playback resumed";
            ninfo.text = _UsernameFromId(packetPair.second);
            scene->ShowNotification(ninfo);
        }
    }
}

void HostPlaybackController::_CheckForPauseRequest()
{
    if (!_pauseRequestReceiver)
        return;

    while (_pauseRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _pauseRequestReceiver->GetPacket();
        _Pause();
        znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PAUSE).From(packetPair.second), _GetUserIds(), 1);

        // Show notification
        Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        if (scene)
        {
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(1, SECONDS);
            ninfo.title = L"Playback paused";
            ninfo.text = _UsernameFromId(packetPair.second);
            scene->ShowNotification(ninfo);
        }
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
        if (seekData.Default())
            continue;
        seekData.userId = packetPair.second;

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
            _bufferedSeekData.userId = seekData.userId;
            continue;
        }

        // Seek
        seekData.defaultTime = 0;
        if (seekData.time.GetTicks() == -1)
        {
            seekData.time = _player->TimerPosition();
            seekData.defaultTime = 1;
        }

        znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(seekData), _GetUserIds(), 1);
        _Seek(seekData);

        // Show notifications
        Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        if (scene)
        {
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(2, SECONDS);
            ninfo.text = _UsernameFromId(seekData.userId);
            if (!seekData.defaultTime)
            {
                std::wstringstream timeStr;
                int h = seekData.time.GetTime(HOURS);
                int m = seekData.time.GetTime(MINUTES) % 60;
                int s = seekData.time.GetTime(SECONDS) % 60;
                if (h > 0) timeStr << h << ":";
                if (m < 10) timeStr << "0" << m << ":";
                else timeStr << m << ":";
                if (s < 10) timeStr << "0" << s;
                else timeStr << s;
                ninfo.title = L"Seek to " + timeStr.str();
                scene->ShowNotification(ninfo);
            }
            if (seekData.videoStreamIndex != std::numeric_limits<int>::min())
            {
                ninfo.title = L"Video stream changed";
                scene->ShowNotification(ninfo);
            }
            if (seekData.audioStreamIndex != std::numeric_limits<int>::min())
            {
                ninfo.title = L"Audio stream changed";
                scene->ShowNotification(ninfo);
            }
            if (seekData.subtitleStreamIndex != std::numeric_limits<int>::min())
            {
                ninfo.title = L"Subtitle stream changed";
                scene->ShowNotification(ninfo);
            }
        }

        //_StartSeeking();
        //_timerController.AddStop("loading");
        //_timerController.RemoveStop("finished");
        //_loading = true;
        //_finished = false;
        //_player->SetTimerPosition(seekData.time);
        //_player->WaitDiscontinuity();

        //IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
        //if (seekData.videoStreamIndex != std::numeric_limits<int>::min())
        //{
        //    _player->SetVideoStream(std::move(seekResult.videoStream));
        //    _currentVideoStream = seekData.videoStreamIndex;
        //}
        //if (seekData.audioStreamIndex != std::numeric_limits<int>::min())
        //{
        //    _player->SetAudioStream(std::move(seekResult.audioStream));
        //    _currentAudioStream = seekData.audioStreamIndex;
        //}
        //if (seekData.subtitleStreamIndex != std::numeric_limits<int>::min())
        //{
        //    _player->SetSubtitleStream(std::move(seekResult.subtitleStream));
        //    _currentSubtitleStream = seekData.subtitleStreamIndex;
        //}
        //_bufferedSeekData = IMediaDataProvider::SeekData();
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
        _lastSeek = ztime::Main();

        // Check for buffered seek
        if (!_bufferedSeekData.Default())
        {
            // Seek again
            if (_bufferedSeekData.time.GetTicks() == -1)
            {
                _bufferedSeekData.time = _player->TimerPosition();
                _bufferedSeekData.defaultTime = 1;
            }

            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(_bufferedSeekData), _GetUserIds(), 1);
            _Seek(_bufferedSeekData);
            _bufferedSeekData = IMediaDataProvider::SeekData();

            //_player->SetTimerPosition(_bufferedSeekData.time);
            //_player->WaitDiscontinuity();

            //IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(_bufferedSeekData);

            //if (_bufferedSeekData.videoStreamIndex != std::numeric_limits<int>::min())
            //{
            //    _player->SetVideoStream(std::move(seekResult.videoStream));
            //    _currentVideoStream = _bufferedSeekData.videoStreamIndex;
            //}
            //if (_bufferedSeekData.audioStreamIndex != std::numeric_limits<int>::min())
            //{
            //    _player->SetAudioStream(std::move(seekResult.audioStream));
            //    _currentAudioStream = _bufferedSeekData.audioStreamIndex;
            //}
            //if (_bufferedSeekData.subtitleStreamIndex != std::numeric_limits<int>::min())
            //{
            //    _player->SetSubtitleStream(std::move(seekResult.subtitleStream));
            //    _currentSubtitleStream = _bufferedSeekData.subtitleStreamIndex;
            //}

            //_bufferedSeekData = IMediaDataProvider::SeekData();
            //_StartSeeking();
            //_timerController.AddStop("loading");
            //_timerController.RemoveStop("finished");
            //_loading = true;
            //_finished = false;
        }
        else
        {
            // Resume playback
            std::cout << "Playback Resumed" << std::endl;
            _timerController.RemoveStop("seeking");
            _seeking = false;

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
        
        _timerController.AddPlayTimer(packetPair.first.Cast<int64_t>());
    }
}

void HostPlaybackController::Play()
{
    _Play();
    int64_t thisId = znet::NetworkInterface::Instance()->ThisUser().id;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::RESUME).From(thisId), _GetUserIds(), 1);
}

void HostPlaybackController::Pause()
{
    _Pause();
    int64_t thisId = znet::NetworkInterface::Instance()->ThisUser().id;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PAUSE).From(thisId), _GetUserIds(), 1);
}

void HostPlaybackController::_Play()
{
    _timerController.Play();
    _paused = false;
}

void HostPlaybackController::_Pause()
{
    _timerController.Pause();
    _paused = true;
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
        _bufferedSeekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
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

    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    seekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(seekData), _GetUserIds(), 1);
    _Seek(seekData);

    //_StartSeeking();
    //_timerController.AddStop("loading");
    //_timerController.RemoveStop("finished");
    //_loading = true;
    //_finished = false;
    //_player->SetTimerPosition(time);
    //_player->WaitDiscontinuity();
    //IMediaDataProvider::SeekData seekData;
    //seekData.time = time;
    //_dataProvider->Seek(seekData);
}

void HostPlaybackController::SetVideoStream(int index)
{
    if (_seeking)
    {
        _bufferedSeekData.videoStreamIndex = index;
        _bufferedSeekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
        return;
    }

    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.defaultTime = 1;
    seekData.videoStreamIndex = index;
    seekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(seekData), _GetUserIds(), 1);
    _Seek(seekData);

    //_StartSeeking();
    //_timerController.AddStop("loading");
    //_timerController.RemoveStop("finished");
    //_loading = true;
    //_finished = false;
    //IMediaDataProvider::SeekData seekData;
    //seekData.time = _player->TimerPosition();
    //seekData.videoStreamIndex = index;
    //IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    //_player->SetVideoStream(std::move(seekResult.videoStream));
    //_player->WaitDiscontinuity();
    //_currentVideoStream = index;
}

void HostPlaybackController::SetAudioStream(int index)
{
    if (_seeking)
    {
        _bufferedSeekData.audioStreamIndex = index;
        _bufferedSeekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
        return;
    }

    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.defaultTime = 1;
    seekData.audioStreamIndex = index;
    seekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(seekData), _GetUserIds(), 1);
    _Seek(seekData);

    //_StartSeeking();
    //_timerController.AddStop("loading");
    //_timerController.RemoveStop("finished");
    //_loading = true;
    //_finished = false;
    //IMediaDataProvider::SeekData seekData;
    //seekData.time = _player->TimerPosition();
    //seekData.audioStreamIndex = index;
    //IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    //_player->SetAudioStream(std::move(seekResult.audioStream));
    //_player->WaitDiscontinuity();
    //_currentAudioStream = index;
}

void HostPlaybackController::SetSubtitleStream(int index)
{
    if (_seeking)
    {
        _bufferedSeekData.subtitleStreamIndex = index;
        _bufferedSeekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
        return;
    }

    IMediaDataProvider::SeekData seekData;
    seekData.time = _player->TimerPosition();
    seekData.defaultTime = 1;
    seekData.subtitleStreamIndex = index;
    seekData.userId = znet::NetworkInterface::Instance()->ThisUser().id;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(seekData), _GetUserIds(), 1);
    _Seek(seekData);

    //_StartSeeking();
    //_timerController.AddStop("loading");
    //_timerController.RemoveStop("finished");
    //_loading = true;
    //_finished = false;
    //IMediaDataProvider::SeekData seekData;
    //seekData.time = _player->TimerPosition();
    //seekData.subtitleStreamIndex = index;
    //IMediaDataProvider::SeekResult seekResult = _dataProvider->Seek(seekData);
    //_player->SetSubtitleStream(std::move(seekResult.subtitleStream));
    //_player->WaitDiscontinuity();
    //_currentSubtitleStream = index;
}

void HostPlaybackController::_Seek(IMediaDataProvider::SeekData seekData)
{
    _StartSeeking();
    _timerController.AddStop("loading");
    _timerController.RemoveStop("finished");
    _loading = true;
    _finished = false;

    if (seekData.time.GetTicks() == -1)
        seekData.time = _player->TimerPosition();

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
    _player->SetTimerPosition(seekData.time);
    _player->WaitDiscontinuity();
}

IPlaybackController::LoadingInfo HostPlaybackController::Loading() const
{
    return { _timerController.Stopped(), L"" };
    //if (_waiting)
    //    return { true, L"Synchronizing" };
    //if (_player->Lagging())
    //    return { true, L"" };
    //return { !_CanPlay(), L"" };
}

void HostPlaybackController::_StartSeeking()
{
    _timerController.ClearTimers();
    _timerController.ClearPlayTimers();
    _timerController.AddStop("seeking");
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

std::wstring HostPlaybackController::_UsernameFromId(int64_t id)
{
    std::wstringstream username(L"");
    if (id == 0)
        username << L"[Owner] ";
    else
        username << L"[User " << id << "] ";
    auto users = znet::NetworkInterface::Instance()->Users();
    for (auto& user : users)
    {
        if (user.id == id)
        {
            username << user.name;
            break;
        }
    }
    return username.str();
}