#include "MediaDecoder.h"

#include <cmath>
#include <thread>
#include <fstream>
#include <iostream>
#include "Functions.h"

#pragma comment( lib,"xaudio2.lib" )
#include <xaudio2.h>

//#include "FFmpegSerializer.h"

#pragma warning(disable:26451) // Disable arithmetic overflow warning
#undef min
#undef max

int64_t MetadataDurationToMicrosecondsOld(std::string data);

// ////////// //
// FRAME_DATA //
// ////////// //

FrameData::FrameData(int width, int height, long long int timestamp) : _width(width), _height(height), _timestamp(timestamp), _bytes(new uchar[width * height * 4])
{}

FrameData::~FrameData()
{
    delete[] _bytes;
}

FrameData::FrameData(const FrameData& fd)
{
    _width = fd._width;
    _height = fd._height;
    _timestamp = fd._timestamp;
    int size = _width * _height * 4;
    _bytes = new uchar[size];
    std::copy(fd._bytes, fd._bytes + size, _bytes);
}

FrameData& FrameData::operator=(const FrameData& fd)
{
    if (this != &fd)
    {
        delete[] _bytes;

        _width = fd._width;
        _height = fd._height;
        _timestamp = fd._timestamp;
        int size = _width * _height * 4;
        _bytes = new uchar[size];
        std::copy(fd._bytes, fd._bytes + size, _bytes);
    }
    return *this;
}

FrameData::FrameData(FrameData&& fd) noexcept
{
    _width = fd._width;
    _height = fd._height;
    _timestamp = fd._timestamp;
    _bytes = fd._bytes;
    fd._width = 0;
    fd._height = 0;
    fd._timestamp = 0;
    fd._bytes = nullptr;
}

FrameData& FrameData::operator=(FrameData&& fd) noexcept
{
    if (this != &fd)
    {
        delete[] _bytes;

        _width = fd._width;
        _height = fd._height;
        _timestamp = fd._timestamp;
        _bytes = fd._bytes;
        fd._width = 0;
        fd._height = 0;
        fd._timestamp = 0;
        fd._bytes = nullptr;
    }
    return *this;
}

const uchar* FrameData::GetBytes() const
{
    return _bytes;
}

void FrameData::SetBytes(const uchar* bytes)
{
    memcpy(_bytes, bytes, _width * _height * 4);
}

void FrameData::ClearBytes()
{
    memset(_bytes, 0, _width * _height * 4);
}

PixelDataOld FrameData::GetPixel(int x, int y) const
{
    int index = y * _width * 4 + x * 4;
    return
    {
        _bytes[index + 2],
        _bytes[index + 1],
        _bytes[index]
    };
}

void FrameData::SetPixel(int x, int y, PixelDataOld pixel)
{
    int index = y * _width * 4 + x * 4;
    _bytes[index] = pixel.B;
    _bytes[index + 1] = pixel.G;
    _bytes[index + 2] = pixel.R;
}

int FrameData::GetWidth() const
{
    return _width;
}

int FrameData::GetHeight() const
{
    return _height;
}

long long int FrameData::GetTimestamp() const
{
    return _timestamp;
}

// ////////// //
// AUDIO_DATA //
// ////////// //

AudioData::AudioData(int sampleCount, int channelCount, int sampleRate, long long int timestamp)
  : _samples(sampleCount),
    _channels(channelCount),
    _sampleRate(sampleRate),
    _timestamp(timestamp),
    _bytes(new char[sampleCount * channelCount * 2])
{
}

AudioData::~AudioData()
{
    delete[] _bytes;
}

AudioData::AudioData(const AudioData& ad)
{
    _samples = ad._samples;
    _channels = ad._channels;
    _sampleRate = ad._sampleRate;
    _timestamp = ad._timestamp;
    int size = _samples * _channels * 2;
    _bytes = new char[size];
    std::copy(ad._bytes, ad._bytes + size, _bytes);
}

AudioData& AudioData::operator=(const AudioData& ad)
{
    if (this != &ad)
    {
        delete[] _bytes;

        _samples = ad._samples;
        _channels = ad._channels;
        _sampleRate = ad._sampleRate;
        _timestamp = ad._timestamp;
        int size = _samples * _channels * 2;
        _bytes = new char[size];
        std::copy(ad._bytes, ad._bytes + size, _bytes);
    }
    return *this;
}

AudioData::AudioData(AudioData&& ad) noexcept
{
    _samples = ad._samples;
    _channels = ad._channels;
    _sampleRate = ad._sampleRate;
    _timestamp = ad._timestamp;
    _bytes = ad._bytes;
    ad._samples = 0;
    ad._channels = 0;
    ad._sampleRate = 0;
    ad._timestamp = 0;
    ad._bytes = nullptr;
}

AudioData& AudioData::operator=(AudioData&& ad) noexcept
{
    if (this != &ad)
    {
        delete[] _bytes;

        _samples = ad._samples;
        _channels = ad._channels;
        _sampleRate = ad._sampleRate;
        _timestamp = ad._timestamp;
        _bytes = ad._bytes;
        ad._samples = 0;
        ad._channels = 0;
        ad._sampleRate = 0;
        ad._timestamp = 0;
        ad._bytes = nullptr;
    }
    return *this;
}

const char* AudioData::GetBytes() const
{
    return _bytes;
}

char* AudioData::GetBytesWithHeader() const
{
    int dataSize = _samples * _channels * 2;
    char* fullData = new char[dataSize + 44];

    WaveHeader header;
    header.chunkID = 0x46464952;
    header.format = 0x45564157;
    header.subChunk1ID = 0x20746d66;
    header.subChunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = _channels;
    header.sampleRate = _sampleRate;
    header.byteRate = header.sampleRate * header.numChannels * 2;
    header.blockAlign = header.numChannels * 2;
    header.bitsPerSample = 2 * 8;
    header.subChunk2ID = 0x61746164;
    header.subChunk2Size = dataSize;
    header.chunkSize = 4 + (8 + header.subChunk1Size) + (8 + header.subChunk2Size);

    memcpy(fullData, &header, 44);
    memcpy(fullData + 44, _bytes, dataSize);
    return fullData;
}

AudioData::WaveHeader AudioData::GetHeader() const
{
    int dataSize = _samples * _channels * 2;

    WaveHeader header;
    header.chunkID = 0x46464952;
    header.format = 0x45564157;
    header.subChunk1ID = 0x20746d66;
    header.subChunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = _channels;
    header.sampleRate = _sampleRate;
    header.byteRate = header.sampleRate * header.numChannels * 2;
    header.blockAlign = header.numChannels * 2;
    header.bitsPerSample = 2 * 8;
    header.subChunk2ID = 0x61746164;
    header.subChunk2Size = dataSize;
    header.chunkSize = 4 + (8 + header.subChunk1Size) + (8 + header.subChunk2Size);

    return header;
}

void AudioData::SetBytes(const char* bytes)
{
    memcpy(_bytes, bytes, _samples * _channels * 2);
}

void AudioData::ClearBytes()
{
    memset(_bytes, 0, _samples * _channels * 2);
}

int AudioData::GetSampleCount() const
{
    return _samples;
}

int AudioData::GetChannelCount() const
{
    return _channels;
}

int AudioData::GetSampleRate() const
{
    return _sampleRate;
}

long long int AudioData::GetTimestamp() const
{
    return _timestamp;
}

// //////////// //
// MEDIA_PLAYER //
// //////////// //

bool MediaDecoder::InitSuccessful()
{
    return _initResult;
}

bool MediaDecoder::Buffering()
{
    if (_packetThreadController.Get<long long int>("seek") != -1) return true;
    //if (_packetThreadController.Get<bool>("eof"))
    {

    }

    bool videoBufferEnded = false;
    bool audioBufferEnded = false;

    // Video packets
    int64_t videoPts = -1;
    _m_video_packets.lock();
    if (_videoPackets.size() > 0)
    {
        videoPts = _videoPackets.back().packet->pts;
    }
    _m_video_packets.unlock();
    if (videoPts == -1) return true;
    if (_mode == PlaybackMode::CLIENT)
    {
        int64_t usBuffered = av_rescale_q(videoPts, _videoTimebase, { 1, AV_TIME_BASE }) - ztime::Game().GetTime();
        if (!_buffering)
        {
            if (usBuffered < 1000000LL)
            {
                _buffering = true;
                return true;
            }
        }
        else
        {
            if (usBuffered < 5000000LL)
            {
                return true;
            }
            else
            {
                videoBufferEnded = true;
            }
        }
    }
    else
    {
        if (av_rescale_q(videoPts, _avf_context->streams[_video_stream_index]->time_base, { 1, AV_TIME_BASE }) < ztime::Game().GetTime() + 1000000LL)
        {
            return true;
        }
    }

    // Audio packets
    int64_t audioPts = -1;
    _m_audio_packets.lock();
    if (_audioPackets.size())
    {
        audioPts = _audioPackets.back().packet->pts;
    }
    _m_audio_packets.unlock();
    if (audioPts == -1) return true;
    if (_mode == PlaybackMode::CLIENT)
    {
        int64_t usBuffered = av_rescale_q(audioPts, _audioTimebase, { 1, AV_TIME_BASE }) - ztime::Game().GetTime();
        if (!_buffering)
        {
            if (usBuffered < 1000000LL)
            {
                _buffering = true;
                return true;
            }
        }
        else
        {
            if (usBuffered < 5000000LL)
            {
                return true;
            }
            else
            {
                audioBufferEnded = true;
            }
        }
    }
    else
    {
        if (av_rescale_q(audioPts, _avf_context->streams[_audio_stream_index]->time_base, { 1, AV_TIME_BASE }) < ztime::Game().GetTime() + 1000000LL)
        {
            return true;
        }
    }

    if (videoBufferEnded && audioBufferEnded)
    {
        _buffering = false;
    }

    //// Video frames
    //_m_video_frames.lock();
    //if (_videoFrames.size() < _MAX_VIDEO_QUEUE_SIZE / 2 - 1)
    //{
    //    _m_video_frames.unlock();
    //    return true;
    //}
    //_m_video_frames.unlock();

    //// Audio frames
    //_m_audio_frames.lock();
    //if (_audioFrames.size() < _MAX_AUDIO_QUEUE_SIZE / 2 - 1)
    //{
    //    _m_audio_frames.unlock();
    //    return true;
    //}
    //_m_audio_frames.unlock();

    return false;
}

bool MediaDecoder::Flushing()
{
    if (_mode != PlaybackMode::CLIENT)
    {
        if (_packetThreadController.Get<long long int>("seek") != -1LL)
        {
            return true;
        }
    }
    else
    {
        if (_videoThreadController.Get<bool>("flush") || _audioThreadController.Get<bool>("flush"))
        {
            return true;
        }
    }
    return false;
}

MediaDecoder::MediaDecoder(std::string filename, bool online, PlayerSettings settings) : _initResult(false)
{
    if (online)
    {
        _mode = PlaybackMode::SERVER;
    }
    else
    {
        _mode = PlaybackMode::OFFLINE;
    }

    _processing = true;
    _fileProcessingThread = std::thread(&MediaDecoder::ProcessFile, this, filename);
    std::this_thread::sleep_for(std::chrono::seconds(50000));
    return;

    _avf_context = avformat_alloc_context();
    if (!_avf_context) return;
    if (avformat_open_input(&_avf_context, filename.c_str(), NULL, NULL) != 0) return;

    _video_stream_index = -1;
    _audio_stream_index = -1;

    _duration = _avf_context->duration;

    // Find video stream
    for (int i = 0; i < _avf_context->nb_streams; i++)
    {
        AVStream* stream = _avf_context->streams[i];
        AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(stream->metadata, "", t, AV_DICT_IGNORE_SUFFIX)) {
            std::cout << "[" << t->key << "]: \"" << t->value << "\"\n";
        }
        std::cout << std::endl;

        continue;

        //uchar* bytes = FFmpegSerializer::SerializeCodecParams(stream->codecpar).data;
        //_avc_params_video = FFmpegSerializer::DeserializeCodecParams(bytes);
        //av_free(bytes); // <-- Heap corruption
        //delete[] bytes;

        _avc_params_video = stream->codecpar;
        _av_codec_video = avcodec_find_decoder(_avc_params_video->codec_id);
        if (!_av_codec_video)
        {
            continue;
        }

        if (_av_codec_video->type == AVMEDIA_TYPE_VIDEO)
        {
            _videoTimebase = stream->time_base;
            _video_stream_index = i;
            _videoInfo.width = _avc_params_video->width;
            _videoInfo.height = _avc_params_video->height;
            auto framerate = _avf_context->streams[_video_stream_index]->r_frame_rate;
            _videoInfo.framerate = std::pair<int, int>(framerate.num, framerate.den);
            break;
        }
    }

    // Find audio stream
    for (int i = 0; i < _avf_context->nb_streams; i++)
    {
        AVStream* stream = _avf_context->streams[i];
        _avc_params_audio = stream->codecpar;
        _av_codec_audio = avcodec_find_decoder(_avc_params_audio->codec_id);
        if (!_av_codec_audio)
        {
            continue;
        }

        if (_av_codec_audio->type == AVMEDIA_TYPE_AUDIO)
        {
            _audioTimebase = stream->time_base;
            _audio_stream_index = i;
            _audioInfo.channels = _avc_params_audio->channels;
            _audioInfo.sampleRate = _avc_params_audio->sample_rate;
            _audioInfo.bitsPerSample = 16;
            break;
        }
    }

    // Find subtitle stream
    for (int i = 0; i < _avf_context->nb_streams; i++)
    {
        AVStream* stream = _avf_context->streams[i];
        _avc_params_subs = stream->codecpar;
        _av_codec_subs = avcodec_find_decoder(_avc_params_subs->codec_id);
        if (!_av_codec_subs)
        {
            continue;
        }

        if (_av_codec_subs->type == AVMEDIA_TYPE_SUBTITLE)
        {
            _subs_stream_index = i;
            break;
        }
    }

    if (!_av_codec_video && !_av_codec_audio && !_av_codec_subs) return;

    // Calculate queue sizes
    if (_video_stream_index != -1)
    {
        long bytesPerVideoFrame = (long)_avc_params_video->width * (long)_avc_params_video->height * 4L;
        _MAX_VIDEO_QUEUE_SIZE = (long)settings.videoQueueSize * 1000000L / bytesPerVideoFrame;
    }
    if (_audio_stream_index != -1)
    {
        long bytesPerAudioFrame = (long)_avc_params_audio->channels * 1024L * 2L;
        //_MAX_AUDIO_QUEUE_SIZE = (long)settings.audioQueueSize * 1000000L / bytesPerAudioFrame;
        _MAX_AUDIO_QUEUE_SIZE = 5;
    }

    // Set up controllers
    _packetThreadController.Add("seek", sizeof(long long int));
    _packetThreadController.Add("stop", sizeof(bool));
    _packetThreadController.Add("eof", sizeof(bool));
    _packetThreadController.Set("seek", -1LL);
    _packetThreadController.Set("stop", false);
    _packetThreadController.Set("eof", false);

    _videoThreadController.Add("flush", sizeof(bool));
    _videoThreadController.Add("stop", sizeof(bool));
    _videoThreadController.Set("flush", false);
    _videoThreadController.Set("stop", false);

    _audioThreadController.Add("flush", sizeof(bool));
    _audioThreadController.Add("stop", sizeof(bool));
    _audioThreadController.Set("flush", false);
    _audioThreadController.Set("stop", false);

    _initResult = true;
}

MediaDecoder::MediaDecoder(uchar* videoCodecContextData, uchar* audioCodecContextData, PlayerSettings settings)
{
    _mode = PlaybackMode::CLIENT;

    _buffering = true;

    _avc_params_video = FFmpeg::DeserializeCodecParams(videoCodecContextData);
    _avc_params_audio = FFmpeg::DeserializeCodecParams(audioCodecContextData);
    _av_codec_video = avcodec_find_decoder(_avc_params_video->codec_id);
    _av_codec_audio = avcodec_find_decoder(_avc_params_audio->codec_id);

    // Video info
    _videoInfo.width = _avc_params_video->width;
    _videoInfo.height = _avc_params_video->height;

    // Audio info
    _audioInfo.channels = _avc_params_audio->channels;
    _audioInfo.sampleRate = _avc_params_audio->sample_rate;
    _audioInfo.bitsPerSample = 16;

    // Calculate queue sizes
    long bytesPerVideoFrame = (long)_avc_params_video->width * (long)_avc_params_video->height * 4L;
    _MAX_VIDEO_QUEUE_SIZE = (long)settings.videoQueueSize * 1000000L / bytesPerVideoFrame;
    long bytesPerAudioFrame = (long)_avc_params_audio->channels * 1024L * 2L;
    _MAX_AUDIO_QUEUE_SIZE = (long)settings.audioQueueSize * 1000000L / bytesPerAudioFrame;

    // Set up controllers
    _packetThreadController.Add("seek", sizeof(long long int));
    _packetThreadController.Add("stop", sizeof(bool));
    _packetThreadController.Add("eof", sizeof(bool));
    _packetThreadController.Set("seek", -1LL);
    _packetThreadController.Set("stop", false);
    _packetThreadController.Set("eof", false);

    _videoThreadController.Add("flush", sizeof(bool));
    _videoThreadController.Add("stop", sizeof(bool));
    _videoThreadController.Set("flush", false);
    _videoThreadController.Set("stop", false);

    _audioThreadController.Add("flush", sizeof(bool));
    _audioThreadController.Add("stop", sizeof(bool));
    _audioThreadController.Set("flush", false);
    _audioThreadController.Set("stop", false);

    _initResult = true;
}

MediaDecoder::~MediaDecoder()
{
    // Stop decoding threads
    _packetThreadController.Set("stop", true);
    _videoThreadController.Set("stop", true);
    _audioThreadController.Set("stop", true);
    if (_packetExtractorThread.joinable()) _packetExtractorThread.join();
    if (_videoDecoderThread.joinable()) _videoDecoderThread.join();
    if (_audioDecoderThread.joinable()) _audioDecoderThread.join();

    // Dealloc packets
    ClearVideoPackets();
    ClearAudioPackets();

    // Close format context
    avformat_close_input(&_avf_context);
    avformat_free_context(_avf_context);
}

bool MediaDecoder::StartDecoding()
{
    _packetExtractorThread = std::thread(&MediaDecoder::PacketExtractor, this);
    _videoDecoderThread = std::thread(&MediaDecoder::VideoPacketDecoder, this);
    _audioDecoderThread = std::thread(&MediaDecoder::AudioPacketDecoder, this);
    return true;
}

TimePoint MediaDecoder::SeekTo(TimePoint timepoint, bool forward)
{
    _buffering = true;
    if (_mode == PlaybackMode::CLIENT)
    {
        ClearVideoPackets();
        ClearAudioPackets();

        _videoThreadController.Set("flush", true);
        _audioThreadController.Set("flush", true);
        std::cout << "Flush started\n";
    }
    else
    {
        _packetThreadController.Set("seek", timepoint.GetTime());
        std::cout << "Flush started\n";
    }
    return timepoint;


    double seconds = timepoint.GetTime() / 1000000.0;
    if (seconds < 0) seconds = 0.0;
    double videoFrameIndex = seconds * _videoInfo.framerate.first / (double)_videoInfo.framerate.second;
    double audioFrameIndex = seconds * _audioInfo.sampleRate / (double)_audioInfo.samplesPerFrame;
    //_videoThreadController.seek = videoFrameIndex;
    //_audioThreadController.seek = audioFrameIndex;

    // Return first audio frame start timepoint for better synchronization
    double audioFramePosition = fmod(audioFrameIndex, 1.0);
    return (seconds - audioFramePosition * (double)_audioInfo.samplesPerFrame / _audioInfo.sampleRate) * 1000000.0;
}

FrameData MediaDecoder::GetVideoFrameData()
{
    _m_video_frames.lock();
    if (_videoFrames.empty())
    {
        _m_video_frames.unlock();
        return FrameData(0, 0, -1);
    }
    FrameData data = std::move(_videoFrames.front());
    _videoFrames.pop_front();
    _m_video_frames.unlock();

    return data;
}

AudioData MediaDecoder::GetAudioFrameData()
{
    _m_audio_frames.lock();
    if (_audioFrames.empty())
    {
        _m_audio_frames.unlock();
        return AudioData(0, 0, 0, 0);
    }
    AudioData data = std::move(_audioFrames.front());
    _audioFrames.pop_front();
    _m_audio_frames.unlock();

    return data;
}

VideoInfo MediaDecoder::GetVideoInfo()
{
    return _videoInfo;
}

AudioInfo MediaDecoder::GetAudioInfo()
{
    return _audioInfo;
}

Duration MediaDecoder::GetDuration()
{
    return _duration;
}

Duration MediaDecoder::GetPacketBufferedDuration()
{
    // Get timestamp in decoder units
    _m_video_packets.lock();
    int64_t vBufferEndTimestamp = _videoPackets.size() > 0 ? _videoPackets.back().packet->dts : -1;
    _m_video_packets.unlock();
    _m_audio_packets.lock();
    int64_t aBufferEndTimestamp = _audioPackets.size() > 0 ? _audioPackets.back().packet->dts : -1;
    _m_audio_packets.unlock();

    if (vBufferEndTimestamp == -1 || aBufferEndTimestamp == -1) return -1;

    // Convert to microseconds
    int64_t vBufferEnd;
    int64_t aBufferEnd;
    if (_mode == PlaybackMode::CLIENT)
    {
        vBufferEnd = av_rescale_q(vBufferEndTimestamp, _videoTimebase, { 1, AV_TIME_BASE });
        aBufferEnd = av_rescale_q(aBufferEndTimestamp, _audioTimebase, { 1, AV_TIME_BASE });
    }
    else
    {
        vBufferEnd = av_rescale_q(vBufferEndTimestamp, _avf_context->streams[_video_stream_index]->time_base, { 1, AV_TIME_BASE });
        aBufferEnd = av_rescale_q(aBufferEndTimestamp, _avf_context->streams[_audio_stream_index]->time_base, { 1, AV_TIME_BASE });
    }

    // Return lower value
    if (vBufferEnd < aBufferEnd)
    {
        return vBufferEnd;
    }
    else
    {
        return aBufferEnd;
    }
}

Duration MediaDecoder::GetFrameBufferedDuration()
{
    _m_video_frames.lock();
    TimePoint vBufferEnd = _videoFrameBufferEnd;
    _m_video_frames.unlock();
    _m_audio_frames.lock();
    TimePoint aBufferEnd = _audioFrameBufferEnd;
    _m_audio_frames.unlock();

    if (vBufferEnd < aBufferEnd)
    {
        return vBufferEnd.GetTime();
    }
    else
    {
        return aBufferEnd.GetTime();
    }
}

FFmpeg::Result MediaDecoder::GetVideoCodecParamData()
{
    return FFmpeg::SerializeCodecParams(_avc_params_video);
}

FFmpeg::Result MediaDecoder::GetAudioCodecParamData()
{
    return FFmpeg::SerializeCodecParams(_avc_params_audio);
}

FFmpeg::Result MediaDecoder::GetVideoPacketData()
{
    FFmpeg::Result data;
    _m_video_packet_data.lock();
    if (_videoPacketData.size() == 0)
    {
        _m_video_packet_data.unlock();
        return { nullptr, 0 };
    }
    // I'm being cautious here because I can't be bothered to test
    FFmpeg::Result result = _videoPacketData.front();
    data.data = result.data;
    data.size = result.size;
    _videoPacketData.pop_front();
    _m_video_packet_data.unlock();
    return data;
}

FFmpeg::Result MediaDecoder::GetAudioPacketData()
{
    FFmpeg::Result data;
    _m_audio_packet_data.lock();
    if (_audioPacketData.size() == 0)
    {
        _m_audio_packet_data.unlock();
        return { nullptr, 0 };
    }
    // I'm being cautious here because I can't be bothered to test
    FFmpeg::Result result = _audioPacketData.front();
    data.data = result.data;
    data.size = result.size;
    _audioPacketData.pop_front();
    _m_audio_packet_data.unlock();
    return data;
}

AVRational MediaDecoder::GetVideoTimebase()
{
    return _videoTimebase;
}

AVRational MediaDecoder::GetAudioTimebase()
{
    return _audioTimebase;
}

void MediaDecoder::SetMetadata(Duration duration, AVRational framerate, AVRational videoTimebase, AVRational audioTimebase)
{
    if (_mode != PlaybackMode::CLIENT) return;
    _duration = duration;
    _videoInfo.framerate = std::pair<int, int>(framerate.num, framerate.den);
    _videoTimebase = videoTimebase;
    _audioTimebase = audioTimebase;
}

void MediaDecoder::AddVideoPacket(uchar* data)
{
    if (_mode != PlaybackMode::CLIENT) return;

    FFmpeg::AVPacketEx packet = FFmpeg::DeserializeAVPacketEx(data);
    _m_video_packets.lock();
    _videoPackets.push_back(packet);
    _m_video_packets.unlock();
}

void MediaDecoder::AddAudioPacket(uchar* data)
{
    if (_mode != PlaybackMode::CLIENT) return;

    FFmpeg::AVPacketEx packet = FFmpeg::DeserializeAVPacketEx(data);
    _m_audio_packets.lock();
    _audioPackets.push_back(packet);
    _m_audio_packets.unlock();
}

void MediaDecoder::ClearVideoPackets()
{
    // Clear packets
    _m_video_packets.lock();
    for (int i = 0; i < _videoPackets.size(); i++)
    {
        if (_mode != PlaybackMode::CLIENT)
        {
            av_packet_unref(_videoPackets[i].packet);
        }
        av_packet_free(&_videoPackets[i].packet);
    }
    _videoPackets.clear();
    std::cout << "Video packets cleared.\n";
    _m_video_packets.unlock();

    // Clear outgoing packet data
    _m_video_packet_data.lock();
    for (int i = 0; i < _videoPacketData.size(); i++)
    {
        delete[] _videoPacketData[i].data;
    }
    _videoPacketData.clear();
    std::cout << "Video packet data cleared.\n";
    _m_video_packet_data.unlock();
}

void MediaDecoder::ClearAudioPackets()
{
    // Clear packets
    _m_audio_packets.lock();
    for (int i = 0; i < _audioPackets.size(); i++)
    {
        if (_mode != PlaybackMode::CLIENT)
        {
            av_packet_unref(_audioPackets[i].packet);
        }
        av_packet_free(&_audioPackets[i].packet);
    }
    _audioPackets.clear();
    std::cout << "Audio packets cleared.\n";
    _m_audio_packets.unlock();

    // Clear outgoing packet data
    _m_audio_packet_data.lock();
    for (int i = 0; i < _audioPacketData.size(); i++)
    {
        delete[] _audioPacketData[i].data;
    }
    _audioPacketData.clear();
    std::cout << "Audio packet data cleared.\n";
    _m_audio_packet_data.unlock();
}

void MediaDecoder::ProcessFile(std::string filename)
{
    _avf_context = avformat_alloc_context();
    if (!_avf_context)
    {
        _initResult = false;
        _processing = false;
        return;
    }
    if (avformat_open_input(&_avf_context, filename.c_str(), NULL, NULL) != 0)
    {
        _initResult = false;
        _processing = false;
        return;
    }

    ExtractStreams();
    FindMissingStreamData();
    CalculateMissingStreamData();

    _duration = 0;
    for (auto& stream : _videoStreams)
    {
        Duration duration = Duration(av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE }));
        if (duration > _duration)
        {
            _duration = duration;
        }
    }
    for (auto& stream : _audioStreams)
    {
        Duration duration = Duration(av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE }));
        if (duration > _duration)
        {
            _duration = duration;
        }
    }
    for (auto& stream : _subtitleStreams)
    {
        Duration duration = Duration(av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE }));
        if (duration > _duration)
        {
            _duration = duration;
        }
    }
}

void MediaDecoder::ExtractStreams()
{
    _currentVideoStream = -1;
    _currentAudioStream = -1;
    _currentSubtitleStream = -1;

    for (int i = 0; i < _avf_context->nb_streams; i++)
    {
        AVStream* stream = _avf_context->streams[i];
        AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(stream->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
        {
            std::cout << "[" << t->key << "]: \"" << t->value << "\"\n";
        }

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            VideoStream vStream;
            vStream.stream = stream;
            vStream.index = i;
            vStream.name = "";
            vStream.packetCount = stream->nb_frames;
            vStream.startTime = stream->start_time;
            vStream.duration = stream->duration;
            vStream.timeBase = stream->time_base;
            vStream.width = stream->codecpar->width;
            vStream.height = stream->codecpar->height;
            //vStream.framerate = stream->r_frame_rate;
            _videoStreams.push_back(vStream);
            std::cout << "[VIDEO]" << std::endl;
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            AudioStream aStream;
            aStream.stream = stream;
            aStream.index = i;
            aStream.name = "";
            aStream.packetCount = stream->nb_frames;
            aStream.startTime = stream->start_time;
            aStream.duration = stream->duration;
            aStream.timeBase = stream->time_base;
            aStream.channels = stream->codecpar->channels;
            aStream.sampleRate = stream->codecpar->sample_rate;
            _audioStreams.push_back(aStream);
            std::cout << "[AUDIO]" << std::endl;
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            SubtitleStream sStream;
            sStream.stream = stream;
            sStream.index = i;
            sStream.name = "";
            sStream.packetCount = stream->nb_frames;
            sStream.startTime = stream->start_time;
            sStream.duration = stream->duration;
            _subtitleStreams.push_back(sStream);
            std::cout << "[SUBTITLE]" << std::endl;
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT)
        {
            std::cout << "[ATTACHMENT]" << std::endl;
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_DATA)
        {
            std::cout << "[DATA]" << std::endl;
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN)
        {
            std::cout << "[UNKNOWN]" << std::endl;
        }

        std::cout << std::endl;
    }

    if (!_videoStreams.empty()) _currentVideoStream = 0;
    if (!_audioStreams.empty()) _currentAudioStream = 0;
    if (!_subtitleStreams.empty()) _currentSubtitleStream = 0;
}

void MediaDecoder::FindMissingStreamData()
{
    // Video Streams
    for (auto& stream : _videoStreams)
    {
        std::unordered_map<std::string, std::string> metadata;
        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(stream.stream->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
        {
            metadata[to_lowercase(t->key)] = t->value;
        }

        for (auto& it : metadata)
        {
            if (it.first.substr(0, strlen("number_of_frames")) == "number_of_frames")
            {
                if (stream.packetCount == 0)
                {
                    stream.packetCount = str_to_int(it.second);
                }
            }
            else if (it.first.substr(0, strlen("duration")) == "duration")
            {
                if (stream.duration == 0 || stream.duration == AV_NOPTS_VALUE)
                {
                    stream.duration = av_rescale_q(MetadataDurationToMicrosecondsOld(it.second), { 1, AV_TIME_BASE }, stream.timeBase);
                }
            }
        }

        if (stream.startTime == AV_NOPTS_VALUE)
        {
            stream.startTime = 0;
        }
    }

    // Audio Streams
    for (auto& stream : _audioStreams)
    {
        std::unordered_map<std::string, std::string> metadata;
        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(stream.stream->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
        {
            metadata[to_lowercase(t->key)] = t->value;
        }

        for (auto& it : metadata)
        {
            if (it.first.substr(0, strlen("number_of_frames")) == "number_of_frames")
            {
                if (stream.packetCount == 0 || stream.packetCount == AV_NOPTS_VALUE)
                {
                    stream.packetCount = str_to_int(it.second);
                }
            }
            else if (it.first.substr(0, strlen("duration")) == "duration")
            {
                if (stream.duration == 0 || stream.duration == AV_NOPTS_VALUE)
                {
                    stream.duration = av_rescale_q(MetadataDurationToMicrosecondsOld(it.second), { 1, AV_TIME_BASE }, stream.timeBase);
                }
            }
        }

        if (stream.startTime == AV_NOPTS_VALUE)
        {
            stream.startTime = 0;
        }
    }

    // Subtitle Streams
    for (auto& stream : _videoStreams)
    {
        std::unordered_map<std::string, std::string> metadata;
        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(stream.stream->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
        {
            metadata[to_lowercase(t->key)] = t->value;
        }

        for (auto& it : metadata)
        {
            if (it.first.substr(0, strlen("number_of_frames")) == "number_of_frames")
            {
                if (stream.packetCount == 0 || stream.packetCount == AV_NOPTS_VALUE)
                {
                    stream.packetCount = str_to_int(it.second);
                }
            }
            else if (it.first.substr(0, strlen("duration")) == "duration")
            {
                if (stream.duration == 0 || stream.duration == AV_NOPTS_VALUE)
                {
                    stream.duration = av_rescale_q(MetadataDurationToMicrosecondsOld(it.second), { 1, AV_TIME_BASE }, stream.timeBase);
                }
            }
        }

        if (stream.startTime == AV_NOPTS_VALUE)
        {
            stream.startTime = 0;
        }
    }
}

void MediaDecoder::CalculateMissingStreamData(bool full)
{
    class StreamDecoder
    {
    public:
        AVCodecContext* codecContext = nullptr;
        MediaStreamOld* mediaStream = nullptr;
        int packetCount = 0;
        int64_t duration = 0;
        int64_t startTime = AV_NOPTS_VALUE;
        int64_t lastPts = AV_NOPTS_VALUE;
        int64_t lastDts = AV_NOPTS_VALUE;
        int64_t lastPtsDuration = AV_NOPTS_VALUE;
        int64_t lastDtsDuration = AV_NOPTS_VALUE;
        bool frameDataGathered = false;

        StreamDecoder(MediaStreamOld* stream)
        {
            mediaStream = stream;

            AVCodecParameters* params = mediaStream->stream->codecpar;
            AVCodec* codec = avcodec_find_decoder(params->codec_id);
            codecContext = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(codecContext, params);
            avcodec_open2(codecContext, codec, NULL);
        }
        virtual ~StreamDecoder()
        {
            // Get final duration
            int64_t ptsDuration = lastPts != AV_NOPTS_VALUE ? lastPts + lastPtsDuration : AV_NOPTS_VALUE;
            int64_t dtsDuration = lastDts != AV_NOPTS_VALUE ? lastDts + lastDtsDuration : AV_NOPTS_VALUE;
            if (ptsDuration != AV_NOPTS_VALUE)
            {
                duration = ptsDuration;
            }
            else if (dtsDuration != AV_NOPTS_VALUE)
            {
                duration = dtsDuration;
            }

            //if (mediaStream->packetCount != packetCount)
            //{
            //    std::cout << "Packet count mismatch (" << packetCount << "/" << mediaStream->packetCount << ") in stream " << mediaStream->index << std::endl;
            //    mediaStream->packetCount = packetCount;
            //}
            //if (mediaStream->duration != duration)
            //{
            //    std::cout << "Duration mismatch (" << duration << "/" << mediaStream->duration << ") in stream " << mediaStream->index << std::endl;
            //    mediaStream->duration = duration;
            //}

            if (mediaStream->packetCount == 0 || mediaStream->packetCount == AV_NOPTS_VALUE)
            {
                mediaStream->packetCount = packetCount;
            }
            if (mediaStream->duration == 0 || mediaStream->duration == AV_NOPTS_VALUE)
            {
                mediaStream->duration = duration;
            }

            avcodec_close(codecContext);
            avcodec_free_context(&codecContext);
        }

        bool Finished() const
        {
            if (!frameDataGathered) return false;
            if (mediaStream->packetCount == 0 || mediaStream->packetCount == AV_NOPTS_VALUE) return false;
            if (mediaStream->duration == 0 || mediaStream->duration == AV_NOPTS_VALUE) return false;
            return true;
        }

        void ExtractInfoFromPacket(AVPacket* packet)
        {
            if (packet->pts != AV_NOPTS_VALUE)
            {
                if (lastPts == AV_NOPTS_VALUE)
                {
                    lastPts = packet->pts;
                    lastPtsDuration = packet->duration;
                }
                else if (lastPts < packet->pts)
                {
                    lastPts = packet->pts;
                    lastPtsDuration = packet->duration;
                }
            }

            if (packet->dts != AV_NOPTS_VALUE)
            {
                if (lastDts == AV_NOPTS_VALUE)
                {
                    lastDts = packet->dts;
                    lastDtsDuration = packet->duration;
                }
                else if (lastDts < packet->dts)
                {
                    lastDts = packet->dts;
                    lastDtsDuration = packet->duration;
                }
            }

            packetCount++;
        }

        void ExtractInfoFromFrame(AVFrame* frame)
        {
            duration = frame->pts + frame->pkt_duration;
            if (startTime == AV_NOPTS_VALUE)
            {
                startTime = frame->pts;
            }

            _ExtractInfoFromFrame(frame);
        }

    private:
        virtual void _ExtractInfoFromFrame(AVFrame* frame) = 0;
    };

    class VideoStreamDecoder : public StreamDecoder
    {
    public:
        VideoStreamDecoder(MediaStreamOld* stream) : StreamDecoder(stream)
        {
            VideoStream* vStream = (VideoStream*)stream;
            if (vStream->width != 0 && vStream->height != 0)
            {
                frameDataGathered = true;
            }
        }

    private:
        void _ExtractInfoFromFrame(AVFrame* frame)
        {
            VideoStream* stream = (VideoStream*)mediaStream;

            if (stream->width == 0)
            {
                stream->width = frame->width;
            }
            if (stream->height == 0)
            {
                stream->height = frame->height;
            }

            frameDataGathered = true;
        }
    };

    class AudioStreamDecoder : public StreamDecoder
    {
    public:
        AudioStreamDecoder(MediaStreamOld* stream) : StreamDecoder(stream)
        {
            AudioStream* aStream = (AudioStream*)stream;
            if (aStream->channels != 0 && aStream->sampleRate != 0)
            {
                frameDataGathered = true;
            }
        }

    private:
        void _ExtractInfoFromFrame(AVFrame* frame)
        {
            AudioStream* stream = (AudioStream*)mediaStream;

            if (stream->channels == 0)
            {
                stream->channels = frame->channels;
            }
            if (stream->sampleRate == 0)
            {
                stream->sampleRate = frame->sample_rate;
            }

            frameDataGathered = true;
        }
    };

    // Align stream_index
    std::vector<std::unique_ptr<StreamDecoder>> streamDecoders;
    streamDecoders.resize(_avf_context->nb_streams);

    for (int i = 0; i < _videoStreams.size(); i++)
    {
        VideoStreamDecoder* decoder = new VideoStreamDecoder(&_videoStreams[i]);
        streamDecoders[decoder->mediaStream->index] = std::unique_ptr<StreamDecoder>(decoder);
    }
    for (int i = 0; i < _audioStreams.size(); i++)
    {
        AudioStreamDecoder* decoder = new AudioStreamDecoder(&_audioStreams[i]);
        streamDecoders[decoder->mediaStream->index] = std::unique_ptr<StreamDecoder>(decoder);
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    // Automatic unref wrapper for AVPacket
    struct PacketHolder
    {
        AVPacket* packet;
        ~PacketHolder() { av_packet_unref(packet); }
    };

    while (av_read_frame(_avf_context, packet) >= 0)
    {
        PacketHolder holder = { packet };

        //if (packet->stream_index == 0)
        //{
        //    int k = 0;
        //    std::cout << packet->pts << " | " << packet->dts;
        //    if (packet->flags & AV_PKT_FLAG_KEY)
        //    {
        //        std::cout << " (KEY)";
        //        k++;
        //    }
        //    std::cout << std::endl;
        //}

        // Check if all decoders have gathered missing data
        bool done = true;
        for (int i = 0; i < streamDecoders.size(); i++)
        {
            if (streamDecoders[i])
            {
                if (!streamDecoders[i]->Finished())
                {
                    done = false;
                    break;
                }
            }
        }
        if (done) break; // UNCOMMENT ///////////////////////////////////////////////////////////////////////

        int index = packet->stream_index;
        if (!streamDecoders[index]) continue;

        streamDecoders[index]->ExtractInfoFromPacket(packet);

        // Only decode until necessary data is extracted
        if (streamDecoders[index]->frameDataGathered) continue; // UNCOMMENT /////////////////////////////////

        int response = avcodec_send_packet(streamDecoders[index]->codecContext, packet);
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            continue;
        }

        response = avcodec_receive_frame(streamDecoders[index]->codecContext, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            continue;
        }

        if (packet->stream_index == 0)
        {
            int k = 0;
            if (frame->key_frame)
            {
                k++;
            }
        }

        streamDecoders[index]->ExtractInfoFromFrame(frame);
    }

    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);

    streamDecoders.clear();
    std::cout << "end" << std::endl;
}

void MediaDecoder::PacketExtractor()
{
    if (_mode == PlaybackMode::CLIENT) return;

    AVCodecContext* avc_context_subs = avcodec_alloc_context3(_av_codec_subs);
    avcodec_parameters_to_context(avc_context_subs, _avc_params_subs);
    avcodec_open2(avc_context_subs, _av_codec_subs, NULL);

    AVPacket* av_packet = av_packet_alloc();
    bool eof = false;
    int64_t packetSize = 0;

    while (!_packetThreadController.Get<bool>("stop"))
    {
        long long int seekTime = _packetThreadController.Get<long long int>("seek");
        // Seek
        if (seekTime >= 0)
        {
            double seconds = seekTime / 1000000.0;
            if (seconds < 0) seconds = 0.0;
            int videoFrameIndex = seconds * _videoInfo.framerate.first / (double)_videoInfo.framerate.second;
            int audioFrameIndex = seconds * _audioInfo.sampleRate / (double)_audioInfo.samplesPerFrame;

            std::cout << "Seeking to " << seconds << std::endl;

            int result;
            int64_t rescaledTime = av_rescale_q(seekTime, { 1, AV_TIME_BASE }, _avf_context->streams[_video_stream_index]->time_base);
            std::cout << "Rescaled time: " << rescaledTime << std::endl;
            result = av_seek_frame(
                _avf_context,
                _video_stream_index,
                rescaledTime,
                //videoFrameIndex,
                AVSEEK_FLAG_BACKWARD
            );
            std::cout << "Video seek result: " << result << std::endl;
            rescaledTime = av_rescale_q(seekTime, { 1, AV_TIME_BASE }, _avf_context->streams[_audio_stream_index]->time_base);
            std::cout << "Rescaled time: " << rescaledTime << std::endl;
            result = av_seek_frame(
                _avf_context,
                _audio_stream_index,
                rescaledTime,
                //audioFrameIndex,
                AVSEEK_FLAG_BACKWARD
            );
            std::cout << "Audio seek result: " << result << std::endl;

            // Clear packet buffer
            ClearVideoPackets();
            ClearAudioPackets();

            // Send flush command to decoder threads
            _videoThreadController.Set("flush", true);
            _audioThreadController.Set("flush", true);

            // Wait for decoder threads to flush
            while (_videoThreadController.Get<bool>("flush") || _audioThreadController.Get<bool>("flush"))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            _packetThreadController.Set("seek", -1LL);
        }

        // Read audio and video packets
        if (av_read_frame(_avf_context, av_packet) >= 0)
        {
            if (eof)
            {
                eof = false;
                _packetThreadController.Set("eof", false);
            }

            //uchar* bytes = FFmpegSerializer::SerializeAVPacket(av_packet).data;
            //av_packet = FFmpegSerializer::DeserializeAVPacket(bytes);

            if (av_packet->stream_index == _videoStreams[_currentVideoStream].index)
            {
                FFmpeg::AVPacketEx pack = { av_packet, false, false };

                if (_mode == PlaybackMode::SERVER)
                {
                    auto data = FFmpeg::SerializeAVPacketEx(pack);
                    _m_video_packet_data.lock();
                    _videoPacketData.push_back(data);
                    _m_video_packet_data.unlock();
                }

                _m_video_packets.lock();
                _videoPackets.push_back(pack);
                _m_video_packets.unlock();
                av_packet = av_packet_alloc();
            }
            else if (av_packet->stream_index == _audioStreams[_currentAudioStream].index)
            {
                FFmpeg::AVPacketEx pack = { av_packet, false, false };

                if (_mode == PlaybackMode::SERVER)
                {
                    auto data = FFmpeg::SerializeAVPacketEx(pack);
                    _m_audio_packet_data.lock();
                    _audioPacketData.push_back(data);
                    _m_audio_packet_data.unlock();
                }

                _m_audio_packets.lock();
                _audioPackets.push_back(pack);
                _m_audio_packets.unlock();
                av_packet = av_packet_alloc();
            }
            else if (av_packet->stream_index == _subtitleStreams[_currentSubtitleStream].index)
            {
                AVSubtitle sub;
                int got;
                avcodec_decode_subtitle2(avc_context_subs, &sub, &got, av_packet);
                //_m_audio_packets.lock();
                //_audioPackets.push_back(av_packet);
                //_m_audio_packets.unlock();
                av_packet = av_packet_alloc();
            }
        }
        else
        {
            if (!eof)
            {
                eof = true;
                _packetThreadController.Set("eof", true);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    av_packet_free(&av_packet);
}

void MediaDecoder::VideoPacketDecoder()
{
    AVCodecContext* avc_context_video = avcodec_alloc_context3(_av_codec_video);
    avcodec_parameters_to_context(avc_context_video, _avc_params_video);
    avcodec_open2(avc_context_video, _av_codec_video, NULL);

    AVFrame* av_frame = av_frame_alloc();

    SwsContext * sws_context = NULL;

    uchar* data = new uchar[avc_context_video->width * avc_context_video->height * 4];
    uchar* dest[4] = { data, NULL, NULL, NULL };
    int dest_linesize[4] = { avc_context_video->width * 4, 0, 0, 0 };

    int frameIndex = 0;
    while (!_videoThreadController.Get<bool>("stop"))
    {
        // Seek
        if (_videoThreadController.Get<bool>("flush"))
        {
            //std::cout << "Video flushing.\n";
            avcodec_flush_buffers(avc_context_video);
            frameIndex = 0;
            _m_video_frames.lock();
            _videoFrames.clear();
            _m_video_frames.unlock();
            std::cout << "Video frames cleared.\n";
            _videoThreadController.Set("flush", false);
            std::cout << "Video flush set to false.\n";
            continue;
        }

        // Limit loaded frame count
        if (_videoFrames.size() >= _MAX_VIDEO_QUEUE_SIZE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // If finished, wait for seek command/new packets
        _m_video_packets.lock(); // Prevent packet flushing on seek
        if (frameIndex >= _videoPackets.size())
        {
            _m_video_packets.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        //std::cout << "ps ";
        //AVBufferRef* bref = _videoPackets[frameIndex]->buf;
        //_videoPackets[frameIndex]->buf = NULL;

        int response = avcodec_send_packet(avc_context_video, _videoPackets[frameIndex].packet);
        _m_video_packets.unlock();
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            break;
        }
        response = avcodec_receive_frame(avc_context_video, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            frameIndex++;
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            break;
        }

        //auto timeBase = _avf_context->streams[_video_stream_index]->time_base;
        //for (int i = 1; i < av_frame->pict_type; i++) std::cout << " ";
        //std::cout << av_frame->pict_type << " " << (double)av_frame->pts / timeBase.den * timeBase.num << " " << std::endl;
        //std::cout << av_frame->pict_type << std::endl;

        if (!sws_context)
        {
            sws_context = sws_getContext(
                avc_context_video->width,
                avc_context_video->height,
                avc_context_video->pix_fmt,
                avc_context_video->width,
                avc_context_video->height,
                AV_PIX_FMT_BGR0,
                SWS_FAST_BILINEAR,
                NULL,
                NULL,
                NULL
            );
        }
        
        sws_scale(sws_context, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);
        AVRational timebase;
        if (_mode == PlaybackMode::CLIENT)
        {
            timebase = _videoTimebase;
        }
        else
        {
            timebase = _avf_context->streams[_video_stream_index]->time_base;
        }
        long long int timestamp = av_rescale_q(av_frame->pts, timebase, { 1, AV_TIME_BASE });
        //std::cout << timestamp << std::endl;

        FrameData fd = FrameData(av_frame->width, av_frame->height, timestamp);
        fd.SetBytes(data);

        _m_video_frames.lock();
        _videoFrames.push_back(std::move(fd));
        _videoFrameBufferEnd = timestamp;
        _m_video_frames.unlock();

        frameIndex++;
    }

    delete[] data;
    sws_freeContext(sws_context);
    avcodec_close(avc_context_video);
    avcodec_free_context(&avc_context_video);
}

void MediaDecoder::AudioPacketDecoder()
{
    std::ofstream sAudioOut;
    std::ofstream cAudioOut;
    int64_t sampleCounter = 0;

    //if (_mode == PlaybackMode::SERVER || _mode == PlaybackMode::OFFLINE)
    //{
    //    sAudioOut.open("server.pkt");
    //}
    //else if (_mode == PlaybackMode::CLIENT)
    //{
    //    cAudioOut.open("client.pkt");
    //    //, std::ios::binary
    //}

    AVCodecContext* avc_context_audio = avcodec_alloc_context3(_av_codec_audio);
    avcodec_parameters_to_context(avc_context_audio, _avc_params_audio);
    avcodec_open2(avc_context_audio, _av_codec_audio, NULL);

    AVFrame* av_frame = av_frame_alloc();

    int frameIndex = 0;
    while (!_audioThreadController.Get<bool>("stop"))
    {
        // Seek
        if (_audioThreadController.Get<bool>("flush"))
        {
            //std::cout << "Audio flushing.\n";
            avcodec_flush_buffers(avc_context_audio);
            frameIndex = 0;
            _m_audio_frames.lock();
            _audioFrames.clear();
            _m_audio_frames.unlock();
            std::cout << "Audio frames cleared.\n";
            _audioThreadController.Set("flush", false);
            std::cout << "Audio flush set to false.\n";
            continue;
        }

        // Limit loaded frame count
        if (_audioFrames.size() >= _MAX_AUDIO_QUEUE_SIZE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // If finished, wait for seek command
        _m_audio_packets.lock(); // Prevent packet flushing on seek
        if (frameIndex >= _audioPackets.size())
        {
            _m_audio_packets.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (_mode == PlaybackMode::SERVER)
        {
            //sAudioOut.write((char*)_audioPackets[frameIndex]->data, _audioPackets[frameIndex]->size);
            //sAudioOut.write(audio_data, chunkSize);
            //sAudioOut << valf << '\n';
        }
        else if (_mode == PlaybackMode::CLIENT)
        {
            //cAudioOut.write((char*)_audioPackets[frameIndex]->data, _audioPackets[frameIndex]->size);
            //cAudioOut.write(audio_data, chunkSize);
            //cAudioOut << valf << '\n';
        }

        //auto timeBase = _avf_context->streams[_audio_stream_index]->time_base;
        //long long int timestamp2 = av_rescale_q(_audioPackets[frameIndex]->pts, timeBase, { 1, AV_TIME_BASE });
        //std::cout << timestamp2 / 1000 << "|";

        int response = avcodec_send_packet(avc_context_audio, _audioPackets[frameIndex].packet);
        _m_audio_packets.unlock();
        frameIndex++;
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            break;
        }
        response = avcodec_receive_frame(avc_context_audio, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            break;
        }

        //auto timeBase = _avf_context->streams[_audio_stream_index]->time_base;
        //std::cout << (double)av_frame->pts / timeBase.den * timeBase.num << " " << std::endl;

        int data_size = av_get_bytes_per_sample(avc_context_audio->sample_fmt);
        if (data_size < 0)
        {
            // This should not occur, checking just for paranoia
            printf("Failed to calculate data size\n");
            exit(1);
        }

        int chunkSize = av_frame->nb_samples * av_frame->channels * 2;
        char* audio_data = new char[chunkSize];
        int curPos = 0;

        for (int i = 0; i < av_frame->nb_samples; i++)
        {
            for (int ch = 0; ch < avc_context_audio->channels; ch++)
            {
                float valf = *(float*)(av_frame->data[ch] + data_size * i);
                if (valf < -1.0f) valf = -1.0f;
                else if (valf > 1.0f) valf = 1.0f;
                int16_t vali = (int16_t)(valf * 32767.0f);
                *(int16_t*)(audio_data + curPos) = vali;
                curPos += 2;

                if (_mode == PlaybackMode::SERVER)
                {
                    //sAudioOut.write(audio_data, chunkSize);
                    //sAudioOut << valf << '\n';
                }
                else if (_mode == PlaybackMode::CLIENT)
                {
                    //cAudioOut.write(audio_data, chunkSize);
                    //cAudioOut << valf << '\n';
                }
            }
        }

        
        
        AVRational timebase;
        if (_mode == PlaybackMode::CLIENT)
        {
            timebase = _audioTimebase;
        }
        else
        {
            timebase = _avf_context->streams[_audio_stream_index]->time_base;
        }
        long long int timestamp = av_rescale_q(av_frame->pts, timebase, { 1, AV_TIME_BASE });
        //std::cout << timestamp / 1000 << " ";

        AudioData ad = AudioData(av_frame->nb_samples, av_frame->channels, av_frame->sample_rate, timestamp);
        ad.SetBytes(audio_data);

        _m_audio_frames.lock();
        _audioFrames.push_back(std::move(ad));
        _audioFrameBufferEnd = timestamp;
        _m_audio_frames.unlock();

        delete[] audio_data;

        //frameIndex++;
    }

    avcodec_free_context(&avc_context_audio);
}

int64_t MetadataDurationToMicrosecondsOld(std::string data)
{
    int64_t time = 0;

    std::vector<std::string> hrsMinVec;
    split_str(data, hrsMinVec, ':');
    if (hrsMinVec.size() != 3)
    {
        return 0;
    }
    if (hrsMinVec.size() == 3)
    {
        time += str_to_int(hrsMinVec[0]) * 3600000000LL;
        time += str_to_int(hrsMinVec[1]) * 60000000LL;
    }
    else if (hrsMinVec.size() == 2)
    {
        time += str_to_int(hrsMinVec[0]) * 60000000LL;
    }

    std::string secNanoStr = hrsMinVec.back();
    std::vector<std::string> secNanoVec;
    split_str(secNanoStr, secNanoVec, '.');
    if (secNanoVec.size() != 2)
    {
        return 0;
    }
    time += str_to_int(secNanoVec[0]) * 1000000;
    time += str_to_int(secNanoVec[1]) / 1000;

    return time;
}