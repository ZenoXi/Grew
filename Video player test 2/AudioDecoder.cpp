#include "AudioDecoder.h"
#include "AudioFrame.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

int16_t ConvertU8Sample(uint8_t* data);
int16_t ConvertS16Sample(uint8_t* data);
int16_t ConvertS32Sample(uint8_t* data);
int16_t ConvertFLTSample(uint8_t* data);
int16_t ConvertDBLSample(uint8_t* data);

AudioDecoder::AudioDecoder(const MediaStream& stream)
{
    AVCodec* codec = avcodec_find_decoder(stream.GetParams()->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream.GetParams());
    avcodec_open2(_codecContext, codec, NULL);

    // placeholder until global options are implemented
    _MAX_FRAME_QUEUE_SIZE = 100;
}

AudioDecoder::~AudioDecoder()
{
    avcodec_close(_codecContext);
    avcodec_free_context(&_codecContext);
}

void AudioDecoder::_DecoderThread()
{
    // Set up audio data convertion variables
    int bytesPerSample = av_get_bytes_per_sample(_codecContext->sample_fmt);
    if (bytesPerSample < 0)
    {
        // This should not occur, checking just for paranoia
        printf("Failed to calculate data size\n");
        exit(1);
    }

    int16_t(*convertSample)(uint8_t*) = nullptr;
    switch (_codecContext->sample_fmt)
    {
    case AV_SAMPLE_FMT_U8:
        convertSample = &ConvertU8Sample;
        break;
    case AV_SAMPLE_FMT_S16:
        convertSample = &ConvertS16Sample;
        break;
    case AV_SAMPLE_FMT_S32:
        convertSample = &ConvertS32Sample;
        break;
    case AV_SAMPLE_FMT_FLT:
        convertSample = &ConvertFLTSample;
        break;
    case AV_SAMPLE_FMT_DBL:
        convertSample = &ConvertDBLSample;
        break;
    default:
        break;
    }
    if (!convertSample) return;

    AVFrame* frame = av_frame_alloc();

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

        // The format accepted by XAudio is signed 16 bit, hence the 2 (bytes) at the end
        int chunkSize = frame->nb_samples * frame->channels * 2;
        char* audioData = new char[chunkSize];
        int curPos = 0;

        // Convert each sample
        for (int i = 0; i < frame->nb_samples; i++)
        {
            for (int ch = 0; ch < _codecContext->channels; ch++)
            {
                int16_t value = convertSample(frame->data[ch] + bytesPerSample * i);
                *(int16_t*)(audioData + curPos) = value;
                curPos += 2;
            }
        }

        AVRational timebase = _codecContext->time_base;
        long long int timestamp = av_rescale_q(frame->pts, timebase, { 1, AV_TIME_BASE });

        AudioFrame* af = new AudioFrame(frame->nb_samples, frame->channels, frame->sample_rate, timestamp);
        af->SetBytes(audioData);

        _m_frames.lock();
        _frames.push((IMediaFrame*)af);
        _m_frames.unlock();

        delete[] audioData;
    }

    av_frame_unref(frame);
    av_frame_free(&frame);
}

int16_t ConvertU8Sample(uint8_t* data)
{
    return 0;
}

int16_t ConvertS16Sample(uint8_t* data)
{
    return 0;
}

int16_t ConvertS32Sample(uint8_t* data)
{
    return 0;
}

int16_t ConvertFLTSample(uint8_t* data)
{
    float val = *(float*)(data);
    if (val < -1.0f) val = -1.0f;
    else if (val > 1.0f) val = 1.0f;
    return (int16_t)(val * 32767.0f);
}

int16_t ConvertDBLSample(uint8_t* data)
{
    double val = *(double*)(data);
    if (val < -1.0) val = -1.0;
    else if (val > 1.0) val = 1.0;
    return (int16_t)(val * 32767.0);
}