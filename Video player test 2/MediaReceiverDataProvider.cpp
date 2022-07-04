#include "MediaReceiverDataProvider.h"

#include <iostream>

MediaReceiverDataProvider::MediaReceiverDataProvider(int64_t hostId)
    : IMediaDataProvider(),
    _metadataReceiver(_streamMetadata, hostId),
    _videoStreamReceiver(znet::PacketType::VIDEO_STREAM, hostId),
    _audioStreamReceiver(znet::PacketType::AUDIO_STREAM, hostId),
    _subtitleStreamReceiver(znet::PacketType::SUBTITLE_STREAM, hostId),
    _attachmentStreamReceiver(znet::PacketType::ATTACHMENT_STREAM, hostId),
    _dataStreamReceiver(znet::PacketType::DATA_STREAM, hostId),
    _unknownStreamReceiver(znet::PacketType::UNKNOWN_STREAM, hostId),
    _chapterReceiver(znet::PacketType::CHAPTER, hostId),
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
    std::cout << "Init started.." << std::endl;

    // Wait for stream metadata
    while (!_metadataReceiver.received && !_INIT_THREAD_STOP)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Wait for all streams
    while (!_INIT_THREAD_STOP)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (_videoStreamReceiver.StreamCount()      < _streamMetadata.videoStreamCount) continue;
        if (_audioStreamReceiver.StreamCount()      < _streamMetadata.audioStreamCount) continue;
        if (_subtitleStreamReceiver.StreamCount()   < _streamMetadata.subtitleStreamCount) continue;
        if (_attachmentStreamReceiver.StreamCount() < _streamMetadata.attachmentStreamCount) continue;
        if (_dataStreamReceiver.StreamCount()       < _streamMetadata.dataStreamCount) continue;
        if (_unknownStreamReceiver.StreamCount()    < _streamMetadata.unknownStreamCount) continue;
        if (_chapterReceiver.ChapterCount()         < _streamMetadata.chapterCount) continue;
        break;
    }
    _videoData.streams    = _videoStreamReceiver.MoveStreams();
    _audioData.streams    = _audioStreamReceiver.MoveStreams();
    _subtitleData.streams = _subtitleStreamReceiver.MoveStreams();
    _attachmentStreams    = _attachmentStreamReceiver.MoveStreams();
    _dataStreams          = _dataStreamReceiver.MoveStreams();
    _unknownStreams       = _unknownStreamReceiver.MoveStreams();
    _videoData.currentStream = _streamMetadata.currentVideoStream;
    _audioData.currentStream = _streamMetadata.currentAudioStream;
    _subtitleData.currentStream = _streamMetadata.currentSubtitleStream;
    _chapters = _chapterReceiver.GetChapters();

    // Send receive confirmation
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::METADATA_CONFIRMATION), { _hostId });

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