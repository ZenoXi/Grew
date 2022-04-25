#pragma once

#include "MediaStream.h"

#include <vector>
#include <thread>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

struct MediaFileProcessing
{
    AVFormatContext* avfContext;
    std::vector<MediaStream> videoStreams;
    std::vector<MediaStream> audioStreams;
    std::vector<MediaStream> subtitleStreams;
    std::vector<MediaStream> attachmentStreams;
    std::vector<MediaStream> dataStreams;
    std::vector<MediaStream> unknownStreams;

    MediaFileProcessing(AVFormatContext* avfc) : avfContext(avfc) {}
    ~MediaFileProcessing() { if (_taskThread.joinable()) _taskThread.join(); }

    /*
    * Only 1 task can be running at once. Attemping to start a task
    * while another one is running will do nothing, and the running
    * task will continue until it finishes or is canceled.
    */

    // Extract the actual streams
    void ExtractStreams();
    // Find missing stream data in metadata
    void FindMissingStreamData();
    // Decode the first few (all if 'full' == true) of the packets to calculate/extract missing data
    void CalculateMissingStreamData(bool full = false);

private:
    std::thread _taskThread;
    bool _taskRunning = false;
    bool _cancelTask = false;

    void _ExtractStreams();
    void _FindMissingStreamData();
    void _CalculateMissingStreamData(bool full = false);
public:
    bool TaskRunning() const
    {
        return _taskRunning;
    }
    // Signals the task thread to stop and waits for its completion
    void CancelTask()
    {
        if (!_taskRunning)
            return;

        _cancelTask = true;
        if (_taskThread.joinable())
            _taskThread.join();
        _cancelTask = false;
    }
};