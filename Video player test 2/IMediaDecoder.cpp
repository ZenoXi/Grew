#include "IMediaDecoder.h"

IMediaDecoder::IMediaDecoder()
{

}

IMediaDecoder::~IMediaDecoder()
{
    ClearPackets();
    ClearFrames();
}

bool IMediaDecoder::PacketQueueFull() const
{
    return _packets.size() >= _MAX_PACKET_QUEUE_SIZE;
}

size_t IMediaDecoder::PacketQueueSize() const
{
    return _packets.size();
}

void IMediaDecoder::AddPacket(MediaPacket packet)
{
    std::lock_guard<std::mutex> lock(_m_packets);
    _packets.push(std::move(packet));
}

size_t IMediaDecoder::FrameCount() const
{
    return _frames.size();
}

std::unique_ptr<IMediaFrame> IMediaDecoder::GetFrame()
{
    std::lock_guard<std::mutex> lock(_m_frames);
    if (_frames.empty()) return nullptr;
    std::unique_ptr<IMediaFrame> frame(_frames.front());
    _frames.pop();
    return frame;
}

void IMediaDecoder::Flush()
{
    _decoderThreadFlush = true;
}

void IMediaDecoder::ClearPackets()
{
    _m_packets.lock();
    while (!_packets.empty()) _packets.pop();
    _m_packets.unlock();
}

void IMediaDecoder::ClearFrames()
{
    _m_frames.lock();
    while (!_frames.empty())
    {
        delete _frames.front();
        _frames.pop();
    }
    _m_frames.unlock();
}

bool IMediaDecoder::Flushing() const
{
    return _decoderThreadFlush;
}

void IMediaDecoder::_StartDecoding()
{
    _decoderThreadStop = false;
    _decoderThread = std::thread(&IMediaDecoder::_DecoderThread, this);
}

void IMediaDecoder::_StopDecoding()
{
    _decoderThreadStop = true;
    if (_decoderThread.joinable()) _decoderThread.join();
}
