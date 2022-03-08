#include "App.h" // App.h must be included first
#include "EntryScene.h"

#include "ResourceManager.h"
#include "Functions.h"
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

    // Main selection
    {
        _mainPanel = new zcom::Panel();
        _mainPanel->SetHorizontalOffsetPercent(0.5f);
        _mainPanel->SetVerticalOffsetPercent(0.5f);
        _mainPanel->SetBaseWidth(380);
        _mainPanel->SetBaseHeight(180);
        _mainPanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));

        _connectButton = new zcom::Button();
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

        _connectLabel = new zcom::Label(L"Connect");
        _connectLabel->SetHorizontalOffsetPixels(20);
        _connectLabel->SetVerticalOffsetPixels(120);
        _connectLabel->SetBaseWidth(100);
        _connectLabel->SetBaseHeight(30);
        _connectLabel->SetFontSize(24.0f);
        _connectLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _connectLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _shareButton = new zcom::Button();
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

        _shareLabel = new zcom::Label(L"Share");
        _shareLabel->SetHorizontalOffsetPixels(140);
        _shareLabel->SetVerticalOffsetPixels(120);
        _shareLabel->SetBaseWidth(100);
        _shareLabel->SetBaseHeight(30);
        _shareLabel->SetFontSize(24.0f);
        _shareLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _shareLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _shareLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _fileButton = new zcom::Button();
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

        _fileLabel = new zcom::Label(L"Offline");
        _fileLabel->SetHorizontalOffsetPixels(260);
        _fileLabel->SetVerticalOffsetPixels(120);
        _fileLabel->SetBaseWidth(100);
        _fileLabel->SetBaseHeight(30);
        _fileLabel->SetFontSize(24.0f);
        _fileLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _fileLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _fileLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
    }

    // Connect input
    {
        _connectPanel = new zcom::Panel();
        _connectPanel->SetHorizontalOffsetPercent(0.5f);
        _connectPanel->SetVerticalOffsetPercent(0.5f);
        _connectPanel->SetBaseWidth(210);
        _connectPanel->SetBaseHeight(140);
        _connectPanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));
        _connectPanel->SetActive(false);
        _connectPanel->SetVisible(false);

        _connectPanelLabel = new zcom::Label(L"Connect");
        _connectPanelLabel->SetHorizontalOffsetPercent(0.5f);
        _connectPanelLabel->SetVerticalOffsetPixels(10);
        _connectPanelLabel->SetBaseWidth(150);
        _connectPanelLabel->SetBaseHeight(40);
        _connectPanelLabel->SetFontSize(32.0f);
        _connectPanelLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectPanelLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

        _connectIpInput = new zcom::TextInput();
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

        _connectPortInput = new zcom::TextInput();
        _connectPortInput->SetHorizontalOffsetPixels(140);
        _connectPortInput->SetVerticalOffsetPixels(60);
        _connectPortInput->SetBaseWidth(50);
        _connectPortInput->SetBaseHeight(20);
        _connectPortInput->SetPlaceholderText(L"Port");
        _connectPortInput->SetTabIndex(1);

        _connectConfirmButton = new zcom::Button(L"Connect");
        _connectConfirmButton->SetHorizontalOffsetPercent(0.5f);
        _connectConfirmButton->SetHorizontalOffsetPixels(-40);
        _connectConfirmButton->SetVerticalOffsetPixels(100);
        _connectConfirmButton->SetBaseWidth(60);
        _connectConfirmButton->SetBaseHeight(20);
        _connectConfirmButton->SetBorderVisibility(true);
        _connectConfirmButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _connectConfirmButton->SetOnActivated([&]() { OnConnectConfirmed(); });
        _connectConfirmButton->SetTabIndex(2);

        _connectCancelButton = new zcom::Button(L"Cancel");
        _connectCancelButton->SetHorizontalOffsetPercent(0.5f);
        _connectCancelButton->SetHorizontalOffsetPixels(40);
        _connectCancelButton->SetVerticalOffsetPixels(100);
        _connectCancelButton->SetBaseWidth(60);
        _connectCancelButton->SetBaseHeight(20);
        _connectCancelButton->SetBorderVisibility(true);
        _connectCancelButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _connectCancelButton->SetOnActivated([&]() { OnConnectCanceled(); });
        _connectCancelButton->SetTabIndex(3);

        _connectLoadingImage = new zcom::LoadingImage();
        _connectLoadingImage->SetHorizontalOffsetPercent(0.5f);
        _connectLoadingImage->SetVerticalOffsetPixels(40);
        _connectLoadingImage->SetBaseWidth(50);
        _connectLoadingImage->SetBaseHeight(50);
        _connectLoadingImage->SetVisible(false);

        _connectLoadingInfoLabel = new zcom::Label(L"Connecting...");
        _connectLoadingInfoLabel->SetHorizontalOffsetPercent(0.5f);
        _connectLoadingInfoLabel->SetVerticalOffsetPixels(70);
        _connectLoadingInfoLabel->SetBaseWidth(100);
        _connectLoadingInfoLabel->SetBaseHeight(20);
        _connectLoadingInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectLoadingInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _connectLoadingInfoLabel->SetVisible(false);

        _connectLoadingCancelButton = new zcom::Button(L"Cancel");
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
    {
        _sharePanel = new zcom::Panel();
        _sharePanel->SetHorizontalOffsetPercent(0.5f);
        _sharePanel->SetVerticalOffsetPercent(0.5f);
        _sharePanel->SetBaseWidth(180);
        _sharePanel->SetBaseHeight(140);
        _sharePanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));
        _sharePanel->SetActive(false);
        _sharePanel->SetVisible(false);

        _sharePanelLabel = new zcom::Label(L"Share");
        _sharePanelLabel->SetHorizontalOffsetPercent(0.5f);
        _sharePanelLabel->SetVerticalOffsetPixels(10);
        _sharePanelLabel->SetBaseWidth(150);
        _sharePanelLabel->SetBaseHeight(40);
        _sharePanelLabel->SetFontSize(32.0f);
        _sharePanelLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _sharePanelLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

        _sharePortInput = new zcom::TextInput();
        _sharePortInput->SetHorizontalOffsetPixels(20);
        _sharePortInput->SetVerticalOffsetPixels(60);
        _sharePortInput->SetBaseWidth(140);
        _sharePortInput->SetBaseHeight(20);
        _sharePortInput->SetPlaceholderText(L"Port");
        _sharePortInput->SetTabIndex(0);

        _shareConfirmButton = new zcom::Button(L"Share");
        _shareConfirmButton->SetHorizontalOffsetPercent(0.5f);
        _shareConfirmButton->SetHorizontalOffsetPixels(-40);
        _shareConfirmButton->SetVerticalOffsetPixels(100);
        _shareConfirmButton->SetBaseWidth(60);
        _shareConfirmButton->SetBaseHeight(20);
        _shareConfirmButton->SetBorderVisibility(true);
        _shareConfirmButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _shareConfirmButton->SetOnActivated([&]() { OnShareConfirmed(); });
        _shareConfirmButton->SetTabIndex(1);

        _shareCancelButton = new zcom::Button(L"Cancel");
        _shareCancelButton->SetHorizontalOffsetPercent(0.5f);
        _shareCancelButton->SetHorizontalOffsetPixels(40);
        _shareCancelButton->SetVerticalOffsetPixels(100);
        _shareCancelButton->SetBaseWidth(60);
        _shareCancelButton->SetBaseHeight(20);
        _shareCancelButton->SetBorderVisibility(true);
        _shareCancelButton->SetBorderColor(D2D1::ColorF(0.4f, 0.4f, 0.4f));
        _shareCancelButton->SetOnActivated([&]() { OnShareCanceled(); });
        _shareCancelButton->SetTabIndex(2);

        _shareLoadingImage = new zcom::LoadingImage();
        _shareLoadingImage->SetHorizontalOffsetPercent(0.5f);
        _shareLoadingImage->SetVerticalOffsetPixels(40);
        _shareLoadingImage->SetBaseWidth(50);
        _shareLoadingImage->SetBaseHeight(50);
        _shareLoadingImage->SetVisible(false);

        _shareLoadingInfoLabel = new zcom::Label(L"Waiting for connection...");
        _shareLoadingInfoLabel->SetHorizontalOffsetPercent(0.5f);
        _shareLoadingInfoLabel->SetVerticalOffsetPixels(70);
        _shareLoadingInfoLabel->SetBaseWidth(150);
        _shareLoadingInfoLabel->SetBaseHeight(20);
        _shareLoadingInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _shareLoadingInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _shareLoadingInfoLabel->SetVisible(false);

        _shareLoadingCancelButton = new zcom::Button(L"Cancel");
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
        _mainPanel->AddItem(_connectButton);
        _mainPanel->AddItem(_connectLabel);
        _mainPanel->AddItem(_shareButton);
        _mainPanel->AddItem(_shareLabel);
        _mainPanel->AddItem(_fileButton);
        _mainPanel->AddItem(_fileLabel);

        _connectPanel->AddItem(_connectPanelLabel);
        _connectPanel->AddItem(_connectIpInput);
        _connectPanel->AddItem(_connectPortInput);
        _connectPanel->AddItem(_connectConfirmButton);
        _connectPanel->AddItem(_connectCancelButton);
        _connectPanel->AddItem(_connectLoadingImage);
        _connectPanel->AddItem(_connectLoadingInfoLabel);
        _connectPanel->AddItem(_connectLoadingCancelButton);

        _sharePanel->AddItem(_sharePanelLabel);
        _sharePanel->AddItem(_sharePortInput);
        _sharePanel->AddItem(_shareConfirmButton);
        _sharePanel->AddItem(_shareCancelButton);
        _sharePanel->AddItem(_shareLoadingImage);
        _sharePanel->AddItem(_shareLoadingInfoLabel);
        _sharePanel->AddItem(_shareLoadingCancelButton);

        //zcom::Panel* yeet = new zcom::Panel();
        //yeet->SetBaseSize(100, 100);
        //yeet->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Red));
        //_mainPanel->AddItem(yeet);

        _canvas->AddComponent(_mainPanel);
        _canvas->AddComponent(_connectPanel);
        _canvas->AddComponent(_sharePanel);
        _canvas->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    }
}

void EntryScene::_Uninit()
{
    _canvas->ClearComponents();
    delete _mainPanel;
    delete _connectButton;
    delete _shareButton;
    delete _fileButton;
    delete _connectLabel;
    delete _shareLabel;
    delete _fileLabel;
    delete _connectPanel;
    delete _connectPanelLabel;
    delete _connectIpInput;
    delete _connectPortInput;
    delete _connectConfirmButton;
    delete _connectCancelButton;
    _mainPanel = nullptr;
    _connectButton = nullptr;
    _shareButton = nullptr;
    _fileButton = nullptr;
    _connectLabel = nullptr;
    _shareLabel = nullptr;
    _fileLabel = nullptr;
    _connectPanel = nullptr;
    _connectPanelLabel = nullptr;
    _connectIpInput = nullptr;
    _connectPortInput = nullptr;
    _connectConfirmButton = nullptr;
    _connectCancelButton = nullptr;
}

void EntryScene::_Focus()
{

}

void EntryScene::_Unfocus()
{

}

void EntryScene::_Update()
{
    if (_setConnectFocus)
    {
        _canvas->ClearSelection(_connectIpInput);
        if (!_connectIpInput->Selected())
        {
            _connectIpInput->OnSelected();
        }
        _setConnectFocus = false;
    }
    if (_setShareFocus)
    {
        _canvas->ClearSelection(_sharePortInput);
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
    _mainPanel->SetActive(false);
    _mainPanel->SetVisible(false);

    _connectPanel->SetActive(true);
    _connectPanel->SetVisible(true);
    _connectIpInput->SetText(L"");
    _connectPortInput->SetText(L"");
    _setConnectFocus = true;
}

void EntryScene::OnShareSelected()
{
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
    std::string ip = wstring_to_string(_connectIpInput->GetText());
    USHORT port = str_to_int(wstring_to_string(_connectPortInput->GetText()));
    znet::NetworkInterface::Instance()->Connect(ip, port);
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