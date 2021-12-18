#pragma once

#pragma comment(lib, "ws2_32.lib")
#include <WS2tcpip.h>

#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>

#include <sstream>

#include "Packets.h"

// Less verbose locking of a mutex
#define LOCK_GUARD(varname, mutexname) std::lock_guard<std::mutex> varname(mutexname)
// Packet type which is sent by a UDP client to "connect"/"disconnect"
#define IDENTIFICATION_PACKET 0x00000000
// Packet type which specifies that the upcoming packets need to be combined into a single packet.
// The packet itself contains a 32-bit value which represents how many packets after this one to combine
// and another 32-bit value which contains the packet id.
#define SPLIT_PACKET 0x0FFFFFFF
// Packet type which specifies that the upcoming packets must be placed in the queue without separation.
// This is useful when using one packet to describe what the next one will contain, as it is possible
// for packets from other clients to arrive in the middle, since a single queue for all incoming packets is used.
// The packet itself contains a single 32-bit value which represents how many packets after this one to combine.
#define MULTIPLE_PACKETS 0x0FFFFFFE

namespace znetold
{
    class WSAHolder
    {
    public:
        WSADATA wsData;
        WSAHolder()
        {
            WORD ver = MAKEWORD(2, 2);
            int wsOk = WSAStartup(ver, &wsData);
        }
        ~WSAHolder()
        {
            WSACleanup();
        }
    };

    inline bool operator==(const sockaddr_in& sin1, const sockaddr_in& sin2)
    {
        return (sin1.sin_port == sin2.sin_port)
            && (sin1.sin_addr.S_un.S_addr == sin2.sin_addr.S_un.S_addr);
    }

    struct Client
    {
        SOCKET sock;
        int id;
        sockaddr_in addr;
    };

    struct PacketView
    {
        int userId;
        char packetId;
        int size;
        sockaddr_in addr;
    };

    struct Packet
    {
    private:
        std::unique_ptr<char> bytes;
    public:
        int userId;
        char packetId;
        int size;
        sockaddr_in addr;

        Packet() : userId(-1), packetId(-1), size(-1), bytes(nullptr), addr({ 0 })
        {}
        Packet(int userId, int packetId, char* bytes, int size, sockaddr_in addr = { 0 })
            : userId(userId), packetId(packetId), bytes(bytes), size(size), addr(addr)
        {}
        Packet(Packet&& p) noexcept
        {
            bytes = std::move(p.bytes);
            userId = p.userId;
            packetId = p.packetId;
            size = p.size;
            addr = p.addr;
        }
        Packet& operator=(Packet&& p) noexcept
        {
            if (this != &p)
            {
                bytes = std::move(p.bytes);
                userId = p.userId;
                packetId = p.packetId;
                size = p.size;
                addr = p.addr;
            }
            return *this;
        }

        PacketView View()
        {
            return { userId, packetId, size, addr };
        }

        template<class T>
        T Cast()
        {
            return *reinterpret_cast<T*>(bytes.get());
        }

        std::unique_ptr<char>&& Move()
        {
            return std::move(bytes);
        }

        char* Get()
        {
            return bytes.get();
        }
    };

    class TCPComponent
    {
    protected:
        std::queue<Packet> _inPackets;
        std::mutex mPak;

    public:
        virtual bool Send(char* data, size_t dataSize, int packetId, int userId = -1) = 0;
        virtual PacketView ReadPacket() = 0;
        virtual Packet GetPacket() = 0;
        virtual std::unique_ptr<char> GetPacketDataRaw() = 0;
        virtual int GetPacketCount() = 0;

        template<class T>
        bool SendStruct(T packet, int packetId, int userId = -1)
        {
            size_t packetSize = sizeof(T);
            char* data = new char[packetSize];
            memcpy(data, &packet, sizeof(T));
            bool result = Send(data, packetSize, packetId, userId);
            delete[] data;
            return result;
        }

        template<class T>
        T GetPacketData()
        {
            LOCK_GUARD(lock, mPak);
            T data = _inPackets.front().Cast<T>();
            _inPackets.pop();
            return data;
        }
    };

    //////////////////////
    // TCP SERVER CLASS //
    //////////////////////
    class TCPServer : public TCPComponent
    {
        friend class UDPServer;

        bool _online;

        int _port;
        int _maxClients;
        std::vector<Client> _clients;

        // Sending/Receiving packets
        std::vector<std::thread> _newPacketHandlers;

        // New connection handling
        SOCKET _listeningSocket;
        bool _listening;
        std::thread _newConnectionHandler;  // Thread for handling new connections
        std::queue<int> _newConnections;    // New connection buffer
        std::queue<int> _disconnects;       // Disconnected user buffer
        std::atomic<bool> _STOP_CONN;       // When set to true, thread will close after finishing accept()

        // Send buffer
        char _sendBuf[1024] = { 0 };
        size_t _maxPacketSize = 1024;

        unsigned _ID_COUNTER;
    public:
        TCPServer()
        {
            _port = -1;
            _maxClients = 10;
            _online = false;
            _listeningSocket = SOCKET_ERROR;
            _listening = false;
            _ID_COUNTER = 0;
        }

        bool StartServer(int port, int maxClients)
        {
            if (_online) return false;

            _port = port;
            _maxClients = maxClients;

            // Clear arrays
            _newPacketHandlers.clear();
            _inPackets = std::queue<Packet>();
            _newConnections = std::queue<int>();

            _ID_COUNTER = 0;

            _online = true;
            return true;
        }

        void CloseServer()
        {
            if (!_online) return;

            _online = false;
            StopNewConnections();
            DisconnectAll();
        }

        bool AllowNewConnections()
        {
            if (!_online) return false;

            _listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (_listeningSocket == INVALID_SOCKET) return false;

            // Bind the ip address and port to a socket
            sockaddr_in hint;
            hint.sin_family = AF_INET;
            hint.sin_port = htons(_port);
            hint.sin_addr.S_un.S_addr = INADDR_ANY;

            bind(_listeningSocket, (sockaddr*)&hint, sizeof(hint));

            // Tell winsock the scket is for listening
            listen(_listeningSocket, SOMAXCONN);

            // Start thread
            _newConnectionHandler = std::thread(&TCPServer::HandleConnections, this);

            return true;
        }

        void StopNewConnections()
        {
            if (!_online) return;

            // Create socket
            SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

            // Fill in the hint structure
            sockaddr_in hint;
            hint.sin_family = AF_INET;
            hint.sin_port = htons(_port);
            inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr);

            // Connect to accept()
            _STOP_CONN = true;
            int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
            _newConnectionHandler.detach();

            closesocket(_listeningSocket);
        }

        int NewConnectionCount()
        {
            LOCK_GUARD(lock, mCon);
            return _newConnections.size();
        }

        int GetNewConnection()
        {
            if (!_online) return -1;

            LOCK_GUARD(lock, mCon);
            if (_newConnections.empty()) {
                return -1;
            }
            else {
                int id = _newConnections.front();
                _newConnections.pop();
                return id;
            }
        }

        std::vector<int> GetConnections()
        {
            std::vector<int> ids;

            LOCK_GUARD(lock, mCon);
            for (int i = 0; i < _clients.size(); i++) {
                ids.push_back(_clients[i].id);
            }
            return ids;
        }

        int ConnectionCount()
        {
            LOCK_GUARD(lock, mCon);
            return _clients.size();
        }

        bool ConnectionExists(int id)
        {
            if (!_online) return false;

            LOCK_GUARD(lock, mCon);
            for (int i = 0; i < _clients.size(); i++) {
                if (_clients[i].id == id) {
                    return true;
                }
            }
            return false;
        }

        int DisconnectedUserCount()
        {
            LOCK_GUARD(lock, mCon);
            return _disconnects.size();
        }

        int GetDisconnectedUser()
        {
            LOCK_GUARD(lock, mCon);
            if (_disconnects.empty()) {
                return -1;
            }
            else {
                int id = _disconnects.front();
                _disconnects.pop();
                return id;
            }
        }

        void DisconnectUser(int id)
        {
            if (!_online) return;

            LOCK_GUARD(lock, mCon);
            for (int i = 0; i < _clients.size(); i++) {
                if (_clients[i].id == id) {
                    closesocket(_clients[i].sock);
                    _clients.erase(_clients.begin() + i);
                    break;
                }
            }
        }

        void DisconnectAll()
        {
            if (!_online) return;

            //mPak.lock();
            //for (int i = 0; i < _newPacketHandlers.size(); i++) {
            //    _newPacketHandlers[i].detach();
            //}
            //_newPacketHandlers.clear();
            //mPak.unlock();
            LOCK_GUARD(lock, mCon);
            for (int i = 0; i < _clients.size(); i++) {
                closesocket(_clients[i].sock);
            }
            _clients.clear();
        }

        // Disconnects most recently connected users until there are less than _maxClients left
        // Change _maxClients with SetMaxClients()
        void DisconnectExcess()
        {
            if (!_online) return;

            LOCK_GUARD(lock, mCon);
            while (_clients.size() > _maxClients) {
                closesocket(_clients[_clients.size() - 1].sock);
                _clients.pop_back();
            }
        }

        bool Online()
        {
            return _online;
        }

        bool Send(char* data, size_t dataSize, int packetId, int userId = -1)
        {
            // Find user
            SOCKET socket = INVALID_SOCKET;
            mCon.lock();
            for (int i = 0; i < _clients.size(); i++) {
                if (_clients[i].id == userId) {
                    socket = _clients[i].sock;
                    break;
                }
            }
            mCon.unlock();
            if (socket == INVALID_SOCKET)
            {
                std::cout << "(Err disconnect) ";
                return false;
            }

            // Prefix contains packet size and id
            size_t prefixSize = sizeof(int) + sizeof(packetId);

            // Calculate packet count
            int packetCount = ceil(dataSize / (double)(_maxPacketSize - prefixSize));
            if (packetCount == 0)
            {
                std::cout << "Empty packet.\n";
            }

            if (packetCount > 1)
            {
                // Send info about split packets
                int packetSize = 4;
                *(int*)(_sendBuf) = sizeof(packetId) + packetSize;
                *(int*)(_sendBuf + 4) = SPLIT_PACKET;
                *(int*)(_sendBuf + 8) = packetCount;
                int sent = send(socket, _sendBuf, prefixSize + packetSize, 0);
                //std::cout << "SENT: " << sent << std::endl;
                if (sent == SOCKET_ERROR)
                {
                    std::cout << "(Err split info: " << WSAGetLastError() << ") ";
                    return false;
                }
                if (sent < prefixSize + packetSize)
                {
                    std::cout << "Send incomplete..\n";
                }
            }

            // Send in parts
            int bytesLeft = dataSize;
            int bytesSent = 0;
            while (bytesLeft > 0)
            {
                int byteCount;
                if (bytesLeft > _maxPacketSize - prefixSize)
                {
                    *(int*)(_sendBuf) = _maxPacketSize - 4;
                    *(int*)(_sendBuf + 4) = packetId;
                    memcpy(_sendBuf + prefixSize, data + bytesSent, _maxPacketSize - prefixSize);
                    bytesSent += _maxPacketSize - prefixSize;
                    bytesLeft -= _maxPacketSize - prefixSize;
                    byteCount = _maxPacketSize;
                }
                else
                {
                    *(int*)(_sendBuf) = bytesLeft + 4;
                    *(int*)(_sendBuf + 4) = packetId;
                    memcpy(_sendBuf + prefixSize, data + bytesSent, bytesLeft);
                    byteCount = bytesLeft + prefixSize;
                    bytesLeft = 0;
                }
                int sent = send(socket, _sendBuf, byteCount, 0);
                if (sent == SOCKET_ERROR)
                {
                    std::cout << "(Err part: " << WSAGetLastError() << ") ";
                    return false;
                }
                if (sent < byteCount)
                {
                    std::cout << "Send incomplete..\n";
                }
            }

            return true;
        }

        PacketView ReadPacket()
        {
            LOCK_GUARD(lock, mPak);
            return _inPackets.front().View();
        }

        Packet GetPacket()
        {
            LOCK_GUARD(lock, mPak);
            Packet packet = std::move(_inPackets.front());
            _inPackets.pop();
            return packet;
        }

        std::unique_ptr<char> GetPacketDataRaw()
        {
            LOCK_GUARD(lock, mPak);
            std::unique_ptr<char> ptr = _inPackets.front().Move();
            _inPackets.pop();
            return ptr;
        }

        int GetPacketCount()
        {
            if (!_online) return -1;
            return _inPackets.size();
        }

        int GetPort()
        {
            if (!_online) return -1;
            return _port;
        }

        int GetMaxClients()
        {
            if (!_online) return -1;
            return _maxClients;
        }

        void SetMaxClients(int count)
        {
            _maxClients = count;
        }

    private:
        std::mutex mCon;

        void HandleConnections()
        {
            while (true) {
                // Wait until there are available spots
                mCon.lock();
                while (_clients.size() >= _maxClients) {
                    mCon.unlock();
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    mCon.lock();
                }
                mCon.unlock();

                // Wait for a connection
                sockaddr_in client;
                int clientSize = sizeof(client);

                SOCKET clientSocket = accept(_listeningSocket, (sockaddr*)&client, &clientSize);
                if (_STOP_CONN) {
                    closesocket(clientSocket);
                    _STOP_CONN = false;
                    return;
                }
                if (clientSocket == INVALID_SOCKET) continue;

                Client newClient = { clientSocket, _ID_COUNTER, client };

                // Add connection to list
                mCon.lock();
                _clients.push_back(newClient);
                _newConnections.push(_ID_COUNTER);
                mCon.unlock();
                _ID_COUNTER++;

                // Start thread to receive messages
                mPak.lock();
                _newPacketHandlers.push_back(std::thread(&TCPServer::ReceivePackets, this, newClient));
                mPak.unlock();

                //char host[NI_MAXHOST];
                //char service[NI_MAXSERV];

                //ZeroMemory(host, NI_MAXHOST);
                //ZeroMemory(service, NI_MAXSERV);

                //if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
                //{
                //    cout << host << " connected on port " << service << endl;
                //}
                //else
                //{
                //    inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
                //    cout << host << " connected on port " << ntohs(client.sin_port) << endl;
                //}
            }
        }

        void ReceivePackets(Client client)
        {
            char* buf = new char[1024];

            std::vector<Packet> packetBufferSplit;
            std::vector<Packet> packetBufferMultiple;
            int queueSizeSplit = 0;
            int queueSizeMultiple = 0;

            while (true) {
                ZeroMemory(buf, 1024);

                // Wait for client to send data
                int byteCount = 4;
                int bytesReceived;
                bytesReceived = recv(client.sock, buf, 4, 0);
                //std::cout << "PREFIX " << byteCount << std::endl;
                if (bytesReceived < 4)
                {
                    //std::cout << "[Incomplete prefix (" << bytesReceived << "/" << byteCount << "), attempting to receive again]\n";
                    int offset = 0;
                    while (true)
                    {
                        offset += bytesReceived;
                        byteCount -= bytesReceived;
                        bytesReceived = recv(client.sock, buf + offset, byteCount, 0);
                        if (bytesReceived < byteCount)
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                if (bytesReceived == SOCKET_ERROR)
                {
                    std::cout << "[Socket error on prefix packet (" << WSAGetLastError() << ")]\n";
                    break;
                }
                if (bytesReceived == 0)
                {
                    std::cout << "[prefix 0 bytes received, closing]\n";
                    break;
                }
                byteCount = *(int*)buf;
                bytesReceived = recv(client.sock, buf, byteCount, 0);
                int totalSize = bytesReceived;
                if (bytesReceived < byteCount)
                {
                    //std::cout << "[Incomplete data (" << bytesReceived << "/" << byteCount << "), attempting to receive again]\n";
                    int offset = 0;
                    while (true)
                    {
                        offset += bytesReceived;
                        byteCount -= bytesReceived;
                        bytesReceived = recv(client.sock, buf + offset, byteCount, 0);
                        totalSize += bytesReceived;
                        if (bytesReceived < byteCount)
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                if (bytesReceived == SOCKET_ERROR)
                {
                    std::cout << "[Socket error on data packet (" << WSAGetLastError() << ")]\n";
                    break;
                }
                if (bytesReceived == 0)
                {
                    std::cout << "[data 0 bytes (/" << byteCount << ") received, closing]\n";
                    break;
                }

                // Extract data
                int packetSize = totalSize - sizeof(int);
                char* bytes = new char[packetSize];
                int packetId = *(int*)buf;
                memcpy(bytes, buf + sizeof(int), packetSize);
                //std::cout << "TYPE " << packetId << std::endl;
                // IMPORTANT: 'bytes' pointer is passed to a smart pointer, deleting here is not necessary

                // Check for special types
                if (packetId == SPLIT_PACKET)
                {
                    queueSizeSplit = *(int*)bytes;
                    delete[] bytes;
                    continue;
                }
                else if (packetId == MULTIPLE_PACKETS)
                {
                    if (queueSizeSplit == 0)
                    {
                        queueSizeMultiple = *(int*)bytes;
                        delete[] bytes;
                        continue;
                    }
                }

                Packet packet = Packet(client.id, packetId, bytes, packetSize);

                // Handle split packets
                if (queueSizeSplit > 0)
                {
                    queueSizeSplit--;
                    packetBufferSplit.push_back(std::move(packet));

                    // Combine packets
                    if (queueSizeSplit == 0)
                    {
                        packet = CombinePackets(packetBufferSplit);
                    }
                    else
                    {
                        continue;
                    }
                }

                // Handle multiple packets
                if (queueSizeMultiple > 0)
                {
                    queueSizeMultiple--;
                    packetBufferMultiple.push_back(std::move(packet));

                    // Deposit buffer to packet queue
                    if (queueSizeMultiple == 0)
                    {
                        DepositPacketBuffer(packetBufferMultiple);
                    }
                    continue;
                }

                mPak.lock();
                _inPackets.push(std::move(packet));
                mPak.unlock();
            }

            closesocket(client.sock);
            mCon.lock();
            _disconnects.push(client.id);
            for (int i = 0; i < _clients.size(); i++) {
                if (_clients[i].id == client.id) {
                    _clients.erase(_clients.begin() + i);
                    mPak.lock();
                    _newPacketHandlers[i].detach();
                    _newPacketHandlers.erase(_newPacketHandlers.begin() + i);
                    mPak.unlock();
                    break;
                }
            }
            mCon.unlock();
        }

        // Combine all packets in the buffer into a single and push it to the main queue
        Packet&& CombinePackets(std::vector<Packet>& buffer)
        {
            // Calculate total size
            size_t combinedSize = 0;
            for (int i = 0; i < buffer.size(); i++)
            {
                combinedSize += buffer[i].View().size;
            }

            // Allocate memory
            char* combinedData = new char[combinedSize];

            // Merge data
            PacketView packetInfo = buffer[0].View();
            int currentByte = 0;
            for (int i = 0; i < buffer.size(); i++)
            {
                int packetSize = buffer[i].View().size;
                std::unique_ptr<char> packetData = buffer[i].Move();
                memcpy(combinedData + currentByte, packetData.get(), packetSize);
                currentByte += packetSize;
            }

            // Clear buffer
            buffer.clear();

            return Packet(packetInfo.userId, packetInfo.packetId, combinedData, combinedSize);
        }

        // Push all packets in the buffer at once
        void DepositPacketBuffer(std::vector<Packet>& buffer)
        {
            mPak.lock();
            for (int i = 0; i < buffer.size(); i++)
            {
                _inPackets.push(std::move(buffer[i]));
            }
            mPak.unlock();
            buffer.clear();
        }

        void ReleaseThread(std::thread::id id)
        {
            LOCK_GUARD(lock, mPak);
            for (int i = 0; i < _newPacketHandlers.size(); i++) {
                if (_newPacketHandlers[i].get_id() == id) {
                    _newPacketHandlers[i].detach();
                    _newPacketHandlers.erase(_newPacketHandlers.begin() + i);
                    return;
                }
            }
        }
    };

    //////////////////////
    // TCP CLIENT CLASS //
    //////////////////////
    class TCPClient : public TCPComponent
    {
        Client _thisClient;

        std::string _serverIp;
        int _serverPort;

        // Receiving packets
        std::thread _newPacketHandler;

        // Send buffer
        char _sendBuf[1024];
        size_t _maxPacketSize = 1024;

    public:
        TCPClient()
        {
            _inPackets = std::queue<Packet>();
        }

        bool Connect(std::string ip, int port)
        {
            _serverIp = ip;
            _serverPort = port;

            // Create socket
            _thisClient.sock = socket(AF_INET, SOCK_STREAM, 0);
            if (_thisClient.sock == INVALID_SOCKET) return false;

            _thisClient.addr.sin_family = AF_INET;
            _thisClient.addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &_thisClient.addr.sin_addr);

            // Connect to server
            int connResult = connect(_thisClient.sock, (sockaddr*)&_thisClient.addr, sizeof(_thisClient.addr));
            if (connResult == SOCKET_ERROR) {
                closesocket(_thisClient.sock);
                return false;
            }

            // Start receiver thread
            _newPacketHandler = std::thread(&TCPClient::ReceivePackets, this);

            return true;
        }

        void Disconnect()
        {
            closesocket(_thisClient.sock);
            _newPacketHandler.detach();
        }

        bool Send(char* data, size_t dataSize, int packetId, int userId = -1)
        {
            size_t prefixSize = sizeof(packetId) + sizeof(int);

            // Calculate packet count
            int packetCount = ceil(dataSize / (double)(_maxPacketSize - prefixSize));
            if (packetCount == 0)
            {
                std::cout << "Empty packet.\n";
            }

            if (packetCount > 1)
            {
                // Send info about split packets
                int packetSize = 4;
                *(int*)(_sendBuf) = sizeof(packetId) + packetSize;
                *(int*)(_sendBuf + 4) = SPLIT_PACKET;
                *(int*)(_sendBuf + 8) = packetCount;
                int sent = send(_thisClient.sock, _sendBuf, prefixSize + packetSize, 0);
                if (sent == SOCKET_ERROR)
                {
                    std::cout << "(Err split info: " << WSAGetLastError() << ") ";
                    return false;
                }
                if (sent < packetSize + 4)
                {
                    std::cout << "Send incomplete..\n";
                }
            }

            // Send in parts
            int bytesLeft = dataSize;
            int bytesSent = 0;
            while (bytesLeft > 0)
            {
                int byteCount;
                if (bytesLeft > _maxPacketSize - prefixSize)
                {
                    *(int*)(_sendBuf) = _maxPacketSize - 4;
                    *(int*)(_sendBuf + 4) = packetId;
                    memcpy(_sendBuf + prefixSize, data + bytesSent, _maxPacketSize - prefixSize);
                    bytesSent += _maxPacketSize - prefixSize;
                    bytesLeft -= _maxPacketSize - prefixSize;
                    byteCount = _maxPacketSize;
                }
                else
                {
                    *(int*)(_sendBuf) = bytesLeft + 4;
                    *(int*)(_sendBuf + 4) = packetId;
                    memcpy(_sendBuf + prefixSize, data + bytesSent, bytesLeft);
                    byteCount = bytesLeft + prefixSize;
                    bytesLeft = 0;
                }
                if (send(_thisClient.sock, _sendBuf, byteCount, 0) == SOCKET_ERROR)
                {
                    std::cout << "(Err part: " << WSAGetLastError() << ") ";
                    return false;
                }
            }

            return true;
        }

        PacketView ReadPacket()
        {
            LOCK_GUARD(lock, mPak);
            return _inPackets.front().View();
        }

        Packet GetPacket()
        {
            LOCK_GUARD(lock, mPak);
            Packet packet = std::move(_inPackets.front());
            _inPackets.pop();
            return packet;
        }

        std::unique_ptr<char> GetPacketDataRaw()
        {
            LOCK_GUARD(lock, mPak);
            std::unique_ptr<char> ptr = _inPackets.front().Move();
            _inPackets.pop();
            return ptr;
        }

        int GetPacketCount()
        {
            LOCK_GUARD(lock, mPak);
            return _inPackets.size();
        }

    private:
        void ReceivePackets()
        {
            char* buf = new char[1024];

            std::vector<Packet> packetBufferSplit;
            std::vector<Packet> packetBufferMultiple;
            int queueSizeSplit = 0;
            int queueSizeMultiple = 0;

            while (true) {
                ZeroMemory(buf, 1024);

                // Wait for server to send data
                int byteCount = 4;
                int bytesReceived;
                bytesReceived = recv(_thisClient.sock, buf, 4, 0);
                //std::cout << "PREFIX " << byteCount << std::endl;
                if (bytesReceived < byteCount)
                {
                    //std::cout << "[Incomplete prefix (" << bytesReceived << "/" << byteCount << "), attempting to receive again]";
                    int offset = 0;
                    while (true)
                    {
                        offset += bytesReceived;
                        byteCount -= bytesReceived;
                        bytesReceived = recv(_thisClient.sock, buf + offset, byteCount, 0);
                        if (bytesReceived < byteCount)
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                if (bytesReceived == SOCKET_ERROR)
                {
                    std::cout << "[Socket error on prefix packet (" << WSAGetLastError() << ")]\n";
                    break;
                }
                if (bytesReceived == 0)
                {
                    std::cout << "[prefix 0 bytes received, closing]\n";
                    break;
                }
                byteCount = *(int*)buf;
                bytesReceived = recv(_thisClient.sock, buf, byteCount, 0);
                int totalSize = bytesReceived;
                if (bytesReceived < byteCount)
                {
                    //std::cout << "[Incomplete data (" << bytesReceived << "/" << byteCount << "), attempting to receive again]\n";
                    int offset = 0;
                    while (true)
                    {
                        offset += bytesReceived;
                        byteCount -= bytesReceived;
                        bytesReceived = recv(_thisClient.sock, buf + offset, byteCount, 0);
                        totalSize += bytesReceived;
                        if (bytesReceived < byteCount)
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                if (bytesReceived == SOCKET_ERROR)
                {
                    std::cout << "[Socket error on data packet (" << WSAGetLastError() << ")]\n";
                    break;
                }
                if (bytesReceived == 0)
                {
                    std::cout << "[data 0 bytes (/" << byteCount << ") received, closing]\n";
                    break;
                }

                // Extract data
                int packetSize = totalSize - sizeof(int);
                char* bytes = new char[packetSize];
                int packetId = *(int*)buf;
                memcpy(bytes, buf + sizeof(int), packetSize);
                //std::cout << "TYPE " << packetId << std::endl;
                // IMPORTANT: 'bytes' pointer is passed to a smart pointer, deleting here is not necessary

                // Check for special types
                if (packetId == SPLIT_PACKET)
                {
                    queueSizeSplit = *(int*)bytes;
                    delete[] bytes;
                    continue;
                }
                else if (packetId == MULTIPLE_PACKETS)
                {
                    if (queueSizeSplit == 0)
                    {
                        queueSizeMultiple = *(int*)bytes;
                        delete[] bytes;
                        continue;
                    }
                }

                Packet packet(-1, packetId, bytes, packetSize);

                // Handle split packets
                if (queueSizeSplit > 0)
                {
                    queueSizeSplit--;
                    packetBufferSplit.push_back(std::move(packet));

                    // Combine packets
                    if (queueSizeSplit == 0)
                    {
                        packet = CombinePackets(packetBufferSplit);
                    }
                    else
                    {
                        continue;
                    }
                }

                // Handle multiple packets
                if (queueSizeMultiple > 0)
                {
                    queueSizeMultiple--;
                    packetBufferMultiple.push_back(std::move(packet));

                    // Deposit buffer to packet queue
                    if (queueSizeMultiple == 0)
                    {
                        DepositPacketBuffer(packetBufferMultiple);
                    }
                    continue;
                }

                mPak.lock();
                _inPackets.push(std::move(packet));
                mPak.unlock();
            }

            closesocket(_thisClient.sock);
            _newPacketHandler.detach();
        }

        // Combine all packets in the buffer into a single and push it to the main queue
        Packet CombinePackets(std::vector<Packet>& buffer)
        {
            // Calculate total size
            size_t combinedSize = 0;
            for (int i = 0; i < buffer.size(); i++)
            {
                combinedSize += buffer[i].View().size;
            }

            // Allocate memory
            char* combinedData = new char[combinedSize];

            // Merge data
            PacketView packetInfo = buffer[0].View();
            int currentByte = 0;
            for (int i = 0; i < buffer.size(); i++)
            {
                int packetSize = buffer[i].View().size;
                std::unique_ptr<char> packetData = buffer[i].Move();
                memcpy(combinedData + currentByte, packetData.get(), packetSize);
                currentByte += packetSize;
            }

            // Clear buffer
            buffer.clear();

            return Packet(packetInfo.userId, packetInfo.packetId, combinedData, combinedSize);
        }

        // Push all packets in the buffer at once
        void DepositPacketBuffer(std::vector<Packet>& buffer)
        {
            mPak.lock();
            for (int i = 0; i < buffer.size(); i++)
            {
                _inPackets.push(std::move(buffer[i]));
            }
            mPak.unlock();
            buffer.clear();
        }
    };

    //////////////////////
    // UDP SERVER CLASS //
    //////////////////////
    class UDPServer
    {
        bool _online;

        int _port;

        // Receiving packets
        SOCKET _inSocket;
        std::thread _newPacketHandler;
        std::queue<Packet> _inPackets;

        // Linked server for packet queue
        TCPServer* _host = nullptr;
        bool _linked = false;

        // Identified users
        std::vector<Client> _clients;
        std::vector<uint64_t> _inPacketCounters;
        std::vector<uint64_t> _outPacketCounters;

        // Send buffer
        char _sendBuf[1024];
        size_t _maxPacketSize = 1024;

    public:
        UDPServer()
        {
            _port = -1;
            _online = false;
            _inSocket = SOCKET_ERROR;
        }

        bool StartServer(int port)
        {
            if (_online) return false;

            _port = port;

            // Clear arrays
            _inPackets = std::queue<Packet>();

            // Create and bind socket
            _inSocket = socket(AF_INET, SOCK_DGRAM, 0);

            sockaddr_in serverHint;
            serverHint.sin_family = AF_INET;
            serverHint.sin_port = htons(_port);
            serverHint.sin_addr.S_un.S_addr = ADDR_ANY;

            if (bind(_inSocket, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) {
                closesocket(_inSocket);
                return false;
            }

            // Start receiver thread
            _newPacketHandler = std::thread(&UDPServer::ReceivePackets, this);

            _online = true;
            return true;
        }

        void CloseServer()
        {
            if (!_online) return;

            closesocket(_inSocket);
            _newPacketHandler.detach();

            _online = false;
        }

        bool Online()
        {
            return _online;
        }

        void Link(TCPServer* host)
        {
            _host = host;
            _linked = true;
        }

        void Unlink()
        {
            _host = nullptr;
            _linked = true;
        }

        bool Linked()
        {
            return _linked;
        }

        bool Send(char* data, size_t dataSize, int packetId, int userId)
        {
            // Find user
            int userIndex = -1;
            LOCK_GUARD(lock, mCon);
            for (int i = 0; i < _clients.size(); i++) {
                if (_clients[i].id == userId) {
                    //dest = _clients[i].addr;
                    //packetNumber = _outPacketCounters[i];
                    //_outPacketCounters[i]++;
                    //foundUser = true;
                    userIndex = i;
                    break;
                }
            }
            if (userIndex == -1)
            {
                return false;
            }

            size_t prefixSize = sizeof(packetId) + sizeof(_outPacketCounters[userIndex]);

            // Calculate packet count
            int packetCount = ceil(dataSize / (double)(_maxPacketSize - prefixSize));
            if (packetCount > 1)
            {
                // Send info about split packets
                *(int*)(_sendBuf) = SPLIT_PACKET;
                *(int*)(_sendBuf + 4) = packetCount;
                if (sendto(_inSocket, _sendBuf, 8, 0, (sockaddr*)&(_clients[userIndex].addr), sizeof(_clients[userIndex].addr)) == SOCKET_ERROR)
                {
                    return false;
                }
            }

            // Send in parts
            int bytesLeft = dataSize;
            int bytesSent = 0;
            while (bytesLeft > 0)
            {
                int byteCount;
                if (bytesLeft > _maxPacketSize - prefixSize)
                {
                    memcpy(_sendBuf, &packetId, sizeof(packetId));
                    memcpy(_sendBuf + sizeof(packetId), &_outPacketCounters[userIndex], sizeof(_outPacketCounters[userIndex]));
                    memcpy(_sendBuf + prefixSize, data + bytesSent, _maxPacketSize - prefixSize);
                    bytesSent += _maxPacketSize - prefixSize;
                    bytesLeft -= _maxPacketSize - prefixSize;
                    byteCount = _maxPacketSize;
                }
                else
                {
                    memcpy(_sendBuf, &packetId, sizeof(packetId));
                    memcpy(_sendBuf + sizeof(packetId), &_outPacketCounters[userIndex], sizeof(_outPacketCounters[userIndex]));
                    memcpy(_sendBuf + prefixSize, data + bytesSent, bytesLeft);
                    byteCount = bytesLeft + prefixSize;
                    bytesLeft = 0;
                }
                _outPacketCounters[userIndex]++;
                if (sendto(_inSocket, _sendBuf, byteCount, 0, (sockaddr*)&(_clients[userIndex].addr), sizeof(_clients[userIndex].addr)) == SOCKET_ERROR)
                {
                    return false;
                }
            }

            return true;
        }

        template<class T>
        bool Send(T packet, int packetId, int userId)
        {
            size_t packetSize = sizeof(T);
            char* data = new char[packetSize];
            memcpy(data, &packet, sizeof(T));
            bool result = Send(data, packetSize, packetId, userId);
            delete[] data;
            return result;
        }


        //template<class T>
        //bool Send(T packet, char packetId, sockaddr_in dest)
        //{
        //    unsigned packetSize = sizeof(char) + sizeof(T) + 1;
        //    ZeroMemory(sendBuf + packetSize - 1, 1);
        //    memcpy(sendBuf, &packetId, sizeof(char));
        //    memcpy(sendBuf + sizeof(char), &packet, sizeof(T));
        //    //msg = reinterpret_cast<char*>(&packet);

        //    int sendResult = sendto(_inSocket, sendBuf, packetSize, 0, (sockaddr*)&dest, sizeof(dest));

        //    if (sendResult == SOCKET_ERROR) return false;
        //    return true;
        //}

        //template<class T>
        //bool Send(T packet, char packetId, int userID)
        //{
        //    unsigned packetSize = sizeof(char) + sizeof(T) + 1;
        //    ZeroMemory(sendBuf + packetSize - 1, 1);
        //    memcpy(sendBuf, &packetId, sizeof(char));
        //    memcpy(sendBuf + sizeof(char), &packet, sizeof(T));
        //    //msg = reinterpret_cast<char*>(&packet);

        //    int sendResult = -2;
        //    mCon.lock();
        //    for (int i = 0; i < _clients.size(); i++) {
        //        if (_clients[i].id == userID) {
        //            sendResult = sendto(_inSocket, sendBuf, packetSize, 0, (sockaddr*)&(_clients[i].addr), sizeof(_clients[i].addr));
        //            if (sendResult == SOCKET_ERROR) {
        //                std::ostringstream ss;
        //                ss << "Error sending packet: " << WSAGetLastError() << "\n";
        //                //OutputDebugString(ss.str().c_str());
        //            }
        //            break;
        //        }
        //    }
        //    mCon.unlock();

        //    if (sendResult == -2) return false;
        //    if (sendResult == SOCKET_ERROR) return false;
        //    return true;
        //}

        PacketView ReadPacket()
        {
            LOCK_GUARD(lock, mPak);
            return _inPackets.front().View();
        }

        template<class T>
        T GetPacketData()
        {
            LOCK_GUARD(lock, mPak);
            T data = _inPackets.front().Cast<T>();
            _inPackets.pop();
            return data;
        }

        int GetPacketCount()
        {
            if (!_online) return -1;
            LOCK_GUARD(lock, mPak);
            return _inPackets.size();
        }

    private:
        std::mutex mCon;
        std::mutex mPak;

        void ReceivePackets()
        {
            sockaddr_in client;
            int clientLength = sizeof(client);

            char buf[1024];

            while (true) {
                ZeroMemory(buf, 1024);

                // Wait for client to send data
                int bytesReceived = recvfrom(_inSocket, buf, 1024, 0, (sockaddr*)&client, &clientLength);
                if (bytesReceived == SOCKET_ERROR) {
                    std::wostringstream ss;
                    ss << "Error receiving packet: " << WSAGetLastError() << "\n";
                    OutputDebugString(ss.str().c_str());
                    break;
                }
                if (bytesReceived == 0) break;

                // Construct packet
                int packetSize = bytesReceived - sizeof(int);
                char* bytes = new char[packetSize];
                int packetId = *(int*)buf;
                uint64_t packetNumber = *(int*)(buf + sizeof(uint64_t));
                memcpy(bytes, buf + sizeof(int), packetSize);
                // IMPORTANT: 'bytes' pointer is passed to a smart pointer, deleting here is not necessary

                if (packetId == 0) {
                    if (packetSize != 4) {
                        OutputDebugString(L"Invalid identifier packet size\n");
                        delete[] bytes;
                        continue;
                    }

                    // Save/erase client data
                    int userID = *reinterpret_cast<int*>(bytes);
                    bool erased = false;

                    mCon.lock();
                    for (int i = 0; i < _clients.size(); i++) {
                        if (_clients[i].id == userID) {
                            _clients.erase(_clients.begin() + i);
                            erased = true;
                            break;
                        }
                    }
                    if (!erased) {
                        _clients.push_back({ 0, userID, client });
                        _inPacketCounters.push_back(0);
                        _outPacketCounters.push_back(0);
                    }
                    mCon.unlock();
                }

                if (!_linked) {
                    mCon.lock();
                    bool found = false;
                    for (int i = 0; i < _clients.size(); i++) {
                        if (_clients[i].addr == client) {
                            found = true;
                            LOCK_GUARD(lock, mPak);
                            _inPackets.push(Packet(_clients[i].id, packetId, bytes, packetSize, client));
                            _inPacketCounters[i]++;
                            break;
                        }
                    }
                    mCon.unlock();

                    if (!found) {
                        mPak.lock();
                        _inPackets.push(Packet(-1, packetId, bytes, packetSize, client));
                        mPak.unlock();
                    }
                }
                else {
                    _host->mCon.lock();
                    bool found = false;
                    for (int i = 0; i < _host->_clients.size(); i++) {
                        if (_host->_clients[i].addr == client) {
                            found = true;
                            LOCK_GUARD(lock, mPak);
                            _host->_inPackets.push(Packet(_host->_clients[i].id, packetId, bytes, packetSize, client));
                            break;
                        }
                    }
                    _host->mCon.unlock();

                    if (!found) {
                        _host->mPak.lock();
                        _host->_inPackets.push(Packet(-1, packetId, bytes, packetSize, client));
                        _host->mPak.unlock();
                    }
                }
            }

            closesocket(_inSocket);
            _newPacketHandler.detach();
        }
    };

    //////////////////////
    // UDP CLIENT CLASS //
    //////////////////////
    class UDPClient
    {
        Client _thisClient;

        sockaddr_in _server;
        std::string _serverIp;
        int _serverPort;

        // Receiving packets
        std::thread _newPacketHandler;
        std::queue<Packet> _inPackets;
    public:

        UDPClient()
        {
            _serverIp = "";
            _serverPort = -1;
        }

        bool Connect(std::string ip, int port)
        {
            _serverIp = ip;
            _serverPort = port;

            // Create a hint structure for the server
            _server.sin_family = AF_INET;
            _server.sin_port = htons(_serverPort);
            inet_pton(AF_INET, _serverIp.c_str(), &_server.sin_addr);

            _thisClient.sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (_thisClient.sock == INVALID_SOCKET) return false;

            _thisClient.addr.sin_family = AF_INET;
            _thisClient.addr.sin_port;
            _thisClient.addr.sin_addr.S_un.S_addr = ADDR_ANY;
            //inet_pton(AF_INET, _serverIp.c_str(), &_thisClient.addr.sin_addr);
            int bindResult = bind(_thisClient.sock, (sockaddr*)&_thisClient.addr, sizeof(_thisClient.addr));
            if (bindResult == -1) return false;

            // Start receiver thread
            _newPacketHandler = std::thread(&UDPClient::ReceivePackets, this);

            return true;
        }

        void Disconnect()
        {
            closesocket(_thisClient.sock);
        }

        template<class T>
        bool Send(T packet, char packetId)
        {
            unsigned packetSize = sizeof(char) + sizeof(T) + 1;
            char* msg = new char[packetSize];
            ZeroMemory(msg, packetSize);
            memcpy(msg, &packetId, sizeof(char));
            memcpy(msg + sizeof(char), &packet, sizeof(T));
            //msg = reinterpret_cast<char*>(&packet);

            int sendResult = sendto(_thisClient.sock, msg, packetSize, 0, (sockaddr*)&_server, sizeof(_server));
            delete[] msg;

            if (sendResult == SOCKET_ERROR) return false;
            return true;
        }

        PacketView ReadPacket()
        {
            LOCK_GUARD(lock, mPak);
            return _inPackets.front().View();
        }

        template<class T>
        T GetPacketData()
        {
            LOCK_GUARD(lock, mPak);
            T data = _inPackets.front().Cast<T>();
            _inPackets.pop();
            return data;
        }

        int GetPacketCount()
        {
            LOCK_GUARD(lock, mPak);
            return _inPackets.size();
        }

    private:
        std::mutex mPak;

        void ReceivePackets()
        {
            sockaddr_in client;
            int clientLength = sizeof(client);

            char buf[1024];

            while (true) {
                ZeroMemory(buf, 1024);

                // Wait for server to send data
                int bytesReceived = recvfrom(_thisClient.sock, buf, 1024, 0, (sockaddr*)&client, &clientLength);
                if (bytesReceived == SOCKET_ERROR) break;
                if (bytesReceived == 0) break;

                // Construct packet
                int packetSize = bytesReceived - sizeof(char);
                char* bytes = new char[packetSize];
                char packetId = *buf;
                memcpy(bytes, buf + sizeof(char), packetSize);
                // IMPORTANT: 'bytes' pointer is passed to a smart pointer, deleting here is not necessary

                mPak.lock();
                _inPackets.push(Packet(0, packetId, bytes, packetSize, client));
                mPak.unlock();
            }

            closesocket(_thisClient.sock);
            _newPacketHandler.detach();
        }
    };
}
