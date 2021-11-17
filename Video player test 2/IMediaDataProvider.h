#pragma once

#include "GameTime.h"
#include "MediaPacket.h"
#include "MediaStream.h"

#include <vector>
#include <thread>
#include <mutex>

struct MediaData
{
    // Stream data
    std::vector<MediaStream> streams;
    int currentStream = -1;

    // Packet data
    std::vector<MediaPacket> packets;
    TimePoint lastPts = TimePoint::Min();
    TimePoint lastDts = TimePoint::Min();
    int currentPacket = -1;
    std::mutex mtx;
};

class IMediaDataProvider
{
protected:
    bool _initializing = false;
    bool _initFailed = false;
    bool _started = false;
    bool _loading = false;

    //std::vector<MediaStream> _videoStreams;
    //std::vector<MediaStream> _audioStreams;
    //std::vector<MediaStream> _subtitleStreams;
    //int _currentVideoStream;
    //int _currentAudioStream;
    //int _currentSubtitleStream;

protected:
    MediaData _videoData;
    MediaData _audioData;
    MediaData _subtitleData;

    //std::vector<MediaPacket> _videoPackets;
    //std::vector<MediaPacket> _audioPackets;
    //std::vector<MediaPacket> _subtitlePackets;
    //int _currentVideoPacket;
    //int _currentAudioPacket;
    //int _currentSubtitlePacket;
    //std::mutex _m_videoPackets;
    //std::mutex _m_audioPackets;
    //std::mutex _m_subtitlePackets;

public:
    IMediaDataProvider() {};
    virtual ~IMediaDataProvider() {};

public:
    virtual void Start() = 0;


    // STATUS
public:
    bool Initializing();
    bool InitFailed();
    // Seeking/changing stream
    bool Loading();
    Duration BufferedDuration();
private:
    Duration _BufferedDuration(MediaData& packetData);


    // METADATA
public:
    Duration MediaDuration();


    // STREAM CONTROL
public:
    void Seek(TimePoint time);
    // Returns a MediaStream object if a stream at the index exists, nullptr otherwise.
    // Valid index can be acquired from GetAvailableVideoStreams().
    // index of -1 selects the default stream.
    std::unique_ptr<MediaStream> SetVideoStream(int index = -1);
    // Returns a MediaStream object if a stream at the index exists, nullptr otherwise.
    // Valid index can be acquired from GetAvailableAudioStreams().
    // index of -1 selects the default stream.
    std::unique_ptr<MediaStream> SetAudioStream(int index = -1);
    // Returns a MediaStream object if a stream at the index exists, nullptr otherwise.
    // Valid index can be acquired from GetAvailableSubtitleStreams().
    // index of -1 selects the default stream.
    std::unique_ptr<MediaStream> SetSubtitleStream(int index = -1);
private:
    std::unique_ptr<MediaStream> _SetStream(MediaData& mediaData, int index = -1);
protected:
    virtual void _Seek(TimePoint time) = 0;
    virtual void _SetVideoStream(int index) = 0;
    virtual void _SetAudioStream(int index) = 0;
    virtual void _SetSubtitleStream(int index) = 0;


    // MEDIA DATA
public:
    std::vector<std::string> GetAvailableVideoStreams();
    std::vector<std::string> GetAvailableAudioStreams();
    std::vector<std::string> GetAvailableSubtitleStreams();
private:
    std::vector<std::string> _GetAvailableStreams(MediaData& mediaData);


    // PACKET OUTPUT
public:
    size_t VideoPacketCount() const;
    size_t AudioPacketCount() const;
    size_t SubtitlePacketCount() const;
    MediaPacket GetVideoPacket();
    MediaPacket GetAudioPacket();
    MediaPacket GetSubtitlePacket();
private:
    size_t _PacketCount(const MediaData& mediaData) const;
    MediaPacket _GetPacket(MediaData& mediaData);


    // PACKET INPUT
protected:
    void _AddVideoPacket(MediaPacket packet);
    void _AddAudioPacket(MediaPacket packet);
    void _AddSubtitlePacket(MediaPacket packet);
private:
    void _AddPacket(MediaData& mediaData, MediaPacket packet);

};