#pragma once

#include <vector>
#include <functional>

struct EventInfo
{
    void* ownerPtr;
    std::string name;
};

template<class _Ret, class... _Types>
class Event
{
    std::vector<std::function<_Ret(_Types...)>> _handlers;
    std::vector<EventInfo> _info;
    typedef typename std::vector<std::function<_Ret(_Types...)>>::iterator iterator;
    typedef typename std::vector<std::function<_Ret(_Types...)>>::const_iterator const_iterator;

public:
    Event() {}

    void Add(const std::function<_Ret(_Types...)>& handler, EventInfo info = { nullptr, "" })
    {
        _handlers.push_back(handler);
        _info.push_back(info);
    }

    void Remove(EventInfo info)
    {
        if (info.ownerPtr == nullptr && info.name == "")
            return;

        for (int i = 0; i < _info.size(); i++)
        {
            if (_info[i].ownerPtr == info.ownerPtr && _info[i].name == info.name)
            {
                _handlers.erase(_handlers.begin() + i);
                _info.erase(_info.begin() + i);
                break;
            }
        }
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