#include "App.h" // App.h must be included first
#include "PlaybackOverlayScene.h"

#include "ConnectScene.h"
#include "StartServerScene.h"

#include "NetworkInterfaceNew.h"
#include "MediaReceiverDataProvider.h"
#include "MediaHostDataProvider.h"

PlaybackOverlayScene::PlaybackOverlayScene()
{
}

void PlaybackOverlayScene::_Init(const SceneOptionsBase* options)
{
    PlaybackOverlaySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PlaybackOverlaySceneOptions*>(options);
    }

    // Set up shortcut handler
    _shortcutHandler = std::make_unique<PlaybackOverlayShortcutHandler>();
    _shortcutHandler->AddOnKeyDown([&](BYTE keyCode)
    {
        return _HandleKeyDown(keyCode);
    });
    
    _playlistPanel = std::make_unique<zcom::Panel>();
    _playlistPanel->SetParentHeightPercent(1.0f);
    _playlistPanel->SetBaseSize(600, -110);
    _playlistPanel->SetOffsetPixels(40, 40);
    _playlistPanel->SetBorderVisibility(true);
    _playlistPanel->SetBorderColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    _playlistPanel->SetCornerRounding(3.0f);
    _playlistPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    zcom::PROP_Shadow shadowProps;
    shadowProps.offsetX = 5.0f;
    shadowProps.offsetY = 5.0f;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 1.0f);
    _playlistPanel->SetProperty(shadowProps);

    _readyItemPanel = std::make_unique<zcom::Panel>();
    _readyItemPanel->SetParentSizePercent(1.0f, 1.0f);
    _readyItemPanel->SetBaseHeight(-140);
    _readyItemPanel->SetBorderVisibility(true);
    _readyItemPanel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _readyItemPanel->VerticalScrollable(true);

    _loadingItemPanel = std::make_unique<zcom::Panel>();
    _loadingItemPanel->SetParentWidthPercent(1.0f);
    _loadingItemPanel->SetBaseHeight(140);
    _loadingItemPanel->SetVerticalAlignment(zcom::Alignment::END);
    _loadingItemPanel->VerticalScrollable(true);

    _playlistPanel->AddItem(_readyItemPanel.get());
    _playlistPanel->AddItem(_loadingItemPanel.get());

    //_mediaQueueItem = new zcom::MediaQueueItem(L"E:\\aots4e5.mkv");
    //_mediaQueueItem->SetBaseSize(200, 35);
    //_mediaQueueItem->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));

    _addFileButton = std::make_unique<zcom::Button>(L"Add file");
    _addFileButton->SetBaseSize(100, 25);
    _addFileButton->SetOffsetPixels(40, -40);
    _addFileButton->SetVerticalAlignment(zcom::Alignment::END);
    _addFileButton->SetBorderVisibility(true);
    _addFileButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _addFileButton->SetOnActivated([&]()
    {
        if (_addingFile)
            return;

        _addingFile = true;
        _fileDialog = std::make_unique<AsyncFileDialog>();
        _fileDialog->Open();
    });

    _closeOverlayButton = std::make_unique<zcom::Button>(L"Close overlay");
    _closeOverlayButton->SetBaseSize(100, 25);
    _closeOverlayButton->SetOffsetPixels(-40, 40);
    _closeOverlayButton->SetHorizontalAlignment(zcom::Alignment::END);
    _closeOverlayButton->SetBorderVisibility(true);
    _closeOverlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _closeOverlayButton->SetOnActivated([&]()
    {
        App::Instance()->MoveSceneToBack(this->GetName());
    });

    _networkStatusLabel = std::make_unique<zcom::Label>(L"");
    _networkStatusLabel->SetBaseSize(200, 25);
    _networkStatusLabel->SetOffsetPixels(-140, 80);
    _networkStatusLabel->SetHorizontalAlignment(zcom::Alignment::END);
    _networkStatusLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _networkStatusLabel->SetFontSize(18.0f);
    _networkStatusLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    zcom::PROP_Shadow textShadow;
    textShadow.offsetX = 2.0f;
    textShadow.offsetY = 2.0f;
    textShadow.blurStandardDeviation = 2.0f;
    textShadow.color = D2D1::ColorF(0, 1.0f);
    _networkStatusLabel->SetProperty(textShadow);

    _disconnectButton = std::make_unique<zcom::Button>(L"Disconnect");
    _disconnectButton->SetBaseSize(100, 25);
    _disconnectButton->SetOffsetPixels(-40, 80);
    _disconnectButton->SetHorizontalAlignment(zcom::Alignment::END);
    _disconnectButton->SetBorderVisibility(true);
    _disconnectButton->SetBorderColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
    _disconnectButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _disconnectButton->SetOnActivated([&]()
    {
        znet::NetworkInterface::Instance()->Disconnect();
    });

    _stopServerButton = std::make_unique<zcom::Button>(L"Close server");
    _stopServerButton->SetBaseSize(100, 25);
    _stopServerButton->SetOffsetPixels(-40, 80);
    _stopServerButton->SetHorizontalAlignment(zcom::Alignment::END);
    _stopServerButton->SetBorderVisibility(true);
    _stopServerButton->SetBorderColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
    _stopServerButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _stopServerButton->SetOnActivated([&]()
    {
        znet::NetworkInterface::Instance()->StopServer();
    });

    _connectedUsersPanel = std::make_unique<zcom::Panel>();
    _connectedUsersPanel->SetBaseSize(300, 200);
    _connectedUsersPanel->SetOffsetPixels(-40, 120);
    _connectedUsersPanel->SetHorizontalAlignment(zcom::Alignment::END);
    _connectedUsersPanel->SetBorderVisibility(true);
    _connectedUsersPanel->SetBorderColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    _connectedUsersPanel->SetCornerRounding(3.0f);
    _connectedUsersPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _connectedUsersPanel->SetProperty(shadowProps);

    _usernameInput = std::make_unique<zcom::TextInput>();
    _usernameInput->SetBaseSize(200, 25);
    _usernameInput->SetOffsetPixels(-140, 330);
    _usernameInput->SetPlaceholderText(L"New username");
    _usernameInput->SetHorizontalAlignment(zcom::Alignment::END);

    _usernameButton = std::make_unique<zcom::Button>(L"Change");
    _usernameButton->SetBaseSize(80, 25);
    _usernameButton->SetOffsetPixels(-40, 330);
    _usernameButton->SetHorizontalAlignment(zcom::Alignment::END);
    _usernameButton->SetBorderVisibility(true);
    _usernameButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _usernameButton->SetOnActivated([&]()
    {
        znet::NetworkInterface::Instance()->SetUsername(_usernameInput->GetText());
        _usernameInput->SetText(L"");
    });

    _offlineLabel = std::make_unique<zcom::Label>(L"Try watching something with others");
    _offlineLabel->SetBaseSize(260, 60);
    _offlineLabel->SetOffsetPixels(-100, -30);
    _offlineLabel->SetAlignment(zcom::Alignment::END, zcom::Alignment::CENTER);
    _offlineLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _offlineLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _offlineLabel->SetFontSize(22.0f);
    _offlineLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    _offlineLabel->SetProperty(textShadow);
    _offlineLabel->SetWordWrap(true);

    _connectButton = std::make_unique<zcom::Button>(L"Connect");
    _connectButton->SetBaseSize(80, 25);
    _connectButton->SetOffsetPixels(-240, 20);
    _connectButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::CENTER);
    _connectButton->Text()->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    _connectButton->Text()->SetFontStretch(DWRITE_FONT_STRETCH_CONDENSED);
    _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
    _connectButton->SetCornerRounding(5.0f);
    _connectButton->SetProperty(textShadow);
    _connectButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _connectButton->SetOnActivated([&]()
    {
        _connectPanelOpen = true;
        App::Instance()->InitScene(ConnectScene::StaticName(), nullptr);
        App::Instance()->MoveSceneToFront(ConnectScene::StaticName());
    });

    _startServerButton = std::make_unique<zcom::Button>(L"Start server");
    _startServerButton->SetBaseSize(80, 25);
    _startServerButton->SetOffsetPixels(-140, 20);
    _startServerButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::CENTER);
    _startServerButton->Text()->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    _startServerButton->Text()->SetFontStretch(DWRITE_FONT_STRETCH_CONDENSED);
    _startServerButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
    _startServerButton->SetCornerRounding(5.0f);
    _startServerButton->SetProperty(textShadow);
    _startServerButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _startServerButton->SetOnActivated([&]()
    {
        _startServerPanelOpen = true;
        App::Instance()->InitScene(StartServerScene::StaticName(), nullptr);
        App::Instance()->MoveSceneToFront(StartServerScene::StaticName());
    });


    _canvas->AddComponent(_playlistPanel.get());
    _canvas->AddComponent(_addFileButton.get());
    _canvas->AddComponent(_closeOverlayButton.get());
    _canvas->AddComponent(_networkStatusLabel.get());
    _canvas->AddComponent(_disconnectButton.get());
    _canvas->AddComponent(_stopServerButton.get());
    _canvas->AddComponent(_connectedUsersPanel.get());
    _canvas->AddComponent(_usernameInput.get());
    _canvas->AddComponent(_usernameButton.get());
    _canvas->AddComponent(_offlineLabel.get());
    _canvas->AddComponent(_connectButton.get());
    _canvas->AddComponent(_startServerButton.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.75f));

    // Init network panel
    _networkMode = znet::NetworkInterface::Instance()->Mode();
    _SetUpNetworkPanel();
}

void PlaybackOverlayScene::_Uninit()
{
    _playlistPanel = nullptr;
}

void PlaybackOverlayScene::_Focus()
{
    App::Instance()->keyboardManager.AddHandler(_shortcutHandler.get());
    GetKeyboardState(_shortcutHandler->KeyStates());
}

void PlaybackOverlayScene::_Unfocus()
{
    App::Instance()->keyboardManager.RemoveHandler(_shortcutHandler.get());
}

void PlaybackOverlayScene::_Update()
{
    _playlistPanel->Update();
    _connectedUsersPanel->Update();

    if (_connectPanelOpen)
    {
        ConnectScene* scene = (ConnectScene*)App::Instance()->FindScene(ConnectScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Connect panel incorrectly marked as open" << std::endl;
            _connectPanelOpen = false;
        }
        else if (scene->CloseScene())
        {
            App::Instance()->UninitScene(ConnectScene::StaticName());
            _connectPanelOpen = false;
        }
    }
    else if (_startServerPanelOpen)
    {
        StartServerScene* scene = (StartServerScene*)App::Instance()->FindScene(StartServerScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Start server panel incorrectly marked as open" << std::endl;
            _startServerPanelOpen = false;
        }
        else if (scene->CloseScene())
        {
            App::Instance()->UninitScene(StartServerScene::StaticName());
            _startServerPanelOpen = false;
        }
    }
    else
    {
        znet::NetworkMode netMode = znet::NetworkInterface::Instance()->Mode();
        if (netMode != _networkMode)
        {
            _networkMode = netMode;
            _SetUpNetworkPanel();
        }
    }

    // Process connected user changes
    _ProcessCurrentUsers();

    // File dialog
    _CheckFileDialogCompletion();

    // Update UI items
    _ManageLoadingItems();
    _ManageReadyItems();
    _RearrangePlaylistPanel();
    _RearrangeNetworkPanel();
}

void PlaybackOverlayScene::_ProcessCurrentUsers()
{
    if (_networkMode == znet::NetworkMode::OFFLINE)
    {
        if (!_currentUserIds.empty())
            _currentUserIds.clear();
        if (!_currentUserNames.empty())
            _currentUserNames.clear();
        return;
    }

    // Create an updated list of connected users, new users, and gone users
    auto users = znet::NetworkInterface::Instance()->Users();
    std::vector<int64_t> newIdList;
    std::vector<std::wstring> newNameList;
    for (int i = 0; i < users.size(); i++)
    {
        newIdList.push_back(users[i].id);
        newNameList.push_back(users[i].name);
    }
    std::vector<int64_t> newUsers;
    for (int i = 0; i < newIdList.size(); i++)
    {
        int index = -1;
        for (int j = 0; j < _currentUserIds.size(); j++)
        {
            if (_currentUserIds[j] == newIdList[i])
            {
                index = j;
                break;
            }
        }

        if (index == -1)
        {
            newUsers.push_back(newIdList[i]);
        }
        else
        {
            // Update user names
            _currentUserNames[index] = newNameList[i];
        }
    }
    std::vector<int64_t> goneUsers;
    for (int i = 0; i < _currentUserIds.size(); i++)
    {
        if (std::find(newIdList.begin(), newIdList.end(), _currentUserIds[i]) == newIdList.end())
        {
            goneUsers.push_back(_currentUserIds[i]);
        }
    }

    if (newUsers.empty() && goneUsers.empty())
    {
        return;
    }
    _currentUserIds = std::move(newIdList);
    _currentUserNames = std::move(newNameList);
}

void PlaybackOverlayScene::_CheckFileDialogCompletion()
{
    if (_addingFile)
    {
        if (_fileDialog->Done())
        {
            if (_fileDialog->Result() != L"")
            {
                auto newItem = std::make_unique<PlaylistItem>(_fileDialog->Result());
                App::Instance()->playlist.Request_AddItem(std::move(newItem));
            }
            _addingFile = false;
            _fileDialog.reset();
        }
    }
}

void PlaybackOverlayScene::_ManageLoadingItems()
{
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        // Delete
        if (_loadingItems[i]->Delete())
        {
            App::Instance()->playlist.Request_DeleteItem(_loadingItems[i]->GetItemId());
            _playlistChanged = true;
        }
    }
}

void PlaybackOverlayScene::_ManageReadyItems()
{
    for (int i = 0; i < _readyItems.size(); i++)
    {
        // Play
        if (_readyItems[i]->Play())
        {
            App::Instance()->playlist.Request_PlayItem(_readyItems[i]->GetItemId());
            _playlistChanged = true;
        }
        // Stop
        else if (_readyItems[i]->Stop())
        {
            App::Instance()->playlist.Request_StopItem(_readyItems[i]->GetItemId());
            _playlistChanged = true;
        }
        // Delete
        else if (_readyItems[i]->Delete())
        {
            App::Instance()->playlist.Request_DeleteItem(_readyItems[i]->GetItemId());
            _playlistChanged = true;
        }
    }
}

ID2D1Bitmap1* PlaybackOverlayScene::_Draw(Graphics g)
{
    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void PlaybackOverlayScene::_Resize(int width, int height)
{

}

void PlaybackOverlayScene::_RearrangePlaylistPanel()
{
    // Uncomment when playlist event handling is added
    //if (!_playlistChanged)
    //    return;
    _playlistChanged = false;
    _readyItemPanel->ClearItems();
    _loadingItemPanel->ClearItems();
    _readyItems.clear();
    _loadingItems.clear();

    _RemoveDeletedItems();
    _AddMissingItems();
    _UpdateItemDetails();
    _SplitItems();
    _SortItems();

    // Add ready items
    for (int i = 0; i < _readyItems.size(); i++)
    {
        _readyItems[i]->SetParentWidthPercent(1.0f);
        _readyItems[i]->SetBaseHeight(25);
        _readyItems[i]->SetVerticalOffsetPixels(25 * i);
        _readyItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.03f * (i % 2)));
        _readyItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_PLAY | zcom::OverlayPlaylistItem::BTN_DELETE);

        // Starting/Playing
        if (_readyItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyPlaying())
        {
            _readyItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
            _readyItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_STOP);
        }
        else if (_readyItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyStarting())
        {
            _readyItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
            _readyItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_STOP);
            _readyItems[i]->SetCustomStatus(L"Starting..");
        }

        // Host missing
        if (_readyItems[i]->HostMissing())
        {
            _readyItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_DELETE);
            if (_readyItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyPlaying())
                _readyItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_STOP);
            if (_readyItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyStarting())
                _readyItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_STOP);
            _readyItems[i]->SetCustomStatus(L"Host missing..");
        }
        _readyItemPanel->AddItem(_readyItems[i]);
    }
    // Add pending items
    for (int i = 0; i < _pendingItems.size(); i++)
    {
        _pendingItems[i]->SetParentWidthPercent(1.0f);
        _pendingItems[i]->SetBaseHeight(25);
        _pendingItems[i]->SetVerticalOffsetPixels(25 * i);
        _pendingItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        _pendingItems[i]->SetButtonVisibility(0);
        _pendingItems[i]->SetCustomStatus(L"Waiting for server..");
        _loadingItemPanel->AddItem(_pendingItems[i]);
    }
    // Add loading items
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        _loadingItems[i]->SetParentWidthPercent(1.0f);
        _loadingItems[i]->SetBaseHeight(25);
        _loadingItems[i]->SetVerticalOffsetPixels(25 * (i + _pendingItems.size()));
        _loadingItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * ((i + _pendingItems.size()) % 2)));
        if (_loadingItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyPlaying())
        {
            _loadingItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
            _loadingItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_STOP);
        }
        else
        {
            _loadingItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_DELETE);
        }
        _loadingItems[i]->SetCustomStatus(L"Initializing..");
        _loadingItemPanel->AddItem(_loadingItems[i]);
    }

    _playlistPanel->Resize();
}

void PlaybackOverlayScene::_RemoveDeletedItems()
{
    // Create set of playlist item ids
    std::set<int64_t> itemIds;
    for (auto& item : App::Instance()->playlist.AllItems())
        itemIds.insert(item->GetItemId());

    // Remove mismatches
    auto it = std::remove_if(
        _playlistItems.begin(),
        _playlistItems.end(),
        [&](const std::unique_ptr<zcom::OverlayPlaylistItem>& item)
        {
            return itemIds.find(item->GetItemId()) == itemIds.end();
        }
    );
    _playlistItems.erase(it, _playlistItems.end());
}

void PlaybackOverlayScene::_AddMissingItems()
{
    // Create set of UI item ids
    std::set<int64_t> itemIds;
    for (auto& item : _playlistItems)
        itemIds.insert(item->GetItemId());

    // Add missing items
    for (auto& item : App::Instance()->playlist.AllItems())
        if (itemIds.find(item->GetItemId()) == itemIds.end())
            _playlistItems.push_back(std::make_unique<zcom::OverlayPlaylistItem>(item->GetItemId()));
}

void PlaybackOverlayScene::_UpdateItemDetails()
{
    auto items = App::Instance()->playlist.AllItems();
    for (int i = 0; i < _playlistItems.size(); i++)
    {
        auto it = std::find_if(
            items.begin(),
            items.end(),
            [&](PlaylistItem* item) { return item->GetItemId() == _playlistItems[i]->GetItemId(); }
        );
        if (it != items.end())
        {
            _playlistItems[i]->SetFilename((*it)->GetFilename());
            _playlistItems[i]->SetDuration((*it)->GetDuration());
            _playlistItems[i]->SetCustomStatus((*it)->GetCustomStatus());
            _playlistItems[i]->HostMissing((*it)->GetUserId() == MISSING_HOST_ID);
        }
    }
}

void PlaybackOverlayScene::_SplitItems()
{
    // Create array containing all UI items
    std::vector<zcom::OverlayPlaylistItem*> items;
    for (auto& item : _playlistItems)
        items.push_back(item.get());

    // Create set of loading playlist item ids
    std::set<int64_t> loadingItemIds;
    for (auto& item : App::Instance()->playlist.LoadingItems())
        loadingItemIds.insert(item->GetItemId());
    // Move loading UI items to separate array
    _loadingItems.clear();
    for (int i = 0; i < items.size(); i++)
    {
        if (loadingItemIds.find(items[i]->GetItemId()) != loadingItemIds.end())
        {
            _loadingItems.push_back(items[i]);
            items.erase(items.begin() + i);
            i--;
        }
    }

    // Create set of pending playlist item ids
    std::set<int64_t> pendingItemIds;
    for (auto& item : App::Instance()->playlist.PendingItems())
        pendingItemIds.insert(item->GetItemId());
    // Move pending UI items to separate array
    _pendingItems.clear();
    for (int i = 0; i < items.size(); i++)
    {
        if (pendingItemIds.find(items[i]->GetItemId()) != pendingItemIds.end())
        {
            _pendingItems.push_back(items[i]);
            items.erase(items.begin() + i);
            i--;
        }
    }

    // Assign remaining items to ready array
    _readyItems = items;
}

void PlaybackOverlayScene::_SortItems()
{
    // Ready items
    auto readyPlaylistItems = App::Instance()->playlist.ReadyItems();
    if (_readyItems.size() != readyPlaylistItems.size())
        std::cout << "[ERROR] UI ITEM AND PLAYLIST ITEM MISMATCH (READY ITEMS)\n";
    for (int i = 0; i < readyPlaylistItems.size(); i++)
    {
        int64_t itemId = readyPlaylistItems[i]->GetItemId();
        // Find UI item with matching id and bring it to i'th index
        for (int j = i; j < _readyItems.size(); j++)
        {
            if (_readyItems[j]->GetItemId() == itemId)
            {
                if (j != i)
                    std::swap(_readyItems[i], _readyItems[j]);
            }
        }
    }

    // Pending items
    auto pendingPlaylistItems = App::Instance()->playlist.PendingItems();
    if (_pendingItems.size() != pendingPlaylistItems.size())
        std::cout << "[ERROR] UI ITEM AND PLAYLIST ITEM MISMATCH (PENDING ITEMS)\n";
    for (int i = 0; i < pendingPlaylistItems.size(); i++)
    {
        int64_t itemId = pendingPlaylistItems[i]->GetItemId();
        // Find UI item with matching id and bring it to i'th index
        for (int j = i; j < _pendingItems.size(); j++)
        {
            if (_pendingItems[j]->GetItemId() == itemId)
            {
                if (j != i)
                    std::swap(_pendingItems[i], _pendingItems[j]);
            }
        }
    }

    // Ready items
    auto loadingPlaylistItems = App::Instance()->playlist.LoadingItems();
    if (_loadingItems.size() != loadingPlaylistItems.size())
        std::cout << "[ERROR] UI ITEM AND PLAYLIST ITEM MISMATCH (LOADING ITEMS)\n";
    for (int i = 0; i < loadingPlaylistItems.size(); i++)
    {
        int64_t itemId = loadingPlaylistItems[i]->GetItemId();
        // Find UI item with matching id and bring it to i'th index
        for (int j = i; j < _loadingItems.size(); j++)
        {
            if (_loadingItems[j]->GetItemId() == itemId)
            {
                if (j != i)
                    std::swap(_loadingItems[i], _loadingItems[j]);
            }
        }
    }
}

void PlaybackOverlayScene::_RearrangeQueuePanel()
{
    //_playlistPanel->ClearItems();
    //for (int i = 0; i < _readyItems.size(); i++)
    //{
    //    _playlistPanel->AddItem(_readyItems[i].get());
    //    _readyItems[i]->SetVerticalOffsetPixels(25 * i);
    //    _readyItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
    //    if (_readyItems[i]->GetMediaId() == _currentlyPlaying)
    //    {
    //        _readyItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
    //        _readyItems[i]->SetButtonVisibility(zcom::MediaQueueItem::BTN_STOP);
    //        //_readyItems[i]->SetNowPlaying(true);
    //    }
    //    else
    //    {
    //        _readyItems[i]->SetButtonVisibility(zcom::MediaQueueItem::BTN_PLAY | zcom::MediaQueueItem::BTN_DELETE);
    //        //_readyItems[i]->SetNowPlaying(false);
    //    }

    //}
    //for (int i = 0; i < _loadingItems.size(); i++)
    //{
    //    _playlistPanel->AddItem(_loadingItems[i].get());
    //    _loadingItems[i]->SetVerticalOffsetPixels(25 * (i + _readyItems.size()));
    //}
    //_playlistPanel->Resize();
}

void PlaybackOverlayScene::_SetUpNetworkPanel()
{
    if (_networkMode == znet::NetworkMode::OFFLINE)
    {
        _networkStatusLabel->SetVisible(false);
        _disconnectButton->SetVisible(false);
        _stopServerButton->SetVisible(false);
        _connectedUsersPanel->SetVisible(false);
        _usernameInput->SetVisible(false);
        _usernameButton->SetVisible(false);
        _offlineLabel->SetVisible(true);
        _connectButton->SetVisible(true);
        _startServerButton->SetVisible(true);
    }
    else if (_networkMode == znet::NetworkMode::SERVER)
    {
        _networkStatusLabel->SetVisible(true);
        _disconnectButton->SetVisible(false);
        _stopServerButton->SetVisible(true);
        _connectedUsersPanel->SetVisible(true);
        _usernameInput->SetVisible(true);
        _usernameButton->SetVisible(true);
        _offlineLabel->SetVisible(false);
        _connectButton->SetVisible(false);
        _startServerButton->SetVisible(false);
    }
    else if (_networkMode == znet::NetworkMode::CLIENT)
    {
        _networkStatusLabel->SetVisible(true);
        _disconnectButton->SetVisible(true);
        _stopServerButton->SetVisible(false);
        _connectedUsersPanel->SetVisible(true);
        _usernameInput->SetVisible(true);
        _usernameButton->SetVisible(true);
        _offlineLabel->SetVisible(false);
        _connectButton->SetVisible(false);
        _startServerButton->SetVisible(false);
    }
}

void PlaybackOverlayScene::_RearrangeNetworkPanel()
{
    if (!_focused)
        return;

    if (_networkMode == znet::NetworkMode::OFFLINE)
        _RearrangeNetworkPanel_Offline();
    else if (_networkMode == znet::NetworkMode::SERVER)
        _RearrangeNetworkPanel_Server();
    else if (_networkMode == znet::NetworkMode::CLIENT)
        _RearrangeNetworkPanel_Client();
}

void PlaybackOverlayScene::_RearrangeNetworkPanel_Offline()
{

}

void PlaybackOverlayScene::_RearrangeNetworkPanel_Server()
{
    // Update network label
    std::string statusString = znet::NetworkInterface::Instance()->StatusString();
    _networkStatusLabel->SetText(string_to_wstring(statusString));

    // Add all users
    _connectedUsersPanel->ClearItems();

    { // This client
        auto user = znet::NetworkInterface::Instance()->ThisUser();
        std::wstring name = L"[User " + string_to_wstring(int_to_str(user.id)) + L"] " + user.name;
        zcom::Label* usernameLabel = new zcom::Label(name);
        usernameLabel->SetBaseHeight(25);
        usernameLabel->SetParentWidthPercent(1.0f);
        usernameLabel->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
        usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        usernameLabel->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        _connectedUsersPanel->AddItem(usernameLabel, true);
    }

    // Others
    for (int i = 0; i < _currentUserIds.size(); i++)
    {
        std::wstring name = L"[User " + string_to_wstring(int_to_str(_currentUserIds[i])) + L"] " + _currentUserNames[i];

        zcom::Label* usernameLabel = new zcom::Label(name);
        usernameLabel->SetBaseHeight(25);
        usernameLabel->SetParentWidthPercent(1.0f);
        usernameLabel->SetVerticalOffsetPixels(25 + 25 * i);
        usernameLabel->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        usernameLabel->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        _connectedUsersPanel->AddItem(usernameLabel, true);
    }
    _connectedUsersPanel->Resize();
}

void PlaybackOverlayScene::_RearrangeNetworkPanel_Client()
{
    // Update network label
    std::string statusString = znet::NetworkInterface::Instance()->StatusString();
    _networkStatusLabel->SetText(string_to_wstring(statusString));

    // Add all users
    _connectedUsersPanel->ClearItems();

    { // This client
        auto user = znet::NetworkInterface::Instance()->ThisUser();
        std::wstring name = L"[User " + string_to_wstring(int_to_str(user.id)) + L"] " + user.name;
        zcom::Label* usernameLabel = new zcom::Label(name);
        usernameLabel->SetBaseHeight(25);
        usernameLabel->SetParentWidthPercent(1.0f);
        usernameLabel->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
        usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        usernameLabel->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        _connectedUsersPanel->AddItem(usernameLabel, true);
    }

    // Others
    for (int i = 0; i < _currentUserIds.size(); i++)
    {
        std::wstring name = L"[User " + string_to_wstring(int_to_str(_currentUserIds[i])) + L"] " + _currentUserNames[i];

        zcom::Label* usernameLabel = new zcom::Label(name);
        usernameLabel->SetBaseHeight(25);
        usernameLabel->SetParentWidthPercent(1.0f);
        usernameLabel->SetVerticalOffsetPixels(25 + 25 * i);
        usernameLabel->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        usernameLabel->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        _connectedUsersPanel->AddItem(usernameLabel, true);
    }
    _connectedUsersPanel->Resize();
}

bool PlaybackOverlayScene::_HandleKeyDown(BYTE keyCode)
{
    switch (keyCode)
    {
    case VK_ESCAPE:
    {
        App::Instance()->MoveSceneToBack(this->GetName());
        return true;
    }
    }
    return false;
}