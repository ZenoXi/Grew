#include "MediaHostDataProvider.h"

#include "NetworkInterfaceNew.h"

#include <iostream>

MediaHostDataProvider::MediaHostDataProvider(LocalFileDataProvider* localDataProvider)
    : IMediaDataProvider(localDataProvider)
{
    _localDataProvider = localDataProvider;
    _localDataProvider->Start();

    _initializing = true;
    _INIT_THREAD_STOP = false;
    _initializationThread = std::thread(&MediaHostDataProvider::_Initialize, this);
}

MediaHostDataProvider::~MediaHostDataProvider()
{
    Stop();
    delete _localDataProvider;
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

    znet::NetworkInterface::Instance()->AbortSend((int32_t)znet::PacketType::VIDEO_PACKET);
    znet::NetworkInterface::Instance()->AbortSend((int32_t)znet::PacketType::AUDIO_PACKET);
    znet::NetworkInterface::Instance()->AbortSend((int32_t)znet::PacketType::SUBTITLE_PACKET);

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

    // Create a list of currently connected users
    auto users = znet::NetworkInterface::Instance()->Users();
    for (auto& user : users)
        _destinationUsers.push_back(user.id);

    // Create playback confirmation receiver
    class PlaybackTracker : public znet::PacketSubscriber
    {
        std::vector<int64_t> _users;
        std::vector<int64_t> _accepted;

        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            for (int i = 0; i < _users.size(); i++)
            {
                if (_users[i] == userId)
                {
                    if (packet.Cast<int8_t>() == 0)
                        _accepted.push_back(_users[i]);
                    _users.erase(_users.begin() + i);
                }
            }
        }
    public:
        PlaybackTracker(std::vector<int64_t> users)
            : PacketSubscriber((int32_t)znet::PacketType::PLAYBACK_CONFIRMATION),
            _users(users)
        {}

        bool AllReceived()
        {
            return _users.empty();
        }

        std::vector<int64_t> Accepted()
        {
            return _accepted;
        }
    } playbackTracker(_destinationUsers);

    // Send playback start notification
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_RESPONSE).From(int8_t(1)), { 0 });

    // Wait for users to confirm/deny the playback request
    while (!playbackTracker.AllReceived() && !_INIT_THREAD_STOP)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    _destinationUsers = playbackTracker.Accepted();
    std::cout << "Participation established!" << std::endl;

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

    // Send participating user list
    size_t userListSize = sizeof(int64_t) * (_destinationUsers.size() + 1);
    auto userListBytes = std::make_unique<int8_t[]>(userListSize);
    ((int64_t*)userListBytes.get())[0] = znet::NetworkInterface::Instance()->ThisUser().id;
    for (int i = 0; i < _destinationUsers.size(); i++)
        ((int64_t*)userListBytes.get())[i + 1] = _destinationUsers[i];
    znet::NetworkInterface::Instance()->Send(znet::Packet(std::move(userListBytes), userListSize, (int)znet::PacketType::PLAYBACK_PARTICIPANT_LIST), _destinationUsers);

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
        _subtitleData.currentStream
    };

    // Send stream metadata
    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::STREAM_METADATA).From(streamMetadata), _destinationUsers);

    // Send streams
    for (int i = 0; i < _videoData.streams.size(); i++)
    {
        auto serializedData = _videoData.streams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::VIDEO_STREAM);
        znet::NetworkInterface::Instance()->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _audioData.streams.size(); i++)
    {
        auto serializedData = _audioData.streams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::AUDIO_STREAM);
        znet::NetworkInterface::Instance()->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _subtitleData.streams.size(); i++)
    {
        auto serializedData = _subtitleData.streams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::SUBTITLE_STREAM);
        znet::NetworkInterface::Instance()->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _attachmentStreams.size(); i++)
    {
        auto serializedData = _attachmentStreams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::ATTACHMENT_STREAM);
        znet::NetworkInterface::Instance()->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _dataStreams.size(); i++)
    {
        auto serializedData = _dataStreams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::DATA_STREAM);
        znet::NetworkInterface::Instance()->Send(std::move(streamPacket), _destinationUsers);
    }
    for (int i = 0; i < _unknownStreams.size(); i++)
    {
        auto serializedData = _unknownStreams[i].Serialize();
        auto bytes = std::make_unique<int8_t[]>(serializedData.Size());
        std::copy_n(serializedData.Bytes(), serializedData.Size(), bytes.get());
        znet::Packet streamPacket(std::move(bytes), serializedData.Size(), (int)znet::PacketType::UNKNOWN_STREAM);
        znet::NetworkInterface::Instance()->Send(std::move(streamPacket), _destinationUsers);
    }

    // Wait for confirmation
    while (!metadataTracker.AllReceived() && !_INIT_THREAD_STOP)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "Metadata receive confirmed!" << std::endl;

    // Wait for all controllers to be ready
    while (!controllerTracker.AllReceived() && !_INIT_THREAD_STOP)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "Controllers ready!" << std::endl;

    _initializing = false;

    std::cout << "Init success!" << std::endl;
}

void MediaHostDataProvider::_ReadPackets()
{
    using namespace znet;

    MediaPacket videoPacket;
    MediaPacket audioPacket;
    MediaPacket subtitlePacket;

    while (!_PACKET_THREAD_STOP)
    {
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
            znet::NetworkInterface::Instance()->AbortSend((int32_t)znet::PacketType::VIDEO_PACKET);
            znet::NetworkInterface::Instance()->AbortSend((int32_t)znet::PacketType::AUDIO_PACKET);
            znet::NetworkInterface::Instance()->AbortSend((int32_t)znet::PacketType::SUBTITLE_PACKET);

            // Send seek order
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::INITIATE_SEEK).From(_seekData), { _destinationUsers });
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
        if (videoPacket.Valid() || videoPacket.flush)
        {
            if (!VideoMemoryExceeded())
            {
                packetPassed = true;

                // Send packet to all receivers
                auto data = videoPacket.Serialize();
                auto bytes = std::make_unique<int8_t[]>(data.Size());
                std::copy_n(data.Bytes(), data.Size(), bytes.get());
                NetworkInterface::Instance()->Send(Packet(std::move(bytes), data.Size(), (int)PacketType::VIDEO_PACKET), _destinationUsers);

                // Add packet to local playback
                _AddVideoPacket(std::move(videoPacket));
            }
        }

        if (audioPacket.Valid() || audioPacket.flush)
        {
            if (!AudioMemoryExceeded())
            {
                packetPassed = true;

                // Send packet to all receivers
                auto data = audioPacket.Serialize();
                auto bytes = std::make_unique<int8_t[]>(data.Size());
                std::copy_n(data.Bytes(), data.Size(), bytes.get());
                NetworkInterface::Instance()->Send(Packet(std::move(bytes), data.Size(), (int)PacketType::AUDIO_PACKET), _destinationUsers);

                // Add packet to local playback
                _AddAudioPacket(std::move(audioPacket));
            }
        }

        if (subtitlePacket.Valid() || subtitlePacket.flush)
        {
            if (!SubtitleMemoryExceeded())
            {
                packetPassed = true;

                // Send packet to all receivers
                auto data = subtitlePacket.Serialize();
                auto bytes = std::make_unique<int8_t[]>(data.Size());
                std::copy_n(data.Bytes(), data.Size(), bytes.get());
                NetworkInterface::Instance()->Send(Packet(std::move(bytes), data.Size(), (int)PacketType::SUBTITLE_PACKET), _destinationUsers);

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

std::vector<int64_t> MediaHostDataProvider::GetDestinationUsers()
{
    // TODO: needs thread safety here
    return _destinationUsers;
}