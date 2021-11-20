#include "VideoDecoder.h"
#include "VideoFrame.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

VideoDecoder::VideoDecoder(const MediaStream& stream)
{
    AVCodec* codec = avcodec_find_decoder(stream.GetParams()->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream.GetParams());
    avcodec_open2(_codecContext, codec, NULL);

    // placeholder until global options are implemented
    _MAX_FRAME_QUEUE_SIZE = 15;
    _MAX_PACKET_QUEUE_SIZE = 100;
}

VideoDecoder::~VideoDecoder()
{
    avcodec_close(_codecContext);
    avcodec_free_context(&_codecContext);
}

void VideoDecoder::_DecoderThread()
{
    AVFrame* frame = av_frame_alloc();

    SwsContext* swsContext = NULL;

    uchar* data = new uchar[_codecContext->width * _codecContext->height * 4];
    uchar* dest[4] = { data, NULL, NULL, NULL };
    int destLinesize[4] = { _codecContext->width * 4, 0, 0, 0 };

    while (!_decoderThreadStop)
    {
        // Seek
        if (_decoderThreadFlush)
        {
            // Flush decoder
            avcodec_send_packet(_codecContext, NULL);
            while (avcodec_receive_frame(_codecContext, frame) != AVERROR_EOF);
            avcodec_flush_buffers(_codecContext);

            ClearFrames();
            _decoderThreadFlush = false;
            continue;
        }

        // Limit loaded frame count
        if (_frames.size() >= _MAX_FRAME_QUEUE_SIZE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
            continue;
        }

        // If finished, wait for seek command/new packets
        _m_packets.lock(); // Prevent packet flushing on seek
        if (_packets.empty())
        {
            _m_packets.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
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

        AVRational timebase = _codecContext->time_base;
        long long int timestamp = av_rescale_q(frame->pts, timebase, { 1, AV_TIME_BASE });

        VideoFrame* vf = new VideoFrame(frame->width, frame->height, timestamp);
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