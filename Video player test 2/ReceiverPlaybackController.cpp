#include "ReceiverPlaybackController.h"

ReceiverPlaybackController::ReceiverPlaybackController(MediaPlayer* player, MediaReceiverDataProvider* dataProvider)
    : BasePlaybackController(player, dataProvider)
{
    _hostId = dataProvider->GetHostId();
    _hostReady = false;
    _hostControllerReadyReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::HOST_CONTROLLER_READY);
    _playReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::RESUME);
    _pauseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PAUSE);
    _initiateSeekReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::INITIATE_SEEK);
    _hostSeekFinishedReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::HOST_SEEK_FINISHED);

    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::CONTROLLER_READY), { _hostId });
}

void ReceiverPlaybackController::Update()
{
    // Process packets
    _CheckForHostControllerReady();
    _CheckForPlay();
    _CheckForPause();
    _CheckForInitiateSeek();
    _CheckForHostSeekFinished();



    if (_loading && _hostReady)
    {
        if (_player->Recovered())
        {
            std::cout << "Loading finished!" << std::endl;
            _loading = false;
            _waitingForResume = true;

            // Notify host of seek finish
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::SEEK_FINISHED), { _hostId }, 1);

            //if (!_paused)
            //{
            //    _player->StartTimer();
            //}
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

void ReceiverPlaybackController::_CheckForHostControllerReady()
{
    if (!_hostControllerReadyReceiver)
        return;

    while (_hostControllerReadyReceiver->PacketCount() > 0)
    {
        auto packetPair = _hostControllerReadyReceiver->GetPacket();
        _hostReady = true;
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
    }
}

void ReceiverPlaybackController::_CheckForPause()
{
    if (!_pauseReceiver)
        return;

    while (_pauseReceiver->PacketCount() > 0)
    {
        ztime::clock[3].Update();
        TimePoint now = ztime::clock[3].Now();
        std::cout << "[HostPlaybackController] " << now.GetTime(SECONDS) << "." << now.GetTime(MICROSECONDS) % 1000000 << std::endl << std::endl;

        auto packetPair = _pauseReceiver->GetPacket();
        BasePlaybackController::Pause();
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

        _loading = true;
        _finished = false;
        _waitingForSeek = false;
        _waitingForResume = false;
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
        _waitingForResume = false;
        if (!_paused)
        {
            _Play();
        }
    }
}

void ReceiverPlaybackController::Play()
{
    if (!_hostReady)
        return;

    _Play();
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::RESUME_REQUEST), { _hostId }, 1);
}

void ReceiverPlaybackController::_Play()
{
    _paused = false;
    if (_loading || _waitingForSeek || _waitingForResume)
        return;
    _player->StartTimer();
}

void ReceiverPlaybackController::Pause()
{
    if (!_hostReady)
        return;

    BasePlaybackController::Pause();
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PAUSE_REQUEST), { _hostId }, 1);
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
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _waitingForSeek = true;
    _player->StopTimer();
    _player->SetTimerPosition(time);
}

void ReceiverPlaybackController::SetVideoStream(int index)
{
    if (!_hostReady)
        return;

    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.videoStreamIndex = index;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _currentVideoStream = index;
    _waitingForSeek = true;
    _player->StopTimer();
}

void ReceiverPlaybackController::SetAudioStream(int index)
{
    if (!_hostReady)
        return;

    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.audioStreamIndex = index;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _currentAudioStream = index;
    _waitingForSeek = true;
    _player->StopTimer();
}

void ReceiverPlaybackController::SetSubtitleStream(int index)
{
    if (!_hostReady)
        return;

    // Send request to host
    IMediaDataProvider::SeekData seekData;
    seekData.subtitleStreamIndex = index;
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::SEEK_REQUEST).From(seekData), { _hostId }, 1);

    _currentSubtitleStream = index;
    _waitingForSeek = true;
    _player->StopTimer();
}