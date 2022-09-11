#include "PlaylistEventHandler_Server.h"
#include "Playlist.h"

#include "App.h"
#include "Network.h"
#include "PacketBuilder.h"

#include "Permissions.h"

PlaylistEventHandler_Server::PlaylistEventHandler_Server(Playlist_Internal* playlist)
    : IPlaylistEventHandler(playlist)
{
    _userConnectedReceiver = std::make_unique<EventReceiver<UserConnectedEvent>>(&App::Instance()->events);
    _userDisconnectedReceiver = std::make_unique<EventReceiver<UserDisconnectedEvent>>(&App::Instance()->events);
    _playlistRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::FULL_PLAYLIST_REQUEST);
    _itemAddRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_ADD_REQUEST);
    _itemRemoveRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_REMOVE_REQUEST);
    _playbackStartRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_REQUEST);
    _playbackStartResponseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_RESPONSE);
    _playbackStopRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_STOP_REQUEST);
    _playbackFinishedReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_FINISHED);
    _itemMoveRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_MOVE_REQUEST);

    // Set up media id generator
    _DISTRIBUTION = std::uniform_int_distribution<int64_t>(0, std::numeric_limits<int64_t>::max());
    std::random_device rnd;
    _ENGINE.seed(rnd());
}

PlaylistEventHandler_Server::~PlaylistEventHandler_Server()
{
    // Clear media and user ids
    for (auto& item : _playlist->readyItems)
    {
        item->SetMediaId(-1);
        item->SetUserId(0);
    }
}

void PlaylistEventHandler_Server::Update()
{
    _CheckForUserDisconnect();
    // Playlist requests should be processed before item add requests
    // to prevent incorrect playlist ordering for new clients
    _CheckForPlaylistRequest();
    _CheckForItemAddRequest();
    _CheckForItemRemoveRequest();
    _CheckForStartRequest();
    _CheckForStartResponse();
    _CheckForStopRequest();
    _CheckForPlaybackFinish();
    _CheckForItemMoveRequest();

    _UpdateStartOrderTimeout();
}

void PlaylistEventHandler_Server::_CheckForUserConnect()
{
    while (_userConnectedReceiver->EventCount() > 0)
    {
        auto ev = _userConnectedReceiver->GetEvent();

        // Send options
    }
}

void PlaylistEventHandler_Server::_CheckForUserDisconnect()
{
    while (_userDisconnectedReceiver->EventCount() > 0)
    {
        auto ev = _userDisconnectedReceiver->GetEvent();
        
        // Remove items hosted by user
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetUserId() == ev.userId)
            {
                // Except if it is currently being played
                if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyPlaying)
                {
                    _playlist->readyItems[i]->SetUserId(MISSING_HOST_ID);
                    continue;
                }

                // If it is starting, abort the start process
                if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyStarting)
                {
                    std::vector<int64_t> destUsers{ _playlist->readyItems[i]->GetUserId() };
                    if (destUsers[0] != _playlist->startRequestIssuer)
                        destUsers.push_back(_playlist->startRequestIssuer);
                    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), destUsers);

                    _playlist->currentlyStarting = -1;
                    _playlist->startRequestIssuer = -1;
                }

                // Send remove order to all clients
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE).From(_playlist->readyItems[i]->GetMediaId()), APP_NETWORK->UserIds(true));

                // Remove from playlist
                _playlist->readyItems.erase(_playlist->readyItems.begin() + i);
                i--;
            }
        }
    }
}

void PlaylistEventHandler_Server::_CheckForPlaylistRequest()
{
    while (_playlistRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playlistRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        // Send all items
        for (auto& item : _playlist->readyItems)
        {
            std::wstring filename = item->GetFilename();

            // Construct packet
            PacketBuilder builder = PacketBuilder();
            builder.Add(item->GetMediaId())
                .Add(int64_t(0))
                .Add(item->GetUserId())
                .Add(item->GetDuration().GetTicks())
                .Add(filename.data(), filename.length());

            // Send packet back to request issuer
            APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_ADD), { userId });
        }
    }
}

void PlaylistEventHandler_Server::_CheckForItemAddRequest()
{
    while (_itemAddRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemAddRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t callbackId = reader.Get<int64_t>();
        int64_t mediaDuration = reader.Get<int64_t>();
        size_t remainingBytes = reader.RemainingBytes();
        size_t filenameLength = remainingBytes / sizeof(wchar_t);
        std::wstring filename;
        filename.resize(filenameLength);
        reader.Get((wchar_t*)filename.data(), filename.length());

        // Check permissions
        auto user = App::Instance()->users.GetUser(userId);
        if (!user || !user->GetPermission(PERMISSION_ADD_ITEMS))
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_ADD_DENIED).From(callbackId), { userId }, 1);
            continue;
        }

        // Create item
        auto item = std::make_unique<PlaylistItem>();
        item->SetFilename(filename);
        item->SetDuration(Duration(mediaDuration));
        int64_t mediaId = _GenerateMediaId();
        item->SetMediaId(mediaId);
        item->SetUserId(userId);

        // Construct packet
        PacketBuilder builder = PacketBuilder();
        builder.Add(item->GetMediaId())
            .Add(callbackId)
            .Add(userId)
            .Add(mediaDuration)
            .Add(filename.data(), filename.length());

        // Send item packet to all clients
        APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_ADD), APP_NETWORK->UserIds(true));

        // Add item to internal playlist
        _playlist->readyItems.push_back(std::move(item));
    }
}

void PlaylistEventHandler_Server::_CheckForItemRemoveRequest()
{
    while (_itemRemoveRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemRemoveRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t mediaId = reader.Get<int64_t>();

        // Check permissions
        auto user = App::Instance()->users.GetUser(userId);
        if (!user || !user->GetPermission(PERMISSION_MANAGE_ITEMS))
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE_DENIED), { userId }, 1);
            continue;
        }

        // Find item
        bool itemFound = false;
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                itemFound = true;

                if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyStarting ||
                    _playlist->readyItems[i]->GetItemId() == _playlist->currentlyPlaying)
                {
                    // Send deny response
                    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE_DENIED).From(mediaId), { userId });
                    break;
                }

                // Send remove order to all clients
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE).From(mediaId), APP_NETWORK->UserIds(true));

                _playlist->readyItems.erase(_playlist->readyItems.begin() + i);
                break;
            }
        }
        if (!itemFound)
        {
            // Send deny response
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE_DENIED).From(mediaId), { userId });
        }
    }
}

void PlaylistEventHandler_Server::_CheckForStartRequest()
{
    while (_playbackStartRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStartRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int64_t mediaId = reader.Get<int64_t>();

        // Check permissions
        auto user = App::Instance()->users.GetUser(userId);
        if (!user || !user->GetPermission(PERMISSION_START_STOP_PLAYBACK))
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { userId }, 1);
            continue;
        }

        if (_playlist->currentlyStarting != -1)
        {
            // Send deny response to issuer
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { userId }, 1);
            continue;
        }

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                // Check if host is still connected
                if (_playlist->readyItems[i]->GetUserId() == MISSING_HOST_ID)
                {
                    // Send deny response to issuer
                    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { userId });
                    break;
                }

                // Send play order to host
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_ORDER).From(mediaId), { _playlist->readyItems[i]->GetUserId() });
                
                _playlist->currentlyStarting = _playlist->readyItems[i]->GetItemId();
                _playlist->startRequestIssuer = userId;
                _startOrderSend = ztime::Main();
                break;
            }
        }

        // Item doesn't exist, send deny response to issuer
        APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { userId });
    }
}

void PlaylistEventHandler_Server::_CheckForStartResponse()
{
    while (_playbackStartResponseReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStartResponseReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int8_t response = reader.Get<int8_t>();

        // TODO: add abort response to host

        if (_playlist->currentlyStarting != -1)
        {
            if (response == 1)
            {
                // Find item
                for (int i = 0; i < _playlist->readyItems.size(); i++)
                {
                    if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyStarting)
                    {
                        int64_t mediaId = _playlist->readyItems[i]->GetMediaId();
                        int64_t hostId = _playlist->readyItems[i]->GetUserId();
                        PacketBuilder builder = PacketBuilder(16);
                        builder.Add(mediaId).Add(hostId);

                        // Send play start order to all clients except the host
                        auto userIds = APP_NETWORK->UserIds(true);
                        for (int j = 0; j < userIds.size(); j++)
                        {
                            if (userIds[j] == _playlist->readyItems[i]->GetUserId())
                            {
                                userIds.erase(userIds.begin() + j);
                                break;
                            }
                        }
                        APP_NETWORK->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYBACK_START), userIds);

                        _playlist->currentlyPlaying = _playlist->readyItems[i]->GetItemId();
                        break;
                    }
                }
            }
            else if (response == 0)
            {
                // Send response to issuer
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { _playlist->startRequestIssuer });
            }

            _playlist->currentlyStarting = -1;
            _playlist->startRequestIssuer = -1;
        }
    }
}

void PlaylistEventHandler_Server::_CheckForStopRequest()
{
    while (_playbackStopRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackStopRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        // Check permissions
        auto user = App::Instance()->users.GetUser(userId);
        if (!user || !user->GetPermission(PERMISSION_START_STOP_PLAYBACK))
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP_DENIED), { userId }, 1);
            continue;
        }

        if (_playlist->currentlyPlaying == -1)
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP_DENIED), { userId }, 1);
            continue;
        }

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyPlaying)
            {
                // Sent playback stop order to all clients
                APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP), APP_NETWORK->UserIds(true));

                _playlist->currentlyPlaying = -1;
                break;
            }
        }
    }
}

void PlaylistEventHandler_Server::_CheckForPlaybackFinish()
{
    while (_playbackFinishedReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _playbackFinishedReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        if (_playlist->currentlyPlaying == -1)
            continue;

        // Check if user is host
        bool userIsHost = false;
        int itemIndex = -1;
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetUserId() == userId)
            {
                userIsHost = true;
                itemIndex = i;
                break;
            }
        }
        if (!userIsHost)
            continue;

        // Sent playback stop order to all clients
        APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP), APP_NETWORK->UserIds(true));
        _playlist->currentlyPlaying = -1;

        // Send self request to play next item
        if (_playlist->options.autoplay && _playlist->readyItems.size() > itemIndex + 1)
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_REQUEST)
                .From(_playlist->readyItems[itemIndex + 1]->GetMediaId()), { 0 }, 1);
        }
    }
}

void PlaylistEventHandler_Server::_CheckForItemMoveRequest()
{
    while (_itemMoveRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        auto packetPair = _itemMoveRequestReceiver->GetPacket();
        znet::Packet packet = std::move(packetPair.first);
        int64_t userId = packetPair.second;

        PacketReader reader = PacketReader(packet.Bytes(), packet.size);
        int32_t slot = reader.Get<int32_t>();
        int64_t callbackId = reader.Get<int64_t>();
        std::vector<int64_t> mediaIds;
        size_t itemCount = reader.RemainingBytes() / sizeof(int64_t);
        mediaIds.resize(itemCount);
        reader.Get(mediaIds.data(), itemCount);

        // Check permissions
        auto user = App::Instance()->users.GetUser(userId);
        if (!user || !user->GetPermission(PERMISSION_MANAGE_ITEMS))
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_MOVE_DENIED).From(callbackId), { userId }, 1);
            continue;
        }

        if (slot + mediaIds.size() > _playlist->readyItems.size() || slot < 0)
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_MOVE_DENIED).From(callbackId), { userId }, 1);
            continue;
        }

        // Convert media ids to item ids
        std::vector<int64_t> itemIds;
        bool notAllPresent = false;
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
            if (newId == -1)
            {
                notAllPresent = true;
                break;
            }
            itemIds.push_back(newId);
        }
        // If not all items are present, deny the request
        if (notAllPresent)
        {
            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_MOVE_DENIED).From(callbackId), { userId }, 1);
            continue;
        }

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

        // Create move order packet (with callback id)
        PacketBuilder builderWIC;
        builderWIC.Add(slot);
        builderWIC.Add(callbackId);
        builderWIC.Add(mediaIds.data(), mediaIds.size());
        // Create move order packet (without callback id)
        PacketBuilder builderWOC;
        builderWOC.Add(slot);
        builderWOC.Add(int64_t(-1));
        builderWOC.Add(mediaIds.data(), mediaIds.size());

        // Send move order to all clients (sender also receives its callback id)
        std::vector<int64_t> userIds = APP_NETWORK->UserIds(true);
        for (int i = 0; i < userIds.size(); i++)
        {
            if (userIds[i] == userId)
            {
                userIds.erase(userIds.begin() + i);
                break;
            }
        }
        APP_NETWORK->Send(znet::Packet(builderWIC.Release(), builderWIC.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_MOVE), { userId }, 1);
        APP_NETWORK->Send(znet::Packet(builderWOC.Release(), builderWOC.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_MOVE), userIds, 1);

        //// Find item
        //for (int i = 0; i < _playlist->readyItems.size(); i++)
        //{
        //    if (_playlist->readyItems[i]->GetMediaId() == mediaId)
        //    {
        //        if (slot == i)
        //        {
        //            APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_MOVE_DENIED).From(mediaId), { userId });
        //            break;
        //        }

        //        int oldIndex = i;
        //        int newIndex = slot;
        //        auto& v = _playlist->readyItems;
        //        if (oldIndex > newIndex)
        //            std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex, v.rend() - newIndex);
        //        else
        //            std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1, v.begin() + newIndex + 1);

        //        // Sent item move order to all clients
        //        struct MoveData { int64_t mediaId; int32_t slot; } moveData{ mediaId, slot };
        //        APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_MOVE).From(moveData), APP_NETWORK->UserIds(true));
        //        // Not that useful (though interesting), but moveData could be replaced by:
        //        // struct { int64_t mediaId; int32_t slot; } { mediaId, slot }

        //        break;
        //    }
        //}
    }
}

void PlaylistEventHandler_Server::_UpdateStartOrderTimeout()
{
    if (_playlist->currentlyStarting != -1)
    {
        // Abort start process
        if ((ztime::Main() - _startOrderSend) > _startOrderTimeout)
        {
            // Find item
            for (int i = 0; i < _playlist->readyItems.size(); i++)
            {
                if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyStarting)
                {
                    std::vector<int64_t> destUsers{ _playlist->readyItems[i]->GetUserId() };
                    if (destUsers[0] != _playlist->startRequestIssuer)
                        destUsers.push_back(_playlist->startRequestIssuer);
                    APP_NETWORK->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), destUsers);

                    _playlist->currentlyStarting = -1;
                    _playlist->startRequestIssuer = -1;
                }
            }
        }
    }
}

int64_t PlaylistEventHandler_Server::_GenerateMediaId()
{
    return _DISTRIBUTION(_ENGINE);
}