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
        TimePoint currentTime;
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

    IMediaDataProvider::SeekData _bufferedSeekData;
    bool _seeking = false;

public:
    HostPlaybackController(MediaPlayer* player, MediaHostDataProvider* dataProvider);

    void Update();
private: // Packet handlers
    void _CheckForPlayRequest();
    void _CheckForPauseRequest();
    void _CheckForSeekRequest();
    void _CheckForSeekFinished();

public:
    void Play();
    void Pause();
private:
    void _Play();
public:
    void Seek(TimePoint time);
    void SetVideoStream(int index);
    void SetAudioStream(int index);
    void SetSubtitleStream(int index);
private:
    void _StartSeeking();
};