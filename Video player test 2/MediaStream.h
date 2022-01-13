#pragma once

#include "ISerializable.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <string>
#include <vector>
#include <stdexcept>

enum class MediaStreamType
{
    NONE,
    VIDEO,
    AUDIO,
    SUBTITLE,
    ATTACHMENT,
    DATA,
    UNKNOWN
};

struct MediaMetadataPair
{
    std::string key;
    std::string value;
};

class MediaStream : public ISerializable
{
    AVCodecParameters* _params;
public:
    std::vector<MediaMetadataPair> metadata;
    int index;
    int packetCount;
    int64_t startTime;
    int64_t duration;
    AVRational timeBase;

    // Video properties
    int width;
    int height;

    // Audio properties
    int channels;
    int sampleRate;

    MediaStreamType type;

    MediaStream()
    {
        _params = nullptr;
        ResetFields();
    }
    // Deep copies the AVCodecParameters struct
    MediaStream(const AVCodecParameters* params) : MediaStream()
    {
        _params = avcodec_parameters_alloc();
        avcodec_parameters_copy(_params, params);
    }
    MediaStream(const MediaStream& other) : MediaStream(other._params)
    {
        CopyFields(other);
    }
    MediaStream& operator=(const MediaStream& other)
    {
        if (this != &other)
        {
            _FreeParams();
            _params = avcodec_parameters_alloc();
            avcodec_parameters_copy(_params, other._params);
            CopyFields(other);
        }
        return *this;
    }
    MediaStream(MediaStream&& other) noexcept
    {
        _params = other._params;
        other._params = nullptr;
        CopyFields(other);
        other.ResetFields();
    }
    MediaStream& operator=(MediaStream&& other) noexcept
    {
        if (this != &other)
        {
            _FreeParams();
            _params = other._params;
            other._params = nullptr;
            CopyFields(other);
            other.ResetFields();
        }
        return *this;
    }
    ~MediaStream()
    {
        _FreeParams();
    }

    AVCodecParameters* GetParams() const
    {
        return _params;
    }

    SerializedData Serialize() const
    {
        if (!_params) return { };

        SerializedData paramBytes = _SerializeCodecParams();
        SerializedData otherBytes = _SerializeRemainingFields();

        size_t totalSize = 0;
        totalSize += paramBytes.Size();
        totalSize += otherBytes.Size();
        auto bytes = std::make_unique<uchar[]>(totalSize);
        size_t memPos = 0;

        std::copy_n(paramBytes.Bytes(), paramBytes.Size(), bytes.get() + memPos);
        memPos += paramBytes.Size();
        std::copy_n(otherBytes.Bytes(), otherBytes.Size(), bytes.get() + memPos);
        memPos += otherBytes.Size();

        return { totalSize, std::move(bytes) };
    }

    size_t Deserialize(SerializedData data)
    {
        _FreeParams();

        size_t usedBytes1 = _DeserializeCodecParams(data.Bytes(), data.Size());
        if (usedBytes1 == 0) return 0;
        size_t usedBytes2 = _DeserializeRemainingFields(data.Bytes() + usedBytes1, data.Size() - usedBytes1);
        if (usedBytes2 == 0) return 0;

        return usedBytes1 + usedBytes2;
    }

private:
    void _FreeParams()
    {
        if (_params)
        {
            avcodec_parameters_free(&_params);
            _params = nullptr;
        }
    }

public:
    void ResetFields()
    {
        index = 0;
        metadata.clear();
        packetCount = 0;
        startTime = AV_NOPTS_VALUE;
        duration = AV_NOPTS_VALUE;
        timeBase = { 0, 0 };
        width = 0;
        height = 0;
        channels = 0;
        sampleRate = 0;
        type = MediaStreamType::NONE;
    }

    void CopyFields(const MediaStream& other)
    {
        index = other.index;
        metadata = other.metadata;
        packetCount = other.packetCount;
        startTime = other.startTime;
        duration = other.duration;
        timeBase = other.timeBase;
        width = other.width;
        height = other.height;
        channels = other.channels;
        sampleRate = other.sampleRate;
        type = other.type;
    }

private:
    SerializedData _SerializeCodecParams() const
    {
        size_t avcpSize = sizeof(AVCodecParameters);
        size_t totalSize = avcpSize + _params->extradata_size;
        auto bytes = std::make_unique<uchar[]>(totalSize);
        size_t memPos = 0;

        std::copy_n((uchar*)_params, avcpSize, bytes.get() + memPos);
        memPos += avcpSize;
        std::copy_n(_params->extradata, _params->extradata_size, bytes.get() + memPos);
        memPos += _params->extradata_size;

        return { totalSize, std::move(bytes) };
    }

    SerializedData _SerializeRemainingFields() const
    {
        /// <summary>
        /// serialized 'metadata' contains, in order:
        /// X - a 64 bit number, vector element count
        /// X times:
        ///     N - a 64 bit number, string character count
        ///     a N byte length data block of the string data
        ///     M - a 64 bit number, string character count
        ///     a M byte length data block of the string data
        /// </summary>
        size_t metadataSize = sizeof(size_t);
        for (size_t i = 0; i < metadata.size(); i++)
        {
            metadataSize += sizeof(size_t);
            metadataSize += metadata[i].key.length();
            metadataSize += sizeof(size_t);
            metadataSize += metadata[i].value.length();
        }

        size_t thisSize = sizeof(*this);
        size_t fieldOffset = (size_t)&index - (size_t)this;
        size_t fieldSize = thisSize - fieldOffset;
        size_t totalSize = fieldSize + metadataSize;
        auto bytes = std::make_unique<uchar[]>(totalSize);
        size_t memPos = 0;

        std::copy_n((uchar*)this + fieldOffset, fieldSize, bytes.get() + memPos);
        memPos += fieldSize;
        // Serialize 'metadata'
        size_t metadataCount = metadata.size();
        std::copy_n((uchar*)&metadataCount, sizeof(metadataCount), bytes.get() + memPos);
        memPos += sizeof(metadataCount);
        for (size_t i = 0; i < metadata.size(); i++)
        {
            // Key
            size_t keyLength = metadata[i].key.length();
            std::copy_n((uchar*)&keyLength, sizeof(keyLength), bytes.get() + memPos);
            memPos += sizeof(keyLength);
            std::copy_n((uchar*)metadata[i].key.data(), keyLength, bytes.get() + memPos);
            memPos += keyLength;
            // Value
            size_t valueLength = metadata[i].value.length();
            std::copy_n((uchar*)&valueLength, sizeof(valueLength), bytes.get() + memPos);
            memPos += sizeof(valueLength);
            std::copy_n((uchar*)metadata[i].value.data(), valueLength, bytes.get() + memPos);
            memPos += valueLength;
        }

        return { totalSize, std::move(bytes) };
    }

    size_t _DeserializeCodecParams(uchar* data, size_t dataSize)
    {
        size_t avcpSize = sizeof(AVCodecParameters);
        size_t memPos = 0;

        AVCodecParameters* params = nullptr;

        try
        {
            params = avcodec_parameters_alloc();
            _SafeCopy(data, memPos, (uchar*)params, avcpSize, dataSize);
            memPos += avcpSize;
            params->extradata = (uint8_t*)av_mallocz(params->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            _SafeCopy(data, memPos, params->extradata, params->extradata_size, dataSize);
            memPos += params->extradata_size;

            _params = params;
        }
        catch (std::out_of_range)
        {
            // Free extradata
            if (params->extradata)
            {
                av_freep(&params->extradata);
            }

            // Free codec parameters
            avcodec_parameters_free(&params);

            return 0;
        }

        return memPos;
    }

    size_t _DeserializeRemainingFields(uchar* data, size_t dataSize)
    {
        size_t thisSize = sizeof(*this);
        size_t fieldOffset = (size_t)&index - (size_t)this;
        size_t fieldSize = thisSize - fieldOffset;
        size_t memPos = 0;

        // Create a copy of current fields in case deserialization fails
        MediaStream stream;
        stream.CopyFields(*this);

        try
        {
            _SafeCopy(data, memPos, (uchar*)this + fieldOffset, fieldSize, dataSize);
            memPos += fieldSize;
            // Deserialize 'metadata'
            metadata.clear();
            size_t metadataCount;
            _SafeCopy(data, memPos, (uchar*)&metadataCount, sizeof(metadataCount), dataSize);
            memPos += sizeof(metadataCount);
            for (size_t i = 0; i < metadataCount; i++)
            {
                MediaMetadataPair pair;
                // Key
                size_t keyLength;
                _SafeCopy(data, memPos, (uchar*)&keyLength, sizeof(keyLength), dataSize);
                memPos += sizeof(keyLength);
                pair.key.resize(keyLength);
                _SafeCopy(data, memPos, (uchar*)pair.key.data(), keyLength, dataSize);
                memPos += keyLength;
                // Value
                size_t valueLength;
                _SafeCopy(data, memPos, (uchar*)&valueLength, sizeof(valueLength), dataSize);
                memPos += sizeof(valueLength);
                pair.value.resize(valueLength);
                _SafeCopy(data, memPos, (uchar*)pair.value.data(), valueLength, dataSize);
                memPos += valueLength;
                metadata.push_back(pair);
            }
        }
        catch (std::out_of_range)
        {
            // Return fields to old state
            CopyFields(stream);

            return 0;
        }

        return memPos;
    }

    // Throws an exception if copy will result in an access violation
    void _SafeCopy(const uchar* src, size_t offset, uchar* dest, size_t count, size_t allocated)
    {
        if (offset + count > allocated) throw std::out_of_range("Possible access violation");
        std::copy_n(src + offset, count, dest);
    }
};