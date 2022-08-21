#include "App.h"
#include "Playlist.h"

#include "PlaylistEventHandler_Offline.h"

// Playlist_Internal
void Playlist_Internal::Reset()
{
    readyItems.clear();
    pendingItems.clear();
    loadingItems.clear();
    pendingItemPlay = -1;
    pendingItemStop = -1;
    pendingItemDeletes.clear();
    pendingItemMoves.clear();
    currentlyPlaying = -1;
    currentlyStarting = -1;
    startRequestIssuer = -1;
}

// Playlist

Playlist::Playlist()
{
    // Alloc playlist internals
    _playlist = std::make_unique<Playlist_Internal>();

    // Init offline event handler
    _eventHandler = std::make_unique<PlaylistEventHandler_Offline>(_playlist.get());
}

Playlist::~Playlist()
{

}

void Playlist::Update()
{
    _eventHandler->Update();
}

void Playlist::Request_AddItem(std::unique_ptr<PlaylistItem> item)
{
    _eventHandler->OnAddItemRequest(std::move(item));
}

void Playlist::Request_DeleteItem(int64_t itemId)
{
    _eventHandler->OnDeleteItemRequest(itemId);
}

void Playlist::Request_PlayItem(int64_t itemId)
{
    _eventHandler->OnPlayItemRequest(itemId);
}

void Playlist::Request_StopItem(int64_t itemId)
{
    _eventHandler->OnStopItemRequest(itemId);
}

void Playlist::Request_MoveItems(std::vector<int64_t> itemIds, int slot)
{
    _eventHandler->OnMoveItemRequest(itemIds, slot);
}

std::vector<PlaylistItem*> Playlist::ReadyItems() const
{
    std::vector<PlaylistItem*> items;
    for (auto& item : _playlist->readyItems)
        items.push_back(item.get());
    return items;
}

std::vector<PlaylistItem*> Playlist::PendingItems() const
{
    std::vector<PlaylistItem*> items;
    for (auto& item : _playlist->pendingItems)
        items.push_back(item.get());
    return items;
}

std::vector<PlaylistItem*> Playlist::LoadingItems() const
{
    std::vector<PlaylistItem*> items;
    for (auto& item : _playlist->loadingItems)
        items.push_back(item.get());
    return items;
}

std::vector<PlaylistItem*> Playlist::FailedItems() const
{
    std::vector<PlaylistItem*> items;
    for (auto& item : _playlist->failedItems)
        items.push_back(item.get());
    return items;
}

std::vector<PlaylistItem*> Playlist::AllItems() const
{
    std::vector<PlaylistItem*> items;
    for (auto& item : _playlist->readyItems)
        items.push_back(item.get());
    for (auto& item : _playlist->pendingItems)
        items.push_back(item.get());
    for (auto& item : _playlist->loadingItems)
        items.push_back(item.get());
    for (auto& item : _playlist->failedItems)
        items.push_back(item.get());
    return items;
}

PlaylistItem* Playlist::GetItem(int64_t itemId) const
{
    auto allItems = AllItems();
    for (auto& item : allItems)
        if (item->GetItemId() == itemId)
            return item;
    return nullptr;
}

int64_t Playlist::PendingItemPlay() const
{
    return _playlist->pendingItemPlay;
}

int64_t Playlist::PendingItemStop() const
{
    return _playlist->pendingItemStop;
}

std::vector<int64_t> Playlist::PendingItemDeletes() const
{
    return _playlist->pendingItemDeletes;
}

std::vector<std::pair<std::vector<int64_t>, int>> Playlist::PendingItemMoves() const
{
    std::vector<std::pair<std::vector<int64_t>, int>> itemMoves;
    itemMoves.reserve(_playlist->pendingItemMoves.size());
    for (auto& move : _playlist->pendingItemMoves)
    {
        std::vector<int64_t> ids = move._Myfirst._Val;
        int slot = move._Get_rest()._Myfirst._Val;
        itemMoves.push_back({ ids, slot });
    }
    return itemMoves;
}

int64_t Playlist::CurrentlyPlaying() const
{
    return _playlist->currentlyPlaying;
}

int64_t Playlist::CurrentlyStarting() const
{
    return _playlist->currentlyStarting;
}