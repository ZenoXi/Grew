#include "MediaPlayerOld.h"

#include <iostream>

MediaPlayerOld::MediaPlayerOld(std::string ip, USHORT port) : _currentFrame(0, 0, -1), _skippedFrame(0, 0, -1), _voiceCallback(_audioBufferLength)
{
    _mode = PlaybackMode::CLIENT;
    //_clientBuffering = false;
    //_waitingForServer = true;
    //_waitingForPackets = false;
    //cAudioOut.open("client.data", std::ios::binary);

    // Init network
    _network = NetworkInterfaceOld::Instance();
    _network->Connect(ip, port);
    while (_network->Connected() == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (_network->Connected() < 0)
    {
        std::cout << "Connection failed. Exiting...";
        exit(-1);
    }

    // Wait for decoder data
    while (!_network->MetadataReceived())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Init decoder
    _decoder = new MediaDecoder(_network->GetVideoParams(), _network->GetAudioParams());
    _decoder->SetMetadata(_network->GetDuration(), _network->GetFramerate(), _network->GetVideoTimebase(), _network->GetAudioTimebase());

    // Get media info
    _vinfo = _decoder->GetVideoInfo();
    _ainfo = _decoder->GetAudioInfo();

    // Set current frame to a black screen
    _currentFrame = FrameData(_vinfo.width, _vinfo.height, 0);
    _currentFrame.ClearBytes();

    // Start decoding
    _decoder->StartDecoding();

    // Init XAudio
    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    hr = XAudio2Create(&_XAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    hr = _XAudio2->CreateMasteringVoice(&_masterVoice);

    _wfx.wFormatTag = 1;
    _wfx.nChannels = _ainfo.channels;
    _wfx.nSamplesPerSec = _ainfo.sampleRate;
    _wfx.nAvgBytesPerSec = _wfx.nChannels * _wfx.nSamplesPerSec * 2;
    _wfx.nBlockAlign = _wfx.nChannels * 2;
    _wfx.wBitsPerSample = 2 * 8;

    hr = _XAudio2->CreateSourceVoice(&_sourceVoice, &_wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_voiceCallback, NULL, NULL);
    SetVolume(0.05f);

    // Set initial variables
    _loading = true;
    _paused = true;
    _waiting = false;
    ztime::clock[CLOCK_GAME] = Clock(0);
    ztime::clock[CLOCK_GAME].Stop();
}

MediaPlayerOld::MediaPlayerOld(USHORT port, std::string filename) : _currentFrame(0, 0, -1), _skippedFrame(0, 0, -1), _voiceCallback(_audioBufferLength)
{
    // Init decoder
    if (port == 0)
    {
        _mode = PlaybackMode::OFFLINE;
        _decoder = new MediaDecoder(filename);
        //_clientBuffering = false;
        //_waitingForServer = false;
        //_waitingForPackets = false;
        _waiting = false;

        // Init network in offline mode
        _network = NetworkInterfaceOld::Instance();
        //_network = new NetworkInterface(false);
    }
    else
    {
        _mode = PlaybackMode::SERVER;
        _decoder = new MediaDecoder(filename, true);
        //_clientBuffering = true;
        //_waitingForServer = false;
        //_waitingForPackets = false;
        _waiting = true;

        // Init network
        _network = NetworkInterfaceOld::Instance();
        _network->StartServer(port);

        // Wait for a connection
        while (!_network->ClientConnected())
        //while (_network->Status() != NetworkStatus::CONNECTED)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::cout << "Client connected.\n";

        // Send decoder data
        _network->SendVideoParams(_decoder->GetVideoCodecParamData());
        _network->SendAudioParams(_decoder->GetAudioCodecParamData());
        std::pair<int, int> framerate = _decoder->GetVideoInfo().framerate;
        _network->SendMetadata(
            _decoder->GetDuration(),
            { framerate.first, framerate.second },
            _decoder->GetVideoTimebase(),
            _decoder->GetAudioTimebase()
        );
        std::cout << "Metadata sent.\n";
    }

    // Get media info
    _vinfo = _decoder->GetVideoInfo();
    _ainfo = _decoder->GetAudioInfo();

    // Set current frame to a black screen
    _currentFrame = FrameData(_vinfo.width, _vinfo.height, 0);
    _currentFrame.ClearBytes();

    // Start decoding
    _decoder->StartDecoding();

    // Init XAudio
    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    hr = XAudio2Create(&_XAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
    hr = _XAudio2->CreateMasteringVoice(&_masterVoice);

    _wfx.wFormatTag = 1;
    _wfx.nChannels = _ainfo.channels;
    _wfx.nSamplesPerSec = _ainfo.sampleRate;
    _wfx.nAvgBytesPerSec = _wfx.nChannels * _wfx.nSamplesPerSec * 2;
    _wfx.nBlockAlign = _wfx.nChannels * 2;
    _wfx.wBitsPerSample = 2 * 8;

    hr = _XAudio2->CreateSourceVoice(&_sourceVoice, &_wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_voiceCallback, NULL, NULL);
    SetVolume(0.05f);

    // Set initial variables
    _loading = true;
    _paused = true;
    ztime::clock[CLOCK_GAME] = Clock(0);
    ztime::clock[CLOCK_GAME].Stop();
}

MediaPlayerOld::MediaPlayerOld(PlaybackMode mode, std::string filename) : _currentFrame(0, 0, -1), _skippedFrame(0, 0, -1), _voiceCallback(_audioBufferLength)
{
    _mode = mode;
    _network = NetworkInterfaceOld::Instance();

    if (mode == PlaybackMode::CLIENT)
    {
        // Wait for decoder data
        while (!_network->MetadataReceived())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Init decoder
        _decoder = new MediaDecoder(_network->GetVideoParams(), _network->GetAudioParams());
        _decoder->SetMetadata(_network->GetDuration(), _network->GetFramerate(), _network->GetVideoTimebase(), _network->GetAudioTimebase());

        // Get media info
        _vinfo = _decoder->GetVideoInfo();
        _ainfo = _decoder->GetAudioInfo();

        // Set current frame to a black screen
        _currentFrame = FrameData(_vinfo.width, _vinfo.height, 0);
        _currentFrame.ClearBytes();

        // Start decoding
        _decoder->StartDecoding();

        // Init XAudio
        HRESULT hr;
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        hr = XAudio2Create(&_XAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
        hr = _XAudio2->CreateMasteringVoice(&_masterVoice);

        _wfx.wFormatTag = 1;
        _wfx.nChannels = _ainfo.channels;
        _wfx.nSamplesPerSec = _ainfo.sampleRate;
        _wfx.nAvgBytesPerSec = _wfx.nChannels * _wfx.nSamplesPerSec * 2;
        _wfx.nBlockAlign = _wfx.nChannels * 2;
        _wfx.wBitsPerSample = 2 * 8;

        hr = _XAudio2->CreateSourceVoice(&_sourceVoice, &_wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_voiceCallback, NULL, NULL);
        SetVolume(0.05f);

        // Set initial variables
        _loading = true;
        _paused = true;
        _waiting = false;
        ztime::clock[CLOCK_GAME] = Clock(0);
        ztime::clock[CLOCK_GAME].Stop();
    }
    else
    {
        if (mode == PlaybackMode::OFFLINE)
        {
            _decoder = new MediaDecoder(filename);
            _waiting = false;
        }
        else if (mode == PlaybackMode::SERVER)
        {
            _decoder = new MediaDecoder(filename, true);
            _waiting = true;

            // Send decoder data
            _network->SendVideoParams(_decoder->GetVideoCodecParamData());
            _network->SendAudioParams(_decoder->GetAudioCodecParamData());
            std::pair<int, int> framerate = _decoder->GetVideoInfo().framerate;
            _network->SendMetadata(
                _decoder->GetDuration(),
                { framerate.first, framerate.second },
                _decoder->GetVideoTimebase(),
                _decoder->GetAudioTimebase()
            );
            std::cout << "Metadata sent.\n";
        }

        // Get media info
        _vinfo = _decoder->GetVideoInfo();
        _ainfo = _decoder->GetAudioInfo();

        // Set current frame to a black screen
        _currentFrame = FrameData(_vinfo.width, _vinfo.height, 0);
        _currentFrame.ClearBytes();

        // Start decoding
        _decoder->StartDecoding();

        // Init XAudio
        HRESULT hr;
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        hr = XAudio2Create(&_XAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
        hr = _XAudio2->CreateMasteringVoice(&_masterVoice);

        _wfx.wFormatTag = 1;
        _wfx.nChannels = _ainfo.channels;
        _wfx.nSamplesPerSec = _ainfo.sampleRate;
        _wfx.nAvgBytesPerSec = _wfx.nChannels * _wfx.nSamplesPerSec * 2;
        _wfx.nBlockAlign = _wfx.nChannels * 2;
        _wfx.wBitsPerSample = 2 * 8;

        hr = _XAudio2->CreateSourceVoice(&_sourceVoice, &_wfx);
        SetVolume(0.05f);

        // Set initial variables
        _loading = true;
        _paused = true;
        ztime::clock[CLOCK_GAME] = Clock(0);
        ztime::clock[CLOCK_GAME].Stop();
    }
}

//bool MediaPlayer::Initializing()
//{
//    return _initializing;
//}

void MediaPlayerOld::Update(double timeLimit)
{
    Clock frameTimer = Clock();

    if (_flushing)
    {
        //std::cout << "Flushing\n";
        if (!_decoder->Flushing())
        {
            _flushing = false;
            std::cout << "Flush finished\n";
        }
    }

    // Send current position
    if (_mode != PlaybackMode::OFFLINE)
    {
        if ((ztime::Game() - _lastPositionNotification).GetDuration(MILLISECONDS) > 100)
        {
            _lastPositionNotification = ztime::Game();
            //_network->SendCurrentPosition(ztime::Game());
        }
    }

    // Process incoming packets
    if (_mode == PlaybackMode::CLIENT)
    {
        {
            //std::unique_ptr<char> vPacket;
            //std::unique_ptr<char> aPacket;
            //while ((vPacket = _network->GetVideoPacket()).get() || (aPacket = _network->GetAudioPacket()).get())
            //{
            //    if (vPacket.get())
            //    {
            //        _decoder->AddVideoPacket((uchar*)vPacket.get());
            //    }
            //    if (aPacket.get())
            //    {
            //        //cAudioOut.write(aPacket.get(), _network->lastSize);
            //        _decoder->AddAudioPacket((uchar*)aPacket.get());
            //    }

            //    // Cap frame time
            //    frameTimer.Update();
            //    if (frameTimer.GetTime() / 1000000.0 > timeLimit)
            //    {
            //        break;
            //    }
            //}

            //while (true)
            //{
            //    // Cap frame time
            //    frameTimer.Update();
            //    if (frameTimer.GetTime() / 1000000.0 > timeLimit)
            //    {
            //        break;
            //    }

            //    std::unique_ptr<char> vPacket = nullptr;
            //    std::unique_ptr<char> aPacket = nullptr;

            //    if (!(vPacket = _network->GetVideoPacket()).get() && !(aPacket = _network->GetAudioPacket()).get()) break;
            //    if (vPacket)
            //    {
            //        _decoder->AddVideoPacket((uchar*)vPacket.get());
            //    }
            //    if (aPacket)
            //    {
            //        //cAudioOut.write(aPacket.get(), _network->lastSize);
            //        _decoder->AddAudioPacket((uchar*)aPacket.get());
            //    }
            //}
        }

        while (!_flushing)
        {
            // Cap frame time
            frameTimer.Update();
            if (frameTimer.Now().GetTime() / 1000000.0 > timeLimit)
            {
                break;
            }

            std::unique_ptr<char> vPacket = _network->GetVideoPacket();
            std::unique_ptr<char> aPacket = _network->GetAudioPacket();
            if (!vPacket && !aPacket) break;
            if (vPacket)
            {
                _decoder->AddVideoPacket((uchar*)vPacket.get());
            }
            if (aPacket)
            {
                //cAudioOut.write(aPacket.get(), _network->lastSize);
                _decoder->AddAudioPacket((uchar*)aPacket.get());
            }
        }

        znetold::Packet p;
        while ((p = _network->GetPacket()).size != -1)
        {
            switch (p.packetId)
            {
            case packet::PAUSE:
            {
                std::cout << "Pause request received.\n";
                if (_waiting)
                {
                    _waiting = false;
                }
                Pause();
                break;
            }
            case packet::RESUME:
            {
                std::cout << "Resume request received.\n";
                if (_waiting)
                {
                    _waiting = false;
                    std::cout << "Wait finished";
                    if (!Paused())
                    {
                        std::cout << ", resuming";
                        ztime::clock[CLOCK_GAME].Update();
                        ztime::clock[CLOCK_GAME].Start();
                        _sourceVoice->Start();
                    }
                    std::cout << ".\n";
                }
                else
                {
                    Play();
                }
                break;
            }
            case packet::SEEK:
            {
                TimePoint position = p.Cast<TimePoint>();
                std::cout << "Seek request received.\n";
                Seek(position);
                break;
            }
            default:
                break;
            }

            // Cap frame time
            frameTimer.Update();
            if (frameTimer.Now().GetTime() / 1000000.0 > timeLimit)
            {
                break;
            }
        }
    }
    else if (_mode == PlaybackMode::SERVER)
    {
        znetold::Packet p;
        while ((p = _network->GetPacket()).size != -1)
        {
            switch (p.packetId)
            {
            case packet::PAUSE:
            {
                std::cout << "Pause request received.\n";
                Pause();
                break;
            }
            case packet::RESUME:
            {
                std::cout << "Resume request received.\n";
                Play();
                break;
            }
            case packet::SEEK:
            {
                TimePoint position = p.Cast<TimePoint>();
                std::cout << "Seek request received.\n";
                Seek(position);
                _network->ConfirmSeekReceived();
                break;
            }
            case packet::BUFFERING_START:
            {
                std::cout << "Client started buffering.\n";
                //_clientBuffering = true;
                ztime::clock[CLOCK_GAME].Update();
                ztime::clock[CLOCK_GAME].Stop();
                _sourceVoice->Stop();

                // Loading set to true so that when the client finishes buffering the playback resumes using the 'load finish' code
                _loading = true;
                _waiting = true;
                break;
            }
            case packet::BUFFERING_END:
            {
                std::cout << "Client finished buffering.\n";
                if (_waiting)
                {
                    std::cout << "Wait finished.\n";
                    _waiting = false;
                }
                //_clientBuffering = false;
                break;
            }
            default:
                break;
            }

            // Cap frame time
            frameTimer.Update();
            if (frameTimer.Now().GetTime() / 1000000.0 > timeLimit)
            {
                break;
            }
        }
    }

    // Send packets
    if (_mode == PlaybackMode::CLIENT)
    {

    }
    else if (_mode == PlaybackMode::SERVER)
    {
        if (!_flushing)
        {
            while (true)
            {
                FFmpeg::Result videoData = _decoder->GetVideoPacketData();
                FFmpeg::Result audioData = _decoder->GetAudioPacketData();
                if (!videoData.data && !audioData.data) break;
                if (videoData.data)
                {
                    //delete[] videoData.data;
                    _network->SendVideoPacket(videoData);
                }
                if (audioData.data)
                {
                    //if (*(char*)audioData.data == -35)
                    //{
                    //    int k = 0;
                    //    k++;
                    //}
                    //delete[] audioData.data;
                    _network->SendAudioPacket(audioData);
                }

                // Cap frame time
                frameTimer.Update();
                if (frameTimer.Now().GetTime() / 1000000.0 > timeLimit)
                {
                    break;
                }

                //if (_network->PacketsBuffered() > 100) break;
            }
        }
    }

    // Client buffer check
    if (_mode == PlaybackMode::CLIENT && !_loading && !_waiting)
    {
        if (_decoder->Buffering())
        {
            ztime::clock[CLOCK_GAME].Update();
            ztime::clock[CLOCK_GAME].Stop();
            _sourceVoice->Stop();
            
            //_network->SendPauseCommand();
            _network->NotifyBufferingStart();
            _loading = true;
            std::cout << "Buffering started.\n";
        }
        //else if (!_decoder->Buffering() && _waitingForPackets)
        //{
        //    //_network->SendResumeCommand();
        //    _network->NotifyBufferingEnd();
        //    _waitingForPackets = false;
        //    std::cout << "Buffering ended.\n";
        //}
    }

    // ////////////// TODO: FIX MEMORY LEAK HERE
    // ////////////// dataBytes NEEDS TO BE STORED AND DELETED
    // ////////////// IN A CALLBACK AFTER THE VOICE HAS PLAYED

    // Buffer audio
    constexpr int BUFFER_LENGTH = 500; // milliseconds
    if (!_decoder->Buffering())
    {
        //std::cout << "Buffer length: " << (_audioBufferEnd - ztime::Game()).GetDuration(MILLISECONDS) << std::endl;
        while ((_audioBufferEnd - ztime::Game()).GetDuration(MILLISECONDS) < BUFFER_LENGTH)
        {
            AudioData ad = _decoder->GetAudioFrameData();
            if (ad.GetSampleRate() == 0)
            {
                std::cout << "-\n";
                break;
            }

            _audioBufferEnd = ad.GetTimestamp() + (1000000LL * ad.GetSampleCount()) / ad.GetSampleRate();
            _audioFramesBuffered++;

            AudioData::WaveHeader header = ad.GetHeader();
            char* dataBytes = new char[header.subChunk2Size]; // << THIS IS NOT DELETED ################################################## M E M O R Y L E A K ##################
            memcpy(dataBytes, ad.GetBytes(), header.subChunk2Size);
            XAUDIO2_BUFFER buffer = { 0 };
            buffer.AudioBytes = header.subChunk2Size;
            buffer.pAudioData = (uchar*)dataBytes;
            buffer.Flags = XAUDIO2_END_OF_STREAM;
            _sourceVoice->SubmitSourceBuffer(&buffer);
        }
    }

    if (_loading)
    {
        if ((_audioBufferEnd - ztime::Game()).GetDuration(MILLISECONDS) >= BUFFER_LENGTH && !_decoder->Buffering())
        {
            // Skip frames
            if (!_skipFinished)
            {
                bool loopAborted = false;
                _skippedFrame = _decoder->GetVideoFrameData();
                while ((_skippedFrame.GetTimestamp() + 1000000.0 * (double)_vinfo.framerate.second / _vinfo.framerate.first) < ztime::Game().GetTime())
                {
                    // Cap frame time
                    frameTimer.Update();
                    if (frameTimer.Now().GetTime() / 1000000.0 > timeLimit)
                    {
                        std::cout << ".\n";
                        loopAborted = true;
                        break;
                    }

                    FrameData newFrame = _decoder->GetVideoFrameData();
                    if (newFrame.GetTimestamp() == -1)
                    {
                        std::cout << ".\n";
                        loopAborted = true;
                        break;
                    }

                    _skippedFrame = std::move(newFrame);
                }

                if (!loopAborted)
                {
                    _skipFinished = true;
                    _currentFrame = std::move(_skippedFrame);
                }
            }

            if (_skipFinished && !_waiting)
            {
                if (_mode != PlaybackMode::CLIENT)
                {
                    _loading = false;

                    std::cout << "Loading finished, " << _audioFramesBuffered << " audio frames buffered.\n";
                    //std::cout << _audioBufferEnd.GetTime(MILLISECONDS) << " " << ztime::Game().GetTime(MILLISECONDS) << std::endl;

                    if (!Paused())
                    {
                        // Send play command
                        _network->SendResumeCommand();
                        std::cout << "Resume command sent.\n";

                        // Start clock
                        ztime::clock[CLOCK_GAME].Update();
                        ztime::clock[CLOCK_GAME].Start();
                        _sourceVoice->Start();
                    }
                }
                else if (_mode == PlaybackMode::CLIENT)
                {
                    _loading = false;

                    std::cout << "Loading finished, " << _audioFramesBuffered << " audio frames buffered.\n";

                    _network->NotifyBufferingEnd();
                    if (!Paused())
                    {
                        _waiting = true;
                        std::cout << "Wait started.\n";
                    }

                    // Enter pause state and wait for server to send a resume command
                    //_paused = true;
                }
            }
        }
    }

    if (!_loading && !_waiting && !Paused())
    {
        // Seek to a new frame
        while (ztime::Game().GetTime() - _currentFrame.GetTimestamp() > 1000000.0 * (double)_vinfo.framerate.second / _vinfo.framerate.first)
        {
            FrameData newFrame = _decoder->GetVideoFrameData();
            if (newFrame.GetTimestamp() == -1) break;
            _currentFrame = std::move(newFrame);
        }
    }
}

void MediaPlayerOld::Play(bool echo)
{
    if (!Paused()) return;
    if (!_loading && !_waiting)
    {
        ztime::clock[CLOCK_GAME].Update();
        ztime::clock[CLOCK_GAME].Start();
        if (_sourceVoice->Start() != S_OK)
        {
            std::cout << "Playback start failed\n";
        }
    }
    std::cout << "Unpaused\n";
    _paused = false;

    if (echo)
    {
        _network->SendResumeCommand();
    }
}

void MediaPlayerOld::Pause(bool echo)
{
    if (Paused()) return;
    if (!_loading && !_waiting)
    {
        ztime::clock[CLOCK_GAME].Update();
        ztime::clock[CLOCK_GAME].Stop();
        if (_sourceVoice->Stop() != S_OK)
        {
            std::cout << "Playback stop failed\n";
            return;
        }
    }
    std::cout << "Paused\n";
    _paused = true;

    if (echo)
    {
        _network->SendPauseCommand();
    }
}

void MediaPlayerOld::Seek(TimePoint position, bool echo)
{
    if (position.GetTime() < 0) position.SetTime(0);
    if (position.GetTime() > GetDuration().GetDuration()) position.SetTime(GetDuration().GetDuration());
    _decoder->SeekTo(position);
    _flushing = true;
    if (_mode == PlaybackMode::SERVER) _waiting = true;

    if (echo)
    {
        _network->SendSeekCommand(position);
    }

    ztime::clock[CLOCK_GAME].Update();
    ztime::clock[CLOCK_GAME].Stop();
    ztime::clock[CLOCK_GAME].SetTime(position.GetTime());

    // Reset audio buffer
    _sourceVoice->Stop();
    _sourceVoice->FlushSourceBuffers();
    _sourceVoice->DestroyVoice();
    _XAudio2->CreateSourceVoice(&_sourceVoice, &_wfx);
    _sourceVoice->SetVolume(_volume);
    //std::cout << "Voice destroyed.\n";
    _audioBufferEnd = 0;
    _audioFramesBuffered = 0;
    _skipFinished = false;

    _loading = true;
}

void MediaPlayerOld::SetVolume(float volume)
{
    if (volume > 1.0f) volume = 1.0f;
    if (volume < 0.0f) volume = 0.0f;
    _volume = volume;
    _sourceVoice->SetVolume(volume);
}

bool MediaPlayerOld::Paused()
{
    return _paused;
}

Duration MediaPlayerOld::GetDuration()
{
    return _decoder->GetDuration();
}

float MediaPlayerOld::GetVolume()
{
    return _volume;
}

bool MediaPlayerOld::Buffering()
{
    return _decoder->Buffering();
}

bool MediaPlayerOld::Loading()
{
    return _loading;
}

Duration MediaPlayerOld::GetBufferedDuration()
{
    return _decoder->GetPacketBufferedDuration();
}

const FrameData* MediaPlayerOld::GetCurrentFrame()
{
    return &_currentFrame;
}