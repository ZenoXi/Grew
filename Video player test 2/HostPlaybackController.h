#pragma once

#include "BasePlaybackController.h"
#include "MediaHostDataProvider.h"
#include "NetworkInterfaceNew.h"

class HostPlaybackController : public BasePlaybackController
{
protected:
    struct _UserData
    {
        int64_t id;
        TimePoint currentTime; // User playback position
        TimePoint receiveTime; // Time at which 'currentTime' was received
        Duration timeOffset; // How far ahead the user is
        bool seekCompleted;
    };
    //struct _SeekData

    std::vector<_UserData> _destinationUsers;
    std::vector<int64_t> _GetUserIds()
    {
        std::vector<int64_t> ids;
        for (auto& user : _destinationUsers)
            ids.push_back(user.id);
        return ids;
    }

    std::unique_ptr<znet::PacketReceiver> _playRequestReceiver;
    std::unique_ptr<znet::PacketReceiver> _pauseRequestReceiver;
    std::unique_ptr<znet::PacketReceiver> _seekRequestReceiver;
    std::unique_ptr<znet::PacketReceiver> _seekFinishedReceiver;
    std::unique_ptr<znet::PacketReceiver> _playbackPositionReceiver;
    std::unique_ptr<znet::PacketReceiver> _syncPauseReceiver;

    IMediaDataProvider::SeekData _bufferedSeekData;
    bool _seeking = false;
    TimePoint _lastSeek = 0;
    TimePoint _lastSync = 0;

    bool _buffering = false;

public:
    HostPlaybackController(IMediaDataProvider* dataProvider, std::vector<int64_t> participants);

private:
    void _OnMediaPlayerAttach();
public:

    void Update();
private: // Packet handlers
    void _CheckForPlayRequest();
    void _CheckForPauseRequest();
    void _CheckForSeekRequest();
    void _CheckForSeekFinished();
    void _CheckForPlaybackPosition();
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
private:
    void _Seek(IMediaDataProvider::SeekData seekData);
public:
    LoadingInfo Loading() const;
private:
    void _StartSeeking();
    void _SyncPlayback();
    std::wstring _UsernameFromId(int64_t id);
};