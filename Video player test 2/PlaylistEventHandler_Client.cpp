#include "PlaylistEventHandler_Client.h"
#include "Playlist.h"

#include "App.h"
#include "PlaylistEvents.h"
#include "PlaybackScene.h"
#include "MediaReceiverDataProvider.h"
#include "MediaHostDataProvider.h"
#include "ReceiverPlaybackController.h"
#include "HostPlaybackController.h"
#include "Network.h"
#include "PacketBuilder.h"

PlaylistEventHandler_Client::PlaylistEventHandler_Client(Playlist_Internal* playlist)
    : IPlaylistEventHandler(playlist)
{
    _userDisconnectedReceiver   = std::make_unique<EventReceiver<UserDisconnectedEvent>>(&App::Instance()->events);
    _itemAddReceiver            = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_ADD);
    _itemAddDenyReceiver        = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_ADD_DENIED);
    _itemRemoveReceiver         = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_REMOVE);
    _itemRemoveDenyReceiver     = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_REMOVE_DENIED);
    _playbackStartOrderReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_ORDER);
    _playbackStartReceiver      = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START);
    _playbackStartDenyReceiver  = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_DENIED);
    _playbackStopReceiver       = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_STOP);
    _playbackStopDenyReceiver   = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_STOP_DENIED);
    _itemMoveReceiver           = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_MOVE);
    _itemMoveDenyReceiver       = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_MOVE_DENIED);

    // Save user id
    _userId = APP_NETWORK->ThisUser().id;

    // Remove all items from other users
    auto it = std::remove_if(
        _playlist->readyItems.begin(),
        _playlist->readyItems.end(),
        [](const std::unique_ptr<PlaylistItem>& item) { return item->GetUserId() != OFFLINE_LOCAL_HOST_ID; }
    );
    _playlist->readyItems.erase(it, _playlist->readyItems.end());

    // Request full playlist
    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::FULL_PLAYLIST_REQUEST), { 0 }, 1);

    // Move ready items to pending list
    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        auto& item = _playlist->readyItems[i];

        // Construct packet
        std::wstring filename = item->GetFilename();
        PacketBuilder builder = PacketBuilder();
        builder.Add(item->GetItemId())
            .Add(item->GetDuration())
            .Add(filename.data(), filename.length());

        // Send add request
        APP_NETWORK->
            Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_ADD_REQUEST), { 0 }, 1);

        // Move item to pending list
        _playlist->pendingItems.push_back(std::move(_playlist->readyItems[i]));
    }
    _playlist->readyItems.clear();

    App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
}

PlaylistEventHandler_Client::~PlaylistEventHandler_Client()
{
    // Clear media and user ids
    for (auto& item : _playlist->readyItems)
    {
        item->SetMediaId(-1);
        if (item->GetUserId() == _userId)
            item->SetUserId(OFFLINE_LOCAL_HOST_ID);
        else
            item->SetUserId(MISSING_HOST_ID);
    }
}

void PlaylistEventHandler_Client::Update()
{
    _CheckForUserDisconnect();
    _CheckForItemAdd();
    _CheckForItemAddDeny();
    _CheckForItemRemove();
    _CheckForItemRemoveDeny();
    _CheckForStartOrder();
    _CheckForStart();
    _CheckForStop();
    _CheckForStopDeny();
    _CheckForItemMove();
    _CheckForItemMoveDeny();

    _ManageLoadingItems();
    _TrackParticipations();
}

void PlaylistEventHandler_Client::OnAddItemRequest(std::unique_ptr<PlaylistItem> item)
{
    _playlist->loadingItems.push_back(std::move(item));
    App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
}

void PlaylistEventHandler_Client::OnDeleteItemRequest(int64_t itemId)
{
    // Check ready items
    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            // Send delete request
            APP_NETWORK->
                Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE_REQUEST).From(_playlist->readyItems[i]->GetMediaId()), { 0 }, 1);

            _playlist->pendingItemDeletes.push_back(_playlist->readyItems[i]->GetItemId());
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

void PlaylistEventHandler_Client::OnPlayItemRequest(int64_t itemId)
{
    if (_playlist->currentlyStarting != -1)
        return;

    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            // Send play request
            APP_NETWORK->
                Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_REQUEST).From(_playlist->readyItems[i]->GetMediaId()), { 0 }, 1);

            _playlist->currentlyStarting = itemId;
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            return;
        }
    }
}

void PlaylistEventHandler_Client::OnStopItemRequest(int64_t itemId, bool playbackFinished)
{
    for (int i = 0; i < _playlist->readyItems.size(); i++)
    {
        if (_playlist->readyItems[i]->GetItemId() == itemId)
        {
            // Abort playback start
            if (_playlist->currentlyStarting != -1)
            {
                _playlist->currentlyStarting = -1;
                if (_playbackParticipationTracker)
                    _playbackParticipationTracker.reset();
            }

            if (playbackFinished)
            {
                // Send playback finished notification
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_FINISHED), { 0 }, 1);
            }
            else
            {
                // Send stop request
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP_REQUEST), { 0 }, 1);

                _playlist->pendingItemStop = itemId;
                App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
            }
            return;
        }
    }
}

void PlaylistEventHandler_Client::OnMoveItemRequest(std::vector<int64_t> itemIds, int slot)
{
    if (slot + itemIds.size() > _playlist->readyItems.size() || slot < 0)
        return;

    int64_t callbackId = _callbackCounter++;

    // Build and send move request
    PacketBuilder builder;
    builder.Add((int32_t)slot);
    builder.Add(callbackId);
    for (auto& id : itemIds)
    {
        // Get item media id
        for (auto& item : _playlist->readyItems)
        {
            if (item->GetItemId() == id)
            {
                builder.Add(item->GetMediaId());
                break;
            }
        }
    }
    APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_MOVE_REQUEST), { 0 }, 1);

    _playlist->pendingItemMoves.push_back({ itemIds, slot, callbackId });
    App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});

    //for (int i = 0; i < _playlist->readyItems.size(); i++)
    //{
    //    if (_playlist->readyItems[i]->GetItemId() == itemId)
    //    {
    //        // Moving to current slot is allowed only when a different move is already pending
    //        bool movePending = false;
    //        for (auto& move : _playlist->pendingItemMoves)
    //        {
    //            if (move.first == itemId)
    //            {
    //                movePending = true;
    //                break;
    //            }
    //        }
    //        if (slot == i && !movePending)
    //            return;

    //        // Send move request
    //        PacketBuilder builder = PacketBuilder(12);
    //        builder.Add(_playlist->readyItems[i]->GetMediaId()).Add(slot);
    //        APP_NETWORK->
    //            Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_MOVE_REQUEST), { 0 });

    //        _playlist->pendingItemMoves.push_back({ itemId, slot });
    //        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    //        return;
    //    }
    //}
}

void PlaylistEventHandler_Client::_CheckForUserDisconnect()
{
    while (_userDisconnectedReceiver->EventCount() > 0)
    {
        auto ev = _userDisconnectedReceiver->GetEvent();

        // Mark hosted items as 'host missing'
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetUserId() == ev.userId)
            {
                _playlist->readyItems[i]->SetUserId(MISSING_HOST_ID);
            }
        }

        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    }
}

void PlaylistEventHandler_Client::_CheckForItemAdd()
{
    while (_itemAddReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemAddReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t mediaId = reader.Get<int64_t>();
        int64_t callbackId = reader.Get<int64_t>();
        int64_t hostId = reader.Get<int64_t>();
        int64_t mediaDuration = reader.Get<int64_t>();
        size_t remainingBytes = reader.RemainingBytes();
        size_t filenameLength = remainingBytes / sizeof(wchar_t);
        std::wstring filename;
        filename.resize(filenameLength);
        reader.Get((wchar_t*)filename.data(), filename.length());

        if (hostId == APP_NETWORK->ThisUser().id)
        {
            for (int i = 0; i < _playlist->pendingItems.size(); i++)
            {
                if (_playlist->pendingItems[i]->GetItemId() == callbackId)
                {
                    // Move confirmed item
                    _playlist->pendingItems[i]->SetMediaId(mediaId);
                    _playlist->pendingItems[i]->SetUserId(hostId);
                    _playlist->pendingItems[i]->SetCustomStatus(L"");
                    _playlist->readyItems.push_back(std::move(_playlist->pendingItems[i]));
                    _playlist->pendingItems.erase(_playlist->pendingItems.begin() + i);
                }
            }
        }
        else
        {
            // Create item
            auto item = std::make_unique<PlaylistItem>();
            item->SetFilename(filename);
            item->SetDuration(Duration(mediaDuration));
            item->SetMediaId(mediaId);
            item->SetUserId(hostId);

            // Add item to internal playlist
            _playlist->readyItems.push_back(std::move(item));
        }

        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    }
}

void PlaylistEventHandler_Client::_CheckForItemAddDeny()
{
    while (_itemAddDenyReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemAddDenyReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        int64_t callbackId = packet.Cast<int64_t>();

        // Find corresponding item
        for (int i = 0; i < _playlist->pendingItems.size(); i++)
        {
            if (_playlist->pendingItems[i]->GetItemId() == callbackId)
            {
                // Move to failed list
                _playlist->pendingItems[i]->SetCustomStatus(L"Denied");
                _playlist->pendingItems[i]->SetMediaId(ADD_DENIED_MEDIA_ID);
                _playlist->failedItems.push_back(std::move(_playlist->pendingItems[i]));
                _playlist->pendingItems.erase(_playlist->pendingItems.begin() + i);
                App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
                break;
            }
        }
    }
}

void PlaylistEventHandler_Client::_CheckForItemRemove()
{
    while (_itemRemoveReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemRemoveReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t mediaId = reader.Get<int64_t>();

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                // Remove from pending
                for (int j = 0; j < _playlist->pendingItemDeletes.size(); j++)
                {
                    if (_playlist->pendingItemDeletes[j] == _playlist->readyItems[i]->GetItemId())
                    {
                        _playlist->pendingItemDeletes.erase(_playlist->pendingItemDeletes.begin() + j);
                        break;
                    }
                }

                _playlist->readyItems.erase(_playlist->readyItems.begin() + i);
                App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
                break;
            }
        }
    }
}

void PlaylistEventHandler_Client::_CheckForItemRemoveDeny()
{
    while (_itemRemoveDenyReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemRemoveDenyReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        int64_t mediaId = packet.Cast<int64_t>();

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                // Remove from pending
                for (int j = 0; j < _playlist->pendingItemDeletes.size(); j++)
                {
                    if (_playlist->pendingItemDeletes[j] == _playlist->readyItems[i]->GetItemId())
                    {
                        _playlist->pendingItemDeletes.erase(_playlist->pendingItemDeletes.begin() + j);
                        break;
                    }
                }
                App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
                break;
            }
        }
    }
}

void PlaylistEventHandler_Client::_CheckForStartOrder()
{
    while (_playbackStartOrderReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStartOrderReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t mediaId = reader.Get<int64_t>();

        // Find item
        int itemIndex = -1;
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                itemIndex = i;
                break;
            }
        }

        if (itemIndex != -1)
        {
            // Init participation receiver
            auto users = APP_NETWORK->Users();
            std::vector<int64_t> userIds;
            for (auto& user : users)
                userIds.push_back(user.id);
            _playbackParticipationTracker = std::make_unique<PlaybackParticipationTracker>(userIds);
            _playlist->currentlyStarting = _playlist->readyItems[itemIndex]->GetItemId();

            // Send playback confirmation
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_RESPONSE).From(int8_t(1)), { 0 });
        }
        else
        {
            _playlist->currentlyStarting = -1;

            // Send playback decline
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_RESPONSE).From(int8_t(0)), { 0 });
        }

        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    }
}

void PlaylistEventHandler_Client::_CheckForStart()
{
    while (_playbackStartReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStartReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t mediaId = reader.Get<int64_t>();
        int64_t hostId = reader.Get<int64_t>();

        // Find item
        int itemIndex = -1;
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                itemIndex = i;
                break;
            }
        }

        if (itemIndex != -1)
        {
            App::Instance()->playback.Stop();
            auto dataProvider = std::make_unique<MediaReceiverDataProvider>(hostId);
            auto controller = std::make_unique<ReceiverPlaybackController>(dataProvider.get(), hostId);
            App::Instance()->playback.Start(std::move(dataProvider), std::move(controller));
            App::Instance()->ReinitScene(PlaybackScene::StaticName(), nullptr);
            _playlist->currentlyPlaying = _playlist->readyItems[itemIndex]->GetItemId();
            _playlist->currentlyStarting = -1;

            // Send participation confirmation
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_CONFIRMATION).From(int8_t(0)), { hostId });
        }
        else
        {
            // Send participation decline
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_CONFIRMATION).From(int8_t(1)), { hostId });
        }

        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    }
}

void PlaylistEventHandler_Client::_CheckForStartDeny()
{
    while (_playbackStartDenyReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStartReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        // Abort playback start
        if (_playlist->currentlyStarting != -1)
        {
            _playlist->currentlyStarting = -1;
            if (_playbackParticipationTracker)
                _playbackParticipationTracker.reset();

            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        }
    }
}

void PlaylistEventHandler_Client::_CheckForStop()
{
    while (_playbackStopReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStopReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        if (_playlist->currentlyPlaying == -1)
            continue;

        App::Instance()->playback.Stop();
        App::Instance()->UninitScene(PlaybackScene::StaticName());
        _playlist->currentlyPlaying = -1;
        _playlist->pendingItemStop = -1;

        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
    }
}

void PlaylistEventHandler_Client::_CheckForStopDeny()
{
    while (_playbackStopDenyReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStopDenyReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        if (_playlist->pendingItemStop != -1)
        {
            _playlist->pendingItemStop = -1;
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        }
    }
}

void PlaylistEventHandler_Client::_CheckForItemMove()
{
    while (_itemMoveReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemMoveReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int32_t slot = reader.Get<int32_t>();
        int64_t callbackId = reader.Get<int64_t>();
        std::vector<int64_t> mediaIds;
        size_t itemCount = reader.RemainingBytes() / sizeof(int64_t);
        mediaIds.resize(itemCount);
        reader.Get(mediaIds.data(), itemCount);

        // Remove from pending
        if (callbackId != -1)
        {
            for (int i = 0; i < _playlist->pendingItemMoves.size(); i++)
            {
                if (_playlist->pendingItemMoves[i]._Get_rest()._Get_rest()._Myfirst._Val == callbackId)
                {
                    _playlist->pendingItemMoves.erase(_playlist->pendingItemMoves.begin() + i);
                    break;
                }
            }
        }

        // Convert media ids to item ids
        for (int i = 0; i < mediaIds.size(); i++)
        {
            int64_t newId = -1;
            for (auto& item : _playlist->readyItems)
            {
                if (item->GetMediaId() == mediaIds[i])
                {
                    newId = item->GetItemId();
                    break;
                }
            }
            if (newId == -1) // Should not realistically happen
            {
                mediaIds.erase(mediaIds.begin() + i);
                i--;
                continue;
            }

            mediaIds[i] = newId;
        }

        if (slot + mediaIds.size() > _playlist->readyItems.size() || slot < 0)
            continue; // Realistically should not be hit

        // Reorder items

        // Calculate item count before (N) and after (M) insertion slot
        // Move N non-moved items to front
        // Move M non-moved items to back

        int beforeCount = slot;
        int afterCount = _playlist->readyItems.size() - beforeCount - mediaIds.size();

        // Repeat until all necessary items have been moved to front (preserving order)
        for (int i = 0; i < beforeCount; i++)
        {
            // Find next item to bring to front
            int index = -1;
            for (int j = i; j < _playlist->readyItems.size(); j++)
            {
                if (std::find(mediaIds.begin(), mediaIds.end(), _playlist->readyItems[j]->GetItemId()) == mediaIds.end())
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
                if (std::find(mediaIds.begin(), mediaIds.end(), _playlist->readyItems[j]->GetItemId()) == mediaIds.end())
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

        // Find item
        //for (int i = 0; i < _playlist->readyItems.size(); i++)
        //{
        //    if (_playlist->readyItems[i]->GetMediaId() == mediaId)
        //    {
        //        if (slot == i)
        //            break; // Realistically should not be hit

        //        // Remove from pending
        //        for (int j = 0; j < _playlist->pendingItemMoves.size(); j++)
        //        {
        //            if (_playlist->pendingItemMoves[j].first == _playlist->readyItems[i]->GetItemId() &&
        //                _playlist->pendingItemMoves[j].second == slot)
        //            {
        //                _playlist->pendingItemMoves.erase(_playlist->pendingItemMoves.begin() + j);
        //                break;
        //            }
        //        }

        //        int oldIndex = i;
        //        int newIndex = slot;
        //        auto& v = _playlist->readyItems;
        //        if (oldIndex > newIndex)
        //            std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex, v.rend() - newIndex);
        //        else
        //            std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1, v.begin() + newIndex + 1);

        //        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        //        break;
        //    }
        //}
    }
}

void PlaylistEventHandler_Client::_CheckForItemMoveDeny()
{
    while (_itemMoveDenyReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemMoveDenyReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;
        if (userId != 0)
            continue;

        int64_t callbackId = packet.Cast<int64_t>();

        // Remove from pending
        if (callbackId != -1)
        {
            for (int i = 0; i < _playlist->pendingItemMoves.size(); i++)
            {
                if (_playlist->pendingItemMoves[i]._Get_rest()._Get_rest()._Myfirst._Val == callbackId)
                {
                    _playlist->pendingItemMoves.erase(_playlist->pendingItemMoves.begin() + i);
                    break;
                }
            }
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        }

        //// Find item
        //for (int i = 0; i < _playlist->readyItems.size(); i++)
        //{
        //    if (_playlist->readyItems[i]->GetMediaId() == mediaId)
        //    {
        //        // Remove from pending
        //        for (int j = 0; j < _playlist->pendingItemMoves.size(); j++)
        //        {
        //            if (_playlist->pendingItemMoves[j].first == _playlist->readyItems[i]->GetItemId())
        //            {
        //                _playlist->pendingItemMoves.erase(_playlist->pendingItemMoves.begin() + j);
        //                break;
        //            }
        //        }
        //        App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        //        break;
        //    }
        //}
    }
}

void PlaylistEventHandler_Client::_ManageLoadingItems()
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
            // Construct packet
            std::wstring filename = item->GetFilename();
            PacketBuilder builder = PacketBuilder();
            builder.Add(item->GetItemId())
                .Add(item->GetDuration())
                .Add(filename.data(), filename.length());

            // Send add request
            APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_ADD_REQUEST), { 0 });

            // Move item to pending list
            _playlist->pendingItems.push_back(std::move(_playlist->loadingItems.front()));
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

void PlaylistEventHandler_Client::_TrackParticipations()
{
    if (_playbackParticipationTracker)
    {
        // AllReceived() is true if timeout is reached
        if (_playbackParticipationTracker->AllReceived())
        {
            auto participants = _playbackParticipationTracker->Accepted();

            // Find item
            int itemIndex = -1;
            for (int i = 0; i < _playlist->readyItems.size(); i++)
            {
                if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyStarting)
                {
                    itemIndex = i;
                    break;
                }
            }

            if (itemIndex != -1)
            {
                // Start playback
                App::Instance()->playback.Stop();
                auto dataProvider = std::make_unique<MediaHostDataProvider>(_playlist->readyItems[itemIndex]->CopyDataProvider(), participants);
                auto controller = std::make_unique<HostPlaybackController>(dataProvider.get(), participants);
                App::Instance()->playback.Start(std::move(dataProvider), std::move(controller));
                App::Instance()->ReinitScene(PlaybackScene::StaticName(), nullptr);
                _playlist->currentlyPlaying = _playlist->readyItems[itemIndex]->GetItemId();
            }
            else
            {
                // Send stop request
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP_REQUEST), { 0 });
            }
            
            _playlist->currentlyStarting = -1;
            _playbackParticipationTracker.reset();
            App::Instance()->events.RaiseEvent(PlaylistChangedEvent{});
        }
    }
}