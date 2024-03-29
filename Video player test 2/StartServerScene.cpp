#include "App.h" // App.h must be included first
#include "StartServerScene.h"

#include "Permissions.h"
#include "Options.h"
#include "OptionNames.h"
#include "LastIpOptionAdapter.h"
#include "Network.h"
#include "ServerManager.h"

StartServerScene::StartServerScene(App* app)
    : Scene(app)
{

}

void StartServerScene::_Init(const SceneOptionsBase* options)
{
    StartServerSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const StartServerSceneOptions*>(options);
    }


    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(500, 500);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);

    _titleLabel = Create<zcom::Label>(L"Start server");
    _titleLabel->SetBaseSize(400, 30);
    _titleLabel->SetOffsetPixels(30, 30);
    _titleLabel->SetFontSize(42.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _separatorTitle = Create<zcom::EmptyPanel>();
    _separatorTitle->SetBaseSize(440, 1);
    _separatorTitle->SetVerticalOffsetPixels(80);
    _separatorTitle->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorTitle->SetBorderVisibility(true);
    _separatorTitle->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _portLabel = Create<zcom::Label>(L"Port:");
    _portLabel->SetBaseSize(80, 30);
    _portLabel->SetOffsetPixels(30, 90);
    _portLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _portLabel->SetFontSize(18.0f);
    _portLabel->SetMargins({ 2.0f });
    _portLabel->SetTextSelectable(true);

    _portInput = Create<zcom::TextInput>();
    _portInput->SetBaseSize(60, 30);
    _portInput->SetOffsetPixels(120, 90);
    _portInput->SetCornerRounding(5.0f);
    _portInput->SetTabIndex(0);
    _portInput->SetPattern(
        L"^[1-9]"
        "|[1-9][0-9]"
        "|[1-9][0-9][0-9]"
        "|[1-9][0-9][0-9][0-9]"
        "|[1-5][0-9][0-9][0-9][0-9]"
        "|6[0-4][0-9][0-9][0-9]"
        "|65[0-4][0-9][0-9]"
        "|655[0-2][0-9]"
        "|6553[0-5]$"
    );
    _portInput->OnSelected();

    // Get recent/default port
    std::wstring defaultPort = Options::Instance()->GetValue(OPTIONS_DEFAULT_PORT);
    if (!defaultPort.empty())
    {
        _portInput->Text()->SetText(defaultPort);
    }
    else
    {
        std::wstring recentPort = Options::Instance()->GetValue(OPTIONS_LAST_PORT);
        if (!recentPort.empty())
        {
            _portInput->Text()->SetText(recentPort);
        }
    }

    _presetDropdown = Create<zcom::DropdownSelection>(L"Presets", L"No presets added");
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

    _separatorTop = Create<zcom::EmptyPanel>();
    _separatorTop->SetBaseSize(440, 1);
    _separatorTop->SetVerticalOffsetPixels(130);
    _separatorTop->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorTop->SetBorderVisibility(true);
    _separatorTop->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _separatorTop->SetZIndex(0);

    _usernameLabel = Create<zcom::Label>(L"Username:");
    _usernameLabel->SetBaseSize(80, 30);
    _usernameLabel->SetOffsetPixels(30, 140);
    _usernameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _usernameLabel->SetFontSize(18.0f);

    _usernameInput = Create<zcom::TextInput>();
    _usernameInput->SetBaseSize(240, 30);
    _usernameInput->SetOffsetPixels(120, 140);
    _usernameInput->SetCornerRounding(5.0f);
    _usernameInput->PlaceholderText()->SetText(L"(Optional)");

    _advancedOptionsPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    _advancedOptionsPanel->SetBaseSize(-60, -212);
    _advancedOptionsPanel->SetParentSizePercent(1.0f, 1.0f);
    _advancedOptionsPanel->SetOffsetPixels(30, 131);
    _advancedOptionsPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);
    _advancedOptionsPanel->ScrollBackgroundColor(D2D1::ColorF(0, 0.25f));
    _advancedOptionsPanel->SetTabIndex(1);
    //_advancedOptionsPanel->ScrollbarHangDuration(zcom::Scrollbar::VERTICAL, 0);

    _advancedOptionsLabel = Create<zcom::Label>(L"Advanced options:");
    _advancedOptionsLabel->SetBaseSize(200, 30);
    _advancedOptionsLabel->SetFontSize(18.0f);
    _advancedOptionsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
    _advancedOptionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _advancedOptionsLabel->SetMargins({ 2.0f });

    // General options
    {
        _generalOptionsOpen = false;
        _generalOptionsPanel = Create<zcom::Panel>();
        _generalOptionsPanel->SetBaseHeight(30);
        _generalOptionsPanel->SetParentWidthPercent(1.0f);

        _generalOptionsArrowImage = Create<zcom::Image>();
        _generalOptionsArrowImage->SetBaseSize(25, 25);
        _generalOptionsArrowImage->SetVerticalOffsetPixels(3);
        _generalOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_right_7x7"));
        _generalOptionsArrowImage->SetPlacement(zcom::ImagePlacement::CENTER);
        _generalOptionsArrowImage->SetTintColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));

        _generalOptionsButton = Create<zcom::Button>(L"General");
        _generalOptionsButton->SetBaseHeight(30);
        _generalOptionsButton->SetParentWidthPercent(1.0f);
        _generalOptionsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
        _generalOptionsButton->SetButtonHoverColor(D2D1::ColorF(0, 0.2f));
        _generalOptionsButton->SetButtonClickColor(D2D1::ColorF(0, 0.4f));
        _generalOptionsButton->Text()->SetFontSize(18.0f);
        _generalOptionsButton->Text()->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
        _generalOptionsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        _generalOptionsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _generalOptionsButton->Text()->SetMargins({ 25.0f });
        _generalOptionsButton->SetActivation(zcom::ButtonActivation::PRESS);
        _generalOptionsButton->SetOnActivated([&]()
        {
            if (!_generalOptionsOpen)
            {
                _generalOptionsPanel->SetBaseHeight(_generalOptionsPanel->GetContentHeight() + 10);
                _generalOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_down_7x7"));
                _generalOptionsOpen = true;
            }
            else
            {
                _generalOptionsPanel->SetBaseHeight(30);
                _generalOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_right_7x7"));
                _generalOptionsOpen = false;
            }
            _UpdateTabIndices();
        });
        _generalOptionsButton->SetTabIndex(0);

        float textWidth = _generalOptionsButton->Text()->GetTextWidth();
        _generalOptionsOpenSeparator = Create<zcom::EmptyPanel>();
        _generalOptionsOpenSeparator->SetParentWidthPercent(1.0f);
        _generalOptionsOpenSeparator->SetBaseSize(-textWidth - 10 - 10, 1);
        _generalOptionsOpenSeparator->SetOffsetPixels(-10, 15);
        _generalOptionsOpenSeparator->SetHorizontalAlignment(zcom::Alignment::END);
        _generalOptionsOpenSeparator->SetBorderVisibility(true);
        _generalOptionsOpenSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        _generalOptionsPanel->AddItem(_generalOptionsButton.get());
        _generalOptionsPanel->AddItem(_generalOptionsArrowImage.get());
        _generalOptionsPanel->AddItem(_generalOptionsOpenSeparator.get());

        _passwordLabel = Create<zcom::Label>(L"Password (optional):");
        _passwordLabel->SetBaseSize(200, 30);
        _passwordLabel->SetOffsetPixels(15, 40);
        _passwordLabel->SetFontSize(16.0f);
        _passwordLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _passwordLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _passwordInput = Create<zcom::TextInput>();
        _passwordInput->SetBaseSize(200, 26);
        _passwordInput->SetOffsetPixels(15, 68);
        _passwordInput->SetCornerRounding(5.0f);

        _maxUsersLabel = Create<zcom::Label>(L"Max users:");
        _maxUsersLabel->SetBaseSize(100, 30);
        _maxUsersLabel->SetOffsetPixels(15, 100);
        _maxUsersLabel->SetFontSize(16.0f);
        _maxUsersLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _maxUsersLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _maxUsersInput = Create<zcom::TextInput>();
        _maxUsersInput->SetBaseSize(50, 26);
        _maxUsersInput->SetOffsetPixels(15, 128);
        _maxUsersInput->SetCornerRounding(5.0f);

        _chatEnabledCheckbox = Create<zcom::Checkbox>();
        _chatEnabledCheckbox->SetBaseSize(20, 20);
        _chatEnabledCheckbox->SetOffsetPixels(15, 165);
        _chatEnabledCheckbox->Checked(true);
        _chatEnabledCheckbox->SetActive(false);

        _chatEnabledLabel = Create<zcom::Label>(L"Chat enabled");
        _chatEnabledLabel->SetBaseSize(100, 30);
        _chatEnabledLabel->SetOffsetPixels(45, 160);
        _chatEnabledLabel->SetFontSize(16.0f);
        _chatEnabledLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _chatEnabledLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        _chatEnabledLabel->SetActive(false);

        _generalOptionsPanel->AddItem(_passwordLabel.get());
        _generalOptionsPanel->AddItem(_passwordInput.get());
        _generalOptionsPanel->AddItem(_maxUsersLabel.get());
        _generalOptionsPanel->AddItem(_maxUsersInput.get());
        _generalOptionsPanel->AddItem(_chatEnabledCheckbox.get());
        _generalOptionsPanel->AddItem(_chatEnabledLabel.get());
    }
    
    // Playlist options
    {
        _playlistOptionsOpen = false;
        _playlistOptionsPanel = Create<zcom::Panel>();
        _playlistOptionsPanel->SetBaseHeight(30);
        _playlistOptionsPanel->SetParentWidthPercent(1.0f);

        _playlistOptionsArrowImage = Create<zcom::Image>();
        _playlistOptionsArrowImage->SetBaseSize(25, 25);
        _playlistOptionsArrowImage->SetVerticalOffsetPixels(3);
        _playlistOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_right_7x7"));
        _playlistOptionsArrowImage->SetPlacement(zcom::ImagePlacement::CENTER);
        _playlistOptionsArrowImage->SetTintColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));

        _playlistOptionsButton = Create<zcom::Button>(L"Playlist");
        _playlistOptionsButton->SetBaseHeight(30);
        _playlistOptionsButton->SetParentWidthPercent(1.0f);
        _playlistOptionsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
        _playlistOptionsButton->SetButtonHoverColor(D2D1::ColorF(0, 0.2f));
        _playlistOptionsButton->SetButtonClickColor(D2D1::ColorF(0, 0.4f));
        _playlistOptionsButton->Text()->SetFontSize(18.0f);
        _playlistOptionsButton->Text()->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
        _playlistOptionsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        _playlistOptionsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _playlistOptionsButton->Text()->SetMargins({ 25.0f });
        _playlistOptionsButton->SetActivation(zcom::ButtonActivation::PRESS);
        _playlistOptionsButton->SetOnActivated([&]()
        {
            if (!_playlistOptionsOpen)
            {
                _playlistOptionsPanel->SetBaseHeight(_playlistOptionsPanel->GetContentHeight() + 10);
                _playlistOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_down_7x7"));
                _playlistOptionsOpen = true;
            }
            else
            {
                _playlistOptionsPanel->SetBaseHeight(30);
                _playlistOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_right_7x7"));
                _playlistOptionsOpen = false;
            }
            _UpdateTabIndices();
        });
        _playlistOptionsButton->SetTabIndex(0);

        float textWidth = _playlistOptionsButton->Text()->GetTextWidth();
        _playlistOptionsOpenSeparator = Create<zcom::EmptyPanel>();
        _playlistOptionsOpenSeparator->SetParentWidthPercent(1.0f);
        _playlistOptionsOpenSeparator->SetBaseSize(-textWidth - 10 - 10, 1);
        _playlistOptionsOpenSeparator->SetOffsetPixels(-10, 15);
        _playlistOptionsOpenSeparator->SetHorizontalAlignment(zcom::Alignment::END);
        _playlistOptionsOpenSeparator->SetBorderVisibility(true);
        _playlistOptionsOpenSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        _playlistOptionsPanel->AddItem(_playlistOptionsButton.get());
        _playlistOptionsPanel->AddItem(_playlistOptionsArrowImage.get());
        _playlistOptionsPanel->AddItem(_playlistOptionsOpenSeparator.get());

        _playlistUserPermissionsLabel = Create<zcom::Label>(L"Allow users to:");
        _playlistUserPermissionsLabel->SetBaseSize(200, 30);
        _playlistUserPermissionsLabel->SetOffsetPixels(15, 40);
        _playlistUserPermissionsLabel->SetFontSize(16.0f);
        _playlistUserPermissionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _playlistUserPermissionsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _allowItemAddingCheckbox = Create<zcom::Checkbox>();
        _allowItemAddingCheckbox->SetBaseSize(20, 20);
        _allowItemAddingCheckbox->SetOffsetPixels(15, 75);
        _allowItemAddingCheckbox->Checked(true);

        _allowItemAddingLabel = Create<zcom::Label>(L"Add media");
        _allowItemAddingLabel->SetBaseSize(100, 30);
        _allowItemAddingLabel->SetOffsetPixels(45, 70);
        _allowItemAddingLabel->SetFontSize(16.0f);
        _allowItemAddingLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _allowItemAddingLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _allowItemManagingCheckbox = Create<zcom::Checkbox>();
        _allowItemManagingCheckbox->SetBaseSize(20, 20);
        _allowItemManagingCheckbox->SetOffsetPixels(15, 105);
        _allowItemManagingCheckbox->Checked(true);

        _allowItemManagingLabel = Create<zcom::Label>(L"Manage (remove/reorder) media");
        _allowItemManagingLabel->SetBaseSize(250, 30);
        _allowItemManagingLabel->SetOffsetPixels(45, 100);
        _allowItemManagingLabel->SetFontSize(16.0f);
        _allowItemManagingLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _allowItemManagingLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _allowPlaybackStartStopCheckbox = Create<zcom::Checkbox>();
        _allowPlaybackStartStopCheckbox->SetBaseSize(20, 20);
        _allowPlaybackStartStopCheckbox->SetOffsetPixels(15, 135);
        _allowPlaybackStartStopCheckbox->Checked(true);

        _allowPlaybackStartStopLabel = Create<zcom::Label>(L"Start/stop playback");
        _allowPlaybackStartStopLabel->SetBaseSize(200, 30);
        _allowPlaybackStartStopLabel->SetOffsetPixels(45, 130);
        _allowPlaybackStartStopLabel->SetFontSize(16.0f);
        _allowPlaybackStartStopLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _allowPlaybackStartStopLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _playlistOptionsPanel->AddItem(_playlistUserPermissionsLabel.get());
        _playlistOptionsPanel->AddItem(_allowItemAddingCheckbox.get());
        _playlistOptionsPanel->AddItem(_allowItemAddingLabel.get());
        _playlistOptionsPanel->AddItem(_allowItemManagingCheckbox.get());
        _playlistOptionsPanel->AddItem(_allowItemManagingLabel.get());
        _playlistOptionsPanel->AddItem(_allowPlaybackStartStopCheckbox.get());
        _playlistOptionsPanel->AddItem(_allowPlaybackStartStopLabel.get());
    }

    // Playback options
    {
        _playbackOptionsOpen = false;
        _playbackOptionsPanel = Create<zcom::Panel>();
        _playbackOptionsPanel->SetBaseHeight(30);
        _playbackOptionsPanel->SetParentWidthPercent(1.0f);

        _playbackOptionsArrowImage = Create<zcom::Image>();
        _playbackOptionsArrowImage->SetBaseSize(25, 25);
        _playbackOptionsArrowImage->SetVerticalOffsetPixels(3);
        _playbackOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_right_7x7"));
        _playbackOptionsArrowImage->SetPlacement(zcom::ImagePlacement::CENTER);
        _playbackOptionsArrowImage->SetTintColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));

        _playbackOptionsButton = Create<zcom::Button>(L"Playback");
        _playbackOptionsButton->SetBaseHeight(30);
        _playbackOptionsButton->SetParentWidthPercent(1.0f);
        _playbackOptionsButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
        _playbackOptionsButton->SetButtonHoverColor(D2D1::ColorF(0, 0.2f));
        _playbackOptionsButton->SetButtonClickColor(D2D1::ColorF(0, 0.4f));
        _playbackOptionsButton->Text()->SetFontSize(18.0f);
        _playbackOptionsButton->Text()->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
        _playbackOptionsButton->Text()->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        _playbackOptionsButton->Text()->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _playbackOptionsButton->Text()->SetMargins({ 25.0f });
        _playbackOptionsButton->SetActivation(zcom::ButtonActivation::PRESS);
        _playbackOptionsButton->SetOnActivated([&]()
        {
            if (!_playbackOptionsOpen)
            {
                _playbackOptionsPanel->SetBaseHeight(_playbackOptionsPanel->GetContentHeight() + 10);
                _playbackOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_down_7x7"));
                _playbackOptionsOpen = true;
            }
            else
            {
                _playbackOptionsPanel->SetBaseHeight(30);
                _playbackOptionsArrowImage->SetImage(ResourceManager::GetImage("menu_arrow_right_7x7"));
                _playbackOptionsOpen = false;
            }
            _UpdateTabIndices();
        });
        _playbackOptionsButton->SetTabIndex(0);

        float textWidth = _playbackOptionsButton->Text()->GetTextWidth();
        _playbackOptionsOpenSeparator = Create<zcom::EmptyPanel>();
        _playbackOptionsOpenSeparator->SetParentWidthPercent(1.0f);
        _playbackOptionsOpenSeparator->SetBaseSize(-textWidth - 10 - 10, 1);
        _playbackOptionsOpenSeparator->SetOffsetPixels(-10, 15);
        _playbackOptionsOpenSeparator->SetHorizontalAlignment(zcom::Alignment::END);
        _playbackOptionsOpenSeparator->SetBorderVisibility(true);
        _playbackOptionsOpenSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        _playbackOptionsPanel->AddItem(_playbackOptionsButton.get());
        _playbackOptionsPanel->AddItem(_playbackOptionsArrowImage.get());
        _playbackOptionsPanel->AddItem(_playbackOptionsOpenSeparator.get());

        _playbackUserPermissionsLabel = Create<zcom::Label>(L"Allow users to:");
        _playbackUserPermissionsLabel->SetBaseSize(200, 30);
        _playbackUserPermissionsLabel->SetOffsetPixels(15, 40);
        _playbackUserPermissionsLabel->SetFontSize(16.0f);
        _playbackUserPermissionsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _playbackUserPermissionsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _allowPlaybackManipulationCheckbox = Create<zcom::Checkbox>();
        _allowPlaybackManipulationCheckbox->SetBaseSize(20, 20);
        _allowPlaybackManipulationCheckbox->SetOffsetPixels(15, 75);
        _allowPlaybackManipulationCheckbox->Checked(true);

        _allowPlaybackManipulationLabel = Create<zcom::Label>(L"Manipulate playback (pause/resume/seek)");
        _allowPlaybackManipulationLabel->SetBaseSize(300, 30);
        _allowPlaybackManipulationLabel->SetOffsetPixels(45, 70);
        _allowPlaybackManipulationLabel->SetFontSize(16.0f);
        _allowPlaybackManipulationLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _allowPlaybackManipulationLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _allowStreamChangingCheckbox = Create<zcom::Checkbox>();
        _allowStreamChangingCheckbox->SetBaseSize(20, 20);
        _allowStreamChangingCheckbox->SetOffsetPixels(15, 105);
        _allowStreamChangingCheckbox->Checked(true);

        _allowStreamChangingLabel = Create<zcom::Label>(L"Change tracks");
        _allowStreamChangingLabel->SetBaseSize(250, 30);
        _allowStreamChangingLabel->SetOffsetPixels(45, 100);
        _allowStreamChangingLabel->SetFontSize(16.0f);
        _allowStreamChangingLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _allowStreamChangingLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        _allowDrawingCheckbox = Create<zcom::Checkbox>();
        _allowDrawingCheckbox->SetBaseSize(20, 20);
        _allowDrawingCheckbox->SetOffsetPixels(15, 135);
        _allowDrawingCheckbox->Checked(true);
        _allowDrawingCheckbox->SetActive(false);

        _allowDrawingLabel = Create<zcom::Label>(L"Draw");
        _allowDrawingLabel->SetBaseSize(200, 30);
        _allowDrawingLabel->SetOffsetPixels(45, 130);
        _allowDrawingLabel->SetFontSize(16.0f);
        _allowDrawingLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _allowDrawingLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        _allowDrawingLabel->SetActive(false);

        _playbackOptionsPanel->AddItem(_playbackUserPermissionsLabel.get());
        _playbackOptionsPanel->AddItem(_allowPlaybackManipulationCheckbox.get());
        _playbackOptionsPanel->AddItem(_allowPlaybackManipulationLabel.get());
        _playbackOptionsPanel->AddItem(_allowStreamChangingCheckbox.get());
        _playbackOptionsPanel->AddItem(_allowStreamChangingLabel.get());
        _playbackOptionsPanel->AddItem(_allowDrawingCheckbox.get());
        _playbackOptionsPanel->AddItem(_allowDrawingLabel.get());
    }

    _advancedOptionsPanel->AddItem(_advancedOptionsLabel.get());
    _advancedOptionsPanel->AddItem(_generalOptionsPanel.get());
    _advancedOptionsPanel->AddItem(_playlistOptionsPanel.get());
    _advancedOptionsPanel->AddItem(_playbackOptionsPanel.get());


    _separatorBottom = Create<zcom::EmptyPanel>();
    _separatorBottom->SetBaseSize(440, 1);
    _separatorBottom->SetVerticalOffsetPixels(-80);
    _separatorBottom->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _separatorBottom->SetVerticalAlignment(zcom::Alignment::END);
    _separatorBottom->SetBorderVisibility(true);
    _separatorBottom->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _separatorBottom->SetZIndex(0);

    _startButton = Create<zcom::Button>(L"Start");
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
    _startButton->SetTabIndex(2);

    _cancelButton = Create<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 30);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("close_100x100"));
    _cancelButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });
    _cancelButton->SetTabIndex(1000);

    _startLoadingInfoLabel = Create<zcom::Label>(L"");
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
    //_mainPanel->AddItem(_usernameLabel.get());
    //_mainPanel->AddItem(_usernameInput.get());
    _mainPanel->AddItem(_advancedOptionsPanel.get());
    _mainPanel->AddItem(_separatorBottom.get());
    _mainPanel->AddItem(_startButton.get());
    _mainPanel->AddItem(_cancelButton.get());
    _mainPanel->AddItem(_startLoadingInfoLabel.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));

    _UpdateTabIndices();

    _starting = false;
    _startSuccessful = false;
    _closeScene = false;
}

void StartServerScene::_Uninit()
{
    _canvas->ClearComponents();
    _advancedOptionsPanel->ClearItems();
    _generalOptionsPanel->ClearItems();
    _playlistOptionsPanel->ClearItems();
    _playbackOptionsPanel->ClearItems();

    _mainPanel = nullptr;
    _titleLabel = nullptr;
    _separatorTitle = nullptr;
    _portLabel = nullptr;
    _portInput = nullptr;
    _presetDropdown = nullptr;
    _usernameLabel = nullptr;
    _usernameInput = nullptr;

    _advancedOptionsPanel = nullptr;

    _generalOptionsPanel = nullptr;
    _generalOptionsButton = nullptr;
    _generalOptionsArrowImage = nullptr;
    _generalOptionsOpenSeparator = nullptr;
    _passwordLabel = nullptr;
    _passwordInput = nullptr;
    _maxUsersLabel = nullptr;
    _maxUsersInput = nullptr;
    _chatEnabledCheckbox = nullptr;
    _chatEnabledLabel = nullptr;

    _playlistOptionsPanel = nullptr;
    _playlistOptionsButton = nullptr;
    _playlistOptionsArrowImage = nullptr;
    _playlistOptionsOpenSeparator = nullptr;
    _playlistUserPermissionsLabel = nullptr;
    _allowItemAddingCheckbox = nullptr;
    _allowItemAddingLabel = nullptr;
    _allowItemManagingCheckbox = nullptr;
    _allowItemManagingLabel = nullptr;
    _allowPlaybackStartStopCheckbox = nullptr;
    _allowPlaybackStartStopLabel = nullptr;

    _playbackOptionsPanel = nullptr;
    _playbackOptionsButton = nullptr;
    _playbackOptionsArrowImage = nullptr;
    _playbackOptionsOpenSeparator = nullptr;
    _playbackUserPermissionsLabel = nullptr;
    _allowPlaybackManipulationCheckbox = nullptr;
    _allowPlaybackManipulationLabel = nullptr;
    _allowStreamChangingCheckbox = nullptr;
    _allowStreamChangingLabel = nullptr;
    _allowDrawingCheckbox = nullptr;
    _allowDrawingLabel = nullptr;

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

    if (_advancedOptionsPanel->MaxScroll(zcom::Scrollbar::VERTICAL) > 0)
        _advancedOptionsPanel->ScrollBackgroundVisible(zcom::Scrollbar::VERTICAL, true);
    else
        _advancedOptionsPanel->ScrollBackgroundVisible(zcom::Scrollbar::VERTICAL, false);

    if (_starting)
    {
        _starting = false;

        // Set default permissions
        App::Instance()->usersServer.DefaultUser()->AddPermission(L"Add items", PERMISSION_ADD_ITEMS);
        App::Instance()->usersServer.DefaultUser()->AddPermission(L"Manage items", PERMISSION_MANAGE_ITEMS);
        App::Instance()->usersServer.DefaultUser()->AddPermission(L"Start/stop playback", PERMISSION_START_STOP_PLAYBACK);
        App::Instance()->usersServer.DefaultUser()->AddPermission(L"Manipulate playback", PERMISSION_MANIPULATE_PLAYBACK);
        App::Instance()->usersServer.DefaultUser()->AddPermission(L"Change tracks", PERMISSION_CHANGE_STREAMS);
        App::Instance()->usersServer.DefaultUser()->AddPermission(L"Draw", PERMISSION_DRAW);

        App::Instance()->usersServer.DefaultUser()->SetPermission(PERMISSION_ADD_ITEMS, _allowItemAddingCheckbox->Checked());
        App::Instance()->usersServer.DefaultUser()->SetPermission(PERMISSION_MANAGE_ITEMS, _allowItemManagingCheckbox->Checked());
        App::Instance()->usersServer.DefaultUser()->SetPermission(PERMISSION_START_STOP_PLAYBACK, _allowPlaybackStartStopCheckbox->Checked());
        App::Instance()->usersServer.DefaultUser()->SetPermission(PERMISSION_MANIPULATE_PLAYBACK, _allowPlaybackManipulationCheckbox->Checked());
        App::Instance()->usersServer.DefaultUser()->SetPermission(PERMISSION_CHANGE_STREAMS, _allowStreamChangingCheckbox->Checked());
        App::Instance()->usersServer.DefaultUser()->SetPermission(PERMISSION_DRAW, _allowDrawingCheckbox->Checked());

        // Set username
        std::wstring defaultUsername = Options::Instance()->GetValue(OPTIONS_DEFAULT_USERNAME);
        if (!defaultUsername.empty())
            _app->users.SetSelfUsername(defaultUsername);

        APP_NETWORK->StartManager();
        _startSuccessful = true;
        _closeScene = true;
    }

    if (_startClicked)
    {
        _startClicked = false;

        _portInput->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        // Check if port is valid
        std::wstring port = _portInput->Text()->GetText();
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

            // Save used port
            Options::Instance()->SetValue(OPTIONS_LAST_PORT, port);

            auto manager = std::make_unique<znet::ServerManager>(str_to_int(wstring_to_string(port)), wstring_to_string(_passwordInput->Text()->GetText()));
            if (manager->InitSuccessful())
            {
                int maxUsers = -1;
                std::wstring maxUsersInput = _maxUsersInput->Text()->GetText();
                if (!maxUsersInput.empty())
                {
                    try
                    {
                        maxUsers = std::stoi(wstring_to_string(maxUsersInput));
                    }
                    catch (std::exception ex) {}
                }
                manager->MaxUsers(maxUsers);
                APP_NETWORK->SetManager(std::move(manager));
                //APP_NETWORK->SetUsername(_usernameInput->GetText());
                // Delay manager start by 1 frame, to allow all event handlers to prepare
                _starting = true;
            }
            else
            {
                std::wostringstream ss;
                ss << manager->FailMessage();
                if (manager->FailCode() != -1)
                    ss << L"(Code: " << manager->FailCode() << ')';

                _startLoadingInfoLabel->SetText(ss.str());
                _startLoadingInfoLabel->SetVisible(true);
            }
        }
    }
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

void StartServerScene::_UpdateTabIndices()
{
    // General
    if (_generalOptionsOpen)
    {
        _passwordInput->SetTabIndex(1);
        _maxUsersInput->SetTabIndex(2);
        _chatEnabledCheckbox->SetTabIndex(3);
    }
    else
    {
        _passwordInput->SetTabIndex(-1);
        _maxUsersInput->SetTabIndex(-1);
        _chatEnabledCheckbox->SetTabIndex(-1);
    }

    // Playlist
    if (_playlistOptionsOpen)
    {
        _allowItemAddingCheckbox->SetTabIndex(1);
        _allowItemManagingCheckbox->SetTabIndex(2);
        _allowPlaybackStartStopCheckbox->SetTabIndex(3);
    }
    else
    {
        _allowItemAddingCheckbox->SetTabIndex(-1);
        _allowItemManagingCheckbox->SetTabIndex(-1);
        _allowPlaybackStartStopCheckbox->SetTabIndex(-1);
    }

    // Playback
    if (_playbackOptionsOpen)
    {
        _allowPlaybackManipulationCheckbox->SetTabIndex(1);
        _allowStreamChangingCheckbox->SetTabIndex(2);
        _allowDrawingCheckbox->SetTabIndex(3);
    }
    else
    {
        _allowPlaybackManipulationCheckbox->SetTabIndex(-1);
        _allowStreamChangingCheckbox->SetTabIndex(-1);
        _allowDrawingCheckbox->SetTabIndex(-1);
    }

    _advancedOptionsPanel->ReindexTabOrder();
    _generalOptionsPanel->ReindexTabOrder();
    _playlistOptionsPanel->ReindexTabOrder();
    _playbackOptionsPanel->ReindexTabOrder();
}

void StartServerScene::_StartClicked()
{
    if (!_starting)
        _startClicked = true;
}

void StartServerScene::_CancelClicked()
{
    _startSuccessful = false;
    _closeScene = true;
}