#pragma once

#include "IMediaDataProvider.h"
#include "Network.h"

class MediaReceiverDataProvider : public IMediaDataProvider
{
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
    };

    class StreamMetadataReceiver : public znet::PacketSubscriber
    {
        StreamMetadata& _metadata;
        int64_t _hostId;
        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            if (userId != _hostId)
                return;

            _metadata = packet.Cast<StreamMetadata>();
            received = true;
        }
    public:
        StreamMetadataReceiver(StreamMetadata& metadata, int64_t hostId)
            : PacketSubscriber((int32_t)znet::PacketType::STREAM_METADATA),
            _metadata(metadata),
            _hostId(hostId)
        {}
        bool received = false;
    };

    class StreamReceiver : public znet::PacketSubscriber
    {
        int64_t _hostId;
        std::vector<MediaStream> _streams;
        std::mutex _m_streams;
        void _OnPacketReceived(znet::Packet packet, int64_t userId)
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
        StreamReceiver(znet::PacketType streamType, int64_t hostId)
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

    class ChapterReceiver : public znet::PacketSubscriber
    {
        int64_t _hostId;
        std::vector<MediaChapter> _chapters;
        std::mutex _m_chapters;
        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            if (userId != _hostId)
                return;

            MediaChapter chapter;
            auto bytes = std::make_unique<uchar[]>(packet.size);
            std::copy_n(packet.Bytes(), packet.size, bytes.get());
            SerializedData data(packet.size, std::move(bytes));
            chapter.Deserialize(std::move(data));

            std::lock_guard<std::mutex> lock(_m_chapters);
            _chapters.push_back(chapter);
        }
    public:
        ChapterReceiver(znet::PacketType streamType, int64_t hostId)
            : PacketSubscriber((int32_t)streamType),
            _hostId(hostId)
        {}
        std::vector<MediaChapter> GetChapters()
        {
            std::lock_guard<std::mutex> lock(_m_chapters);
            return _chapters;
        }
        size_t ChapterCount()
        {
            std::lock_guard<std::mutex> lock(_m_chapters);
            return _chapters.size();
        }
    };

    class PacketReceiver : public znet::PacketSubscriber
    {
        std::queue<MediaPacket> _packets;
        std::mutex _m_packets;

        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            MediaPacket mediaPacket;
            auto bytes = std::make_unique<uchar[]>(packet.size);
            std::copy_n(packet.Bytes(), packet.size, bytes.get());
            SerializedData data(packet.size, std::move(bytes));
            mediaPacket.Deserialize(std::move(data));

            std::lock_guard<std::mutex> lock(_m_packets);
            _packets.push(std::move(mediaPacket));
        }
    public:
        PacketReceiver(znet::PacketType packetType)
            : PacketSubscriber((int32_t)packetType)
        {

        }
        size_t PacketCount()
        {
            std::lock_guard<std::mutex> lock(_m_packets);
            return _packets.size();
        }
        MediaPacket GetPacket()
        {
            std::lock_guard<std::mutex> lock(_m_packets);
            MediaPacket packet = std::move(_packets.front());
            _packets.pop();
            return packet;
        }
        void Clear()
        {
            std::lock_guard<std::mutex> lock(_m_packets);
            while (!_packets.empty())
                _packets.pop();
        }
    };

    class InitiateSeekReceiver : public znet::PacketSubscriber
    {
        std::function<void()> _onPacket;
        bool _packetReceived = false;

        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            _onPacket();
            _packetReceived = true;
        }
    public:
        InitiateSeekReceiver(std::function<void()> onPacket)
          : PacketSubscriber((int32_t)znet::PacketType::SEEK_DISCONTINUITY),
            _onPacket(onPacket)
        { }
        bool SeekDataReceived()
        {
            return _packetReceived;
        }
        void Reset()
        {
            _packetReceived = false;
        }
    };

    std::thread _initializationThread;
    std::thread _packetReadingThread;
    bool _INIT_THREAD_STOP = false;
    bool _PACKET_THREAD_STOP = false;

    int64_t _hostId = 0;
    StreamMetadata _streamMetadata;
    StreamMetadataReceiver _metadataReceiver;
    StreamReceiver _videoStreamReceiver;
    StreamReceiver _audioStreamReceiver;
    StreamReceiver _subtitleStreamReceiver;
    StreamReceiver _attachmentStreamReceiver;
    StreamReceiver _dataStreamReceiver;
    StreamReceiver _unknownStreamReceiver;
    ChapterReceiver _chapterReceiver;
    PacketReceiver _videoPacketReceiver;
    PacketReceiver _audioPacketReceiver;
    PacketReceiver _subtitlePacketReceiver;
    std::unique_ptr<InitiateSeekReceiver> _initiateSeekReceiver = nullptr;

    std::queue<MediaPacket> _bufferedVideoPackets;
    std::queue<MediaPacket> _bufferedAudioPackets;
    std::queue<MediaPacket> _bufferedSubtitlePackets;
    std::mutex _m_seek;
    bool _seekReceived = false;
    bool _waitingForSeek = false;

public:
    MediaReceiverDataProvider(int64_t hostId);
    ~MediaReceiverDataProvider();
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

    // Receiver specific
public:
    int64_t GetHostId();
};