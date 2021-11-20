#include "LocalFileDataProvider.h"

#include "MediaFileProcessing.h"

#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

struct StreamChangeDesc
{
    int streamIndex;
    MediaData* mediaDataPtr;
};

LocalFileDataProvider::LocalFileDataProvider(std::string filename) : _filename(filename)
{
    _packetThreadController.Add("seek", sizeof(int64_t));
    _packetThreadController.Add("stream", sizeof(StreamChangeDesc));
    _packetThreadController.Add("stop", sizeof(bool));
    _packetThreadController.Add("eof", sizeof(bool));

    _initializing = true;
    _initializationThread = std::thread(&LocalFileDataProvider::_Initialize, this);
    _initializationThread.detach();
}

LocalFileDataProvider::~LocalFileDataProvider()
{

}

void LocalFileDataProvider::Start()
{
    _packetThreadController.Set("seek", (int64_t)-1);
    _packetThreadController.Set("stream", StreamChangeDesc{ -1, nullptr });
    _packetThreadController.Set("stop", false);
    _packetThreadController.Set("eof", false);
    _packetReadingThread = std::thread(&LocalFileDataProvider::_ReadPackets, this);
}

void LocalFileDataProvider::_Seek(TimePoint time)
{
    _packetThreadController.Set("seek", time.GetTime());
}

void LocalFileDataProvider::_SetVideoStream(int index)
{

}

void LocalFileDataProvider::_SetAudioStream(int index)
{

}

void LocalFileDataProvider::_SetSubtitleStream(int index)
{

}

void LocalFileDataProvider::_Initialize()
{
    // Open file
    _avfContext = avformat_alloc_context();
    if (!_avfContext)
    {
        _initFailed = true;
        _initializing = false;
        return;
    }
    if (avformat_open_input(&_avfContext, _filename.c_str(), NULL, NULL) != 0)
    {
        _initFailed = true;
        _initializing = false;
        return;
    }

    // Process file
    MediaFileProcessing fprocessor(_avfContext);
    fprocessor.ExtractStreams();
    fprocessor.FindMissingStreamData();
    fprocessor.CalculateMissingStreamData();
    _videoData.streams = std::move(fprocessor.videoStreams);
    _audioData.streams = std::move(fprocessor.audioStreams);
    _subtitleData.streams = std::move(fprocessor.subtitleStreams);
    _attachmentStreams = std::move(fprocessor.attachmentStreams);
    _dataStreams = std::move(fprocessor.dataStreams);
    _unknownStreams = std::move(fprocessor.unknownStreams);

    _initializing = false;
}

void LocalFileDataProvider::_ReadPackets()
{
    AVPacket* packet = av_packet_alloc();
    bool eof = false;
    int64_t packetSize = 0;

    while (!_packetThreadController.Get<bool>("stop"))
    {
        // Seek
        int64_t seekTime = _packetThreadController.Get<int64_t>("seek");
        if (seekTime >= 0)
        {
            _packetThreadController.Set("seek", -1LL);

            double seconds = seekTime / 1000000.0;
            if (seconds < 0) seconds = 0.0;
            std::cout << "Seeking to " << seconds << "s" << std::endl;

            avformat_seek_file(
                _avfContext,
                -1,
                std::numeric_limits<int64_t>::min(),
                seekTime,
                std::numeric_limits<int64_t>::max(),
                AVSEEK_FLAG_BACKWARD
            );

            // Clear packet buffers
            _ClearVideoPackets();
            _ClearAudioPackets();
            _ClearSubtitlePackets();
        }

        // Read audio and video packets
        if (av_read_frame(_avfContext, packet) >= 0)
        {
            if (eof)
            {
                eof = false;
                _packetThreadController.Set("eof", false);
            }

            if (packet->stream_index == _videoData.streams[_videoData.currentStream].index)
            {
                std::unique_lock<std::mutex> lock(_videoData.mtx);
                _videoData.packets.push_back(MediaPacket(packet));
                packet = av_packet_alloc();
            }
            else if (packet->stream_index == _audioData.streams[_audioData.currentStream].index)
            {
                std::unique_lock<std::mutex> lock(_audioData.mtx);
                _audioData.packets.push_back(MediaPacket(packet));
                packet = av_packet_alloc();
            }
            else if (packet->stream_index == _subtitleData.streams[_subtitleData.currentStream].index)
            {
                std::unique_lock<std::mutex> lock(_subtitleData.mtx);
                _subtitleData.packets.push_back(MediaPacket(packet));
                packet = av_packet_alloc();
            }
        }
        else
        {
            if (!eof)
            {
                eof = true;
                _packetThreadController.Set("eof", true);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    av_packet_free(&packet);
}