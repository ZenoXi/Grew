#pragma once

#include "LocalFileDataProvider.h"
#include "PacketSubscriber.h"

#include <unordered_map>

class MediaHostDataProvider : public IMediaDataProvider
{
    std::thread _initializationThread;
    std::thread _packetReadingThread;
    bool _INIT_THREAD_STOP = false;
    bool _PACKET_THREAD_STOP = false;

    std::unique_ptr<LocalFileDataProvider> _localDataProvider = nullptr;
    bool _waitForDiscontinuity = false;
    SeekData _seekData;
    std::mutex _m_seek;

    std::vector<int64_t> _destinationUsers;

    std::unique_ptr<znet::PacketReceiver> _videoMemoryPacketReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _audioMemoryPacketReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _subtitleMemoryPacketReceiver = nullptr;
    std::unordered_map<int64_t, size_t> _videoMemoryLimits;
    std::unordered_map<int64_t, size_t> _audioMemoryLimits;
    std::unordered_map<int64_t, size_t> _subtitleMemoryLimits;

public:
    MediaHostDataProvider(std::unique_ptr<LocalFileDataProvider> localDataProvider, std::vector<int64_t> participants);
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

    void _CheckForVideoMemoryPackets();
    void _CheckForAudioMemoryPackets();
    void _CheckForSubtitleMemoryPackets();
    void _UpdateSelfMemoryLimit();

    // Host specific
public:
    std::vector<int64_t> GetDestinationUsers();
};