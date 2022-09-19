#include "LocalFileDataProvider.h"

#include "App.h"
#include "OverlayScene.h"
#include "PlaybackEvents.h"

#include "MediaFileProcessing.h"

#include <set>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
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

    _sources = other->_sources;
    _videoStreamSourceIndex = other->_videoStreamSourceIndex;
    _audioStreamSourceIndex = other->_audioStreamSourceIndex;
    _subtitleStreamSourceIndex = other->_subtitleStreamSourceIndex;

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

    // Open all sources
    std::lock_guard lock(_m_sources);
    for (int i = 0; i < _sources.size(); i++)
    {
        if (avformat_open_input(&_sources[i].avfContext, _sources[i].filename.c_str(), NULL, NULL) != 0)
        {
            zcom::NotificationInfo ninfo;
            ninfo.borderColor = D2D1::ColorF(0.8f, 0.2f, 0.2f);
            ninfo.duration = Duration(5, SECONDS);
            ninfo.title = L"Failed to open media source";
            ninfo.text = L"The file '" + utf8_to_wstr(_sources[i].filename) + L"' could not be opened. The corresponding stream will contain no output";
            App::Instance()->Overlay()->ShowNotification(ninfo);
            return;
        }
    }

    _packetReadingThread = std::thread(&LocalFileDataProvider::_ReadPackets, this);
}

void LocalFileDataProvider::Stop()
{
    _packetThreadController.Set("stop", true);
    _abortSourceAdd = true;
    if (_packetReadingThread.joinable())
        _packetReadingThread.join();
    if (_sourceAddThread.joinable())
        _sourceAddThread.join();

    // Close open sources
    std::lock_guard lock(_m_sources);
    for (int i = 0; i < _sources.size(); i++)
    {
        if (_sources[i].avfContext)
            avformat_close_input(&_sources[i].avfContext);
    }
}

bool LocalFileDataProvider::AddLocalMedia(std::string path, int streams)
{
    _abortSourceAdd = false;
    _sourceAddThread = std::thread(&LocalFileDataProvider::_AddLocalMedia, this, path, streams);
    return true;
}

void LocalFileDataProvider::_AddLocalMedia(std::string path, int streams)
{
    AVFormatContext* context = nullptr;
    int result = avformat_open_input(&context, path.c_str(), NULL, NULL);
    if (result != 0)
    {
        zcom::NotificationInfo ninfo;
        ninfo.borderColor = D2D1::ColorF(0.8f, 0.2f, 0.2f);
        ninfo.duration = Duration(5, SECONDS);
        ninfo.title = L"Failed to add media";
        ninfo.text = L"The file could not be opened";
        App::Instance()->Overlay()->ShowNotification(ninfo);
        return;
    }

    // Process file
    MediaFileProcessing fprocessor(context);
    fprocessor.ignoreVideoStreams       = !(streams & STREAM_SELECTION_VIDEO);
    fprocessor.ignoreAudioStreams       = !(streams & STREAM_SELECTION_AUDIO);
    fprocessor.ignoreSubtitleStreams    = !(streams & STREAM_SELECTION_SUBTITLE);
    fprocessor.ignoreAttachmentStreams  = !(streams & STREAM_SELECTION_ATTACHMENT);
    fprocessor.ignoreDataStreams        = !(streams & STREAM_SELECTION_DATA);
    fprocessor.ignoreUnknownStreams     = !(streams & STREAM_SELECTION_UNKNOWN);

    if (!_abortSourceAdd)
    {
        fprocessor.ExtractStreams();
        while (fprocessor.TaskRunning())
        {
            if (_abortSourceAdd)
            {
                fprocessor.CancelTask();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    if (!_abortSourceAdd)
    {
        fprocessor.FindMissingStreamData();
        while (fprocessor.TaskRunning())
        {
            if (_abortSourceAdd)
            {
                fprocessor.CancelTask();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    if (!_abortSourceAdd)
    {
        fprocessor.CalculateMissingStreamData();
        while (fprocessor.TaskRunning())
        {
            if (_abortSourceAdd)
            {
                fprocessor.CancelTask();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    std::unique_lock lockExtra(_m_extraStreams);
    for (int i = 0; i < fprocessor.attachmentStreams.size(); i++)
        _attachmentStreams.push_back(std::move(fprocessor.attachmentStreams[i]));
    for (int i = 0; i < fprocessor.dataStreams.size(); i++)
        _dataStreams.push_back(std::move(fprocessor.dataStreams[i]));
    for (int i = 0; i < fprocessor.unknownStreams.size(); i++)
        _unknownStreams.push_back(std::move(fprocessor.unknownStreams[i]));
    lockExtra.unlock();

    LocalMediaSource source;
    source.filename = path;
    source.LtoG_StreamIndex.resize(context->nb_streams, { -1, LocalMediaSource::OTHER });
    std::unique_lock lock(_m_sources);
    if (!fprocessor.videoStreams.empty())
    {
        std::lock_guard lock(_videoData.mtx);
        for (int i = 0; i < fprocessor.videoStreams.size(); i++)
        {
            source.LtoG_StreamIndex[fprocessor.videoStreams[i].index] = { _videoData.streams.size(), LocalMediaSource::VIDEO_STREAM };
            //source.LtoG_StreamIndexVideo.push_back(_videoData.streams.size());
            _videoData.streams.push_back(std::move(fprocessor.videoStreams[i]));
            _videoStreamSourceIndex.push_back(_sources.size());
        }
    }
    if (!fprocessor.audioStreams.empty())
    {
        std::lock_guard lock(_audioData.mtx);
        for (int i = 0; i < fprocessor.audioStreams.size(); i++)
        {
            source.LtoG_StreamIndex[fprocessor.audioStreams[i].index] = { _audioData.streams.size(), LocalMediaSource::AUDIO_STREAM };
            //source.LtoG_StreamIndexAudio.push_back(_audioData.streams.size());
            _audioData.streams.push_back(std::move(fprocessor.audioStreams[i]));
            _audioStreamSourceIndex.push_back(_sources.size());
        }
    }
    if (!fprocessor.subtitleStreams.empty())
    {
        std::lock_guard lock(_subtitleData.mtx);
        for (int i = 0; i < fprocessor.subtitleStreams.size(); i++)
        {
            source.LtoG_StreamIndex[fprocessor.subtitleStreams[i].index] = { _subtitleData.streams.size(), LocalMediaSource::SUBTITLE_STREAM };
            //source.LtoG_StreamIndexSubtitle.push_back(_subtitleData.streams.size());
            _subtitleData.streams.push_back(std::move(fprocessor.subtitleStreams[i]));
            _subtitleStreamSourceIndex.push_back(_sources.size());
        }
    }
    _sources.push_back(std::move(source));
    lock.unlock();

    // Show notification
    zcom::NotificationInfo ninfo;
    ninfo.borderColor = D2D1::ColorF(0.2f, 0.8f, 0.2f);
    ninfo.duration = Duration(3, SECONDS);
    ninfo.title = L"Subtitles added";
    ninfo.text = L"";
    App::Instance()->Overlay()->ShowNotification(ninfo);

    avformat_close_input(&context);

    App::Instance()->events.RaiseEvent(InputSourcesChangedEvent{});
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

    _sources.push_back({ _filename });
    _sources[0].LtoG_StreamIndex.resize(_avfContext->nb_streams, { -1, LocalMediaSource::OTHER });
    for (int i = 0; i < _videoData.streams.size(); i++)
    {
        _sources[0].LtoG_StreamIndex[_videoData.streams[i].index] = { i, LocalMediaSource::VIDEO_STREAM };
        //_sources[0].LtoG_StreamIndexVideo.push_back(i);
        _videoStreamSourceIndex.push_back(0);
    }
    for (int i = 0; i < _audioData.streams.size(); i++)
    {
        _sources[0].LtoG_StreamIndex[_audioData.streams[i].index] = { i, LocalMediaSource::AUDIO_STREAM };
        //_sources[0].LtoG_StreamIndexAudio.push_back(i);
        _audioStreamSourceIndex.push_back(0);
    }
    for (int i = 0; i < _subtitleData.streams.size(); i++)
    {
        _sources[0].LtoG_StreamIndex[_subtitleData.streams[i].index] = { i, LocalMediaSource::SUBTITLE_STREAM };
        //_sources[0].LtoG_StreamIndexSubtitle.push_back(i);
        _subtitleStreamSourceIndex.push_back(0);
    }

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
    //AVPacket* packet = av_packet_alloc();
    //bool holdPacket = false;
    bool eof = false;

    //int videoStreamIndex = _videoData.currentStream != -1 ? _videoData.streams[_videoData.currentStream].index : -1;
    //int audioStreamIndex = _audioData.currentStream != -1 ? _audioData.streams[_audioData.currentStream].index : -1;
    //int subtitleStreamIndex = _subtitleData.currentStream != -1 ? _subtitleData.streams[_subtitleData.currentStream].index : -1;

    int videoStreamIndex = _videoData.currentStream;
    int audioStreamIndex = _audioData.currentStream;
    int subtitleStreamIndex = _subtitleData.currentStream;

    std::set<int> activeSourceIndices;
    if (_videoData.currentStream != -1 && _videoData.currentStream < _videoStreamSourceIndex.size())
        activeSourceIndices.insert(_videoStreamSourceIndex[_videoData.currentStream]);
    if (_audioData.currentStream != -1 && _audioData.currentStream < _audioStreamSourceIndex.size())
        activeSourceIndices.insert(_audioStreamSourceIndex[_audioData.currentStream]);
    if (_subtitleData.currentStream != -1 && _subtitleData.currentStream < _subtitleStreamSourceIndex.size())
        activeSourceIndices.insert(_subtitleStreamSourceIndex[_subtitleData.currentStream]);

    ////avformat_new_stream()
    ////av_open_inpu
    //avformat_open_input();

    //_avfContext;
    //AVInputFormat;

    while (!_packetThreadController.Get<bool>("stop"))
    {
        // Check if seek is valid
        auto seekData = _packetThreadController.Get<IMediaDataProvider::SeekData>("seek");
        if (!seekData.Default())
        {
            _packetThreadController.Set("seek", IMediaDataProvider::SeekData());
            activeSourceIndices.clear();

            std::unique_lock lock(_m_sources);

            // Change streams
            if (seekData.videoStreamIndex != std::numeric_limits<int>::min())
                _videoData.currentStream = seekData.videoStreamIndex;
            if (seekData.audioStreamIndex != std::numeric_limits<int>::min())
                _audioData.currentStream = seekData.audioStreamIndex;
            if (seekData.subtitleStreamIndex != std::numeric_limits<int>::min())
                _subtitleData.currentStream = seekData.subtitleStreamIndex;

            if (_videoData.currentStream != -1 && _videoData.currentStream < _videoStreamSourceIndex.size())
                activeSourceIndices.insert(_videoStreamSourceIndex[_videoData.currentStream]);
            if (_audioData.currentStream != -1 && _audioData.currentStream < _audioStreamSourceIndex.size())
                activeSourceIndices.insert(_audioStreamSourceIndex[_audioData.currentStream]);
            if (_subtitleData.currentStream != -1 && _subtitleData.currentStream < _subtitleStreamSourceIndex.size())
                activeSourceIndices.insert(_subtitleStreamSourceIndex[_subtitleData.currentStream]);

            // Init required sources
            for (auto index : activeSourceIndices)
            {
                if (!_sources[index].avfContext)
                {
                    if (avformat_open_input(&_sources[index].avfContext, _sources[index].filename.c_str(), NULL, NULL) != 0)
                    {
                        zcom::NotificationInfo ninfo;
                        ninfo.borderColor = D2D1::ColorF(0.8f, 0.2f, 0.2f);
                        ninfo.duration = Duration(5, SECONDS);
                        ninfo.title = L"Failed to open media source";
                        ninfo.text = L"The file '" + utf8_to_wstr(_sources[index].filename) + L"' could not be opened. The corresponding stream will contain no output";
                        App::Instance()->Overlay()->ShowNotification(ninfo);
                    }
                }
            }

            // Seek all sources
            int64_t seekTime = seekData.time.GetTime(MICROSECONDS);
            for (auto index : activeSourceIndices)
            {
                if (_sources[index].avfContext)
                {
                    avformat_seek_file(
                        _sources[index].avfContext,
                        -1,
                        std::numeric_limits<int64_t>::min(),
                        seekTime,
                        std::numeric_limits<int64_t>::max(),
                        AVSEEK_FLAG_BACKWARD
                    );
                }
            }

            lock.unlock();

            // Clear packet buffers
            _ClearVideoPackets();
            _ClearAudioPackets();
            _ClearSubtitlePackets();

            // Send flush packets
            _AddVideoPacket(MediaPacket(true));
            _AddAudioPacket(MediaPacket(true));
            _AddSubtitlePacket(MediaPacket(true));

            // Clear held packets
            for (auto index : activeSourceIndices)
            {
                if (_sources[index].heldPacket)
                {
                    av_packet_free(&_sources[index].heldPacket);
                }
            }
        }

        std::unique_lock lockSources(_m_sources);

        bool sleep = true;

        // Read from active sources
        for (auto index : activeSourceIndices)
        {
            AVPacket* packet = nullptr;

            // Read packet / Use held packet
            int result = 0;
            if (!_sources[index].heldPacket)
            {
                packet = av_packet_alloc();
                result = av_read_frame(_sources[index].avfContext, packet);
            }
            else
            {
                packet = _sources[index].heldPacket;
                _sources[index].heldPacket = nullptr;
            }

            // Process packet
            if (result >= 0)
            {
                if (eof)
                {
                    eof = false;
                    _packetThreadController.Set("eof", false);
                }

                // Get packet stream info
                auto globalStreamData = _sources[index].LtoG_StreamIndex[packet->stream_index];
                int streamIndex = globalStreamData.first;
                auto streamType = globalStreamData.second;
                if (streamIndex == -1)
                {
                    av_packet_free(&packet);
                    continue;
                }

                // Pass packet to correct stream
                if (streamType == LocalMediaSource::VIDEO_STREAM && streamIndex == _videoData.currentStream)
                {
                    if (VideoMemoryExceeded())
                    {
                        _sources[index].heldPacket = packet;
                    }
                    else
                    {
                        _sources[index].heldPacket = nullptr;
                        _AddVideoPacket(MediaPacket(packet));
                    }
                }
                else if (streamType == LocalMediaSource::AUDIO_STREAM && streamIndex == _audioData.currentStream)
                {
                    if (AudioMemoryExceeded())
                    {
                        _sources[index].heldPacket = packet;
                    }
                    else
                    {
                        _sources[index].heldPacket = nullptr;
                        _AddAudioPacket(MediaPacket(packet));
                    }
                }
                else if (streamType == LocalMediaSource::SUBTITLE_STREAM && streamIndex == _subtitleData.currentStream)
                {
                    if (SubtitleMemoryExceeded())
                    {
                        _sources[index].heldPacket = packet;
                    }
                    else
                    {
                        _sources[index].heldPacket = nullptr;
                        _AddSubtitlePacket(MediaPacket(packet));
                    }
                }
                else
                {
                    av_packet_free(&packet);
                }

                if (!_sources[index].heldPacket)
                    sleep = false;
            }
            else
            {
                av_packet_free(&packet);
                if (!eof)
                {
                    eof = true;
                    _packetThreadController.Set("eof", true);
                }
            }
        }

        lockSources.unlock();

        if (sleep)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Clear held packets
    std::lock_guard lockSources2(_m_sources);
    for (auto& source : _sources)
    {
        if (source.heldPacket)
        {
            av_packet_free(&source.heldPacket);
        }
    }
}