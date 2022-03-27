#pragma once

#include "GameTime.h"
#include "MediaPacket.h"
#include "MediaStream.h"

#include <vector>
#include <thread>
#include <mutex>

class IMediaDataProvider
{
public:
    struct MediaData
    {
        // Stream data
        std::vector<MediaStream> streams;
        int currentStream = -1;

        // Packet data
        std::vector<MediaPacket> packets;
        TimePoint lastPts = TimePoint::Min();
        TimePoint lastDts = TimePoint::Min();
        int currentPacket = 0;
        size_t totalMemoryUsed = 0;
        size_t allowedMemory = 100000000;
        std::mutex mtx;
    };

    struct SeekData
    {
        TimePoint time = -1;
        int8_t defaultTime = 0;
        int videoStreamIndex = std::numeric_limits<int>::min();
        int audioStreamIndex = std::numeric_limits<int>::min();
        int subtitleStreamIndex = std::numeric_limits<int>::min();
        int64_t userId = -1;

        SeekData() = default;
        SeekData(const SeekData&) = default;
        SeekData& operator=(const SeekData&) = default;
        SeekData(SeekData&&) = default;
        SeekData& operator=(SeekData&&) = default;
        bool Default()
        {
            return time.GetTicks() == -1
                && videoStreamIndex == std::numeric_limits<int>::min()
                && audioStreamIndex == std::numeric_limits<int>::min()
                && subtitleStreamIndex == std::numeric_limits<int>::min();
        }
    };
    
    struct SeekResult
    {
        std::unique_ptr<MediaStream> videoStream = nullptr;
        std::unique_ptr<MediaStream> audioStream = nullptr;
        std::unique_ptr<MediaStream> subtitleStream = nullptr;
    };

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
    std::vector<MediaStream> _attachmentStreams;
    std::vector<MediaStream> _dataStreams;
    std::vector<MediaStream> _unknownStreams;


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
    IMediaDataProvider();
    IMediaDataProvider(IMediaDataProvider* other);
    virtual ~IMediaDataProvider() {};

public:
    virtual void Start() = 0;
    virtual void Stop() = 0;


    // STATUS
public:
    bool Initializing();
    bool InitFailed();
    // Seeking/changing stream
    bool Loading();
    Duration BufferedDuration(bool ignoreSubtitles = true);
private:
    Duration _BufferedDuration(MediaData& packetData);


    // METADATA
public:
    Duration MediaDuration();
    Duration MaxMediaDuration();


    // MEMORY MANAGEMENT
public:
    void SetAllowedVideoMemory(size_t bytes);
    void SetAllowedAudioMemory(size_t bytes);
    void SetAllowedSubtitleMemory(size_t bytes);
    bool VideoMemoryExceeded() const;
    bool AudioMemoryExceeded() const;
    bool SubtitleMemoryExceeded() const;
private:
    void _SetAllowedMemory(MediaData& mediaData, size_t bytes);
    bool _MemoryExceeded(const MediaData& mediaData) const;


    // STREAM CONTROL
public:
    SeekResult Seek(SeekData seekData);
    void Seek(TimePoint time);
    // Returns a MediaStream object if a stream at the index exists, nullptr otherwise.
    // Valid index can be acquired from GetAvailableVideoStreams().
    // index of -1 selects the default stream.
    // time: where to continue playback after stream change.
    std::unique_ptr<MediaStream> SetVideoStream(int index = -1, TimePoint time = 0);
    // Returns a MediaStream object if a stream at the index exists, nullptr otherwise.
    // Valid index can be acquired from GetAvailableAudioStreams().
    // index of -1 selects the default stream.
    // time: where to continue playback after stream change.
    std::unique_ptr<MediaStream> SetAudioStream(int index = -1, TimePoint time = 0);
    // Returns a MediaStream object if a stream at the index exists, nullptr otherwise.
    // Valid index can be acquired from GetAvailableSubtitleStreams().
    // index of -1 selects the default stream.
    // time: where to continue playback after stream change.
    std::unique_ptr<MediaStream> SetSubtitleStream(int index = -1, TimePoint time = 0);
private:
    std::unique_ptr<MediaStream> _SetStream(MediaData& mediaData, int index);
public:
    std::unique_ptr<MediaStream> CurrentVideoStream();
    std::unique_ptr<MediaStream> CurrentAudioStream();
    std::unique_ptr<MediaStream> CurrentSubtitleStream();
    std::unique_ptr<MediaStream> CurrentVideoStreamView();
    std::unique_ptr<MediaStream> CurrentAudioStreamView();
    std::unique_ptr<MediaStream> CurrentSubtitleStreamView();
    int CurrentVideoStreamIndex() const;
    int CurrentAudioStreamIndex() const;
    int CurrentSubtitleStreamIndex() const;
protected:
    std::unique_ptr<MediaStream> _CurrentStream(MediaData& mediaData);
    std::unique_ptr<MediaStream> _CurrentStreamView(MediaData& mediaData);
    int _CurrentStreamIndex(const MediaData& mediaData) const;
protected:
    virtual void _Seek(SeekData seekData) = 0;
    virtual void _Seek(TimePoint time) = 0;
    virtual void _SetVideoStream(int index, TimePoint time) = 0;
    virtual void _SetAudioStream(int index, TimePoint time) = 0;
    virtual void _SetSubtitleStream(int index, TimePoint time) = 0;


    // MEDIA DATA
public:
    std::vector<std::string> GetAvailableVideoStreams();
    std::vector<std::string> GetAvailableAudioStreams();
    std::vector<std::string> GetAvailableSubtitleStreams();
private:
    std::vector<std::string> _GetAvailableStreams(MediaData& mediaData);
public:
    std::vector<MediaStream> GetFontStreams();


    // PACKET OUTPUT
public:
    size_t VideoPacketCount() const;
    size_t AudioPacketCount() const;
    size_t SubtitlePacketCount() const;
    MediaPacket GetVideoPacket();
    MediaPacket GetAudioPacket();
    MediaPacket GetSubtitlePacket();
    bool FlushVideoPacketNext();
    bool FlushAudioPacketNext();
    bool FlushSubtitlePacketNext();
protected:
    size_t _PacketCount(const MediaData& mediaData) const;
    MediaPacket _GetPacket(MediaData& mediaData);
    bool _FlushPacketNext(MediaData& mediaData);


    // PACKET MANIPULATION
protected:
    void _AddVideoPacket(MediaPacket packet);
    void _AddAudioPacket(MediaPacket packet);
    void _AddSubtitlePacket(MediaPacket packet);
    // A stub for now.
    void _AddFlushPackets();
    void _ClearVideoPackets();
    void _ClearAudioPackets();
    void _ClearSubtitlePackets();
    void _AddPacket(MediaData& mediaData, MediaPacket packet);
    void _ClearPackets(MediaData& mediaData);

};