#include "VideoDecoder.h"
//#include "VideoFrame.h"

#include "VideoFrame_BGRA.h"

#include "App.h"

#include "Options.h"
#include "OptionNames.h"
#include "IntOptionAdapter.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/hwcontext.h>
}

VideoDecoder::VideoDecoder(const MediaStream& stream)
{
    AVCodec* codec = avcodec_find_decoder(stream.GetParams()->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream.GetParams());

    // Find supported hw acceleration modes
    if (avcodec_get_hw_config(codec, 0))
    {
        AVBufferRef* dctx = nullptr;
        for (int i = 0; !dctx; i++)
        {
            const AVCodecHWConfig* hwconfig = avcodec_get_hw_config(codec, i);
            if (!hwconfig)
                break;

            //int ret = av_hwdevice_ctx_create(&dctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0);
            //int ret = av_hwdevice_ctx_create(&dctx, AV_HWDEVICE_TYPE_DXVA2, NULL, NULL, 0);
            int ret = av_hwdevice_ctx_create(&dctx, hwconfig->device_type, NULL, NULL, 0);
        }
        if (dctx)
        {
            _codecContext->hw_device_ctx = dctx;
            _hwDeviceCtx = dctx;
            _hwAccelerated = true;
        }
    }

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

        // If packet is last, immediatelly send it to frame queue
        if (_packets.front().last)
        {
            _packets.pop();
            _m_packets.unlock();

            IMediaFrame* lastFrame = new IMediaFrame(-1);
            lastFrame->last = true;
            _m_frames.lock();
            _frames.push(lastFrame);
            _m_frames.unlock();

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

        int err = 0;
        AVPixelFormat outputFormat = _codecContext->pix_fmt;
        while (_hwAccelerated && frame->hw_frames_ctx)
        {
            AVPixelFormat* pformats;
            err = av_hwframe_transfer_get_formats(frame->hw_frames_ctx, AV_HWFRAME_TRANSFER_DIRECTION_FROM, &pformats, 0);
            if (err < 0)
                break;
            std::vector<AVPixelFormat> formats;
            while (*pformats != AV_PIX_FMT_NONE)
                formats.push_back(*(pformats++));
            if (formats.empty())
            {
                err = -1;
                break;
            }

            // Set up output frame
            AVFrame* oframe = NULL;
            outputFormat = formats[0];
            if (frame->format == outputFormat)
                break;
            oframe = av_frame_alloc();
            if (!oframe)
            {
                err = -1;
                break;
            }
            oframe->format = outputFormat;

            // Move data from hw frame
            err = av_hwframe_transfer_data(oframe, frame, 0);
            if (err < 0)
            {
                av_frame_free(&oframe);
                break;
            }
            err = av_frame_copy_props(oframe, frame);

            // Swap frame data
            av_frame_unref(frame);
            av_frame_move_ref(frame, oframe);
            av_frame_free(&oframe);

            break;
        }
        if (err < 0)
        {
            std::cout << "HW error " << err << '\n';
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
                outputFormat,
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

        auto pData = std::make_unique<unsigned char[]>(frame->width * frame->height * 4);
        std::copy_n(data, frame->width * frame->height * 4, pData.get());
        VideoFrame_BGRA* videoFrame = new VideoFrame_BGRA(TimePoint(timestamp, MICROSECONDS), frame->width, frame->height, std::move(pData));

        //VideoFrame* vf = new VideoFrame(frame->width, frame->height, timestamp, discontinuity);
        discontinuity = false;
        //vf->SetBytes(data);

        _m_frames.lock();
        _frames.push((IMediaFrame*)videoFrame);
        _m_frames.unlock();
    }

    delete[] data;

    if (swsContext)
        sws_freeContext(swsContext);
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