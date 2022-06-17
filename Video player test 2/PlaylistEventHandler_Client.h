#pragma once

#include "IPlaylistEventHandler.h"

#include "PacketSubscriber.h"

#include <memory>

class PlaylistEventHandler_Client : public IPlaylistEventHandler
{
public:
    void OnAddItemRequest(std::unique_ptr<PlaylistItem> item);
    void OnDeleteItemRequest(int64_t itemId);
    void OnPlayItemRequest(int64_t itemId);
    void OnStopItemRequest(int64_t itemId);
    void OnMoveItemRequest(int64_t itemId, int slot);

    void Update();

    PlaylistEventHandler_Client(Playlist_Internal* playlist);
    ~PlaylistEventHandler_Client();

private:
    int64_t _userId = -1;

    std::unique_ptr<znet::PacketReceiver> _itemAddReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _itemRemoveReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartOrderReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStopReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _itemMoveReceiver = nullptr;

    void _CheckForItemAdd();
    void _CheckForItemRemove();
    void _CheckForStartOrder();
    void _CheckForStart();
    void _CheckForStop();
    void _CheckForItemMove();

    void _ManageLoadingItems();

    class PlaybackParticipationTracker : public znet::PacketSubscriber
    {
        std::vector<int64_t> _users;
        std::vector<int64_t> _accepted;
        Clock _timeoutTimer;
        TimePoint _timeout;

        void _OnPacketReceived(znet::Packet packet, int64_t userId)
        {
            for (int i = 0; i < _users.size(); i++)
            {
                if (_users[i] == userId)
                {
                    if (packet.Cast<int8_t>() == 0)
                        _accepted.push_back(_users[i]);
                    _users.erase(_users.begin() + i);
                }
            }
        }
    public:
        PlaybackParticipationTracker(std::vector<int64_t> users, Duration timeout = Duration(3, SECONDS))
            : PacketSubscriber((int32_t)znet::PacketType::PLAYBACK_CONFIRMATION),
            _users(users),
            _timeoutTimer(Clock(0)),
            _timeout(timeout.GetTicks())
        {
        }

        bool AllReceived()
        {
            _timeoutTimer.Update();
            return _users.empty() || _timeoutTimer.Now() > _timeout;
        }

        std::vector<int64_t> Accepted()
        {
            return _accepted;
        }
    };
    std::unique_ptr<PlaybackParticipationTracker> _playbackParticipationTracker = nullptr;
    void _TrackParticipations();
};