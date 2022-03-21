#pragma once

#include "BasePlaybackController.h"
#include "MediaReceiverDataProvider.h"
#include "NetworkInterfaceNew.h"

class ReceiverPlaybackController : public BasePlaybackController
{
protected:
    int64_t _hostId;
    bool _hostReady;

    std::unique_ptr<znet::PacketReceiver> _hostControllerReadyReceiver;
    std::unique_ptr<znet::PacketReceiver> _playReceiver;
    std::unique_ptr<znet::PacketReceiver> _pauseReceiver;
    std::unique_ptr<znet::PacketReceiver> _initiateSeekReceiver;
    std::unique_ptr<znet::PacketReceiver> _hostSeekFinishedReceiver;
    std::unique_ptr<znet::PacketReceiver> _syncPauseReceiver;

    TimePoint _lastPositionNotification = 0;

    bool _waitingForSeek = false;
    bool _waitingForResume = false;
    bool _buffering = false;

public:
    ReceiverPlaybackController(MediaPlayer* player, MediaReceiverDataProvider* dataProvider);

    void Update();
private: // Packet handlers
    void _CheckForHostControllerReady();
    void _CheckForPlay();
    void _CheckForPause();
    void _CheckForInitiateSeek();
    void _CheckForHostSeekFinished();
    void _CheckForSyncPause();

public:
    void Play();
    void Pause();
private:
    void _Play();
    void _Pause();
    bool _CanPlay() const;
public:
    void Seek(TimePoint time);
    void SetVideoStream(int index);
    void SetAudioStream(int index);
    void SetSubtitleStream(int index);
public:
    LoadingInfo Loading() const;
};