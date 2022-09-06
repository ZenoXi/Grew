#include "VideoDecoder.h"
#include "VideoFrame.h"

#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

VideoDecoder::VideoDecoder(const MediaStream& stream)
{
    AVCodec* codec = avcodec_find_decoder(stream.GetParams()->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream.GetParams());
    avcodec_open2(_codecContext, codec, NULL);

    _timebase = stream.timeBase;

    _LoadOptions();

    // Start decoding thread
    _decoderThread = std::thread(&VideoDecoder::_DecoderThread, this);
}

VideoDecoder::~VideoDecoder()
{
    _decoderThreadStop = true;
    if (_decoderThread.joinable())
        _decoderThread.join();
    avcodec_close(_codecContext);
    avcodec_free_context(&_codecContext);
}

void VideoDecoder::_DecoderThread()
{
    AVFrame* frame = av_frame_alloc();

    SwsContext* swsContext = NULL;

    uchar* data = nullptr;
    uchar* dest[4] = { NULL, NULL, NULL, NULL };
    int destLinesize[4] = { 0, 0, 0, 0 };

    // Codec context might have uninitialized width/height values at this point
    if (_codecContext->width != 0 && _codecContext->height != 0)
    {
        data = new uchar[_codecContext->width * _codecContext->height * 4];
        dest[0] = data;
        destLinesize[0] = _codecContext->width * 4;
    }

    bool discontinuity = true;

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
            avcodec_send_packet(_codecContext, NULL);
            while (avcodec_receive_frame(_codecContext, frame) != AVERROR_EOF);
            avcodec_flush_buffers(_codecContext);

            ClearFrames();
            ClearPackets();
            _decoderThreadFlush = false;
            discontinuity = true;
            continue;
        }

        // Limit loaded frame count
        if (_frames.size() >= _MAX_FRAME_QUEUE_SIZE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

        int response = avcodec_send_packet(_codecContext, _packets.front().GetPacket());
        if (response != AVERROR(EAGAIN))
        {
            _packets.pop();
        }
        _m_packets.unlock();
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            continue;
        }

        response = avcodec_receive_frame(_codecContext, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            continue;
        }

        // Deferred destination buffer initialization
        if (data == nullptr)
        {
            data = new uchar[_codecContext->width * _codecContext->height * 4];
            dest[0] = data;
            destLinesize[0] = _codecContext->width * 4;
        }

        if (!swsContext)
        {
            swsContext = sws_getContext(
                _codecContext->width,
                _codecContext->height,
                _codecContext->pix_fmt,
                _codecContext->width,
                _codecContext->height,
                AV_PIX_FMT_BGRA,
                SWS_FAST_BILINEAR,
                NULL,
                NULL,
                NULL
            );
        }
        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dest, destLinesize);

        long long int timestamp = av_rescale_q(frame->pts, _timebase, { 1, AV_TIME_BASE });
        if (frame->pts == AV_NOPTS_VALUE)
            timestamp = AV_NOPTS_VALUE;
        VideoFrame* vf = new VideoFrame(frame->width, frame->height, timestamp, discontinuity);
        discontinuity = false;
        vf->SetBytes(data);

        _m_frames.lock();
        _frames.push((IMediaFrame*)vf);
        _m_frames.unlock();
    }

    delete[] data;

    if (swsContext) sws_freeContext(swsContext);
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void VideoDecoder::_LoadOptions()
{
    // Frame buffer size
    std::wstring optStr = Options::Instance()->GetValue(OPTIONS_MAX_VIDEO_FRAMES);
    _MAX_FRAME_QUEUE_SIZE = IntOptionAdapter(optStr, 12).Value();

    // Packet buffer size
    _MAX_PACKET_QUEUE_SIZE = 30;
}