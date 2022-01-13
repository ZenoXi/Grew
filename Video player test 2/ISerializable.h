#pragma once

#include <memory>
#include <limits>

typedef unsigned char uchar;

class SerializedData
{
    size_t _size;
    std::unique_ptr<uchar[]> _bytes;
public:
    SerializedData()
    {
        _size = 0;
        _bytes = nullptr;
    }
    SerializedData(size_t size, std::unique_ptr<uchar[]>&& bytes)
    {
        _size = size;
        _bytes = std::move(bytes);
    }
    SerializedData(SerializedData&& other) noexcept
    {
        _size = other._size;
        _bytes = std::move(other._bytes);
        other._size = 0;
    }
    SerializedData& operator=(SerializedData&& other) noexcept
    {
        if (this != &other)
        {
            _size = other._size;
            _bytes = std::move(other._bytes);
            other._size = 0;
        }
        return *this;
    }
    SerializedData(const SerializedData&) = delete;
    SerializedData& operator=(const SerializedData&) = delete;
    size_t Size() const
    {
        return _size;
    }
    uchar* Bytes() const
    {
        return _bytes.get();
    }
    SerializedData Copy(size_t startByte = 0, size_t count = std::numeric_limits<size_t>::max())
    {
        if (startByte >= _size) return { };
        if (startByte + count > _size) count = _size - startByte;

        auto bytes = std::make_unique<uchar[]>(count);
        std::copy_n(_bytes.get() + startByte, count, bytes.get());
        return { count, std::move(bytes) };
    }
};

class ISerializable
{
public:
    virtual SerializedData Serialize() const = 0;
    virtual size_t Deserialize(SerializedData) = 0;
};