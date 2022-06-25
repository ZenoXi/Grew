#pragma once

#include "Scene.h"
#include "PlaybackScene.h"
#include "PacketSubscriber.h"
#include "PlaylistEvents.h"

#include "Label.h"
#include "TextInput.h"
#include "OverlayPlaylistItem.h"

#include "FileDialog.h"

class PlaybackOverlayShortcutHandler : public KeyboardEventHandler
{
    bool _OnKeyDown(BYTE vkCode) { return false; }
    bool _OnKeyUp(BYTE vkCode) { return false; }
    bool _OnChar(wchar_t ch) { return false; }
};

struct PlaybackOverlaySceneOptions : public SceneOptionsBase
{

};

class PlaybackOverlayScene : public Scene
{
    std::unique_ptr<zcom::Panel> _playlistPanel = nullptr;
    std::unique_ptr<zcom::Panel> _readyItemPanel = nullptr;
    std::unique_ptr<zcom::Panel> _loadingItemPanel = nullptr;
    std::vector<std::unique_ptr<zcom::OverlayPlaylistItem>> _playlistItems;
    std::vector<zcom::OverlayPlaylistItem*> _readyItems;
    std::vector<zcom::OverlayPlaylistItem*> _pendingItems;
    std::vector<zcom::OverlayPlaylistItem*> _loadingItems;
    bool _addingFile = false;
    std::unique_ptr<AsyncFileDialog> _fileDialog = nullptr;
    std::unique_ptr<zcom::Button> _addFileButton = nullptr;
    std::unique_ptr<zcom::Button> _closeOverlayButton = nullptr;
    std::unique_ptr<zcom::Label> _networkStatusLabel = nullptr;
    std::unique_ptr<zcom::Button> _disconnectButton = nullptr;
    std::unique_ptr<zcom::Button> _stopServerButton = nullptr;
    std::unique_ptr<zcom::Panel> _connectedUsersPanel = nullptr;
    std::unique_ptr<zcom::TextInput> _usernameInput = nullptr;
    std::unique_ptr<zcom::Button> _usernameButton = nullptr;

    std::unique_ptr<zcom::Label> _offlineLabel = nullptr;
    std::unique_ptr<zcom::Button> _connectButton = nullptr;
    std::unique_ptr<zcom::Button> _startServerButton = nullptr;
    bool _connectPanelOpen = false;
    bool _startServerPanelOpen = false;

    std::unique_ptr<PlaybackOverlayShortcutHandler> _shortcutHandler = nullptr;

    znet::NetworkMode _networkMode = znet::NetworkMode::OFFLINE;
    std::vector<int64_t> _currentUserIds;
    std::vector<std::wstring> _currentUserNames;

public:
    PlaybackOverlayScene();

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
private:
    void _SetUpPacketReceivers(znet::NetworkMode netMode);
    void _ProcessCurrentUsers();
    void _CheckFileDialogCompletion();
    void _ManageLoadingItems();
    void _ManageReadyItems();
public:
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

    const char* GetName() const { return "playback_overlay"; }
    static const char* StaticName() { return "playback_overlay"; }

private:
    void _RearrangePlaylistPanel();
    void _RemoveDeletedItems();
    void _AddMissingItems();
    void _UpdateItemDetails();
    void _SplitItems();
    void _SortItems();
    void _RearrangeNetworkPanel();
    void _RearrangeQueuePanel();
    void _SetUpNetworkPanel();
    void _RearrangeNetworkPanel_Offline();
    void _RearrangeNetworkPanel_Server();
    void _RearrangeNetworkPanel_Client();

    // Playlist change tracking

    std::unique_ptr<EventReceiver<PlaylistChangedEvent>> _playlistChangedReceiver = nullptr;
    bool _playlistChanged = false;
    TimePoint _lastPlaylistUpdate = 0;
    Duration _playlistUpdateInterval = Duration(5, SECONDS);
    void _InvokePlaylistChange();

    // Playlist item reordering

    int64_t _heldItemId = -1;
    int _clickYPos = 0;
    int _currentMouseYPos = 0;
    bool _movedFar = false;

    void _HandlePlaylistLeftClick(zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int x, int y);
    void _HandlePlaylistLeftRelease(zcom::Base* item, int x, int y);
    void _HandlePlaylistMouseMove(zcom::Base* item, int x, int y);

    // Shorcut handling

    bool _HandleKeyDown(BYTE keyCode);
};