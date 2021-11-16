#include "NetworkInterface.h"

NetworkInterface* NetworkInterface::_instance = nullptr;

NetworkInterface::NetworkInterface(bool online)
{
    if (!online) _mode = NetworkMode::OFFLINE;

    _serverStartController.Add("done", sizeof(int));
    _serverStartController.Set("done", 0);
    _clientStartController.Add("done", sizeof(int));
    _clientStartController.Set("done", 0);
}

NetworkInterface::NetworkInterface(USHORT port)
    : NetworkInterface(true)
{
    //sAudioOut.open("server.data", std::ios::binary);
    _mode = NetworkMode::SERVER;
    _port = port;

    _serverStartThread = std::thread(&NetworkInterface::_StartServer, this);
    _serverStartThread.detach();
}

NetworkInterface::NetworkInterface(std::string ip, USHORT port)
    : NetworkInterface(true)
{
    _mode = NetworkMode::CLIENT;
    _ip = ip;
    _port = port;

    _clientStartThread = std::thread(&NetworkInterface::_StartClient, this);
    _clientStartThread.detach();
}

NetworkInterface::NetworkInterface()
{
    _serverStartController.Add("done", sizeof(int));
    _serverStartController.Set("done", 0);
    _clientStartController.Add("done", sizeof(int));
    _clientStartController.Set("done", 0);
    _networkThreadController.Add("stop", sizeof(bool));
    _networkThreadController.Set("stop", false);
}

void NetworkInterface::Init()
{
    if (!_instance)
    {
        _instance = new NetworkInterface();
    }
}

NetworkInterface* NetworkInterface::Instance()
{
    return _instance;
}

void NetworkInterface::StartServer(USHORT port)
{
    _mode = NetworkMode::SERVER;
    _status = NetworkStatus::WAITING_FOR_CONNECTION;
    _port = port;

    _serverStartThread = std::thread(&NetworkInterface::_StartServer, this);
    _serverStartThread.detach();
}

void NetworkInterface::StopServer()
{
    if (_serverStartController.Get<int>("done") == 1)
    {
        _serverStartController.Set("done", 0);
        _networkThreadController.Set("stop", true);

        _status = NetworkStatus::STOPPING_SERVER;
        _port = 0;
    }
}

void NetworkInterface::Connect(std::string ip, USHORT port)
{
    _mode = NetworkMode::CLIENT;
    _status = NetworkStatus::CONNECTING;
    _ip = ip;
    _port = port;

    _clientStartThread = std::thread(&NetworkInterface::_StartClient, this);
    _clientStartThread.detach();
}

void NetworkInterface::Disconnect()
{
    if (_clientStartController.Get<int>("done") == 1)
    {
        _clientStartController.Set("done", 0);
        _networkThreadController.Set("stop", true);

        _status = NetworkStatus::DISCONNECTING;
        _ip = "";
        _port = 0;
    }
}

void NetworkInterface::_StartServer()
{
    // Create and bind the socket
    _server.StartServer(_port, 1);
    if (!_server.AllowNewConnections())
    {
        _serverStartController.Set("done", -1);
        std::cout << "Start failed.\n";
        _status = NetworkStatus::OFFLINE;
        return;
    }

    // Wait for a connection
    while (_server.NewConnectionCount() == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // Get client id
    _clientId = _server.GetNewConnection();

    // Start server thread
    _network = &_server;
    _networkThreadController.Set("stop", false);
    _networkThread = std::thread(&NetworkInterface::NetworkThread, this);

    // Return success
    _serverStartController.Set("done", 1);
    _status = NetworkStatus::CONNECTED;
}

void NetworkInterface::_StartClient()
{
    // Connect to the server
    if (!_client.Connect(_ip, _port))
    {
        _clientStartController.Set("done", -1);
        std::cout << "Connection failed.\n";
        _status = NetworkStatus::OFFLINE;
        return;
    }

    // Start client thread
    _network = &_client;
    _networkThreadController.Set("stop", false);
    _networkThread = std::thread(&NetworkInterface::NetworkThread, this);

    // Return success
    _clientStartController.Set("done", 1);
    _status = NetworkStatus::CONNECTED;
}

int NetworkInterface::ServerStarted()
{
    return _serverStartController.Get<int>("done");
}

int NetworkInterface::Connected()
{
    return _clientStartController.Get<int>("done");
}

NetworkMode NetworkInterface::Mode()
{
    return _mode;
}

NetworkStatus NetworkInterface::Status()
{
    return _status;
}

// //////////////// //
// SENDER FUNCTIONS //
// //////////////// //

void NetworkInterface::SendVideoParams(FFmpeg::Result params)
{
    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(_clientId, packet::VIDEO_PARAMS, (char*)params.data, params.size));
    _mtxNetThread.unlock();
    params.data = nullptr;
    params.size = 0;
}

void NetworkInterface::SendAudioParams(FFmpeg::Result params)
{
    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(_clientId, packet::AUDIO_PARAMS, (char*)params.data, params.size));
    _mtxNetThread.unlock();
    params.data = nullptr;
    params.size = 0;
}

void NetworkInterface::SendVideoPacket(FFmpeg::Result packet)
{
    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(_clientId, packet::VIDEO_PACKET, (char*)packet.data, packet.size));
    _mtxNetThread.unlock();
    packet.data = nullptr;
    packet.size = 0;
}

void NetworkInterface::SendAudioPacket(FFmpeg::Result packet)
{
    //sAudioOut.write((char*)packet.data, packet.size);

    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(_clientId, packet::AUDIO_PACKET, (char*)packet.data, packet.size));
    _mtxNetThread.unlock();
    packet.data = nullptr;
    packet.size = 0;
}

void NetworkInterface::SendMetadata(Duration duration, AVRational framerate, AVRational videoTimebase, AVRational audioTimebase)
{
    int totalSize = sizeof(duration) + sizeof(framerate) + sizeof(videoTimebase) + sizeof(audioTimebase);
    char* data = new char[totalSize];
    int offset = 0;
    memcpy(data + offset, &duration, sizeof(duration));
    offset += sizeof(duration);
    memcpy(data + offset, &framerate, sizeof(framerate));
    offset += sizeof(framerate);
    memcpy(data + offset, &videoTimebase, sizeof(videoTimebase));
    offset += sizeof(videoTimebase);
    memcpy(data + offset, &audioTimebase, sizeof(audioTimebase));
    offset += sizeof(audioTimebase);
    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(_clientId, packet::METADATA, data, totalSize));
    _mtxNetThread.unlock();
}

void NetworkInterface::ConfirmSeekReceived()
{
    char* padding = new char;
    _mtxNetThread.lock();
    _outPacketQueue.push_front(znet::Packet(_clientId, packet::SEEK_CONFIRMATION, padding, 1));
    int count = 0;
    for (int i = 0; i < _outPacketQueue.size(); i++)
    {
        if (_outPacketQueue[i].packetId == packet::AUDIO_PACKET || _outPacketQueue[i].packetId == packet::VIDEO_PACKET)
        {
            count++;
            _outPacketQueue.erase(_outPacketQueue.begin() + i);
            i--;
        }
    }
    _mtxNetThread.unlock();
    std::cout << count << " packets discarded.\n";
}

TimePoint NetworkInterface::ClientPosition()
{
    _mServer.lock();
    TimePoint time = _clientPosition;
    _mServer.unlock();
    return time;
}

TimePoint NetworkInterface::GetBufferedDuration()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _bufferedDuration;
}

bool NetworkInterface::ClientConnected()
{
    return _clientId >= 0;
}

// ////////////////// //
// RECEIVER FUNCTIONS //
// ////////////////// //

uchar* NetworkInterface::GetVideoParams()
{
    LOCK_GUARD(lock, _mtxNetThread);
    if (_videoParamsPkt.size > 0)
    {
        return (uchar*)_videoParamsPkt.Get();
    }
    return nullptr;
}

uchar* NetworkInterface::GetAudioParams()
{
    LOCK_GUARD(lock, _mtxNetThread);
    if (_audioParamsPkt.size > 0)
    {
        return (uchar*)_audioParamsPkt.Get();
    }
    return nullptr;
}

std::unique_ptr<char> NetworkInterface::GetVideoPacket()
{
    LOCK_GUARD(lock, _mtxNetThread);
    if (!_videoPacketPkts.empty())
    {
        std::unique_ptr<char> ptr = _videoPacketPkts.front().Move();
        _videoPacketPkts.pop_front();
        return ptr;
    }
    return nullptr;
}

std::unique_ptr<char> NetworkInterface::GetAudioPacket()
{
    LOCK_GUARD(lock, _mtxNetThread);
    if (!_audioPacketPkts.empty())
    {
        lastSize = _audioPacketPkts.front().size;
        std::unique_ptr<char> ptr = _audioPacketPkts.front().Move();
        _audioPacketPkts.pop_front();
        return ptr;
    }
    return nullptr;
}

Duration NetworkInterface::GetDuration()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _duration;
}

AVRational NetworkInterface::GetFramerate()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _framerate;
}

AVRational NetworkInterface::GetVideoTimebase()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _videoTimebase;
}

AVRational NetworkInterface::GetAudioTimebase()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _audioTimebase;
}

bool NetworkInterface::MetadataReceived()
{
    LOCK_GUARD(lock, _mClient);
    if (_videoParamsPkt.size == -1) return false;
    if (_audioParamsPkt.size == -1) return false;
    if (_duration == -1) return false;
    if (_framerate.num == -1) return false;
    if (_videoTimebase.num == -1) return false;
    if (_audioTimebase.num == -1) return false;
    return true;
}

void NetworkInterface::NotifyBufferingEnd()
{
    char* padding = new char;
    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(-1, packet::BUFFERING_END, padding, 1));
    _mtxNetThread.unlock();
}

void NetworkInterface::NotifyBufferingStart()
{
    char* padding = new char;
    _mtxNetThread.lock();
    _outPacketQueue.push_back(znet::Packet(-1, packet::BUFFERING_START, padding, 1));
    _mtxNetThread.unlock();
}

void NetworkInterface::SendBufferedDuration(TimePoint bufferEnd)
{
    TimePoint* tp = new TimePoint;
    *tp = bufferEnd;
    _mtxNetThread.lock();
    _outPacketQueue.push_front(znet::Packet(-1, packet::BUFFERING_START, (char*)tp, sizeof(bufferEnd)));
    _mtxNetThread.unlock();
}

TimePoint NetworkInterface::ServerPosition()
{
    _mClient.lock();
    TimePoint time = _serverPosition;
    _mClient.unlock();
    return time;
}

// //////////////// //
// SHARED FUNCTIONS //
// //////////////// //

void NetworkInterface::SendSeekCommand(TimePoint position)
{
    if (Status() != NetworkStatus::CONNECTED) return;
    std::cout << "Seek command sent.\n";

    TimePoint* tp = new TimePoint;
    *tp = position;
    _mtxNetThread.lock();
    _outPacketQueue.push_front(znet::Packet(_clientId, packet::SEEK, (char*)tp, sizeof(position)));
    if (Mode() == NetworkMode::CLIENT)
    {
        _awaitingSeekConfirmation = true;
        _videoPacketPkts.clear();
        _audioPacketPkts.clear();
    }
    else if (Mode() == NetworkMode::SERVER)
    {
        for (int i = 0; i < _outPacketQueue.size(); i++)
        {
            if (_outPacketQueue[i].packetId == packet::AUDIO_PACKET || _outPacketQueue[i].packetId == packet::VIDEO_PACKET)
            {
                _outPacketQueue.erase(_outPacketQueue.begin() + i);
                i--;
            }
        }
    }
    _mtxNetThread.unlock();
    //if (_mode == SERVER)
    //{
    //    TimePoint* tp = new TimePoint;
    //    *tp = position;
    //    _mServer.lock();
    //    _outServerPacketQueue.push_front(znet::Packet(_clientId, packet::SEEK, (char*)tp, sizeof(position)));
    //    _mServer.unlock();
    //}
    //else
    //{
    //    TimePoint* tp = new TimePoint;
    //    *tp = position;
    //    _mClient.lock();
    //    _outClientPacketQueue.push_front(znet::Packet(-1, packet::SEEK, (char*)tp, sizeof(position)));
    //    _awaitingSeekConfirmation = true;
    //    _videoPacketPkts.clear();
    //    _audioPacketPkts.clear();
    //    _mClient.unlock();
    //}
}

void NetworkInterface::SendPauseCommand()
{
    if (Status() != NetworkStatus::CONNECTED) return;

    char* padding = new char;
    _mtxNetThread.lock();
    _outPacketQueue.push_front(znet::Packet(_clientId, packet::PAUSE, padding, 1));
    _mtxNetThread.unlock();
    //if (_mode == SERVER)
    //{
    //    char* padding = new char;
    //    _mServer.lock();
    //    _outServerPacketQueue.push_front(znet::Packet(_clientId, packet::PAUSE, padding, 1));
    //    _mServer.unlock();
    //}
    //else
    //{
    //    char* padding = new char;
    //    _mClient.lock();
    //    _outClientPacketQueue.push_front(znet::Packet(-1, packet::PAUSE, padding, 1));
    //    _mClient.unlock();
    //}
}

void NetworkInterface::SendResumeCommand()
{
    if (Status() != NetworkStatus::CONNECTED) return;

    char* padding = new char;
    _mtxNetThread.lock();
    _outPacketQueue.push_front(znet::Packet(_clientId, packet::RESUME, padding, 1));
    _mtxNetThread.unlock();
    //if (_mode == SERVER)
    //{
    //    char* padding = new char;
    //    _mServer.lock();
    //    _outServerPacketQueue.push_front(znet::Packet(_clientId, packet::RESUME, padding, 1));
    //    _mServer.unlock();
    //}
    //else
    //{
    //    char* padding = new char;
    //    _mClient.lock();
    //    _outClientPacketQueue.push_front(znet::Packet(-1, packet::RESUME, padding, 1));
    //    _mClient.unlock();
    //}
}

void NetworkInterface::SendCurrentPosition(TimePoint position)
{
    if (Status() != NetworkStatus::CONNECTED) return;

    TimePoint* tp = new TimePoint;
    *tp = position;
    _mtxNetThread.lock();
    _outPacketQueue.push_front(znet::Packet(_clientId, packet::POSITION, (char*)tp, sizeof(position)));
    _mtxNetThread.unlock();
    //if (_mode == SERVER)
    //{
    //    TimePoint* tp = new TimePoint;
    //    *tp = position;
    //    _mServer.lock();
    //    _outServerPacketQueue.push_front(znet::Packet(_clientId, packet::POSITION, (char*)tp, sizeof(position)));
    //    _mServer.unlock();
    //}
    //else
    //{
    //    TimePoint* tp = new TimePoint;
    //    *tp = position;
    //    _mClient.lock();
    //    _outClientPacketQueue.push_front(znet::Packet(-1, packet::POSITION, (char*)tp, sizeof(position)));
    //    _mClient.unlock();
    //}
}

znet::Packet NetworkInterface::GetPacket()
{
    if (Mode() == NetworkMode::OFFLINE) return znet::Packet();

    LOCK_GUARD(lock, _mtxNetThread);
    if (_inPacketQueue.size() > 0)
    {
        znet::Packet pkt = std::move(_inPacketQueue.front());
        _inPacketQueue.pop_front();
        return pkt;
    }
    return znet::Packet();
    //if (_mode == SERVER)
    //{
    //    LOCK_GUARD(lock, _mServer);
    //    if (_inServerPacketQueue.size() > 0)
    //    {
    //        znet::Packet pkt = std::move(_inServerPacketQueue.front());
    //        _inServerPacketQueue.pop_front();
    //        return pkt;
    //    }
    //}
    //else if (_mode == CLIENT)
    //{
    //    LOCK_GUARD(lock, _mClient);
    //    if (_inClientPacketQueue.size() > 0)
    //    {
    //        znet::Packet pkt = std::move(_inClientPacketQueue.front());
    //        _inClientPacketQueue.pop_front();
    //        return pkt;
    //    }
    //}
    //return znet::Packet();
}

int NetworkInterface::PacketsBuffered()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _outPacketQueue.size();
}

TimePoint NetworkInterface::PeerPosition()
{
    LOCK_GUARD(lock, _mtxNetThread);
    return _clientPosition;
}

// //////////////// //
// THREAD FUNCTIONS //
// //////////////// //

void NetworkInterface::NetworkThread()
{
    uint64_t bytesSent = 0;
    uint64_t bytesConfirmed = 0;
    uint64_t packetsSent = 0;
    uint64_t packetsReceived = 0;

    while (!_networkThreadController.Get<bool>("stop"))
    {
        // TODO: Check if server/client is still connected
        //

        int inPacketCount = _network->GetPacketCount();
        int outPacketCount = 0;

        // Read incoming packets
        if (inPacketCount > 0)
        {
            znet::PacketView pv = _network->ReadPacket();
            // Confirm packet received
            if (pv.packetId != packet::BYTE_CONFIRMATION)
            {
                _network->SendStruct(pv.size, packet::BYTE_CONFIRMATION, _clientId);
            }

            switch (pv.packetId)
            {
            case packet::BYTE_CONFIRMATION:
            {
                int byteCount = _network->GetPacketData<int>();
                bytesConfirmed += byteCount;
                break;
            }
            case packet::POSITION:
            {
                TimePoint position = _network->GetPacketData<TimePoint>();
                _mtxNetThread.lock();
                _position = position;
                _mtxNetThread.unlock();
                break;
            }
            case packet::BUFFERED_DURATION:
            {
                TimePoint position = _network->GetPacketData<TimePoint>();
                _mtxNetThread.lock();
                _bufferedDuration = position;
                _mtxNetThread.unlock();
                break;
            }
            case packet::VIDEO_PACKET:
            {
                _mtxNetThread.lock();
                if (_awaitingSeekConfirmation)
                {
                    // Discard packet
                    _network->GetPacket();
                }
                else
                {
                    _videoPacketPkts.push_back(_network->GetPacket());
                }
                _mtxNetThread.unlock();
                break;
            }
            case packet::AUDIO_PACKET:
            {
                _mtxNetThread.lock();
                if (_awaitingSeekConfirmation)
                {
                    // Discard packet
                    _network->GetPacket();
                }
                else
                {
                    _audioPacketPkts.push_back(_network->GetPacket());
                }
                _mtxNetThread.unlock();
                break;
            }
            case packet::VIDEO_PARAMS:
            {
                _mtxNetThread.lock();
                _videoParamsPkt = _network->GetPacket();
                _mtxNetThread.unlock();
                std::cout << "Video parameters received.\n";
                break;
            }
            case packet::AUDIO_PARAMS:
            {
                _mtxNetThread.lock();
                _audioParamsPkt = _network->GetPacket();
                _mtxNetThread.unlock();
                std::cout << "Audio parameters received.\n";
                break;
            }
            case packet::METADATA:
            {
                znet::Packet pkt = _network->GetPacket();

                int offset = 0;
                _mtxNetThread.lock();
                memcpy(&_duration, pkt.Get() + offset, sizeof(_duration));
                offset += sizeof(_duration);
                memcpy(&_framerate, pkt.Get() + offset, sizeof(_framerate));
                offset += sizeof(_framerate);
                memcpy(&_videoTimebase, pkt.Get() + offset, sizeof(_videoTimebase));
                offset += sizeof(_videoTimebase);
                memcpy(&_audioTimebase, pkt.Get() + offset, sizeof(_audioTimebase));
                offset += sizeof(_audioTimebase);
                _mtxNetThread.unlock();
                std::cout << "Metadata received.\n";

                break;
            }
            case packet::SEEK_CONFIRMATION:
            {
                auto pkt = _network->GetPacket();
                _mtxNetThread.lock();
                _awaitingSeekConfirmation = false;
                _mtxNetThread.unlock();
                std::cout << "Seek confirmed\n";
                break;
            }
            default:
                _mtxNetThread.lock();
                _inPacketQueue.push_back(_network->GetPacket());
                _mtxNetThread.unlock();
                break;
            }
            packetsReceived++;
        }

        // Send queued packets
        if (bytesSent - bytesConfirmed < 1000000LL)
        {
            _mtxNetThread.lock();
            outPacketCount = _outPacketQueue.size();
            if (outPacketCount > 0)
            {
                // Artificially throttle speed
                //int64_t MAX_BYTES_PER_SECOND = 400000LL;
                int64_t MAX_BYTES_PER_SECOND = (~0ULL) >> 1;
                znet::PacketView pv = _outPacketQueue.front().View();
                if (true/*bytesSent - bytesSentLastPrint < MAX_BYTES_PER_SECOND || (pv.packetId != packet::AUDIO_PACKET && pv.packetId != packet::VIDEO_PACKET)*/)
                {
                    // Send packet
                    znet::Packet packet = std::move(_outPacketQueue.front());
                    _outPacketQueue.pop_front();
                    //_outServerPacketQueue.erase(_outServerPacketQueue.begin());
                    _mtxNetThread.unlock();

                    _network->Send(packet.Get(), packet.size, packet.packetId, packet.userId);
                    bytesSent += packet.size;
                    packetsSent++;
                }
                else
                {
                    _mtxNetThread.unlock();
                }
            }
            else
            {
                _mtxNetThread.unlock();
            }
        }

        // Limit CPU usage when traffic is low
        if (inPacketCount <= 1 && outPacketCount <= 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    if (Mode() == NetworkMode::SERVER)
    {
        _server.CloseServer();
    }
    else if (Mode() == NetworkMode::CLIENT)
    {
        _client.Disconnect();
    }
    _network = nullptr;

    _mode = NetworkMode::OFFLINE;
    _status = NetworkStatus::OFFLINE;
    _outPacketQueue.clear();
    _networkThreadController.Set("stop", false);
}

void NetworkInterface::ServerThread()
{
    uint64_t bytesSent = 0;
    uint64_t bytesConfirmed = 0;
    uint64_t packetsSent = 0;
    uint64_t packetsReceived = 0;
    uint64_t cycles = 0;
    long long int timeSpentSending = 0;
    long long int timeSpentReceiving = 0;
    long long int timeInCycle = 0;

    Clock threadClock = Clock();
    TimePoint lastPrint = threadClock.Now();
    uint64_t bytesSentLastPrint = 0;
    uint64_t packetsSentLastPrint = 0;

    while (true)
    {
        Clock cTimer = Clock();
        // Read incoming packets
        if (_server.GetPacketCount() > 0)
        {
            Clock timer = Clock();
            znet::PacketView pv = _server.ReadPacket();
            switch (pv.packetId)
            {
            case packet::BYTE_CONFIRMATION:
            {
                int byteCount = _server.GetPacketData<int>();
                bytesConfirmed += byteCount;
                break;
            }
            case packet::POSITION:
            {
                TimePoint position = _server.GetPacketData<TimePoint>();
                _mServer.lock();
                _clientPosition = position;
                _mServer.unlock();
                break;
            }
            case packet::BUFFERED_DURATION:
            {
                TimePoint position = _server.GetPacketData<TimePoint>();
                _mServer.lock();
                _bufferedDuration = position;
                _mServer.unlock();
                break;
            }
            default:
                _mServer.lock();
                _inServerPacketQueue.push_back(_server.GetPacket());
                _mServer.unlock();
                break;
            }
            packetsReceived++;
            timer.Update();
            timeSpentReceiving += timer.Now().GetTime();
        }

        // Send queued packets
        if (bytesSent - bytesConfirmed < 1000000LL)
        {
            Clock timer = Clock();
            _mServer.lock();
            if (_outServerPacketQueue.size() > 0)
            {
                //if (packetsSent == 11000)
                //{
                //    int k = 0;
                //    k++;
                //}

                //char* ptr = _outServerPacketQueue.front().Get();
                //if (*ptr == -35 || packetsSent > 50000)
                //{
                //    // || _outServerPacketQueue.size() > 50000
                //    int k = 0;
                //    k++;
                //}

                // Artificially throttle speed
                //int64_t MAX_BYTES_PER_SECOND = 400000LL;
                int64_t MAX_BYTES_PER_SECOND = (~0ULL) >> 1;
                znet::PacketView pv = _outServerPacketQueue.front().View();
                if (bytesSent - bytesSentLastPrint < MAX_BYTES_PER_SECOND || (pv.packetId != packet::AUDIO_PACKET && pv.packetId != packet::VIDEO_PACKET))
                {
                    // Send packet
                    znet::Packet packet = std::move(_outServerPacketQueue.front());
                    _outServerPacketQueue.pop_front();
                    //_outServerPacketQueue.erase(_outServerPacketQueue.begin());
                    _mServer.unlock();

                    _server.Send(packet.Get(), packet.size, packet.packetId, packet.userId);
                    bytesSent += packet.size;
                    packetsSent++;
                }
                else
                {
                    _mServer.unlock();
                }

                timer.Update();
                timeSpentSending += timer.Now().GetTime();
            }
            else
            {
                _mServer.unlock();
            }
        }
        else
        {
            //std::cout << "Waiting for " << bytesSent - bytesConfirmed << " bytes\n";
        }

        threadClock.Update();
        long long int msElapsed = (threadClock.Now() - lastPrint).GetDuration(MILLISECONDS);
        if (msElapsed > 1000)
        {
            lastPrint = threadClock.Now();
            uint64_t sendAmount = bytesSent - bytesSentLastPrint;
            uint64_t sendCount = packetsSent - packetsSentLastPrint;
            bytesSentLastPrint = bytesSent;
            packetsSentLastPrint = packetsSent;
            double kbps = sendAmount / (double)msElapsed;
            double pps = sendCount / (msElapsed / 1000.0);
            std::cout << "Bytes sent: " << bytesSent << " (" << kbps << "kb/s | " << pps << "p/s)\n";
            //std::cout << "Average us per packet sent: " << timeSpentSending / packetsSent << "\n";
            //std::cout << "Average us per packet received: " << timeSpentReceiving / packetsReceived << "\n";
            //std::cout << "Average us per cycle: " << timeInCycle / cycles << "\n";
        }

        cycles++;
        cTimer.Update();
        timeInCycle += cTimer.Now().GetTime();

        //std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void NetworkInterface::ClientThread()
{
    while (true)
    {
        // Read incoming packets
        if (_client.GetPacketCount() > 0)
        {
            znet::PacketView pv = _client.ReadPacket();
            _client.SendStruct(pv.size, packet::BYTE_CONFIRMATION);

            switch (pv.packetId)
            {
            case packet::POSITION:
            {
                TimePoint position = _client.GetPacketData<TimePoint>();
                _mClient.lock();
                _serverPosition = position;
                _mClient.unlock();
                break;
            }
            case packet::VIDEO_PACKET:
            {
                _mClient.lock();
                if (_awaitingSeekConfirmation)
                {
                    // Discard packet
                    _client.GetPacket();
                }
                else
                {
                    _videoPacketPkts.push_back(_client.GetPacket());
                }
                _mClient.unlock();
                break;
            }
            case packet::AUDIO_PACKET:
            {
                _mClient.lock();
                if (_awaitingSeekConfirmation)
                {
                    // Discard packet
                    _client.GetPacket();
                }
                else
                {
                    _audioPacketPkts.push_back(_client.GetPacket());
                }
                _mClient.unlock();
                break;
            }
            case packet::VIDEO_PARAMS:
            {
                _mClient.lock();
                _videoParamsPkt = _client.GetPacket();
                _mClient.unlock();
                std::cout << "Video parameters received.\n";
                break;
            }
            case packet::AUDIO_PARAMS:
            {
                _mClient.lock();
                _audioParamsPkt = _client.GetPacket();
                _mClient.unlock();
                std::cout << "Audio parameters received.\n";
                break;
            }
            case packet::METADATA:
            {
                znet::Packet pkt = _client.GetPacket();

                int offset = 0;
                _mClient.lock();
                memcpy(&_duration, pkt.Get() + offset, sizeof(_duration));
                offset += sizeof(_duration);
                memcpy(&_framerate, pkt.Get() + offset, sizeof(_framerate));
                offset += sizeof(_framerate);
                memcpy(&_videoTimebase, pkt.Get() + offset, sizeof(_videoTimebase));
                offset += sizeof(_videoTimebase);
                memcpy(&_audioTimebase, pkt.Get() + offset, sizeof(_audioTimebase));
                offset += sizeof(_audioTimebase);
                _mClient.unlock();
                std::cout << "Metadata received.\n";

                break;
            }
            //case packet::BUFFERING_END:
            //{
            //    _mClient.lock();
            //    _inClientPacketQueue.push_back(_client.GetPacket());
            //    _mClient.unlock();
            //    std::cout << "Buffer end acknowledged.\n";
            //    break;
            //}
            case packet::SEEK_CONFIRMATION:
            {
                auto pkt = _client.GetPacket();
                _mClient.lock();
                _awaitingSeekConfirmation = false;
                _mClient.unlock();
                std::cout << "Seek confirmed\n";
                break;
            }
            default:
            {
                _mClient.lock();
                _inClientPacketQueue.push_back(_client.GetPacket());
                _mClient.unlock();
                break;
            }
            }
        }

        // Send queued packets
        _mClient.lock();
        if (_outClientPacketQueue.size() > 0)
        {
            znet::Packet packet = std::move(_outClientPacketQueue.front());
            _outClientPacketQueue.pop_front();
            _mClient.unlock();

            _client.Send(packet.Get(), packet.size, packet.packetId);
        }
        else
        {
            _mClient.unlock();
        }

        //std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}