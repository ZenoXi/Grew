#include "App.h" // App.h must be included first
#include "PlaybackOverlayScene.h"

#include "OverlayScene.h"
#include "ConnectScene.h"
#include "StartServerScene.h"
#include "OpenPlaylistScene.h"
#include "SavePlaylistScene.h"
#include "ManagePlaylistsScene.h"

#include "Network.h"
#include "ServerManager.h"
#include "ClientManager.h"
#include "MediaReceiverDataProvider.h"
#include "MediaHostDataProvider.h"

#include "FileTypes.h"
#include "Permissions.h"

#include <filesystem>

PlaybackOverlayScene::PlaybackOverlayScene(App* app)
    : Scene(app)
{}

void PlaybackOverlayScene::_Init(const SceneOptionsBase* options)
{
    PlaybackOverlaySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PlaybackOverlaySceneOptions*>(options);
    }

    // Init event receivers
    _playlistChangedReceiver = std::make_unique<EventReceiver<PlaylistChangedEvent>>(&App::Instance()->events);
    _networkStatsEventReceiver = std::make_unique<EventReceiver<NetworkStatsEvent>>(&App::Instance()->events);
    _permissionsChangedReceiver = std::make_unique<EventReceiver<UserPermissionChangedEvent>>(&App::Instance()->events);

    // Set up shortcut handler
    _shortcutHandler = std::make_unique<PlaybackOverlayShortcutHandler>();
    _shortcutHandler->AddOnKeyDown([&](BYTE keyCode)
    {
        return _HandleKeyDown(keyCode);
    });

    // Set up file drop handler
    _playlistFileDropHandler = std::make_unique<DragDropHandler>(RECT{ 0, 0, 0, 0 });
    _playlistFileDropHandler->SetAllowedExtensions(ExtractPureExtensions(L"" EXTENSIONS_VIDEO ";" EXTENSIONS_AUDIO));
    _playlistFileDropHandler->SetAllowFolders(true);
    _playlistFileDropHandler->SetAllowMultipleFiles(true);
    _playlistFileDropHandler->SetMatchAllExtensions(false);

    zcom::PROP_Shadow shadowProps;
    shadowProps.offsetX = 0.0f;
    shadowProps.offsetY = 0.0f;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 1.0f);
    
    _sideMenuPanel = Create<zcom::Panel>();
    _sideMenuPanel->SetParentHeightPercent(1.0f);
    _sideMenuPanel->SetBaseWidth(199);
    _sideMenuPanel->SetBackgroundColor(D2D1::ColorF(0.13f, 0.13f, 0.13f));
    _sideMenuPanel->SetTabIndex(-1);
    _sideMenuPanel->SetProperty(shadowProps);
    _sideMenuPanel->SetZIndex(1);

    _playlistPanel = Create<zcom::Panel>();
    _playlistPanel->SetParentSizePercent(1.0f, 1.0f);
    _playlistPanel->SetBaseWidth(-200 /* Side panel */ + -400 /* Right side */);
    _playlistPanel->SetHorizontalOffsetPixels(200);
    _playlistPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _playlistPanel->SetProperty(shadowProps);
    _playlistPanel->SetTabIndex(-1);

    _readyItemPanel = Create<zcom::Panel>();
    _readyItemPanel->SetParentSizePercent(1.0f, 1.0f);
    _readyItemPanel->SetBaseHeight(-40);
    _readyItemPanel->SetVerticalOffsetPixels(40);
    _readyItemPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);
    _readyItemPanel->ScrollBackgroundVisible(zcom::Scrollbar::VERTICAL, true);
    _readyItemPanel->AddPostLeftPressed([=](zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int x, int y) { _HandlePlaylistLeftClick(item, targets, x, y); }, { this });
    _readyItemPanel->AddOnLeftReleased([=](zcom::Base* item, int x, int y) { _HandlePlaylistLeftRelease(item, x, y); }, { this });
    _readyItemPanel->AddPostMouseMove([=](zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int deltaX, int deltaY) { _HandlePlaylistMouseMove(item, targets, deltaX, deltaY); }, { this });
    _readyItemPanel->SetTabIndex(-1);

    _fileDropLabel = Create<zcom::Label>(L"Drop files..");
    _fileDropLabel->SetBaseSize(200, 50);
    _fileDropLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _fileDropLabel->SetFontSize(32.0f);
    _fileDropLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    _fileDropLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _fileDropLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    // Copy ready item panel layout
    _fileDropOverlay = Create<zcom::Panel>();
    _fileDropOverlay->SetParentSizePercent(1.0f, 1.0f);
    _fileDropOverlay->SetBaseHeight(-40);
    _fileDropOverlay->SetVerticalOffsetPixels(40);
    _fileDropOverlay->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));
    _fileDropOverlay->SetZIndex(2);
    _fileDropOverlay->SetVisible(false);
    _fileDropOverlay->AddItem(_fileDropLabel.get());

    _playlistLabel = Create<zcom::Label>(L"Playlist");
    _playlistLabel->SetBaseSize(100, 40);
    _playlistLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _playlistLabel->SetMargins({ 10.0f });
    _playlistLabel->SetFontSize(24.0f);

    _addFileButton = Create<zcom::Button>(L"Add files");
    _addFileButton->SetParentWidthPercent(1.0f);
    _addFileButton->SetBaseHeight(30);
    _addFileButton->SetVerticalOffsetPixels(0);
    _addFileButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _addFileButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _addFileButton->Text()->SetFontSize(16.0f);
    _addFileButton->Text()->SetMargins({ 10.0f });
    _addFileButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _addFileButton->SetOnActivated([&]()
    {
        if (_fileDialog)
            return;

        _addingFile = true;
        _fileDialog = std::make_unique<AsyncFileDialog>();
        FileDialogOptions opt;
        opt.allowedExtensions = SUPORTED_MEDIA_FILE_TYPES_WINAPI;
        _fileDialog->Open(opt);
    });

    _addFolderButton = Create<zcom::Button>(L"Add folder");
    _addFolderButton->SetParentWidthPercent(1.0f);
    _addFolderButton->SetBaseHeight(30);
    _addFolderButton->SetVerticalOffsetPixels(30);
    _addFolderButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _addFolderButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _addFolderButton->Text()->SetFontSize(16.0f);
    _addFolderButton->Text()->SetMargins({ 10.0f });
    _addFolderButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _addFolderButton->SetOnActivated([&]()
    {
        if (_fileDialog)
            return;

        _addingFolder = true;
        _fileDialog = std::make_unique<AsyncFileDialog>();
        FileDialogOptions opt;
        opt.openFolders = true;
        _fileDialog->Open(opt);
    });

    _openPlaylistButton = Create<zcom::Button>(L"Open playlist");
    _openPlaylistButton->SetParentWidthPercent(1.0f);
    _openPlaylistButton->SetBaseHeight(30);
    _openPlaylistButton->SetVerticalOffsetPixels(70);
    _openPlaylistButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _openPlaylistButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _openPlaylistButton->Text()->SetFontSize(16.0f);
    _openPlaylistButton->Text()->SetMargins({ 10.0f });
    _openPlaylistButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _openPlaylistButton->SetOnActivated([&]()
    {
        _openPlaylistSceneOpen = true;
        App::Instance()->InitScene(OpenPlaylistScene::StaticName(), nullptr);
        App::Instance()->MoveSceneToFront(OpenPlaylistScene::StaticName());
    });

    _savePlaylistButton = Create<zcom::Button>(L"Save playlist");
    _savePlaylistButton->SetParentWidthPercent(1.0f);
    _savePlaylistButton->SetBaseHeight(30);
    _savePlaylistButton->SetVerticalOffsetPixels(100);
    _savePlaylistButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _savePlaylistButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _savePlaylistButton->Text()->SetFontSize(16.0f);
    _savePlaylistButton->Text()->SetMargins({ 10.0f });
    _savePlaylistButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _savePlaylistButton->SetOnActivated([&]()
    {
        _savePlaylistSceneOpen = true;
        SavePlaylistSceneOptions opt;
        opt.openedPlaylistName = _currentPlaylistName;
        App::Instance()->InitScene(SavePlaylistScene::StaticName(), &opt);
        App::Instance()->MoveSceneToFront(SavePlaylistScene::StaticName());
    });

    _manageSavedPlaylistsButton = Create<zcom::Button>(L"Manage saved playlists");
    _manageSavedPlaylistsButton->SetParentWidthPercent(1.0f);
    _manageSavedPlaylistsButton->SetBaseHeight(30);
    _manageSavedPlaylistsButton->SetVerticalOffsetPixels(130);
    _manageSavedPlaylistsButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _manageSavedPlaylistsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _manageSavedPlaylistsButton->Text()->SetFontSize(16.0f);
    _manageSavedPlaylistsButton->Text()->SetMargins({ 10.0f });
    _manageSavedPlaylistsButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _manageSavedPlaylistsButton->SetOnActivated([&]()
    {
        _managePlaylistsSceneOpen = true;
        App::Instance()->InitScene(ManagePlaylistsScene::StaticName(), nullptr);
        App::Instance()->MoveSceneToFront(ManagePlaylistsScene::StaticName());
    });

    _closeOverlayButton = Create<zcom::Button>(L"Close overlay (Esc)");
    _closeOverlayButton->SetParentWidthPercent(1.0f);
    _closeOverlayButton->SetBaseHeight(30);
    _closeOverlayButton->SetVerticalAlignment(zcom::Alignment::END);
    _closeOverlayButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _closeOverlayButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _closeOverlayButton->Text()->SetFontSize(16.0f);
    _closeOverlayButton->Text()->SetMargins({ 10.0f });
    _closeOverlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _closeOverlayButton->SetOnActivated([&]()
    {
        App::Instance()->MoveSceneToBack(this->GetName());
    });

    _networkBannerPanel = Create<zcom::Panel>();
    _networkBannerPanel->SetBaseSize(340, 60);
    _networkBannerPanel->SetOffsetPixels(-30, 30);
    _networkBannerPanel->SetHorizontalAlignment(zcom::Alignment::END);
    _networkBannerPanel->SetBackgroundColor(D2D1::ColorF(0.13f, 0.13f, 0.13f));
    _networkBannerPanel->SetProperty(shadowProps);
    _networkBannerPanel->SetZIndex(1);

    _networkStatusLabel = Create<zcom::Label>(L"");
    _networkStatusLabel->SetBaseSize(200, 30);
    _networkStatusLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _networkStatusLabel->SetFontSize(22.0f);
    _networkStatusLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    _networkStatusLabel->SetMargins({ 10.0f });
    zcom::PROP_Shadow textShadow;
    textShadow.offsetX = 2.0f;
    textShadow.offsetY = 2.0f;
    textShadow.blurStandardDeviation = 2.0f;
    textShadow.color = D2D1::ColorF(0, 1.0f);
    _networkStatusLabel->SetProperty(textShadow);

    _closeNetworkButton = Create<zcom::Button>(L"");
    _closeNetworkButton->SetBaseSize(100, 30);
    _closeNetworkButton->SetHorizontalAlignment(zcom::Alignment::END);
    _closeNetworkButton->Text()->SetFontSize(16.0f);
    _closeNetworkButton->SetBackgroundColor(D2D1::ColorF(0.15f, 0.1f, 0.1f));
    _closeNetworkButton->SetButtonHoverColor(D2D1::ColorF(0.2f, 0.1f, 0.1f));
    _closeNetworkButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _closeNetworkButton->SetOnActivated([&]()
    {
        APP_NETWORK->CloseManager();
    });

    _toggleChatButton = Create<zcom::Button>();
    _toggleChatButton->SetBaseSize(30, 30);
    _toggleChatButton->SetOffsetPixels(0, 30);
    _toggleChatButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _toggleChatButton->SetButtonImage(ResourceManager::GetImage("chat_vdim"));
    _toggleChatButton->SetButtonHoverImage(ResourceManager::GetImage("chat_dim"));
    _toggleChatButton->SetButtonClickImage(ResourceManager::GetImage("chat_dim"));
    _toggleChatButton->SetButtonColorAll(D2D1::ColorF(0.13f, 0.13f, 0.13f));
    _toggleChatButton->SetSelectable(false);
    _toggleChatButton->SetActivation(zcom::ButtonActivation::PRESS);
    _toggleChatButton->SetOnActivated([&]()
    {
        if (_chatPanel->GetVisible())
        {
            _chatPanel->SetVisible(false);
            _toggleChatButton->SetButtonColorAll(D2D1::ColorF(0.13f, 0.13f, 0.13f));
        }
        else
        {
            _chatPanel->SetVisible(true);
            _toggleChatButton->SetButtonColorAll(D2D1::ColorF(0.1f, 0.1f, 0.1f));
        }
    });

    _toggleUserListButton = Create<zcom::Button>();
    _toggleUserListButton->SetBaseSize(30, 30);
    _toggleUserListButton->SetOffsetPixels(30, 30);
    _toggleUserListButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _toggleUserListButton->SetButtonImage(ResourceManager::GetImage("user_vdim"));
    _toggleUserListButton->SetButtonHoverImage(ResourceManager::GetImage("user_dim"));
    _toggleUserListButton->SetButtonClickImage(ResourceManager::GetImage("user_dim"));
    _toggleUserListButton->SetButtonColorAll(D2D1::ColorF(0.13f, 0.13f, 0.13f));
    _toggleUserListButton->SetSelectable(false);
    _toggleUserListButton->SetActivation(zcom::ButtonActivation::PRESS);
    _toggleUserListButton->SetOnActivated([&]()
    {
        if (_connectedUsersPanel->GetVisible())
        {
            _connectedUsersPanel->SetVisible(false);
            _toggleUserListButton->SetButtonColorAll(D2D1::ColorF(0.13f, 0.13f, 0.13f));
        }
        else
        {
            _connectedUsersPanel->SetVisible(true);
            _toggleUserListButton->SetButtonColorAll(D2D1::ColorF(0.1f, 0.1f, 0.1f));
        }
    });

    _connectedUserCountLabel = Create<zcom::Label>(L"");
    _connectedUserCountLabel->SetBaseSize(30, 12);
    _connectedUserCountLabel->SetOffsetPixels(30, 48);
    _connectedUserCountLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _connectedUserCountLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _connectedUserCountLabel->SetFontSize(10.0f);
    //_connectedUserCountLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    //_connectedUserCountLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _connectedUserCountLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));

    _usernameButton = Create<zcom::Button>(L"");
    _usernameButton->SetBaseSize(180, 30);
    _usernameButton->SetOffsetPixels(60, 30);
    _usernameButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _usernameButton->SetButtonColor(D2D1::ColorF(0.13f, 0.13f, 0.13f));
    _usernameButton->SetButtonHoverColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _usernameButton->SetButtonClickColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _usernameButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _usernameButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _usernameButton->Text()->SetFontSize(14.0f);
    _usernameButton->Text()->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _usernameButton->Text()->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));
    _usernameButton->Text()->SetMargins({ 10.0f, 0.0f, 10.0f });
    _usernameButton->Text()->SetCutoff(L"...");
    _usernameButton->SetSelectable(false);
    _usernameButton->SetActivation(zcom::ButtonActivation::PRESS);
    _usernameButton->SetOnActivated([&]()
    {
        _usernameButton->SetVisible(false);
        _usernameInput->SetVisible(true);
        auto user = _app->users.GetThisUser();
        _usernameInput->SetText(user ? user->name : L"");
        _selectUsernameInput = true;
    });
    _usernameButton->SetZIndex(1);

    _usernameInput = Create<zcom::TextInput>();
    _usernameInput->SetBaseSize(180, 30);
    _usernameInput->SetOffsetPixels(60, 30);
    _usernameInput->SetPlaceholderText(L"New username");
    _usernameInput->SetBorderVisibility(false);
    _usernameInput->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
    _usernameInput->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _usernameInput->SetVisible(false);
    _usernameInput->AddOnKeyDown([&](BYTE key)
    {
        bool quit = false;
        if (key == VK_ESCAPE)
        {
            quit = true;
        }
        else if (key == VK_RETURN)
        {
            App::Instance()->users.SetSelfUsername(_usernameInput->GetText());
            //APP_NETWORK->SetUsername(_usernameInput->GetText());
            _networkPanelChanged = true;
            quit = true;
        }

        if (quit)
        {
            _usernameButton->SetVisible(true);
            _usernameInput->SetVisible(false);
            return true;
        }
        return false;
    });

    _networkLatencyLabel = Create<zcom::Label>(L"-");
    _networkLatencyLabel->SetBaseSize(45, 30);
    _networkLatencyLabel->SetOffsetPixels(240, 30);
    _networkLatencyLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _networkLatencyLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _networkLatencyLabel->SetFontSize(14.0f);
    _networkLatencyLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _networkLatencyLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));

    _downloadSpeedImage = Create<zcom::Image>();
    _downloadSpeedImage->SetBaseSize(15, 15);
    _downloadSpeedImage->SetOffsetPixels(285, 31);
    _downloadSpeedImage->SetImage(ResourceManager::GetImage("download_color_15"));

    _downloadSpeedLabel = Create<zcom::Label>(L"0b/s");
    _downloadSpeedLabel->SetBaseSize(40, 15);
    _downloadSpeedLabel->SetOffsetPixels(300, 31);
    _downloadSpeedLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _downloadSpeedLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _downloadSpeedLabel->SetFontSize(11.0f);
    _downloadSpeedLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _downloadSpeedLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));

    _uploadSpeedImage = Create<zcom::Image>();
    _uploadSpeedImage->SetBaseSize(15, 15);
    _uploadSpeedImage->SetOffsetPixels(285, 44);
    _uploadSpeedImage->SetImage(ResourceManager::GetImage("upload_color_15"));

    _uploadSpeedLabel = Create<zcom::Label>(L"0b/s");
    _uploadSpeedLabel->SetBaseSize(40, 15);
    _uploadSpeedLabel->SetOffsetPixels(300, 44);
    _uploadSpeedLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _uploadSpeedLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _uploadSpeedLabel->SetFontSize(11.0f);
    _uploadSpeedLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _uploadSpeedLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));

    _connectedUsersPanel = Create<zcom::Panel>();
    _connectedUsersPanel->SetBaseSize(340, 200);
    _connectedUsersPanel->SetOffsetPixels(-30, 91);
    _connectedUsersPanel->SetHorizontalAlignment(zcom::Alignment::END);
    _connectedUsersPanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    _connectedUsersPanel->SetProperty(shadowProps);
    _connectedUsersPanel->SetVisible(false);

    _userContextMenu = Create<zcom::MenuPanel>();

    _chatPanel = Create<zcom::Panel>();
    _chatPanel->SetVisible(false);

    _offlineLabel = Create<zcom::Label>(L"Try watching something with others");
    _offlineLabel->SetBaseSize(260, 60);
    _offlineLabel->SetOffsetPixels(-70, -30);
    _offlineLabel->SetAlignment(zcom::Alignment::END, zcom::Alignment::CENTER);
    _offlineLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _offlineLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _offlineLabel->SetFontSize(22.0f);
    _offlineLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    _offlineLabel->SetProperty(textShadow);
    _offlineLabel->SetWordWrap(true);

    _connectButton = Create<zcom::Button>(L"Connect");
    _connectButton->SetBaseSize(80, 25);
    _connectButton->SetOffsetPixels(-210, 20);
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

    _startServerButton = Create<zcom::Button>(L"Start server");
    _startServerButton->SetBaseSize(80, 25);
    _startServerButton->SetOffsetPixels(-110, 20);
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

    // NESTING

    _sideMenuPanel->AddItem(_addFileButton.get());
    _sideMenuPanel->AddItem(_addFolderButton.get());
    _sideMenuPanel->AddItem(_openPlaylistButton.get());
    _sideMenuPanel->AddItem(_savePlaylistButton.get());
    _sideMenuPanel->AddItem(_manageSavedPlaylistsButton.get());
    _sideMenuPanel->AddItem(_closeOverlayButton.get());

    _playlistPanel->AddItem(_playlistLabel.get());
    _playlistPanel->AddItem(_readyItemPanel.get());
    _playlistPanel->AddItem(_fileDropOverlay.get());

    _networkBannerPanel->AddItem(_networkStatusLabel.get());
    _networkBannerPanel->AddItem(_closeNetworkButton.get());
    _networkBannerPanel->AddItem(_toggleChatButton.get());
    _networkBannerPanel->AddItem(_toggleUserListButton.get());
    _networkBannerPanel->AddItem(_connectedUserCountLabel.get());
    _networkBannerPanel->AddItem(_usernameButton.get());
    _networkBannerPanel->AddItem(_usernameInput.get());
    _networkBannerPanel->AddItem(_networkLatencyLabel.get());
    _networkBannerPanel->AddItem(_downloadSpeedImage.get());
    _networkBannerPanel->AddItem(_downloadSpeedLabel.get());
    _networkBannerPanel->AddItem(_uploadSpeedImage.get());
    _networkBannerPanel->AddItem(_uploadSpeedLabel.get());

    _canvas->AddComponent(_sideMenuPanel.get());
    _canvas->AddComponent(_playlistPanel.get());
    _canvas->AddComponent(_networkBannerPanel.get());
    _canvas->AddComponent(_connectedUsersPanel.get());
    _canvas->AddComponent(_offlineLabel.get());
    _canvas->AddComponent(_connectButton.get());
    _canvas->AddComponent(_startServerButton.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.75f));

    // Init network panel
    _networkPanelChanged = true;
    _RearrangeNetworkPanel();
}

void PlaybackOverlayScene::_Uninit()
{
    _playlistPanel = nullptr;
}

void PlaybackOverlayScene::_Focus()
{
    App::Instance()->keyboardManager.AddHandler(_shortcutHandler.get());
    GetKeyboardState(_shortcutHandler->KeyStates());
    _app->window.AddDragDropHandler(_playlistFileDropHandler.get());
}

void PlaybackOverlayScene::_Unfocus()
{
    App::Instance()->keyboardManager.RemoveHandler(_shortcutHandler.get());
    _app->window.RemoveDragDropHandler(_playlistFileDropHandler.get());
}

void PlaybackOverlayScene::_Update()
{
    _canvas->Update();
    //_sideMenuPanel->Update();
    //_playlistPanel->Update();
    //_connectedUsersPanel->Update();

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
    else if (_openPlaylistSceneOpen)
    {
        OpenPlaylistScene* scene = (OpenPlaylistScene*)App::Instance()->FindScene(OpenPlaylistScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Open playlist scene incorrectly marked as open" << std::endl;
            _openPlaylistSceneOpen = false;
        }
        else if (scene->CloseScene())
        {
            if (!scene->PlaylistName().empty())
                _currentPlaylistName = scene->PlaylistName();
            App::Instance()->UninitScene(OpenPlaylistScene::StaticName());
            _openPlaylistSceneOpen = false;
        }
    }
    else if (_savePlaylistSceneOpen)
    {
        SavePlaylistScene* scene = (SavePlaylistScene*)App::Instance()->FindScene(SavePlaylistScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Save playlist scene incorrectly marked as open" << std::endl;
            _savePlaylistSceneOpen = false;
        }
        else if (scene->CloseScene())
        {
            if (!scene->PlaylistName().empty())
                _currentPlaylistName = scene->PlaylistName();
            App::Instance()->UninitScene(SavePlaylistScene::StaticName());
            _savePlaylistSceneOpen = false;
        }
    }
    else if (_managePlaylistsSceneOpen)
    {
        ManagePlaylistsScene* scene = (ManagePlaylistsScene*)App::Instance()->FindScene(ManagePlaylistsScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Manage playlists scene incorrectly marked as open" << std::endl;
            _managePlaylistsSceneOpen = false;
        }
        else if (scene->CloseScene())
        {
            App::Instance()->UninitScene(ManagePlaylistsScene::StaticName());
            _managePlaylistsSceneOpen = false;
        }
    }

    if (_selectUsernameInput)
    {
        _usernameInput->OnSelected();
        _selectUsernameInput = false;
    }

    _InvokePlaylistChange();
    _InvokeNetworkPanelChange();
    _UpdateNetworkStats();
    _HandlePermissionChange();

    // Files
    _UpdateFileDropHandler();
    _CheckFileDialogCompletion();

    // Update UI items
    _ManageLoadingItems();
    _ManageFailedItems();
    _ManageReadyItems();
    _RearrangePlaylistPanel();
    _RearrangeNetworkPanel();
    _UpdatePermissions();

    _UpdateItemAppearance();
}

void PlaybackOverlayScene::_UpdateFileDropHandler()
{
    namespace fs = std::filesystem;

    // Update drop rect
    auto user = _app->users.GetThisUser();
    bool canDrop = user && user->GetPermission(PERMISSION_ADD_ITEMS);
    if (canDrop)
    {
        _playlistFileDropHandler->SetDropRect(
        {
            _readyItemPanel->GetScreenX(),
            _readyItemPanel->GetScreenY() + _app->MenuHeight(),
            _readyItemPanel->GetScreenX() + _readyItemPanel->GetWidth(),
            _readyItemPanel->GetScreenY() + _app->MenuHeight() + _readyItemPanel->GetHeight()
        });
    }
    else
    {
        _playlistFileDropHandler->SetDropRect({ 0, 0, -1, -1 });
    }

    // Set overlay visibility 
    _fileDropOverlay->SetVisible(canDrop && _playlistFileDropHandler->Dragging());

    // Get drop result
    auto result = _playlistFileDropHandler->GetDropResult();
    if (result)
    {
        auto allowedExtensions = ExtractPureExtensions(L"" EXTENSIONS_VIDEO ";" EXTENSIONS_AUDIO);
        bool someSkipped = false;
        for (auto& path : result->paths)
        {
            if (fs::is_regular_file(path))
            {
                auto ext = fs::path(path).extension().wstring().substr(1);
                if (std::find(allowedExtensions.begin(), allowedExtensions.end(), ext) == allowedExtensions.end())
                {
                    someSkipped = true;
                    continue;
                }

                auto item = std::make_unique<PlaylistItem>(path);
                _app->playlist.Request_AddItem(std::move(item));
            }
            else if (fs::is_directory(path))
            {
                if (!_OpenAllFilesInFolder(path))
                {
                    someSkipped = true;
                }
            }
            else
            {
                someSkipped = true;
            }
        }

        // Show notification
        if (someSkipped)
        {
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(3, SECONDS);
            ninfo.title = L"File drop:";
            ninfo.text = L"Some files were not opened";
            ninfo.borderColor = D2D1::ColorF(0.65f, 0.4f, 0.0f);
            _app->Overlay()->ShowNotification(ninfo);
        }
    }
}

void PlaybackOverlayScene::_CheckFileDialogCompletion()
{
    if (!_fileDialog)
        return;
    if (!_fileDialog->Done())
        return;

    if (_addingFile)
    {
        auto paths = _fileDialog->ParsedResult();
        if (!paths.empty())
        {
            // Open all selected files
            for (int i = 0; i < paths.size(); i++)
            {
                auto item = std::make_unique<PlaylistItem>(paths[i]);
                App::Instance()->playlist.Request_AddItem(std::move(item));
            }
        }
        _addingFile = false;
        _fileDialog.reset();
    }
    else if (_addingFolder)
    {
        auto path = _fileDialog->Result();
        if (!path.empty())
        {
            _OpenAllFilesInFolder(path);
        }
        _addingFolder = false;
        _fileDialog.reset();
    }
    else if (_openingPlaylist)
    {
        auto path = _fileDialog->Result();
        if (!path.empty())
        {
            _OpenPlaylist(path);
        }
        _openingPlaylist = false;
        _fileDialog.reset();
    }
}

bool PlaybackOverlayScene::_OpenAllFilesInFolder(std::wstring path)
{
    namespace fs = std::filesystem;

    auto allowedExtensions = ExtractPureExtensions(L"" EXTENSIONS_VIDEO ";" EXTENSIONS_AUDIO);
    bool someSkipped = false;
    try
    {
        for (const auto& entry : fs::recursive_directory_iterator(path))
        {
            if (entry.is_regular_file())
            {
                auto path = entry.path();
                std::wstring ext = L"";
                if (path.has_extension())
                    ext = path.extension().wstring().substr(1);
                if (std::find(allowedExtensions.begin(), allowedExtensions.end(), ext) == allowedExtensions.end())
                {
                    someSkipped = true;
                    continue;
                }

                // Add file to playlist
                auto item = std::make_unique<PlaylistItem>(entry.path());
                App::Instance()->playlist.Request_AddItem(std::move(item));
            }
        }
    }
    catch (std::exception ex)
    {
        std::cout << ex.what();
        someSkipped = true;
    }
    return !someSkipped;
}

void PlaybackOverlayScene::_OpenPlaylist(std::wstring path)
{
    std::ifstream fin(wstring_to_string(path));
    if (!fin)
    {
        zcom::NotificationInfo ninfo;
        ninfo.duration = Duration(3, SECONDS);
        ninfo.title = L"Open playlist:";
        ninfo.text = L"The playlist file could not be opened";
        ninfo.borderColor = D2D1::ColorF(0.65f, 0.0f, 0.0f);
        _app->Overlay()->ShowNotification(ninfo);
        return;
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

void PlaybackOverlayScene::_ManageFailedItems()
{
    for (int i = 0; i < _failedItems.size(); i++)
    {
        // Delete
        if (_failedItems[i]->Delete())
        {
            App::Instance()->playlist.Request_DeleteItem(_failedItems[i]->GetItemId());
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

void PlaybackOverlayScene::_Resize(int width, int height)
{

}

void PlaybackOverlayScene::_RearrangePlaylistPanel()
{
    if (!_playlistChanged)
        return;
    _playlistChanged = false;
    _itemAppearanceChanged = true;

    //auto moveColor = D2D1::ColorF(D2D1::ColorF::DodgerBlue);
    //moveColor.r *= 0.4f;
    //moveColor.g *= 0.4f;
    //moveColor.b *= 0.4f;
    //auto playColor = D2D1::ColorF(D2D1::ColorF::Orange);
    //playColor.r *= 0.3f;
    //playColor.g *= 0.3f;
    //playColor.b *= 0.3f;

    _readyItemPanel->DeferLayoutUpdates();
    _readyItemPanel->ClearItems();
    _readyItems.clear();
    _loadingItems.clear();
    _failedItems.clear();

    _RemoveDeletedItems();
    _AddMissingItems();
    _UpdateItemDetails();
    _SplitItems();
    _SortItems();

    //const int ITEM_HEIGHT = 25;

    // Add ready items
    // Use 'while' to allow early exiting if reordering is aborted
    while (_movingItems)
    {
        // Separate selected and non-selected items while KEEPING ORDER
        std::vector<zcom::OverlayPlaylistItem*> selectedItems;
        std::vector<zcom::OverlayPlaylistItem*> remainingItems;
        int heldItemIndex = -1;
        for (auto& item : _readyItems)
        {
            if (_selectedItemIds.find(item->GetItemId()) != _selectedItemIds.end())
            {
                if (item->GetItemId() == _heldItemId)
                    heldItemIndex = selectedItems.size();
                selectedItems.push_back(item);
            }
            else
                remainingItems.push_back(item);
        }

        // If primary (the one which was clicked) moved item is gone, abort reordering
        if (heldItemIndex == -1)
        {
            _movingItems = false;
            _heldItemId = -1;
            break;
        }

        int panelHeight = _readyItems.size() * _ITEM_HEIGHT;
        // Calculate selected block placement
        int blockHeight = selectedItems.size() * _ITEM_HEIGHT;
        int blockTopPos = _currentMouseYPos - (heldItemIndex * _ITEM_HEIGHT + _heldItemClickYPos);
        if (blockTopPos + blockHeight > panelHeight)
            blockTopPos = panelHeight - blockHeight;
        if (blockTopPos < 0)
            blockTopPos = 0;
        int insertionSlot = (blockTopPos + _ITEM_HEIGHT / 2) / _ITEM_HEIGHT;

        // Place non-selected items
        for (int i = 0; i < remainingItems.size(); i++)
        {
            int yPos = i * _ITEM_HEIGHT;
            if (i >= insertionSlot)
                yPos += blockHeight;

            remainingItems[i]->SetZIndex(0);
            remainingItems[i]->SetVerticalOffsetPixels(yPos);
            remainingItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.02f * ((yPos / _ITEM_HEIGHT) % 2 == 0)));
        }

        // Place selected Items
        for (int i = 0; i < selectedItems.size(); i++)
        {
            int yPos = i * _ITEM_HEIGHT + blockTopPos;
            selectedItems[i]->SetZIndex(1);
            selectedItems[i]->SetVerticalOffsetPixels(yPos);
        }

        break;
    }
    if (!_movingItems)
    {
        for (int i = 0; i < _readyItems.size(); i++)
        {
            _readyItems[i]->SetParentWidthPercent(1.0f);
            _readyItems[i]->SetBaseHeight(_ITEM_HEIGHT);
            _readyItems[i]->SetVerticalOffsetPixels(i * _ITEM_HEIGHT);
            _readyItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.02f * (i % 2 == 0)));
            _readyItems[i]->SetZIndex(0);
        }
    }

    // Update ready item visuals
    for (int i = 0; i < _readyItems.size(); i++)
    {
        Playlist& playlist = App::Instance()->playlist;
        bool playVisible = true;
        bool deleteVisible = true;
        bool stopVisible = false;
        std::wstring status = L"";

        // Deleting
        auto pendingDeletes = playlist.PendingItemDeletes();
        if (std::find(pendingDeletes.begin(), pendingDeletes.end(), _readyItems[i]->GetItemId()) != pendingDeletes.end())
        {
            playVisible = false;
            deleteVisible = false;
            status = L"Deleting..";
        }

        // Moving
        auto pendingMoves = playlist.PendingItemMoves();
        for (auto& move : pendingMoves)
        {
            if (std::find(move.first.begin(), move.first.end(), _readyItems[i]->GetItemId()) != move.first.end())
            {
                // Will require a different display method
                status = L"Moving..";
                break;
            }
        }

        // Starting/Playing
        if (_readyItems[i]->GetItemId() == playlist.CurrentlyPlaying())
        {
            _readyItems[i]->SetBackgroundColor(_PLAY_COLOR);
            playVisible = false;
            deleteVisible = false;
            stopVisible = true;
        }
        else if (_readyItems[i]->GetItemId() == playlist.CurrentlyStarting())
        {
            _readyItems[i]->SetBackgroundColor(_PLAY_COLOR);
            playVisible = false;
            deleteVisible = false;
            stopVisible = true;
            status = L"Starting..";
        }

        // Stopping
        if (playlist.PendingItemStop() == _readyItems[i]->GetItemId())
        {
            stopVisible = false;
            status = L"Stopping..";
        }

        // Host missing
        if (_readyItems[i]->HostMissing())
        {
            playVisible = false;
            if (_readyItems[i]->GetItemId() == playlist.CurrentlyPlaying())
                deleteVisible = false;
            if (_readyItems[i]->GetItemId() == playlist.CurrentlyStarting())
                deleteVisible = false;
            status = L"Host missing..";
        }

        // Permissions
        auto user = _app->users.GetThisUser();
        if (user && !user->GetPermission(PERMISSION_START_STOP_PLAYBACK))
        {
            playVisible = false;
            stopVisible = false;
        }
        if (user && !user->GetPermission(PERMISSION_MANAGE_ITEMS))
        {
            deleteVisible = false;
        }

        int buttons = 0;
        buttons |= playVisible ? zcom::OverlayPlaylistItem::BTN_PLAY : 0;
        buttons |= deleteVisible ? zcom::OverlayPlaylistItem::BTN_DELETE : 0;
        buttons |= stopVisible ? zcom::OverlayPlaylistItem::BTN_STOP : 0;
        _readyItems[i]->SetButtonVisibility(buttons);
        if (!status.empty())
            _readyItems[i]->SetCustomStatus(status);

        _readyItemPanel->AddItem(_readyItems[i]);
    }
    int currentHeight = _readyItems.size() * _ITEM_HEIGHT;
    // Add pending items
    for (int i = 0; i < _pendingItems.size(); i++)
    {
        _pendingItems[i]->SetParentWidthPercent(1.0f);
        _pendingItems[i]->SetBaseHeight(_ITEM_HEIGHT);
        _pendingItems[i]->SetVerticalOffsetPixels(currentHeight);
        _pendingItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.02f * ((currentHeight / _ITEM_HEIGHT) % 2 == 0)));
        _pendingItems[i]->SetButtonVisibility(0);
        _pendingItems[i]->SetCustomStatus(L"Waiting for server..");
        _readyItemPanel->AddItem(_pendingItems[i]);
        currentHeight += _ITEM_HEIGHT;
    }
    // Add loading items
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        _loadingItems[i]->SetParentWidthPercent(1.0f);
        _loadingItems[i]->SetBaseHeight(_ITEM_HEIGHT);
        _loadingItems[i]->SetVerticalOffsetPixels(currentHeight);
        _loadingItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.02f * ((currentHeight / _ITEM_HEIGHT) % 2 == 0)));
        if (_loadingItems[i]->GetItemId() == App::Instance()->playlist.CurrentlyPlaying())
        {
            _loadingItems[i]->SetBackgroundColor(_PLAY_COLOR);
            _loadingItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_STOP);
        }
        else
        {
            _loadingItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_DELETE);
        }
        _loadingItems[i]->SetCustomStatus(L"Initializing..");
        _readyItemPanel->AddItem(_loadingItems[i]);
        currentHeight += _ITEM_HEIGHT;
    }
    // Add failed items
    for (int i = 0; i < _failedItems.size(); i++)
    {
        _failedItems[i]->SetParentWidthPercent(1.0f);
        _failedItems[i]->SetBaseHeight(_ITEM_HEIGHT);
        _failedItems[i]->SetVerticalOffsetPixels(currentHeight);
        _failedItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.03 + 0.01f * ((currentHeight / _ITEM_HEIGHT) % 2 == 0)));
        _failedItems[i]->SetButtonVisibility(zcom::OverlayPlaylistItem::BTN_DELETE);
        _readyItemPanel->AddItem(_failedItems[i]);
        currentHeight += _ITEM_HEIGHT;
    }

    _readyItemPanel->ResumeLayoutUpdates();
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
            _playlistItems.push_back(Create<zcom::OverlayPlaylistItem>(item->GetItemId()));
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

    // Create set of failed playlist item ids
    std::set<int64_t> failedItemIds;
    for (auto& item : App::Instance()->playlist.FailedItems())
        failedItemIds.insert(item->GetItemId());
    // Move pending UI items to separate array
    _failedItems.clear();
    for (int i = 0; i < items.size(); i++)
    {
        if (failedItemIds.find(items[i]->GetItemId()) != failedItemIds.end())
        {
            _failedItems.push_back(items[i]);
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
    // Reorder currently moving items
    auto pendingMoves = App::Instance()->playlist.PendingItemMoves();
    for (int i = 0; i < pendingMoves.size(); i++)
    {
        // Create list of pending positions
        std::unordered_map<int64_t, int> movedItems;

        for (int j = 0; j < pendingMoves[i].first.size(); j++)
        {
            movedItems[pendingMoves[i].first[j]] = pendingMoves[i].second + j;
        }

        // Place ready items in their positions
        if (!movedItems.empty())
        {
            std::vector<zcom::OverlayPlaylistItem*> sortedItems;
            sortedItems.resize(_readyItems.size(), nullptr);
            // Place moved items first
            for (auto& movedItem : movedItems)
            {
                for (int i = 0; i < _readyItems.size(); i++)
                {
                    if (_readyItems[i]->GetItemId() == movedItem.first)
                    {
                        sortedItems[movedItem.second] = _readyItems[i];
                        _readyItems.erase(_readyItems.begin() + i);
                        break;
                    }
                }
            }
            // Place remaining items into gaps
            int currentSlot = 0;
            int currentItem = 0;
            while (currentSlot < sortedItems.size())
            {
                if (sortedItems[currentSlot] == nullptr)
                    sortedItems[currentSlot] = _readyItems[currentItem++];
                currentSlot++;
            }

            _readyItems = std::move(sortedItems);
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

void PlaybackOverlayScene::_RearrangeNetworkPanel()
{
    if (!_networkPanelChanged)
        return;
    _networkPanelChanged = false;
    
    bool online = false;
    auto clientMgr = APP_NETWORK->GetManager<znet::ClientManager>();
    if (clientMgr && clientMgr->Online())
        online = true;
    auto serverMgr = APP_NETWORK->GetManager<znet::ServerManager>();
    if (serverMgr && serverMgr->InitSuccessful())
        online = true;

    if (online)
        _RearrangeNetworkPanel_Online();
    else
        _RearrangeNetworkPanel_Offline();
}

void PlaybackOverlayScene::_RearrangeNetworkPanel_Offline()
{
    _networkBannerPanel->SetVisible(false);
    _connectedUsersPanel->SetVisible(false);
    _offlineLabel->SetVisible(true);
    _connectButton->SetVisible(true);
    _startServerButton->SetVisible(true);

    _connectedUsersPanel->ClearItems();
}

void PlaybackOverlayScene::_RearrangeNetworkPanel_Online()
{
    znet::NetworkMode netMode = znet::NetworkMode::OFFLINE;
    if (APP_NETWORK->GetManager<znet::ClientManager>())
        netMode = znet::NetworkMode::CLIENT;
    else if (APP_NETWORK->GetManager<znet::ServerManager>())
        netMode = znet::NetworkMode::SERVER;
    else
    {
        std::cout << "[ERROR] Unknown netMode\n";
        return;
    }

    _networkBannerPanel->SetVisible(true);
    if (netMode == znet::NetworkMode::CLIENT)
        _closeNetworkButton->Text()->SetText(L"Disconnect");
    else if (netMode == znet::NetworkMode::SERVER)
        _closeNetworkButton->Text()->SetText(L"Close server");
    _offlineLabel->SetVisible(false);
    _connectButton->SetVisible(false);
    _startServerButton->SetVisible(false);

    // Update network status label
    if (netMode == znet::NetworkMode::CLIENT)
        _networkStatusLabel->SetText(L"Connected");
    else if (netMode == znet::NetworkMode::SERVER)
        _networkStatusLabel->SetText(L"Server running");

    auto thisUser = App::Instance()->users.GetThisUser();// APP_NETWORK->ThisUser();
    if (!thisUser)
        return;

    // Update username label
    if (thisUser->name.empty())
        _usernameButton->Text()->SetText(L"No username set");
    else 
        _usernameButton->Text()->SetText(thisUser->name);

    // Add all users
    _connectedUsersPanel->DeferLayoutUpdates();
    _connectedUsersPanel->ClearItems();

    { // This client
        std::wstring name = L"[User " + string_to_wstring(int_to_str(thisUser->id)) + L"] " + thisUser->name;
        auto usernameLabel = Create<zcom::Label>(name);
        usernameLabel->SetBaseHeight(25);
        usernameLabel->SetParentWidthPercent(1.0f);
        usernameLabel->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
        usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        usernameLabel->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        _connectedUsersPanel->AddItem(usernameLabel.release(), true);
    }

    // Others
    auto users = App::Instance()->users.GetUsers(false);//APP_NETWORK->Users();
    for (int i = 0; i < users.size(); i++)
    {
        std::wstring name = L"[User " + string_to_wstring(int_to_str(users[i]->id)) + L"] " + users[i]->name;

        auto usernameLabel = Create<zcom::Label>(name);
        usernameLabel->SetBaseHeight(25);
        usernameLabel->SetParentWidthPercent(1.0f);
        usernameLabel->SetVerticalOffsetPixels(25 + 25 * i);
        usernameLabel->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        usernameLabel->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });

        if (thisUser->GetPermission(PERMISSION_EDIT_PERMISSIONS))
        {
            int64_t userId = users[i]->id;
            usernameLabel->AddOnRightReleased([&, userId](zcom::Base* item, int x, int y)
            {
                auto user = App::Instance()->users.GetUser(userId);
                if (!user)
                    return;

                _userContextMenu->ClearItems();
                auto permissions = user->GetPermissionNames();
                for (auto& perm : permissions)
                {
                    std::wstring permName = perm.first;
                    std::string permKey = perm.second;
                    auto permissionItem = Create<zcom::MenuItem>(permName, [&, userId, permKey](bool checked)
                    {
                        App::Instance()->users.SetPermission(userId, permKey, checked);
                    });
                    permissionItem->SetCheckable(true);
                    permissionItem->SetChecked(user->GetPermission(permKey));
                    permissionItem->SetCloseOnClick(false);
                    _userContextMenu->AddItem(std::move(permissionItem));
                }

                RECT clickPoint;
                clickPoint.left = item->GetScreenX() + x;
                clickPoint.right = item->GetScreenX() + x;
                clickPoint.top = item->GetScreenY() + y;
                clickPoint.bottom = item->GetScreenY() + y;
                App::Instance()->Overlay()->ShowMenu(_userContextMenu.get(), clickPoint);
            });
        }

        _connectedUsersPanel->AddItem(usernameLabel.release(), true);
    }
    _connectedUsersPanel->ResumeLayoutUpdates();

    // Update user count label
    std::wostringstream ss(L"");
    ss << string_to_wstring(int_to_str(users.size() + 1));// << " user";
    //if (!users.empty())
    //    ss << L's';
    _connectedUserCountLabel->SetText(ss.str());
}

void PlaybackOverlayScene::_UpdatePermissions()
{
    auto user = _app->users.GetThisUser();
    if (!user)
        return;

    bool allowItemAdd = user->GetPermission(PERMISSION_ADD_ITEMS);
    _addFileButton->SetActive(allowItemAdd);
    _addFolderButton->SetActive(allowItemAdd);
    _openPlaylistButton->SetActive(allowItemAdd);
}

void PlaybackOverlayScene::_InvokePlaylistChange()
{
    while (_playlistChangedReceiver->EventCount() > 0)
    {
        _playlistChangedReceiver->GetEvent();
        _playlistChanged = true;
        _itemAppearanceChanged = true;
        _lastPlaylistUpdate = ztime::Main();
    }

    // Update periodically to account for any bugged cases
    if (ztime::Main() - _lastPlaylistUpdate > _playlistUpdateInterval)
    {
        _playlistChanged = true;
        _itemAppearanceChanged = true;
        _lastPlaylistUpdate = ztime::Main();
    }
}

void PlaybackOverlayScene::_UpdateItemAppearance()
{
    if (!_itemAppearanceChanged)
        return;
    _itemAppearanceChanged = false;

    // Alternating backgrounds on all items
    for (auto& item : _playlistItems)
        item->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.02f * ((item->GetY() / _ITEM_HEIGHT) % 2 == 0)));

    // Played item colors
    for (auto& item : _playlistItems)
    {
        if (item->GetItemId() == _app->playlist.CurrentlyPlaying() ||
            item->GetItemId() == _app->playlist.CurrentlyPlaying())
        {
            item->SetBackgroundColor(_PLAY_COLOR);
        }
    }

    // Selected item colors and move handles
    for (auto& item : _readyItems)
    {
        item->SetMoveHandleVisibility(false);
        if (_selectedItemIds.find(item->GetItemId()) != _selectedItemIds.end())
        {
            item->SetBackgroundColor(_SELECTION_COLOR);
            item->SetMoveHandleVisibility(true);
        }
    }
}

void PlaybackOverlayScene::_HandlePlaylistLeftClick(zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int x, int y)
{
    if (targets.empty())
        return;

    // If a button was clicked, don't handle
    if (targets.front().target->GetName() == zcom::Button::Name())
        return;

    // If click was outside ready item range, don't handle (ignores pending/loading items)
    int slot = (y + _readyItemPanel->VisualScrollPosition(zcom::Scrollbar::VERTICAL)) / 25;
    if (slot >= _readyItems.size())
        return;

    // Check if move handle was clicked
    for (auto& target : targets)
    {
        if (target.target->GetName() == zcom::DotGrid::Name())
        {
            // Start move
            _movingItems = true;
        }
        else if (target.target->GetName() == zcom::OverlayPlaylistItem::Name())
        {
            _heldItemId = ((zcom::OverlayPlaylistItem*)target.target)->GetItemId();
            _heldItemClickYPos = target.y;
        }
    }
    if (_movingItems && _heldItemId != -1)
    {
        // If the moved item is not selected, unselect all other items
        // and select the moved one
        if (_selectedItemIds.find(_heldItemId) == _selectedItemIds.end())
        {
            _selectedItemIds.clear();
            _selectedItemIds.insert(_heldItemId);
        }

        // If move was started, don't do further checking
        return;
    }
    else
    {
        _movingItems = false;
        _heldItemId = -1;
    }

    // Check if item was clicked
    for (auto& target : targets)
    {
        if (target.target->GetName() == zcom::OverlayPlaylistItem::Name())
        {
            int64_t itemId = ((zcom::OverlayPlaylistItem*)target.target)->GetItemId();

            // Holding CTRL allows selecting multiple items. Otherwise, clicking on any item clears the selection.
            // Clicking an unselected item selects it, while clicking a selected item unselects it.
            // The above 2 rules work in combination.
            bool alreadySelected = _selectedItemIds.find(itemId) != _selectedItemIds.end();
            if (!_app->keyboardManager.KeyState(VK_CONTROL))
                _selectedItemIds.clear();
            if (!alreadySelected)
                _selectedItemIds.insert(itemId);
            else
                _selectedItemIds.erase(itemId);

            // Start drag-selecting if CTRL is not clicked
            if (!_app->keyboardManager.KeyState(VK_CONTROL))
            {
                _selectingItems = true;
                _selectionStartPos = y + _readyItemPanel->VisualScrollPosition(zcom::Scrollbar::VERTICAL);
            }

            _itemAppearanceChanged = true;
            //_heldItemId = ((zcom::OverlayPlaylistItem*)target.target)->GetItemId();
            //_clickYPos = y + _readyItemPanel->VisualVerticalScroll();
            //_movedFar = false;
        }
    }
}

void PlaybackOverlayScene::_HandlePlaylistLeftRelease(zcom::Base* item, int x, int y)
{
    _selectingItems = false;
    _selectionStartPos = -1;

    if (_movingItems)
    {
        std::vector<zcom::OverlayPlaylistItem*> selectedItems;
        int heldItemIndex = -1;
        for (auto& item : _readyItems)
        {
            if (_selectedItemIds.find(item->GetItemId()) != _selectedItemIds.end())
            {
                if (item->GetItemId() == _heldItemId)
                    heldItemIndex = selectedItems.size();
                selectedItems.push_back(item);
            }
        }
        if (heldItemIndex != -1)
        {
            int panelHeight = _readyItems.size() * _ITEM_HEIGHT;
            // Calculate selected block placement
            int blockHeight = _selectedItemIds.size() * _ITEM_HEIGHT;
            int blockTopPos = _currentMouseYPos - (heldItemIndex * _ITEM_HEIGHT + _heldItemClickYPos);
            if (blockTopPos + blockHeight > panelHeight)
                blockTopPos = panelHeight - blockHeight;
            if (blockTopPos < 0)
                blockTopPos = 0;
            int insertionSlot = (blockTopPos + _ITEM_HEIGHT / 2) / _ITEM_HEIGHT;

            // Send move request
            std::vector<int64_t> selectedItemIds;
            for (auto& item : selectedItems)
                selectedItemIds.push_back(item->GetItemId());
            App::Instance()->playlist.Request_MoveItems(selectedItemIds, insertionSlot);
        }
        
        _movingItems = false;
        _heldItemId = -1;
        // Set flag here, because Request_MoveItem has edge cases where it doesn't do that itself
        _playlistChanged = true;
    }
}

void PlaybackOverlayScene::_HandlePlaylistMouseMove(zcom::Base* item, std::vector<zcom::EventTargets::Params> targets, int deltaX, int deltaY)
{
    int trueY = item->GetMousePosY() + _readyItemPanel->VisualScrollPosition(zcom::Scrollbar::VERTICAL);

    // Drag-select items
    if (_selectingItems)
    {
        // If mouse is outside ready item range, don't handle (ignores pending/loading items)
        int slot = trueY / _ITEM_HEIGHT;
        if (slot >= _readyItems.size())
            return;

        int upperSlot = _selectionStartPos / _ITEM_HEIGHT;
        int lowerSlot = trueY / _ITEM_HEIGHT;
        if (lowerSlot < upperSlot)
            std::swap(upperSlot, lowerSlot);
        if (upperSlot < 0)
            upperSlot = 0;
        if (lowerSlot > _readyItems.size() - 1)
            lowerSlot = _readyItems.size() - 1;

        // Select all items inbetween start and current mouse pos
        _selectedItemIds.clear();
        for (int i = upperSlot; i <= lowerSlot; i++)
        {
            _selectedItemIds.insert(_readyItems[i]->GetItemId());
            _itemAppearanceChanged = true;
        }

        // Check if item was hovered
        // We cannot use 'targets' here because moving the mouse while
        // holding either left/right buttons constrains events to the
        // initial click target.
        //for (auto& item : _readyItems)
        //{
        //    int trueY = y + _readyItemPanel->VisualVerticalScroll();
        //    if (x >= item->GetX() && x < item->GetX() + item->GetWidth() &&
        //        trueY >= item->GetY() && trueY < item->GetY() + item->GetHeight())
        //    {
        //        _selectedItemIds.insert(item->GetItemId());
        //        _itemBackgroundsChanged = true;
        //    }
        //}

        int y = item->GetMousePosY();
        // Auto-scroll
        if (y < 0)
            _readyItemPanel->Scroll(zcom::Scrollbar::VERTICAL, _readyItemPanel->ScrollPosition(zcom::Scrollbar::VERTICAL) - (-y / 10 + 1));
        else if (y >= _readyItemPanel->GetHeight())
            _readyItemPanel->Scroll(zcom::Scrollbar::VERTICAL, _readyItemPanel->ScrollPosition(zcom::Scrollbar::VERTICAL) + ((y - _readyItemPanel->GetHeight()) / 10 + 1));

        return;
    }
    else if (_selectionStartPos != -1)
    {
        int initialSlot = _selectionStartPos / _ITEM_HEIGHT;
        int currentSlot = trueY / _ITEM_HEIGHT;
        if (initialSlot != currentSlot)
            _selectingItems = true;
    }


    if (_movingItems)
    {
        _playlistChanged = true;

        _currentMouseYPos = item->GetMousePosY() + _readyItemPanel->VisualScrollPosition(zcom::Scrollbar::VERTICAL);
        //if (abs(_clickYPos - _currentMouseYPos) > 3)
        //{
        //    _movedFar = true;
        //}

        int y = item->GetMousePosY();
        // Auto-scroll
        if (y < 0)
            _readyItemPanel->Scroll(zcom::Scrollbar::VERTICAL, _readyItemPanel->ScrollPosition(zcom::Scrollbar::VERTICAL) - (-y / 10 + 1));
        else if (y >= _readyItemPanel->GetHeight())
            _readyItemPanel->Scroll(zcom::Scrollbar::VERTICAL, _readyItemPanel->ScrollPosition(zcom::Scrollbar::VERTICAL) + ((y - _readyItemPanel->GetHeight()) / 10 + 1));
    }
}

void PlaybackOverlayScene::_InvokeNetworkPanelChange()
{
    if (_networkEventTracker.EventReceived())
    {
        _networkPanelChanged = true;
        _lastNetworkPanelUpdate = ztime::Main();
    }

    // Update periodically to account for any bugged cases
    if (ztime::Main() - _lastNetworkPanelUpdate > _networkPanelUpdateInterval)
    {
        _networkPanelChanged = true;
        _lastNetworkPanelUpdate = ztime::Main();
    }
}

std::wstring BytesToString(size_t bytes)
{
    std::wostringstream ss(L"");

    // Gigabytes
    if (bytes >= 1000000000)
    {
        size_t gbWhole = bytes / 1000000000;
        size_t gbFrac = (bytes / 100000000) % 10;
        if (gbWhole >= 100)
        {
            ss << gbWhole << L"gb";
            return ss.str();
        }
        else
        {
            ss << gbWhole << L'.' << gbFrac << L"gb";
            return ss.str();
        }
    }
    // Megabytes
    else if (bytes >= 1000000)
    {
        size_t mbWhole = bytes / 1000000;
        size_t mbFrac = (bytes / 100000) % 10;
        if (mbWhole >= 100)
        {
            ss << mbWhole << L"mb";
            return ss.str();
        }
        else
        {
            ss << mbWhole << L'.' << mbFrac << L"mb";
            return ss.str();
        }
    }
    // Kilobytes
    else if (bytes >= 1000)
    {
        size_t kbWhole = bytes / 1000;
        size_t kbFrac = (bytes / 100) % 10;
        if (kbWhole >= 100)
        {
            ss << kbWhole << L"kb";
            return ss.str();
        }
        else
        {
            ss << kbWhole << L'.' << kbFrac << L"kb";
            return ss.str();
        }
    }
    // Bytes
    else
    {
        ss << bytes << L"b";
        return ss.str();
    }
}

void PlaybackOverlayScene::_UpdateNetworkStats()
{
    if (_networkStatsEventReceiver->EventCount() > 0)
    {
        auto ev = _networkStatsEventReceiver->GetEvent();

        // Latency
        if (ev.latency != -1)
            _networkLatencyLabel->SetText(string_to_wstring(int_to_str(ev.latency / 1000) + "ms"));
        else
            _networkLatencyLabel->SetText(L"-");

        // Download
        _downloadSpeedLabel->SetText(BytesToString(ev.bytesReceived * 1000 / ev.timeInterval.GetDuration(MILLISECONDS)) + L"/s");
        // Upload
        _uploadSpeedLabel->SetText(BytesToString(ev.bytesSent * 1000 / ev.timeInterval.GetDuration(MILLISECONDS)) + L"/s");
    }
}

void PlaybackOverlayScene::_HandlePermissionChange()
{
    while (_permissionsChangedReceiver->EventCount() > 0)
    {
        _permissionsChangedReceiver->GetEvent();
        _playlistChanged = true;
        _networkPanelChanged = true;
        _lastPlaylistUpdate = ztime::Main();
        _lastNetworkPanelUpdate = ztime::Main();
    }
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
    //case VK_NUMPAD0:
        //_offlineLabel->SetVisible(!_offlineLabel->GetVisible());
        //return true;
    }
    return false;
}