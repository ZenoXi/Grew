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

protected:
    void _Seek(TimePoint time);
    void _SetVideoStream(int index);
    void _SetAudioStream(int index);
    void _SetSubtitleStream(int index);
private:
    void _ReadPackets();
};