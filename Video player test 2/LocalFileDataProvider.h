#pragma once

#include "IMediaDataProvider.h"

#include "ThreadController.h"

#include <string>

struct LocalMediaSource
{
    enum StreamType
    {
        VIDEO_STREAM,
        AUDIO_STREAM,
        SUBTITLE_STREAM,
        OTHER
    };

    std::string filename = "";
    AVFormatContext* avfContext = nullptr;
    AVPacket* heldPacket = nullptr;

    // A map from media source stream (local) indices to the
    // data provider (global) stream indices (also includes stream type).
    std::vector<std::pair<int, StreamType>> LtoG_StreamIndex;
};

class LocalFileDataProvider : public IMediaDataProvider
{
    std::thread _initializationThread;

    std::string _filename;
    AVFormatContext* _avfContext = nullptr;
    std::thread _packetReadingThread;
    ThreadController _packetThreadController;

    bool _abortSourceAdd = false;
    std::thread _sourceAddThread;

    std::mutex _m_sources;
    std::vector<LocalMediaSource> _sources;
    std::vector<int> _videoStreamSourceIndex;
    std::vector<int> _audioStreamSourceIndex;
    std::vector<int> _subtitleStreamSourceIndex;

    bool _abortInit = false;

public:
    LocalFileDataProvider(std::string filename);
    LocalFileDataProvider(LocalFileDataProvider* other);
    ~LocalFileDataProvider();
private:
    void _Initialize();
public:
    void Start();
    void Stop();

    std::string GetFilename() const { return _filename; }

public:
    bool AddLocalMedia(std::string path, int streams = STREAM_SELECTION_ALL);
private:
    void _AddLocalMedia(std::string path, int streams);
protected:
    void _Seek(SeekData seekData);
    void _Seek(TimePoint time);
    void _SetVideoStream(int index, TimePoint time);
    void _SetAudioStream(int index, TimePoint time);
    void _SetSubtitleStream(int index, TimePoint time);
private:
    void _ReadPackets();
};