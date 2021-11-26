#include "IMediaDataProvider.h"

bool IMediaDataProvider::Initializing()
{
    return _initializing;
}

bool IMediaDataProvider::InitFailed()
{
    return _initFailed;
}

bool IMediaDataProvider::Loading()
{
    return _loading;
}

Duration IMediaDataProvider::BufferedDuration()
{
    Duration videoBuffer = _BufferedDuration(_videoData);
    Duration audioBuffer = _BufferedDuration(_audioData);
    Duration subtitleBuffer = _BufferedDuration(_subtitleData);
    Duration buffered = Duration::Min();
    if (videoBuffer > buffered) buffered = videoBuffer;
    if (audioBuffer > buffered) buffered = audioBuffer;
    if (subtitleBuffer > buffered) buffered = subtitleBuffer;
    return buffered;
}

Duration IMediaDataProvider::_BufferedDuration(MediaData& mediaData)
{
    std::unique_lock<std::mutex> lock(mediaData.mtx);
    if (mediaData.currentStream == -1)
    {
        return Duration::Max();
    }
    return mediaData.lastPts > mediaData.lastDts ? mediaData.lastPts.GetTicks() : mediaData.lastDts.GetTicks();
}

Duration IMediaDataProvider::MediaDuration()
{
    int64_t finalDuration = 0;
    if (_videoData.currentStream != -1)
    {
        int64_t startTime = _videoData.streams[_videoData.currentStream].startTime;
        int64_t duration = _videoData.streams[_videoData.currentStream].duration;
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    if (_audioData.currentStream != -1)
    {
        int64_t startTime = _audioData.streams[_audioData.currentStream].startTime;
        int64_t duration = _audioData.streams[_audioData.currentStream].duration;
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    if (_subtitleData.currentStream != -1)
    {
        int64_t startTime = _subtitleData.streams[_subtitleData.currentStream].startTime;
        int64_t duration = _subtitleData.streams[_subtitleData.currentStream].duration;
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    return Duration(finalDuration, MILLISECONDS);
}

void IMediaDataProvider::Seek(TimePoint time)
{
    _Seek(time);
}

std::unique_ptr<MediaStream> IMediaDataProvider::SetVideoStream(int index, TimePoint time)
{
    auto stream = _SetStream(_videoData, index);
    if (stream || index == -1) _SetVideoStream(index, time);
    return stream;
}

std::unique_ptr<MediaStream> IMediaDataProvider::SetAudioStream(int index, TimePoint time)
{
    auto stream = _SetStream(_audioData, index);
    if (stream || index == -1) _SetAudioStream(index, time);
    return stream;
}

std::unique_ptr<MediaStream> IMediaDataProvider::SetSubtitleStream(int index, TimePoint time)
{
    auto stream = _SetStream(_subtitleData, index);
    if (stream || index == -1) _SetSubtitleStream(index, time);
    return stream;
}

std::unique_ptr<MediaStream> IMediaDataProvider::_SetStream(MediaData& mediaData, int index)
{
    if (index < 0) return nullptr;
    if (index >= mediaData.streams.size()) return nullptr;
    return std::make_unique<MediaStream>(mediaData.streams[index]);
}

std::unique_ptr<MediaStream> IMediaDataProvider::CurrentVideoStream()
{
    return _CurrentStream(_videoData);
}

std::unique_ptr<MediaStream> IMediaDataProvider::CurrentAudioStream()
{
    return _CurrentStream(_audioData);
}

std::unique_ptr<MediaStream> IMediaDataProvider::CurrentSubtitleStream()
{
    return _CurrentStream(_subtitleData);
}

std::unique_ptr<MediaStream> IMediaDataProvider::_CurrentStream(MediaData& mediaData)
{
    if (mediaData.currentStream != -1)
    {
        return std::make_unique<MediaStream>(mediaData.streams[mediaData.currentStream]);
    }
    else
    {
        return nullptr;
    }
}

int IMediaDataProvider::CurrentVideoStreamIndex() const
{
    return _CurrentStreamIndex(_videoData);
}

int IMediaDataProvider::CurrentAudioStreamIndex() const
{
    return _CurrentStreamIndex(_audioData);
}

int IMediaDataProvider::CurrentSubtitleStreamIndex() const
{
    return _CurrentStreamIndex(_subtitleData);
}

int IMediaDataProvider::_CurrentStreamIndex(const MediaData& mediaData) const
{
    return mediaData.currentStream;
}

std::vector<std::string> IMediaDataProvider::GetAvailableVideoStreams()
{
    return _GetAvailableStreams(_videoData);
}

std::vector<std::string> IMediaDataProvider::GetAvailableAudioStreams()
{
    return _GetAvailableStreams(_audioData);
}

std::vector<std::string> IMediaDataProvider::GetAvailableSubtitleStreams()
{
    return _GetAvailableStreams(_subtitleData);
}

std::vector<std::string> IMediaDataProvider::_GetAvailableStreams(MediaData& mediaData)
{
    std::vector<std::string> streams;
    streams.reserve(mediaData.streams.size());
    for (auto& stream : mediaData.streams)
    {
        streams.push_back("");
    }
    return streams;
}

size_t IMediaDataProvider::VideoPacketCount() const
{
    return _PacketCount(_videoData);
}

size_t IMediaDataProvider::AudioPacketCount() const
{
    return _PacketCount(_audioData);
}

size_t IMediaDataProvider::SubtitlePacketCount() const
{
    return _PacketCount(_subtitleData);
}

size_t IMediaDataProvider::_PacketCount(const MediaData& mediaData) const
{
    return mediaData.packets.size();
}

MediaPacket IMediaDataProvider::GetVideoPacket()
{
    return _GetPacket(_videoData);
}

MediaPacket IMediaDataProvider::GetAudioPacket()
{
    return _GetPacket(_audioData);
}

MediaPacket IMediaDataProvider::GetSubtitlePacket()
{
    return _GetPacket(_subtitleData);
}

MediaPacket IMediaDataProvider::_GetPacket(MediaData& mediaData)
{
    std::unique_lock<std::mutex> lock(mediaData.mtx);
    if (mediaData.currentPacket >= mediaData.packets.size())
    {
        return MediaPacket();
    }
    else
    {
        MediaPacket copy;
        copy.Deserialize(mediaData.packets[mediaData.currentPacket++].Serialize());
        return copy;
    }
}

bool IMediaDataProvider::FlushVideoPacketNext()
{
    return _FlushPacketNext(_videoData);
}

bool IMediaDataProvider::FlushAudioPacketNext()
{
    return _FlushPacketNext(_audioData);
}

bool IMediaDataProvider::FlushSubtitlePacketNext()
{
    return _FlushPacketNext(_subtitleData);
}

bool IMediaDataProvider::_FlushPacketNext(MediaData& mediaData)
{
    std::unique_lock<std::mutex> lock(mediaData.mtx);
    if (mediaData.currentPacket >= mediaData.packets.size())
    {
        return false;
    }
    else
    {
        return mediaData.packets[mediaData.currentPacket].flush;
    }
}

void IMediaDataProvider::_AddVideoPacket(MediaPacket packet)
{
    _AddPacket(_videoData, std::move(packet));
}

void IMediaDataProvider::_AddAudioPacket(MediaPacket packet)
{
    _AddPacket(_audioData, std::move(packet));
}

void IMediaDataProvider::_AddSubtitlePacket(MediaPacket packet)
{
    _AddPacket(_subtitleData, std::move(packet));
}

void IMediaDataProvider::_AddPacket(MediaData& mediaData, MediaPacket packet)
{
    std::unique_lock<std::mutex> lock(mediaData.mtx);
    if (!packet.flush && packet.Valid())
    {
        AVRational timebase = mediaData.streams[mediaData.currentStream].timeBase;
        if (packet.GetPacket()->pts != AV_NOPTS_VALUE)
        {
            TimePoint pts = TimePoint(av_rescale_q(packet.GetPacket()->pts, timebase, { 1, AV_TIME_BASE }), MICROSECONDS);
            if (pts > mediaData.lastPts) mediaData.lastPts = pts;
        }
        if (packet.GetPacket()->dts != AV_NOPTS_VALUE)
        {
            TimePoint dts = TimePoint(av_rescale_q(packet.GetPacket()->dts, timebase, { 1, AV_TIME_BASE }), MICROSECONDS);
            if (dts > mediaData.lastDts) mediaData.lastDts = dts;
        }
    }
    mediaData.packets.push_back(std::move(packet));
}

void IMediaDataProvider::_ClearVideoPackets()
{
    _ClearPackets(_videoData);
}

void IMediaDataProvider::_ClearAudioPackets()
{
    _ClearPackets(_audioData);
}

void IMediaDataProvider::_ClearSubtitlePackets()
{
    _ClearPackets(_subtitleData);
}

void IMediaDataProvider::_ClearPackets(MediaData& mediaData)
{
    std::unique_lock<std::mutex> lock(mediaData.mtx);
    mediaData.packets.clear();
    mediaData.lastPts = TimePoint::Min();
    mediaData.lastDts = TimePoint::Min();
    mediaData.currentPacket = 0;
}