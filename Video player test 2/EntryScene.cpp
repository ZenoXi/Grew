#include "App.h" // App.h must be included first
#include "EntryScene.h"

#include "ResourceManager.h"
#include "Options.h"
#include "LastIpOptionAdapter.h"
#include "Functions.h"
#include "ConnectScene.h"
#include "StartServerScene.h"
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "NetworkInterfaceNew.h"

EntryScene::EntryScene() {}

void EntryScene::_Init(const SceneOptionsBase* options)
{
    EntrySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const EntrySceneOptions*>(options);
    }

    zcom::PROP_Shadow shadowProps;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 0.75f);

    // Main selection
    _mainPanel = std::make_unique<zcom::Panel>();
    {
        _mainPanel->SetHorizontalOffsetPercent(0.5f);
        _mainPanel->SetVerticalOffsetPercent(0.5f);
        _mainPanel->SetBaseWidth(380);
        _mainPanel->SetBaseHeight(180);
        _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f));
        //_mainPanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));
        _mainPanel->VerticalScrollable(true);
        _mainPanel->HorizontalScrollable(true);
        _mainPanel->SetCornerRounding(5.0f);
        _mainPanel->SetProperty(shadowProps);

        _connectButton = std::make_unique<zcom::Button>();
        _connectButton->SetHorizontalOffsetPixels(20);
        _connectButton->SetVerticalOffsetPixels(20);
        _connectButton->SetBaseWidth(100);
        _connectButton->SetBaseHeight(100);
        _connectButton->SetButtonImage(ResourceManager::GetImage("connect_dim"));
        _connectButton->SetButtonHoverImage(ResourceManager::GetImage("connect_bright"));
        _connectButton->SetButtonClickImage(ResourceManager::GetImage("connect_bright"));
        _connectButton->SetButtonHoverColor(D2D1::ColorF(0, 0.0f));
        _connectButton->SetOnActivated([&]() { OnConnectSelected(); });
        _connectButton->SetTabIndex(0);

        _connectLabel = std::make_unique<zcom::Label>(L"Connect");
        _connectLabel->SetHorizontalOffsetPixels(20);
        _connectLabel->SetVerticalOffsetPixels(120);
        _connectLabel->SetBaseWidth(100);
        _connectLabel->SetBaseHeight(30);
        _connectLabel->SetFontSize(24.0f);
        _connectLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _connectLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _shareButton = std::make_unique<zcom::Button>();
        _shareButton->SetHorizontalOffsetPixels(140);
        _shareButton->SetVerticalOffsetPixels(20);
        _shareButton->SetBaseWidth(100);
        _shareButton->SetBaseHeight(100);
        _shareButton->SetButtonImage(ResourceManager::GetImage("share_dim"));
        _shareButton->SetButtonHoverImage(ResourceManager::GetImage("share_bright"));
        _shareButton->SetButtonClickImage(ResourceManager::GetImage("share_bright"));
        _shareButton->SetButtonHoverColor(D2D1::ColorF(0, 0.0f));
        _shareButton->SetOnActivated([&]() { OnShareSelected(); });
        _shareButton->SetTabIndex(1);

        _shareLabel = std::make_unique<zcom::Label>(L"Share");
        _shareLabel->SetHorizontalOffsetPixels(140);
        _shareLabel->SetVerticalOffsetPixels(120);
        _shareLabel->SetBaseWidth(100);
        _shareLabel->SetBaseHeight(30);
        _shareLabel->SetFontSize(24.0f);
        _shareLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _shareLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _shareLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _fileButton = std::make_unique<zcom::Button>();
        _fileButton->SetHorizontalOffsetPixels(260);
        _fileButton->SetVerticalOffsetPixels(20);
        _fileButton->SetBaseWidth(100);
        _fileButton->SetBaseHeight(100);
        _fileButton->SetButtonImage(ResourceManager::GetImage("file_dim"));
        _fileButton->SetButtonHoverImage(ResourceManager::GetImage("file_bright"));
        _fileButton->SetButtonClickImage(ResourceManager::GetImage("file_bright"));
        _fileButton->SetButtonHoverColor(D2D1::ColorF(0, 0.0f));
        _fileButton->SetOnActivated([&]() { OnFileSelected(); });
        _fileButton->SetTabIndex(2);

        _fileLabel = std::make_unique<zcom::Label>(L"Offline");
        _fileLabel->SetHorizontalOffsetPixels(260);
        _fileLabel->SetVerticalOffsetPixels(120);
        _fileLabel->SetBaseWidth(100);
        _fileLabel->SetBaseHeight(30);
        _fileLabel->SetFontSize(24.0f);
        _fileLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _fileLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _fileLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _testButton = std::make_unique<zcom::Button>(L"Hi");
        _testButton->SetOffsetPixels(1000, 300);
        _testButton->SetBaseSize(100, 25);
        _testButton->SetBorderVisibility(true);
    }

    // Connect input
    _connectPanel = std::make_unique<zcom::Panel>();
    {
        _connectPanel->SetHorizontalOffsetPercent(0.5f);
        _connectPanel->SetVerticalOffsetPercent(0.5f);
        _connectPanel->SetBaseWidth(210);
        _connectPanel->SetBaseHeight(220);
        //_connectPanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));
        _connectPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f));
        _connectPanel->SetActive(false);
        _connectPanel->SetVisible(false);
        _connectPanel->SetCornerRounding(5.0f);
        _connectPanel->SetProperty(shadowProps);

        _connectPanelLabel = std::make_unique<zcom::Label>(L"Connect");
        _connectPanelLabel->SetHorizontalOffsetPercent(0.5f);
        _connectPanelLabel->SetVerticalOffsetPixels(10);
        _connectPanelLabel->SetBaseWidth(150);
        _connectPanelLabel->SetBaseHeight(40);
        _connectPanelLabel->SetFontSize(32.0f);
        _connectPanelLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectPanelLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

        _connectIpInput = std::make_unique<zcom::TextInput>();
        _connectIpInput->SetHorizontalOffsetPixels(20);
        _connectIpInput->SetVerticalOffsetPixels(60);
        _connectIpInput->SetBaseWidth(110);
        _connectIpInput->SetBaseHeight(20);
        _connectIpInput->SetPlaceholderText(L"IP address");
        _connectIpInput->SetTabIndex(0);
        //_connectIpInput->AddOnChar([&](wchar_t ch) -> bool
        //{
        //    if (ch == '\t')
        //    {
        //        _canvas->ClearSelection();
        //        _connectPortInput->OnSelected();
        //        return true;
        //    }
        //    return false;
        //});
        _connectPortInput = std::make_unique<zcom::TextInput>();
        _connectPortInput->SetHorizontalOffsetPixels(140);
        _connectPortInput->SetVerticalOffsetPixels(60);
        _connectPortInput->SetBaseWidth(50);
        _connectPortInput->SetBaseHeight(20);
        _connectPortInput->SetPlaceholderText(L"Port");
        _connectPortInput->SetTabIndex(1);

        _connectConfirmButton = std::make_unique<zcom::Button>(L"Connect");
        _connectConfirmButton->SetOffsetPixels(20, 180);
        _connectConfirmButton->SetBaseSize(60, 20);
        _connectConfirmButton->SetBorderVisibility(true);
        _connectConfirmButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _connectConfirmButton->SetOnActivated([&]() { OnConnectConfirmed(); });
        _connectConfirmButton->SetTabIndex(2);

        _connectCancelButton = std::make_unique<zcom::Button>(L"Cancel");
        _connectCancelButton->SetOffsetPixels(130, 180);
        _connectCancelButton->SetBaseSize(60, 20);
        _connectCancelButton->SetBorderVisibility(true);
        _connectCancelButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _connectCancelButton->SetOnActivated([&]() { OnConnectCanceled(); });
        _connectCancelButton->SetTabIndex(3);

        _recentConnectionsLabel = std::make_unique<zcom::Label>(L"Recent:");
        _recentConnectionsLabel->SetOffsetPixels(20, 80);
        _recentConnectionsLabel->SetBaseSize(60, 20);
        _recentConnectionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

        _recentConnectionsPanel = std::make_unique<zcom::Panel>();
        _recentConnectionsPanel->SetOffsetPixels(20, 100);
        _recentConnectionsPanel->SetBaseSize(170, 70);
        _recentConnectionsPanel->SetBorderVisibility(true);
        _recentConnectionsPanel->SetBorderColor(_connectIpInput->GetBorderColor());
        _recentConnectionsPanel->SetBackgroundColor(_connectIpInput->GetBackgroundColor());
        _recentConnectionsPanel->VerticalScrollable(true);
        _recentConnectionsPanel->SetTabIndex(-1);

        _connectLoadingImage = std::make_unique<zcom::LoadingImage>();
        _connectLoadingImage->SetHorizontalOffsetPercent(0.5f);
        _connectLoadingImage->SetVerticalOffsetPixels(40);
        _connectLoadingImage->SetBaseWidth(50);
        _connectLoadingImage->SetBaseHeight(50);
        _connectLoadingImage->SetVisible(false);

        _connectLoadingInfoLabel = std::make_unique<zcom::Label>(L"Connecting...");
        _connectLoadingInfoLabel->SetHorizontalOffsetPercent(0.5f);
        _connectLoadingInfoLabel->SetVerticalOffsetPixels(70);
        _connectLoadingInfoLabel->SetBaseWidth(100);
        _connectLoadingInfoLabel->SetBaseHeight(20);
        _connectLoadingInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectLoadingInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _connectLoadingInfoLabel->SetVisible(false);

        _connectLoadingCancelButton = std::make_unique<zcom::Button>(L"Cancel");
        _connectLoadingCancelButton->SetHorizontalOffsetPercent(0.5f);
        _connectLoadingCancelButton->SetVerticalOffsetPixels(100);
        _connectLoadingCancelButton->SetBaseWidth(60);
        _connectLoadingCancelButton->SetBaseHeight(20);
        _connectLoadingCancelButton->SetBorderVisibility(true);
        _connectLoadingCancelButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _connectLoadingCancelButton->SetOnActivated([&]() { OnConnectLoadingCanceled(); });
        _connectLoadingCancelButton->SetVisible(false);
    }

    // Share input
    _sharePanel = std::make_unique<zcom::Panel>();
    {
        _sharePanel->SetHorizontalOffsetPercent(0.5f);
        _sharePanel->SetVerticalOffsetPercent(0.5f);
        _sharePanel->SetBaseWidth(180);
        _sharePanel->SetBaseHeight(140);
        //_sharePanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));
        _sharePanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f));
        _sharePanel->SetActive(false);
        _sharePanel->SetVisible(false);
        _sharePanel->SetCornerRounding(5.0f);
        _sharePanel->SetProperty(shadowProps);

        _sharePanelLabel = std::make_unique<zcom::Label>(L"Share");
        _sharePanelLabel->SetHorizontalOffsetPercent(0.5f);
        _sharePanelLabel->SetVerticalOffsetPixels(10);
        _sharePanelLabel->SetBaseWidth(150);
        _sharePanelLabel->SetBaseHeight(40);
        _sharePanelLabel->SetFontSize(32.0f);
        _sharePanelLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _sharePanelLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

        _sharePortInput = std::make_unique<zcom::TextInput>();
        _sharePortInput->SetHorizontalOffsetPixels(20);
        _sharePortInput->SetVerticalOffsetPixels(60);
        _sharePortInput->SetBaseWidth(140);
        _sharePortInput->SetBaseHeight(20);
        _sharePortInput->SetPlaceholderText(L"Port");
        _sharePortInput->SetTabIndex(0);

        _shareConfirmButton = std::make_unique<zcom::Button>(L"Share");
        _shareConfirmButton->SetHorizontalOffsetPercent(0.5f);
        _shareConfirmButton->SetHorizontalOffsetPixels(-40);
        _shareConfirmButton->SetVerticalOffsetPixels(100);
        _shareConfirmButton->SetBaseWidth(60);
        _shareConfirmButton->SetBaseHeight(20);
        _shareConfirmButton->SetBorderVisibility(true);
        _shareConfirmButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _shareConfirmButton->SetOnActivated([&]() { OnShareConfirmed(); });
        _shareConfirmButton->SetTabIndex(1);

        _shareCancelButton = std::make_unique<zcom::Button>(L"Cancel");
        _shareCancelButton->SetHorizontalOffsetPercent(0.5f);
        _shareCancelButton->SetHorizontalOffsetPixels(40);
        _shareCancelButton->SetVerticalOffsetPixels(100);
        _shareCancelButton->SetBaseWidth(60);
        _shareCancelButton->SetBaseHeight(20);
        _shareCancelButton->SetBorderVisibility(true);
        _shareCancelButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _shareCancelButton->SetOnActivated([&]() { OnShareCanceled(); });
        _shareCancelButton->SetTabIndex(2);

        _shareLoadingImage = std::make_unique<zcom::LoadingImage>();
        _shareLoadingImage->SetHorizontalOffsetPercent(0.5f);
        _shareLoadingImage->SetVerticalOffsetPixels(40);
        _shareLoadingImage->SetBaseWidth(50);
        _shareLoadingImage->SetBaseHeight(50);
        _shareLoadingImage->SetVisible(false);

        _shareLoadingInfoLabel = std::make_unique<zcom::Label>(L"Waiting for connection...");
        _shareLoadingInfoLabel->SetHorizontalOffsetPercent(0.5f);
        _shareLoadingInfoLabel->SetVerticalOffsetPixels(70);
        _shareLoadingInfoLabel->SetBaseWidth(150);
        _shareLoadingInfoLabel->SetBaseHeight(20);
        _shareLoadingInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _shareLoadingInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _shareLoadingInfoLabel->SetVisible(false);

        _shareLoadingCancelButton = std::make_unique<zcom::Button>(L"Cancel");
        _shareLoadingCancelButton->SetHorizontalOffsetPercent(0.5f);
        _shareLoadingCancelButton->SetVerticalOffsetPixels(100);
        _shareLoadingCancelButton->SetBaseWidth(60);
        _shareLoadingCancelButton->SetBaseHeight(20);
        _shareLoadingCancelButton->SetBorderVisibility(true);
        _shareLoadingCancelButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _shareLoadingCancelButton->SetOnActivated([&]() { OnShareLoadingCanceled(); });
        _shareLoadingCancelButton->SetVisible(false);
    }

    // Nesting
    {
        _mainPanel->AddItem(_connectButton.get());
        _mainPanel->AddItem(_connectLabel.get());
        _mainPanel->AddItem(_shareButton.get());
        _mainPanel->AddItem(_shareLabel.get());
        _mainPanel->AddItem(_fileButton.get());
        _mainPanel->AddItem(_fileLabel.get());
        _mainPanel->AddItem(_testButton.get());

        _connectPanel->AddItem(_connectPanelLabel.get());
        _connectPanel->AddItem(_connectIpInput.get());
        _connectPanel->AddItem(_connectPortInput.get());
        _connectPanel->AddItem(_connectConfirmButton.get());
        _connectPanel->AddItem(_connectCancelButton.get());
        _connectPanel->AddItem(_recentConnectionsLabel.get());
        _connectPanel->AddItem(_recentConnectionsPanel.get());
        _connectPanel->AddItem(_connectLoadingImage.get());
        _connectPanel->AddItem(_connectLoadingInfoLabel.get());
        _connectPanel->AddItem(_connectLoadingCancelButton.get());

        _sharePanel->AddItem(_sharePanelLabel.get());
        _sharePanel->AddItem(_sharePortInput.get());
        _sharePanel->AddItem(_shareConfirmButton.get());
        _sharePanel->AddItem(_shareCancelButton.get());
        _sharePanel->AddItem(_shareLoadingImage.get());
        _sharePanel->AddItem(_shareLoadingInfoLabel.get());
        _sharePanel->AddItem(_shareLoadingCancelButton.get());

        //_canvas->AddComponent(_testNotification.get());
        //_canvas->GetPanel()->AddItemShadow(_testNotification.get());
        _canvas->AddComponent(_mainPanel.get());
        _canvas->AddComponent(_connectPanel.get());
        _canvas->AddComponent(_sharePanel.get());
        _canvas->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    }
}

void EntryScene::_Uninit()
{
    _canvas->ClearComponents();

    _mainPanel = nullptr;
    _connectButton = nullptr;
    _shareButton = nullptr;
    _fileButton = nullptr;
    _connectLabel = nullptr;
    _shareLabel = nullptr;
    _fileLabel = nullptr;
    _testButton = nullptr;

    _connectPanel = nullptr;
    _connectPanelLabel = nullptr;
    _connectIpInput = nullptr;
    _connectPortInput = nullptr;
    _connectConfirmButton = nullptr;
    _connectCancelButton = nullptr;
    _recentConnectionsLabel = nullptr;
    _recentConnectionsPanel = nullptr;
    _connectLoadingImage = nullptr;
    _connectLoadingInfoLabel = nullptr;
    _connectLoadingCancelButton = nullptr;

    _sharePanel = nullptr;
    _sharePanelLabel = nullptr;
    _sharePortInput = nullptr;
    _shareConfirmButton = nullptr;
    _shareCancelButton = nullptr;
    _shareLoadingImage = nullptr;
    _shareLoadingInfoLabel = nullptr;
    _shareLoadingCancelButton = nullptr;
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
                if (scene->ConnectionSuccessful())
                {
                    App::Instance()->UninitScene(ConnectScene::StaticName());
                    App::Instance()->UninitScene(GetName());
                    return;
                }
                else
                {
                    App::Instance()->UninitScene(ConnectScene::StaticName());
                    _connectPanelOpen = false;
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
                if (scene->StartSuccessful())
                {
                    App::Instance()->UninitScene(StartServerScene::StaticName());
                    App::Instance()->UninitScene(GetName());
                    return;
                }
                else
                {
                    App::Instance()->UninitScene(StartServerScene::StaticName());
                    _startServerPanelOpen = false;
                }
            }
        }
    }

    if (_setConnectFocus)
    {
        _canvas->ClearSelection(_connectIpInput.get());
        if (!_connectIpInput->Selected())
        {
            _connectIpInput->OnSelected();
        }
        _setConnectFocus = false;
    }
    if (_setShareFocus)
    {
        _canvas->ClearSelection(_sharePortInput.get());
        if (!_sharePortInput->Selected())
        {
            _sharePortInput->OnSelected();
        }
        _setShareFocus = false;
    }

    if (_connectLoading)
    {
        if (NetworkInterfaceOld::Instance()->Connected())
        {
            PlaybackSceneOptions options;
            options.playbackMode = PlaybackMode::CLIENT;
            App::Instance()->SetScene(PlaybackScene::StaticName(), &options);
        }
    }
    else if (_shareLoading)
    {
        if (NetworkInterfaceOld::Instance()->ClientConnected())
        {
            PlaybackSceneOptions options;
            options.fileName = _shareFilename;
            options.playbackMode = PlaybackMode::SERVER;
            App::Instance()->SetScene(PlaybackScene::StaticName(), &options);
        }
    }
    else if (_fileLoading)
    {
        if (_fileDialog.Done())
        {
            _fileLoading = false;
            //std::wstring filename = OpenFile();
            std::wstring filename = _fileDialog.Result();
            if (filename != L"")
            {
                //PlaybackSceneOptions* options = new PlaybackSceneOptions();
                //options->fileName = wstring_to_string(filename);
                //options->mode = PlaybackMode::OFFLINE;
                //App::Instance()->UninitScene(GetName());
                //App::Instance()->InitScene(PlaybackScene::StaticName(), options);
                //App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
                //App::Instance()->SetScene("playback_scene", options);
                //delete options;

                App::Instance()->UninitScene(GetName());
                Scene* scenePtr = App::Instance()->FindScene(PlaybackOverlayScene::StaticName());
                if (scenePtr)
                {
                    PlaybackOverlayScene* scene = (PlaybackOverlayScene*)scenePtr;
                    scene->AddItem(filename);
                    scene->WaitForLoad(true);
                }
                else
                {
                    std::cout << "[FATAL] Playback overlay scene missing, exiting" << std::endl;
                    exit(1);
                }
            }
        }
    }
}

ID2D1Bitmap1* EntryScene::_Draw(Graphics g)
{
    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
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

    _mainPanel->SetActive(false);
    _mainPanel->SetVisible(false);

    _connectPanel->SetActive(true);
    _connectPanel->SetVisible(true);

    // Init recent connections panel
    LastIpOptionAdapter optAdapter(Options::Instance()->GetValue("lastIps"));
    auto ipList = optAdapter.GetList();
    _recentConnectionsPanel->ClearItems();
    for (int i = 0; i < ipList.size(); i++)
    {
        std::string ip = ipList[i];
        zcom::Button* button = new zcom::Button(string_to_wstring(ip));
        button->SetParentWidthPercent(1.0f);
        button->SetBaseHeight(20);
        button->SetVerticalOffsetPixels(i * 20);
        button->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        button->Text()->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        button->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        button->SetOnActivated([&, ip]()
        {
            std::array<std::string, 2> ipParts;
            split_str(ip, ipParts, ':');
            _connectIpInput->SetText(string_to_wstring(ipParts[0]));
            _connectPortInput->SetText(string_to_wstring(ipParts[1]));
        });
        _recentConnectionsPanel->AddItem(button);
    }
    _recentConnectionsPanel->Resize();

    _setConnectFocus = true;
}

void EntryScene::OnShareSelected()
{
    App::Instance()->InitScene(StartServerScene::StaticName(), nullptr);
    App::Instance()->MoveSceneToFront(StartServerScene::StaticName());
    _startServerPanelOpen = true;
    return;

    _mainPanel->SetActive(false);
    _mainPanel->SetVisible(false);

    _sharePanel->SetActive(true);
    _sharePanel->SetVisible(true);
    _sharePortInput->SetText(L"");
    _setShareFocus = true;
}

void EntryScene::OnFileSelected()
{
    if (!_fileLoading)
    {
        _fileDialog.Open();
        _fileLoading = true;
    }
}

void EntryScene::OnConnectConfirmed()
{
    std::string ipStr = wstring_to_string(_connectIpInput->GetText());
    std::string portStr = wstring_to_string(_connectPortInput->GetText());
    LastIpOptionAdapter optAdapter(Options::Instance()->GetValue("lastIps"));
    bool ipValid = optAdapter.AddIp(ipStr, portStr);
    if (!ipValid)
    {
        // Add visual error indication
        return;
    }
    Options::Instance()->SetValue("lastIps", optAdapter.ToOptionString());

    USHORT port = str_to_int(portStr);
    znet::NetworkInterface::Instance()->Connect(ipStr, port);
    App::Instance()->UninitScene(GetName());
    return;

    _connectIpInput->SetVisible(false);
    _connectPortInput->SetVisible(false);
    _connectConfirmButton->SetVisible(false);
    _connectCancelButton->SetVisible(false);
    _connectLoadingImage->SetVisible(true);
    _connectLoadingInfoLabel->SetVisible(true);
    _connectLoadingCancelButton->SetVisible(true);
    _connectLoading = true;
}

void EntryScene::OnConnectCanceled()
{
    _connectPanel->SetActive(false);
    _connectPanel->SetVisible(false);

    _mainPanel->SetActive(true);
    _mainPanel->SetVisible(true);
}

void EntryScene::OnConnectLoadingCanceled()
{
    _connectIpInput->SetVisible(true);
    _connectPortInput->SetVisible(true);
    _connectConfirmButton->SetVisible(true);
    _connectCancelButton->SetVisible(true);
    _connectLoadingImage->SetVisible(false);
    _connectLoadingInfoLabel->SetVisible(false);
    _connectLoadingCancelButton->SetVisible(false);
    _connectLoading = false;
}

void EntryScene::OnShareConfirmed()
{
    //_shareFilename = wstring_to_string(OpenFile());
    USHORT port = str_to_int(wstring_to_string(_sharePortInput->GetText()));
    znet::NetworkInterface::Instance()->StartServer(port);
    App::Instance()->UninitScene(GetName());
    return;

    _sharePortInput->SetVisible(false);
    _shareConfirmButton->SetVisible(false);
    _shareCancelButton->SetVisible(false);
    _shareLoadingImage->SetVisible(true);
    _shareLoadingInfoLabel->SetVisible(true);
    _shareLoadingCancelButton->SetVisible(true);
    _shareLoading = true;
}

void EntryScene::OnShareCanceled()
{
    _sharePanel->SetActive(false);
    _sharePanel->SetVisible(false);

    _mainPanel->SetActive(true);
    _mainPanel->SetVisible(true);
}

void EntryScene::OnShareLoadingCanceled()
{
    _sharePortInput->SetVisible(true);
    _shareConfirmButton->SetVisible(true);
    _shareCancelButton->SetVisible(true);
    _shareLoadingImage->SetVisible(false);
    _shareLoadingInfoLabel->SetVisible(false);
    _shareLoadingCancelButton->SetVisible(false);
    _shareLoading = false;
    NetworkInterfaceOld::Instance()->StopServer();
}