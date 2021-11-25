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

    _recovering = true;

    _playbackTimer = Clock(0);
    _playbackTimer.Stop();
}

MediaPlayer::~MediaPlayer()
{
    if (_videoDecoder) delete _videoDecoder;
    if (_audioDecoder) delete _audioDecoder;
    if (_subtitleDecoder) delete _subtitleDecoder;
}

int MediaPlayer::_PassPacket(IMediaDecoder* decoder, MediaPacket packet)
{
    // Flush packet
    if (packet.flush)
    {
        decoder->Flush();
        _recovering = true;
        _recovered = false;
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
        if (_videoDecoder && !_videoDecoder->Flushing())
        {
            if (!_videoDecoder->PacketQueueFull() || _dataProvider->FlushVideoPacketNext())
            {
                int passResult = _PassPacket(_videoDecoder, _dataProvider->GetVideoPacket());
                // If flushing, clear the stored next frame to prevent
                // frames with lower timestamps after flush getting stuck
                if (passResult == 2) _nextVideoFrame.reset(nullptr);
                packetGot += passResult;
            }
        }
        if (_audioDecoder && !_audioDecoder->Flushing())
        {
            if (!_audioDecoder->PacketQueueFull() || _dataProvider->FlushAudioPacketNext())
            {
                int passResult = _PassPacket(_audioDecoder, _dataProvider->GetAudioPacket());
                // See above
                if (passResult == 2) _nextAudioFrame.reset(nullptr);
                packetGot += passResult;
            }
        }
        if (_subtitleDecoder && !_subtitleDecoder->Flushing())
        {
            if (!_subtitleDecoder->PacketQueueFull() || _dataProvider->FlushSubtitlePacketNext())
            {
                int passResult = _PassPacket(_subtitleDecoder, _dataProvider->GetSubtitlePacket());
                // See above
                //if (passResult == 2) _nextSubtitleFrame.reset(nullptr);
                packetGot += passResult;
            }
        }
        if (!packetGot) break;
    }

    // Get decoded frames
    while (true)
    {
        if (TimeExceeded(funcTimer, timeLimit)) break;

        // Get next frames
        if (_videoDecoder && !_nextVideoFrame)
        {
            _nextVideoFrame.reset((VideoFrame*)(_videoDecoder->GetFrame().release()));
        }
        if (_audioDecoder && !_nextAudioFrame)
        {
            _nextAudioFrame.reset((AudioFrame*)(_audioDecoder->GetFrame().release()));
        }

        // Switch to new frames
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
            // Reset audio playback
            if (_nextAudioFrame->First())
            {
                _audioOutputAdapter->Reset(_nextAudioFrame->GetChannelCount(), _nextAudioFrame->GetSampleRate());
                _audioOutputAdapter->SetTime(_playbackTimer.Now().GetTime());
            }

            // Audio frames are buffered for up to 500ms
            if (_nextAudioFrame->GetTimestamp() <= (_playbackTimer.Now() + Duration(500, MILLISECONDS)).GetTime())
            {
                _currentAudioFrame.reset(_nextAudioFrame.release());
                _audioOutputAdapter->AddRawData(*_currentAudioFrame);
                frameAdvanced = true;
            }
        }
        if (!frameAdvanced)
        {
            if (_nextVideoFrame && _nextAudioFrame && _recovering)
            {
                _recovering = false;
                _recovered = true;
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