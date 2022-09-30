#include "App.h" // App.h must be included first
#include "EntryScene.h"

#include "ResourceManager.h"
#include "Options.h"
#include "OptionNames.h"
#include "LastIpOptionAdapter.h"
#include "Functions.h"
#include "ConnectScene.h"
#include "StartServerScene.h"
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "Network.h"
#include "FileTypes.h"

EntryScene::EntryScene(App* app)
    : Scene(app)
{}

void EntryScene::_Init(const SceneOptionsBase* options)
{
    EntrySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const EntrySceneOptions*>(options);
    }

    zcom::PROP_Shadow mainPanelShadowProps;
    mainPanelShadowProps.blurStandardDeviation = 5.0f;
    mainPanelShadowProps.color = D2D1::ColorF(0, 0.75f);

    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetParentHeightPercent(1.0f);
    _mainPanel->SetBaseWidth(500);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _mainPanel->SetProperty(mainPanelShadowProps);

    zcom::PROP_Shadow titleShadowProps;
    titleShadowProps.blurStandardDeviation = 2.0f;
    titleShadowProps.color = D2D1::ColorF(0);
    titleShadowProps.offsetX = 2.0f;
    titleShadowProps.offsetY = 2.0f;

    _logoImage = Create<zcom::Image>(ResourceManager::GetImage("grew_logo_full_2192x756"));
    _logoImage->SetBaseSize(300, 100);
    _logoImage->SetOffsetPixels(100, 50);
    _logoImage->SetPlacement(zcom::ImagePlacement::FIT);
    _logoImage->SetProperty(titleShadowProps);

    _titleLabel = Create<zcom::Label>(L"The best way to share and watch media with friends across the globe");
    _titleLabel->SetBaseSize(300, 60);
    _titleLabel->SetOffsetPixels(100, 150);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _titleLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _titleLabel->SetFontSize(16.0f);
    _titleLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _titleLabel->SetWordWrap(true);
    _titleLabel->SetProperty(titleShadowProps);

    _skipHomePageButton = Create<zcom::Button>(L"Skip this page, and don't show it again");
    _skipHomePageButton->SetBaseSize(300, 25);
    _skipHomePageButton->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::END);
    _skipHomePageButton->SetVerticalOffsetPixels(-20);
    _skipHomePageButton->Text()->SetFontColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
    _skipHomePageButton->Text()->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _skipHomePageButton->SetHoverText(L"Start the application directly in the playlist\nThis can be changed at any time in the settings");
    _skipHomePageButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _skipHomePageButton->SetActivation(zcom::ButtonActivation::PRESS);
    _skipHomePageButton->SetOnActivated([&]()
    {
        // Set option
        Options::Instance()->SetValue(OPTIONS_START_IN_PLAYLIST, L"true");

        // Close scene
        App::Instance()->UninitScene(GetName());
        App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
    });
    _skipHomePageButton->AddOnMouseEnter([&](zcom::Base*)
    {
        _skipHomePageButton->Text()->SetUnderline({ 0, (UINT32)_skipHomePageButton->Text()->GetText().length() });
    });
    _skipHomePageButton->AddOnMouseLeave([&](zcom::Base*)
    {
        _skipHomePageButton->Text()->SetUnderline({ 0, 0 });
    });
    // If skip home page option is already set, don't show the prompt
    if (Options::Instance()->GetValue(OPTIONS_START_IN_PLAYLIST) == L"true")
        _skipHomePageButton->SetVisible(false);

    auto connectImage = ResourceManager::GetImage("connect_40x40");
    auto shareImage = ResourceManager::GetImage("share_40x40");
    auto fileImage = ResourceManager::GetImage("file_40x40");
    auto playlistImage = ResourceManager::GetImage("playlist_40x40");

    _connectButton = Create<zcom::Button>(L"Connect to server");
    _connectButton->SetBaseSize(300, 40);
    _connectButton->SetOffsetPixels(100, 250);
    _connectButton->SetButtonImageAll(connectImage);
    _connectButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::FILL);
    _connectButton->ButtonImage()->SetTargetRect({ 5.0f, 0.0f, 45.0f, 40.0f });
    _connectButton->UseImageParamsForAll(_connectButton->ButtonImage());
    _connectButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _connectButton->SetButtonHoverColor(D2D1::ColorF(0, 0.25f));
    _connectButton->SetButtonClickColor(D2D1::ColorF(0, 0.5f));
    _connectButton->Text()->SetFontSize(20.0f);
    _connectButton->Text()->SetMargins({ 50.0f });
    _connectButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _connectButton->SetOnActivated([&]() { OnConnectSelected(); });
    _connectButton->AddOnMouseEnter([&](zcom::Base*)
    {
        _connectMarker->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
        _connectInfoLabel->SetVisible(true);
    });
    _connectButton->AddOnMouseLeave([&](zcom::Base*)
    {
        _connectMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        _connectInfoLabel->SetVisible(false);
    });
    _connectButton->SetTabIndex(1);

    _connectMarker = Create<zcom::EmptyPanel>();
    _connectMarker->SetBaseSize(3, 40);
    _connectMarker->SetOffsetPixels(97, 250);
    _connectMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _shareButton = Create<zcom::Button>(L"Create server");
    _shareButton->SetBaseSize(300, 40);
    _shareButton->SetOffsetPixels(100, 300);
    _shareButton->SetButtonImageAll(shareImage);
    _shareButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::FILL);
    _shareButton->ButtonImage()->SetTargetRect({ 5.0f, 0.0f, 45.0f, 40.0f });
    _shareButton->UseImageParamsForAll(_shareButton->ButtonImage());
    _shareButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _shareButton->SetButtonHoverColor(D2D1::ColorF(0, 0.25f));
    _shareButton->SetButtonClickColor(D2D1::ColorF(0, 0.5f));
    _shareButton->Text()->SetFontSize(20.0f);
    _shareButton->Text()->SetMargins({ 50.0f });
    _shareButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _shareButton->SetOnActivated([&]() { OnShareSelected(); });
    _shareButton->AddOnMouseEnter([&](zcom::Base*)
    {
        _shareMarker->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
        _shareInfoLabel->SetVisible(true);
    });
    _shareButton->AddOnMouseLeave([&](zcom::Base*)
    {
        _shareMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        _shareInfoLabel->SetVisible(false);
    });
    _shareButton->SetTabIndex(1);

    _shareMarker = Create<zcom::EmptyPanel>();
    _shareMarker->SetBaseSize(3, 40);
    _shareMarker->SetOffsetPixels(97, 300);
    _shareMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _fileButton = Create<zcom::Button>(L"Open file");
    _fileButton->SetBaseSize(300, 40);
    _fileButton->SetOffsetPixels(100, 350);
    _fileButton->SetButtonImageAll(fileImage);
    _fileButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::FILL);
    _fileButton->ButtonImage()->SetTargetRect({ 5.0f, 0.0f, 45.0f, 40.0f });
    _fileButton->UseImageParamsForAll(_fileButton->ButtonImage());
    _fileButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _fileButton->SetButtonHoverColor(D2D1::ColorF(0, 0.25f));
    _fileButton->SetButtonClickColor(D2D1::ColorF(0, 0.5f));
    _fileButton->Text()->SetFontSize(20.0f);
    _fileButton->Text()->SetMargins({ 50.0f });
    _fileButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _fileButton->SetOnActivated([&]() { OnFileSelected(); });
    _fileButton->AddOnMouseEnter([&](zcom::Base*)
    {
        _fileMarker->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
        _fileInfoLabel->SetVisible(true);
    });
    _fileButton->AddOnMouseLeave([&](zcom::Base*)
    {
        _fileMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        _fileInfoLabel->SetVisible(false);
    });
    _fileButton->SetTabIndex(2);

    _fileMarker = Create<zcom::EmptyPanel>();
    _fileMarker->SetBaseSize(3, 40);
    _fileMarker->SetOffsetPixels(97, 350);
    _fileMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _playlistButton = Create<zcom::Button>(L"Go to playlist");
    _playlistButton->SetBaseSize(300, 40);
    _playlistButton->SetOffsetPixels(100, 400);
    _playlistButton->SetButtonImageAll(playlistImage);
    _playlistButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::FILL);
    _playlistButton->ButtonImage()->SetTargetRect({ 5.0f, 0.0f, 45.0f, 40.0f });
    _playlistButton->UseImageParamsForAll(_playlistButton->ButtonImage());
    _playlistButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _playlistButton->SetButtonHoverColor(D2D1::ColorF(0, 0.25f));
    _playlistButton->SetButtonClickColor(D2D1::ColorF(0, 0.5f));
    _playlistButton->Text()->SetFontSize(20.0f);
    _playlistButton->Text()->SetMargins({ 50.0f });
    _playlistButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _playlistButton->SetOnActivated([&]() { OnPlaylistSelected(); });
    _playlistButton->AddOnMouseEnter([&](zcom::Base*)
    {
        _playlistMarker->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
        _playlistInfoLabel->SetVisible(true);
    });
    _playlistButton->AddOnMouseLeave([&](zcom::Base*)
    {
        _playlistMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        _playlistInfoLabel->SetVisible(false);
    });
    _playlistButton->SetTabIndex(3);

    _playlistMarker = Create<zcom::EmptyPanel>();
    _playlistMarker->SetBaseSize(3, 40);
    _playlistMarker->SetOffsetPixels(97, 400);
    _playlistMarker->SetBackgroundColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _mainPanel->AddItem(_logoImage.get());
    _mainPanel->AddItem(_titleLabel.get());
    _mainPanel->AddItem(_skipHomePageButton.get());
    _mainPanel->AddItem(_connectButton.get());
    _mainPanel->AddItem(_shareButton.get());
    _mainPanel->AddItem(_playlistButton.get());
    _mainPanel->AddItem(_fileButton.get());
    _mainPanel->AddItem(_connectMarker.get());
    _mainPanel->AddItem(_shareMarker.get());
    _mainPanel->AddItem(_fileMarker.get());
    _mainPanel->AddItem(_playlistMarker.get());

    // Side panel

    _sidePanel = Create<zcom::Panel>();
    _sidePanel->SetParentSizePercent(1.0f, 1.0f);
    _sidePanel->SetBaseWidth(-500);
    _sidePanel->SetHorizontalAlignment(zcom::Alignment::END);
    _sidePanel->SetTabIndex(-1);

    _creditsLabel = Create<zcom::Label>(L"v0.0.0 \x00A9 Zenox");
    _creditsLabel->SetBaseSize(200, 30);
    _creditsLabel->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _creditsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::TRAILING);
    _creditsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _creditsLabel->SetMargins({ 0.0f, 0.0f, 10.0f });
    _creditsLabel->SetFontColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));

    _connectInfoLabel = Create<zcom::Label>(L"Connect to an existing server on the internet or the local network");
    _connectInfoLabel->SetBaseSize(300, 300);
    _connectInfoLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _connectInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _connectInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _connectInfoLabel->SetFontSize(16.0f);
    _connectInfoLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _connectInfoLabel->SetWordWrap(true);
    _connectInfoLabel->SetVisible(false);

    _shareInfoLabel = Create<zcom::Label>(L"Create a server that others can connect to\n\n"
        "If you want users across the internet to be able to connect, you might need port forwarding");
    _shareInfoLabel->SetBaseSize(300, 300);
    _shareInfoLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _shareInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _shareInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _shareInfoLabel->SetFontSize(16.0f);
    _shareInfoLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _shareInfoLabel->SetWordWrap(true);
    _shareInfoLabel->SetVisible(false);

    _fileInfoLabel = Create<zcom::Label>(L"Open local file or files for playback");
    _fileInfoLabel->SetBaseSize(300, 300);
    _fileInfoLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _fileInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _fileInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _fileInfoLabel->SetFontSize(16.0f);
    _fileInfoLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _fileInfoLabel->SetWordWrap(true);
    _fileInfoLabel->SetVisible(false);

    _playlistInfoLabel = Create<zcom::Label>(L"Close this page and go to the playlist");
    _playlistInfoLabel->SetBaseSize(300, 300);
    _playlistInfoLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _playlistInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _playlistInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _playlistInfoLabel->SetFontSize(16.0f);
    _playlistInfoLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _playlistInfoLabel->SetWordWrap(true);
    _playlistInfoLabel->SetVisible(false);

    _sidePanel->AddItem(_creditsLabel.get());
    _sidePanel->AddItem(_connectInfoLabel.get());
    _sidePanel->AddItem(_shareInfoLabel.get());
    _sidePanel->AddItem(_fileInfoLabel.get());
    _sidePanel->AddItem(_playlistInfoLabel.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->AddComponent(_sidePanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
}

void EntryScene::_Uninit()
{
    _canvas->ClearComponents();
    _mainPanel->ClearItems();
    _sidePanel->ClearItems();

    _mainPanel = nullptr;
    _logoImage = nullptr;
    _titleLabel = nullptr;
    _skipHomePageButton = nullptr;

    _connectButton = nullptr;
    _shareButton = nullptr;
    _fileButton = nullptr;
    _playlistButton = nullptr;
    _connectMarker = nullptr;
    _shareMarker = nullptr;
    _fileMarker = nullptr;
    _playlistMarker = nullptr;

    _sidePanel = nullptr;
    _creditsLabel = nullptr;

    _connectInfoLabel = nullptr;
    _shareInfoLabel = nullptr;
    _fileInfoLabel = nullptr;
    _playlistInfoLabel = nullptr;

    _popoutPlaybackView = nullptr;
}

void EntryScene::_Focus()
{

}

void EntryScene::_Unfocus()
{

}

void EntryScene::_Update()
{
    _canvas->Update();

    if (_connectPanelOpen)
    {
        ConnectScene* scene = (ConnectScene*)App::Instance()->FindScene(ConnectScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Connect panel incorrectly marked as open" << std::endl;
            _connectPanelOpen = false;
        }
        else
        {
            if (scene->CloseScene())
            {
                _connectPanelOpen = false;
                if (scene->ConnectionSuccessful())
                {
                    App::Instance()->UninitScene(ConnectScene::StaticName());
                    App::Instance()->UninitScene(GetName());
                    return;
                }
                else
                {
                    App::Instance()->UninitScene(ConnectScene::StaticName());
                }
            }
        }
    }

    if (_startServerPanelOpen)
    {
        StartServerScene* scene = (StartServerScene*)App::Instance()->FindScene(StartServerScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Start server panel incorrectly marked as open" << std::endl;
            _startServerPanelOpen = false;
        }
        else
        {
            if (scene->CloseScene())
            {
                _startServerPanelOpen = false;
                if (scene->StartSuccessful())
                {
                    App::Instance()->UninitScene(StartServerScene::StaticName());
                    App::Instance()->UninitScene(GetName());
                    return;
                }
                else
                {
                    App::Instance()->UninitScene(StartServerScene::StaticName());
                }
            }
        }
    }

    if (_fileLoading)
    {
        if (_fileDialog.Done())
        {
            _fileLoading = false;

            auto paths = _fileDialog.ParsedResult();
            if (!paths.empty())
            {
                // Open all selected files
                for (int i = 0; i < paths.size(); i++)
                {
                    auto item = std::make_unique<PlaylistItem>(paths[i]);
                    int64_t itemId = item->GetItemId();
                    if (i == 0)
                    {
                        // Play first item
                        item->StartInitializing();
                        App::Instance()->playlist.Request_AddItem(std::move(item));
                        App::Instance()->playlist.Request_PlayItem(itemId);
                    }
                    else
                    {
                        App::Instance()->playlist.Request_AddItem(std::move(item));
                    }
                }

                App::Instance()->UninitScene(GetName());
                App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
            }
        }
    }

    // Check for playback
    if (!_app->playback.Initializing())
    {
        if (!_popoutPlaybackView)
        {
            _popoutPlaybackView = Create<zcom::PlaybackView>();
            _popoutPlaybackView->SetBaseSize(320, 180);
            _popoutPlaybackView->SetOffsetPixels(-30, -30);
            _popoutPlaybackView->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
            _popoutPlaybackView->SetBackgroundColor(D2D1::ColorF(0));
            zcom::PROP_Shadow popoutShadow;
            popoutShadow.blurStandardDeviation = 5.0f;
            popoutShadow.color = D2D1::ColorF(0, 0.75f);
            popoutShadow.offsetX = 2.0f;
            popoutShadow.offsetY = 2.0f;
            _popoutPlaybackView->SetProperty(popoutShadow);
            _popoutPlaybackView->SetZIndex(1);
            _popoutPlaybackView->SetDefaultCursor(CursorIcon::HAND);
            _popoutPlaybackView->AddOnLeftPressed([&](zcom::Base*, int, int)
            {
                OnPlaylistSelected();
            });
            _canvas->AddComponent(_popoutPlaybackView.get());
        }
    }
    else
    {
        if (_popoutPlaybackView)
        {
            _canvas->RemoveComponent(_popoutPlaybackView.get());
            _popoutPlaybackView = nullptr;
        }
    }
}

void EntryScene::_Resize(int width, int height)
{
    
}

void EntryScene::OnConnectSelected()
{
    App::Instance()->InitScene(ConnectScene::StaticName(), nullptr);
    App::Instance()->MoveSceneToFront(ConnectScene::StaticName());
    _connectPanelOpen = true;
    return;
}

void EntryScene::OnShareSelected()
{
    App::Instance()->InitScene(StartServerScene::StaticName(), nullptr);
    App::Instance()->MoveSceneToFront(StartServerScene::StaticName());
    _startServerPanelOpen = true;
    return;
}

void EntryScene::OnFileSelected()
{
    if (!_fileLoading)
    {
        FileDialogOptions opt;
        opt.allowedExtensions = SUPORTED_MEDIA_FILE_TYPES_WINAPI;
        _fileDialog.Open(opt);
        _fileLoading = true;
    }
}

void EntryScene::OnPlaylistSelected()
{
    App::Instance()->UninitScene(GetName());
    App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
}