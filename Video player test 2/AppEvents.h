#pragma once

#include <vector>
#include <mutex>

template<class _Event>
class EventSubscriber;

class AppEvents
{
    // Packet subscription
private:
    template<class _Event>
    friend class EventSubscriber;

    struct _EventSubscription
    {
        uint64_t eventsRaised;
        const char* eventName;
        std::vector<void*> subs;
    };
    std::vector<_EventSubscription> _subscriptions;
    std::mutex _m_subscriptions;

    template<class _Event>
    void _Subscribe(EventSubscriber<_Event>* sub);

    template<class _Event>
    void _Unsubscribe(EventSubscriber<_Event>* sub);

public:
    template<class _Event>
    void RaiseEvent(_Event ev);
};

#include "EventSubscriber.h"

template<class _Event>
void AppEvents::_Subscribe(EventSubscriber<_Event>* sub)
{
    std::unique_lock<std::mutex> lock(_m_subscriptions);
    for (auto subscription = _subscriptions.begin(); subscription != _subscriptions.end(); subscription++)
    {
        if (subscription->eventName == _Event::_NAME_())
        {
            auto subscriber = std::find(subscription->subs.begin(), subscription->subs.end(), (void*)sub);
            if (subscriber == subscription->subs.end())
            {
                subscription->subs.push_back((void*)sub);
            }
            return;
        }
    }
    _subscriptions.push_back({ 0, _Event::_NAME_(), { (void*)sub } });
}

template<class _Event>
void AppEvents::_Unsubscribe(EventSubscriber<_Event>* sub)
{
    std::unique_lock<std::mutex> lock(_m_subscriptions);
    for (auto subscription = _subscriptions.begin(); subscription != _subscriptions.end(); subscription++)
    {
        if (subscription->eventName == _Event::_NAME_())
        {
            auto subscriber = std::find(subscription->subs.begin(), subscription->subs.end(), (void*)sub);
            if (subscriber != subscription->subs.end())
            {
                subscription->subs.erase(subscriber);
                if (subscription->subs.empty())
                {
                    _subscriptions.erase(subscription);
                }
            }
            return;
        }
    }
}

template<class _Event>
void AppEvents::RaiseEvent(_Event ev)
{
    std::lock_guard<std::mutex> lock(_m_subscriptions);
    for (int i = 0; i < _subscriptions.size(); i++)
    {
        if (_subscriptions[i].eventName == _Event::_NAME_())
        {
            // Send event to subscribers
            for (int j = 0; j < _subscriptions[i].subs.size(); j++)
            {
                EventSubscriber<_Event>* subscriber = static_cast<EventSubscriber<_Event>*>(_subscriptions[i].subs[j]);
                subscriber->_OnEvent(ev);
            }

            // Move often called events closer to vector start
            _subscriptions[i].eventsRaised++;
            if (i > 0 && _subscriptions[i].eventsRaised > _subscriptions[i - 1].eventsRaised)
                std::swap(_subscriptions[i], _subscriptions[i - 1]);

            break;
        }
    }
}