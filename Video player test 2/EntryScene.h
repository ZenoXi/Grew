#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "Button.h"
#include "TextInput.h"
#include "Label.h"
#include "LoadingImage.h"

#include "FileDialog.h"

struct EntrySceneOptions : public SceneOptionsBase
{

};

class EntryScene : public Scene
{
    // Main selection
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Button> _connectButton = nullptr;
    std::unique_ptr<zcom::Button> _shareButton = nullptr;
    std::unique_ptr<zcom::Button> _fileButton = nullptr;
    std::unique_ptr<zcom::Label> _connectLabel = nullptr;
    std::unique_ptr<zcom::Label> _shareLabel = nullptr;
    std::unique_ptr<zcom::Label> _fileLabel = nullptr;
    std::unique_ptr<zcom::Button> _testButton = nullptr;

    // Connect input
    std::unique_ptr<zcom::Panel> _connectPanel = nullptr;
    std::unique_ptr<zcom::Label> _connectPanelLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _connectIpInput = nullptr;
    std::unique_ptr<zcom::TextInput> _connectPortInput = nullptr;
    std::unique_ptr<zcom::Button> _connectConfirmButton = nullptr;
    std::unique_ptr<zcom::Button> _connectCancelButton = nullptr;
    bool _setConnectFocus = false;
    std::unique_ptr<zcom::Label> _recentConnectionsLabel = nullptr;
    std::unique_ptr<zcom::Panel> _recentConnectionsPanel = nullptr;
    std::unique_ptr<zcom::LoadingImage> _connectLoadingImage = nullptr;
    std::unique_ptr<zcom::Label> _connectLoadingInfoLabel = nullptr;
    std::unique_ptr<zcom::Button> _connectLoadingCancelButton = nullptr;
    bool _connectLoading = false;
    bool _connectPanelOpen = false;

    // Share input
    std::unique_ptr<zcom::Panel> _sharePanel = nullptr;
    std::unique_ptr<zcom::Label> _sharePanelLabel = nullptr;
    std::unique_ptr<zcom::TextInput> _sharePortInput = nullptr;
    std::unique_ptr<zcom::Button> _shareConfirmButton = nullptr;
    std::unique_ptr<zcom::Button> _shareCancelButton = nullptr;
    bool _setShareFocus = false;
    std::unique_ptr<zcom::LoadingImage> _shareLoadingImage = nullptr;
    std::unique_ptr<zcom::Label> _shareLoadingInfoLabel = nullptr;
    std::unique_ptr<zcom::Button> _shareLoadingCancelButton = nullptr;
    bool _shareLoading = false;
    std::string _shareFilename = "";
    bool _startServerPanelOpen = false;

    // File
    AsyncFileDialog _fileDialog;
    bool _fileLoading = false;


public:
    EntryScene();

    const char* GetName() const { return "entry"; }
    static const char* StaticName() { return "entry"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

    void OnConnectSelected();
    void OnShareSelected();
    void OnFileSelected();

    void OnConnectConfirmed();
    void OnConnectCanceled();
    void OnConnectLoadingCanceled();

    void OnShareConfirmed();
    void OnShareCanceled();
    void OnShareLoadingCanceled();
};