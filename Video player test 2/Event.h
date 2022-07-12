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
public:
    Event() {}

    void Add(const std::function<_Ret(_Types...)>& handler, EventInfo info = { nullptr, "" })
    {
        _handlers.push_back(handler);
        _info.push_back(info);
        _remove.push_back(false);
    }

    void Remove(EventInfo info)
    {
        if (info.ownerPtr == nullptr && info.name == "")
            return;

        for (int i = 0; i < _info.size(); i++)
        {
            if (_info[i].ownerPtr == info.ownerPtr && _info[i].name == info.name)
            {
                if (!_locked)
                {
                    _handlers.erase(_handlers.begin() + i);
                    _info.erase(_info.begin() + i);
                    _remove.erase(_remove.begin() + i);
                }
                else
                {
                    _handlers[i] = nullptr;
                    _info[i] = { nullptr, "" };
                    _remove[i] = true;
                }
                break;
            }
        }
    }

    // Removing a handler while locked doesn't remove anything
    // from the vector to prevent out of bounds access. The handler
    // is reset, and won't be called.
    // LOCKING DOES NOT PROVIDE ANY THREAD SAFETY (bad naming i guess)
    void Lock()
    {
        _locked = true;
    }

    // Applies the handler removals which happened since calling Lock()
    void Unlock()
    {
        _locked = false;
        for (int i = 0; i < _remove.size(); i++)
        {
            if (_remove[i])
            {
                _handlers.erase(_handlers.begin() + i);
                _info.erase(_info.begin() + i);
                _remove.erase(_remove.begin() + i);
                i--;
            }
        }
    }

    void InvokeAll(_Types... args)
    {
        for (auto& handler : _handlers)
        {
            if (handler)
                handler(std::forward<_Types>(args)...);
        }
    }

    typedef typename std::vector<std::function<_Ret(_Types...)>>::iterator iterator;
    typedef typename std::vector<std::function<_Ret(_Types...)>>::const_iterator const_iterator;

    inline iterator begin() noexcept { return _handlers.begin(); }
    inline const_iterator cbegin() const noexcept { return _handlers.cbegin(); }
    inline iterator end() noexcept { return _handlers.end(); }
    inline const_iterator cend() const noexcept { return _handlers.cend(); }

private:
    std::vector<std::function<_Ret(_Types...)>> _handlers;
    std::vector<EventInfo> _info;

    // Locking
    bool _locked = false;
    std::vector<bool> _remove;
};