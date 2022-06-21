#include "PlaylistEventHandler_Server.h"
#include "Playlist.h"

#include "App.h"
#include "NetworkInterfaceNew.h"
#include "PacketBuilder.h"

PlaylistEventHandler_Server::PlaylistEventHandler_Server(Playlist_Internal* playlist)
    : IPlaylistEventHandler(playlist)
{
    _userDisconnectedReceiver = std::make_unique<EventReceiver<UserDisconnectedEvent>>(&App::Instance()->events);
    _playlistRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::FULL_PLAYLIST_REQUEST);
    _itemAddRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_ADD_REQUEST);
    _itemRemoveRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYLIST_ITEM_REMOVE_REQUEST);
    _playbackStartRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_REQUEST);
    _playbackStartResponseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_RESPONSE);
    _playbackStopRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_STOP_REQUEST);
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
    // TODO: add timeout for playback order response

    _CheckForUserDisconnect();
    // Playlist requests should be processed before item add requests
    // to prevent incorrect playlist ordering for new clients
    _CheckForPlaylistRequest();
    _CheckForItemAddRequest();
    _CheckForItemRemoveRequest();
    _CheckForStartRequest();
    _CheckForStartResponse();
    _CheckForStopRequest();
    _CheckForItemMoveRequest();
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

                // Send remove order to all clients
                auto users = znet::NetworkInterface::Instance()->Users();
                std::vector<int64_t> userIds;
                userIds.push_back(0);
                for (auto& user : users)
                    userIds.push_back(user.id);
                znet::NetworkInterface::Instance()->
                    Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE).From(_playlist->readyItems[i]->GetMediaId()), userIds);

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
            znet::NetworkInterface::Instance()->
                Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_ADD), { userId });
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
        auto users = znet::NetworkInterface::Instance()->Users();
        std::vector<int64_t> userIds;
        userIds.push_back(0);
        for (auto& user : users)
            userIds.push_back(user.id);
        znet::NetworkInterface::Instance()->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYLIST_ITEM_ADD), userIds);

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

        // TODO: add decline response if item doesn't exist
        // TODO: add decline response if playback of item is starting

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                // Send remove order to all clients
                auto users = znet::NetworkInterface::Instance()->Users();
                std::vector<int64_t> userIds;
                userIds.push_back(0);
                for (auto& user : users)
                    userIds.push_back(user.id);
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_REMOVE).From(mediaId), userIds);

                _playlist->readyItems.erase(_playlist->readyItems.begin() + i);
                break;
            }
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

        if (_playlist->currentlyStarting != -1)
        {
            // Send response to issuer
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { userId });
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
                    // Send response to issuer
                    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { userId });
                    break;
                }

                // Send play order to host
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_ORDER).From(mediaId), { _playlist->readyItems[i]->GetUserId() });

                _playlist->currentlyStarting = _playlist->readyItems[i]->GetItemId();
                _playlist->startRequestIssuer = userId;
                break;
            }
        }
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
                        auto users = znet::NetworkInterface::Instance()->Users();
                        std::vector<int64_t> userIds;
                        if (_playlist->readyItems[i]->GetUserId() != 0)
                            userIds.push_back(0);
                        for (auto& user : users)
                            if (user.id != _playlist->readyItems[i]->GetUserId())
                                userIds.push_back(user.id);
                        znet::NetworkInterface::Instance()->Send(znet::Packet(builder.Release(), builder.UsedBytes(), (int)znet::PacketType::PLAYBACK_START), userIds);

                        _playlist->currentlyPlaying = _playlist->readyItems[i]->GetItemId();
                        break;
                    }
                }
            }
            else if (response == 0)
            {
                // Send response to issuer
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_DENIED), { _playlist->startRequestIssuer });
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

        if (_playlist->currentlyPlaying == -1)
            continue;

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetItemId() == _playlist->currentlyPlaying)
            {
                // Sent playback stop order to all clients
                auto users = znet::NetworkInterface::Instance()->Users();
                std::vector<int64_t> userIds;
                userIds.push_back(0);
                for (auto& user : users)
                    userIds.push_back(user.id);
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP), userIds);

                _playlist->currentlyPlaying = -1;
                break;
            }
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
        int64_t mediaId = reader.Get<int64_t>();
        int32_t slot = reader.Get<int32_t>();

        if (slot >= _playlist->readyItems.size())
            continue; // TODO: add decline response

        // Find item
        for (int i = 0; i < _playlist->readyItems.size(); i++)
        {
            if (_playlist->readyItems[i]->GetMediaId() == mediaId)
            {
                if (slot == i)
                    break; // TODO: add decline response

                int oldIndex = i;
                int newIndex = slot;
                auto& v = _playlist->readyItems;
                if (oldIndex > newIndex)
                    std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex, v.rend() - newIndex);
                else
                    std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1, v.begin() + newIndex + 1);

                // Sent item move order to all clients
                auto users = znet::NetworkInterface::Instance()->Users();
                std::vector<int64_t> userIds;
                userIds.push_back(0);
                for (auto& user : users)
                    userIds.push_back(user.id);
                struct MoveData { int64_t mediaId; int32_t slot; } moveData{ mediaId, slot };
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYLIST_ITEM_MOVE).From(moveData), userIds);
                // Not that useful (though interesting), but moveData could be replaced by:
                // struct { int64_t mediaId; int32_t slot; } { mediaId, slot }

                break;
            }
        }
    }
}

int64_t PlaylistEventHandler_Server::_GenerateMediaId()
{
    return _DISTRIBUTION(_ENGINE);
}