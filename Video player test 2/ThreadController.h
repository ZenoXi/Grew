#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

class ThreadController
{
    class Value
    {
        void* _data;
        size_t _size;

    public:
        Value(size_t size) : _size(size)
        {
            _data = malloc(_size);
        }
        ~Value()
        {
            free(_data);
        }
        Value(const Value& v)
        {
            _size = v._size;
            _data = malloc(_size);
            memcpy(_data, v._data, _size);
        }
        Value& operator=(const Value& v)
        {
            if (this != &v)
            {
                free(_data);
                _size = v._size;
                _data = malloc(_size);
                memcpy(_data, v._data, _size);
            }
            return *this;
        }
        Value(Value&& v) noexcept
        {
            _size = v._size;
            _data = v._data;
            v._size = 0;
            v._data = nullptr;
        }
        Value& operator=(Value&& v) noexcept
        {
            if (this != &v)
            {
                free(_data);
                _size = v._size;
                _data = v._data;
                v._size = 0;
                v._data = nullptr;
            }
            return *this;
        }

        size_t Size()
        {
            return _size;
        }
        template<typename T>
        T Get()
        {
            return *((T*)_data);
        }
        template<typename T>
        void Set(T value)
        {
            *(T*)_data = value;
        }
    };

    std::unordered_map<std::string, Value> _values;
    std::mutex _mutex;

public:
    ThreadController() {}

    void Add(std::string key, size_t size)
    {
        _mutex.lock();
        _values.emplace(key, Value(size));
        _mutex.unlock();
    }

    template<typename T>
    void Set(std::string key, T value)
    {
        _mutex.lock();
        auto val = _values.find(key);
        if (val != _values.end())
        {
            val->second.Set(value);
        }
        _mutex.unlock();
    }

    template<typename T>
    T Get(std::string key)
    {
        _mutex.lock();
        T val = T();
        auto it = _values.find(key);
        if (it != _values.end())
        {
            val = it->second.Get<T>();
        }
        _mutex.unlock();
        return val;
    }
};