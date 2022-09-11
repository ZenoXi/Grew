#pragma once

#include "IPlaylistEventHandler.h"

class PlaylistEventHandler_None : public IPlaylistEventHandler
{
public:
    void OnAddItemRequest(std::unique_ptr<PlaylistItem> item) {}
    void OnDeleteItemRequest(int64_t itemId) {}
    void OnPlayItemRequest(int64_t itemId) {}
    void OnStopItemRequest(int64_t itemId, bool playbackFinished) {}
    void OnMoveItemRequest(std::vector<int64_t> itemIds, int slot) {}

    void Update() {}

    PlaylistEventHandler_None(Playlist_Internal* playlist);
    ~PlaylistEventHandler_None() {}
};
