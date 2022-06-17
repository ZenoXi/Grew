#pragma once

#include "PlaylistItem.h"

#include <memory>
#include <cstdint>

class Playlist_Internal;

class IPlaylistEventHandler
{
public:
    virtual void OnAddItemRequest(std::unique_ptr<PlaylistItem> item) = 0;
    virtual void OnDeleteItemRequest(int64_t itemId) = 0;
    virtual void OnPlayItemRequest(int64_t itemId) = 0;
    virtual void OnStopItemRequest(int64_t itemId) = 0;
    virtual void OnMoveItemRequest(int64_t itemId, int slot) = 0;

    virtual void Update() = 0;

    IPlaylistEventHandler(Playlist_Internal* playlist) : _playlist(playlist) {};
    virtual ~IPlaylistEventHandler() {};
    IPlaylistEventHandler(IPlaylistEventHandler&&) = delete;
    IPlaylistEventHandler& operator=(IPlaylistEventHandler&&) = delete;
    IPlaylistEventHandler(const IPlaylistEventHandler&) = delete;
    IPlaylistEventHandler& operator=(const IPlaylistEventHandler&) = delete;

protected:
    Playlist_Internal* _playlist = nullptr;
};