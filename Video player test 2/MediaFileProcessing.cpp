#include "MediaFileProcessing.h"

#include "Functions.h"

#include <iostream>
#include <unordered_map>

int64_t MetadataDurationToMicroseconds(std::string data);

void MediaFileProcessing::ExtractStreams()
{
    for (int i = 0; i < avfContext->nb_streams; i++)
    {
        AVStream* avstream = avfContext->streams[i];

        MediaStream stream(avstream->codecpar);
        stream.index = i;
        stream.packetCount = avstream->nb_frames;
        stream.startTime = avstream->start_time;
        stream.duration = avstream->duration;
        stream.timeBase = avstream->time_base;

        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(avstream->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
        {
            stream.metadata.push_back({ t->key, t->value });
            std::cout << "[" << t->key << "]: \"" << t->value << "\"\n";
        }
        std::cout << std::endl;

        switch (avstream->codecpar->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
        {
            stream.width = avstream->codecpar->width;
            stream.height = avstream->codecpar->height;
            stream.type = MediaStreamType::VIDEO;
            videoStreams.push_back(std::move(stream));
            continue;
        }
        case AVMEDIA_TYPE_AUDIO:
        {
            stream.channels = avstream->codecpar->channels;
            stream.sampleRate = avstream->codecpar->sample_rate;
            stream.type = MediaStreamType::AUDIO;
            audioStreams.push_back(std::move(stream));
            continue;
        }
        case AVMEDIA_TYPE_SUBTITLE:
        {
            stream.type = MediaStreamType::SUBTITLE;
            subtitleStreams.push_back(std::move(stream));
            continue;
        }
        case AVMEDIA_TYPE_ATTACHMENT:
        {
            stream.type = MediaStreamType::ATTACHMENT;
            attachmentStreams.push_back(std::move(stream));
            continue;
        }
        case AVMEDIA_TYPE_DATA:
        {
            stream.type = MediaStreamType::DATA;
            dataStreams.push_back(std::move(stream));
            continue;
        }
        case AVMEDIA_TYPE_UNKNOWN:
        {
            stream.type = MediaStreamType::UNKNOWN;
            unknownStreams.push_back(std::move(stream));
            continue;
        }
        }
    }
}

void MediaFileProcessing::FindMissingStreamData()
{
    // CODE HERE CAN PROBABLY BE IMPROVED BUT I CANT BE ARSED TO DO SO

    // Video Streams
    for (auto& stream : videoStreams)
    {
        // A copy of metadata is necessary because it will be modified
        std::vector<MediaMetadataPair> metadata = stream.metadata;
        for (auto& it : metadata)
        {
            it.key = to_lowercase(it.key);
        }

        for (auto& it : metadata)
        {
            if (it.key.substr(0, strlen("number_of_frames")) == "number_of_frames")
            {
                if (stream.packetCount == 0)
                {
                    stream.packetCount = str_to_int(it.value);
                }
            }
            else if (it.key.substr(0, strlen("duration")) == "duration")
            {
                if (stream.duration == 0 || stream.duration == AV_NOPTS_VALUE)
                {
                    stream.duration = av_rescale_q(MetadataDurationToMicroseconds(it.value), { 1, AV_TIME_BASE }, stream.timeBase);
                }
            }
        }

        if (stream.startTime == AV_NOPTS_VALUE)
        {
            stream.startTime = 0;
        }
    }

    // Audio Streams
    for (auto& stream : audioStreams)
    {
        // A copy of metadata is necessary because it will be modified
        std::vector<MediaMetadataPair> metadata = stream.metadata;
        for (auto& it : metadata)
        {
            it.key = to_lowercase(it.key);
        }

        for (auto& it : metadata)
        {
            if (it.key.substr(0, strlen("number_of_frames")) == "number_of_frames")
            {
                if (stream.packetCount == 0 || stream.packetCount == AV_NOPTS_VALUE)
                {
                    stream.packetCount = str_to_int(it.value);
                }
            }
            else if (it.key.substr(0, strlen("duration")) == "duration")
            {
                if (stream.duration == 0 || stream.duration == AV_NOPTS_VALUE)
                {
                    stream.duration = av_rescale_q(MetadataDurationToMicroseconds(it.value), { 1, AV_TIME_BASE }, stream.timeBase);
                }
            }
        }

        if (stream.startTime == AV_NOPTS_VALUE)
        {
            stream.startTime = 0;
        }
    }

    // Subtitle Streams
    for (auto& stream : subtitleStreams)
    {
        // A copy of metadata is necessary because it will be modified
        std::vector<MediaMetadataPair> metadata = stream.metadata;
        for (auto& it : metadata)
        {
            it.key = to_lowercase(it.key);
        }

        for (auto& it : metadata)
        {
            if (it.key.substr(0, strlen("number_of_frames")) == "number_of_frames")
            {
                if (stream.packetCount == 0 || stream.packetCount == AV_NOPTS_VALUE)
                {
                    stream.packetCount = str_to_int(it.value);
                }
            }
            else if (it.key.substr(0, strlen("duration")) == "duration")
            {
                if (stream.duration == 0 || stream.duration == AV_NOPTS_VALUE)
                {
                    stream.duration = av_rescale_q(MetadataDurationToMicroseconds(it.value), { 1, AV_TIME_BASE }, stream.timeBase);
                }
            }
        }

        if (stream.startTime == AV_NOPTS_VALUE)
        {
            stream.startTime = 0;
        }
    }
}

void MediaFileProcessing::CalculateMissingStreamData(bool full)
{
    // These classes are only used in this function

    class StreamDecoder
    {
    public:
        AVCodecContext* codecContext = nullptr;
        MediaStream* mediaStream = nullptr;
        int packetCount = 0;
        int64_t duration = 0;
        int64_t startTime = AV_NOPTS_VALUE;
        int64_t lastPts = AV_NOPTS_VALUE;
        int64_t lastDts = AV_NOPTS_VALUE;
        int64_t lastPtsDuration = AV_NOPTS_VALUE;
        int64_t lastDtsDuration = AV_NOPTS_VALUE;
        bool frameDataGathered = false;

        StreamDecoder(MediaStream* stream) : mediaStream(stream)
        {
            AVCodecParameters* params = mediaStream->GetParams();
            AVCodec* codec = avcodec_find_decoder(params->codec_id);
            codecContext = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(codecContext, params);
            avcodec_open2(codecContext, codec, NULL);
        }
        virtual ~StreamDecoder()
        {
            // Get final duration
            int64_t ptsDuration = lastPts != AV_NOPTS_VALUE ? lastPts + lastPtsDuration : AV_NOPTS_VALUE;
            int64_t dtsDuration = lastDts != AV_NOPTS_VALUE ? lastDts + lastDtsDuration : AV_NOPTS_VALUE;
            if (ptsDuration != AV_NOPTS_VALUE)
            {
                duration = ptsDuration;
            }
            else if (dtsDuration != AV_NOPTS_VALUE)
            {
                duration = dtsDuration;
            }

            //if (mediaStream->packetCount != packetCount)
            //{
            //    std::cout << "Packet count mismatch (" << packetCount << "/" << mediaStream->packetCount << ") in stream " << mediaStream->index << std::endl;
            //    mediaStream->packetCount = packetCount;
            //}
            //if (mediaStream->duration != duration)
            //{
            //    std::cout << "Duration mismatch (" << duration << "/" << mediaStream->duration << ") in stream " << mediaStream->index << std::endl;
            //    mediaStream->duration = duration;
            //}

            if (mediaStream->packetCount == 0 || mediaStream->packetCount == AV_NOPTS_VALUE)
            {
                mediaStream->packetCount = packetCount;
            }
            if (mediaStream->duration == 0 || mediaStream->duration == AV_NOPTS_VALUE)
            {
                mediaStream->duration = duration;
            }

            avcodec_close(codecContext);
            avcodec_free_context(&codecContext);
        }

        bool Finished() const
        {
            if (!frameDataGathered) return false;
            if (mediaStream->packetCount == 0 || mediaStream->packetCount == AV_NOPTS_VALUE) return false;
            if (mediaStream->duration == 0 || mediaStream->duration == AV_NOPTS_VALUE) return false;
            return true;
        }

        void ExtractInfoFromPacket(AVPacket* packet)
        {
            if (packet->pts != AV_NOPTS_VALUE)
            {
                if (lastPts == AV_NOPTS_VALUE)
                {
                    lastPts = packet->pts;
                    lastPtsDuration = packet->duration;
                }
                else if (lastPts < packet->pts)
                {
                    lastPts = packet->pts;
                    lastPtsDuration = packet->duration;
                }
            }

            if (packet->dts != AV_NOPTS_VALUE)
            {
                if (lastDts == AV_NOPTS_VALUE)
                {
                    lastDts = packet->dts;
                    lastDtsDuration = packet->duration;
                }
                else if (lastDts < packet->dts)
                {
                    lastDts = packet->dts;
                    lastDtsDuration = packet->duration;
                }
            }

            packetCount++;
        }

        void ExtractInfoFromFrame(AVFrame* frame)
        {
            duration = frame->pts + frame->pkt_duration;
            if (startTime == AV_NOPTS_VALUE)
            {
                startTime = frame->pts;
            }

            _ExtractInfoFromFrame(frame);
        }

    private:
        virtual void _ExtractInfoFromFrame(AVFrame* frame) = 0;
    };

    class VideoStreamDecoder : public StreamDecoder
    {
    public:
        VideoStreamDecoder(MediaStream* stream) : StreamDecoder(stream)
        {
            if (stream->width != 0 && stream->height != 0)
            {
                frameDataGathered = true;
            }
        }

    private:
        void _ExtractInfoFromFrame(AVFrame* frame)
        {
            if (mediaStream->width == 0)
            {
                mediaStream->width = frame->width;
            }
            if (mediaStream->height == 0)
            {
                mediaStream->height = frame->height;
            }

            frameDataGathered = true;
        }
    };

    class AudioStreamDecoder : public StreamDecoder
    {
    public:
        AudioStreamDecoder(MediaStream* stream) : StreamDecoder(stream)
        {
            if (stream->channels != 0 && stream->sampleRate != 0)
            {
                frameDataGathered = true;
            }
        }

    private:
        void _ExtractInfoFromFrame(AVFrame* frame)
        {
            if (mediaStream->channels == 0)
            {
                mediaStream->channels = frame->channels;
            }
            if (mediaStream->sampleRate == 0)
            {
                mediaStream->sampleRate = frame->sample_rate;
            }

            frameDataGathered = true;
        }
    };

    // Align stream_index
    std::vector<std::unique_ptr<StreamDecoder>> streamDecoders;
    streamDecoders.resize(avfContext->nb_streams);

    for (int i = 0; i < videoStreams.size(); i++)
    {
        VideoStreamDecoder* decoder = new VideoStreamDecoder(&videoStreams[i]);
        streamDecoders[decoder->mediaStream->index] = std::unique_ptr<StreamDecoder>(decoder);
    }
    for (int i = 0; i < audioStreams.size(); i++)
    {
        AudioStreamDecoder* decoder = new AudioStreamDecoder(&audioStreams[i]);
        streamDecoders[decoder->mediaStream->index] = std::unique_ptr<StreamDecoder>(decoder);
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    // Automatic unref wrapper for AVPacket
    struct PacketHolder
    {
        AVPacket* packet;
        ~PacketHolder() { av_packet_unref(packet); }
    };

    while (av_read_frame(avfContext, packet) >= 0)
    {
        PacketHolder holder = { packet };

        //if (packet->stream_index == 0)
        //{
        //    int k = 0;
        //    std::cout << packet->pts << " | " << packet->dts;
        //    if (packet->flags & AV_PKT_FLAG_KEY)
        //    {
        //        std::cout << " (KEY)";
        //        k++;
        //    }
        //    std::cout << std::endl;
        //}

        // Check if all decoders have gathered missing data
        bool done = true;
        for (int i = 0; i < streamDecoders.size(); i++)
        {
            if (streamDecoders[i])
            {
                if (!streamDecoders[i]->Finished())
                {
                    done = false;
                    break;
                }
            }
        }
        if (done) break; // UNCOMMENT ///////////////////////////////////////////////////////////////////////

        int index = packet->stream_index;
        if (!streamDecoders[index]) continue;

        streamDecoders[index]->ExtractInfoFromPacket(packet);

        // Only decode until necessary data is extracted
        if (streamDecoders[index]->frameDataGathered) continue; // UNCOMMENT /////////////////////////////////

        int response = avcodec_send_packet(streamDecoders[index]->codecContext, packet);
        if (response < 0)
        {
            printf("Packet decode error %d\n", response);
            continue;
        }

        response = avcodec_receive_frame(streamDecoders[index]->codecContext, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            continue;
        }
        else if (response != 0)
        {
            printf("Packet decode error %d\n", response);
            continue;
        }

        if (packet->stream_index == 0)
        {
            int k = 0;
            if (frame->key_frame)
            {
                k++;
            }
        }

        streamDecoders[index]->ExtractInfoFromFrame(frame);
    }

    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);

    streamDecoders.clear();
}

int64_t MetadataDurationToMicroseconds(std::string data)
{
    int64_t time = 0;

    std::vector<std::string> hrsMinVec;
    split_str(data, hrsMinVec, ':');
    if (hrsMinVec.size() != 3)
    {
        return 0;
    }
    if (hrsMinVec.size() == 3)
    {
        time += str_to_int(hrsMinVec[0]) * 3600000000LL;
        time += str_to_int(hrsMinVec[1]) * 60000000LL;
    }
    else if (hrsMinVec.size() == 2)
    {
        time += str_to_int(hrsMinVec[0]) * 60000000LL;
    }

    std::string secNanoStr = hrsMinVec.back();
    std::vector<std::string> secNanoVec;
    split_str(secNanoStr, secNanoVec, '.');
    if (secNanoVec.size() != 2)
    {
        return 0;
    }
    time += str_to_int(secNanoVec[0]) * 1000000;
    time += str_to_int(secNanoVec[1]) / 1000;

    return time;
}