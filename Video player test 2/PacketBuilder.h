#pragma once

#include <memory>

// Used to conveniently construct a byte array from various values
class PacketBuilder
{
public:
    // 'lessReinit' = true, reallocs exponentially more memory to reduce reallocation count
    PacketBuilder(size_t initialSize = 32, bool lessReinit = true)
    {
        _size = initialSize;
        _lessReinit = lessReinit;
        _bytes = std::make_unique<int8_t[]>(_size);
    }

    size_t UsedBytes() const
    {
        return _position;
    }

    size_t AllocatedBytes() const
    {
        return _size;
    }

    std::unique_ptr<int8_t[]> Release()
    {
        return std::move(_bytes);
    }

    template<typename T>
    PacketBuilder& Add(T value)
    {
        // Resize
        if (_position + sizeof(T) > _size)
            _Resize(_position + sizeof(T));

        *(T*)(_bytes.get() + _position) = value;
        _position += sizeof(T);

        return *this;
    }

    template<typename T>
    PacketBuilder& Add(const T* startPos, size_t count)
    {
        size_t copySize = sizeof(T) * count;

        // Resize
        if (_position + copySize > _size)
            _Resize(_position + copySize);

        std::copy_n(startPos, count, (T*)(_bytes.get() + _position));
        _position += copySize;

        return *this;
    }

private:
    std::unique_ptr<int8_t[]> _bytes = nullptr;
    size_t _size = 0;
    size_t _position = 0;
    bool _lessReinit = false;

    void _Resize(size_t minNewSize)
    {
        size_t newSize = 0;
        if (_lessReinit)
        {
            newSize = _size * 2;
            while (newSize < minNewSize)
                newSize *= 2;
        }
        else
        {
            newSize = minNewSize;
        }

        auto newBytes = std::make_unique<int8_t[]>(newSize);
        std::copy_n(_bytes.get(), _position, newBytes.get());
        _bytes.reset(newBytes.release());
        _size = newSize;
    }
};

// Used to conveniently deconstruct a byte array into various values
class PacketReader
{
public:
    PacketReader(int8_t* dataSource, size_t size)
    {
        _sourceBytes = dataSource;
        _size = size;
    }

    size_t RemainingBytes() const
    {
        return _size - _position;
    }

    template<typename T>
    T Get()
    {
        if (_position + sizeof(T) > _size)
            return 0;

        T value = *(T*)(_sourceBytes + _position);
        _position += sizeof(T);
        return value;
    }

    template<typename T>
    bool Get(T* startPos, size_t count)
    {
        size_t copySize = sizeof(T) * count;
        if (_position + copySize > _size)
            return false;

        std::copy_n((T*)(_sourceBytes + _position), count, startPos);
        _position += copySize;
        return true;
    }

private:
    int8_t* _sourceBytes = nullptr;
    size_t _size = 0;
    size_t _position = 0;
};