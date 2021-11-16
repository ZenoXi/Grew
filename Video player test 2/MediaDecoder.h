#pragma once

//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

#include <vector>
#include <string>
#include <deque>
#include <memory>
#include <mutex>
#include <fstream>
#include "ChiliWin.h"

#include "GameTime.h"
//#include "DisplayWindow.h"
#include "ThreadController.h"
#include "PlaybackMode.h"

#include "FFmpegSerializer.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;

/// <summary>
/// Specifies where the packets are sourced from:
/// - LOCAL: AVFormatContext reads from a local file
/// - REMOTE: Packets are passed through AddVideoPacket/AddAudioPacket
/// </summary>
enum class PacketSource
{
    LOCAL,
    REMOTE
};

struct PixelData
{
    uchar R, G, B;
};

class FrameData
{
    uchar* _bytes;
    int _width;
    int _height;
    long long int _timestamp;

public:
    FrameData(int width, int height, long long int timestamp);
    ~FrameData();
    FrameData(const FrameData& fd);
    FrameData& operator=(const FrameData& fd);
    FrameData(FrameData&& fd) noexcept;
    FrameData& operator=(FrameData&& fd) noexcept;

    const uchar* GetBytes() const;
    void SetBytes(const uchar* bytes);
    void ClearBytes();
    PixelData GetPixel(int x, int y) const;
    void SetPixel(int x, int y, PixelData pixel);
    int GetWidth() const;
    int GetHeight() const;
    long long int GetTimestamp() const;
};

class AudioData
{
public:
    struct WaveHeader
    {
        DWORD chunkID;       // 0x46464952 "RIFF" in little endian
        DWORD chunkSize;     // 4 + (8 + subChunk1Size) + (8 + subChunk2Size)
        DWORD format;        // 0x45564157 "WAVE" in little endian

        DWORD subChunk1ID;   // 0x20746d66 "fmt " in little endian
        DWORD subChunk1Size; // 16 for PCM
        WORD  audioFormat;   // 1 for PCM, 3 fot EEE floating point , 7 for -law
        WORD  numChannels;   // 1 for mono, 2 for stereo
        DWORD sampleRate;    // 8000, 22050, 44100, etc...
        DWORD byteRate;      // sampleRate * numChannels * bitsPerSample/8
        WORD  blockAlign;    // numChannels * bitsPerSample/8
        WORD  bitsPerSample; // number of bits (8 for 8 bits, etc...)

        DWORD subChunk2ID;   // 0x61746164 "data" in little endian
        DWORD subChunk2Size; // numSamples * numChannels * bitsPerSample/8 (this is the actual data size in bytes)
    };

private:
    char* _bytes;
    int _samples;
    int _channels;
    int _sampleRate;
    long long int _timestamp;

public:
    AudioData(int sampleCount, int channelCount, int sampleRate, long long int timestamp);
    ~AudioData();
    AudioData(const AudioData& fd);
    AudioData& operator=(const AudioData& fd);
    AudioData(AudioData&& fd) noexcept;
    AudioData& operator=(AudioData&& fd) noexcept;

    const char* GetBytes() const;
    char* GetBytesWithHeader() const;
    WaveHeader GetHeader() const;
    void SetBytes(const char* bytes);
    void ClearBytes();
    int GetSampleCount() const;
    int GetChannelCount() const;
    int GetSampleRate() const;
    long long int GetTimestamp() const;
};

struct VideoInfo
{
    int width;
    int height;
    std::pair<int, int> framerate;
};

struct AudioInfo
{
    int channels;
    int sampleRate;
    int bitsPerSample;
    int samplesPerFrame;
};

struct PlayerSettings
{
    int videoQueueSize = 1024; // In megabytes
    int audioQueueSize = 2; // In megabytes
};

struct MediaStreamOld
{
    AVStream* stream;
    int index;
    std::string name;
    int packetCount;
    int64_t startTime;
    int64_t duration;
    AVRational timeBase;
};

struct VideoStream : MediaStreamOld
{
    int width;
    int height;
};

struct AudioStream : MediaStreamOld
{
    int channels;
    int sampleRate;
};

struct SubtitleStream : MediaStreamOld
{

};

class MediaDecoder
{
    PlaybackMode _mode;
    PacketSource _sourceType;

    int _MAX_VIDEO_QUEUE_SIZE;
    int _MAX_AUDIO_QUEUE_SIZE;
    std::deque<FrameData> _videoFrames;
    std::deque<AudioData> _audioFrames;
    std::vector<FFmpeg::AVPacketEx> _videoPackets;
    std::vector<FFmpeg::AVPacketEx> _audioPackets;
    std::deque<FFmpeg::Result> _videoPacketData;
    std::deque<FFmpeg::Result> _audioPacketData;
    TimePoint _videoPacketBufferEnd = 0;
    TimePoint _audioPacketBufferEnd = 0;
    TimePoint _videoFrameBufferEnd = 0;
    TimePoint _audioFrameBufferEnd = 0;

    AVFormatContext* _avf_context = nullptr;
    AVCodecParameters* _avc_params_video = nullptr;
    AVCodec* _av_codec_video = nullptr;
    AVCodecParameters* _avc_params_audio = nullptr;
    AVCodec* _av_codec_audio = nullptr;
    AVCodecParameters* _avc_params_subs = nullptr;
    AVCodec* _av_codec_subs = nullptr;
    int _video_stream_index;
    int _audio_stream_index;
    int _subs_stream_index;
    std::vector<VideoStream> _videoStreams;
    std::vector<AudioStream> _audioStreams;
    std::vector<SubtitleStream> _subtitleStreams;
    int _currentVideoStream;
    int _currentAudioStream;
    int _currentSubtitleStream;

    VideoInfo _videoInfo;
    AudioInfo _audioInfo;

    Duration _duration;

    bool _processing;
    bool _initResult;
public:

    MediaDecoder(std::string filename, bool online = false, PlayerSettings settings = PlayerSettings());
    MediaDecoder(uchar* videoCodecContextData, uchar* audioCodecContextData, PlayerSettings settings = PlayerSettings());
    ~MediaDecoder();
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;
    MediaDecoder(MediaDecoder&&) = delete;
    MediaDecoder& operator=(MediaDecoder&&) = delete;
    bool InitSuccessful();
    bool Buffering();
    bool Flushing();

    bool StartDecoding();
    void StopDecoding();
    void ClearDecoder();
    TimePoint SeekTo(TimePoint timePoint, bool forward = true);
    FrameData GetVideoFrameData();
    AudioData GetAudioFrameData();
    VideoInfo GetVideoInfo();
    AudioInfo GetAudioInfo();
    Duration GetDuration();
    Duration GetPacketBufferedDuration();
    Duration GetFrameBufferedDuration();
    //std::pair<int, int> GetFramerateRatio();


private: // Online mode variables
    AVRational _videoTimebase;
    AVRational _audioTimebase;
    bool _buffering; // true when a client is waiting for enough video/audio packets. DOES NOT ALIGN WITH THE RETURN VALUE OF Buffering()

public:
    FFmpeg::Result GetVideoCodecParamData();
    FFmpeg::Result GetAudioCodecParamData();
    FFmpeg::Result GetVideoPacketData();
    FFmpeg::Result GetAudioPacketData();
    AVRational GetVideoTimebase();
    AVRational GetAudioTimebase();
    void SetMetadata(Duration duration, AVRational framerate, AVRational videoTimebase, AVRational audioTimebase);
    void AddVideoPacket(uchar* data);
    void AddAudioPacket(uchar* data);

private: // Packet decoder threads
    ThreadController _packetThreadController;
    ThreadController _videoThreadController;
    ThreadController _audioThreadController;
    std::thread _packetExtractorThread;
    std::thread _videoDecoderThread;
    std::thread _audioDecoderThread;
    std::mutex _m_video_packets;
    std::mutex _m_audio_packets;
    std::mutex _m_video_frames;
    std::mutex _m_audio_frames;
    std::mutex _m_video_packet_data;
    std::mutex _m_audio_packet_data;

    void ClearVideoPackets();
    void ClearAudioPackets();
    void PacketExtractor();
    void VideoPacketDecoder();
    void AudioPacketDecoder();

    // File processing thread
    std::thread _fileProcessingThread;

    void ProcessFile(std::string filename);
    void ExtractStreams();
    void FindMissingStreamData();
    // Decode the first few (all if 'full' == true) of the packets to calculate/extract missing data
    void CalculateMissingStreamData(bool full = false);
};