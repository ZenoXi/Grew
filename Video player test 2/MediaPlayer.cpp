#include "MediaPlayer.h"

#include <iostream>

bool TimeExceeded(Clock timer, double timeLimit)
{
    timer.Update();
    return timer.Now().GetTime() / 1000000.0 > timeLimit;
}

MediaPlayer::MediaPlayer(
    IMediaDataProvider* dataProvider,
    std::unique_ptr<IVideoOutputAdapter> videoAdapter,
    std::unique_ptr<IAudioOutputAdapter> audioAdapter
) : _dataProvider(dataProvider),
    _videoOutputAdapter(std::move(videoAdapter)),
    _audioOutputAdapter(std::move(audioAdapter))
{
    std::unique_ptr<MediaStream> videoStream = _dataProvider->CurrentVideoStream();
    std::unique_ptr<MediaStream> audioStream = _dataProvider->CurrentAudioStream();
    std::unique_ptr<MediaStream> subtitleStream = _dataProvider->CurrentSubtitleStream();

    if (videoStream) _videoData.decoder = new VideoDecoder(*videoStream);
    if (audioStream) _audioData.decoder = new AudioDecoder(*audioStream);
    if (subtitleStream)
    {
        SubtitleDecoder* decoder = new SubtitleDecoder(*subtitleStream);
        auto fontStreams = _dataProvider->GetFontStreams();
        std::vector<SubtitleDecoder::FontDesc> fonts;
        for (auto& stream : fontStreams)
        {
            SubtitleDecoder::FontDesc font;
            font.data = (char*)stream.GetParams()->extradata;
            font.dataSize = stream.GetParams()->extradata_size;
            font.name = (char*)"";
            fonts.push_back(font);
        }
        decoder->AddFonts(fonts);
        _subtitleData.decoder = decoder;
    }

    _recovering = true;

    _playbackTimer = Clock(0);
    _playbackTimer.Stop();
}

MediaPlayer::~MediaPlayer()
{
    if (_videoData.decoder) delete _videoData.decoder;
    if (_audioData.decoder) delete _audioData.decoder;
    if (_subtitleData.decoder) delete _subtitleData.decoder;
}

int MediaPlayer::_PassPacket(MediaData& mediaData, MediaPacket packet)
{
    // Flush packet
    if (packet.flush)
    {
        _recovering = true;
        _recovered = false;
        if (mediaData.expectingStream)
        {
            std::cout << "Decoder reset\n";
            delete mediaData.decoder;
            mediaData.decoder = nullptr;
            mediaData.expectingStream = false;
            return 3;
        }
        else
        {
            std::cout << "Flush started\n";
            mediaData.decoder->Flush();
            return 2;
        }
    }

    if (packet.Valid())
    {
        mediaData.decoder->AddPacket(std::move(packet));
        return 1;
    }
    return 0;
}

void MediaPlayer::Update(double timeLimit)
{
    _playbackTimer.Update();

    Clock funcTimer = Clock();

    // Sync timer to audio
    //TimePoint adapterTime = TimePoint(_audioOutputAdapter->CurrentTime() + _audioOutputAdapter->TimeSinceLastBufferStart(), MICROSECONDS);
    //Duration timeDelta = adapterTime - _playbackTimer.Now();
    //if (std::abs(timeDelta.GetDuration(MILLISECONDS)) > 50)
    //{
    //    _playbackTimer.AdvanceTime(timeDelta);
    //}

    // Read and pass packets to decoders
    while (true)
    {
        if (TimeExceeded(funcTimer, timeLimit)) break;

        // If this is 0 (no packets sent to decoder), the loop is broken
        int packetGot = 0;
        if (_videoData.decoder && !_videoData.decoder->Flushing())
        {
            if (!_videoData.decoder->PacketQueueFull() || _dataProvider->FlushVideoPacketNext())
            {
                int passResult = _PassPacket(_videoData, _dataProvider->GetVideoPacket());
                // If flushing, clear the stored next frame to prevent
                // frames with lower timestamps after flush getting stuck
                if (passResult == 2 || passResult == 3) _videoData.nextFrame.reset(nullptr);
                if (passResult == 3)
                {
                    if (_videoData.pendingStream) _videoData.decoder = new VideoDecoder(*_videoData.pendingStream);
                }
                packetGot += passResult;
            }
        }
        if (_audioData.decoder && !_audioData.decoder->Flushing())
        {
            if (!_audioData.decoder->PacketQueueFull() || _dataProvider->FlushAudioPacketNext())
            {
                int passResult = _PassPacket(_audioData, _dataProvider->GetAudioPacket());
                // See above
                if (passResult == 2 || passResult == 3) _audioData.nextFrame.reset(nullptr);
                if (passResult == 3)
                {
                    if (_audioData.pendingStream) _audioData.decoder = new AudioDecoder(*_audioData.pendingStream);
                }
                packetGot += passResult;
            }
        }
        if (_subtitleData.decoder && !_subtitleData.decoder->Flushing())
        {
            if (!_subtitleData.decoder->PacketQueueFull() || _dataProvider->FlushSubtitlePacketNext())
            {
                int passResult = _PassPacket(_subtitleData, _dataProvider->GetSubtitlePacket());
                // See above
                if (passResult == 2 || passResult == 3) _subtitleData.nextFrame.reset(nullptr);
                if (passResult == 3)
                {
                    if (_subtitleData.pendingStream)
                    {
                        SubtitleDecoder* decoder = new SubtitleDecoder(*_subtitleData.pendingStream);
                        auto fontStreams = _dataProvider->GetFontStreams();
                        std::vector<SubtitleDecoder::FontDesc> fonts;
                        for (auto& stream : fontStreams)
                        {
                            SubtitleDecoder::FontDesc font;
                            font.data = (char*)stream.GetParams()->extradata;
                            font.dataSize = stream.GetParams()->extradata_size;
                            font.name = (char*)"";
                            fonts.push_back(font);
                        }
                        decoder->AddFonts(fonts);
                        _subtitleData.decoder = decoder;
                    }
                }
                packetGot += passResult;
            }
        }
        if (packetGot == 0) break;
    }

    // Get decoded frames
    while (true)
    {
        if (TimeExceeded(funcTimer, timeLimit)) break;

        // Get next frames
        if (_videoData.decoder && !_videoData.nextFrame)
        {
            _videoData.nextFrame.reset(_videoData.decoder->GetFrame().release());
        }
        if (_audioData.decoder && !_audioData.nextFrame)
        {
            _audioData.nextFrame.reset(_audioData.decoder->GetFrame().release());
        }
        //if (_subtitleData.decoder && !_subtitleData.nextFrame)
        //{
        //    _subtitleData.nextFrame.reset(_subtitleData.decoder->GetFrame().release());
        //}

        // Switch to new frames
        bool frameAdvanced = false;
        if (_videoData.nextFrame)
        {
            VideoFrame* currentFrame = (VideoFrame*)_videoData.currentFrame.get();
            VideoFrame* nextFrame = (VideoFrame*)_videoData.nextFrame.get();
            if (nextFrame->GetTimestamp() <= _playbackTimer.Now().GetTime())
            {
                _videoData.currentFrame.reset(_videoData.nextFrame.release());
                currentFrame = (VideoFrame*)_videoData.currentFrame.get();
                if (!_recovering) // Prevent ugly fast forwarding after seeking
                {
                    _videoOutputAdapter->SetVideoData(*currentFrame);
                }
                frameAdvanced = true;
            }
        }
        if (_audioData.nextFrame)
        {
            AudioFrame* currentFrame = (AudioFrame*)_audioData.currentFrame.get();
            AudioFrame* nextFrame = (AudioFrame*)_audioData.nextFrame.get();

            // Audio frames are buffered for ~500ms
            if (nextFrame->GetTimestamp() <= (_playbackTimer.Now() + Duration(100, MILLISECONDS)).GetTime())
            {
                // Reset audio playback
                if (nextFrame->First())
                {
                    std::cout << "Audio reset" << std::endl;
                    _audioOutputAdapter->Reset(nextFrame->GetChannelCount(), nextFrame->GetSampleRate());
                    _audioOutputAdapter->SetTime(_playbackTimer.Now().GetTime());
                }

                _audioData.currentFrame.reset(_audioData.nextFrame.release());
                currentFrame = (AudioFrame*)_audioData.currentFrame.get();
                // Skip late audio frames
                if (currentFrame->GetTimestamp() + currentFrame->CalculateDuration().GetDuration() >= _playbackTimer.Now().GetTime())
                {
                    _audioOutputAdapter->AddRawData(*currentFrame);
                }
                frameAdvanced = true;
            }
        }
        if (_subtitleData.decoder)
        {
            if ((_playbackTimer.Now() - _lastSubtitleRender).GetDuration(MILLISECONDS) > 50)
            {
                VideoFrame subtitle = ((SubtitleDecoder*)_subtitleData.decoder)->RenderFrame(_playbackTimer.Now());
                _videoOutputAdapter->SetSubtitleData(subtitle);
                _lastSubtitleRender = _playbackTimer.Now();
            }
        }
        if (!frameAdvanced)
        {
            if ((_videoData.nextFrame || !_videoData.decoder) &&
                (_audioData.nextFrame || !_audioData.decoder) &&
                _recovering)
            {
                _recovering = false;
                _recovered = true;
                std::cout << "Recovered\n";
            }
            break;
        }
    }
}

void MediaPlayer::StartTimer()
{
    if (_playbackTimer.Paused())
    {
        _playbackTimer.Update();
        _playbackTimer.Start();
        _audioOutputAdapter->Play();
    }
}

void MediaPlayer::StopTimer()
{
    if (!_playbackTimer.Paused())
    {
        _playbackTimer.Update();
        _playbackTimer.Stop();
        _audioOutputAdapter->Pause();
    }
}

bool MediaPlayer::TimerRunning() const
{
    return !_playbackTimer.Paused();
}

void MediaPlayer::SetTimerPosition(TimePoint time)
{
    _playbackTimer.SetTime(time);
    _lastSubtitleRender = time;
}

TimePoint MediaPlayer::TimerPosition() const
{
    return _playbackTimer.Now();
}

void MediaPlayer::SetVolume(float volume)
{
    _audioOutputAdapter->SetVolume(volume);
}

void MediaPlayer::SetBalance(float balance)
{
    _audioOutputAdapter->SetBalance(balance);
}

void MediaPlayer::SetVideoStream(std::unique_ptr<MediaStream> stream)
{
    _SetStream(_videoData, std::move(stream));
}

void MediaPlayer::SetAudioStream(std::unique_ptr<MediaStream> stream)
{
    _SetStream(_audioData, std::move(stream));
}

void MediaPlayer::SetSubtitleStream(std::unique_ptr<MediaStream> stream)
{
    _SetStream(_subtitleData, std::move(stream));
}

void MediaPlayer::_SetStream(MediaData& mediaData, std::unique_ptr<MediaStream> stream)
{
    mediaData.pendingStream = std::move(stream);
    mediaData.expectingStream = true;
}

bool MediaPlayer::Lagging() const
{
    return _lagging;
}

bool MediaPlayer::Buffering() const
{
    return _buffering;
}

bool MediaPlayer::Skipping() const
{
    return _skipping;
}

bool MediaPlayer::Recovered()
{
    bool recovered = _recovered;
    _recovered = false;
    return recovered;
}