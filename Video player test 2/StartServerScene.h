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
    std::unique_ptr<zcom::EmptyPanel> _separatorBottom = nullptr;
    std::unique_ptr<zcom::Button> _startButton = nullptr;
    std::unique_ptr<zcom::Button> _cancelButton = nullptr;
    std::unique_ptr<zcom::Label> _startLoadingInfoLabel = nullptr;
    bool _starting = false;

    bool _startSuccessful = false;
    bool _closeScene = false;

public:
    StartServerScene();

    const char* GetName() const { return "start_server"; }
    static const char* StaticName() { return "start_server"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

public:
    bool StartSuccessful() const;
    // Either start was successful or scene was closed
    bool CloseScene() const;

private:
    void _StartClicked();
    void _CancelClicked();
};