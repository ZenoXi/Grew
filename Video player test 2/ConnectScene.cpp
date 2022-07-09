#include "App.h" // App.h must be included first
#include "ConnectScene.h"

#include "Options.h"
#include "LastIpOptionAdapter.h"
#include "Network.h"
#include "ClientManager.h"

ConnectScene::ConnectScene()
{

}

void ConnectScene::_Init(const SceneOptionsBase* options)
{
    ConnectSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const ConnectSceneOptions*>(options);
    }

    _mainPanel = std::make_unique<zcom::Panel>();
    _mainPanel->SetBaseSize(500, 500);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);

    _titleLabel = std::make_unique<zcom::Label>(L"Connect");
    _titleLabel->SetBaseSize(400, 30);
    _titleLabel->SetOffsetPixels(30, 30);
    _titleLabel->SetFontSize(42.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _separator1 = std::make_unique<zcom::EmptyPanel>();
    _separator1->SetBaseSize(440, 1);
    _separator1->SetVerticalOffsetPixels(80);
    _separator1->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separator1->SetBorderVisibility(true);
    _separator1->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _ipLabel = std::make_unique<zcom::Label>(L"Server IP:");
    _ipLabel->SetBaseSize(80, 30);
    _ipLabel->SetOffsetPixels(30, 100);
    _ipLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _ipLabel->SetFontSize(18.0f);

    _ipInput = std::make_unique<zcom::TextInput>();
    _ipInput->SetBaseSize(120, 30);
    _ipInput->SetOffsetPixels(120, 100);
    _ipInput->SetCornerRounding(5.0f);
    _ipInput->SetTabIndex(0);

    _portLabel = std::make_unique<zcom::Label>(L"Port:");
    _portLabel->SetBaseSize(40, 30);
    _portLabel->SetOffsetPixels(250, 100);
    _portLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _portLabel->SetFontSize(18.0f);

    _portInput = std::make_unique<zcom::TextInput>();
    _portInput->SetBaseSize(60, 30);
    _portInput->SetOffsetPixels(300, 100);
    _portInput->SetCornerRounding(5.0f);
    _portInput->SetTabIndex(1);

    _recentConnectionsLabel = std::make_unique<zcom::Label>(L"Recent:");
    _recentConnectionsLabel->SetBaseSize(80, 30);
    _recentConnectionsLabel->SetOffsetPixels(30, 140);
    _recentConnectionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _recentConnectionsLabel->SetFontSize(18.0f);

    _noRecentConnectionsLabel = std::make_unique<zcom::Label>(L"No saved connections");
    _noRecentConnectionsLabel->SetBaseSize(200, 20);
    _noRecentConnectionsLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _noRecentConnectionsLabel->SetFontSize(16.0f);
    _noRecentConnectionsLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _noRecentConnectionsLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));
    _noRecentConnectionsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _noRecentConnectionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _recentConnectionsPanel = std::make_unique<zcom::Panel>();
    _recentConnectionsPanel->SetBaseSize(240, 100);
    _recentConnectionsPanel->SetOffsetPixels(120, 140);
    _recentConnectionsPanel->SetBorderVisibility(true);
    _recentConnectionsPanel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _recentConnectionsPanel->SetBackgroundColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _recentConnectionsPanel->SetCornerRounding(5.0f);
    _recentConnectionsPanel->VerticalScrollable(true);
    _recentConnectionsPanel->SetTabIndex(-1);
    _RearrangeRecentConnectionsPanel();

    _usernameLabel = std::make_unique<zcom::Label>(L"Username:");
    _usernameLabel->SetBaseSize(80, 30);
    _usernameLabel->SetOffsetPixels(30, 250);
    _usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _usernameLabel->SetFontSize(18.0f);

    _usernameInput = std::make_unique<zcom::TextInput>();
    _usernameInput->SetBaseSize(240, 30);
    _usernameInput->SetOffsetPixels(120, 250);
    _usernameInput->SetCornerRounding(5.0f);
    _usernameInput->SetPlaceholderText(L"(Optional)");
    _usernameInput->SetTabIndex(2);

    _separator2 = std::make_unique<zcom::EmptyPanel>();
    _separator2->SetBaseSize(440, 1);
    _separator2->SetVerticalOffsetPixels(-80);
    _separator2->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separator2->SetVerticalAlignment(zcom::Alignment::END);
    _separator2->SetBorderVisibility(true);
    _separator2->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _connectButton = std::make_unique<zcom::Button>(L"Connect");
    _connectButton->SetBaseSize(80, 30);
    _connectButton->SetOffsetPixels(-30, -30);
    _connectButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _connectButton->Text()->SetFontSize(18.0f);
    _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
    //_connectButton->SetSelectedBorderColor(D2D1::ColorF(0, 0));
    _connectButton->SetTabIndex(3);
    _connectButton->SetCornerRounding(5.0f);
    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;
    _connectButton->SetProperty(buttonShadow);
    _connectButton->SetOnActivated([&]() { _ConnectClicked(); });

    _cancelButton = std::make_unique<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 30);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("item_delete"));
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });

    _connectLoadingImage = std::make_unique<zcom::LoadingImage>();
    _connectLoadingImage->SetBaseSize(30, 30);
    _connectLoadingImage->SetOffsetPixels(-130, -30);
    _connectLoadingImage->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _connectLoadingImage->SetDotHeight(10.0f);
    _connectLoadingImage->SetDotRadius(2.5f);
    _connectLoadingImage->SetDotSpacing(8.0f);
    _connectLoadingImage->SetDotColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
    _connectLoadingImage->SetVisible(false);

    _connectLoadingInfoLabel = std::make_unique<zcom::Label>(L"");
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

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));

    _connectionSuccessful = false;
    _closeScene = false;
}

void ConnectScene::_Uninit()
{
    _canvas->ClearComponents();

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
        auto status = APP_NETWORK->ManagerStatus();
        if (status == znet::NetworkStatus::ONLINE)
        {
            _connectionSuccessful = true;
            _connecting = false;
            _closeScene = true;
            APP_NETWORK->SetUsername(_usernameInput->GetText());
        }
        else if (status != znet::NetworkStatus::INITIALIZING)
        {
            _connectionSuccessful = false;
            _connecting = false;
            _connectButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
            _connectButton->Text()->SetText(L"Connect");
            _connectLoadingInfoLabel->SetText(L"Connection unsuccessful..");
            _connectLoadingInfoLabel->SetVisible(true);
        }
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

void ConnectScene::_ConnectClicked()
{
    if (!_connecting)
    {
        _ipInput->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        _portInput->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        // Check if ip/port is valid
        std::string ip = wstring_to_string(_ipInput->GetText());
        std::string port = wstring_to_string(_portInput->GetText());
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
            _connectButton->SetBackgroundColor(D2D1::ColorF(0.5f, 0.25f, 0.25f));
            _connectButton->Text()->SetText(L"Cancel");
            _connectLoadingImage->ResetAnimation();
            _connectLoadingInfoLabel->SetVisible(false);
            APP_NETWORK->SetManager(std::make_unique<znet::ClientManager>(ip, str_to_int(port)));

            LastIpOptionAdapter optAdapter(Options::Instance()->GetValue("lastIps"));
            bool ipValid = optAdapter.AddIp(ip, port);
            Options::Instance()->SetValue("lastIps", optAdapter.ToOptionString());
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
}

void ConnectScene::_RearrangeRecentConnectionsPanel()
{
    LastIpOptionAdapter optAdapter(Options::Instance()->GetValue("lastIps"));
    auto ipList = optAdapter.GetList();
    ID2D1Bitmap* removeIcon = ResourceManager::GetImage("item_delete");
    _recentConnectionsPanel->DeferLayoutUpdates();
    _recentConnectionsPanel->ClearItems();
    for (int i = 0; i < ipList.size(); i++)
    {
        std::string ip = ipList[i];
        zcom::Button* ipButton = new zcom::Button(string_to_wstring(ip));
        ipButton->SetParentWidthPercent(1.0f);
        ipButton->SetBaseSize(-22, 22);
        ipButton->SetVerticalOffsetPixels(i * 22);
        ipButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        ipButton->Text()->SetMargins({ 5.0f, 0.0f, 5.0f, 0.0f });
        ipButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        ipButton->SetOnActivated([&, ip]()
        {
            std::array<std::string, 2> ipParts;
            split_str(ip, ipParts, ':');
            _ipInput->SetText(string_to_wstring(ipParts[0]));
            _portInput->SetText(string_to_wstring(ipParts[1]));
        });
        zcom::Button* removeButton = new zcom::Button();
        removeButton->SetBaseSize(22, 22);
        removeButton->SetVerticalOffsetPixels(i * 22);
        removeButton->SetHorizontalAlignment(zcom::Alignment::END);
        removeButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        removeButton->SetBackgroundImage(removeIcon);
        removeButton->SetOnActivated([&, ip]()
        {
            LastIpOptionAdapter _optAdapter(Options::Instance()->GetValue("lastIps"));
            if (_optAdapter.RemoveIp(ip))
            {
                Options::Instance()->SetValue("lastIps", _optAdapter.ToOptionString());
                _RearrangeRecentConnectionsPanel();
            }
        });
        _recentConnectionsPanel->AddItem(ipButton, true);
        _recentConnectionsPanel->AddItem(removeButton, true);
    }
    _recentConnectionsPanel->ResumeLayoutUpdates();

    if (ipList.empty())
        _noRecentConnectionsLabel->SetVisible(true);
    else
        _noRecentConnectionsLabel->SetVisible(false);
}