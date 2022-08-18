#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "EmptyPanel.h"
#include "Button.h"
#include "TextInput.h"
#include "Label.h"
#include "LoadingImage.h"

#include "FileDialog.h"

struct ConnectSceneOptions : public SceneOptionsBase
{

};

class ConnectScene : public Scene
{
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Label> _titleLabel = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _separator1 = nullptr;
    std::unique_ptr<zcom::Label> _ipLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _ipInput = nullptr;
    std::unique_ptr<zcom::Label> _portLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _portInput = nullptr;
    std::unique_ptr<zcom::Label> _recentConnectionsLabel = nullptr;
    std::unique_ptr<zcom::Panel> _recentConnectionsPanel = nullptr;
    std::unique_ptr<zcom::Label> _noRecentConnectionsLabel = nullptr;
    std::unique_ptr<zcom::Label> _usernameLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _usernameInput = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _separator2 = nullptr;
    std::unique_ptr<zcom::Button> _connectButton = nullptr;
    std::unique_ptr<zcom::Button> _cancelButton = nullptr;
    std::unique_ptr<zcom::LoadingImage> _connectLoadingImage = nullptr;
    std::unique_ptr<zcom::Label> _connectLoadingInfoLabel = nullptr;

    // Password input
    std::unique_ptr<zcom::EmptyPanel> _dimPanel = nullptr;
    std::unique_ptr<zcom::Panel> _passwordPanel = nullptr;
    std::unique_ptr<zcom::Label> _passwordLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _passwordInput = nullptr;
    std::unique_ptr<zcom::Button> _pwContinueButton = nullptr;
    std::unique_ptr<zcom::Button> _pwCancelButton = nullptr;

    bool _connecting = false;
    bool _fromPasswordPanel = false;
    std::string _ip;
    uint16_t _port;
    bool _connectionSuccessful = false;
    bool _closeScene = false;

public:
    ConnectScene(App* app);

    const char* GetName() const { return "connect"; }
    static const char* StaticName() { return "connect"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

public:
    bool ConnectionSuccessful() const;
    // Either connection was successful or scene was closed
    bool CloseScene() const;

private:
    void _ConnectClicked();
    void _CancelClicked();
    void _PwContinueClicked();
    void _PwCancelClicked();

    void _OpenPasswordInput();
    void _ClosePasswordInput();

private:
    void _RearrangeRecentConnectionsPanel();
};