#pragma once

namespace znet
{
    enum class PacketType
    {
        //
        // Traffic/connection control
        //

        // Contains (client -> server):
        //  array of int64_t - destination user ids
        // Contains (server -> client):
        //  int64_t - source user id
        USER_ID = 0,
        // Contains:
        //  int64_t - amount of bytes confirmed to have been received
        BYTE_CONFIRMATION,
        // Sent by client before terminating connection
        // Used to signal an intentional disconnection
        DISCONNECT_REQUEST,
        // Contains:
        //  int64_t - user id
        //  remaining bytes - wchar_t string
        USER_DATA,
        // Sent by the server to all clients when a new user connects
        // Contains:
        //  int64_t - new user id
        NEW_USER,
        // Sent by the server to all clients when a user disconnects
        // Contains:
        //  int64_t - disconnected user id
        DISCONNECTED_USER,
        // Sent by the server to a new connection
        // Contains:
        //  array of int64_t - currently connected user ids
        USER_LIST,

        //
        // Playback
        //

        PAUSE = 100,
        RESUME,
        SEEK,
        VIDEO_PACKET,
        AUDIO_PACKET,
        SUBTITLE_PACKET,
        STREAM,

        //
        // Other (placeholder)
        //

        OTHER = 200
    };
}