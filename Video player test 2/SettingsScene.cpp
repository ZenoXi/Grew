#include "App.h" // App.h must be included first
#include "SettingsScene.h"

#include "Options.h"

SettingsScene::SettingsScene(App* app)
    : Scene(app)
{

}

void SettingsScene::_Init(const SceneOptionsBase* options)
{
    SettingsSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const SettingsSceneOptions*>(options);
    }

    _changedSettings.clear();

    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(650, 500);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);

    _titleLabel = Create<zcom::Label>(L"Settings");
    _titleLabel->SetBaseSize(400, 40);
    _titleLabel->SetOffsetPixels(30, 15);
    _titleLabel->SetFontSize(36.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _cancelButton = Create<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 20);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("item_delete"));
    _cancelButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });

    _separatorTitle = Create<zcom::EmptyPanel>();
    _separatorTitle->SetBaseSize(-60, 1);
    _separatorTitle->SetParentWidthPercent(1.0f);
    _separatorTitle->SetVerticalOffsetPixels(70);
    _separatorTitle->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorTitle->SetBorderVisibility(true);
    _separatorTitle->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _horizontalSeparator = Create<zcom::EmptyPanel>();
    _horizontalSeparator->SetBaseSize(1, -70 -80);
    _horizontalSeparator->SetParentHeightPercent(1.0f);
    _horizontalSeparator->SetOffsetPixels(149 + 30, 70);
    _horizontalSeparator->SetBorderVisibility(true);
    _horizontalSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _horizontalSeparator->SetZIndex(0);

    _selectedTabSeparator = Create<zcom::EmptyPanel>();
    _selectedTabSeparator->SetBaseSize(2, 30);
    _selectedTabSeparator->SetOffsetPixels(150 + 30 - 2, 70);
    _selectedTabSeparator->SetBorderVisibility(true);
    _selectedTabSeparator->SetBorderColor(D2D1::ColorF(0.1176f * 0.7f, 0.5647f * 0.7f, 1.0f * 0.7f));
    //_selectedTabSeparator->SetBorderColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
    _selectedTabSeparator->SetZIndex(1);

    _sideMenuPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    _sideMenuPanel->SetBaseSize(150, -70 -80);
    _sideMenuPanel->SetParentHeightPercent(1.0f);
    _sideMenuPanel->SetOffsetPixels(30, 70);
    //_sideMenuPanel->SetBackgroundColor(D2D1::ColorF(0.17f, 0.17f, 0.17f));

    _mainSettingsButton = Create<zcom::Button>(L"Main");
    _mainSettingsButton->SetBaseHeight(30);
    _mainSettingsButton->SetParentWidthPercent(1.0f);
    _mainSettingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _mainSettingsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _mainSettingsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _mainSettingsButton->Text()->SetFontSize(16.0f);
    _mainSettingsButton->Text()->SetMargins({ 10.0f, 0.0f, 10.0f });
    _mainSettingsButton->Text()->SetCutoff(L"...");
    _mainSettingsButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
    _mainSettingsButton->SetActivation(zcom::ButtonActivation::PRESS);
    _mainSettingsButton->SetOnActivated([&]()
    {
        _SelectTab(SettingsSceneOptions::MAIN);
    });

    _playbackSettingsButton = Create<zcom::Button>(L"Playback");
    _playbackSettingsButton->SetBaseHeight(30);
    _playbackSettingsButton->SetParentWidthPercent(1.0f);
    _playbackSettingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _playbackSettingsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _playbackSettingsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _playbackSettingsButton->Text()->SetFontSize(16.0f);
    _playbackSettingsButton->Text()->SetMargins({ 10.0f, 0.0f, 10.0f });
    _playbackSettingsButton->Text()->SetCutoff(L"...");
    _playbackSettingsButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
    _playbackSettingsButton->SetActivation(zcom::ButtonActivation::PRESS);
    _playbackSettingsButton->SetOnActivated([&]()
    {
        _SelectTab(SettingsSceneOptions::PLAYBACK);
    });

    _networkSettingsButton = Create<zcom::Button>(L"Network");
    _networkSettingsButton->SetBaseHeight(30);
    _networkSettingsButton->SetParentWidthPercent(1.0f);
    _networkSettingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _networkSettingsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _networkSettingsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _networkSettingsButton->Text()->SetFontSize(16.0f);
    _networkSettingsButton->Text()->SetMargins({ 10.0f, 0.0f, 10.0f });
    _networkSettingsButton->Text()->SetCutoff(L"...");
    _networkSettingsButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
    _networkSettingsButton->SetActivation(zcom::ButtonActivation::PRESS);
    _networkSettingsButton->SetOnActivated([&]()
    {
        _SelectTab(SettingsSceneOptions::NETWORK);
    });

    _advancedSettingsButton = Create<zcom::Button>(L"Advanced");
    _advancedSettingsButton->SetBaseHeight(30);
    _advancedSettingsButton->SetParentWidthPercent(1.0f);
    _advancedSettingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _advancedSettingsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _advancedSettingsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _advancedSettingsButton->Text()->SetFontSize(16.0f);
    _advancedSettingsButton->Text()->SetMargins({ 10.0f, 0.0f, 10.0f });
    _advancedSettingsButton->Text()->SetCutoff(L"...");
    _advancedSettingsButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
    _advancedSettingsButton->SetActivation(zcom::ButtonActivation::PRESS);
    _advancedSettingsButton->SetOnActivated([&]()
    {
        _SelectTab(SettingsSceneOptions::ADVANCED);
    });

    _keybindSettingsButton = Create<zcom::Button>(L"Keybinds");
    _keybindSettingsButton->SetBaseHeight(30);
    _keybindSettingsButton->SetParentWidthPercent(1.0f);
    _keybindSettingsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
    _keybindSettingsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _keybindSettingsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _keybindSettingsButton->Text()->SetFontSize(16.0f);
    _keybindSettingsButton->Text()->SetMargins({ 10.0f, 0.0f, 10.0f });
    _keybindSettingsButton->Text()->SetCutoff(L"...");
    _keybindSettingsButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
    _keybindSettingsButton->SetActivation(zcom::ButtonActivation::PRESS);
    _keybindSettingsButton->SetOnActivated([&]()
    {
        _SelectTab(SettingsSceneOptions::KEYBINDS);
    });

    _sideMenuPanel->AddItem(_mainSettingsButton.get());
    _sideMenuPanel->AddItem(_playbackSettingsButton.get());
    _sideMenuPanel->AddItem(_networkSettingsButton.get());
    _sideMenuPanel->AddItem(_advancedSettingsButton.get());
    _sideMenuPanel->AddItem(_keybindSettingsButton.get());
    _tabButtons.push_back({ SettingsSceneOptions::MAIN, _mainSettingsButton.get() });
    _tabButtons.push_back({ SettingsSceneOptions::PLAYBACK, _playbackSettingsButton.get() });
    _tabButtons.push_back({ SettingsSceneOptions::NETWORK, _networkSettingsButton.get() });
    _tabButtons.push_back({ SettingsSceneOptions::ADVANCED, _advancedSettingsButton.get() });
    _tabButtons.push_back({ SettingsSceneOptions::KEYBINDS, _keybindSettingsButton.get() });

    _settingsPanel = Create<zcom::Panel>();
    _settingsPanel->SetBaseSize(-150 -60, -70 -80);
    _settingsPanel->SetParentSizePercent(1.0f, 1.0f);
    _settingsPanel->SetOffsetPixels(150 + 30, 70);
    _settingsPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);
    //_settingsPanel->SetBackgroundColor(D2D1::ColorF(0.17f, 0.17f, 0.17f));

    _separatorBottom = Create<zcom::EmptyPanel>();
    _separatorBottom->SetBaseSize(-60, 1);
    _separatorBottom->SetParentWidthPercent(1.0f);
    _separatorBottom->SetVerticalOffsetPixels(-80);
    _separatorBottom->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorBottom->SetVerticalAlignment(zcom::Alignment::END);
    _separatorBottom->SetBorderVisibility(true);
    _separatorBottom->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _separatorBottom->SetZIndex(0);

    _saveButton = Create<zcom::Button>(L"Save");
    _saveButton->SetBaseSize(80, 30);
    _saveButton->SetOffsetPixels(-30, -30);
    _saveButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _saveButton->Text()->SetFontSize(18.0f);
    _saveButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
    _saveButton->SetTabIndex(3);
    _saveButton->SetCornerRounding(5.0f);
    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;
    _saveButton->SetProperty(buttonShadow);
    _saveButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _saveButton->SetOnActivated([&]() { _SaveClicked(); });

    _mainPanel->AddItem(_titleLabel.get());
    _mainPanel->AddItem(_cancelButton.get());
    //_mainPanel->AddItem(_separatorTitle.get());
    _mainPanel->AddItem(_sideMenuPanel.get());
    _mainPanel->AddItem(_horizontalSeparator.get());
    _mainPanel->AddItem(_selectedTabSeparator.get());
    _mainPanel->AddItem(_settingsPanel.get());
    //_mainPanel->AddItem(_separatorBottom.get());
    _mainPanel->AddItem(_saveButton.get());

    _SelectTab(SettingsSceneOptions::MAIN);

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));
}

void SettingsScene::_Uninit()
{
    _canvas->ClearComponents();
    _mainPanel->ClearItems();
    _sideMenuPanel->ClearItems();
    _settingsPanel->ClearItems();
    //_mainSettingsPanel->ClearItems();
    //_playbackSettingsPanel->ClearItems();
    //_networkSettingsPanel->ClearItems();
    //_advancedSettingsPanel->ClearItems();
    //_keybindSettingsPanel->ClearItems();

    _mainPanel = nullptr;
    _titleLabel = nullptr;
    _cancelButton = nullptr;
    _separatorTitle = nullptr;

    _tabButtons.clear();
    _sideMenuPanel = nullptr;
    _mainSettingsButton = nullptr;
    _playbackSettingsButton = nullptr;
    _networkSettingsButton = nullptr;
    _advancedSettingsButton = nullptr;
    _keybindSettingsButton = nullptr;

    _horizontalSeparator = nullptr;
    _selectedTabSeparator = nullptr;

    _settingTabs.clear();
    _settingsPanel = nullptr;
    //_mainSettingsPanel = nullptr;
    //_playbackSettingsPanel = nullptr;
    //_networkSettingsPanel = nullptr;
    //_advancedSettingsPanel = nullptr;
    //_keybindSettingsPanel = nullptr;

    _separatorBottom = nullptr;
    _saveButton = nullptr;
}

void SettingsScene::_Focus()
{

}

void SettingsScene::_Unfocus()
{

}

void SettingsScene::_Update()
{
    _canvas->Update();

    if (_changedSettings.empty())
        _saveButton->SetActive(false);
    else
        _saveButton->SetActive(true);
}

void SettingsScene::_Resize(int width, int height)
{

}

void SettingsScene::_SelectTab(SettingsSceneOptions::SettingsTab tab)
{
    _ResetTabButtons();
    switch (tab)
    {
    case SettingsSceneOptions::MAIN:
    {
        _ShowMainSettingsTab();
        break;
    }
    case SettingsSceneOptions::PLAYBACK:
    {
        _ShowPlaybackSettingsTab();
        break;
    }
    case SettingsSceneOptions::NETWORK:
    {
        _ShowNetworkSettingsTab();
        break;
    }
    case SettingsSceneOptions::ADVANCED:
    {
        _ShowAdvancedSettingsTab();
        break;
    }
    case SettingsSceneOptions::KEYBINDS:
    {
        _ShowKeybindSettingsTab();
        break;
    }
    }
}

void SettingsScene::_ResetTabButtons()
{
    for (int i = 0; i < _tabButtons.size(); i++)
    {
        if (i % 2 == 0)
            _tabButtons[i].second->SetButtonColor(D2D1::ColorF(D2D1::ColorF::White, 0.03f));
        else
            _tabButtons[i].second->SetButtonColor(D2D1::ColorF(0, 0.0f));

        _tabButtons[i].second->SetButtonHoverColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
        _tabButtons[i].second->SetButtonClickColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    }
}

std::wstring SettingsScene::_LoadSavedOption(std::wstring name)
{
    if (_changedSettings.find(name) != _changedSettings.end())
        return _changedSettings[name];
    else
        return Options::Instance()->GetValue(name);
}

void SettingsScene::_SaveClicked()
{
    for (auto& pair : _changedSettings)
    {
        Options::Instance()->SetValue(pair.first, pair.second);
    }
    _changedSettings.clear();
}

void SettingsScene::_CancelClicked()
{
    _app->UninitScene(GetName());
}