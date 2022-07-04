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

class NetworkInterfaceOld
{
    znetold::TCPServer _server;
    znetold::TCPClient _client;
    znetold::TCPComponent* _network;

    //PlaybackMode _mode = OFFLINE;
    NetworkStatus _status = NetworkStatus::OFFLINE;
    znet::NetworkMode _mode = znet::NetworkMode::OFFLINE;

    std::string _ip = "";
    USHORT _port = 0;
    int _clientId = -1;

    ThreadController _serverStartController;
    ThreadController _clientStartController;
    std::thread _serverStartThread;
    std::thread _clientStartThread;

private: // Singleton interface
    NetworkInterfaceOld();
    static NetworkInterfaceOld* _instance;
public:
    static void Init();
    static NetworkInterfaceOld* Instance();

private:
    // Blank constructor
    NetworkInterfaceOld(bool online);
    // Server constructor
    NetworkInterfaceOld(USHORT port);
    // Client constructor
    NetworkInterfaceOld(std::string ip, USHORT port);

public:
    void StartServer(USHORT port);
    void StopServer();
    void Connect(std::string ip, USHORT port);
    void Disconnect();

    znet::NetworkMode Mode();
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
    znetold::Packet GetPacket();
    int PacketsBuffered();
    TimePoint PeerPosition();

private:
    // Network thread
    void NetworkThread();
    std::mutex _mtxNetThread;
    std::thread _networkThread;
    ThreadController _networkThreadController;
    std::deque<znetold::Packet> _inPacketQueue;
    std::deque<znetold::Packet> _outPacketQueue;
    TimePoint _position = 0;

    // Server thread
    void ServerThread();
    std::mutex _mServer;
    std::thread _serverThread;
    std::deque<znetold::Packet> _inServerPacketQueue;
    std::deque<znetold::Packet> _outServerPacketQueue;
    TimePoint _clientPosition = 0;
    TimePoint _bufferedDuration = 0;

    // Client thread
    void ClientThread();
    std::mutex _mClient;
    std::thread _clientThread;
    std::deque<znetold::Packet> _inClientPacketQueue;
    std::deque<znetold::Packet> _outClientPacketQueue;
    TimePoint _serverPosition;
    znetold::Packet _videoParamsPkt;
    znetold::Packet _audioParamsPkt;
    std::deque<znetold::Packet> _videoPacketPkts;
    std::deque<znetold::Packet> _audioPacketPkts;
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