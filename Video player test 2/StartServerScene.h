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

struct StartServerSceneOptions : public SceneOptionsBase
{

};

class StartServerScene : public Scene
{
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Label> _titleLabel = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _separatorTitle = nullptr;
    std::unique_ptr<zcom::Label> _portLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _portInput = nullptr;
    std::unique_ptr<zcom::DropdownSelection> _presetDropdown = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _separatorTop = nullptr;
    std::unique_ptr<zcom::Label> _usernameLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _usernameInput = nullptr;

    std::unique_ptr<zcom::DirectionalPanel> _advancedOptionsPanel = nullptr;
    std::unique_ptr<zcom::Label> _advancedOptionsLabel = nullptr;
    // General options
    bool _generalOptionsOpen = false;
    std::unique_ptr<zcom::Panel> _generalOptionsPanel = nullptr;
    std::unique_ptr<zcom::Button> _generalOptionsButton = nullptr;
    std::unique_ptr<zcom::Image> _generalOptionsArrowImage = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _generalOptionsOpenSeparator = nullptr;
    std::unique_ptr<zcom::Label> _passwordLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _passwordInput = nullptr;
    std::unique_ptr<zcom::Label> _maxUsersLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _maxUsersInput = nullptr;
    std::unique_ptr<zcom::Checkbox> _chatEnabledCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _chatEnabledLabel = nullptr;

    // Playlist options
    bool _playlistOptionsOpen = false;
    std::unique_ptr<zcom::Panel> _playlistOptionsPanel = nullptr;
    std::unique_ptr<zcom::Button> _playlistOptionsButton = nullptr;
    std::unique_ptr<zcom::Image> _playlistOptionsArrowImage = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _playlistOptionsOpenSeparator = nullptr;
    std::unique_ptr<zcom::Label> _playlistUserPermissionsLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _allowItemAddingCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _allowItemAddingLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _allowItemManagingCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _allowItemManagingLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _allowPlaybackStartStopCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _allowPlaybackStartStopLabel = nullptr;

    // Playback options
    bool _playbackOptionsOpen = false;
    std::unique_ptr<zcom::Panel> _playbackOptionsPanel = nullptr;
    std::unique_ptr<zcom::Button> _playbackOptionsButton = nullptr;
    std::unique_ptr<zcom::Image> _playbackOptionsArrowImage = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _playbackOptionsOpenSeparator = nullptr;
    std::unique_ptr<zcom::Label> _playbackUserPermissionsLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _allowPlaybackManipulationCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _allowPlaybackManipulationLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _allowStreamChangingCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _allowStreamChangingLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _allowDrawingCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _allowDrawingLabel = nullptr;

    std::unique_ptr<zcom::EmptyPanel> _separatorBottom = nullptr;
    std::unique_ptr<zcom::Button> _startButton = nullptr;
    std::unique_ptr<zcom::Button> _cancelButton = nullptr;
    std::unique_ptr<zcom::Label> _startLoadingInfoLabel = nullptr;
    bool _starting = false;

    bool _startSuccessful = false;
    bool _closeScene = false;

public:
    StartServerScene(App* app);

    const char* GetName() const { return "start_server"; }
    static const char* StaticName() { return "start_server"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

public:
    bool StartSuccessful() const;
    // Either start was successful or scene was closed
    bool CloseScene() const;

private:
    void _StartClicked();
    void _CancelClicked();
};