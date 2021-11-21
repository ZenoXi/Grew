#include "AudioDecoder.h"
#include "AudioFrame.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

struct AudioChunkData
{
    AVFrame* frame;
    char* outputData;
    int bytesPerSample;
};

void ConvertU8Chunk(AudioChunkData data);
void ConvertS16Chunk(AudioChunkData data);
void ConvertS32Chunk(AudioChunkData data);
void ConvertFLTChunk(AudioChunkData data);
void ConvertDBLChunk(AudioChunkData data);
void ConvertU8PlanarChunk(AudioChunkData data);
void ConvertS16PlanarChunk(AudioChunkData data);
void ConvertS32PlanarChunk(AudioChunkData data);
void ConvertFLTPlanarChunk(AudioChunkData data);
void ConvertDBLPlanarChunk(AudioChunkData data);

AudioDecoder::AudioDecoder(const MediaStream& stream)
{
    AVCodec* codec = avcodec_find_decoder(stream.GetParams()->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream.GetParams());
    avcodec_open2(_codecContext, codec, NULL);

    _timebase = stream.timeBase;

    // placeholder until global options are implemented
    _MAX_FRAME_QUEUE_SIZE = 100;
    _MAX_PACKET_QUEUE_SIZE = 500;

    // Start decoding thread
    _decoderThread = std::thread(&AudioDecoder::_DecoderThread, this);
}

AudioDecoder::~AudioDecoder()
{
    _decoderThreadStop = true;
    if (_decoderThread.joinable())
        _decoderThread.join();
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

    void(*convertChunk)(AudioChunkData) = nullptr;
    switch (_codecContext->sample_fmt)
    {
    case AV_SAMPLE_FMT_U8:
        convertChunk = &ConvertU8Chunk;
        break;
    case AV_SAMPLE_FMT_S16:
        convertChunk = &ConvertS16Chunk;
        break;
    case AV_SAMPLE_FMT_S32:
        convertChunk = &ConvertS32Chunk;
        break;
    case AV_SAMPLE_FMT_FLT:
        convertChunk = &ConvertFLTChunk;
        break;
    case AV_SAMPLE_FMT_DBL:
        convertChunk = &ConvertDBLChunk;
        break;
    case AV_SAMPLE_FMT_U8P:
        convertChunk = &ConvertU8PlanarChunk;
        break;
    case AV_SAMPLE_FMT_S16P:
        convertChunk = &ConvertS16PlanarChunk;
        break;
    case AV_SAMPLE_FMT_S32P:
        convertChunk = &ConvertS32PlanarChunk;
        break;
    case AV_SAMPLE_FMT_FLTP:
        convertChunk = &ConvertFLTPlanarChunk;
        break;
    case AV_SAMPLE_FMT_DBLP:
        convertChunk = &ConvertDBLPlanarChunk;
        break;
    default:
        break;
    }
    if (!convertChunk) return;

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

        // Convert chunk
        convertChunk({ frame, audioData, bytesPerSample });

        // Create AudioFrame
        long long int timestamp = av_rescale_q(frame->pts, _timebase, { 1, AV_TIME_BASE });
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

void ConvertU8Chunk(AudioChunkData data)
{
    return;
}

void ConvertS16Chunk(AudioChunkData data)
{
    std::copy_n(
        data.frame->data[0],
        data.frame->nb_samples * data.frame->channels * 2,
        data.outputData
    );
}

void ConvertS32Chunk(AudioChunkData data)
{
    return;
}

void ConvertFLTChunk(AudioChunkData data)
{
    //float val = *(float*)(data);
    //if (val < -1.0f) val = -1.0f;
    //else if (val > 1.0f) val = 1.0f;
    //return (int16_t)(val * 32767.0f);
}

void ConvertDBLChunk(AudioChunkData data)
{
    //double val = *(double*)(data);
    //if (val < -1.0) val = -1.0;
    //else if (val > 1.0) val = 1.0;
    //return (int16_t)(val * 32767.0);
}

void ConvertU8PlanarChunk(AudioChunkData data)
{
    return;
}

void ConvertS16PlanarChunk(AudioChunkData data)
{
    return;
}

void ConvertS32PlanarChunk(AudioChunkData data)
{
    return;
}

void ConvertFLTPlanarChunk(AudioChunkData data)
{
    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            float val = *(float*)(data.frame->data[ch] + data.bytesPerSample * i);
            if (val < -1.0f) val = -1.0f;
            else if (val > 1.0f) val = 1.0f;
            int16_t value = (int16_t)(val * 32767.0f);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
    //float val = *(float*)(data);
    //if (val < -1.0f) val = -1.0f;
    //else if (val > 1.0f) val = 1.0f;
    //return (int16_t)(val * 32767.0f);
}

void ConvertDBLPlanarChunk(AudioChunkData data)
{
    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            double val = *(double*)(data.frame->data[ch] + data.bytesPerSample * i);
            if (val < -1.0) val = -1.0;
            else if (val > 1.0) val = 1.0;
            int16_t value = (int16_t)(val * 32767.0f);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
}