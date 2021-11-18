#pragma once

#include "MediaStream.h"

#include <vector>

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
    // Extract the actual streams
    void ExtractStreams();
    // Find missing stream data in metadata
    void FindMissingStreamData();
    // Decode the first few (all if 'full' == true) of the packets to calculate/extract missing data
    void CalculateMissingStreamData(bool full = false);
};