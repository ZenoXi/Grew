#include "App.h" // App.h must be included first
#include "StartServerScene.h"

#include "Options.h"
#include "LastIpOptionAdapter.h"
#include "Network.h"
#include "ServerManager.h"

StartServerScene::StartServerScene()
{

}

void StartServerScene::_Init(const SceneOptionsBase* options)
{
    StartServerSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const StartServerSceneOptions*>(options);
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

    _titleLabel = std::make_unique<zcom::Label>(L"Start server");
    _titleLabel->SetBaseSize(400, 30);
    _titleLabel->SetOffsetPixels(30, 30);
    _titleLabel->SetFontSize(42.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _separatorTitle = std::make_unique<zcom::EmptyPanel>();
    _separatorTitle->SetBaseSize(440, 1);
    _separatorTitle->SetVerticalOffsetPixels(80);
    _separatorTitle->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorTitle->SetBorderVisibility(true);
    _separatorTitle->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _portLabel = std::make_unique<zcom::Label>(L"Port:");
    _portLabel->SetBaseSize(80, 30);
    _portLabel->SetOffsetPixels(30, 90);
    _portLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _portLabel->SetFontSize(18.0f);

    _portInput = std::make_unique<zcom::TextInput>();
    _portInput->SetBaseSize(60, 30);
    _portInput->SetOffsetPixels(120, 90);
    _portInput->SetCornerRounding(5.0f);
    _portInput->SetTabIndex(0);

    _presetDropdown = std::make_unique<zcom::DropdownSelection>(L"Presets", _canvas, L"No presets added");
    _presetDropdown->SetBaseSize(100, 30);
    _presetDropdown->SetOffsetPixels(-30, 90);
    _presetDropdown->SetHorizontalAlignment(zcom::Alignment::END);
    _presetDropdown->Text()->SetFontSize(18.0f);
    _presetDropdown->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
    _presetDropdown->SetCornerRounding(5.0f);
    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;
    _presetDropdown->SetProperty(buttonShadow);

    _separatorTop = std::make_unique<zcom::EmptyPanel>();
    _separatorTop->SetBaseSize(440, 1);
    _separatorTop->SetVerticalOffsetPixels(130);
    _separatorTop->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorTop->SetBorderVisibility(true);
    _separatorTop->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _usernameLabel = std::make_unique<zcom::Label>(L"Username:");
    _usernameLabel->SetBaseSize(80, 30);
    _usernameLabel->SetOffsetPixels(30, 140);
    _usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _usernameLabel->SetFontSize(18.0f);

    _usernameInput = std::make_unique<zcom::TextInput>();
    _usernameInput->SetBaseSize(240, 30);
    _usernameInput->SetOffsetPixels(120, 140);
    _usernameInput->SetCornerRounding(5.0f);
    _usernameInput->SetPlaceholderText(L"(Optional)");
    _usernameInput->SetTabIndex(2);

    _separatorBottom = std::make_unique<zcom::EmptyPanel>();
    _separatorBottom->SetBaseSize(440, 1);
    _separatorBottom->SetVerticalOffsetPixels(-80);
    _separatorBottom->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorBottom->SetVerticalAlignment(zcom::Alignment::END);
    _separatorBottom->SetBorderVisibility(true);
    _separatorBottom->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _startButton = std::make_unique<zcom::Button>(L"Start");
    _startButton->SetBaseSize(80, 30);
    _startButton->SetOffsetPixels(-30, -30);
    _startButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _startButton->Text()->SetFontSize(18.0f);
    _startButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
    _startButton->SetTabIndex(3);
    _startButton->SetCornerRounding(5.0f);
    _startButton->SetProperty(buttonShadow);
    _startButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _startButton->SetOnActivated([&]() { _StartClicked(); });

    _cancelButton = std::make_unique<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 30);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("item_delete"));
    _cancelButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });

    _startLoadingInfoLabel = std::make_unique<zcom::Label>(L"");
    _startLoadingInfoLabel->SetBaseSize(300, 30);
    _startLoadingInfoLabel->SetOffsetPixels(-130, -30);
    _startLoadingInfoLabel->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _startLoadingInfoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::TRAILING);
    _startLoadingInfoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _startLoadingInfoLabel->SetFontColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
    _startLoadingInfoLabel->SetFontSize(16.0f);
    _startLoadingInfoLabel->SetVisible(false);

    _mainPanel->AddItem(_titleLabel.get());
    _mainPanel->AddItem(_separatorTitle.get());
    _mainPanel->AddItem(_portLabel.get());
    _mainPanel->AddItem(_portInput.get());
    _mainPanel->AddItem(_presetDropdown.get());
    _mainPanel->AddItem(_separatorTop.get());
    _mainPanel->AddItem(_usernameLabel.get());
    _mainPanel->AddItem(_usernameInput.get());
    _mainPanel->AddItem(_separatorBottom.get());
    _mainPanel->AddItem(_startButton.get());
    _mainPanel->AddItem(_cancelButton.get());
    _mainPanel->AddItem(_startLoadingInfoLabel.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));

    _starting = false;
    _startSuccessful = false;
    _closeScene = false;
}

void StartServerScene::_Uninit()
{
    _mainPanel = nullptr;
    _titleLabel = nullptr;
    _separatorTitle = nullptr;
    _portLabel = nullptr;
    _portInput = nullptr;
    _presetDropdown = nullptr;
    _usernameLabel = nullptr;
    _usernameInput = nullptr;
    _separatorBottom = nullptr;
    _startButton = nullptr;
    _cancelButton = nullptr;
    _startLoadingInfoLabel = nullptr;
}

void StartServerScene::_Focus()
{

}

void StartServerScene::_Unfocus()
{

}

void StartServerScene::_Update()
{
    _canvas->Update();
}

ID2D1Bitmap1* StartServerScene::_Draw(Graphics g)
{
    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void StartServerScene::_Resize(int width, int height)
{

}

bool StartServerScene::StartSuccessful() const
{
    return _startSuccessful;
}

bool StartServerScene::CloseScene() const
{
    return _closeScene;
}

void StartServerScene::_StartClicked()
{
    if (!_starting)
    {
        _portInput->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        // Check if port is valid
        std::string port = wstring_to_string(_portInput->GetText());
        bool addressValid = true;
        if (!LastIpOptionAdapter::ValidatePort(port))
        {
            _portInput->SetBorderColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
            addressValid = false;
        }

        if (!addressValid)
        {
            _startLoadingInfoLabel->SetText(L"Invalid input.");
            _startLoadingInfoLabel->SetVisible(true);
        }
        else
        {
            _startLoadingInfoLabel->SetVisible(false);

            auto serverManager = std::make_unique<znet::ServerManager>(str_to_int(port));
            if (serverManager->Status() == znet::NetworkStatus::ONLINE)
            {
                APP_NETWORK->SetManager(std::move(serverManager));
                APP_NETWORK->SetUsername(_usernameInput->GetText());
                _starting = true;
                _startSuccessful = true;
                _closeScene = true;
            }
            else
            {
                _startLoadingInfoLabel->SetText(L"Server start failed");
                _startLoadingInfoLabel->SetVisible(true);
            }
        }
    }
}

void StartServerScene::_CancelClicked()
{
    _startSuccessful = false;
    _closeScene = true;
}