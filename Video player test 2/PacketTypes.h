#pragma once

namespace znet
{
    enum class PacketType
    {
        // // // // // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //
        // TRAFFIC/CONNECTION CONTROL // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //

        // Contains (client -> server):
        //  array of int64_t - destination user ids
        // Contains (server -> client):
        //  int64_t - source user id
        USER_ID = 0,

        // Sent when to the user when he connects
        // Contains:
        //  int64_t - the assigned id
        ASSIGNED_USER_ID,

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

        // Sent by the server periodically to check whether connection is broken
        KEEP_ALIVE,

        // Data about a packet that will arrive split into multiple small packets
        // Contains:
        //  int64_t - split id, used to identify packets belonging to this split
        //  int32_t - packet id
        //  int32_t - number of parts
        //  size_t - total packet size
        SPLIT_PACKET_HEAD,

        // Part of split packet data
        // Contains:
        //  int64_t - split id
        //  int32_t - packet id (included here as well to allow AbortSend() to work corectly)
        //  remaining bytes - data
        SPLIT_PACKET_PART,

        // Signals that the specified number of parts will not arrive and already
        // received parts should be discarded
        // Contains:
        //  int64_t - split id
        SPLIT_PACKET_ABORT,

        // // // // // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //
        // PLAYBACK // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //

        // Sent by media receiver after pausing
        PAUSE_REQUEST = 100,

        // Sent by media host to all receivers after pausing, or after receiving the PAUSE_REQUEST packet
        // Contains:
        //  int64_t - pause issuer user id
        PAUSE,

        // See PAUSE_REQUEST
        RESUME_REQUEST,

        // See PAUSE
        RESUME,

        // Sent to the host when a receiver wants to seek/change stream
        // Contains:
        //  int64_t - time point to seek to, in 'TimePoint' ticks
        //  int8_t - unused
        //  int32_t - new video stream index
        //  int32_t - new audio stream index
        //  int32_t - new subtitle stream index
        //  int64_t - unused
        SEEK_REQUEST,

        // Sent by the media host to all receivers after seeking, or receiving the SEEK_REQUEST packet
        // Contains:
        //  int64_t - time point to seek to, in 'TimePoint' ticks
        //  int8_t - '1': the time point shouldn't be regarded as a time seek command (for notifications)
        //  int32_t - new video stream index
        //  int32_t - new audio stream index
        //  int32_t - new subtitle stream index
        //  int64_t - seek issuer user id
        INITIATE_SEEK,

        // Sent by the media host data provider to all receiver data providers when it seeks.
        // It signals that media packets received after this one are from a different stream/time
        SEEK_DISCONTINUITY,

        // Sent to the host when the media player recovers after a seek
        SEEK_FINISHED,

        // Sent by the host to everyone when it receives confirmation
        // from all receivers that they recovered
        HOST_SEEK_FINISHED,

        // Video packet data
        // Contains:
        //  a serialized MediaPacket object
        VIDEO_PACKET,

        // Audio packet data
        // Contains:
        //  a serialized MediaPacket object
        AUDIO_PACKET,

        // Subtitle packet data
        // Contains:
        //  a serialized MediaPacket object
        SUBTITLE_PACKET,

        // Sent by user if it wants the playback of some media, which it does not host
        // Contains:
        //  int32_t - media id
        //PLAYBACK_REQUEST,

        // Sent by the host to all users after starting playback
        // Contains:
        //  int32_t - media id
        //PLAYBACK_START,

        // Sent by host before metadata
        // Contains:
        //  array of int64_t values - participating user ids
        PLAYBACK_PARTICIPANT_LIST,
        
        // Sent before providing streams
        // Contains:
        //  int8_t - video stream count
        //  int8_t - audio stream count
        //  int8_t - subtitle stream count
        //  int8_t - attachment stream count
        //  int8_t - data stream count
        //  int8_t - unknown stream count
        //  int8_t - current video stream
        //  int8_t - current audio stream
        //  int8_t - current subtitle stream
        //  int8_t - chapter count
        STREAM_METADATA,

        // Sent by the receiver after receiving all metadata
        METADATA_CONFIRMATION,

        // Sent by the receiver when its playback controller sets up its packet receivers
        CONTROLLER_READY,

        // Sent to all receivers when hosts' controller is ready
        HOST_CONTROLLER_READY,

        // Sent by the receiver after their media player loads
        LOAD_FINISHED,

        // Video stream data
        // Contains:
        //  a serialized MediaStream object
        VIDEO_STREAM,

        // Audio stream data
        // Contains:
        //  a serialized MediaStream object
        AUDIO_STREAM,

        // Subtitle stream data
        // Contains:
        //  a serialized MediaStream object
        SUBTITLE_STREAM,

        // Attachment stream data
        // Contains:
        //  a serialized MediaStream object
        ATTACHMENT_STREAM,

        // Data stream data
        // Contains:
        //  a serialized MediaStream object
        DATA_STREAM,

        // Unknown stream data
        // Contains:
        //  a serialized MediaStream object
        UNKNOWN_STREAM,

        // Chapter data
        // Contains:
        //  a serialized MediaChapter object
        CHAPTER,

        // Current receiver playback position
        // Contains:
        //  int64_t - TimePoint ticks
        PLAYBACK_POSITION,

        // Sent by the host to synchronize playback
        // Contains:
        //  int64_t - Duration ticks how long to pause for
        SYNC_PAUSE,

        // // // // // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //
        // PLAYBACK QUEUE // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //
        // // // // // // // // // // // // // // // // // // //

        // Sent when a client wants to add an item to the playback queue
        // Contains:
        //  int64_t - callback id, used by the client to differentiate between request confirmations
        //  int64_t - media duration in 'Duration' ticks
        //  remaining bytes - wstring of the media name
        PLAYLIST_ITEM_ADD_REQUEST = 200,

        // Sent to all clients when an item is added to the queue
        // Contains:
        //  int64_t - media id (if -1, this packet signifies a declined operation and is sent only to the request issuer)
        //  int64_t - callback id (only used by the request issuer)
        //  int64_t - media host user id
        //  int64_t - media duration in 'Duration' ticks
        //  remaining bytes - wstring of the media name
        PLAYLIST_ITEM_ADD,

        // Sent by the server to client when its item add request is denied
        // Contains:
        //  int64_t - callback id
        PLAYLIST_ITEM_ADD_DENIED,

        // Sent when a client wants to remove an item from the playback queue
        // Contains:
        //  int64_t - media id
        PLAYLIST_ITEM_REMOVE_REQUEST,

        // Sent to all clients when an item is removed from the queue
        // Contains:
        //  int64_t - media id
        PLAYLIST_ITEM_REMOVE,

        // Sent by the server to client when its item remove request is denied
        // Contains:
        //  int64_t - media id
        PLAYLIST_ITEM_REMOVE_DENIED,

        // (client/server -> server)
        // Sent when a client/server wishes for playback to start
        // Contains:
        //  int64_t - media id
        PLAYBACK_START_REQUEST,

        // (server -> media host)
        // Indicates that the host should prepare for playback
        // Contains:
        //  int64_t - media id
        PLAYBACK_START_ORDER,

        // Sent by the media host when it is ready for playback or declines the playback
        // Contains:
        //  int8_t - 0: playback declined | 1: playback ready
        PLAYBACK_START_RESPONSE,

        // Sent by the server to playback issuer when host denies playback
        PLAYBACK_START_DENIED,

        // Sent by the server to all clients (except host) to begin playback
        // Contains:
        //  int64_t - mediaId
        //  int64_t - hostId
        PLAYBACK_START,

        // Sent by the receiver when its data provider is ready to receive packets,
        // or when it declines playback
        // Contains:
        //  int8_t - whether the user will participate in the playback (0: accepted | 1: declined)
        PLAYBACK_CONFIRMATION,

        // Sent when a client wants to stop currently playing item
        PLAYBACK_STOP_REQUEST,

        // Sent to all clients when playback should be aborted
        PLAYBACK_STOP,

        // Sent by the server to client when its playback stop request is denied
        PLAYBACK_STOP_DENIED,

        // Sent when a client wants to change the position of an item in the playlist
        // Contains:
        //  int64_t - mediaId
        //  int32_t - new slot (0-index)
        PLAYLIST_ITEM_MOVE_REQUEST,

        // Sent by the server to all clients when the position of an item in the playlist changes
        // Contains:
        //  int64_t - mediaId
        //  int32_t - new slot (0-index)
        PLAYLIST_ITEM_MOVE,

        // Sent by the server to client when its item move request is denied
        // Contains:
        //  int64_t - media id
        PLAYLIST_ITEM_MOVE_DENIED,

        // A request from client to server to send all playlist items
        FULL_PLAYLIST_REQUEST,
    };
}