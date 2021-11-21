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

    if (videoStream) _videoDecoder = new VideoDecoder(*videoStream);
    if (audioStream) _audioDecoder = new AudioDecoder(*audioStream);
    // Subtitles not currently supported
    //if (subtitleStream) _subtitleDecoder = new VideoDecoder(*subtitleStream);

    _playbackTimer = Clock(0);
    _playbackTimer.Stop();
}

MediaPlayer::~MediaPlayer()
{
    if (_videoDecoder) delete _videoDecoder;
    if (_audioDecoder) delete _audioDecoder;
    if (_subtitleDecoder) delete _subtitleDecoder;
}

bool MediaPlayer::_PassPacket(IMediaDecoder* decoder, MediaPacket packet)
{
    // Flush packet
    if (packet.flush)
    {
        decoder->Flush();
        StopTimer();
        return true;
    }

    if (packet.Valid())
    {
        decoder->AddPacket(std::move(packet));
        return true;
    }
    return false;
}

void MediaPlayer::Update(double timeLimit)
{
    _playbackTimer.Update();

    Clock funcTimer = Clock();

    // Sync timer to audio
    TimePoint adapterTime = TimePoint(_audioOutputAdapter->CurrentTime() + _audioOutputAdapter->TimeSinceLastBufferStart(), MICROSECONDS);
    Duration timeDelta = adapterTime - _playbackTimer.Now();
    if (std::abs(timeDelta.GetDuration(MILLISECONDS)) > 50)
    {
        _playbackTimer.AdvanceTime(timeDelta);
    }

    // Read and pass packets to decoders
    while (true)
    {
        if (TimeExceeded(funcTimer, timeLimit)) break;

        // If this is false, the loop is broken
        bool packetGot = false;
        if (_videoDecoder && !_videoDecoder->PacketQueueFull() && !_videoDecoder->Flushing())
        {
            packetGot |= _PassPacket(_videoDecoder, _dataProvider->GetVideoPacket());
        }
        if (_audioDecoder && !_audioDecoder->PacketQueueFull() && !_audioDecoder->Flushing())
        {
            packetGot |= _PassPacket(_audioDecoder, _dataProvider->GetAudioPacket());
        }
        if (_subtitleDecoder && !_subtitleDecoder->PacketQueueFull() && !_subtitleDecoder->Flushing())
        {
            packetGot |= _PassPacket(_subtitleDecoder, _dataProvider->GetSubtitlePacket());
        }
        if (!packetGot) break;
    }

    // Get decoded frames
    while (true)
    {
        if (TimeExceeded(funcTimer, timeLimit)) break;

        if (_videoDecoder && !_nextVideoFrame)
        {
            _nextVideoFrame.reset((VideoFrame*)(_videoDecoder->GetFrame().release()));
        }
        if (_audioDecoder && !_nextAudioFrame)
        {
            _nextAudioFrame.reset((AudioFrame*)(_audioDecoder->GetFrame().release()));
        }

        bool frameAdvanced = false;
        if (_nextVideoFrame)
        {
            if (_nextVideoFrame->GetTimestamp() <= _playbackTimer.Now().GetTime())
            {
                _currentVideoFrame.reset(_nextVideoFrame.release());
                _videoOutputAdapter->SetVideoData(*_currentVideoFrame);
                frameAdvanced = true;
            }
        }
        if (_nextAudioFrame)
        {
            // Audio frames are buffered for up to 500ms
            if (_nextAudioFrame->GetTimestamp() <= (_playbackTimer.Now() + Duration(500, MILLISECONDS)).GetTime())
            {
                _currentAudioFrame.reset(_nextAudioFrame.release());
                _audioOutputAdapter->AddRawData(*_currentAudioFrame);
                frameAdvanced = true;
            }
        }
        if (!frameAdvanced) break;
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

bool MediaPlayer::Lagging() const
{
    return _lagging;
}

bool MediaPlayer::Buffering() const
{
    return _buffering;
}