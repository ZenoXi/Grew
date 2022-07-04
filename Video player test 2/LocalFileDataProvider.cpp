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
    IMediaDataProvider::MediaData* mediaDataPtr;
    TimePoint time;
};

LocalFileDataProvider::LocalFileDataProvider(std::string filename) : _filename(filename)
{
    //_packetThreadController.Add("seek", sizeof(int64_t));
    _packetThreadController.Add("seek", sizeof(IMediaDataProvider::SeekData));
    _packetThreadController.Add("stream", sizeof(StreamChangeDesc));
    _packetThreadController.Add("stop", sizeof(bool));
    _packetThreadController.Add("eof", sizeof(bool));

    _initializing = true;
    _initializationThread = std::thread(&LocalFileDataProvider::_Initialize, this);
}

LocalFileDataProvider::LocalFileDataProvider(LocalFileDataProvider* other)
    : IMediaDataProvider(other)
{
    //_packetThreadController.Add("seek", sizeof(int64_t));
    _packetThreadController.Add("seek", sizeof(IMediaDataProvider::SeekData));
    _packetThreadController.Add("stream", sizeof(StreamChangeDesc));
    _packetThreadController.Add("stop", sizeof(bool));
    _packetThreadController.Add("eof", sizeof(bool));

    _filename = other->_filename;
}

LocalFileDataProvider::~LocalFileDataProvider()
{
    // Stop initialization
    if (Initializing())
        _abortInit = true;
    if (_initializationThread.joinable())
        _initializationThread.join();

    Stop();
    if (_avfContext)
    {
        avformat_close_input(&_avfContext);
        avformat_free_context(_avfContext);
    }
}

void LocalFileDataProvider::Start()
{
    _packetThreadController.Set("seek", IMediaDataProvider::SeekData());
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

void LocalFileDataProvider::_Seek(SeekData seekData)
{
    _packetThreadController.Set("seek", seekData);
}

void LocalFileDataProvider::_Seek(TimePoint time)
{
    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    _packetThreadController.Set("seek", seekData);
    //_packetThreadController.Set("seek", time.GetTime());
}

void LocalFileDataProvider::_SetVideoStream(int index, TimePoint time)
{
    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    seekData.videoStreamIndex = index;
    _packetThreadController.Set("seek", seekData);
    //_packetThreadController.Set("stream", StreamChangeDesc{ index, &_videoData, time });
}

void LocalFileDataProvider::_SetAudioStream(int index, TimePoint time)
{
    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    seekData.audioStreamIndex = index;
    _packetThreadController.Set("seek", seekData);
    //_packetThreadController.Set("stream", StreamChangeDesc{ index, &_audioData, time });
}

void LocalFileDataProvider::_SetSubtitleStream(int index, TimePoint time)
{
    IMediaDataProvider::SeekData seekData;
    seekData.time = time;
    seekData.subtitleStreamIndex = index;
    _packetThreadController.Set("seek", seekData);
    //_packetThreadController.Set("stream", StreamChangeDesc{ index, &_subtitleData, time });
}

void LocalFileDataProvider::_Initialize()
{
    std::cout << "Init started.." << std::endl;

    // Open file
    if (avformat_open_input(&_avfContext, _filename.c_str(), NULL, NULL) != 0)
    {
        _initFailed = true;
        _initializing = false;
        std::cout << "Init failed." << std::endl;
        return;
    }

    // Read chapters
    for (int i = 0; i < _avfContext->nb_chapters; i++)
    {
        std::string title = "";
        AVDictionaryEntry* t = NULL;
        while (t = av_dict_get(_avfContext->chapters[i]->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
        {
            if (t->key == std::string("title"))
            {
                title = t->value;
                break;
            }
        }

        TimePoint start = AV_NOPTS_VALUE;
        if (_avfContext->chapters[i]->start != AV_NOPTS_VALUE)
            start = av_rescale_q(_avfContext->chapters[i]->start, _avfContext->chapters[i]->time_base, { 1, 1000000000 });
        TimePoint end = AV_NOPTS_VALUE;
        if (_avfContext->chapters[i]->end != AV_NOPTS_VALUE)
            end = av_rescale_q(_avfContext->chapters[i]->end, _avfContext->chapters[i]->time_base, { 1, 1000000000 });

        _chapters.push_back(
        {
            _avfContext->chapters[i]->id,
            start,
            end,
            title
        });
    }

    // Process file
    MediaFileProcessing fprocessor(_avfContext);
    if (!_abortInit)
    {
        fprocessor.ExtractStreams();
        while (fprocessor.TaskRunning())
        {
            if (_abortInit)
            {
                fprocessor.CancelTask();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    if (!_abortInit)
    {
        fprocessor.FindMissingStreamData();
        while (fprocessor.TaskRunning())
        {
            if (_abortInit)
            {
                fprocessor.CancelTask();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    if (!_abortInit)
    {
        fprocessor.CalculateMissingStreamData();
        while (fprocessor.TaskRunning())
        {
            if (_abortInit)
            {
                fprocessor.CancelTask();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    _videoData.streams = std::move(fprocessor.videoStreams);
    _audioData.streams = std::move(fprocessor.audioStreams);
    _subtitleData.streams = std::move(fprocessor.subtitleStreams);
    _attachmentStreams = std::move(fprocessor.attachmentStreams);
    _dataStreams = std::move(fprocessor.dataStreams);
    _unknownStreams = std::move(fprocessor.unknownStreams);

    // Close file handle
    avformat_close_input(&_avfContext);

    if (!_videoData.streams.empty()) _videoData.currentStream = 0;
    if (!_audioData.streams.empty()) _audioData.currentStream = 0;
    if (!_subtitleData.streams.empty()) _subtitleData.currentStream = 0;

    _initializing = false;

    std::cout << "Init success!" << std::endl;
}

void LocalFileDataProvider::_ReadPackets()
{
    // Open file
    if (avformat_open_input(&_avfContext, _filename.c_str(), NULL, NULL) != 0)
        return; // TODO: show error notification

    AVPacket* packet = av_packet_alloc();
    bool holdPacket = false;
    bool eof = false;

    int videoStreamIndex = _videoData.currentStream != -1 ? _videoData.streams[_videoData.currentStream].index : -1;
    int audioStreamIndex = _audioData.currentStream != -1 ? _audioData.streams[_audioData.currentStream].index : -1;
    int subtitleStreamIndex = _subtitleData.currentStream != -1 ? _subtitleData.streams[_subtitleData.currentStream].index : -1;

    while (!_packetThreadController.Get<bool>("stop"))
    {
        // Check if seek is valid
        auto seekData = _packetThreadController.Get<IMediaDataProvider::SeekData>("seek");
        if (!seekData.Default())
        {
            _packetThreadController.Set("seek", IMediaDataProvider::SeekData());

            // Change stream
            if (seekData.videoStreamIndex != std::numeric_limits<int>::min())
            {
                _videoData.currentStream = seekData.videoStreamIndex;
                videoStreamIndex = _videoData.currentStream != -1 ? _videoData.streams[_videoData.currentStream].index : -1;
            }
            if (seekData.audioStreamIndex != std::numeric_limits<int>::min())
            {
                _audioData.currentStream = seekData.audioStreamIndex;
                audioStreamIndex = _audioData.currentStream != -1 ? _audioData.streams[_audioData.currentStream].index : -1;
            }
            if (seekData.subtitleStreamIndex != std::numeric_limits<int>::min())
            {
                _subtitleData.currentStream = seekData.subtitleStreamIndex;
                subtitleStreamIndex = _subtitleData.currentStream != -1 ? _subtitleData.streams[_subtitleData.currentStream].index : -1;
            }

            // Seek
            int64_t seekTime = seekData.time.GetTime(MICROSECONDS);
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
            _AddVideoPacket(MediaPacket(true));
            _AddAudioPacket(MediaPacket(true));
            _AddSubtitlePacket(MediaPacket(true));

            if (holdPacket)
            {
                av_packet_unref(packet);
                holdPacket = false;
            }
        }

        //// Seek
        //int64_t seekTime = _packetThreadController.Get<int64_t>("seek");
        //if (seekTime >= 0)
        //{
        //    _packetThreadController.Set("seek", -1LL);

        //    double seconds = seekTime / 1000000.0;
        //    if (seconds < 0) seconds = 0.0;
        //    std::cout << "Seeking to " << seconds << "s" << std::endl;

        //    avformat_seek_file(
        //        _avfContext,
        //        -1,
        //        std::numeric_limits<int64_t>::min(),
        //        seekTime,
        //        std::numeric_limits<int64_t>::max(),
        //        AVSEEK_FLAG_BACKWARD
        //    );

        //    // Clear packet buffers
        //    _ClearVideoPackets();
        //    _ClearAudioPackets();
        //    _ClearSubtitlePackets();

        //    // Send flush packets
        //    _AddVideoPacket(MediaPacket(true));
        //    _AddAudioPacket(MediaPacket(true));
        //    _AddSubtitlePacket(MediaPacket(true));

        //    if (holdPacket)
        //    {
        //        av_packet_unref(packet);
        //        holdPacket = false;
        //    }
        //}

        //// Change stream
        //StreamChangeDesc newStream = _packetThreadController.Get<StreamChangeDesc>("stream");
        //if (newStream.mediaDataPtr)
        //{
        //    _packetThreadController.Set("stream", StreamChangeDesc{ -1, nullptr, 0 });
        //    newStream.mediaDataPtr->currentStream = newStream.streamIndex;
        //    _packetThreadController.Set("seek", newStream.time.GetTime());

        //    // Update stream indexes
        //    videoStreamIndex = _videoData.currentStream != -1 ? _videoData.streams[_videoData.currentStream].index : -1;
        //    audioStreamIndex = _audioData.currentStream != -1 ? _audioData.streams[_audioData.currentStream].index : -1;
        //    subtitleStreamIndex = _subtitleData.currentStream != -1 ? _subtitleData.streams[_subtitleData.currentStream].index : -1;

        //    continue;
        //}

        // Read audio and video packets
        int result = 0;
        if (!holdPacket)
            result = av_read_frame(_avfContext, packet);

        if (result >= 0)
        {
            if (eof)
            {
                eof = false;
                _packetThreadController.Set("eof", false);
            }

            if (packet->stream_index == videoStreamIndex)
            {
                if (VideoMemoryExceeded())
                {
                    holdPacket = true;
                }
                else
                {
                    holdPacket = false;
                    _AddVideoPacket(MediaPacket(packet));
                    packet = av_packet_alloc();
                }
            }
            else if (packet->stream_index == audioStreamIndex)
            {
                if (AudioMemoryExceeded())
                {
                    holdPacket = true;
                }
                else
                {
                    holdPacket = false;
                    _AddAudioPacket(MediaPacket(packet));
                    packet = av_packet_alloc();
                }
            }
            else if (packet->stream_index == subtitleStreamIndex)
            {
                if (SubtitleMemoryExceeded())
                {
                    holdPacket = true;
                }
                else
                {
                    holdPacket = false;
                    _AddSubtitlePacket(MediaPacket(packet));
                    packet = av_packet_alloc();
                }
            }
            else
            {
                av_packet_unref(packet);
            }

            if (holdPacket)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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