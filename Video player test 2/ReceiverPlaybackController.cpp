#include "ReceiverPlaybackController.h"

#include "App.h"
#include "PlaybackScene.h"

ReceiverPlaybackController::ReceiverPlaybackController(IMediaDataProvider* dataProvider, int64_t hostId)
    : BasePlaybackController(dataProvider)
{
    _hostId = hostId;
    _hostReady = false;
    _hostControllerReadyReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::HOST_CONTROLLER_READY);
    _playReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::RESUME);
    _pauseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PAUSE);
    _initiateSeekReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::INITIATE_SEEK);
    _hostSeekFinishedReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::HOST_SEEK_FINISHED);
    _syncPauseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::SYNC_PAUSE);
}

void ReceiverPlaybackController::Update()
{
    // Process packets
    _CheckForHostControllerReady();
    _CheckForPlay();
    _CheckForPause();
    _CheckForInitiateSeek();
    _CheckForHostSeekFinished();
    _CheckForSyncPause();

    // Send current playback position
    if ((ztime::Main() - _lastPositionNotification).GetDuration(SECONDS) >= 1)
    {
        APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_POSITION).From(_player->TimerPosition().GetTicks()), { _hostId }, 3);
        _lastPositionNotification = ztime::Main();
    }

    if (_loading && _hostReady)
    {
        if (_player->Recovered())
        {
            std::cout << "Loading finished!" << std::endl;
            _timerController.RemoveStop("loading");
            _timerController.AddStop("waitresume");
            _loading = false;
            _waitingForResume = true;

            // Notify host of seek finish
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::SEEK_FINISHED), { _hostId }, 1);
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
            //_player->StopTimer();
            _finished = true;
            _Pause();
        }
    }

    _timerController.Update();
}

void ReceiverPlaybackController::_CheckForHostControllerReady()
{
    if (!_hostControllerReadyReceiver)
        return;

    while (_hostControllerReadyReceiver->PacketCount() > 0)
    {
        auto packetPair = _hostControllerReadyReceiver->GetPacket();
        _hostReady = true;
        std::cout << "Host ready!\n";
    }
}

void ReceiverPlaybackController::_CheckForPlay()
{
    if (!_playReceiver)
        return;

    while (_playReceiver->PacketCount() > 0)
    {
        auto packetPair = _playReceiver->GetPacket();
        _Play();

        // Show notification
        int64_t userId = packetPair.first.Cast<int64_t>();
        Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        if (scene && userId != APP_NETWORK->ThisUser().id)
        {
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(1, SECONDS);
            ninfo.title = L"Playback resumed";
            ninfo.text = _UsernameFromId(userId);
            scene->ShowNotification(ninfo);
        }
    }
}

void ReceiverPlaybackController::_CheckForPause()
{
    if (!_pauseReceiver)
        return;

    while (_pauseReceiver->PacketCount() > 0)
    {
        auto packetPair = _pauseReceiver->GetPacket();
        _Pause();

        // Show notification
        int64_t userId = packetPair.first.Cast<int64_t>();
        Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        if (scene && userId != APP_NETWORK->ThisUser().id)
        {
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(1, SECONDS);
            ninfo.title = L"Playback paused";
            ninfo.text = _UsernameFromId(userId);
            scene->ShowNotification(ninfo);
        }
    }
}

void ReceiverPlaybackController::_CheckForInitiateSeek()
{
    if (!_initiateSeekReceiver)
        return;

    while (_initiateSeekReceiver->PacketCount() > 0)
    {
        auto packetPair = _initiateSeekReceiver->GetPacket();
        auto seekData = packetPair.first.Cast<IMediaDataProvider::SeekData>();

        std::cout << "Seek order received (t:" << seekData.time.GetTime(MICROSECONDS) / 1000.0f
            << " | v:" << seekData.videoStreamIndex
            << " | a:" << seekData.audioStreamIndex 
            << " | s:" << seekData.subtitleStreamIndex << ")" << std::endl;

        // Seek
        _timerController.ClearTimers();
        _timerController.ClearPlayTimers();
        _timerController.AddStop("loading");
        _timerController.RemoveStop("finished");
        _timerController.RemoveStop("waitseek");
        _timerController.RemoveStop("waitresume");
        _loading = true;
        _finished = false;
        _waitingForSeek = false;
        _waitingForResume = false;
        _player->SetTimerPosition(seekData.time);
        _player->WaitDiscontinuity();

        //Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        //if (scene && seekData.userId != APP_NETWORK->ThisUser().id)
        //{
        //    zcom::NotificationInfo ninfo;
        //    ninfo.duration = Duration(1, SECONDS);
        //    ninfo.title = L"Playback resumed";
        //    ninfo.text = _UsernameFromId(seekData.userId);
        //    scene->ShowNotification(ninfo);
        //}

        //if (seekData.defaultTime == 1)
        //{

        //}

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

        // Show notifications
        Scene* scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
        if (scene && seekData.userId != APP_NETWORK->ThisUser().id)
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
    }
}

void ReceiverPlaybackController::_CheckForHostSeekFinished()
{
    if (!_hostSeekFinishedReceiver)
        return;

    while (_hostSeekFinishedReceiver->PacketCount() > 0)
    {
        auto packetPair = _hostSeekFinishedReceiver->GetPacket();

        std::cout << "Playback resumed" << std::endl;
        _timerController.RemoveStop("waitresume");
        _waitingForResume = false;
    }
}

void ReceiverPlaybackController::_CheckForSyncPause()
{
    if (!_syncPauseReceiver)
        return;

    while (_syncPauseReceiver->PacketCount() > 0)
    {
        auto packetPair = _syncPauseReceiver->GetPacket();

        _timerController.AddPlayTimer(packetPair.first.Cast<int64_t>());
    }
}

void ReceiverPlaybackController::Play()
{
    if (!_hostReady)
        return;

    _Play();
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::RESUME_REQUEST), { _hostId }, 1);
}

void ReceiverPlaybackController::Pause()
{
    if (!_hostReady)
        return;

    _Pause();
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PAUSE_REQUEST), { _hostId }, 1);
}

void ReceiverPlaybackController::_Play()
{
    _timerController.Play();
    _paused = false;
}

void ReceiverPlaybackController::_Pause()
{
    _timerController.Pause();
    _paused = true;
}

bool ReceiverPlaybackController::_CanPlay() const
{
    return !_loading && !_waitingForSeek && !_waitingForResume;
}

void ReceiverPlaybackController::Seek(TimePoint time)
{
    if (!_hostReady)
        return;

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
    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _timerController.AddStop("waitseek");
    _waitingForSeek = true;
    _player->SetTimerPosition(time);
}

void ReceiverPlaybackController::SetVideoStream(int index)
{
    if (!_hostReady)
        return;

    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.videoStreamIndex = index;
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _currentVideoStream = index;
    _timerController.AddStop("waitseek");
    _waitingForSeek = true;
}

void ReceiverPlaybackController::SetAudioStream(int index)
{
    if (!_hostReady)
        return;

    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.audioStreamIndex = index;
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _currentAudioStream = index;
    _timerController.AddStop("waitseek");
    _waitingForSeek = true;
}

void ReceiverPlaybackController::SetSubtitleStream(int index)
{
    if (!_hostReady)
        return;

    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.subtitleStreamIndex = index;
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _currentSubtitleStream = index;
    _timerController.AddStop("waitseek");
    _waitingForSeek = true;
}

IPlaybackController::LoadingInfo ReceiverPlaybackController::Loading() const
{
    return { _timerController.Stopped(), L"" };
    //if (_waiting)
    //    return { true, L"Synchronizing" };
    //if (_player->Lagging())
    //    return { true, L"" };
    //return { !_CanPlay(), L"" };
}

std::wstring ReceiverPlaybackController::_UsernameFromId(int64_t id)
{
    std::wstringstream username(L"");
    if (id == _hostId)
        username << L"[Host] ";
    else if (id == 0)
        username << L"[Owner] ";
    else
        username << L"[User " << id << "] ";
    auto users = APP_NETWORK->Users();
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