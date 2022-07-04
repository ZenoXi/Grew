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

void PlaylistEventHandler_Offline::OnMoveItemRequest(int64_t itemId, int slot)
{
    if (slot >= _playlist->readyItems.size() || slot < 0)
        return;

    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            if (slot == i)
                return;

            int oldIndex = i;
            int newIndex = slot;
            auto& v = _playlist->readyItems;
            if (oldIndex > newIndex)
                std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex, v.rend() - newIndex);
            else
                std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1, v.begin() + newIndex + 1);

            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }
}

void PlaylistEventHandler_Offline::_ManageLoadingItems()
{
    for (int i = 0; i < _playlist->loadingItems.size(); i++)
    {
        auto& item = _playlist->loadingItems[i];
        if (!item->DataProvider()->Initializing())
        {
            if (!item->DataProvider()->InitFailed())
            {
                // Move item to ready list
                _playlist->readyItems.push_back(std::move(_playlist->loadingItems[i]));
                _playlist->loadingItems.erase(_playlist->loadingItems.begin() + i);
            }
            else
            {
                // Move to failed list
                _playlist->loadingItems[i]->SetCustomStatus(L"Init failed..");
                _playlist->failedItems.push_back(std::move(_playlist->loadingItems[i]));
                _playlist->loadingItems.erase(_playlist->loadingItems.begin() + i);
            }

            i--;
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        }
    }
}