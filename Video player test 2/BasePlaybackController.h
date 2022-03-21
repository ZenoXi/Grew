#pragma once

#include "IPlaybackController.h"

#include "MediaPlayer.h"
#include "IMediaDataProvider.h"

#include <set>

class BasePlaybackController : public IPlaybackController
{
protected:
    class _TimerController
    {
        struct _PauseTimer
        {
            Clock timer;
            Duration left;
        };

        MediaPlayer* _player;

        // Main pause flag.
        // If 'true' the playback will always stay paused.
        // If 'false' the playback will go on when no other stops are in place.
        bool _paused = false;
        // Playback stop flags
        // If the set in not empty, the playback will always stay paused.
        // If the set is empty, the playback will go on when no other stops are in place.
        std::set<std::string> _stops;
        // Playback stop timers. Like '_stops', but automatically expire after their specified duration
        // If the vector in not empty, the playback will always stay paused.
        // If the vector is empty, the playback will go on when no other stops are in place.
        std::vector<_PauseTimer> _timers;
        // Playback stop timers. Like '_timers', but only count down while no other stops are in place.
        std::vector<_PauseTimer> _playTimers;

    public:
        _TimerController(MediaPlayer* player, bool paused)
        {
            _player = player;
            _paused = paused;
        }

        void Update()
        {
            // Update timers
            for (auto it = _timers.begin(); it != _timers.end();)
            {
                it->timer.Update();
                if (it->timer.Now().GetTicks() > it->left)
                {
                    it = _timers.erase(it);
                }
                else
                {
                    it++;
                }
            }
            for (auto it = _playTimers.begin(); it != _playTimers.end();)
            {
                it->timer.Update();
                if (it->timer.Now().GetTicks() > it->left)
                {
                    it = _playTimers.erase(it);
                }
                else
                {
                    it++;
                }
            }

            // Update state
            _UpdateState();
        }

        void Pause()
        {
            if (!_paused)
            {
                _paused = true;
                _UpdateState();
            }
        }

        void Play()
        {
            if (_paused)
            {
                _paused = false;
                _UpdateState();
            }
        }

        bool Paused() const
        {
            return _paused;
        }

        bool Stopped() const
        {
            return !_stops.empty() || !_timers.empty() || !_playTimers.empty();
        }

        void AddStop(std::string name)
        {
            _stops.insert(name);
            _UpdateState();
        }

        void RemoveStop(std::string name)
        {
            auto it = _stops.find(name);
            if (it != _stops.end())
                _stops.erase(it);
            _UpdateState();
        }

        void AddTimer(Duration duration)
        {
            _PauseTimer timer = { Clock(), duration };
            _timers.push_back(timer);
            _UpdateState();
        }

        void AddPlayTimer(Duration duration)
        {
            _PauseTimer timer = { Clock(), duration };
            timer.timer.Stop();
            _playTimers.push_back(timer);
            _UpdateState();
        }

        void ClearTimers()
        {
            _timers.clear();
            _UpdateState();
        }

        void ClearPlayTimers()
        {
            _playTimers.clear();
            _UpdateState();
        }

    private:
        void _UpdateState()
        {
            bool stopped = false;

            // Check all stops
            if (_paused)
                stopped = true;
            if (!_stops.empty())
                stopped = true;
            if (!_timers.empty())
                stopped = true;

            // Check play timers
            for (auto& timer : _playTimers)
            {
                if (!stopped)
                {
                    timer.timer.Update();
                    timer.timer.Start();
                }
                else
                {
                    timer.timer.Update();
                    timer.timer.Stop();
                }
            }
            if (!_playTimers.empty())
                stopped = true;

            // Start/stop playback
            if (!stopped)
                _player->StartTimer();
            else
                _player->StopTimer();
        }
    };

    MediaPlayer* _player;
    IMediaDataProvider* _dataProvider;

    bool _paused = true;
    bool _loading = true;
    bool _finished = false;

    float _volume = 0.1f;
    float _balance = 0.0f;

    std::vector<std::string> _videoStreams;
    std::vector<std::string> _audioStreams;
    std::vector<std::string> _subtitleStreams;
    int _currentVideoStream;
    int _currentAudioStream;
    int _currentSubtitleStream;

    _TimerController _timerController;

public:
    BasePlaybackController(MediaPlayer* player, IMediaDataProvider* dataProvider);

    void Update();

    void Play();
    void Pause();
    void SetVolume(float volume, bool bounded = true);
    void SetBalance(float balance, bool bounded = true);
    void Seek(TimePoint time);
    void SetVideoStream(int index);
    void SetAudioStream(int index);
    void SetSubtitleStream(int index);

    bool Paused() const;
    LoadingInfo Loading() const;
    bool Finished() const;
    float GetVolume() const;
    float GetBalance() const;
    TimePoint CurrentTime() const;
    Duration GetDuration() const;
    Duration GetBufferedDuration() const;

    int CurrentVideoStream() const;
    int CurrentAudioStream() const;
    int CurrentSubtitleStream() const;
    std::vector<std::string> GetAvailableVideoStreams() const;
    std::vector<std::string> GetAvailableAudioStreams() const;
    std::vector<std::string> GetAvailableSubtitleStreams() const;
};