#pragma once

#include "GameTime.h"

#include <vector>
#include <string>


class IPlaybackController
{
public:
    virtual void Update() = 0;

    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void SetVolume(float volume) = 0;
    virtual void SetBalance(float balance) = 0;
    virtual void Seek(TimePoint time) = 0;
    virtual void SetVideoStream(int index) = 0;
    virtual void SetAudioStream(int index) = 0;
    virtual void SetSubtitleStream(int index) = 0;

    virtual bool Paused() = 0;
    virtual float GetVolume() const = 0;
    virtual float GetBalance() const = 0;
    virtual TimePoint CurrentTime() const = 0;
    virtual int CurrentVideoStream() const = 0;
    virtual int CurrentAudioStream() const = 0;
    virtual int CurrentSubtitleStream() const = 0;
    virtual Duration GetDuration() const = 0;

    virtual std::vector<std::string> GetAvailableVideoStreams() const = 0;
    virtual std::vector<std::string> GetAvailableAudioStreams() const = 0;
    virtual std::vector<std::string> GetAvailableSubtitleStreams() const = 0;

};