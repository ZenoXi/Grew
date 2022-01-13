#pragma once

#include "IMediaDataProvider.h"
#include "NetworkInterfaceNew.h"

class MediaReceiverDataProvider : public IMediaDataProvider
{
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
        PacketReceiver& _videoPacketReceiver;
        PacketReceiver& _audioPacketReceiver;
        PacketReceiver& _subtitlePacketReceiver;
        std::mutex& _m_seek;
        bool& _waitingForSeek;
        bool _packetReceived = false;
        IMediaDataProvider::SeekData _seekData;
        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            std::lock_guard<std::mutex> lock(_m_seek);
            _videoPacketReceiver.Clear();
            _audioPacketReceiver.Clear();
            _subtitlePacketReceiver.Clear();
            _seekData = packet.Cast<IMediaDataProvider::SeekData>();
            _waitingForSeek = true;
            _packetReceived = true;
        }
    public:
        InitiateSeekReceiver(
            PacketReceiver& video,
            PacketReceiver& audio,
            PacketReceiver& subtitle,
            std::mutex& m_seek,
            bool& waitingForSeek
        ) :
            PacketSubscriber((int32_t)znet::PacketType::INITIATE_SEEK),
            _videoPacketReceiver(video),
            _audioPacketReceiver(audio),
            _subtitlePacketReceiver(subtitle),
            _m_seek(m_seek),
            _waitingForSeek(waitingForSeek)
        {

        }
        bool SeekDataReceived()
        {
            return _packetReceived;
        }
        IMediaDataProvider::SeekData SeekData()
        {
            return _seekData;
        }
        void Reset()
        {
            _packetReceived = false;
            _seekData = IMediaDataProvider::SeekData();
        }
    };

    std::thread _initializationThread;
    std::thread _packetReadingThread;
    bool _INIT_THREAD_STOP = false;
    bool _PACKET_THREAD_STOP = false;

    int64_t _hostId = 0;
    std::vector<int64_t> _userList;
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