#pragma once

#include "IMediaDataProvider.h"

#include "ThreadController.h"

#include <string>

class LocalFileDataProvider : public IMediaDataProvider
{
    std::thread _initializationThread;

    std::string _filename;
    AVFormatContext* _avfContext = nullptr;
    std::thread _packetReadingThread;
    ThreadController _packetThreadController;

public:
    LocalFileDataProvider(std::string filename);
    ~LocalFileDataProvider();
private:
    void _Initialize();
public:
    void Start();
    void Stop();

protected:
    void _Seek(TimePoint time);
    void _SetVideoStream(int index, TimePoint time);
    void _SetAudioStream(int index, TimePoint time);
    void _SetSubtitleStream(int index, TimePoint time);
private:
    void _ReadPackets();
};