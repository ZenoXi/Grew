#include "MediaHostDataProvider.h"

#include "Network.h"
#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"

#include <iostream>

MediaHostDataProvider::MediaHostDataProvider(std::unique_ptr<LocalFileDataProvider> localDataProvider, std::vector<int64_t> participants)
    : IMediaDataProvider(localDataProvider.get())
{
    _videoMemoryPacketReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::VIDEO_MEMORY_LIMIT);
    _audioMemoryPacketReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::AUDIO_MEMORY_LIMIT);
    _subtitleMemoryPacketReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::SUBTITLE_MEMORY_LIMIT);
    // Update manually with input from clients
    AutoUpdateMemoryFromSettings(false);

    _localDataProvider = std::move(localDataProvider);
    _destinationUsers = participants;

    _initializing = true;
    _INIT_THREAD_STOP = false;
    _initializationThread = std::thread(&MediaHostDataProvider::_Initialize, this);
}

MediaHostDataProvider::~MediaHostDataProvider()
{
    Stop();
}

void MediaHostDataProvider::Start()
{
    _PACKET_THREAD_STOP = false;
    _packetReadingThread = std::thread(&MediaHostDataProvider::_ReadPackets, this);
}

void MediaHostDataProvider::Stop()
{
    _INIT_THREAD_STOP = true;
    _PACKET_THREAD_STOP = true;

    APP_NETWORK->AbortSend((int32_t)znet::PacketType::VIDEO_PACKET);
    APP_NETWORK->AbortSend((int32_t)znet::PacketType::AUDIO_PACKET);
    APP_NETWORK->AbortSend((int32_t)znet::PacketType::SUBTITLE_PACKET);

    if (_initializationThread.joinable())
        _initializationThread.join();
    if (_packetReadingThread.joinable())
        _packetReadingThread.join();
}

void MediaHostDataProvider::_Seek(SeekData seekData)
{
    std::unique_lock<std::mutex> lock(_m_seek);
    _waitForDiscontinuity = true;
    _seekData = seekData;
    lock.unlock();
    _localDataProvider->Seek(seekData);
}

void MediaHostDataProvider::_Seek(TimePoint time)
{
    //_localDataProvider->Seek(time);
}

void MediaHostDataProvider::_SetVideoStream(int index, TimePoint time)
{
    //_localDataProvider->SetVideoStream(index, time);
}

void MediaHostDataProvider::_SetAudioStream(int index, TimePoint time)
{
    //_localDataProvider->SetAudioStream(index, time);
}

void MediaHostDataProvider::_SetSubtitleStream(int index, TimePoint time)
{
    //_localDataProvider->SetSubtitleStream(index, time);
}

void MediaHostDataProvider::_Initialize()
{
    std::cout << "Init started.." << std::endl;

    // Set small memory limits, since this class is unable to directly manage
    // packet reading in local file data provider
    _localDataProvider->SetAllowedVideoMemory(10000000); // 10 MB
    _localDataProvider->SetAllowedAudioMemory(5000000); // 5 MB
    _localDataProvider->SetAllowedSubtitleMemory(1000000); // 1 MB
    _localDataProvider->AutoUpdateMemoryFromSettings(false);
    _localDataProvider->Start();

    // Create metadata confirmation receiver
    class MetadataTracker : public znet::PacketSubscriber
    {
        std::vector<int64_t> _users;

        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            for (int i = 0; i < _users.size(); i++)
            {
                if (_users[i] == userId)
                {
                    _users.erase(_users.begin() + i);
                }
            }
        }
    public:
        MetadataTracker(std::vector<int64_t> users)
          : PacketSubscriber((int32_t)znet::PacketType::METADATA_CONFIRMATION),
            _users(users)
        {}

        bool AllReceived()
        {
            return _users.empty();
        }
    } metadataTracker(_destinationUsers);

    // Create controller ready confirmation receiver
    class ControllerTracker : public znet::PacketSubscriber
    {
        std::vector<int64_t> _users;

        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            for (int i = 0; i < _users.size(); i++)
            {
                if (_users[i] == userId)
                {
                    _users.erase(_users.begin() + i);
                }
            }
        }
    public:
        ControllerTracker(std::vector<int64_t> users)
            : PacketSubscriber((int32_t)znet::PacketType::CONTROLLER_READY),
            _users(users)
        {}

        bool AllReceived()
        {
            return _users.empty();
        }
    } controllerTracker(_destinationUsers);

    // Create stream metadata struct
    struct StreamMetadata
    {
        int8_t videoStreamCount;
        int8_t audioStreamCount;
        int8_t subtitleStreamCount;
        int8_t attachmentStreamCount;
        int8_t dataStreamCount;
        int8_t unknownStreamCount;
        int8_t currentVideoStream;
        int8_t currentAudioStream;
        int8_t currentSubtitleStream;
        int8_t chapterCount;
    } streamMetadata
    {
        _videoData.streams.size(),
        _audioData.streams.size(),
        _subtitleData.streams.size(),
        _attachmentStreams.size(),
        _dataStreams.size(),
        _unknownStreams.size(),
        _videoData.currentStream,
        _audioData.currentStream,
        _subtitleData.currentStream,
        _chapters.size()
    };

    // Send stream metadata
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::STREAM_METADATA).From(streamMetadata), _destinationUsers);

    // Send streams
    for (int i = 0; i < _videoData.streams.size(); i++)
    {
        auto serializedData = _videoData.streams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::VIDEO_STREAM);
        APP_NETWORK->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _audioData.streams.size(); i++)
    {
        auto serializedData = _audioData.streams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::AUDIO_STREAM);
        APP_NETWORK->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _subtitleData.streams.size(); i++)
    {
        auto serializedData = _subtitleData.streams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::SUBTITLE_STREAM);
        APP_NETWORK->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _attachmentStreams.size(); i++)
    {
        auto serializedData = _attachmentStreams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::ATTACHMENT_STREAM);
        APP_NETWORK->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _dataStreams.size(); i++)
    {
        auto serializedData = _dataStreams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::DATA_STREAM);
        APP_NETWORK->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _unknownStreams.size(); i++)
    {
        auto serializedData = _unknownStreams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::UNKNOWN_STREAM);
        APP_NETWORK->Send(std::move(streamPacket), _destinationUsers);
    }
    // Send chapters
    for (int i = 0; i < _chapters.size(); i++)
    {
        auto serializedData = _chapters[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet chapterPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::CHAPTER);
        APP_NETWORK->Send(std::move(chapterPacket), _destinationUsers);
    }

    // Wait for confirmation
    while (!metadataTracker.AllReceived() && !_INIT_THREAD_STOP)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "Metadata receive confirmed!" << std::endl;

    _initializing = false;

    std::cout << "Init success!" << std::endl;
}

void MediaHostDataProvider::_ReadPackets()
{
    using namespace znet;

    MediaPacket videoPacket;
    MediaPacket audioPacket;
    MediaPacket subtitlePacket;

    TimePoint lastMemoryUpdate = -1;
    Duration memoryUpdateInterval = Duration(2, SECONDS);
    Clock threadClock = Clock(0);

    while (!_PACKET_THREAD_STOP)
    {
        threadClock.Update();

        // Periodically update memory limits in case options change
        if (threadClock.Now() >= lastMemoryUpdate + memoryUpdateInterval)
        {
            lastMemoryUpdate = threadClock.Now();
            _UpdateSelfMemoryLimit();
        }

        _CheckForVideoMemoryPackets();
        _CheckForAudioMemoryPackets();
        _CheckForSubtitleMemoryPackets();

        std::unique_lock<std::mutex> lock(_m_seek);
        if (_waitForDiscontinuity)
        {
            // Wait until all packet types are flush
            if (!_localDataProvider->FlushVideoPacketNext())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
                continue;
            }
            if (!_localDataProvider->FlushAudioPacketNext())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
                continue;
            }
            if (!_localDataProvider->FlushSubtitlePacketNext())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
                continue;
            }

            videoPacket = _localDataProvider->GetVideoPacket();
            audioPacket = _localDataProvider->GetAudioPacket();
            subtitlePacket = _localDataProvider->GetSubtitlePacket();
            _ClearVideoPackets();
            _ClearAudioPackets();
            _ClearSubtitlePackets();

            // Clear outgoing packets
            APP_NETWORK->AbortSend((int32_t)znet::PacketType::VIDEO_PACKET);
            APP_NETWORK->AbortSend((int32_t)znet::PacketType::AUDIO_PACKET);
            APP_NETWORK->AbortSend((int32_t)znet::PacketType::SUBTITLE_PACKET);

            // Send seek order
            //znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(_seekData), { _destinationUsers });
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::SEEK_DISCONTINUITY)/*.From(_seekData)*/, { _destinationUsers });
            std::cout << "Seek order sent" << std::endl;

            _waitForDiscontinuity = false;
        }
        lock.unlock();

        // Read packets
        if (!videoPacket.Valid() && !videoPacket.flush)
            videoPacket = _localDataProvider->GetVideoPacket();
        if (!audioPacket.Valid() && !audioPacket.flush)
            audioPacket = _localDataProvider->GetAudioPacket();
        if (!subtitlePacket.Valid() && !subtitlePacket.flush)
            subtitlePacket = _localDataProvider->GetSubtitlePacket();


        // Pass packets to receivers and data provider
        bool packetPassed = false;
        if (videoPacket.Valid() || videoPacket.flush || videoPacket.last)
        {
            if (!VideoMemoryExceeded())
            {
                packetPassed = true;

                // Send packet to all receivers
                auto data = videoPacket.Serialize();
                auto bytes = std::make_unique<int8_t[]>(data.Size());
                std::copy_n(data.Bytes(), data.Size(), bytes.get());
                APP_NETWORK->Send(Packet(std::move(bytes), data.Size(), (int)PacketType::VIDEO_PACKET), _destinationUsers);

                // Add packet to local playback
                _AddVideoPacket(std::move(videoPacket));
            }
        }

        if (audioPacket.Valid() || audioPacket.flush || audioPacket.last)
        {
            if (!AudioMemoryExceeded())
            {
                packetPassed = true;

                // Send packet to all receivers
                auto data = audioPacket.Serialize();
                auto bytes = std::make_unique<int8_t[]>(data.Size());
                std::copy_n(data.Bytes(), data.Size(), bytes.get());
                APP_NETWORK->Send(Packet(std::move(bytes), data.Size(), (int)PacketType::AUDIO_PACKET), _destinationUsers);

                // Add packet to local playback
                _AddAudioPacket(std::move(audioPacket));
            }
        }

        if (subtitlePacket.Valid() || subtitlePacket.flush || subtitlePacket.last)
        {
            if (!SubtitleMemoryExceeded())
            {
                packetPassed = true;

                // Send packet to all receivers
                auto data = subtitlePacket.Serialize();
                auto bytes = std::make_unique<int8_t[]>(data.Size());
                std::copy_n(data.Bytes(), data.Size(), bytes.get());
                APP_NETWORK->Send(Packet(std::move(bytes), data.Size(), (int)PacketType::SUBTITLE_PACKET), _destinationUsers);

                // Add packet to local playback
                _AddSubtitlePacket(std::move(subtitlePacket));
            }
        }

        if (!packetPassed)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void MediaHostDataProvider::_ManageNetwork()
{

}

void MediaHostDataProvider::_CheckForVideoMemoryPackets()
{
    if (!_videoMemoryPacketReceiver)
        return;

    if (_videoMemoryPacketReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _videoMemoryPacketReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        if (packet.size == sizeof(size_t))
        {
            size_t bytes = packet.Cast<size_t>();
            _videoMemoryLimits[userId] = bytes;
            _UpdateSelfMemoryLimit();
        }
    }
}

void MediaHostDataProvider::_CheckForAudioMemoryPackets()
{
    if (!_audioMemoryPacketReceiver)
        return;

    if (_audioMemoryPacketReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _audioMemoryPacketReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        if (packet.size == sizeof(size_t))
        {
            size_t bytes = packet.Cast<size_t>();
            _audioMemoryLimits[userId] = bytes;
            _UpdateSelfMemoryLimit();
        }
    }
}

void MediaHostDataProvider::_CheckForSubtitleMemoryPackets()
{
    if (!_subtitleMemoryPacketReceiver)
        return;

    if (_subtitleMemoryPacketReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _subtitleMemoryPacketReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        if (packet.size == sizeof(size_t))
        {
            size_t bytes = packet.Cast<size_t>();
            _subtitleMemoryLimits[userId] = bytes;
            _UpdateSelfMemoryLimit();
        }
    }
}

void MediaHostDataProvider::_UpdateSelfMemoryLimit()
{
    size_t MIN_VIDEO_MEMORY = 100000000; // 100 mb
    size_t MIN_AUDIO_MEMORY = 10000000; // 10 mb
    size_t MIN_SUBTITLE_MEMORY = 1000000; // 1 mb

    // Calculate allowed video memory
    size_t allowedVideoMemory = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_MAX_VIDEO_MEMORY), 250).Value() * 1000000;
    for (auto& pair : _videoMemoryLimits)
    {
        if (pair.second < allowedVideoMemory)
        {
            allowedVideoMemory = pair.second;
        }
    }
    if (allowedVideoMemory < MIN_VIDEO_MEMORY)
        allowedVideoMemory = MIN_VIDEO_MEMORY;

    // Calculate allowed audio memory
    size_t allowedAudioMemory = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_MAX_AUDIO_MEMORY), 10).Value() * 1000000;
    for (auto& pair : _audioMemoryLimits)
    {
        if (pair.second < allowedAudioMemory)
        {
            allowedVideoMemory = pair.second;
        }
    }
    if (allowedAudioMemory < MIN_AUDIO_MEMORY)
        allowedAudioMemory = MIN_AUDIO_MEMORY;

    // Calculate allowed subtitle memory
    size_t allowedSubtitleMemory = IntOptionAdapter(Options::Instance()->GetValue(OPTIONS_MAX_SUBTITLE_MEMORY), 1).Value() * 1000000;
    for (auto& pair : _subtitleMemoryLimits)
    {
        if (pair.second < allowedSubtitleMemory)
        {
            allowedSubtitleMemory = pair.second;
        }
    }
    if (allowedSubtitleMemory < MIN_SUBTITLE_MEMORY)
        allowedSubtitleMemory = MIN_SUBTITLE_MEMORY;

    SetAllowedVideoMemory(allowedVideoMemory);
    SetAllowedAudioMemory(allowedAudioMemory);
    SetAllowedSubtitleMemory(allowedSubtitleMemory);
}

std::vector<int64_t> MediaHostDataProvider::GetDestinationUsers()
{
    // TODO: needs thread safety here
    return _destinationUsers;
}