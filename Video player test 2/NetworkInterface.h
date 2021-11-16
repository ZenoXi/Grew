#pragma once

#include <fstream>

#include "NetBase.h"
#include "GameTime.h"
#include "NetworkMode.h"
#include "ThreadController.h"
#include "FFmpegSerializer.h"

enum class NetworkStatus
{
    WAITING_FOR_CONNECTION = -4,
    STOPPING_SERVER,
    CONNECTING,
    DISCONNECTING,
    OFFLINE = 0,
    CONNECTED
};

struct PacketData
{
    char* data;
    size_t size;
    int packetId;
    int userId;
};

class NetworkInterface
{
    znet::TCPServer _server;
    znet::TCPClient _client;
    znet::TCPComponent* _network;

    //PlaybackMode _mode = OFFLINE;
    NetworkStatus _status = NetworkStatus::OFFLINE;
    NetworkMode _mode = NetworkMode::OFFLINE;

    std::string _ip = "";
    USHORT _port = 0;
    int _clientId = -1;

    ThreadController _serverStartController;
    ThreadController _clientStartController;
    std::thread _serverStartThread;
    std::thread _clientStartThread;

private: // Singleton interface
    NetworkInterface();
    static NetworkInterface* _instance;
public:
    static void Init();
    static NetworkInterface* Instance();

private:
    // Blank constructor
    NetworkInterface(bool online);
    // Server constructor
    NetworkInterface(USHORT port);
    // Client constructor
    NetworkInterface(std::string ip, USHORT port);

public:
    void StartServer(USHORT port);
    void StopServer();
    void Connect(std::string ip, USHORT port);
    void Disconnect();

    NetworkMode Mode();
    NetworkStatus Status();

private:
    // Start thread
    void _StartServer();
    // Start thread
    void _StartClient();

public:
    // 1 - success; 0 - in progress; <0 error
    int ServerStarted();
    // 1 - success; 0 - in progress; <0 error
    int Connected();

    // Sender functions
    void SendVideoParams(FFmpeg::Result params);
    void SendAudioParams(FFmpeg::Result params);
    void SendVideoPacket(FFmpeg::Result packet);
    void SendAudioPacket(FFmpeg::Result packet);
    void SendMetadata(Duration duration, AVRational framerate, AVRational videoTimebase, AVRational audioTimebase);
    void ConfirmSeekReceived();
    TimePoint ClientPosition();
    TimePoint GetBufferedDuration();
    bool ClientConnected();

    // Receiver functions
    uchar* GetVideoParams();
    uchar* GetAudioParams();
    std::unique_ptr<char> GetVideoPacket();
    std::unique_ptr<char> GetAudioPacket();
    Duration GetDuration();
    AVRational GetFramerate();
    AVRational GetVideoTimebase();
    AVRational GetAudioTimebase();
    bool MetadataReceived();
    void NotifyBufferingEnd();
    void NotifyBufferingStart();
    void SendBufferedDuration(TimePoint bufferEnd);
    TimePoint ServerPosition();

    // Shared functions
    void SendSeekCommand(TimePoint position);
    void SendPauseCommand();
    void SendResumeCommand();
    void SendCurrentPosition(TimePoint position);
    znet::Packet GetPacket();
    int PacketsBuffered();
    TimePoint PeerPosition();

private:
    // Network thread
    void NetworkThread();
    std::mutex _mtxNetThread;
    std::thread _networkThread;
    ThreadController _networkThreadController;
    std::deque<znet::Packet> _inPacketQueue;
    std::deque<znet::Packet> _outPacketQueue;
    TimePoint _position = 0;

    // Server thread
    void ServerThread();
    std::mutex _mServer;
    std::thread _serverThread;
    std::deque<znet::Packet> _inServerPacketQueue;
    std::deque<znet::Packet> _outServerPacketQueue;
    TimePoint _clientPosition = 0;
    TimePoint _bufferedDuration = 0;

    // Client thread
    void ClientThread();
    std::mutex _mClient;
    std::thread _clientThread;
    std::deque<znet::Packet> _inClientPacketQueue;
    std::deque<znet::Packet> _outClientPacketQueue;
    TimePoint _serverPosition;
    znet::Packet _videoParamsPkt;
    znet::Packet _audioParamsPkt;
    std::deque<znet::Packet> _videoPacketPkts;
    std::deque<znet::Packet> _audioPacketPkts;
    Duration _duration = -1;
    AVRational _framerate = { -1, -1 };
    AVRational _videoTimebase = { -1, -1 };
    AVRational _audioTimebase = { -1, -1 };

    // When a client issues a seek command and clears its buffers, the server might still be transmitting
    // old packets, which corrupt the buffers. While this var is true, no packets from the server will be
    // accepted to prevent said corruption.
    bool _awaitingSeekConfirmation = false;

    //std::ofstream sAudioOut;
public:
    int lastSize = 0;
};