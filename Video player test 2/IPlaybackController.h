#pragma once

#include "GameTime.h"

#include <vector>
#include <string>

class IPlaybackController
{
public:
    struct LoadingInfo
    {
        bool loading;
        std::wstring message;
    };

    virtual void Update() = 0;

    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void SetVolume(float volume, bool bounded = true) = 0;
    virtual void SetBalance(float balance, bool bounded = true) = 0;
    virtual void Seek(TimePoint time) = 0;
    virtual void SetVideoStream(int index) = 0;
    virtual void SetAudioStream(int index) = 0;
    virtual void SetSubtitleStream(int index) = 0;

    virtual bool Paused() const = 0; // The play button is in the 'paused' state. !Paused() does not always mean that the playback is running
    virtual LoadingInfo Loading() const = 0; // The playback cannot start because something needs to be done
    virtual bool Finished() const = 0; // The playback has reached the end
    virtual float GetVolume() const = 0;
    virtual float GetBalance() const = 0;
    virtual TimePoint CurrentTime() const = 0;
    virtual Duration GetDuration() const = 0;
    virtual Duration GetBufferedDuration() const = 0;

    virtual int CurrentVideoStream() const = 0;
    virtual int CurrentAudioStream() const = 0;
    virtual int CurrentSubtitleStream() const = 0;
    virtual std::vector<std::string> GetAvailableVideoStreams() = 0;
    virtual std::vector<std::string> GetAvailableAudioStreams() = 0;
    virtual std::vector<std::string> GetAvailableSubtitleStreams() = 0;

};