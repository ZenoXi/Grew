#pragma once

class AppEvents;

template<class _Event>
class EventSubscriber
{
    friend class AppEvents;

    AppEvents* _eventClass;

    virtual void _OnEvent(_Event) = 0;
public:
    EventSubscriber(AppEvents* eventClass);
    EventSubscriber(const EventSubscriber<_Event>&) = delete;
    virtual ~EventSubscriber();
};

#include "AppEvents.h"

template<class _Event>
EventSubscriber<_Event>::EventSubscriber(AppEvents* eventClass) : _eventClass(eventClass)
{
    _eventClass->_Subscribe<_Event>(this);
}

template<class _Event>
EventSubscriber<_Event>::~EventSubscriber()
{
    _eventClass->_Unsubscribe<_Event>(this);
}

#include <queue>
#include <mutex>

// Thread safe helper receiver class
template<class _Event>
class EventReceiver : public EventSubscriber<_Event>
{
private:
    std::queue<_Event> _events;
    std::mutex _m_events;

    void _OnEvent(_Event ev)
    {
        std::lock_guard<std::mutex> lock(_m_events);
        _events.push(ev);
    }
public:
    EventReceiver(AppEvents* appEvents) : EventSubscriber<_Event>(appEvents) {}

    size_t EventCount()
    {
        std::lock_guard<std::mutex> lock(_m_events);
        return _events.size();
    }
    _Event GetEvent()
    {
        std::lock_guard<std::mutex> lock(_m_events);
        _Event ev = _events.front();
        _events.pop();
        return ev;
    }
};

template<class _Event>
class EventHandler : public EventSubscriber<_Event>
{
private:
    std::function<void(_Event)> _handler;

    void _OnEvent(_Event ev)
    {
        _handler(ev);
    }
public:
    EventHandler(AppEvents* appEvents, std::function<void(_Event)> handler)
        : EventSubscriber<_Event>(appEvents)
        , _handler(handler)
    {}
};