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
    TimePoint time;
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
    Stop();
    avformat_close_input(&_avfContext);
    avformat_free_context(_avfContext);
}

void LocalFileDataProvider::Start()
{
    _packetThreadController.Set("seek", (int64_t)-1);
    _packetThreadController.Set("stream", StreamChangeDesc{ -1, nullptr, 0 });
    _packetThreadController.Set("stop", false);
    _packetThreadController.Set("eof", false);
    _packetReadingThread = std::thread(&LocalFileDataProvider::_ReadPackets, this);
}

void LocalFileDataProvider::Stop()
{
    _packetThreadController.Set("stop", true);
    if (_packetReadingThread.joinable())
    {
        _packetReadingThread.join();
    }
}

void LocalFileDataProvider::_Seek(TimePoint time)
{
    _packetThreadController.Set("seek", time.GetTime());
}

void LocalFileDataProvider::_SetVideoStream(int index, TimePoint time)
{
    _packetThreadController.Set("stream", StreamChangeDesc{ index, &_videoData, time });
}

void LocalFileDataProvider::_SetAudioStream(int index, TimePoint time)
{
    _packetThreadController.Set("stream", StreamChangeDesc{ index, &_audioData, time });
}

void LocalFileDataProvider::_SetSubtitleStream(int index, TimePoint time)
{
    _packetThreadController.Set("stream", StreamChangeDesc{ index, &_subtitleData, time });
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

            // Send flush packets
            _videoData.packets.push_back(MediaPacket(true));
            _audioData.packets.push_back(MediaPacket(true));
            _subtitleData.packets.push_back(MediaPacket(true));
        }

        // Change stream
        StreamChangeDesc newStream = _packetThreadController.Get<StreamChangeDesc>("stream");
        if (newStream.mediaDataPtr)
        {
            _packetThreadController.Set("stream", StreamChangeDesc{ -1, nullptr, 0 });
            newStream.mediaDataPtr->currentStream = newStream.streamIndex;
            _packetThreadController.Set("seek", newStream.time.GetTime());
            continue;
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