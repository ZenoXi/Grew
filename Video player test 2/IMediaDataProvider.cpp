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
    return mediaData.lastPts > mediaData.lastDts ? mediaData.lastPts.GetTime(NANOSECONDS) : mediaData.lastDts.GetTime(NANOSECONDS);
}

Duration IMediaDataProvider::MediaDuration()
{
    Duration finalDuration = 0;
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
    return finalDuration;
}

void IMediaDataProvider::Seek(TimePoint time)
{
    _Seek(time);
}

std::unique_ptr<MediaStream> IMediaDataProvider::SetVideoStream(int index)
{
    auto stream = _SetStream(_videoData, index);
    if (stream) _SetVideoStream(index);
    return stream;
}

std::unique_ptr<MediaStream> IMediaDataProvider::SetAudioStream(int index)
{
    auto stream = _SetStream(_videoData, index);
    if (stream) _SetAudioStream(index);
    return stream;
}

std::unique_ptr<MediaStream> IMediaDataProvider::SetSubtitleStream(int index)
{
    auto stream = _SetStream(_videoData, index);
    if (stream) _SetSubtitleStream(index);
    return stream;
}

std::unique_ptr<MediaStream> IMediaDataProvider::_SetStream(MediaData& mediaData, int index)
{
    if (index == -1) index = 0; // -1 means default stream
    if (index < -1) return nullptr;
    if (index >= mediaData.streams.size()) return nullptr;
    mediaData.currentStream = index;
    return std::make_unique<MediaStream>(mediaData.streams[index]);
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
        streams.push_back(stream.name);
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
        return nullptr;
    }
    else
    {
        MediaPacket copy;
        copy.Deserialize(mediaData.packets[mediaData.currentPacket++].Serialize());
        return copy;
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
    if (packet.GetPacket()->pts > mediaData.lastPts) mediaData.lastPts = packet.GetPacket()->pts;
    if (packet.GetPacket()->dts > mediaData.lastDts) mediaData.lastDts = packet.GetPacket()->dts;
    mediaData.packets.push_back(std::move(packet));
}