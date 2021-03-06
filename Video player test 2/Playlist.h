#pragma once

#include "PlaylistItem.h"
#include "IPlaylistEventHandler.h"

#include <vector>

struct ServerOptions
{
    int maxClients;
    bool clientCanAddItem;
    bool clientCanDeleteItem;
    bool clientCanPlayItem;
    bool clientCanStopItem;
    bool clientCanMoveItem;

    // std::string ToString() const { return ""; }
    // void FromString(std::string optString) {}
};

class Playlist_Internal
{
public:
    std::vector<std::unique_ptr<PlaylistItem>> readyItems;
    std::vector<std::unique_ptr<PlaylistItem>> pendingItems;
    std::vector<std::unique_ptr<PlaylistItem>> loadingItems;
    std::vector<std::unique_ptr<PlaylistItem>> failedItems;

    int64_t pendingItemPlay = -1;
    int64_t pendingItemStop = -1;
    std::vector<int64_t> pendingItemDeletes;
    std::vector<std::pair<int64_t, int>> pendingItemMoves;

    int64_t currentlyPlaying = -1;
    int64_t currentlyStarting = -1;
    int64_t startRequestIssuer = -1; // TODO: move to server handler

    ServerOptions options;

    void Reset();
};

class Playlist
{
public:
    Playlist();
    ~Playlist();

    void Request_AddItem(std::unique_ptr<PlaylistItem> item);
    void Request_DeleteItem(int64_t itemId);
    void Request_PlayItem(int64_t itemId);
    void Request_StopItem(int64_t itemId);
    void Request_MoveItem(int64_t itemId, int slot);

    std::vector<PlaylistItem*> ReadyItems() const;
    std::vector<PlaylistItem*> PendingItems() const;
    std::vector<PlaylistItem*> LoadingItems() const;
    std::vector<PlaylistItem*> FailedItems() const;
    std::vector<PlaylistItem*> AllItems() const;
    int64_t PendingItemPlay() const;
    int64_t PendingItemStop() const;
    std::vector<int64_t> PendingItemDeletes() const;
    std::vector<std::pair<int64_t, int>> PendingItemMoves() const;
    int64_t CurrentlyPlaying() const;
    int64_t CurrentlyStarting() const;

    void SetOptions(ServerOptions opt);
    ServerOptions GetOptions() const;

    void Update();

    template<class _Hnd, class... _Args>
    void SetEventHandler(_Args... args);

private:
    std::unique_ptr<Playlist_Internal> _playlist = nullptr;
    std::unique_ptr<IPlaylistEventHandler> _eventHandler = nullptr;

private:
};

template<class _Hnd, class... _Args>
void Playlist::SetEventHandler(_Args... args)
{
    // Handler destructor and constructor takes care of necessary cleanup
    _eventHandler.reset();
    _eventHandler = std::make_unique<_Hnd>(_playlist.get(), std::forward<_Args>(args)...);
}