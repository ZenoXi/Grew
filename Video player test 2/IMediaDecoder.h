#pragma once

#include <vector>
#include <string>
#include <deque>
#include <queue>
#include <memory>
#include <mutex>
#include "ChiliWin.h"

#include "GameTime.h"
#include "ThreadController.h"

#include "MediaStream.h"
#include "MediaPacket.h"
#include "IMediaFrame.h"

class IMediaDecoder
{
protected:
    std::queue<MediaPacket> _packets;
    std::queue<IMediaFrame*> _frames;
    std::mutex _m_packets;
    std::mutex _m_frames;

    size_t _MAX_FRAME_QUEUE_SIZE;
    size_t _MAX_PACKET_QUEUE_SIZE;

    bool _decoderThreadStop = false;
    bool _decoderThreadFlush = false;
    std::thread _decoderThread;

    AVRational _timebase;

public:
    IMediaDecoder();
    virtual ~IMediaDecoder();

    bool PacketQueueFull() const;
    size_t PacketQueueSize() const;
    void AddPacket(MediaPacket packet);
    size_t FrameCount() const;
    std::unique_ptr<IMediaFrame> GetFrame();

    // Should be called before adding packets after a seek
    void Flush();
    void ClearPackets();
    void ClearFrames();
    bool Flushing() const;

private:
    void _StartDecoding();
    void _StopDecoding();
    virtual void _DecoderThread() = 0;
};