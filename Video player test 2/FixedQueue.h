#pragma once

#include <memory>

template<typename T>
class FixedQueue
{
    T* _data = nullptr;
    size_t _head = 0;
    size_t _size = 0;
public:
    FixedQueue() {};
    FixedQueue(size_t size)
    {
        _data = new T[size]();
        _head = 0;
        _size = size;
    }
    FixedQueue(const FixedQueue& other)
    {
        _data = new T[other._size]();
        _head = other._head;
        _size = other._size;

        std::copy_n(other._data, other._size, _data);
    }
    FixedQueue& operator= (const FixedQueue& other)
    {
        if (this != &other)
        {
            delete[] _data;

            _data = new T[other._size]();
            _head = other._head;
            _size = other._size;

            std::copy_n(other._data, other._size, _data);
        }
        return *this;
    }
    FixedQueue(FixedQueue&& other) noexcept
    {
        _data = other._data;
        _head = other._head;
        _size = other._size;

        other._data = nullptr;
        other._head = 0;
        other._size = 0;
    }
    FixedQueue& operator= (const FixedQueue&& other) noexcept
    {
        if (this != &other)
        {
            _data = other._data;
            _head = other._head;
            _size = other._size;

            other._data = nullptr;
            other._head = 0;
            other._size = 0;
        }
        return *this;
    }
    ~FixedQueue()
    {
        delete[] _data;
    }

    void Fill(const T& value)
    {
        std::fill_n(_data, _size, value);
        _head = 0u;
    }

    size_t Size() const
    {
        return _size;
    }

    void Push(const T& value)
    {
        _data[_head] = value;
        _head++;
        _head %= _size;
    }

    // Index of 0 is the oldest element
    T& operator[] (size_t index)
    {
        index = (index + _head) % _size;
        return _data[index];
    }

    // Returns the oldest element
    T& Front()
    {
        return this->operator[](0);
    }

    // Returns the most recently inserted element
    T& Back()
    {
        return this->operator[](Size() - 1);
    }
};            