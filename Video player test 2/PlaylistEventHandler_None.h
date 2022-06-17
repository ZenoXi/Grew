#pragma once

#include "IPlaylistEventHandler.h"

class PlaylistEventHandler_None : public IPlaylistEventHandler
{
public:
    void OnAddItemRequest(std::unique_ptr<PlaylistItem> item) {}
    void OnDeleteItemRequest(int64_t itemId) {}
    void OnPlayItemRequest(int64_t itemId) {}
    void OnStopItemRequest(int64_t itemId) {}
    void OnMoveItemRequest(int64_t itemId, int slot) {}

    void Update() {}

    PlaylistEventHandler_None(Playlist_Internal* playlist);
    ~PlaylistEventHandler_None() {}
};
