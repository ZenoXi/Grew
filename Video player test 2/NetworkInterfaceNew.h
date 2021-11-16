#pragma once

#include <fstream>

#include "NetBase.h"
#include "GameTime.h"
#include "NetworkMode.h"
#include "ThreadController.h"
#include "FFmpegSerializer.h"

struct PacketData
{
    char* data;
    size_t size;
    int packetId;
    int userId;
};

enum class NetworkStatus
{
    WAITING_FOR_CONNECTION = -2,
    CONNECTING,
    OFFLINE,
    CONNECTED
};

class NetworkInterface
{
    znet::TCPServer _server;
    znet::TCPClient _client;

    NetworkMode _mode = NetworkMode::OFFLINE;
    NetworkStatus _status = NetworkStatus::OFFLINE;

public:
    NetworkInterface();

    void StartServer(USHORT port);
    void StopServer();
    void Connect(std::string ip, USHORT port);
    void Disconnect();

    NetworkMode Mode();
    NetworkStatus Status();

    // Sender functions
    void SendVideoParams(FFmpeg::Result params);
    void SendAudioParams(FFmpeg::Result params);
    void SendVideoPacket(FFmpeg::Result packet);
    void SendAudioPacket(FFmpeg::Result packet);
    void SendMetadata(Duration duration, AVRational framerate, AVRational videoTimebase, AVRational audioTimebase);
};
