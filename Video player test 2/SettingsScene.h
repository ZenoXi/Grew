#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "EmptyPanel.h"
#include "Button.h"
#include "DropdownSelection.h"
#include "TextInput.h"
#include "Label.h"
#include "LoadingImage.h"
#include "Image.h"
#include "DirectionalPanel.h"
#include "Checkbox.h"

#include "FileDialog.h"

struct SettingsSceneOptions : public SceneOptionsBase
{
    enum SettingsTab
    {
        MAIN,
        PLAYBACK,
        NETWORK,
        ADVANCED,
        KEYBINDS
    };

    SettingsTab initialTab = MAIN;
};

class SettingsScene : public Scene
{
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Label> _titleLabel = nullptr;
    std::unique_ptr<zcom::Button> _cancelButton = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _separatorTitle = nullptr;

    std::vector<std::pair<SettingsSceneOptions::SettingsTab, zcom::Button*>> _tabButtons;
    std::unique_ptr<zcom::DirectionalPanel> _sideMenuPanel = nullptr;
    std::unique_ptr<zcom::Button> _mainSettingsButton = nullptr;
    std::unique_ptr<zcom::Button> _playbackSettingsButton = nullptr;
    std::unique_ptr<zcom::Button> _networkSettingsButton = nullptr;
    std::unique_ptr<zcom::Button> _advancedSettingsButton = nullptr;
    std::unique_ptr<zcom::Button> _keybindSettingsButton = nullptr;

    std::unique_ptr<zcom::EmptyPanel> _horizontalSeparator = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _selectedTabSeparator = nullptr;

    std::vector<std::pair<SettingsSceneOptions::SettingsTab, zcom::Panel*>> _settingTabs;
    std::unique_ptr<zcom::Panel> _settingsPanel = nullptr;
    //std::unique_ptr<zcom::Panel> _mainSettingsPanel = nullptr;
    //std::unique_ptr<zcom::Panel> _playbackSettingsPanel = nullptr;
    //std::unique_ptr<zcom::Panel> _networkSettingsPanel = nullptr;
    //std::unique_ptr<zcom::Panel> _advancedSettingsPanel = nullptr;
    //std::unique_ptr<zcom::Panel> _keybindSettingsPanel = nullptr;

    std::unique_ptr<zcom::EmptyPanel> _separatorBottom = nullptr;
    std::unique_ptr<zcom::Button> _saveButton = nullptr;

    std::unordered_map<std::wstring, std::wstring> _changedSettings;

public:
    SettingsScene(App* app);

    const char* GetName() const { return "settings"; }
    static const char* StaticName() { return "settings"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

    void _SelectTab(SettingsSceneOptions::SettingsTab tab);
    void _ResetTabButtons();
    void _ShowMainSettingsTab();
    void _ShowPlaybackSettingsTab();
    void _ShowNetworkSettingsTab();
    void _ShowAdvancedSettingsTab();
    void _ShowKeybindSettingsTab();

    std::wstring _LoadSavedOption(std::wstring name);

    void _SaveClicked();
    void _CancelClicked();
};