#include "App.h" // App.h must be included first
#include "SettingsScene.h"

#include "Options.h"
#include "OptionNames.h"
#include "LastIpOptionAdapter.h"
#include "BoolOptionAdapter.h"
#include "IntOptionAdapter.h"
#include "NumberInput.h"

void SettingsScene::_ShowMainSettingsTab()
{
    // Alter button visuals
    _mainSettingsButton->SetButtonColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _mainSettingsButton->SetButtonHoverColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _mainSettingsButton->SetButtonClickColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _selectedTabSeparator->SetVerticalOffsetPixels(70 + _mainSettingsButton->GetVerticalOffsetPixels());

    _settingsPanel->ClearItems();

    auto mainPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    mainPanel->SetParentSizePercent(1.0f, 1.0f);
    mainPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);

    { // Start maximized
        std::wstring optStr = _LoadSavedOption(OPTIONS_START_MAXIMIZED);
        bool value = BoolOptionAdapter(optStr).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Start maximized");
        label->SetBaseSize(300, 30);
        label->SetHorizontalOffsetPixels(45);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Launch the application window maximized");

        auto checkbox = Create<zcom::Checkbox>();
        checkbox->SetBaseSize(20, 20);
        checkbox->SetHorizontalOffsetPixels(15);
        checkbox->SetVerticalAlignment(zcom::Alignment::CENTER);
        checkbox->Checked(value);
        checkbox->AddOnStateChanged([&](bool newState)
        {
            _changedSettings[OPTIONS_START_MAXIMIZED] = newState ? L"true" : L"false";
        });

        panel->AddItem(label.release(), true);
        panel->AddItem(checkbox.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Start in playlist
        std::wstring optStr = _LoadSavedOption(OPTIONS_START_IN_PLAYLIST);
        bool value = BoolOptionAdapter(optStr).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Start in playlist");
        label->SetBaseSize(300, 30);
        label->SetHorizontalOffsetPixels(45);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Skip the home screen after launching the application and directly open the playlist");

        auto checkbox = Create<zcom::Checkbox>();
        checkbox->SetBaseSize(20, 20);
        checkbox->SetHorizontalOffsetPixels(15);
        checkbox->SetVerticalAlignment(zcom::Alignment::CENTER);
        checkbox->Checked(value);
        checkbox->AddOnStateChanged([&](bool newState)
        {
            _changedSettings[OPTIONS_START_IN_PLAYLIST] = newState ? L"true" : L"false";
        });

        panel->AddItem(label.release(), true);
        panel->AddItem(checkbox.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    _settingsPanel->AddItem(mainPanel.release(), true);
}

void SettingsScene::_ShowPlaybackSettingsTab()
{
    // Alter button visuals
    _playbackSettingsButton->SetButtonColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _playbackSettingsButton->SetButtonHoverColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _playbackSettingsButton->SetButtonClickColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _selectedTabSeparator->SetVerticalOffsetPixels(70 + _playbackSettingsButton->GetVerticalOffsetPixels());

    _settingsPanel->ClearItems();

    auto mainPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    mainPanel->SetParentSizePercent(1.0f, 1.0f);
    mainPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);

    int INPUT_OFFSET = 250;
    int INPUT_WIDTH = 60;

    { // Subtitle framerate
        std::wstring optStr = _LoadSavedOption(OPTIONS_SUBTITLE_FRAMERATE);
        int value = IntOptionAdapter(optStr, 20).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Subtitle framerate:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"The framerate at which subtitles are rendered.\n"
            "Increasing this value makes animated subtitles look nicer at the cost of performance");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(360));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_SUBTITLE_FRAMERATE] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto fpsLabel = Create<zcom::Label>(L"fps");
        fpsLabel->SetBaseSize(80, 30);
        fpsLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        fpsLabel->SetFontSize(16.0f);
        fpsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        fpsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(fpsLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Visual gap
        auto panel = Create<zcom::EmptyPanel>();
        panel->SetBaseHeight(15);
        panel->SetParentWidthPercent(1.0f);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Regular seek size
        std::wstring optStr = _LoadSavedOption(OPTIONS_SEEK_AMOUNT);
        int value = IntOptionAdapter(optStr, 15).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Normal seek length:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(3600));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_SEEK_AMOUNT] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto secLabel = Create<zcom::Label>(L"seconds");
        secLabel->SetBaseSize(80, 30);
        secLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        secLabel->SetFontSize(16.0f);
        secLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        secLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(secLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Small seek size
        std::wstring optStr = _LoadSavedOption(OPTIONS_SEEK_AMOUNT_SMALL);
        int value = IntOptionAdapter(optStr, 5).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Small seek length:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(3600));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_SEEK_AMOUNT_SMALL] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto secLabel = Create<zcom::Label>(L"seconds");
        secLabel->SetBaseSize(80, 30);
        secLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        secLabel->SetFontSize(16.0f);
        secLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        secLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(secLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Large seek size
        std::wstring optStr = _LoadSavedOption(OPTIONS_SEEK_AMOUNT_LARGE);
        int value = IntOptionAdapter(optStr, 60).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Large seek length:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(3600));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_SEEK_AMOUNT_LARGE] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto secLabel = Create<zcom::Label>(L"seconds");
        secLabel->SetBaseSize(80, 30);
        secLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        secLabel->SetFontSize(16.0f);
        secLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        secLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(secLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Visual gap
        auto panel = Create<zcom::EmptyPanel>();
        panel->SetBaseHeight(15);
        panel->SetParentWidthPercent(1.0f);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Regular volume change amount
        std::wstring optStr = _LoadSavedOption(OPTIONS_VOLUME_CHANGE_AMOUNT);
        int value = IntOptionAdapter(optStr, 5).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Normal volume change amount:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(100));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_VOLUME_CHANGE_AMOUNT] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto percLabel = Create<zcom::Label>(L"%");
        percLabel->SetBaseSize(30, 30);
        percLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        percLabel->SetFontSize(16.0f);
        percLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        percLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(percLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Small volume change amount
        std::wstring optStr = _LoadSavedOption(OPTIONS_VOLUME_CHANGE_AMOUNT_SMALL);
        int value = IntOptionAdapter(optStr, 1).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Small volume change amount:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(100));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_VOLUME_CHANGE_AMOUNT_SMALL] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto percLabel = Create<zcom::Label>(L"%");
        percLabel->SetBaseSize(30, 30);
        percLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        percLabel->SetFontSize(16.0f);
        percLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        percLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(percLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Large volume change amount
        std::wstring optStr = _LoadSavedOption(OPTIONS_VOLUME_CHANGE_AMOUNT_LARGE);
        int value = IntOptionAdapter(optStr, 20).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Large volume change amount:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(100));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_VOLUME_CHANGE_AMOUNT_LARGE] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto percLabel = Create<zcom::Label>(L"%");
        percLabel->SetBaseSize(30, 30);
        percLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        percLabel->SetFontSize(16.0f);
        percLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        percLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(percLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    _settingsPanel->AddItem(mainPanel.release(), true);
}

void SettingsScene::_ShowNetworkSettingsTab()
{
    // Alter button visuals
    _networkSettingsButton->SetButtonColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _networkSettingsButton->SetButtonHoverColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _networkSettingsButton->SetButtonClickColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _selectedTabSeparator->SetVerticalOffsetPixels(70 + _networkSettingsButton->GetVerticalOffsetPixels());

    _settingsPanel->ClearItems();

    auto mainPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    mainPanel->SetParentSizePercent(1.0f, 1.0f);
    mainPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);

    { // Default username
        std::wstring optStr = _LoadSavedOption(OPTIONS_DEFAULT_USERNAME);
        std::wstring value = optStr.substr(0, 32);

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(60);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Default username:");
        label->SetBaseSize(200, 25);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"The default username will be entered automatically when connecting to or creating a server.\n"
            "If no username is set, the last entered username will be used");

        auto input = Create<zcom::TextInput>();
        input->SetBaseSize(200, 30);
        input->SetOffsetPixels(15, 25);
        input->SetCornerRounding(5.0f);
        input->PlaceholderText()->SetText(L"Use last entered username");
        input->Text()->SetText(value);
        input->AddOnTextChanged([&](zcom::Label*, std::wstring& newValue)
        {
            _changedSettings[OPTIONS_DEFAULT_USERNAME] = newValue;
        });

        //auto help = Create<zcom::Label>(L"?");
        //help->SetBaseSize(30, 30);
        //help->SetOffsetPixels(215, 25);
        //help->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        //help->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        //help->SetFontSize(18.0f);
        //help->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
        //help->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
        //help->SetFontColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
        //zcom::PROP_Shadow helpShadow;
        //helpShadow.offsetX = 1.0f;
        //helpShadow.offsetY = 1.0f;
        //helpShadow.blurStandardDeviation = 2.0f;
        //helpShadow.color = D2D1::ColorF(0);
        //help->SetProperty(helpShadow);
        //help->SetHoverText(L"Test help bubble");
        //help->SetHoverTextDelay(Duration(250, MILLISECONDS));

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        //panel->AddItem(help.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Default port
        std::wstring optStr = _LoadSavedOption(OPTIONS_DEFAULT_PORT);
        std::wstring value;
        if (LastIpOptionAdapter::ValidatePort(optStr))
            value = optStr;

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(60);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Default server port:");
        label->SetBaseSize(200, 25);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"The default server port will be entered automatically when creating a server.\n"
            "If no port is set, the last entered port will be used");

        auto input = Create<zcom::TextInput>();
        input->SetBaseSize(200, 30);
        input->SetOffsetPixels(15, 25);
        input->SetCornerRounding(5.0f);
        input->PlaceholderText()->SetText(L"Use last entered port");
        input->Text()->SetText(value);
        input->AddOnTextChanged([&](zcom::Label*, std::wstring& newValue)
        {
            _changedSettings[OPTIONS_DEFAULT_USERNAME] = newValue;
        });

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    _settingsPanel->AddItem(mainPanel.release(), true);
}

void SettingsScene::_ShowAdvancedSettingsTab()
{
    // Alter button visuals
    _advancedSettingsButton->SetButtonColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _advancedSettingsButton->SetButtonHoverColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _advancedSettingsButton->SetButtonClickColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _selectedTabSeparator->SetVerticalOffsetPixels(70 + _advancedSettingsButton->GetVerticalOffsetPixels());

    _settingsPanel->ClearItems();

    auto mainPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    mainPanel->SetParentSizePercent(1.0f, 1.0f);
    mainPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);

    int INPUT_OFFSET = 280;
    int INPUT_WIDTH = 60;

    { // Max buffered video frames
        std::wstring optStr = _LoadSavedOption(OPTIONS_MAX_VIDEO_FRAMES);
        int value = IntOptionAdapter(optStr, 12).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Video frame buffer size:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Maximum number of decoded video frames that can be buffered.\n"
            "Higher values could help if certain points in the media keep stuttering.\n"
            "(A single 1080p frame takes up 8mb of RAM, as such, large values can be very memory intensive)");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(1000));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_MAX_VIDEO_FRAMES] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto frameLabel = Create<zcom::Label>(L"frames");
        frameLabel->SetBaseSize(80, 30);
        frameLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        frameLabel->SetFontSize(16.0f);
        frameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        frameLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(frameLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Max buffered audio frames
        std::wstring optStr = _LoadSavedOption(OPTIONS_MAX_AUDIO_FRAMES);
        int value = IntOptionAdapter(optStr, 100).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Audio frame buffer size:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Maximum number of decoded audio frames that can be buffered.\n"
            "Very small values could cause stability issues.\n"
            "Changing this value could be useful in very specific scenarios.");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(10000));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_MAX_AUDIO_FRAMES] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto frameLabel = Create<zcom::Label>(L"frames");
        frameLabel->SetBaseSize(80, 30);
        frameLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        frameLabel->SetFontSize(16.0f);
        frameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        frameLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(frameLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Max buffered subtitle frames
        std::wstring optStr = _LoadSavedOption(OPTIONS_MAX_SUBTITLE_FRAMES);
        int value = IntOptionAdapter(optStr, 10).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Subtitle frame buffer size:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Maximum number of decoded subtitle frames that can be buffered.\n"
            "Higher values could help if certain points in the subtitles keep stuttering.\n"
            "As with video frames, this setting is very memory intensive.");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(10000));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_MAX_SUBTITLE_FRAMES] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto frameLabel = Create<zcom::Label>(L"frames");
        frameLabel->SetBaseSize(80, 30);
        frameLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        frameLabel->SetFontSize(16.0f);
        frameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        frameLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(frameLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Visual gap
        auto panel = Create<zcom::EmptyPanel>();
        panel->SetBaseHeight(15);
        panel->SetParentWidthPercent(1.0f);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Max video packet memory
        std::wstring optStr = _LoadSavedOption(OPTIONS_MAX_VIDEO_MEMORY);
        int value = IntOptionAdapter(optStr, 250).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Video packet buffer memory limit:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Buffer size in megabytes for read video packets.\n"
            "This value has has little effect when playing local files.\n"
            "When online, the lowest value for this setting among all users gets applied to everyone (min. limit when online: 100mb).");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(10));
        input->SetMaxValue(NumberInputValue(10000));
        input->SetStepSize(NumberInputValue(10));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_MAX_VIDEO_MEMORY] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto mbLabel = Create<zcom::Label>(L"mb");
        mbLabel->SetBaseSize(80, 30);
        mbLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        mbLabel->SetFontSize(16.0f);
        mbLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        mbLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(mbLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Max audio packet memory
        std::wstring optStr = _LoadSavedOption(OPTIONS_MAX_AUDIO_MEMORY);
        int value = IntOptionAdapter(optStr, 10).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Audio packet buffer memory limit:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Buffer size in megabytes for read audio packets.\n"
            "This value has has little effect when playing local files.\n"
            "When online, the lowest value for this setting among all users gets applied to everyone (min. limit when online: 10mb).");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(1000));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_MAX_AUDIO_MEMORY] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto mbLabel = Create<zcom::Label>(L"mb");
        mbLabel->SetBaseSize(80, 30);
        mbLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        mbLabel->SetFontSize(16.0f);
        mbLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        mbLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(mbLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    { // Max subtitle packet memory
        std::wstring optStr = _LoadSavedOption(OPTIONS_MAX_SUBTITLE_MEMORY);
        int value = IntOptionAdapter(optStr, 1).Value();

        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(30);
        panel->SetParentWidthPercent(1.0f);

        auto label = Create<zcom::Label>(L"Subtitle packet buffer memory limit:");
        label->SetBaseSize(INPUT_OFFSET - 30, 30);
        label->SetHorizontalOffsetPixels(15);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetHoverText(L"Buffer size in megabytes for read subtitle packets.\n"
            "This value has has little effect when playing local files.\n"
            "When online, the lowest value for this setting among all users gets applied to everyone (min. limit when online: 1mb).");

        auto input = Create<zcom::NumberInput>();
        input->SetBaseSize(60, 28);
        input->SetHorizontalOffsetPixels(INPUT_OFFSET);
        input->SetVerticalAlignment(zcom::Alignment::CENTER);
        input->SetCornerRounding(5.0f);
        input->SetValue(NumberInputValue(value));
        input->SetMinValue(NumberInputValue(1));
        input->SetMaxValue(NumberInputValue(100));
        input->AddOnValueChanged([&](NumberInputValue newValue)
        {
            _changedSettings[OPTIONS_MAX_SUBTITLE_MEMORY] = IntOptionAdapter(newValue.getAsInteger()).ToOptionString();
        });

        auto mbLabel = Create<zcom::Label>(L"mb");
        mbLabel->SetBaseSize(80, 30);
        mbLabel->SetHorizontalOffsetPixels(INPUT_OFFSET + INPUT_WIDTH + 10);
        mbLabel->SetFontSize(16.0f);
        mbLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        mbLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        panel->AddItem(mbLabel.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    _settingsPanel->AddItem(mainPanel.release(), true);
}

void SettingsScene::_ShowKeybindSettingsTab()
{
    // Alter button visuals
    _keybindSettingsButton->SetButtonColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _keybindSettingsButton->SetButtonHoverColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _keybindSettingsButton->SetButtonClickColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    _selectedTabSeparator->SetVerticalOffsetPixels(70 + _keybindSettingsButton->GetVerticalOffsetPixels());

    _settingsPanel->ClearItems();

    auto mainPanel = Create<zcom::DirectionalPanel>(zcom::PanelDirection::DOWN);
    mainPanel->SetParentSizePercent(1.0f, 1.0f);
    mainPanel->SetBaseHeight(-30);
    mainPanel->SetVerticalOffsetPixels(30);
    mainPanel->Scrollable(zcom::Scrollbar::VERTICAL, true);
    mainPanel->ScrollBackgroundVisible(zcom::Scrollbar::VERTICAL, true);
    mainPanel->ScrollBackgroundColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));

    auto infoLabel = Create<zcom::Label>(L"Custom keybinds will be added later, for now this is just an info page");
    infoLabel->SetParentWidthPercent(1.0f);
    infoLabel->SetBaseHeight(30);
    infoLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    infoLabel->SetFontColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    infoLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    infoLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _settingsPanel->AddItem(infoLabel.release(), true);

    std::vector<std::pair<std::wstring, std::wstring>> keybinds;
    keybinds.push_back({ L"Pause/play"              , L"Space" });
    keybinds.push_back({ L"Seek forward"            , L"Right" });
    keybinds.push_back({ L"Seek backward"           , L"Left" });
    keybinds.push_back({ L"Small seek forward"      , L"Shift + Right" });
    keybinds.push_back({ L"Small seek backward"     , L"Shift + Left" });
    keybinds.push_back({ L"Large seek forward"      , L"Ctrl + Right" });
    keybinds.push_back({ L"Large seek backward"     , L"Ctrl + Left" });
    keybinds.push_back({ L"Volume up"               , L"Up" });
    keybinds.push_back({ L"Volume down"             , L"Down" });
    keybinds.push_back({ L"Small volume up"         , L"Shift + Up" });
    keybinds.push_back({ L"Small volume down"       , L"Shift + Down" });
    keybinds.push_back({ L"Large volume up"         , L"Ctrl + Up" });
    keybinds.push_back({ L"Large volume down"       , L"Ctrl + Down" });
    keybinds.push_back({ L"Mute"                    , L"M" });
    keybinds.push_back({ L"Toggle visualizer"       , L"V" });
    keybinds.push_back({ L"Toggle fullscreen"       , L"F" });
    keybinds.push_back({ L"Open playlist"           , L"O" });
    keybinds.push_back({ L"Close playlist"          , L"Esc" });
    keybinds.push_back({ L"Open chat"               , L"Enter" });
    keybinds.push_back({ L"Next video track"        , L"Numpad2" });
    keybinds.push_back({ L"Previous video track"    , L"Numpad1" });
    keybinds.push_back({ L"Disable video track"     , L"Numpad3" });
    keybinds.push_back({ L"Next audio track"        , L"Numpad5" });
    keybinds.push_back({ L"Previous audio track"    , L"Numpad4" });
    keybinds.push_back({ L"Disable audio track"     , L"Numpad6" });
    keybinds.push_back({ L"Next subtitle track"     , L"Numpad8" });
    keybinds.push_back({ L"Previous subtitle track" , L"Numpad7" });
    keybinds.push_back({ L"Disable subtitle track"  , L"Numpad9" });

    for (int i = 0; i < keybinds.size(); i++)
    {
        auto panel = Create<zcom::Panel>();
        panel->SetBaseHeight(25);
        panel->SetParentWidthPercent(1.0f);
        if (i % 2 == 0)
            panel->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::White, 0.03f));
        else
            panel->SetBackgroundColor(D2D1::ColorF(0, 0.0f));

        auto label = Create<zcom::Label>(keybinds[i].first);
        label->SetParentWidthPercent(1.0f);
        label->SetBaseSize(-150, 25);
        label->SetFontSize(16.0f);
        label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        label->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        label->SetMargins({ 15.0f });

        auto input = Create<zcom::Label>(keybinds[i].second);
        input->SetBaseSize(150, 25);
        input->SetHorizontalAlignment(zcom::Alignment::END);
        input->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
        input->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        input->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);

        panel->AddItem(label.release(), true);
        panel->AddItem(input.release(), true);
        mainPanel->AddItem(panel.release(), true);
    }

    _settingsPanel->AddItem(mainPanel.release(), true);
}