#include "MediaReceiverDataProvider.h"

#include <iostream>

MediaReceiverDataProvider::MediaReceiverDataProvider(int64_t hostId)
    : IMediaDataProvider(),
    _videoPacketReceiver(znet::PacketType::VIDEO_PACKET),
    _audioPacketReceiver(znet::PacketType::AUDIO_PACKET),
    _subtitlePacketReceiver(znet::PacketType::SUBTITLE_PACKET)
{
    _initiateSeekReceiver = std::make_unique<InitiateSeekReceiver>([&]()
    {
        std::lock_guard<std::mutex> lock(_m_seek);
        _videoPacketReceiver.Clear();
        _audioPacketReceiver.Clear();
        _subtitlePacketReceiver.Clear();
        _waitingForSeek = true;
    });

    _hostId = hostId;
    _initializing = true;
    _INIT_THREAD_STOP = false;
    _initializationThread = std::thread(&MediaReceiverDataProvider::_Initialize, this);
}

MediaReceiverDataProvider::~MediaReceiverDataProvider()
{
    Stop();
}

void MediaReceiverDataProvider::Start()
{
    _PACKET_THREAD_STOP = false;
    _packetReadingThread = std::thread(&MediaReceiverDataProvider::_ReadPackets, this);
}

void MediaReceiverDataProvider::Stop()
{
    _INIT_THREAD_STOP = true;
    if (_initializationThread.joinable())
        _initializationThread.join();

    _PACKET_THREAD_STOP = true;
    if (_packetReadingThread.joinable())
        _packetReadingThread.join();
}

void MediaReceiverDataProvider::_Seek(SeekData seekData)
{
    _seekReceived = true;
}

void MediaReceiverDataProvider::_Seek(TimePoint time)
{

}

void MediaReceiverDataProvider::_SetVideoStream(int index, TimePoint time)
{

}

void MediaReceiverDataProvider::_SetAudioStream(int index, TimePoint time)
{

}

void MediaReceiverDataProvider::_SetSubtitleStream(int index, TimePoint time)
{

}

void MediaReceiverDataProvider::_Initialize()
{
    using namespace znet;
    std::cout << "Init started.." << std::endl;

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
    } streamMetadata;
    std::vector<int64_t> userList;

    // Set up user list receiver
    class UserListReceiver : public PacketSubscriber
    {
        std::vector<int64_t>& _userIds;
        int64_t _hostId;
        void _OnPacketReceived(Packet packet, int64_t userId)
        {
            if (userId != _hostId)
                return;

            int64_t thisId = znet::NetworkInterface::Instance()->ThisUser().id;
            for (int i = 0; i < packet.size / sizeof(int64_t); i++)
            {
                int64_t user = ((int64_t*)packet.Bytes())[i];
                if (user != thisId)
                    _userIds.push_back(user);
            }

            received = true;
        }
    public:
        UserListReceiver(std::vector<int64_t>& userIds, int64_t hostId)
            : PacketSubscriber((int32_t)PacketType::PLAYBACK_PARTICIPANT_LIST),
            _userIds(userIds),
            _hostId(hostId)
        {}
        bool received = false;
    };
    //UserListReceiver userListReceiver(userList, _hostId);

    // Set up metadata receiver
    class StreamMetadataReceiver : public PacketSubscriber
    {
        StreamMetadata& _metadata;
        int64_t _hostId;
        void _OnPacketReceived(Packet packet, int64_t userId)
        {
            if (userId != _hostId)
                return;

            _metadata = packet.Cast<StreamMetadata>();
            received = true;
        }
    public:
        StreamMetadataReceiver(StreamMetadata& metadata, int64_t hostId)
          : PacketSubscriber((int32_t)PacketType::STREAM_METADATA),
            _metadata(metadata),
            _hostId(hostId)
        {}
        bool received = false;
    };
    StreamMetadataReceiver metadataReceiver(streamMetadata, _hostId);

    // Set up stream receivers
    class StreamReceiver : public PacketSubscriber
    {
        int64_t _hostId;
        std::vector<MediaStream> _streams;
        std::mutex _m_streams;
        void _OnPacketReceived(Packet packet, int64_t userId)
        {
            if (userId != _hostId)
                return;

            MediaStream stream;
            auto bytes = std::make_unique<uchar[]>(packet.size);
            std::copy_n(packet.Bytes(), packet.size, bytes.get());
            SerializedData data(packet.size, std::move(bytes));
            stream.Deserialize(std::move(data));

            std::lock_guard<std::mutex> lock(_m_streams);
            _streams.push_back(std::move(stream));
        }
    public:
        StreamReceiver(PacketType streamType, int64_t hostId)
            : PacketSubscriber((int32_t)streamType),
            _hostId(hostId)
        {}
        std::vector<MediaStream>&& MoveStreams()
        {
            std::lock_guard<std::mutex> lock(_m_streams);
            return std::move(_streams);
        }
        size_t StreamCount()
        {
            std::lock_guard<std::mutex> lock(_m_streams);
            return _streams.size();
        }
    };
    StreamReceiver videoStreamReceiver(PacketType::VIDEO_STREAM, _hostId);
    StreamReceiver audioStreamReceiver(PacketType::AUDIO_STREAM, _hostId);
    StreamReceiver subtitleStreamReceiver(PacketType::SUBTITLE_STREAM, _hostId);
    StreamReceiver attachmentStreamReceiver(PacketType::ATTACHMENT_STREAM, _hostId);
    StreamReceiver dataStreamReceiver(PacketType::DATA_STREAM, _hostId);
    StreamReceiver unknownStreamReceiver(PacketType::UNKNOWN_STREAM, _hostId);

    // Send ready notification
    NetworkInterface::Instance()->Send(Packet((int)PacketType::PLAYBACK_CONFIRMATION).From(int8_t(0)), { _hostId });

    // Wait for user list
    //while (!userListReceiver.received)
    //    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //_userList = userList;

    // Wait for stream metadata
    while (!metadataReceiver.received && !_INIT_THREAD_STOP)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Wait for all streams
    while (!_INIT_THREAD_STOP)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (videoStreamReceiver.StreamCount()      < streamMetadata.videoStreamCount) continue;
        if (audioStreamReceiver.StreamCount()      < streamMetadata.audioStreamCount) continue;
        if (subtitleStreamReceiver.StreamCount()   < streamMetadata.subtitleStreamCount) continue;
        if (attachmentStreamReceiver.StreamCount() < streamMetadata.attachmentStreamCount) continue;
        if (dataStreamReceiver.StreamCount()       < streamMetadata.dataStreamCount) continue;
        if (unknownStreamReceiver.StreamCount()    < streamMetadata.unknownStreamCount) continue;
        break;
    }
    _videoData.streams    = videoStreamReceiver.MoveStreams();
    _audioData.streams    = audioStreamReceiver.MoveStreams();
    _subtitleData.streams = subtitleStreamReceiver.MoveStreams();
    _attachmentStreams    = attachmentStreamReceiver.MoveStreams();
    _dataStreams          = dataStreamReceiver.MoveStreams();
    _unknownStreams       = unknownStreamReceiver.MoveStreams();
    _videoData.currentStream = streamMetadata.currentVideoStream;
    _audioData.currentStream = streamMetadata.currentAudioStream;
    _subtitleData.currentStream = streamMetadata.currentSubtitleStream;

    // Send receive confirmation
    NetworkInterface::Instance()->Send(Packet((int)PacketType::METADATA_CONFIRMATION), { _hostId });

    _initializing = false;

    std::cout << "Init success!" << std::endl;
}

void MediaReceiverDataProvider::_ReadPackets()
{
    while (!_PACKET_THREAD_STOP)
    {
        if (_seekReceived && _waitingForSeek)// && _initiateSeekReceiver->SeekDataReceived())
        {
            _waitingForSeek = false;
            _seekReceived = false;
            _initiateSeekReceiver->Reset();

            _ClearVideoPackets();
            _ClearAudioPackets();
            _ClearSubtitlePackets();

            std::cout << "Packets cleared" << std::endl;

            while (!_bufferedVideoPackets.empty())
            {
                _AddVideoPacket(std::move(_bufferedVideoPackets.front()));
                _bufferedVideoPackets.pop();
            }
            while (!_bufferedAudioPackets.empty())
            {
                _AddAudioPacket(std::move(_bufferedAudioPackets.front()));
                _bufferedAudioPackets.pop();
            }
            while (!_bufferedSubtitlePackets.empty())
            {
                _AddSubtitlePacket(std::move(_bufferedSubtitlePackets.front()));
                _bufferedSubtitlePackets.pop();
            }
        }

        std::unique_lock<std::mutex> lock(_m_seek);
        if (_waitingForSeek)
        {
            if (_videoPacketReceiver.PacketCount() > 0)
                _bufferedVideoPackets.push(_videoPacketReceiver.GetPacket());
            if (_audioPacketReceiver.PacketCount() > 0)
                _bufferedAudioPackets.push(_audioPacketReceiver.GetPacket());
            if (_subtitlePacketReceiver.PacketCount() > 0)
                _bufferedSubtitlePackets.push(_subtitlePacketReceiver.GetPacket());
        }
        else
        {
            if (_videoPacketReceiver.PacketCount() > 0)
                _AddVideoPacket(_videoPacketReceiver.GetPacket());
            if (_audioPacketReceiver.PacketCount() > 0)
                _AddAudioPacket(_audioPacketReceiver.GetPacket());
            if (_subtitlePacketReceiver.PacketCount() > 0)
                _AddSubtitlePacket(_subtitlePacketReceiver.GetPacket());
        }
        lock.unlock();


        if (_videoPacketReceiver.PacketCount() == 0
            && _audioPacketReceiver.PacketCount() == 0
            && _subtitlePacketReceiver.PacketCount() == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void MediaReceiverDataProvider::_ManageNetwork()
{

}

int64_t MediaReceiverDataProvider::GetHostId()
{
    return _hostId;
}