#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

typedef unsigned char uchar;

struct FFmpeg
{
    struct AVPacketEx
    {
        AVPacket* packet = nullptr;
        bool first = false;
        bool last = false;
    };

    struct Result
    {
        uchar* data = nullptr;
        size_t size = 0;
    };

    static Result SerializeCodecParams(const AVCodecParameters* params)
    {
        size_t structSize = sizeof(AVCodecParameters);
        size_t totalSize = structSize + params->extradata_size;
        uchar* bytes = new uchar[totalSize]();
        memcpy(bytes, params, structSize);
        memcpy(bytes + structSize, params->extradata, params->extradata_size);
        return { bytes, totalSize };
    }

    static AVCodecParameters* DeserializeCodecParams(uchar* bytes)
    {
        size_t structSize = sizeof(AVCodecParameters);
        AVCodecParameters* params = (AVCodecParameters*)av_malloc(structSize);
        memcpy(params, bytes, structSize);
        params->extradata = (uint8_t*)av_mallocz(params->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(params->extradata, bytes + structSize, params->extradata_size);
        return params;
    }

    static Result SerializeAVPacket(const AVPacket* packet)
    {
        size_t avpSize = sizeof(AVPacket);
        size_t avpsdSize = sizeof(AVPacketSideData);

        size_t totalSize = avpSize + packet->size;
        AVPacketSideData* ptr = packet->side_data;
        for (int i = 0; i < packet->side_data_elems; i++)
        {
            totalSize += avpsdSize;
            totalSize += ptr->size;
            ptr++;
        }

        uchar* bytes = new uchar[totalSize]();
        int memPos = 0;
        memcpy(bytes + memPos, packet, avpSize);
        memPos += avpSize;
        memcpy(bytes + memPos, packet->data, packet->size);
        memPos += packet->size;
        ptr = packet->side_data;
        for (int i = 0; i < packet->side_data_elems; i++)
        {
            memcpy(bytes + memPos, ptr, avpsdSize);
            memPos += avpsdSize;
            memcpy(bytes + memPos, ptr->data, ptr->size);
            memPos += ptr->size;
            ptr++;
        }

        return { bytes, totalSize };
    }

    static AVPacket* DeserializeAVPacket(uchar* bytes)
    {
        size_t avpSize = sizeof(AVPacket);
        size_t avpsdSize = sizeof(AVPacketSideData);

        AVPacket* packet = (AVPacket*)av_malloc(avpSize);
        int memPos = 0;
        memcpy(packet, bytes + memPos, avpSize);
        memPos += avpSize;
        packet->data = (uint8_t*)av_malloc(packet->size);
        memcpy(packet->data, bytes + memPos, packet->size);
        memPos += packet->size;

        AVPacketSideData* sideData = (AVPacketSideData*)av_malloc(avpsdSize * packet->side_data_elems);
        for (int i = 0; i < packet->side_data_elems; i++)
        {
            memcpy(&sideData[i], bytes + memPos, avpsdSize);
            memPos += avpsdSize;
            sideData[i].data = (uint8_t*)av_malloc(sideData[i].size);
            memcpy(sideData[i].data, bytes + memPos, sideData[i].size);
            memPos += sideData[i].size;
        }
        packet->side_data = sideData;
        packet->buf = nullptr;

        return packet;
    }

    static Result SerializeAVPacketEx(const AVPacketEx& packetex)
    {
        Result avpacketres = SerializeAVPacket(packetex.packet);
        size_t additionalSize = sizeof(AVPacketEx) - sizeof(AVPacket*);
        avpacketres.data = (uchar*)realloc(avpacketres.data, avpacketres.size + additionalSize);
        memcpy(avpacketres.data + sizeof(AVPacket), &packetex + sizeof(AVPacket*), additionalSize);
        avpacketres.size += additionalSize;
        return avpacketres;
    }

    static AVPacketEx DeserializeAVPacketEx(uchar* bytes)
    {
        // AVPacket
        size_t avpSize = sizeof(AVPacket);
        size_t avpsdSize = sizeof(AVPacketSideData);

        AVPacket* packet = (AVPacket*)av_malloc(avpSize);
        int memPos = 0;
        memcpy(packet, bytes + memPos, avpSize);
        memPos += avpSize;
        packet->data = (uint8_t*)av_malloc(packet->size);
        memcpy(packet->data, bytes + memPos, packet->size);
        memPos += packet->size;

        AVPacketSideData* sideData = (AVPacketSideData*)av_malloc(avpsdSize * packet->side_data_elems);
        for (int i = 0; i < packet->side_data_elems; i++)
        {
            memcpy(&sideData[i], bytes + memPos, avpsdSize);
            memPos += avpsdSize;
            sideData[i].data = (uint8_t*)av_malloc(sideData[i].size);
            memcpy(sideData[i].data, bytes + memPos, sideData[i].size);
            memPos += sideData[i].size;
        }
        packet->side_data = sideData;
        packet->buf = nullptr;

        // AVPacketEx
        AVPacketEx packetex;
        size_t additionalSize = sizeof(AVPacketEx) - sizeof(AVPacket*);
        packetex.packet = packet;
        memcpy(&packetex + sizeof(AVPacket*), bytes + memPos, additionalSize);
        memPos += additionalSize;

        return packetex;
    }
};