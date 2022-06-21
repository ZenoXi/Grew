#pragma once

#include "IPlaylistEventHandler.h"

#include "PacketSubscriber.h"
#include "EventSubscriber.h"
#include "NetworkEvents.h"

class PlaylistEventHandler_Server : public IPlaylistEventHandler
{
public:
    void OnAddItemRequest(std::unique_ptr<PlaylistItem> item) {}
    void OnDeleteItemRequest(int64_t itemId) {}
    void OnPlayItemRequest(int64_t itemId) {}
    void OnStopItemRequest(int64_t itemId) {}
    void OnMoveItemRequest(int64_t itemId, int slot) {}

    void Update();

    PlaylistEventHandler_Server(Playlist_Internal* playlist);
    ~PlaylistEventHandler_Server();

private:
    std::unique_ptr<EventReceiver<UserDisconnectedEvent>> _userDisconnectedReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playlistRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _itemAddRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _itemRemoveRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartResponseReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStopRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _itemMoveRequestReceiver = nullptr;

    void _CheckForUserDisconnect();
    void _CheckForPlaylistRequest();
    void _CheckForItemAddRequest();
    void _CheckForItemRemoveRequest();
    void _CheckForStartRequest();
    void _CheckForStartResponse();
    void _CheckForStopRequest();
    void _CheckForItemMoveRequest();

    // Media id generation

    std::mt19937_64 _ENGINE;
    std::uniform_int_distribution<int64_t> _DISTRIBUTION;
    int64_t _GenerateMediaId();
};