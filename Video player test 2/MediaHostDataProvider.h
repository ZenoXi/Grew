#pragma once

#include "LocalFileDataProvider.h"

class MediaHostDataProvider : public IMediaDataProvider
{
    std::thread _initializationThread;
    std::thread _packetReadingThread;
    bool _INIT_THREAD_STOP = false;
    bool _PACKET_THREAD_STOP = false;

    LocalFileDataProvider* _localDataProvider = nullptr;
    bool _waitForDiscontinuity = false;
    SeekData _seekData;
    std::mutex _m_seek;

    std::vector<int64_t> _destinationUsers;

public:
    MediaHostDataProvider(LocalFileDataProvider* localDataProvider);
    ~MediaHostDataProvider();
private:
    void _Initialize();
public:
    void Start();
    void Stop();

protected:
    void _Seek(SeekData seekData);
    void _Seek(TimePoint time);
    void _SetVideoStream(int index, TimePoint time);
    void _SetAudioStream(int index, TimePoint time);
    void _SetSubtitleStream(int index, TimePoint time);
private:
    void _ReadPackets();
    void _ManageNetwork();

    // Host specific
public:
    std::vector<int64_t> GetDestinationUsers();
};