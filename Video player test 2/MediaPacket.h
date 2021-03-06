#pragma once

#include "ISerializable.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <atomic>

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
        Reset();
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
            Reset();

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

    MediaPacket Reference() const
    {
        AVPacket* avpkt = nullptr;
        if (_packet)
        {
            avpkt = av_packet_alloc();
            av_packet_ref(avpkt, _packet);
        }
        MediaPacket newPacket(avpkt);
        newPacket.last = last;
        newPacket.flush = flush;
        return newPacket;
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

        return { totalSize, std::move(bytes) };
    }

    size_t Deserialize(SerializedData data)
    {
        Reset();

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

    void Reset()
    {
        if (_packet)
        {
            //struct AVBufferFake
            //{
            //    uint8_t* data;
            //    uint32_t size;
            //    std::atomic_uint refcount;
            //    void (*free)(void* opaque, uint8_t* data);
            //    void* opaque;
            //    int flags;
            //    int flags_internal;
            //};
            //AVBufferFake* buf = (AVBufferFake*)_packet->buf->buffer;
            if (!_packet->buf)
            {
                av_free(_packet->data);
            }
            av_packet_free(&_packet);
            _packet = nullptr;
        }
        last = 0;
        flush = 0;
    }

    void ResetBad()
    {
        _packet = nullptr;
        last = 0;
        flush = 0;
    }

private:
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

        return { totalSize, std::move(bytes) };
    }

    SerializedData _SerializeRemainingFields() const
    {
        size_t thisSize = sizeof(*this);
        size_t fieldSize = thisSize - sizeof(_packet);
        auto bytes = std::make_unique<uchar[]>(fieldSize);
        size_t memPos = sizeof(_packet);

        std::copy_n((uchar*)this + memPos, fieldSize, bytes.get());
        memPos += fieldSize;

        return { fieldSize, std::move(bytes) };
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
            packet = av_packet_alloc();
            //av_packet_free_side_data();
            //packet = (AVPacket*)av_mallocz(avpSize);
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
                    sideData[i].data = (uint8_t*)av_mallocz(sideData[i].size + AV_INPUT_BUFFER_PADDING_SIZE);
                    _SafeCopy(data, memPos, sideData[i].data, sideData[i].size, dataSize);
                    memPos += sideData[i].size;
                }
            }
            packet->side_data = sideData;
            packet->buf = nullptr;

            // 'av_packet_make_refcounted' for some fucking reason just copies packet->data
            // and overwrites the pointer to it without deletion, resulting in a memory leak.
            // I'm probably using av_packet_make_refcounted() wrong but w/e
            uint8_t* data = packet->data;
            av_packet_make_refcounted(packet);
            av_free(data);

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