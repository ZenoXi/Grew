#include "AudioDecoder.h"
#include "AudioFrame.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <iostream>

struct AudioChunkData
{
    AVFrame* frame;
    char* outputData;
    int bytesPerSample;
};

void SelectSampleConverter(void(**convertChunk)(AudioChunkData), int& bytesPerSample, int sampleFormat);
void ConvertNoneChunk(AudioChunkData data);
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

    int currentSampleFormat = _codecContext->sample_fmt;
    void(*convertChunk)(AudioChunkData) = nullptr;
    SelectSampleConverter(&convertChunk, bytesPerSample, _codecContext->sample_fmt);
    //if (!convertChunk) return;

    AVFrame* frame = av_frame_alloc();

    bool discontinuity = true;

    while (!_decoderThreadStop)
    {
        // Seek
        if (_decoderThreadFlush)
        {
            // Flush decoder
            avcodec_send_packet(_codecContext, NULL);
            while (avcodec_receive_frame(_codecContext, frame) != AVERROR_EOF);
            avcodec_flush_buffers(_codecContext);

            ClearPackets();
            ClearFrames();
            _decoderThreadFlush = false;
            discontinuity = true;
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

        // Account for possible mid stream sample format change
        if (currentSampleFormat != frame->format)
        {
            currentSampleFormat = frame->format;
            SelectSampleConverter(&convertChunk, bytesPerSample, frame->format);
            discontinuity = true;
            std::cout << "[AudioDecoder] Sample format changed to " << av_get_sample_fmt_name((AVSampleFormat)currentSampleFormat) << std::endl;
        }

        // The format accepted by XAudio is signed 16 bit, hence the 2 (bytes) at the end
        int chunkSize = frame->nb_samples * frame->channels * 2;
        char* audioData = new char[chunkSize];

        // Convert chunk
        convertChunk({ frame, audioData, bytesPerSample });

        // Create AudioFrame
        long long int timestamp = av_rescale_q(frame->pts, _timebase, { 1, AV_TIME_BASE });
        AudioFrame* af = new AudioFrame(frame->nb_samples, frame->channels, frame->sample_rate, timestamp, discontinuity);
        discontinuity = false;
        af->SetBytes(audioData);

        _m_frames.lock();
        _frames.push((IMediaFrame*)af);
        _m_frames.unlock();

        delete[] audioData;
    }

    av_frame_unref(frame);
    av_frame_free(&frame);
}

void SelectSampleConverter(void(**convertChunk)(AudioChunkData), int& bytesPerSample, int sampleFormat)
{
    switch (sampleFormat)
    {
    case AV_SAMPLE_FMT_U8:
        (*convertChunk) = &ConvertU8Chunk;
        bytesPerSample = 1;
        break;
    case AV_SAMPLE_FMT_S16:
        (*convertChunk) = &ConvertS16Chunk;
        bytesPerSample = 2;
        break;
    case AV_SAMPLE_FMT_S32:
        (*convertChunk) = &ConvertS32Chunk;
        bytesPerSample = 4;
        break;
    case AV_SAMPLE_FMT_FLT:
        (*convertChunk) = &ConvertFLTChunk;
        bytesPerSample = 4;
        break;
    case AV_SAMPLE_FMT_DBL:
        (*convertChunk) = &ConvertDBLChunk;
        bytesPerSample = 8;
        break;
    case AV_SAMPLE_FMT_U8P:
        (*convertChunk) = &ConvertU8PlanarChunk;
        bytesPerSample = 1;
        break;
    case AV_SAMPLE_FMT_S16P:
        (*convertChunk) = &ConvertS16PlanarChunk;
        bytesPerSample = 2;
        break;
    case AV_SAMPLE_FMT_S32P:
        (*convertChunk) = &ConvertS32PlanarChunk;
        bytesPerSample = 4;
        break;
    case AV_SAMPLE_FMT_FLTP:
        (*convertChunk) = &ConvertFLTPlanarChunk;
        bytesPerSample = 4;
        break;
    case AV_SAMPLE_FMT_DBLP:
        (*convertChunk) = &ConvertDBLPlanarChunk;
        bytesPerSample = 8;
        break;
    default:
        (*convertChunk) = &ConvertNoneChunk;
        break;
    }
}

void ConvertNoneChunk(AudioChunkData data)
{
    std::fill_n(
        data.frame->data[0],
        data.frame->nb_samples * data.frame->channels * 2,
        (int8_t)0
    );
}

void ConvertU8Chunk(AudioChunkData data)
{
    static constexpr int16_t num = std::numeric_limits<int16_t>::max();
    static constexpr int16_t den = std::numeric_limits<int8_t>::max();
    static constexpr int16_t ratio = num / den;

    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            uint8_t uval = *(uint8_t*)(data.frame->data[0] + data.bytesPerSample * data.frame->channels * i + data.bytesPerSample * ch);
            int8_t sval = (int8_t)(uval - 128);
            int16_t value = (int16_t)(sval * ratio);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
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
    static constexpr int32_t num = std::numeric_limits<int32_t>::max();
    static constexpr int32_t den = std::numeric_limits<int16_t>::max();
    static constexpr int32_t ratio = num / den;

    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            int32_t val = *(int32_t*)(data.frame->data[0] + data.bytesPerSample * data.frame->channels * i + data.bytesPerSample * ch);
            int16_t value = (int16_t)(val / ratio);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
}

void ConvertFLTChunk(AudioChunkData data)
{
    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            float val = *(float*)(data.frame->data[0] + data.bytesPerSample * data.frame->channels * i + data.bytesPerSample * ch);
            if (val < -1.0f) val = -1.0f;
            else if (val > 1.0f) val = 1.0f;
            int16_t value = (int16_t)(val * 32767.0f);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
}

void ConvertDBLChunk(AudioChunkData data)
{
    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            double val = *(double*)(data.frame->data[0] + data.bytesPerSample * data.frame->channels * i + data.bytesPerSample * ch);
            if (val < -1.0) val = -1.0;
            else if (val > 1.0) val = 1.0;
            int16_t value = (int16_t)(val * 32767.0);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
}

void ConvertU8PlanarChunk(AudioChunkData data)
{
    static constexpr int16_t num = std::numeric_limits<int16_t>::max();
    static constexpr int16_t den = std::numeric_limits<int8_t>::max();
    static constexpr int16_t ratio = num / den;

    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            uint8_t uval = *(uint8_t*)(data.frame->data[ch] + data.bytesPerSample * i);
            int8_t sval = (int8_t)(uval - 128);
            int16_t value = (int16_t)(sval * ratio);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
}

void ConvertS16PlanarChunk(AudioChunkData data)
{
    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            int16_t val = *(int16_t*)(data.frame->data[ch] + data.bytesPerSample * i);
            *(int16_t*)(data.outputData + curPos) = val;
            curPos += 2;
        }
    }
}

void ConvertS32PlanarChunk(AudioChunkData data)
{
    static constexpr int32_t num = std::numeric_limits<int32_t>::max();
    static constexpr int32_t den = std::numeric_limits<int16_t>::max();
    static constexpr int32_t ratio = num / den;

    int curPos = 0;
    for (int i = 0; i < data.frame->nb_samples; i++)
    {
        for (int ch = 0; ch < data.frame->channels; ch++)
        {
            int32_t val = *(int32_t*)(data.frame->data[ch] + data.bytesPerSample * i);
            int16_t value = (int16_t)(val / ratio);
            *(int16_t*)(data.outputData + curPos) = value;
            curPos += 2;
        }
    }
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