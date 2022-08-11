#include "PlaylistEventHandler_Offline.h"
#include "Playlist.h"

#include "App.h"
#include "PlaylistEvents.h"
#include "PlaybackScene.h"

PlaylistEventHandler_Offline::PlaylistEventHandler_Offline(Playlist_Internal* playlist)
    : IPlaylistEventHandler(playlist)
{
}

PlaylistEventHandler_Offline::~PlaylistEventHandler_Offline()
{
    // Stop playback
    if (_playlist->currentlyPlaying != -1)
    {
        App::Instance()->playback.Stop();
        App::Instance()->UninitScene(PlaybackScene::StaticName());
        _playlist->currentlyPlaying = -1;
    }
}

void PlaylistEventHandler_Offline::Update()
{
    _ManageLoadingItems();

    // Check for playback end
    if (_playlist->currentlyPlaying != -1 && App::Instance()->playback.Controller()->Finished())
    {
        App::Instance()->playback.Stop();
        _playlist->currentlyPlaying = -1;
    }
}

void PlaylistEventHandler_Offline::OnAddItemRequest(std::unique_ptr<PlaylistItem> item)
{
    _playlist->loadingItems.push_back(std::move(item));
    App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
}

void PlaylistEventHandler_Offline::OnDeleteItemRequest(int64_t itemId)
{
    // Check ready items
    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            _playlist->readyItems.erase(_playlist->readyItems.begin() + i);
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }

    // Check loading items
    for (int i = 0; i < _playlist->loadingItems.size(); i++)
    {
        if (_playlist->loadingItems[i]->GetItemId() == itemId)
        {
            _playlist->loadingItems.erase(_playlist->loadingItems.begin() + i);
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }

    // Check pending items
    for (int i = 0; i < _playlist->pendingItems.size(); i++)
    {
        if (_playlist->pendingItems[i]->GetItemId() == itemId)
        {
            _playlist->pendingItems.erase(_playlist->pendingItems.begin() + i);
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }

    // Check failed items
    for (int i = 0; i < _playlist->failedItems.size(); i++)
    {
        if (_playlist->failedItems[i]->GetItemId() == itemId)
        {
            _playlist->failedItems.erase(_playlist->failedItems.begin() + i);
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }
}

void PlaylistEventHandler_Offline::OnPlayItemRequest(int64_t itemId)
{
    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            auto dataProvider = _playlist->readyItems[i]->CopyDataProvider();
            auto controller = std::make_unique<BasePlaybackController>(dataProvider.get());
            App::Instance()->playback.Stop();
            App::Instance()->playback.Start(std::move(dataProvider), std::move(controller));
            App::Instance()->ReinitScene(PlaybackScene::StaticName(), nullptr);
            _playlist->currentlyPlaying = itemId;
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }
    for (int i = 0; i < _playlist->loadingItems.size(); i++)
    {
        if (_playlist->loadingItems[i]->GetItemId() == itemId)
        {
            auto dataProvider = std::make_unique<LocalFileDataProvider>(_playlist->loadingItems[i]->DataProvider()->GetFilename());
            //auto dataProvider = _playlist->readyItems[i]->CopyDataProvider();
            auto controller = std::make_unique<BasePlaybackController>(dataProvider.get());
            App::Instance()->playback.Stop();
            App::Instance()->playback.Start(std::move(dataProvider), std::move(controller));
            App::Instance()->ReinitScene(PlaybackScene::StaticName(), nullptr);
            _playlist->currentlyPlaying = itemId;
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }
}

void PlaylistEventHandler_Offline::OnStopItemRequest(int64_t itemId)
{
    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            if (_playlist->currentlyPlaying == itemId)
            {
                App::Instance()->playback.Stop();
                App::Instance()->UninitScene(PlaybackScene::StaticName());
                _playlist->currentlyPlaying = -1;
                App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            }
            return;
        }
    }
}

void PlaylistEventHandler_Offline::OnMoveItemRequest(std::vector<int64_t> itemIds, int slot)
{
    if (slot + itemIds.size() > _playlist->readyItems.size() || slot < 0)
        return;

    // Reorder items

    // Calculate item count before (N) and after (M) insertion slot
    // Move N non-moved items to front
    // Move M non-moved items to back

    int beforeCount = slot;
    int afterCount = _playlist->readyItems.size() - beforeCount - itemIds.size();

    // Repeat until all necessary items have been moved to front (preserving order)
    for (int i = 0; i < beforeCount; i++)
    {
        // Find next item to bring to front
        int index = -1;
        for (int j = i; j < _playlist->readyItems.size(); j++)
        {
            if (std::find(itemIds.begin(), itemIds.end(), _playlist->readyItems[j]->GetItemId()) == itemIds.end())
            {
                index = j;
                break;
            }
        }
        for (int j = index; j > i; j--)
        {
            // Bring to front
            std::swap(_playlist->readyItems[j], _playlist->readyItems[j - 1]);
        }
    }

    // Repeat until all necessary items have been moved to back (preserving order)
    for (int i = 0; i < afterCount; i++)
    {
        // Find next item to bring to front
        int index = -1;
        for (int j = _playlist->readyItems.size() - 1 - i; j >= 0; j--)
        {
            if (std::find(itemIds.begin(), itemIds.end(), _playlist->readyItems[j]->GetItemId()) == itemIds.end())
            {
                index = j;
                break;
            }
        }
        for (int j = index; j < _playlist->readyItems.size() - 1 - i; j++)
        {
            // Bring to back
            std::swap(_playlist->readyItems[j], _playlist->readyItems[j + 1]);
        }
    }

    App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});

    //for (int i = 0; i < _playlist->readyItems.size(); i++)
    //{
    //    if (_playlist->readyItems[i]->GetItemId() == itemId)
    //    {
    //        if (slot == i)
    //            return;

    //        int oldIndex = i;
    //        int newIndex = slot;
    //        auto& v = _playlist->readyItems;
    //        if (oldIndex > newIndex)
    //            std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex, v.rend() - newIndex);
    //        else
    //            std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1, v.begin() + newIndex + 1);

    //        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    //        return;
    //    }
    //}
}

void PlaylistEventHandler_Offline::_ManageLoadingItems()
{
    if (_playlist->loadingItems.empty())
        return;

    // Initialize 1 item at a time, because initializing multiple
    // files from a hard drive makes a jarring sound on my machine
    // and takes a very long time to complete (I assume this happens
    // because while initializing multiple files at once the disk
    // must perform many random reads, while initializing a single
    // file uses more sequential reads)
    auto& item = _playlist->loadingItems.front();
    if (!item->InitStarted())
        item->StartInitializing();

    if (!item->DataProvider()->Initializing())
    {
        if (!item->DataProvider()->InitFailed())
        {
            // Move item to ready list
            _playlist->readyItems.push_back(std::move(_playlist->loadingItems.front()));
            _playlist->loadingItems.erase(_playlist->loadingItems.begin());
        }
        else
        {
            // Move to failed list
            _playlist->loadingItems.front()->SetCustomStatus(L"Init failed..");
            _playlist->failedItems.push_back(std::move(_playlist->loadingItems.front()));
            _playlist->loadingItems.erase(_playlist->loadingItems.begin());
        }
        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    }
}