#pragma once

#include "Scene.h"
#include "PlaybackScene.h"
#include "PacketSubscriber.h"
#include "PlaylistEvents.h"
#include "UserEvents.h"

#include "Label.h"
#include "Image.h"
#include "TextInput.h"
#include "OverlayPlaylistItem.h"

#include "FileDialog.h"

#include <set>

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
    std::unique_ptr<zcom::Panel> _sideMenuPanel = nullptr;
    std::unique_ptr<zcom::Panel> _playlistPanel = nullptr;
    std::unique_ptr<zcom::Panel> _readyItemPanel = nullptr;
    std::unique_ptr<zcom::Label> _playlistLabel = nullptr;
    std::vector<std::unique_ptr<zcom::OverlayPlaylistItem>> _playlistItems;
    std::vector<zcom::OverlayPlaylistItem*> _readyItems;
    std::vector<zcom::OverlayPlaylistItem*> _pendingItems;
    std::vector<zcom::OverlayPlaylistItem*> _loadingItems;
    std::vector<zcom::OverlayPlaylistItem*> _failedItems;

    std::unique_ptr<zcom::Button> _addFileButton = nullptr;
    std::unique_ptr<zcom::Button> _addFolderButton = nullptr;
    bool _addingFile = false;
    bool _addingFolder = false;
    std::unique_ptr<AsyncFileDialog> _fileDialog = nullptr;

    std::unique_ptr<zcom::Button> _openPlaylistButton = nullptr;
    std::unique_ptr<zcom::Button> _savePlaylistButton = nullptr;
    std::unique_ptr<zcom::Button> _manageSavedPlaylistsButton = nullptr;
    bool _openingPlaylist = false;
    bool _openPlaylistSceneOpen = false;
    bool _savePlaylistSceneOpen = false;
    bool _managePlaylistsSceneOpen = false;
    std::wstring _currentPlaylistName = L"";

    std::unique_ptr<zcom::Button> _closeOverlayButton = nullptr;

    std::unique_ptr<zcom::Panel> _networkBannerPanel = nullptr;
    std::unique_ptr<zcom::Label> _networkStatusLabel = nullptr;
    std::unique_ptr<zcom::Button> _closeNetworkButton = nullptr;
    std::unique_ptr<zcom::Button> _toggleChatButton = nullptr;
    std::unique_ptr<zcom::Button> _usernameButton = nullptr;
    std::unique_ptr<zcom::TextInput> _usernameInput = nullptr;
    bool _selectUsernameInput = false;
    std::unique_ptr<zcom::Button> _toggleUserListButton = nullptr;
    std::unique_ptr<zcom::Label> _connectedUserCountLabel = nullptr;
    std::unique_ptr<zcom::Label> _networkLatencyLabel = nullptr;
    std::unique_ptr<zcom::Image> _downloadSpeedImage = nullptr;
    std::unique_ptr<zcom::Label> _downloadSpeedLabel = nullptr;
    std::unique_ptr<zcom::Image> _uploadSpeedImage = nullptr;
    std::unique_ptr<zcom::Label> _uploadSpeedLabel = nullptr;
    std::unique_ptr<zcom::Panel> _connectedUsersPanel = nullptr;
    std::unique_ptr<zcom::MenuPanel> _userContextMenu = nullptr;

    std::unique_ptr<zcom::Panel> _chatPanel = nullptr;

    std::unique_ptr<zcom::Label> _offlineLabel = nullptr;
    std::unique_ptr<zcom::Button> _connectButton = nullptr;
    std::unique_ptr<zcom::Button> _startServerButton = nullptr;
    bool _connectPanelOpen = false;
    bool _startServerPanelOpen = false;

    std::unique_ptr<PlaybackOverlayShortcutHandler> _shortcutHandler = nullptr;
    std::unique_ptr<DragDropHandler> _playlistFileDropHandler = nullptr;
    std::unique_ptr<zcom::Panel> _fileDropOverlay = nullptr;
    std::unique_ptr<zcom::Label> _fileDropLabel = nullptr;

public:
    PlaybackOverlayScene(App* app);

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
private:
    void _UpdateFileDropHandler();
    void _CheckFileDialogCompletion();
    // Returns true if no files were skipped
    bool _OpenAllFilesInFolder(std::wstring path);
    void _OpenPlaylist(std::wstring path);
    void _ManageLoadingItems();
    void _ManageFailedItems();
    void _ManageReadyItems();
public:
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
    void _RearrangeNetworkPanel_Offline();
    void _RearrangeNetworkPanel_Online();
    void _UpdatePermissions();

    int _ITEM_HEIGHT = 25;

    // Colors

    // Dodger blue at 40% brightness
    D2D1_COLOR_F _SELECTION_COLOR = D2D1::ColorF(0.1176f * 0.4f, 0.5647f * 0.4f, 1.0f * 0.4f);
    // Orange at 30% brightness
    D2D1_COLOR_F _PLAY_COLOR = D2D1::ColorF(1.0f * 0.3f, 0.6471f * 0.3f, 0.0f * 0.3f);
    bool _itemAppearanceChanged = false;
    void _UpdateItemAppearance();

    // Playlist change tracking

    std::unique_ptr<EventReceiver<PlaylistChangedEvent>> _playlistChangedReceiver = nullptr;
    bool _playlistChanged = false;
    TimePoint _lastPlaylistUpdate = 0;
    Duration _playlistUpdateInterval = Duration(5, SECONDS);
    void _InvokePlaylistChange();

    // Playlist item selection

    std::set<int64_t> _selectedItemIds;
    bool _selectingItems = false;
    int _selectionStartPos = -1;

    // Playlist item reordering

    bool _movingItems = false;
    int64_t _heldItemId = -1;
    int _heldItemClickYPos = 0;
    int _currentMouseYPos = 0;
    // When reordering, all of the selected items are bunched together.
    // This variable holds holds the mouse click position relative to
    // the top of the first selected item (after bunching them together).
    int _selectionClickYPos = 0;

    //int64_t _heldItemId = -1;
    int _clickYPos = 0;
    //int _currentMouseYPos = 0;
    bool _movedFar = false;

    // Playlist mouse events

    void _HandlePlaylistLeftClick(zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int x, int y);
    void _HandlePlaylistLeftRelease(zcom::Base* item, int x, int y);
    void _HandlePlaylistMouseMove(zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int deltaX, int deltaY);

    // Network panel change tracking

    class NetworkEventTracker
    {
        std::unique_ptr<EventHandler<ServerStartEvent>> _serverStartEvent;
        std::unique_ptr<EventHandler<ServerStopEvent>> _serverStopEvent;
        std::unique_ptr<EventHandler<ConnectionStartEvent>> _connectionStartEvent;
        std::unique_ptr<EventHandler<ConnectionSuccessEvent>> _connectionSuccessEvent;
        std::unique_ptr<EventHandler<ConnectionFailEvent>> _connectionFailEvent;
        std::unique_ptr<EventHandler<DisconnectEvent>> _disconnectEvent;
        std::unique_ptr<EventHandler<ConnectionLostEvent>> _connectionLostEvent;
        std::unique_ptr<EventHandler<ConnectionClosedEvent>> _connectionClosedEvent;
        std::unique_ptr<EventHandler<NetworkModeChangedEvent>> _networkModeChangedEvent;
        //std::unique_ptr<EventHandler<UserConnectedEvent>> _userConnectedEvent;
        //std::unique_ptr<EventHandler<UserDisconnectedEvent>> _userDisconnectedEvent;
        //std::unique_ptr<EventHandler<UserNameChangedEvent>> _userNameChangedEvent;
        std::unique_ptr<EventHandler<UserAddedEvent>> _userAddedEvent;
        std::unique_ptr<EventHandler<UserRemovedEvent>> _userRemovedEvent;
        std::unique_ptr<EventHandler<UserNameChangedEvent>> _userNameChangedEvent;

        std::mutex _m_event;
        bool _eventReceived = false;

    public:
        NetworkEventTracker()
        {
            _serverStartEvent = std::make_unique<EventHandler<ServerStartEvent>>(&App::Instance()->events, [&](ServerStartEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _serverStopEvent = std::make_unique<EventHandler<ServerStopEvent>>(&App::Instance()->events, [&](ServerStopEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _connectionStartEvent = std::make_unique<EventHandler<ConnectionStartEvent>>(&App::Instance()->events, [&](ConnectionStartEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _connectionSuccessEvent = std::make_unique<EventHandler<ConnectionSuccessEvent>>(&App::Instance()->events, [&](ConnectionSuccessEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _connectionFailEvent = std::make_unique<EventHandler<ConnectionFailEvent>>(&App::Instance()->events, [&](ConnectionFailEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _disconnectEvent = std::make_unique<EventHandler<DisconnectEvent>>(&App::Instance()->events, [&](DisconnectEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _connectionLostEvent = std::make_unique<EventHandler<ConnectionLostEvent>>(&App::Instance()->events, [&](ConnectionLostEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _connectionClosedEvent = std::make_unique<EventHandler<ConnectionClosedEvent>>(&App::Instance()->events, [&](ConnectionClosedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _networkModeChangedEvent = std::make_unique<EventHandler<NetworkModeChangedEvent>>(&App::Instance()->events, [&](NetworkModeChangedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            //_userConnectedEvent = std::make_unique<EventHandler<UserConnectedEvent>>(&App::Instance()->events, [&](UserConnectedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            //_userDisconnectedEvent = std::make_unique<EventHandler<UserDisconnectedEvent>>(&App::Instance()->events, [&](UserDisconnectedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _userAddedEvent = std::make_unique<EventHandler<UserAddedEvent>>(&App::Instance()->events, [&](UserAddedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _userRemovedEvent = std::make_unique<EventHandler<UserRemovedEvent>>(&App::Instance()->events, [&](UserRemovedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
            _userNameChangedEvent = std::make_unique<EventHandler<UserNameChangedEvent>>(&App::Instance()->events, [&](UserNameChangedEvent) { _m_event.lock(); _eventReceived = true; _m_event.unlock(); });
        }

        bool EventReceived()
        {
            std::lock_guard<std::mutex> lock(_m_event);
            bool value = _eventReceived;
            _eventReceived = false;
            return value;
        }
    };
    NetworkEventTracker _networkEventTracker;
    bool _networkPanelChanged = false;
    TimePoint _lastNetworkPanelUpdate = 0;
    Duration _networkPanelUpdateInterval = Duration(5, SECONDS);
    void _InvokeNetworkPanelChange();

    std::unique_ptr<EventReceiver<NetworkStatsEvent>> _networkStatsEventReceiver = nullptr;
    void _UpdateNetworkStats();

    // Permissions change handling

    std::unique_ptr<EventReceiver<UserPermissionChangedEvent>> _permissionsChangedReceiver = nullptr;
    void _HandlePermissionChange();

    // Shorcut handling

    bool _HandleKeyDown(BYTE keyCode);
};