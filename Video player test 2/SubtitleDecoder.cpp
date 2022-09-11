#include "SubtitleDecoder.h"

#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"

extern "C"
{
#include <libswscale/swscale.h>
}


SubtitleDecoder::SubtitleDecoder(const MediaStream& stream)
    : _stream(stream)
{
    AVCodec* codec = avcodec_find_decoder(stream.GetParams()->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream.GetParams());
    avcodec_open2(_codecContext, codec, NULL);

    if (stream.GetParams()->codec_id == AV_CODEC_ID_ASS ||
        stream.GetParams()->codec_id == AV_CODEC_ID_SSA)
    {
        _subType = SubtitleType::ASS;
    }
    else if (stream.GetParams()->codec_id == AV_CODEC_ID_HDMV_PGS_SUBTITLE)
    {
        _subType = SubtitleType::IMAGE;
        AV_PIX_FMT_PAL8;
    }
    else if (stream.GetParams()->codec_id == AV_CODEC_ID_SUBRIP ||
        stream.GetParams()->codec_id == AV_CODEC_ID_SRT)
    {
        _subType = SubtitleType::TEXT;
    }

    if (_subType == SubtitleType::ASS)
    {
        _library = ass_library_init();
        _track = ass_new_track(_library);
        ass_process_data(_track, (char*)_stream.GetParams()->extradata, _stream.GetParams()->extradata_size);
        _ResetRenderer();
    }

    _timebase = _stream.timeBase;

    _LoadOptions();

    // Start decoding and rendering threads
    _decoderThread = std::thread(&SubtitleDecoder::_DecoderThread, this);
    if (_subType == SubtitleType::ASS)
        _renderingThread = std::thread(&SubtitleDecoder::_RenderingThread, this);
}

SubtitleDecoder::~SubtitleDecoder()
{
    _decoderThreadStop = true;
    if (_decoderThread.joinable())
        _decoderThread.join();
    if (_renderingThread.joinable())
        _renderingThread.join();
    avcodec_close(_codecContext);
    avcodec_free_context(&_codecContext);

    if (_subType == SubtitleType::ASS)
    {
        ass_free_track(_track);
        ass_renderer_done(_renderer);
        ass_library_done(_library);
    }
}

void SubtitleDecoder::_DecoderThread()
{
    Clock threadClock = Clock(0);

    while (!_decoderThreadStop)
    {
        threadClock.Update();

        // Update options
        if (threadClock.Now() > _lastOptionCheck + _optionCheckInterval)
        {
            _lastOptionCheck = threadClock.Now();
            _LoadOptions();
        }

        // Seek
        if (_decoderThreadFlush)
        {
            // Flush decoder
            //AVPacket* flushPkt = av_packet_alloc();
            //AVSubtitle sub;
            //int gotSub;
            //while (avcodec_decode_subtitle2(_codecContext, &sub, &gotSub, flushPkt) >= 0);
            //avcodec_flush_buffers(_codecContext);

            if (_subType == SubtitleType::ASS)
            {
                std::unique_lock<std::mutex> lock(_m_ass);
                ass_free_track(_track);
                _track = ass_new_track(_library);
                ass_process_data(_track, (char*)_stream.GetParams()->extradata, _stream.GetParams()->extradata_size);
                _lastRenderedFrameTime = -1;
                lock.unlock();
            }

            ClearFrames();
            ClearPackets();
            _decoderThreadFlush = false;
            continue;
        }

        // If finished, wait for seek command/new packets
        _m_packets.lock(); // Prevent packet flushing on seek
        if (_packets.empty())
        {
            _m_packets.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        MediaPacket packet = std::move(_packets.front());
        _packets.pop();
        _m_packets.unlock();

        //// Cut UTF-8 characters, because the decoder really doesnt like them for some reason
        //for (int i = 0; i < packet.GetPacket()->size; i++)
        //{
        //    packet.GetPacket()->data[i] %= 128;
        //}

        int64_t timestamp = av_rescale_q(packet.GetPacket()->pts, _timebase, { 1, AV_TIME_BASE });
        int64_t duration = av_rescale_q(packet.GetPacket()->duration, _timebase, { 1, AV_TIME_BASE });

        if (_subType == SubtitleType::IMAGE)
        {
            // Decode
            AVSubtitle sub;
            int gotSub;
            int bytesUsed = avcodec_decode_subtitle2(_codecContext, &sub, &gotSub, packet.GetPacket());
            if (!gotSub || bytesUsed < 0)
                continue;

            // Create output frame
            int width, height;
            if (_outputWidth != 0 && _outputHeight != 0)
            {
                width = _outputWidth;
                height = _outputHeight;
            }
            else
            {
                width = 1280;
                height = 720;
            }
            VideoFrame* vf = new VideoFrame(width, height, timestamp);
            std::fill_n((uchar*)vf->GetBytes(), vf->GetWidth() * vf->GetHeight() * 4, 0);

            // Copy rects to frame
            for (int i = 0; i < sub.num_rects; i++)
            {
                AVSubtitleRect* rect = sub.rects[i];

                // Create sub frame
                VideoFrame* sf = new VideoFrame(rect->w, rect->h, 0);

                // Convert
                SwsContext* swsContext = sws_getContext(
                    rect->w,
                    rect->h,
                    _codecContext->pix_fmt,
                    rect->w,
                    rect->h,
                    AV_PIX_FMT_BGRA,
                    SWS_FAST_BILINEAR,
                    NULL,
                    NULL,
                    NULL
                );

                uchar* dest[4] = { NULL, NULL, NULL, NULL };
                int destLinesize[4] = { 0, 0, 0, 0 };
                dest[0] = (uchar*)sf->GetBytes();
                destLinesize[0] = rect->w * 4;

                sws_scale(swsContext, rect->data, rect->linesize, 0, rect->h, dest, destLinesize);
                sws_freeContext(swsContext);

                // Copy contents from subtitle frame to video frame
                for (int y = rect->y; y < rect->y + rect->h; y++)
                {
                    for (int x = rect->x; x < rect->x + rect->w; x++)
                    {
                        if (x < 0 || x >= vf->GetWidth())
                            continue;
                        if (y < 0 || y >= vf->GetHeight())
                            continue;
                        vf->SetPixel(x, y, sf->GetPixel(x - rect->x, y - rect->y));
                    }
                }
                delete sf;
            }

            avsubtitle_free(&sub);

            std::lock_guard<std::mutex> lock(_m_frames);
            _frames.push((IMediaFrame*)vf);
        }
        else if (_subType == SubtitleType::ASS)
        {
            // Pass to renderer
            std::unique_lock<std::mutex> lock(_m_ass);
            ass_process_chunk(_track, (char*)packet.GetPacket()->data, packet.GetPacket()->size, timestamp / 1000, duration / 1000);
            lock.unlock();

            if (_lastRenderedFrameTime == -1)
                _lastRenderedFrameTime = TimePoint(timestamp, MICROSECONDS) - _timeBetweenFrames;
            _lastBufferedSubtitleTime = TimePoint(timestamp + duration, MICROSECONDS);
        }

        //for (int i = 0; i < sub.num_rects; i++)
        //{
        //    ass_process_chunk(_track, sub.rects[i]->ass, strlen(sub.rects[i]->ass), timestamp / 1000, duration / 1000);
        //}
    }
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

// fg - foreground, bg - background
PixelData BlendPixel(PixelData fg, PixelData bg)
{
    // nvm this blending sucks ass but i'll just leave it here cause it looks kinda nice and reminds me of my wasted time

    // THIS FUNCTION MIGHT NEED HEAVY OPTIMISATION IN THE FORM OF PACKING RGB VALUES
    // INTO A SINGLE VARIABLE OR SIMPLY NOT USING FLOATS
    // Solution taken from here https://stackoverflow.com/a/48343059

    struct PixelDataFloat
    {
        float r;
        float g;
        float b;
        float a;
    };
    PixelDataFloat fgf = { fg.r / 255.0f, fg.g / 255.0f, fg.b / 255.0f, fg.a / 255.0f };
    PixelDataFloat bgf = { bg.r / 255.0f, bg.g / 255.0f, bg.b / 255.0f, bg.a / 255.0f };

    float newA = (1.0f - fgf.a) * bgf.a + fgf.a;
    float newR = ((1.0f - fgf.a) * bgf.a * bgf.r + fgf.a * fgf.r) / newA;
    float newG = ((1.0f - fgf.a) * bgf.a * bgf.g + fgf.a * fgf.g) / newA;
    float newB = ((1.0f - fgf.a) * bgf.a * bgf.b + fgf.a * fgf.b) / newA;

    // Ensuring value validity, might need uncommenting if overflow/underflow appears to be common
    //if (newR > 1.0f) newR = 1.0f;
    //else if (newR < 0.0f) newR = 0.0f;
    //if (newG > 1.0f) newG = 1.0f;
    //else if (newG < 0.0f) newG = 0.0f;
    //if (newB > 1.0f) newB = 1.0f;
    //else if (newB < 0.0f) newB = 0.0f;

    return { uchar(newR * 255), uchar(newG * 255) , uchar(newB * 255) , uchar(newA * 255) };
}

void BlendSingle(VideoFrame* frame, ASS_Image* img)
{
    PixelData imgPd = { _r(img->color), _g(img->color), _b(img->color), 255 - _a(img->color) };

    unsigned char* src = img->bitmap;
    for (int y = 0; y < img->h; y++)
    {
        for (int x = 0; x < img->w; x++)
        {
            int screenX = img->dst_x + x;
            int screenY = img->dst_y + y;

            //PixelData imgpdNewA = imgpd;
            //imgpdNewA.a = ((unsigned)src[x]) * imgpd.a / 255;
            //frame->SetPixel(screenX, screenY, BlendPixel(frame->GetPixel(screenX, screenY), imgpdNewA));

            PixelData pdCopy = imgPd;
            pdCopy.a = ((unsigned)src[x]) * imgPd.a / 255;
            PixelData framePd = frame->GetPixel(screenX, screenY);

            framePd.r = (pdCopy.a * pdCopy.r + (255 - pdCopy.a) * framePd.r) / 255;
            framePd.g = (pdCopy.a * pdCopy.g + (255 - pdCopy.a) * framePd.g) / 255;
            framePd.b = (pdCopy.a * pdCopy.b + (255 - pdCopy.a) * framePd.b) / 255;
            framePd.a = (255 - pdCopy.a) * framePd.a / 255 + pdCopy.a;

            frame->SetPixel(screenX, screenY, framePd);
        }
        src += img->stride;
    }
}

void Blend(VideoFrame* frame, ASS_Image* img)
{
    while (img)
    {
        BlendSingle(frame, img);
        img = img->next;
    }
}

void SubtitleDecoder::_RenderingThread()
{
    while (!_decoderThreadStop)
    {
        if (
            _frames.size() >= _MAX_FRAME_QUEUE_SIZE ||
            _lastRenderedFrameTime.GetTicks() == -1 ||
            _lastRenderedFrameTime > _lastBufferedSubtitleTime ||
            !_renderer
            ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Busy sleep for 500us to allow other functions to acquire the lock
        Clock timer = Clock(0);
        while (timer.Now().GetTime(MICROSECONDS) < 500)
            timer.Update();

        std::unique_lock<std::mutex> lock(_m_ass);                                                          // <---
        // Do an additional check here because the flush usually modifies the value while waiting for the lock here
        if (_lastRenderedFrameTime.GetTicks() == -1)
            continue;
        _lastRenderedFrameTime += _timeBetweenFrames;

        int change = 0;
        ASS_Image* img = ass_render_frame(_renderer, _track, _lastRenderedFrameTime.GetTime(MILLISECONDS), &change);
        if (change != 0)
        {
            int width, height;
            if (_outputWidth != 0 && _outputHeight != 0)
            {
                width = _outputWidth;
                height = _outputHeight;
            }
            else
            {
                width = _track->PlayResX;
                height = _track->PlayResY;
            }
            VideoFrame* vf = new VideoFrame(width, height, _lastRenderedFrameTime.GetTime(MICROSECONDS));
            std::fill_n((uchar*)vf->GetBytes(), vf->GetWidth() * vf->GetHeight() * 4, 0);
            Blend(vf, img);

            std::lock_guard<std::mutex> lock2(_m_frames);
            _frames.push((IMediaFrame*)vf);
        }

        lock.unlock();
    }
}

VideoFrame SubtitleDecoder::RenderFrame(TimePoint time)
{
    std::unique_lock<std::mutex> lock(_m_ass);
    ASS_Image* img = ass_render_frame(_renderer, _track, time.GetTime(MILLISECONDS), NULL);
    lock.unlock();

    //VideoFrame vf = VideoFrame(_track->PlayResX, _track->PlayResY, time.GetTime(MICROSECONDS));
    int width, height;
    if (_outputWidth != 0 && _outputHeight != 0)
    {
        width = _outputWidth;
        height = _outputHeight;
    }
    else
    {
        width = _track->PlayResX;
        height = _track->PlayResY;
    }
    VideoFrame vf = VideoFrame(width, height, time.GetTime(MICROSECONDS));
    std::fill_n((uchar*)vf.GetBytes(), vf.GetWidth() * vf.GetHeight() * 4, 0);
    Blend(&vf, img);
    return vf;
}

void SubtitleDecoder::AddFonts(const std::vector<FontDesc>& fonts)
{
    std::unique_lock<std::mutex> lock(_m_ass);

    for (auto& font : fonts)
    {
        ass_add_font(_library, font.name, font.data, font.dataSize);
    }
    _ResetRenderer();
}

void SubtitleDecoder::SetOutputSize(int width, int height)
{
    std::unique_lock<std::mutex> lock(_m_ass);

    _outputWidth = width;
    _outputHeight = height;
    _ResetRenderer();
}

int SubtitleDecoder::GetOutputWidth() const
{
    return _outputWidth;
}

int SubtitleDecoder::GetOutputHeight() const
{
    return _outputHeight;
}

void SubtitleDecoder::SetFramerate(int fps)
{
    _timeBetweenFrames = Duration(1000000LL / fps, MICROSECONDS);
}

void SubtitleDecoder::SkipForward(Duration amount)
{
    _lastRenderedFrameTime += amount;
    ClearFrames();
}

void SubtitleDecoder::_LoadOptions()
{
    // Subtitle framerate
    std::wstring optStr = Options::Instance()->GetValue(OPTIONS_SUBTITLE_FRAMERATE);
    int64_t fps = IntOptionAdapter(optStr, 20).Value();
    _timeBetweenFrames = Duration(1000 / fps, MILLISECONDS);
    if (_timeBetweenFrames < Duration(1, MILLISECONDS))
        _timeBetweenFrames = Duration(1, MILLISECONDS);

    // Frame buffer size
    optStr = Options::Instance()->GetValue(OPTIONS_MAX_SUBTITLE_FRAMES);
    _MAX_FRAME_QUEUE_SIZE = IntOptionAdapter(optStr, 10).Value();

    // Packet buffer size
    _MAX_PACKET_QUEUE_SIZE = 30;
}

void SubtitleDecoder::_ResetRenderer()
{
    if (_subType != SubtitleType::ASS)
        return;

    if (_renderer)
    {
        ass_renderer_done(_renderer);
    }
    _renderer = ass_renderer_init(_library);
    if (_outputWidth != 0 && _outputHeight != 0)
    {
        ass_set_frame_size(_renderer, _outputWidth, _outputHeight);
    }
    else
    {
        ass_set_frame_size(_renderer, _track->PlayResX, _track->PlayResY);
    }
    ass_set_fonts(_renderer, NULL, "sans-serif", ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
}