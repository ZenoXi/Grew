#pragma once

#include <vector>
#include <functional>

template<class _Ret, class... _Types>
class Event
{
    std::vector<std::function<_Ret(_Types...)>> _handlers;
    typedef typename std::vector<std::function<_Ret(_Types...)>>::iterator iterator;
    typedef typename std::vector<std::function<_Ret(_Types...)>>::const_iterator const_iterator;

public:
    Event() {}

    void Add(const std::function<_Ret(_Types...)>& handler)
    {
        _handlers.push_back(handler);
        //handler();
    }

    void InvokeAll(_Types... args)
    {
        for (auto& handler : _handlers)
        {
            handler(std::forward<_Types>(args)...);
        }
    }

    inline iterator begin() noexcept { return _handlers.begin(); }
    inline const_iterator cbegin() const noexcept { return _handlers.cbegin(); }
    inline iterator end() noexcept { return _handlers.end(); }
    inline const_iterator cend() const noexcept { return _handlers.cend(); }
};