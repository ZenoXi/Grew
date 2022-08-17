#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>
#include "ChiliWin.h"
#include <Mmsystem.h>
#include <bitset>

//#pragma comment( lib,"xaudio2.lib" )
//#include <xaudio2.h>

#include "Options.h"
#include "NetBase2.h"
#include "ResourceManager.h"
#include "FileDialog.h"
#include "DisplayWindow.h"
#include "MediaDecoder.h"
#include "MediaPlayerOld.h"
#include "App.h"
#include "EntryScene.h"
#include "Network.h"

#include "Event.h"

#pragma comment( lib,"Winmm.lib" )

int WINAPI main(HINSTANCE hInst, HINSTANCE, LPWSTR pArgs, INT)
{
    // Initialize WSA 
    znet::WSAHolder holder = znet::WSAHolder();

    // Create window
    DisplayWindow window(hInst, pArgs, L"class");

    // Load resources
    ResourceManager::Init("Resources/Images/resources.resc", window.gfx.GetDeviceContext());

    // Generate beep
    //int samples = 4096;
    //uchar* beep = new uchar[samples * 4];
    //for (int i = 0; i < samples; i++)
    //{
    //    short s = (i % 16 < 8) ? 500 : -500;
    //    *(short*)(beep + i * 4) = s;
    //    *(short*)(beep + i * 4 + 2) = s;
    //}

    // Update clock
    ztime::clock[CLOCK_GAME] = Clock();
    ztime::clock[CLOCK_GAME].Stop();

    // Initialize singleton objects

    znet::Network::Init();
    Options::Init();
    App::Init(window, EntryScene::StaticName());
    //App app(window, "entry_scene");

    //Options::Instance()->SetValue("volume", "0.2");
    //Options::Instance()->SetValue("lastIp", "193.219.91.103:7099");

    Clock msgTimer = Clock(0);

    // Main window loop
    while (true)
    {
        // Messages
        bool msgProcessed = window.ProcessMessages();
        WindowMessage wm = window.GetExitResult();
        if (!wm.handled) exit(0);

        window.HandleFullscreenChange();
        window.HandleCursorVisibility();

        // Limit cpu usage
        if (!msgProcessed)
        {
            // If no messages are received for 50ms or more, sleep to limit cpu usage.
            // This way we allow for full* mouse poll rate utilization when necessary.
            //
            // * the very first mouse move after a break will have a very small delay
            // which may be noticeable in certain situations (FPS games)
            msgTimer.Update();
            if (msgTimer.Now().GetTime(MILLISECONDS) >= 50)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
        {
            msgTimer.Reset();
        }
        continue;

#if 0
        ztime::clock[CLOCK_GAME].Update();
        ztime::clock[CLOCK_MAIN].Update();
        //if ((ztime::Game() - playbackStartTime).GetDuration(SECONDS) > 25) exit(0);

        player->Update();
        //if (!player->Loading() && !started)
        //{
        //    player->Play();
        //    std::cout << "Playback started.\n";
        //    started = true;
        //}

        // Check for fullscreen key
        if (ButtonPressed(fClicked, 'F'))
        {

        }

        // Check for pause key
        if (ButtonPressed(spaceClicked, VK_SPACE))
        {
            std::cout << "Spacebar clicked.\n";
            if (player->Paused())
            {
                player->Play(true);
                playButton->SetPaused(false);
            }
            else
            {
                player->Pause(true);
                playButton->SetPaused(true);
            }
        }
        if (player->Paused() != playButton->GetPaused())
        {
            if (playButton->Clicked())
            {
                std::cout << "Pause synced.\n";
                if (playButton->GetPaused())
                {
                    player->Pause(true);
                }
                else
                {
                    player->Play(true);
                }
            }
            else
            {
                playButton->SetPaused(player->Paused());
            }
        }

        // Check for seek commands
        TimePoint seekTo = seekBar->SeekTime();
        if (seekTo.GetTime() != -1)
        {
            player->Seek(seekTo, true);
            //player->Seek(20000000, true);
        }
        else
        {
            bool seekBack = ButtonPressed(leftClicked, VK_LEFT);
            bool seekForward = ButtonPressed(rightClicked, VK_RIGHT);
            if (seekBack || seekForward)
            {
                if (seekBack)
                {
                    player->Seek(ztime::Game() - Duration(5, SECONDS), true);
                }
                else if (seekForward)
                {
                    player->Seek(ztime::Game() + Duration(5, SECONDS), true);
                }
            }
        }

        // Check for volume commands
        if (ButtonPressed(upClicked, VK_UP))
        {
            float newVolume = player->GetVolume() + 0.05f;
            player->SetVolume(newVolume);
            volumeSlider->SetVolume(newVolume);
        }
        if (ButtonPressed(downClicked, VK_DOWN))
        {
            float newVolume = player->GetVolume() - 0.05f;
            player->SetVolume(newVolume);
            volumeSlider->SetVolume(newVolume);
        }
        if (volumeSlider->GetVolume() != player->GetVolume())
        {
            player->SetVolume(volumeSlider->GetVolume());
        }

        // Update buffered time in seekbar
        Duration bDuration = player->GetBufferedDuration();
        if (bDuration.GetDuration() != -1)
        {
            seekBar->SetBufferedDuration(bDuration);
        }

        //if (loading)
        //{
        //    if (!decoder.Buffering())
        //    {
        //        loading = false;

        //        // Buffer audio
        //        bufferedDuration = 0;
        //        while (bufferedDuration < 0.25)
        //        {
        //            ad = decoder.GetAudioFrameData();
        //            if (ad.GetSampleRate() == 0) break;

        //            bufferedDuration += ad.GetSampleCount() / (double)ad.GetSampleRate();
        //            AudioData::WaveHeader header = ad.GetHeader();
        //            char* dataBytes = new char[header.subChunk2Size];
        //            memcpy(dataBytes, ad.GetBytes(), header.subChunk2Size);
        //            XAUDIO2_BUFFER buffer = { 0 };
        //            buffer.AudioBytes = header.subChunk2Size;
        //            buffer.pAudioData = (uchar*)dataBytes;
        //            buffer.Flags = XAUDIO2_END_OF_STREAM;
        //            hr = pSourceVoice->SubmitSourceBuffer(&buffer);
        //        }

        //        // Skip frames
        //        fd = decoder.GetVideoFrameData();
        //        while ((fd.GetTimestamp() + (double)vinfo.framerate.second / vinfo.framerate.first) < syncTime.GetTime())
        //        {
        //            fd = decoder.GetVideoFrameData();
        //        }

        //        // Resume playback
        //        pSourceVoice->Start();
        //        ztime::clock[CLOCK_GAME] = Clock(syncTime.GetTime());
        //        audioBufferEnd = ztime::Game() + Duration(bufferedDuration * 1000000.0);
        //    }
        //}

        //if (!loading)
        //{
        //    // Keep audio buffered >=250ms in advance
        //    TimePoint targetAudioBufferEnd = ztime::Game() + Duration(250, MILLISECONDS);
        //    while (targetAudioBufferEnd > audioBufferEnd)
        //    {
        //        ad = decoder.GetAudioFrameData();
        //        if (ad.GetSampleRate() == 0) break;

        //        audioBufferEnd += Duration(ad.GetSampleCount() / (double)ad.GetSampleRate() * 1000000.0);
        //        AudioData::WaveHeader header = ad.GetHeader();
        //        char* dataBytes = new char[header.subChunk2Size];
        //        memcpy(dataBytes, ad.GetBytes(), header.subChunk2Size);
        //        XAUDIO2_BUFFER buffer = { 0 };
        //        buffer.AudioBytes = header.subChunk2Size;
        //        buffer.pAudioData = (uchar*)dataBytes;
        //        buffer.Flags = XAUDIO2_END_OF_STREAM;
        //        hr = pSourceVoice->SubmitSourceBuffer(&buffer);
        //    }

        //    // Seek to new frame
        //    while (ztime::Game().GetTime() > (fd.GetTimestamp() + (double)vinfo.framerate.second / vinfo.framerate.first))
        //    {
        //        fd = decoder.GetVideoFrameData();
        //    }
        //}

        window.gfx.BeginFrame();

        // Show video frame
        const FrameData* fd = player->GetCurrentFrame();
        ID2D1Bitmap* frame = window.gfx.CreateBitmap(fd->GetWidth(), fd->GetHeight());
        D2D1_RECT_U rect = D2D1::RectU(0, 0, fd->GetWidth(), fd->GetHeight());
        frame->CopyFromMemory(&rect, fd->GetBytes(), fd->GetWidth() * 4);
        window.gfx.DrawBitmap(
            D2D1::RectF(0.0f, 0.0f, fd->GetWidth(), fd->GetHeight()),
            D2D1::RectF(0.0f, 0.0f, window.width, window.height),
            frame
        );

        // Draw Controls
        //BrushInfo bi = BrushInfo(D2D1::ColorF::White);
        //bi.size = 2.0f;
        //window.gfx.DrawLine({ 0.0f, 2.0f }, { 5.0f, 2.0f }, bi);

        componentCanvas.Update();
        ID2D1Bitmap* bitmap = componentCanvas.Draw(window.gfx.GetGraphics());
        window.gfx.GetDeviceContext()->DrawBitmap(bitmap);

        window.gfx.EndFrame();

        frame->Release();
#endif
    }

    return 0;

#if 0
    AVFormatContext* avf_context = avformat_alloc_context();
    if (!avf_context) return 1000;
    if (avformat_open_input(&avf_context, "E:\\aots2e12comp.m4v", NULL, NULL) != 0) return 1001;
    //if (avformat_open_input(&avf_context, "E:\\aots4e5comp2.m4v", NULL, NULL) != 0) return 1001;

    int video_stream_index = -1;
    int audio_stream_index = -1;
    double fps = 0;
    AVCodecParameters* avc_params_video = nullptr;
    AVCodec* av_codec_video = nullptr;
    AVCodecParameters* avc_params_audio = nullptr;
    AVCodec* av_codec_audio = nullptr;

    // Find video stream
    for (int i = 0; i < avf_context->nb_streams; i++)
    {
        AVStream* stream = avf_context->streams[i];
        avc_params_video = stream->codecpar;
        av_codec_video = avcodec_find_decoder(avc_params_video->codec_id);
        if (!av_codec_video)
        {
            continue;
        }

        if (av_codec_video->type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1)
        {
            video_stream_index = i;
            fps = av_q2d(avf_context->streams[video_stream_index]->r_frame_rate);
            break;
        }
    }

    // Find audio stream
    for (int i = 0; i < avf_context->nb_streams; i++)
    {
        AVStream* stream = avf_context->streams[i];
        avc_params_audio = stream->codecpar;
        av_codec_audio = avcodec_find_decoder(avc_params_audio->codec_id);
        if (!av_codec_audio)
        {
            continue;
        }

        if (av_codec_audio->type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1)
        {
            audio_stream_index = i;
            break;
        }
    }

    if (!av_codec_video)
    {
        return 1003;
    }
    if (video_stream_index == -1)
    {
        return 1004;
    }

    AVCodecContext* avc_context_audio = avcodec_alloc_context3(av_codec_audio);
    avcodec_parameters_to_context(avc_context_audio, avc_params_audio);
    avcodec_open2(avc_context_audio, av_codec_audio, NULL);

    AVFrame* av_frame_audio = av_frame_alloc();
    AVPacket* av_packet_audio = av_packet_alloc();

    std::vector<char*> audio_packets;
    std::vector<int> audio_packet_sizes;

    while (av_read_frame(avf_context, av_packet_audio) >= 0)
    {
        if (av_packet_audio->stream_index != audio_stream_index)
        {
            continue;
        }

        int response = avcodec_send_packet(avc_context_audio, av_packet_audio);
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            return 1005;
        }
        response = avcodec_receive_frame(avc_context_audio, av_frame_audio);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            return 1006;
        }

        int data_size = av_get_bytes_per_sample(avc_context_audio->sample_fmt);
        if (data_size < 0)
        {
            // This should not occur, checking just for paranoia
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }

        int chunkSize = av_frame_audio->nb_samples * avc_context_audio->channels * 2;
        char* audio_data = new char[chunkSize];
        int curPos = 0;

        for (int i = 0; i < av_frame_audio->nb_samples; i++)
        {
            for (int ch = 0; ch < avc_context_audio->channels; ch++)
            {
                float valf = *(float*)(av_frame_audio->data[ch] + data_size * i);
                int16_t vali = (int16_t)(valf * 32767.0f);
                *(int16_t*)(audio_data + curPos) = vali;
                curPos += 2;
            }
        }

        audio_packets.push_back(audio_data);
        audio_packet_sizes.push_back(chunkSize);

        av_packet_unref(av_packet_audio);
    }

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

    WaveHeader header;
    header.chunkID = 0x46464952;
    header.format = 0x45564157;
    header.subChunk1ID = 0x20746d66;
    header.subChunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = avc_context_audio->channels;
    header.sampleRate = avc_context_audio->sample_rate;
    header.byteRate = header.sampleRate * header.numChannels * 2;
    header.blockAlign = header.numChannels * 2;
    header.bitsPerSample = 2 * 8;
    header.subChunk2ID = 0x61746164;
    int headerSize = 44;

    int finalSize = 0;
    for (int i = 0; i < audio_packet_sizes.size(); i++)
    {
        finalSize += audio_packet_sizes[i];
    }
    header.subChunk2Size = finalSize;
    header.chunkSize = 4 + (8 + header.subChunk1Size) + (8 + header.subChunk2Size);

    char* wav_bytes = new char[finalSize + headerSize];
    memcpy(wav_bytes, &header, headerSize);
    int curPos = headerSize;
    for (int i = 0; i < audio_packets.size(); i++)
    {
        memcpy(wav_bytes + curPos, audio_packets[i], audio_packet_sizes[i]);
        curPos += audio_packet_sizes[i];
        delete[] audio_packets[i];
    }

    std::ofstream out("E:\\test.wav", std::ios::binary);
    out.write(wav_bytes, finalSize + headerSize);
    out.close();

    PlaySoundA(wav_bytes, NULL, SND_MEMORY);
    PlaySoundA(wav_bytes, NULL, SND_MEMORY);

    AVCodecContext* avc_context_video = avcodec_alloc_context3(av_codec_video);
    avcodec_parameters_to_context(avc_context_video, avc_params_video);
    avcodec_open2(avc_context_video, av_codec_video, NULL);

    AVFrame* av_frame = av_frame_alloc();
    AVPacket* av_packet = av_packet_alloc();

    SwsContext* sws_context = NULL;

    uchar* data = new uchar[avc_context_video->width * avc_context_video->height * 4];
    uchar* dest[4] = { data, NULL, NULL, NULL };
    int dest_linesize[4] = { avc_context_video->width * 4, 0, 0, 0 };

    std::vector<FrameData> frames;

    int frameCounter = 0;
    while (av_read_frame(avf_context, av_packet) >= 0 && frameCounter < 1000000)
    {
        if (av_packet->stream_index != video_stream_index)
        {
            continue;
        }

        //avcodec_decode_audio4()

        int response = avcodec_send_packet(avc_context_video, av_packet);
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            return 1005;
        }
        response = avcodec_receive_frame(avc_context_video, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            return 1006;
        }

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
            if (!sws_context)
            {
                return 1007;
            }
        }
        sws_scale(sws_context, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);
        FrameData fd = FrameData(av_frame->width, av_frame->height, 0);
        fd.SetBytes(data);
        frames.push_back(std::move(fd));

        if (!fd.GetBytes() && frameCounter % 100 == 0)
        {
            std::cout << "nullptr set (" << frameCounter << ")" << std::endl;
        }

        av_packet_unref(av_packet);

        frameCounter++;
    }

    sws_freeContext(sws_context);

    //DisplayWindow window(hInst, pArgs, L"class");

    ztime::clock[CLOCK_MAIN].Update();
    TimePoint startTime = ztime::Main();
    for (int i = 0; i < frames.size(); i++)
    {
        // Render frame
        window.gfx.BeginFrame();

        D2D1_BITMAP_PROPERTIES props;
        props.dpiX = 96.0f;
        props.dpiY = 96.0f;
        props.pixelFormat = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        );
        ID2D1Bitmap* frame;
        window.gfx.GetDeviceContext()->CreateBitmap(
            D2D1::SizeU(frames[i].GetWidth(), frames[i].GetHeight()),
            props,
            &frame
        );
        D2D1_RECT_U rect = D2D1::RectU(0, 0, frames[i].GetWidth(), frames[i].GetHeight());
        if (frame->CopyFromMemory(&rect, frames[i].GetBytes(), frames[i].GetWidth() * 4) != S_OK) { return 1006; }
        window.gfx.GetDeviceContext()->DrawBitmap(
            frame,
            D2D1::RectF(0.0f, 0.0f, window.width, window.height),
            1.0f,
            D2D1_INTERPOLATION_MODE_CUBIC,
            D2D1::RectF(0.0f, 0.0f, frames[i].GetWidth(), frames[i].GetHeight())
        );

        frame->Release();

        // Wait for time to display
        using namespace std::chrono;
        double frameStartTimeUs = 1000000.0 / fps * i;
        ztime::clock[CLOCK_MAIN].Update();
        double currentTimeUs = (ztime::Main() - startTime).GetDuration();
        double sleepTime = frameStartTimeUs - currentTimeUs;
        std::this_thread::sleep_for(microseconds((long)sleepTime));

        window.gfx.EndFrame();
    }

    int k;
    std::cin >> k;

    avformat_close_input(&avf_context);
    avformat_free_context(avf_context);
    av_frame_free(&av_frame);
    av_packet_free(&av_packet);
    avcodec_free_context(&avc_context_video);

    std::cout << "helo" << std::endl;
    return 0;
#endif
}