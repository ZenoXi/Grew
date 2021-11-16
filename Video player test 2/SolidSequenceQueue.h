#pragma once

#include <list>

/// <summary>
/// This class is used to find keep track of the largest number before a
/// gap in a sequence <para/>
/// Example: 1 2 3 5 9 | 
/// Here the number is 3 because 5 and 9 cannot be reached without gaps <para/>
/// 
/// </summary>
class SolidSequenceQueue
{
    std::list<int64_t> _sequence;
    int64_t _num;
    int64_t _den;
    int64_t _tolerance;

public:
    SolidSequenceQueue(int64_t interval, int64_t tolerance)
    {
        _num = interval;
        _den = 1;
        _tolerance = tolerance;
    }

    SolidSequenceQueue(int64_t numerator, int64_t denominator, int64_t tolerance)
    {
        _num = numerator;
        _den = denominator;
        _tolerance = tolerance;
    }

    /// <summary>
    /// Returns false if number doesn't fit the sequence
    /// </summary>
    bool Add(int64_t number)
    {
        
    }
};

//class BufferTracker
//{
//    int _head = -1;
//    int _tail = -1;
//    std::deque<int> _values;
//    AVRational _framerate;
//    AVRational _timebase;
//public:
//    BufferTracker(AVRational framerate, AVRational timebase)
//    {
//        _framerate = framerate;
//        _timebase = timebase;
//    }
//
//    void AddTime(uint64_t time)
//    {
//        double rtime = (time / (double)_timebase.den) * _timebase.num;
//        int index = round(rtime * ((double)_framerate.num / _framerate.den));
//
//        if (_tail == -1)
//        {
//            _tail = index;
//            if (index == 0)
//            {
//                _head = 0;
//            }
//            return;
//        }
//
//        if (index > _tail)
//        {
//            if (_head != _tail)
//            {
//                _values.push_back(_tail);
//                _tail = index;
//            }
//            else
//            {
//                if (index == _head + 1)
//                {
//                    _head++;
//                    _tail++;
//                }
//                else
//                {
//                    _tail = index;
//                }
//            }
//        }
//        else
//        {
//            if (index == _head + 1)
//            {
//                _head++;
//                while (!_values.empty())
//                {
//                    if (_values.front() == _head + 1)
//                    {
//                        _values.pop_front();
//                        _head++;
//                    }
//                    else
//                    {
//                        break;
//                    }
//                }
//                if (_tail == _head + 1)
//                {
//                    _head++;
//                }
//            }
//            else
//            {
//                for (int i = 0; i < _values.size(); i++)
//                {
//                    if (_values[i] > index)
//                    {
//                        _values.insert(_values.begin() + i, index);
//                        return;
//                    }
//                }
//                _values.push_back(index);
//            }
//        }
//    }
//
//    uint64_t BufferedDuration()
//    {
//        if (_head == -1) return 0;
//        double rtime = (_head / (double)_framerate.num) * _framerate.den;
//        return round(rtime * ((double)_timebase.den / _timebase.num));
//    }
//};