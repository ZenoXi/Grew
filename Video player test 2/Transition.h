#pragma once

#include "GameTime.h"
#include <functional>

template<typename T>
class Transition
{
private:
    bool _transitioning = false;
    T _initialValue = 0;
    T _targetValue = 0;
    TimePoint _startTime = TimePoint(0);
    Duration _duration = Duration(0);
    std::function<float(float)> _transitionFunction;

public:
    Transition(Duration duration)
    {
        _duration = duration;
    }

    Transition(Duration duration, std::function<float(float)> function) : Transition(duration)
    {
        _transitionFunction = function;
    }

    void Start(T initialValue, T targetValue)
    {
        _initialValue = initialValue;
        _targetValue = targetValue;
        _startTime = ztime::Main();
        _transitioning = true;
    }

    void Abort()
    {
        _transitioning = false;
    }

    void Apply(T& var)
    {
        if (_transitioning)
        {
            float timeProgress = (ztime::Main() - _startTime).GetDuration() / (float)_duration.GetDuration();
            if (timeProgress >= 1.0f)
            {
                _transitioning = false;
                var = _targetValue;
            }
            else
            {
                float changeProgress = 0.0f;
                if (_transitionFunction)
                {
                    changeProgress = _transitionFunction(timeProgress);
                }
                else
                {
                    changeProgress = 1.0f - powf(timeProgress - 1.0f, 2.0f);
                }
                var = _initialValue + (_targetValue - _initialValue) * changeProgress;
            }
        }
    }

    void SetDuration(Duration duration)
    {
        _duration = duration;
    }
};