#pragma once

#include "GameTime.h"

#include <cstdint>
#include <string>

struct ServerStartEvent
{
    static const char* _NAME_() { return "server_start"; }
    uint16_t port;
};

struct ServerStopEvent
{
    static const char* _NAME_() { return "server_stop"; }
};

struct ConnectionStartEvent
{
    static const char* _NAME_() { return "connection_start"; }
    std::string ip;
    uint16_t port;
};

struct ConnectionSuccessEvent
{
    static const char* _NAME_() { return "connection_success"; }
};

struct ConnectionFailEvent
{
    static const char* _NAME_() { return "connection_fail"; }
    std::string msg;
};

struct DisconnectEvent
{
    static const char* _NAME_() { return "disconnect"; }
};

struct ConnectionLostEvent
{
    static const char* _NAME_() { return "connection_lost"; }
};

struct ConnectionClosedEvent
{
    static const char* _NAME_() { return "connection_closed"; }
};

struct UserConnectedEvent
{
    static const char* _NAME_() { return "user_connected"; }
    int64_t userId;
};

struct UserDisconnectedEvent
{
    static const char* _NAME_() { return "user_disconnected"; }
    int64_t userId;
};

struct UserNameChangedEvent
{
    static const char* _NAME_() { return "user_name_changed"; }
    int64_t userId;
    std::wstring newName;
};

struct NetworkStatsEvent
{
    static const char* _NAME_() { return "network_stats"; }
    Duration timeInterval;
    int64_t bytesSent;
    int64_t bytesReceived;
    int64_t latency; // Microseconds
};