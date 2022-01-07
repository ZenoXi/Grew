#pragma once

#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>

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

namespace znet
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

    // // // //
    struct PacketView
    {
        size_t size;
        int32_t id;
    };

    // // // //
    struct Packet
    {
    private:
        std::shared_ptr<int8_t[]> _bytes;
    public:
        size_t size;
        int32_t id;

        Packet() : _bytes(nullptr), size(0), id(-1)
        {}
        Packet(std::unique_ptr<int8_t[]>&& bytes, int size, int id)
            : _bytes(std::move(bytes)), size(size), id(id)
        {}
        Packet(int packetId)
            : _bytes(nullptr), size(0), id(packetId)
        {}
        Packet(Packet&& p) noexcept
        {
            _bytes = std::move(p._bytes);
            size = p.size;
            id = p.id;
            p.size = 0;
            p.id = -1;
        }
        Packet& operator=(Packet&& p) noexcept
        {
            if (this != &p)
            {
                _bytes = std::move(p._bytes);
                size = p.size;
                id = p.id;
                p.size = 0;
                p.id = -1;
            }
            return *this;
        }

        // Creates a packet pointing to the same memory
        // NOTE: changes to one packets data will be seen in the other packets data
        Packet Reference() const
        {
            Packet refPacket;
            refPacket._bytes = _bytes;
            refPacket.size = size;
            refPacket.id = id;
            return refPacket;
        }

        // Allocates memory and creates a copy of the packet 
        Packet Clone() const
        {
            auto newBytes = std::make_unique<int8_t[]>(size);
            std::copy_n(_bytes.get(), size, newBytes.get());
            return Packet(std::move(newBytes), size, id);
        }

        PacketView View() const
        {
            return { size, id };
        }

        // Cast packet data to specified type and return the resulting object
        template<class T>
        T Cast() const
        {
            return *reinterpret_cast<T*>(_bytes.get());
        }

        // Shallow copy object into packet data
        template<class T>
        Packet& From(const T& object)
        {
            _bytes.reset(new int8_t[sizeof(T)]);
            memcpy(Bytes(), &object, sizeof(T));
            size = sizeof(T);
            return *this;
        }

        // Get packet data bytes
        int8_t* Bytes()
        {
            return _bytes.get();
        }
    };

    //////////////////////////
    // TCP CONNECTION CLASS //
    //////////////////////////
    class TCPConnection
    {
        SOCKET _socket;
        std::deque<Packet> _inPackets;
        std::deque<std::pair<Packet, int>> _outPackets; // outgoing packets have priority
        std::thread _inPacketHandler;
        std::thread _outPacketHandler;
        std::mutex _m_inPackets;
        std::mutex _m_outPackets;
        std::queue<Packet> _packetQueue;
        bool _blockPackets = false;

        const size_t _MAX_PACKET_SIZE;

    public:
        sockaddr_in sockaddr;

    public:
        TCPConnection(SOCKET socket, sockaddr_in sockaddr = {0}, size_t maxPacketSize = 1024)
            : _socket(socket), sockaddr(sockaddr), _MAX_PACKET_SIZE(maxPacketSize)
        {
            if (_socket == SOCKET_ERROR) return;

            _inPacketHandler = std::thread(&TCPConnection::HandleIncomingPackets, this);
            _outPacketHandler = std::thread(&TCPConnection::HandleOutgoingPackets, this);
        }
        ~TCPConnection()
        {
            Disconnect();
        }

        bool Connected() const
        {
            return _socket != SOCKET_ERROR;
        }

        void Disconnect()
        {
            if (_socket != SOCKET_ERROR)
            {
                closesocket(_socket);
                _socket = SOCKET_ERROR;
            }
            if (_inPacketHandler.joinable()) _inPacketHandler.join();
            if (_outPacketHandler.joinable()) _outPacketHandler.join();
        }

        size_t PacketCount() const
        {
            return _inPackets.size();
        }

        PacketView PeekPacket()
        {
            LOCK_GUARD(lock, _m_inPackets);
            return _inPackets.front().View();
        }

        Packet GetPacket()
        {
            LOCK_GUARD(lock, _m_inPackets);
            Packet packet = std::move(_inPackets.front());
            _inPackets.pop_front();
            return packet;
        }

        void ClearPackets()
        {
            LOCK_GUARD(lock, _m_inPackets);
            _inPackets.clear();
        }

        void BlockPackets()
        {
            _blockPackets = true;
        }

        void UnblockPackets()
        {
            _blockPackets = false;
        }
        
        bool Blocked() const
        {
            return _blockPackets;
        }

        // Packets are placed in queue after all packets of the same or higher priority
        void Send(Packet&& packet, int priority = 0)
        {
            if (!Connected()) return;

            if (priority == 0)
            {
                LOCK_GUARD(lock, _m_outPackets);
                _outPackets.push_back({ std::move(packet), priority });
                return;
            }

            LOCK_GUARD(lock, _m_outPackets);
            for (int i = 0; i < _outPackets.size(); i++)
            {
                if (_outPackets[i].second < priority)
                {
                    _outPackets.insert(_outPackets.begin() + i, { std::move(packet), priority });
                    return;
                }
            }
        }

        // Packets in the queue are sent bundled together, with no packets inbetween
        void AddToQueue(Packet&& packet)
        {
            if (!Connected()) return;

            _packetQueue.push(std::move(packet));
        }

        // Packets are placed in queue after all packets of the same or higher priority
        void SendQueue(int priority = 0)
        {
            if (!Connected()) return;

            if (priority == 0)
            {
                LOCK_GUARD(lock, _m_outPackets);
                if (!_packetQueue.empty())
                {
                    _outPackets.push_back({ std::move(Packet(MULTIPLE_PACKETS).From(_packetQueue.size())), priority });
                }
                while (!_packetQueue.empty())
                {
                    _outPackets.push_back({ std::move(_packetQueue.front()), priority });
                    _packetQueue.pop();
                }
                return;
            }

            LOCK_GUARD(lock, _m_outPackets);
            for (int i = 0; i < _outPackets.size(); i++)
            {
                if (_outPackets[i].second < priority)
                {
                    if (!_packetQueue.empty())
                    {
                        _outPackets.push_back({ std::move(Packet(MULTIPLE_PACKETS).From(_packetQueue.size())), priority });
                    }
                    while (!_packetQueue.empty())
                    {
                        _outPackets.insert(_outPackets.begin() + i, { std::move(_packetQueue.front()), priority });
                        _packetQueue.pop();
                        i++;
                    }
                    return;
                }
            }
        }

    private:
        int32_t ReceiveBytes(int8_t* buffer, int byteCount)
        {
            int totalBytesReceived = 0;
            while (totalBytesReceived < byteCount)
            {
                int bytesReceived = recv(_socket, (char*)buffer + totalBytesReceived, byteCount - totalBytesReceived, 0);
                if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
                {
                    return bytesReceived;
                }
                totalBytesReceived += bytesReceived;
            }
            return totalBytesReceived;
        }

        int32_t SendBytes(int8_t* buffer, int byteCount)
        {
            int totalBytesSent = 0;
            while (totalBytesSent < byteCount)
            {
                int bytesSent = send(_socket, (char*)buffer + totalBytesSent, byteCount - totalBytesSent, 0);
                if (bytesSent == SOCKET_ERROR)
                {
                    return bytesSent;
                }
                totalBytesSent += bytesSent;
            }
            return totalBytesSent;
        }

        void HandleIncomingPackets()
        {
            auto inBuf = std::make_unique<int8_t[]>(_MAX_PACKET_SIZE);

            std::vector<Packet> packetBufferSplit;
            std::vector<Packet> packetBufferMultiple;
            int queueSizeSplit = 0;
            int queueSizeMultiple = 0;

            while (true)
            {
                //ZeroMemory(inBuf, _MAX_PACKET_SIZE);

                // Wait for packet prefix which contains the packet size
                int32_t byteCount = 4;
                int32_t bytesReceived = ReceiveBytes(inBuf.get(), byteCount);
                if (bytesReceived == SOCKET_ERROR)
                {
                    std::cout << "[Socket '" << _socket << "': error on prefix packet (" << WSAGetLastError() << ")]\n";
                    break;
                }
                if (bytesReceived == 0)
                {
                    std::cout << "[Socket '" << _socket << "': prefix 0 bytes received, closing]\n";
                    break;
                }

                // Wait for packet data
                byteCount = *(int32_t*)inBuf.get();
                bytesReceived = ReceiveBytes(inBuf.get(), byteCount);
                if (bytesReceived == SOCKET_ERROR)
                {
                    std::cout << "[Socket '" << _socket << "': error on data packet (" << WSAGetLastError() << ")]\n";
                    break;
                }
                if (bytesReceived == 0)
                {
                    std::cout << "[Socket '" << _socket << "': data 0 bytes received, closing]\n";
                    break;
                }

                // Extract data
                int32_t packetSize = byteCount - sizeof(int32_t);
                auto bytes = std::make_unique<int8_t[]>(packetSize);
                //std::unique_ptr<int8_t[]> bytes(new int8_t[packetSize]);
                int32_t packetId = *(int32_t*)inBuf.get();
                memcpy(bytes.get(), (char*)inBuf.get() + sizeof(int32_t), packetSize);

                // Check for special types
                if (packetId == SPLIT_PACKET)
                {
                    queueSizeSplit = *(int*)bytes.get();
                    continue;
                }
                else if (packetId == MULTIPLE_PACKETS)
                {
                    if (queueSizeSplit == 0)
                    {
                        queueSizeMultiple = *(int*)bytes.get();
                        continue;
                    }
                }

                Packet packet = Packet(std::move(bytes), packetSize, packetId);

                // Handle split packets
                if (queueSizeSplit > 0)
                {
                    queueSizeSplit--;
                    packetBufferSplit.push_back(std::move(packet));

                    // Combine packets
                    if (queueSizeSplit == 0)
                    {
                        packet = std::move(CombinePackets(packetBufferSplit));
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

                if (!_blockPackets)
                {
                    _m_inPackets.lock();
                    _inPackets.push_back(std::move(packet));
                    _m_inPackets.unlock();
                }
            }

            closesocket(_socket);
            _socket = SOCKET_ERROR;
        }

        void HandleOutgoingPackets()
        {
            auto outBuf = std::make_unique<int8_t[]>(_MAX_PACKET_SIZE);
            //auto outBuf = new int8_t[_MAX_PACKET_SIZE];

            std::vector<Packet> packets;
            while (true)
            {
                packets.clear();
                _m_outPackets.lock();
                if (_outPackets.empty())
                {
                    _m_outPackets.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                packets.push_back(std::move(_outPackets.front().first));
                _outPackets.pop_front();

                // If multiple packets specified, take them out of the queue while it is locked,
                // to prevent any packets getting sent in between
                if (packets[0].id == MULTIPLE_PACKETS)
                {
                    int32_t packetCount = packets[0].Cast<int32_t>();
                    if (packetCount > _outPackets.size())
                    {
                        _m_outPackets.unlock();
                        continue;
                    }
                    for (int i = 0; i < packetCount; i++)
                    {
                        packets.push_back(std::move(_outPackets.front().first));
                        _outPackets.pop_front();
                    }
                }
                _m_outPackets.unlock();

                bool stop = false;
                for (int i = 0; i < packets.size(); i++)
                {
                    // Prefix contains packet size and id
                    size_t prefixSize = sizeof(int32_t) + sizeof(Packet::id);
                    int packetCount = ceil(packets[i].size / (double)(_MAX_PACKET_SIZE - prefixSize));
                    if (packetCount > 1)
                    {
                        // Send info about split packets
                        int packetSize = 4;
                        *(int32_t*)(outBuf.get()) = sizeof(Packet::id) + packetSize;
                        *(int32_t*)(outBuf.get() + 4) = SPLIT_PACKET;
                        *(int32_t*)(outBuf.get() + 8) = packetCount;
                        int bytesSent = SendBytes(outBuf.get(), prefixSize + packetSize);
                        if (bytesSent == SOCKET_ERROR)
                        {
                            std::cout << "[Socket '" << _socket << "': error on split info packet (" << WSAGetLastError() << ")]\n";
                            stop = true;
                            break;
                        }
                        if (bytesSent == 0)
                        {
                            std::cout << "[Socket '" << _socket << "': data 0 bytes sent, closing]\n";
                            stop = true;
                            break;
                        }
                    }

                    // Send in parts (if necessary)
                    int bytesLeft = packets[i].size;
                    int totalBytesSent = 0;
                    do
                    {
                        int byteCount;
                        if (bytesLeft > _MAX_PACKET_SIZE - prefixSize)
                        {
                            *(int32_t*)(outBuf.get()) = _MAX_PACKET_SIZE - 4;
                            *(int32_t*)(outBuf.get() + 4) = packets[i].id;
                            memcpy(outBuf.get() + prefixSize, packets[i].Bytes() + totalBytesSent, _MAX_PACKET_SIZE - prefixSize);
                            totalBytesSent += _MAX_PACKET_SIZE - prefixSize;
                            bytesLeft -= _MAX_PACKET_SIZE - prefixSize;
                            byteCount = _MAX_PACKET_SIZE;
                        }
                        else
                        {
                            *(int32_t*)(outBuf.get()) = bytesLeft + 4;
                            *(int32_t*)(outBuf.get() + 4) = packets[i].id;
                            memcpy(outBuf.get() + prefixSize, packets[i].Bytes() + totalBytesSent, bytesLeft);
                            byteCount = bytesLeft + prefixSize;
                            bytesLeft = 0;
                        }

                        int bytesSent = SendBytes(outBuf.get(), byteCount);
                        if (bytesSent == SOCKET_ERROR)
                        {
                            std::cout << "[Socket '" << _socket << "': error on partial packet (" << WSAGetLastError() << ")]\n";
                            stop = true;
                            break;
                        }
                        if (bytesSent == 0)
                        {
                            std::cout << "[Socket '" << _socket << "': data 0 bytes sent, closing]\n";
                            stop = true;
                            break;
                        }
                    }
                    while (bytesLeft > 0);
                }
                if (stop) break;
            }

            closesocket(_socket);
            _socket = SOCKET_ERROR;
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
            auto combinedData = std::make_unique<int8_t[]>(combinedSize);
            //std::unique_ptr<int8_t> combinedData(new int8_t[combinedSize]);

            // Merge data
            PacketView packetInfo = buffer[0].View();
            int currentByte = 0;
            for (int i = 0; i < buffer.size(); i++)
            {
                int packetSize = buffer[i].View().size;
                memcpy(combinedData.get() + currentByte, buffer[i].Bytes(), packetSize);
                currentByte += packetSize;
            }

            // Clear buffer
            buffer.clear();

            return Packet(std::move(combinedData), combinedSize, packetInfo.id);
        }

        // Push all packets in the buffer at once
        void DepositPacketBuffer(std::vector<Packet>& buffer)
        {
            if (!_blockPackets)
            {
                _m_inPackets.lock();
                for (int i = 0; i < buffer.size(); i++)
                {
                    _inPackets.push_back(std::move(buffer[i]));
                }
                _m_inPackets.unlock();
            }
            buffer.clear();
        }
    };

    // // // //
    struct Client
    {
        friend class TCPConnectionManager;

        std::unique_ptr<TCPConnection> connection;
        int64_t id;
        int* refCount;

    private:
        Client(std::unique_ptr<TCPConnection>&& connection, int64_t id)
            : connection(std::move(connection)), id(id)
        {
            refCount = new int(0);
        }
    public:
        Client(Client&& other)
        {
            connection = std::move(other.connection);
            id = other.id;
            refCount = other.refCount;
            other.id = -1;
            other.refCount = nullptr;
        }
        Client& operator=(Client&& other)
        {
            if (&other != this)
            {
                connection = std::move(other.connection);
                id = other.id;
                refCount = other.refCount;
                other.id = -1;
                other.refCount = nullptr;
            }
            return *this;
        }
        ~Client()
        {
            delete refCount;
        }
    };

    // // // //
    class TCPClientRef
    {
        friend class TCPConnectionManager;

        Client* _client;

        TCPClientRef(Client* client)
            : _client(client)
        {
            if (_client)
            {
                _client->refCount++;
            }
        }
    public:
        ~TCPClientRef()
        {
            if (_client)
            {
                _client->refCount--;
            }
        }
        TCPClientRef(const TCPClientRef& other)
            : TCPClientRef(other._client)
        {}
        TCPClientRef& operator=(const TCPClientRef& other)
        {
            if (this != &other)
            {
                if (_client)
                {
                    _client->refCount--;
                }
                _client = other._client;
                _client->refCount++;
            }
            return *this;
        }
        TCPClientRef(TCPClientRef&& other)
        {
            _client = other._client;
            other._client = nullptr;
        }
        TCPClientRef& operator=(TCPClientRef&& other)
        {
            if (this != &other)
            {
                if (_client)
                {
                    _client->refCount--;
                }
                _client = other._client;
                other._client = nullptr;
            }
            return *this;
        }

        bool Valid() const
        {
            return _client;
        }

        TCPConnection* Get()
        {
            return _client->connection.get();
        }

        TCPConnection* operator->()
        {
            return _client->connection.get();
        }

        TCPConnection& operator*()
        {
            return *(_client->connection);
        }

        int64_t Id() const
        {
            return _client->id;
        }
    };

    // // // //
    class TCPConnectionManager
    {
        std::vector<std::unique_ptr<Client>> _connections;
        std::mutex _m_connections;
        std::thread _garbageCollector;
        bool _canSelfDestruct;

    public:
        static TCPConnectionManager* Create()
        {
            return new TCPConnectionManager();
        }
        // Informs the manager that it can delete itself when all connections are closed and references destroyed.
        // This object must not be accessed after calling this.
        void AllowSelfDestruct()
        {
            _canSelfDestruct = true;
        }
    private:
        TCPConnectionManager()
        {
            _canSelfDestruct = false;
            _garbageCollector = std::thread(&TCPConnectionManager::CollectGarbage, this);
        }
        ~TCPConnectionManager()
        {
            _garbageCollector.detach();
        }

        void CollectGarbage()
        {
            while (true)
            {
                // Clear connections
                _m_connections.lock();
                for (int i = 0; i < _connections.size(); i++)
                {
                    if (_connections[i]->refCount == 0 &&
                        _connections[i]->connection->PacketCount() == 0 &&
                        !_connections[i]->connection->Connected())
                    {
                        _connections.erase(_connections.begin() + i);
                        i--;
                    }
                }
                _m_connections.unlock();

                // Self destruct
                if (_canSelfDestruct)
                {
                    if (_connections.empty())
                    {
                        delete this;
                        return;
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }

    public:
        TCPClientRef AddClient(std::unique_ptr<TCPConnection>&& connection, int64_t id)
        {
            std::unique_ptr<Client> client(new Client(std::move(connection), id));
            // Error here because constructor is private
            //auto client = std::make_unique<Client>(std::move(connection), id);
            Client* clientPtr = client.get();
            LOCK_GUARD(lock, _m_connections);
            _connections.push_back(std::move(client));
            return clientPtr;
        }

        TCPClientRef GetClient(int64_t id)
        {
            LOCK_GUARD(lock, _m_connections);
            for (auto& it : _connections)
            {
                if (it->id == id)
                {
                    return it.get();
                }
            }
            return nullptr;
        }

        TCPClientRef GetFront()
        {
            LOCK_GUARD(lock, _m_connections);
            if (!_connections.empty())
            {
                return _connections.front().get();
            }
            else
            {
                return nullptr;
            }
        }
    };

    //////////////////////
    // TCP SERVER CLASS //
    //////////////////////
    class TCPServer
    {
        bool _running;
        uint16_t _port;
        uint32_t _maxConnections;

        // Existing connections
        TCPConnectionManager* _connectionManager;
        std::vector<TCPClientRef> _connections;
        std::mutex _m_connections;
        std::thread _disconnectHandler;
        bool _disconnectHandlerStop;

        // New connection/disconnection handling
        SOCKET _listeningSocket;
        bool _listening;
        std::thread _newConnectionHandler;
        std::queue<int64_t> _newConnections;
        std::queue<int64_t> _disconnects;
        std::mutex _m_newConnections;
        std::mutex _m_disconnects;

        size_t _connectionMaxPacketSize = 1024;

        // User id generation
    public:
        class IDGenerator
        {
        public:
            virtual int64_t _GetNewID() = 0;
        };
    private:
        IDGenerator* _generator = nullptr;
    public:
        void SetGenerator(IDGenerator* generator)
        {
            _generator = generator;
        }
    private:
        int64_t _ID_COUNTER = 0;
        int64_t _GetNewID()
        {
            if (_generator)
                return _generator->_GetNewID();
            else
                return _ID_COUNTER++;
        }


    public:
        TCPServer()
        {
            _port = -1;
            _maxConnections = 10;
            _running = false;
            _listeningSocket = SOCKET_ERROR;
            _listening = false;
            _ID_COUNTER = 0;
            _connectionManager = TCPConnectionManager::Create();
            _disconnectHandlerStop = false;
            _disconnectHandler = std::thread(&TCPServer::HandleDisconnects, this);
        }
        ~TCPServer()
        {
            StopServer();
            _connectionManager->AllowSelfDestruct();
            _disconnectHandlerStop = true;
            _disconnectHandler.join();
        }

        bool Running()
        {
            return _running;
        }

        bool StartServer(uint16_t port)
        {
            if (_running) return false;
            _port = port;
            _running = true;
            return true;
        }

        void StopServer()
        {
            if (!_running) return;
            _running = false;
            StopNewConnections();
            DisconnectAll();
        }

        /// <param name="enforce">If true, disconnects excess users</param>
        void SetMaxConnections(uint32_t maxConnections, bool enforce = true)
        {
            _maxConnections = maxConnections;
            if (enforce)
            {
                DisconnectExcess();
            }
        }

        bool AllowNewConnections()
        {
            if (!_running) return false;

            _listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (_listeningSocket == INVALID_SOCKET) return false;

            // Bind the ip address and port to a socket
            sockaddr_in hint;
            hint.sin_family = AF_INET;
            hint.sin_port = htons(_port);
            hint.sin_addr.S_un.S_addr = INADDR_ANY;
            bind(_listeningSocket, (sockaddr*)&hint, sizeof(hint));

            // Tell winsock the socket is for listening
            listen(_listeningSocket, SOMAXCONN);

            // Start thread
            _newConnectionHandler = std::thread(&TCPServer::HandleNewConnections, this);

            return true;
        }

        void StopNewConnections()
        {
            if (!_running) return;
            if (!_listening) return;

            _listeningSocket = SOCKET_ERROR;
            closesocket(_listeningSocket);
            _newConnectionHandler.join();
        }

        size_t NewConnectionCount()
        {
            LOCK_GUARD(lock, _m_newConnections);
            return _newConnections.size();
        }

        int64_t GetNewConnection()
        {
            LOCK_GUARD(lock, _m_newConnections);
            if (_newConnections.empty())
            {
                return -1;
            }
            else
            {
                int64_t id = _newConnections.front();
                _newConnections.pop();
                return id;
            }
        }

        std::vector<int64_t> GetCurrentConnections()
        {
            std::vector<int64_t> ids;

            LOCK_GUARD(lock, _m_connections);
            for (int i = 0; i < _connections.size(); i++)
            {
                ids.push_back(_connections[i].Id());
            }
            return ids;
        }

        size_t ConnectionCount()
        {
            LOCK_GUARD(lock, _m_connections);
            return _connections.size();
        }

        bool ConnectionExists(int64_t id)
        {
            LOCK_GUARD(lock, _m_connections);
            for (int i = 0; i < _connections.size(); i++)
            {
                if (_connections[i].Id() == id)
                {
                    return true;
                }
            }
            return false;
        }

        size_t DisconnectedUserCount()
        {
            LOCK_GUARD(lock, _m_disconnects);
            return _disconnects.size();
        }

        int64_t GetDisconnectedUser()
        {
            LOCK_GUARD(lock, _m_disconnects);
            if (_disconnects.empty())
            {
                return -1;
            }
            else
            {
                int64_t id = _disconnects.front();
                _disconnects.pop();
                return id;
            }
        }

        void DisconnectUser(int64_t id)
        {
            LOCK_GUARD(lock, _m_connections);
            for (auto it = _connections.begin(); it != _connections.end(); it++)
            {
                if (it->Id() == id)
                {
                    it->Get()->Disconnect();
                    _connections.erase(it);
                    return;
                }
            }
        }

        void DisconnectAll()
        {
            LOCK_GUARD(lock, _m_connections);
            for (auto it = _connections.begin(); it != _connections.end(); it++)
            {
                it->Get()->Disconnect();
            }
            _connections.clear();
        }
        
        /// <summary>
        /// Disconnects most recent connections until there are less than _maxConnections left
        /// Change _maxConnections with SetMaxConnections()
        /// </summary>
        void DisconnectExcess()
        {
            LOCK_GUARD(lock, _m_connections);
            if (_connections.size() > _maxConnections)
            {
                for (int i = _maxConnections; i < _connections.size(); i++)
                {
                    _connections[i].Get()->Disconnect();
                }
                _connections.erase(_connections.begin() + _maxConnections, _connections.end());
            }
        }

        /// <summary>
        /// All new connections will have the specified maximum packet size and will split larger
        /// packets into smaller ones.
        /// </summary>
        void SetMaxPacketSize(size_t bytes)
        {
            _connectionMaxPacketSize = bytes;
        }

        TCPClientRef Connection(int64_t id)
        {
            return _connectionManager->GetClient(id);
        }

        // Packets are placed in queue after all packets of the same or higher priority
        void Send(Packet&& packet, int64_t userId, int priority = 0)
        {
            LOCK_GUARD(lock, _m_connections);
            auto con = FindConnection(userId);
            if (con)
                con->Send(std::move(packet), priority);
        }

        // Packets in the queue are sent bundled together, with no packets inbetween
        void AddToQueue(Packet&& packet, int64_t userId)
        {
            LOCK_GUARD(lock, _m_connections);
            auto con = FindConnection(userId);
            if (con)
                con->AddToQueue(std::move(packet));
        }

        // Packets are placed in queue after all packets of the same or higher priority
        void SendQueue(int64_t userId, int priority = 0)
        {
            LOCK_GUARD(lock, _m_connections);
            auto con = FindConnection(userId);
            if (con)
                con->SendQueue(priority);
        }

    private:
        // _m_connections MUST BE LOCKED BEFORE CALLING THIS FUNCTION
        TCPConnection* FindConnection(int64_t id)
        {
            for (int i = 0; i < _connections.size(); i++)
            {
                if (_connections[i].Id() == id)
                {
                    return _connections[i].Get();
                }
            }
            return nullptr;
        }

        void HandleNewConnections()
        {
            _listening = true;
            while (true)
            {
                // Wait for a connection
                sockaddr_in socketInfo;
                int socketInfoSize = sizeof(socketInfo);

                if (_listeningSocket == SOCKET_ERROR)
                {
                    break;
                }
                SOCKET socket = accept(_listeningSocket, (sockaddr*)&socketInfo, &socketInfoSize);
                if (socket == SOCKET_ERROR)
                {
                    std::cout << "[Accept error: " << WSAGetLastError() << "]" << std::endl;
                    continue;
                }
                if (_connections.size() >= _maxConnections)
                {
                    closesocket(socket);
                    continue;
                }

                // Add connection
                //std::unique_ptr<TCPConnection> newConnection(new TCPConnection(socket, socketInfo, _connectionMaxPacketSize));
                auto newConnection = std::make_unique<TCPConnection>(socket, socketInfo, _connectionMaxPacketSize);
                int64_t newId = _GetNewID();
                _m_connections.lock();
                _connections.push_back(_connectionManager->AddClient(std::move(newConnection), newId));
                _m_connections.unlock();
                _m_newConnections.lock();
                _newConnections.push(newId);
                _m_newConnections.unlock();

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

            if (_listeningSocket != SOCKET_ERROR)
            {
                closesocket(_listeningSocket);
            }
            _listening = false;
        }

        void HandleDisconnects()
        {
            while (true)
            {
                _m_disconnects.lock();
                _m_connections.lock();
                for (int i = 0; i < _connections.size(); i++)
                {
                    if (!_connections[i]->Connected())
                    {
                        _disconnects.push(_connections[i].Id());
                        _connections.erase(_connections.begin() + i);
                        i--;
                    }
                }
                _m_connections.unlock();
                _m_disconnects.unlock();

                // (very scuffed) (try not to cringe)
                // Sleep for atleast 100 ms in small steps and check the stop flag inbetween
                // This is to minimize the destructor execution time while also minimizing disconnect checking
                for (int i = 0; i < 10; i++)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if (_disconnectHandlerStop)
                    {
                        return;
                    }
                }
            }
        }
    };

    //////////////////////
    // TCP CLIENT CLASS //
    //////////////////////
    class TCPClient
    {
        TCPConnectionManager* _connectionManager;

        // User id generation
    public:
        class IDGenerator
        {
        public:
            virtual int64_t _GetNewID() = 0;
        };
    private:
        IDGenerator* _generator = nullptr;
    public:
        void SetGenerator(IDGenerator* generator)
        {
            _generator = generator;
        }
    private:
        int64_t _ID_COUNTER = 0;
        int64_t _GetID()
        {
            if (_generator)
                return _generator->_GetNewID();
            else
                return _ID_COUNTER++;
        }

    public:
        TCPClient()
        {
            _connectionManager = TCPConnectionManager::Create();
        }
        ~TCPClient()
        {
            while (true)
            {
                TCPClientRef ref = _connectionManager->GetFront();
                if (ref.Valid())
                {
                    ref->BlockPackets();
                    ref->Disconnect();
                }
                else
                {
                    break;
                }
            }
            _connectionManager->AllowSelfDestruct();
        }

        /// <returns>If successful, the id of the connection (>=1); 0 otherwise</returns>
        int64_t Connect(std::string ip, uint16_t port)
        {
            // Create socket
            SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == INVALID_SOCKET) return 0;

            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

            // Connect to server
            int connResult = connect(sock, (sockaddr*)&addr, sizeof(addr));
            if (connResult == SOCKET_ERROR) {
                closesocket(sock);
                return 0;
            }

            int64_t newId = _GetID();
            auto client = std::make_unique<TCPConnection>(sock, addr);
            _connectionManager->AddClient(std::move(client), newId);
            return newId;
        }

        TCPClientRef Connection(int64_t id)
        {
            return _connectionManager->GetClient(id);
        }
    };
}