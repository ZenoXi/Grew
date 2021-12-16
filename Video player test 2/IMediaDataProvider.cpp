#include "IMediaDataProvider.h"

#include "Functions.h"

IMediaDataProvider::IMediaDataProvider(IMediaDataProvider* other)
{
    _videoData.streams = other->_videoData.streams;
    _videoData.currentStream = other->_videoData.currentStream;
    _audioData.streams = other->_audioData.streams;
    _audioData.currentStream = other->_audioData.currentStream;
    _subtitleData.streams = other->_subtitleData.streams;
    _subtitleData.currentStream = other->_subtitleData.currentStream;
    _attachmentStreams = other->_attachmentStreams;
    _dataStreams = other->_dataStreams;
    _unknownStreams = other->_unknownStreams;
}

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
        MediaStream& stream = _videoData.streams[_videoData.currentStream];
        int64_t startTime = av_rescale_q(stream.startTime, stream.timeBase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE });
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    if (_audioData.currentStream != -1)
    {
        MediaStream& stream = _audioData.streams[_audioData.currentStream];
        int64_t startTime = av_rescale_q(stream.startTime, stream.timeBase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE });
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    if (_subtitleData.currentStream != -1)
    {
        MediaStream& stream = _subtitleData.streams[_subtitleData.currentStream];
        int64_t startTime = av_rescale_q(stream.startTime, stream.timeBase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE });
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    return Duration(finalDuration, MICROSECONDS);
}

Duration IMediaDataProvider::MaxMediaDuration()
{
    int64_t finalDuration = 0;
    for (auto& stream : _videoData.streams)
    {
        int64_t startTime = av_rescale_q(stream.startTime, stream.timeBase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE });
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    for (auto& stream : _audioData.streams)
    {
        int64_t startTime = av_rescale_q(stream.startTime, stream.timeBase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE });
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    for (auto& stream : _subtitleData.streams)
    {
        int64_t startTime = av_rescale_q(stream.startTime, stream.timeBase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(stream.duration, stream.timeBase, { 1, AV_TIME_BASE });
        if (startTime + duration > finalDuration)
        {
            finalDuration = startTime + duration;
        }
    }
    return Duration(finalDuration, MICROSECONDS);
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

std::vector<MediaStream> IMediaDataProvider::GetFontStreams()
{
    std::vector<MediaStream> fontStreams;

    // Search attachment streams
    for (auto& stream : _attachmentStreams)
    {
        bool isFont = false;
        for (auto& data : stream.metadata)
        {
            if (data.key == "filename")
            {
                std::vector<std::string> split;
                split_str(data.value, split, '.');
                if (!split.empty())
                {
                    std::string value = to_lowercase(split.back());
                    if (value == "ttf") { isFont = true; break; }
                    else if (value == "otf") { isFont = true; break; }
                }
            }
            else if (data.key == "mimetype")
            {
                // This list will likely grow in the future, this is just a naive implementation
                if (data.value == "application/vnd.ms-opentype") { isFont = true; break; }
                else if (data.value == "application/x-truetype-font") { isFont = true; break; }
                else if (data.value == "application/font-sfnt") { isFont = true; break; }
                else if (data.value == "application/font-woff") { isFont = true; break; }
                else if (data.value == "application/font-woff2") { isFont = true; break; }
                else if (data.value == "application/x-font-truetype") { isFont = true; break; }
                else if (data.value == "application/x-font-opentype") { isFont = true; break; }
            }
        }
        if (isFont) fontStreams.push_back(stream);
    }

    // Search data streams

    // Search unknown streams

    return fontStreams;
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

void IMediaDataProvider::_AddFlushPackets()
{
    
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