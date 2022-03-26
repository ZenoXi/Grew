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