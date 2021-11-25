#pragma once

#include "ISerializable.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <stdexcept>

class MediaPacket : public ISerializable
{
    AVPacket* _packet = nullptr;
public:
    bool last = false;
    bool flush = false;

    MediaPacket(bool flush = false) : flush(flush) {}
    // Takes over ownership of the packet
    MediaPacket(AVPacket* packet)
    {
        _packet = packet;
    }
    ~MediaPacket()
    {
        _Reset();
    }
    MediaPacket(MediaPacket&& other) noexcept
    {
        _packet = other._packet;
        other._packet = nullptr;

        last = other.last;
        flush = other.flush;
        other.last = 0;
        other.flush = 0;
    }
    MediaPacket& operator=(MediaPacket&& other) noexcept
    {
        if (this != &other)
        {
            _packet = other._packet;
            other._packet = nullptr;

            last = other.last;
            flush = other.flush;
            other.last = 0;
            other.flush = 0;
        }
        return *this;
    }
    MediaPacket(const MediaPacket&) = delete;
    MediaPacket& operator=(const MediaPacket&) = delete;

    // Releases ownership of the AVPacket and returns its pointer
    AVPacket* ReleasePacket()
    {
        AVPacket* pack = _packet;
        _packet = nullptr;
        return pack;
    }

    // Returns a pointer to the AVPacket
    AVPacket* GetPacket() const
    {
        return _packet;
    }

    bool Valid() const
    {
        return _packet != nullptr;
    }

    SerializedData Serialize() const
    {
        SerializedData packetBytes;
        SerializedData otherBytes;
        if (_packet)
        {
            packetBytes = _SerializeAVPacket();
        }
        otherBytes = _SerializeRemainingFields();

        // First byte indicates wether '_packet' exists or not
        size_t totalSize = 1;
        totalSize += packetBytes.Size();
        totalSize += otherBytes.Size();
        auto bytes = std::make_unique<uchar[]>(totalSize);
        size_t memPos = 0;

        std::fill_n(bytes.get() + memPos, 1, _packet ? (uchar)1 : (uchar)0);
        memPos += 1;
        std::copy_n(packetBytes.Bytes(), packetBytes.Size(), bytes.get() + memPos);
        memPos += packetBytes.Size();
        std::copy_n(otherBytes.Bytes(), otherBytes.Size(), bytes.get() + memPos);
        memPos += otherBytes.Size();

        return { totalSize, bytes };
    }

    size_t Deserialize(SerializedData data)
    {
        _Reset();

        uchar packetExists;
        _SafeCopy(data.Bytes(), 0, &packetExists, 1, data.Size());

        size_t usedBytes1 = 0;
        size_t usedBytes2 = 0;

        if (packetExists == 1)
        {
            usedBytes1 = _DeserializeAVPacket(data.Bytes() + 1, data.Size() - 1);
        }
        usedBytes2 = _DeserializeRemainingFields(data.Bytes() + 1 + usedBytes1, data.Size() - 1 - usedBytes1);

        return usedBytes1 + usedBytes2;
    }

private:
    void _Reset()
    {
        if (_packet)
        {
            av_packet_unref(_packet);
            av_packet_free(&_packet);
            _packet = nullptr;
        }
        last = 0;
        flush = 0;
    }

    SerializedData _SerializeAVPacket() const
    {
        size_t avpSize = sizeof(AVPacket);
        size_t avpsdSize = sizeof(AVPacketSideData);
        size_t totalSize = avpSize + _packet->size;
        // Calculate side data size
        AVPacketSideData* ptr = _packet->side_data;
        for (int i = 0; i < _packet->side_data_elems; i++)
        {
            totalSize += avpsdSize;
            totalSize += ptr->size;
            ptr++;
        }
        auto bytes = std::make_unique<uchar[]>(totalSize);
        size_t memPos = 0;
        
        std::copy_n((uchar*)_packet, avpSize, bytes.get());
        memPos += avpSize;
        std::copy_n(_packet->data, _packet->size, bytes.get() + memPos);
        memPos += _packet->size;
        ptr = _packet->side_data;
        for (int i = 0; i < _packet->side_data_elems; i++)
        {
            std::copy_n((uchar*)ptr, avpsdSize, bytes.get() + memPos);
            memPos += avpsdSize;
            std::copy_n(ptr->data, ptr->size, bytes.get() + memPos);
            memPos += ptr->size;
            ptr++;
        }

        return { totalSize, bytes };
    }

    SerializedData _SerializeRemainingFields() const
    {
        size_t thisSize = sizeof(*this);
        size_t fieldSize = thisSize - sizeof(_packet);
        auto bytes = std::make_unique<uchar[]>(fieldSize);
        size_t memPos = sizeof(_packet);

        std::copy_n((uchar*)this + memPos, fieldSize, bytes.get());
        memPos += fieldSize;

        return { fieldSize, bytes };
    }

    size_t _DeserializeAVPacket(uchar* data, size_t dataSize)
    {
        size_t avpSize = sizeof(AVPacket);
        size_t avpsdSize = sizeof(AVPacketSideData);
        size_t memPos = 0;

        AVPacket* packet = nullptr;
        AVPacketSideData* sideData = nullptr;

        try
        {
            packet = (AVPacket*)av_mallocz(avpSize);
            _SafeCopy(data, memPos, (uchar*)packet, avpSize, dataSize);
            memPos += avpSize;
            packet->data = (uint8_t*)av_malloc(packet->size);
            _SafeCopy(data, memPos, packet->data, packet->size, dataSize);
            memPos += packet->size;

            if (packet->side_data_elems > 0)
            {
                sideData = (AVPacketSideData*)av_mallocz(avpsdSize * packet->side_data_elems);
                for (int i = 0; i < packet->side_data_elems; i++)
                {
                    _SafeCopy(data, memPos, (uchar*)&sideData[i], avpsdSize, dataSize);
                    memPos += avpsdSize;
                    sideData[i].data = (uint8_t*)av_malloc(sideData[i].size);
                    _SafeCopy(data, memPos, sideData[i].data, sideData[i].size, dataSize);
                    memPos += sideData[i].size;
                }
            }
            packet->side_data = sideData;
            packet->buf = nullptr;

            _packet = packet;
        }
        catch (std::out_of_range)
        {
            // Clean up side data
            if (sideData)
            {
                for (int i = 0; i < packet->side_data_elems; i++)
                {
                    if (sideData[i].data)
                    {
                        av_free(sideData[i].data);
                    }
                    else
                    {
                        break;
                    }
                }
                av_free(sideData);
            }

            // Clean up packet
            if (packet->data)
            {
                av_free(packet->data);
            }
            av_packet_free(&packet);

            return 0;
        }

        return memPos;
    }

    size_t _DeserializeRemainingFields(uchar* data, size_t dataSize)
    {
        size_t thisSize = sizeof(*this);
        size_t fieldSize = thisSize - sizeof(_packet);
        
        try
        {
            AVPacket* copy = _packet;
            _SafeCopy(data, 0, (uchar*)this + sizeof(_packet), fieldSize, dataSize);
            _packet = copy;
        }
        catch (std::out_of_range)
        {
            return 0;
        }

        return fieldSize;
    }

    // Throws an exception if copy will result in an access violation
    void _SafeCopy(const uchar* src, size_t offset, uchar* dest, size_t count, size_t allocated)
    {
        if (offset + count > allocated) throw std::out_of_range("Possible access violation");
        std::copy_n(src + offset, count, dest);
    }
};