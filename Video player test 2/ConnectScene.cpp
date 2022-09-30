#include "App.h" // App.h must be included first
#include "ConnectScene.h"
#include "OverlayScene.h"

#include "Options.h"
#include "OptionNames.h"
#include "LastIpOptionAdapter.h"
#include "Network.h"
#include "ClientManager.h"

ConnectScene::ConnectScene(App* app)
    : Scene(app)
{

}

void ConnectScene::_Init(const SceneOptionsBase* options)
{
    ConnectSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const ConnectSceneOptions*>(options);
    }
    _owned = opt.owned;

    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);

    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;

    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(500, 500);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    _mainPanel->SetProperty(mainPanelShadow);

    _titleLabel = Create<zcom::Label>(L"Connect");
    _titleLabel->SetBaseSize(400, 30);
    _titleLabel->SetOffsetPixels(30, 30);
    _titleLabel->SetFontSize(42.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _separator1 = Create<zcom::EmptyPanel>();
    _separator1->SetBaseSize(440, 1);
    _separator1->SetVerticalOffsetPixels(80);
    _separator1->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separator1->SetBorderVisibility(true);
    _separator1->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _ipLabel = Create<zcom::Label>(L"Server IP:");
    _ipLabel->SetBaseSize(80, 30);
    _ipLabel->SetOffsetPixels(30, 100);
    _ipLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _ipLabel->SetFontSize(18.0f);

    _ipInput = Create<zcom::TextInput>();
    _ipInput->SetBaseSize(120, 30);
    _ipInput->SetOffsetPixels(120, 100);
    _ipInput->SetCornerRounding(5.0f);
    _ipInput->SetTabIndex(0);
    _ipInput->OnSelected();

    _portLabel = Create<zcom::Label>(L"Port:");
    _portLabel->SetBaseSize(40, 30);
    _portLabel->SetOffsetPixels(250, 100);
    _portLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _portLabel->SetFontSize(18.0f);

    _portInput = Create<zcom::TextInput>();
    _portInput->SetBaseSize(60, 30);
    _portInput->SetOffsetPixels(300, 100);
    _portInput->SetCornerRounding(5.0f);
    _portInput->SetTabIndex(1);

    _recentConnectionsLabel = Create<zcom::Label>(L"Recent:");
    _recentConnectionsLabel->SetBaseSize(80, 30);
    _recentConnectionsLabel->SetOffsetPixels(30, 140);
    _recentConnectionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _recentConnectionsLabel->SetFontSize(18.0f);

    _noRecentConnectionsLabel = Create<zcom::Label>(L"No saved connections");
    _noRecentConnectionsLabel->SetBaseSize(200, 20);
    _noRecentConnectionsLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _noRecentConnectionsLabel->SetFontSize(16.0f);
    _noRecentConnectionsLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _noRecentConnectionsLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));
    _noRecentConnectionsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _noRecentConnectionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _recentConnectionsPanel = Create<zcom::Panel>();
    _recentConnectionsPanel->SetBaseSize(240, 100);
    _recentConnectionsPanel->SetOffsetPixels(120, 140);
    _recentConnectionsPanel->SetBorderVisibility(true);
    _recentConnectionsPanel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _recentConnectionsPanel->SetBackgroundColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _recentConnectionsPanel->SetCornerRounding(5.0f);
    _recentConnectionsPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);
    _recentConnectionsPanel->SetTabIndex(-1);
    _RearrangeRecentConnectionsPanel();

    _usernameLabel = Create<zcom::Label>(L"Username:");
    _usernameLabel->SetBaseSize(80, 30);
    _usernameLabel->SetOffsetPixels(30, 250);
    _usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _usernameLabel->SetFontSize(18.0f);

    _usernameInput = Create<zcom::TextInput>();
    _usernameInput->SetBaseSize(240, 30);
    _usernameInput->SetOffsetPixels(120, 250);
    _usernameInput->SetCornerRounding(5.0f);
    _usernameInput->PlaceholderText()->SetText(L"(Optional)");
    _usernameInput->SetTabIndex(2);

    // Get recent/default username
    std::wstring defaultUsername = Options::Instance()->GetValue(OPTIONS_DEFAULT_USERNAME);
    if (!defaultUsername.empty())
    {
        _usernameInput->Text()->SetText(defaultUsername);
    }
    else
    {
        std::wstring recentUsername = Options::Instance()->GetValue(OPTIONS_LAST_USERNAME);
        if (!recentUsername.empty())
        {
            _usernameInput->Text()->SetText(recentUsername);
        }
    }

    _separator2 = Create<zcom::EmptyPanel>();
    _separator2->SetBaseSize(440, 1);
    _separator2->SetVerticalOffsetPixels(-80);
    _separator2->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separator2->SetVerticalAlignment(zcom::Alignment::END);
    _separator2->SetBorderVisibility(true);
    _separator2->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _connectButton = Create<zcom::Button>(L"Connect");
    _connectButton->SetBaseSize(80, 30);
    _connectButton->SetOffsetPixels(-30, -30);
    _connectButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _connectButton->Text()->SetFontSize(18.0f);
    _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
    //_connectButton->SetSelectedBorderColor(D2D1::ColorF(0, 0));
    _connectButton->SetTabIndex(3);
    _connectButton->SetCornerRounding(5.0f);
    _connectButton->SetProperty(buttonShadow);
    _connectButton->SetOnActivated([&]() { _ConnectClicked(); });

    _cancelButton = Create<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 30);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("close_100x100"));
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });
    _cancelButton->SetTabIndex(1000);

    _connectLoadingImage = Create<zcom::LoadingImage>();
    _connectLoadingImage->SetBaseSize(30, 30);
    _connectLoadingImage->SetOffsetPixels(-130, -30);
    _connectLoadingImage->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _connectLoadingImage->SetDotHeight(10.0f);
    _connectLoadingImage->SetDotRadius(2.5f);
    _connectLoadingImage->SetDotSpacing(8.0f);
    _connectLoadingImage->SetDotColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _connectLoadingImage->SetVisible(false);

    _connectLoadingInfoLabel = Create<zcom::Label>(L"");
    _connectLoadingInfoLabel->SetBaseSize(300, 30);
    _connectLoadingInfoLabel->SetOffsetPixels(-130, -30);
    _connectLoadingInfoLabel->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _connectLoadingInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::TRAILING);
    _connectLoadingInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _connectLoadingInfoLabel->SetFontColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
    _connectLoadingInfoLabel->SetFontSize(16.0f);
    _connectLoadingInfoLabel->SetVisible(false);

    _mainPanel->AddItem(_titleLabel.get());
    _mainPanel->AddItem(_separator1.get());
    _mainPanel->AddItem(_ipLabel.get());
    _mainPanel->AddItem(_ipInput.get());
    _mainPanel->AddItem(_portLabel.get());
    _mainPanel->AddItem(_portInput.get());
    _mainPanel->AddItem(_recentConnectionsLabel.get());
    _mainPanel->AddItem(_recentConnectionsPanel.get());
    _mainPanel->AddItem(_usernameLabel.get());
    _mainPanel->AddItem(_usernameInput.get());
    _mainPanel->AddItem(_separator2.get());
    _mainPanel->AddItem(_connectButton.get());
    _mainPanel->AddItem(_cancelButton.get());
    _mainPanel->AddItem(_connectLoadingImage.get());
    _mainPanel->AddItem(_connectLoadingInfoLabel.get());

    _dimPanel = Create<zcom::EmptyPanel>();
    _dimPanel->SetParentSizePercent(1.0f, 1.0f);
    _dimPanel->SetBackgroundColor(D2D1::ColorF(0, 0.4f));
    _dimPanel->SetZIndex(1);
    _dimPanel->SetVisible(false);

    _passwordPanel = Create<zcom::Panel>();
    _passwordPanel->SetBaseSize(240, 140);
    _passwordPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _passwordPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _passwordPanel->SetCornerRounding(5.0f);
    _passwordPanel->SetProperty(mainPanelShadow);
    _passwordPanel->SetZIndex(2);
    _passwordPanel->SetVisible(false);

    _passwordLabel = Create<zcom::Label>(L"Password required");
    _passwordLabel->SetBaseSize(200, 30);
    _passwordLabel->SetOffsetPixels(20, 10);
    _passwordLabel->SetFontSize(24.0f);

    _passwordInput = Create<zcom::TextInput>();
    _passwordInput->SetBaseSize(200, 30);
    _passwordInput->SetOffsetPixels(20, 50);
    _passwordInput->SetCornerRounding(5.0f);

    _pwContinueButton = Create<zcom::Button>(L"Continue");
    _pwContinueButton->SetBaseSize(90, 30);
    _pwContinueButton->SetOffsetPixels(20, -20);
    _pwContinueButton->SetAlignment(zcom::Alignment::START, zcom::Alignment::END);
    _pwContinueButton->Text()->SetFontSize(18.0f);
    _pwContinueButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
    _pwContinueButton->SetTabIndex(3);
    _pwContinueButton->SetCornerRounding(5.0f);
    _pwContinueButton->SetProperty(buttonShadow);
    _pwContinueButton->SetOnActivated([&]() { _PwContinueClicked(); });

    _pwCancelButton = Create<zcom::Button>(L"Cancel");
    _pwCancelButton->SetBaseSize(90, 30);
    _pwCancelButton->SetOffsetPixels(-20, -20);
    _pwCancelButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _pwCancelButton->Text()->SetFontSize(18.0f);
    _pwCancelButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
    _pwCancelButton->SetTabIndex(3);
    _pwCancelButton->SetCornerRounding(5.0f);
    _pwCancelButton->SetProperty(buttonShadow);
    _pwCancelButton->SetOnActivated([&]() { _PwCancelClicked(); });

    _passwordPanel->AddItem(_passwordLabel.get());
    _passwordPanel->AddItem(_passwordInput.get());
    _passwordPanel->AddItem(_pwContinueButton.get());
    _passwordPanel->AddItem(_pwCancelButton.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->AddComponent(_dimPanel.get());
    _canvas->AddComponent(_passwordPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0, 0.2f));

    _connectionSuccessful = false;
    _closeScene = false;

    if (!opt.connectionString.empty())
        _ProcessConnectionString(opt.connectionString);
}

void ConnectScene::_Uninit()
{
    _canvas->ClearComponents();
    _mainPanel->ClearItems();
    _passwordPanel->ClearItems();
    _recentConnectionsPanel->ClearItems();

    _mainPanel = nullptr;
    _titleLabel = nullptr;
    _separator1 = nullptr;
    _ipLabel = nullptr;
    _ipInput = nullptr;
    _portLabel = nullptr;
    _portInput = nullptr;
    _recentConnectionsLabel = nullptr;
    _recentConnectionsPanel = nullptr;
    _noRecentConnectionsLabel = nullptr;
    _usernameLabel = nullptr;
    _usernameInput = nullptr;
    _separator2 = nullptr;
    _connectButton = nullptr;
    _cancelButton = nullptr;
    _connectLoadingImage = nullptr;
    _connectLoadingInfoLabel = nullptr;

    _dimPanel = nullptr;
    _passwordPanel = nullptr;
    _passwordLabel = nullptr;
    _passwordInput = nullptr;
    _pwContinueButton = nullptr;
    _pwCancelButton = nullptr;
}

void ConnectScene::_Focus()
{

}

void ConnectScene::_Unfocus()
{

}

void ConnectScene::_Update()
{
    _canvas->Update();

    // Show/hide loading image
    if (_connecting)
        _connectLoadingImage->SetVisible(true);
    else
        _connectLoadingImage->SetVisible(false);

    // Check connection status
    if (_connecting)
    {
        auto manager = APP_NETWORK->GetManager<znet::ClientManager>();
        if (manager)
        {
            if (!manager->Connecting())
            {
                if (manager->ConnectSuccessful())
                {
                    APP_NETWORK->StartManager();
                    _connectionSuccessful = true;
                    _closeScene = true;
                    _app->users.SetSelfUsername(_usernameInput->Text()->GetText());

                    // Save used username
                    Options::Instance()->SetValue(OPTIONS_LAST_USERNAME, _usernameInput->Text()->GetText());

                    if (!_owned)
                        _app->UninitScene(this->GetName());
                }
                else if (manager->PasswordRequired())
                {
                    APP_NETWORK->CloseManager();
                    _connectionSuccessful = false;
                    _connecting = false;

                    _OpenPasswordInput();

                    // Show incorrect password notification
                    if (_fromPasswordPanel)
                    {
                        zcom::NotificationInfo ninfo;
                        ninfo.title = L"Incorrect password";
                        ninfo.text = L"";
                        ninfo.borderColor = D2D1::ColorF(0.8f, 0.2f, 0.2f);
                        ninfo.duration = Duration(3, SECONDS);
                        _app->Overlay()->ShowNotification(ninfo);
                    }
                }
                else
                {
                    std::wostringstream ss;
                    ss << manager->FailMessage();
                    if (manager->FailCode() != -1)
                        ss << L"(Code: " << manager->FailCode() << ')';

                    APP_NETWORK->CloseManager();
                    _connectionSuccessful = false;
                    _connecting = false;
                    _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
                    _connectButton->Text()->SetText(L"Connect");
                    _connectLoadingInfoLabel->SetText(ss.str());
                    _connectLoadingInfoLabel->SetVisible(true);
                }
                _connecting = false;
            }
        }
        else
        {
            _connectLoadingInfoLabel->SetText(L"FATAL ERROR: network not in client mode");
            _connectLoadingInfoLabel->SetVisible(true);
        }

        //auto status = APP_NETWORK->ManagerStatus();
        //if (status == znet::NetworkStatus::ONLINE)
        //{
        //    APP_NETWORK->StartManager();
        //    _connectionSuccessful = true;
        //    _connecting = false;
        //    _closeScene = true;
        //    _app->users.SetSelfUsername(_usernameInput->GetText());
        //}
        //else if (status != znet::NetworkStatus::INITIALIZING)
        //{
        //    APP_NETWORK->CloseManager();
        //    _connectionSuccessful = false;
        //    _connecting = false;
        //    _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
        //    _connectButton->Text()->SetText(L"Connect");
        //    _connectLoadingInfoLabel->SetText(L"Connection unsuccessful..");
        //    _connectLoadingInfoLabel->SetVisible(true);
        //}
    }
}

void ConnectScene::_Resize(int width, int height)
{

}

bool ConnectScene::ConnectionSuccessful() const
{
    return _connectionSuccessful;
}

bool ConnectScene::CloseScene() const
{
    return _closeScene;
}

void ConnectScene::_ConnectClicked(std::wstring password)
{
    if (!_connecting)
    {
        _ipInput->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        _portInput->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        // Check if ip/port is valid
        std::wstring ip = _ipInput->Text()->GetText();
        std::wstring port = _portInput->Text()->GetText();
        bool addressValid = true;
        std::wstring errorMessage = L"Invalid";
        if (!LastIpOptionAdapter::ValidateIp(ip))
        {
            _ipInput->SetBorderColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
            if (!addressValid)
                errorMessage += L",";
            errorMessage += L" IP";
            addressValid = false;
        }
        if (!LastIpOptionAdapter::ValidatePort(port))
        {
            _portInput->SetBorderColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
            if (!addressValid)
                errorMessage += L",";
            errorMessage += L" port";
            addressValid = false;
        }

        if (!addressValid)
        {
            //_connectLoadingInfoLabel->SetText(errorMessage);
            _connectLoadingInfoLabel->SetText(L"Invalid input.");
            _connectLoadingInfoLabel->SetVisible(true);
        }
        else
        {
            _connecting = true;
            _fromPasswordPanel = false;
            _connectButton->SetBackgroundColor(D2D1::ColorF(0.5f, 0.25f, 0.25f));
            _connectButton->Text()->SetText(L"Cancel");
            _connectLoadingImage->ResetAnimation();
            _connectLoadingInfoLabel->SetVisible(false);
            _ip = wstring_to_string(ip);
            _port = str_to_int(wstring_to_string(port));
            APP_NETWORK->SetManager(std::make_unique<znet::ClientManager>(_ip, _port, wstring_to_string(password)));

            LastIpOptionAdapter optAdapter(Options::Instance()->GetValue(OPTIONS_RECENT_IPS));
            bool ipValid = optAdapter.AddIp(ip, port);
            Options::Instance()->SetValue(OPTIONS_RECENT_IPS, optAdapter.ToOptionString());
            _RearrangeRecentConnectionsPanel();
        }
    }
    else
    {
        _connecting = false;
        _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
        _connectButton->Text()->SetText(L"Connect");
    }
}

void ConnectScene::_CancelClicked()
{
    if (_connecting)
    {
        APP_NETWORK->CloseManager();
        _connecting = false;
    }
    _connectionSuccessful = false;
    _closeScene = true;

    if (!_owned)
        _app->UninitScene(this->GetName());
}

void ConnectScene::_PwContinueClicked()
{
    _connecting = true;
    _fromPasswordPanel = true;
    _connectButton->SetBackgroundColor(D2D1::ColorF(0.5f, 0.25f, 0.25f));
    _connectButton->Text()->SetText(L"Cancel");
    _connectLoadingImage->ResetAnimation();
    _connectLoadingInfoLabel->SetVisible(false);
    APP_NETWORK->SetManager(std::make_unique<znet::ClientManager>(_ip, _port, wstring_to_string(_passwordInput->Text()->GetText())));
    _ClosePasswordInput();
}

void ConnectScene::_PwCancelClicked()
{
    _ClosePasswordInput();
}

void ConnectScene::_OpenPasswordInput()
{
    _dimPanel->SetVisible(true);
    _passwordPanel->SetVisible(true);
}

void ConnectScene::_ClosePasswordInput()
{
    _dimPanel->SetVisible(false);
    _passwordPanel->SetVisible(false);
}

void ConnectScene::_ProcessConnectionString(std::wstring str)
{
    std::vector<std::wstring> urlParts;
    split_wstr(str, urlParts, L'/', true);

    if (urlParts.empty())
        return;
    // Get ip:port
    if (urlParts.size() >= 1)
    {
        std::array<std::wstring, 2> addrParts;
        split_wstr(urlParts[0], addrParts, L':');

        _ipInput->Text()->SetText(addrParts[0]);
        _portInput->Text()->SetText(addrParts[1]);
    }
    // Get password
    std::wstring password;
    if (urlParts.size() >= 2)
    {
        password = urlParts[1];
    }

    _ConnectClicked(password);
}

void ConnectScene::_RearrangeRecentConnectionsPanel()
{
    LastIpOptionAdapter optAdapter(Options::Instance()->GetValue(OPTIONS_RECENT_IPS));
    auto ipList = optAdapter.GetList();
    ID2D1Bitmap* removeIcon = ResourceManager::GetImage("delete_15x15");
    _recentConnectionsPanel->DeferLayoutUpdates();
    _recentConnectionsPanel->ClearItems();
    for (int i = 0; i < ipList.size(); i++)
    {
        std::wstring ip = ipList[i];
        auto ipButton = Create<zcom::Button>(ip);
        ipButton->SetParentWidthPercent(1.0f);
        ipButton->SetBaseSize(-23, 23);
        ipButton->SetVerticalOffsetPixels(i * 23);
        ipButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        ipButton->Text()->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        ipButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        ipButton->SetOnActivated([&, ip]()
        {
            std::array<std::wstring, 2> ipParts;
            split_wstr(ip, ipParts, ':');
            _ipInput->Text()->SetText(ipParts[0]);
            _portInput->Text()->SetText(ipParts[1]);
        });
        auto removeButton = Create<zcom::Button>();
        removeButton->SetBaseSize(23, 23);
        removeButton->SetVerticalOffsetPixels(i * 23);
        removeButton->SetHorizontalAlignment(zcom::Alignment::END);
        removeButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        removeButton->SetButtonImageAll(removeIcon);
        removeButton->ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
        removeButton->ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
        removeButton->UseImageParamsForAll(removeButton->ButtonImage());
        removeButton->SetOnActivated([&, ip]()
        {
            LastIpOptionAdapter _optAdapter(Options::Instance()->GetValue(OPTIONS_RECENT_IPS));
            if (_optAdapter.RemoveIp(ip))
            {
                Options::Instance()->SetValue(OPTIONS_RECENT_IPS, _optAdapter.ToOptionString());
                _RearrangeRecentConnectionsPanel();
            }
        });
        _recentConnectionsPanel->AddItem(ipButton.release(), true);
        _recentConnectionsPanel->AddItem(removeButton.release(), true);
    }
    _recentConnectionsPanel->ResumeLayoutUpdates();

    if (ipList.empty())
        _noRecentConnectionsLabel->SetVisible(true);
    else
        _noRecentConnectionsLabel->SetVisible(false);
}