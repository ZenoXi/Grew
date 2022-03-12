#pragma once

#include "Scene.h"
#include "PlaybackScene.h"
#include "PacketSubscriber.h"

#include "Label.h"
#include "TextInput.h"
#include "MediaQueueItem.h"

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
    PlaybackScene* _playbackScene = nullptr;
    // Media id of currently playing item
    int32_t _currentlyPlaying = -1;
    // Set to media id when a playback request is processed
    // >-1 until PLAYBACK_START_READY packet from the host is received
    int32_t _loadingPlayback = -1;
    bool _autoplay = true;
    bool _waiting = false;

    std::unique_ptr<zcom::Panel> _playbackQueuePanel = nullptr;
    std::vector<std::unique_ptr<zcom::MediaQueueItem>> _readyItems;
    std::vector<std::unique_ptr<zcom::MediaQueueItem>> _loadingItems;
    bool _addingFile = false;
    AsyncFileDialog* _fileDialog = nullptr;
    std::unique_ptr<zcom::Button> _addFileButton = nullptr;
    std::unique_ptr<zcom::Button> _closeOverlayButton = nullptr;
    std::unique_ptr<zcom::Label> _networkStatusLabel = nullptr;
    std::unique_ptr<zcom::Panel> _connectedUsersPanel = nullptr;
    std::unique_ptr<zcom::TextInput> _usernameInput = nullptr;
    std::unique_ptr<zcom::Button> _usernameButton = nullptr;
    std::unique_ptr<zcom::Button> _disconnectButton = nullptr;


    std::unique_ptr<PlaybackOverlayShortcutHandler> _shortcutHandler = nullptr;

    znet::NetworkMode _networkMode = znet::NetworkMode::OFFLINE;
    std::vector<int64_t> _currentUserIds;
    std::vector<std::wstring> _currentUserNames;
    std::unique_ptr<znet::PacketReceiver> _queueAddRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _queueAddReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _queueRemoveRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _queueRemoveReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartOrderReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartResponseReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStartReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStopRequestReceiver = nullptr;
    std::unique_ptr<znet::PacketReceiver> _playbackStopReceiver = nullptr;

    int32_t _MEDIA_ID_COUNTER = 0;
    int32_t _GenerateMediaId() { return _MEDIA_ID_COUNTER++; }

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
    void _CheckForItemAddRequest();
    void _CheckForItemAdd();
    void _CheckForItemRemoveRequest();
    void _CheckForItemRemove();
    void _CheckForStartRequest();
    void _CheckForStartOrder();
    void _CheckForStartResponse();
    void _CheckForStart();
    void _CheckForStopRequest();
    void _CheckForStop();
    bool _ManageLoadingItems();
    bool _ManageReadyItems();
    void _PlayNextItem();
public:
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

    const char* GetName() const { return "playback_overlay"; }
    static const char* StaticName() { return "playback_overlay"; }

public:
    void AddItem(std::wstring filepath);
    // Shows a placeholder playback scene until an available (loaded) item appears
    void WaitForLoad(bool focus);
    void SetAutoplay(bool autoplay);
    bool GetAutoplay() const;

private:
    void _RearrangeQueuePanel();
    void _RearrangeNetworkPanel();

private:
    bool _HandleKeyDown(BYTE keyCode);
};