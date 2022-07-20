#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "Button.h"
#include "TextInput.h"
#include "Label.h"
#include "EmptyPanel.h"
#include "Checkbox.h"

#include <optional>

struct SavePlaylistSceneOptions : public SceneOptionsBase
{
    std::wstring openedPlaylistName;
};

class SavePlaylistScene : public Scene
{
public:
    bool CloseScene() const { return _closeScene; };
    std::wstring PlaylistName() const { return _playlistName; }

    SavePlaylistScene(App* app);

    const char* GetName() const { return "save_playlist"; }
    static const char* StaticName() { return "save_playlist"; }

private:
    // Main selection
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Label> _titleLabel = nullptr;
    std::unique_ptr<zcom::Button> _cancelButton = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _titleSeparator = nullptr;
    std::unique_ptr<zcom::TextInput> _playlistNameInput = nullptr;
    std::unique_ptr<zcom::Label> _playlistFilenameLabel = nullptr;
    std::unique_ptr<zcom::Checkbox> _overwriteExistingCheckbox = nullptr;
    std::unique_ptr<zcom::Label> _overwriteExistingLabel = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _bottomSeparator = nullptr;
    std::unique_ptr<zcom::Button> _saveButton = nullptr;
    std::unique_ptr<zcom::Label> _saveErrorLabel = nullptr;

    bool _closeScene = false;
    std::wstring _playlistName = L"";

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

    void _UpdateFilenameLabel();
    void _SaveClicked();
    void _CancelClicked();
};