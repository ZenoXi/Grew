#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "Button.h"
#include "TextInput.h"
#include "Label.h"
#include "LoadingImage.h"

struct EntrySceneOptions : public SceneOptionsBase
{

};

class EntryScene : public Scene
{
    // Main selection
    Panel* _mainPanel = nullptr;
    Button* _connectButton = nullptr;
    Button* _shareButton = nullptr;
    Button* _fileButton = nullptr;
    Label* _connectLabel = nullptr;
    Label* _shareLabel = nullptr;
    Label* _fileLabel = nullptr;

    // Connect input
    Panel* _connectPanel = nullptr;
    Label* _connectPanelLabel = nullptr;
    TextInput* _connectIpInput = nullptr;
    TextInput* _connectPortInput = nullptr;
    Button* _connectConfirmButton = nullptr;
    Button* _connectCancelButton = nullptr;
    bool _setConnectFocus = false;
    LoadingImage* _connectLoadingImage = nullptr;
    Label* _connectLoadingInfoLabel = nullptr;
    Button* _connectLoadingCancelButton = nullptr;
    bool _connectLoading = false;

    // Share input
    Panel* _sharePanel = nullptr;
    Label* _sharePanelLabel = nullptr;
    TextInput* _sharePortInput = nullptr;
    Button* _shareConfirmButton = nullptr;
    Button* _shareCancelButton = nullptr;
    bool _setShareFocus = false;
    LoadingImage* _shareLoadingImage = nullptr;
    Label* _shareLoadingInfoLabel = nullptr;
    Button* _shareLoadingCancelButton = nullptr;
    bool _shareLoading = false;
    std::string _shareFilename = "";


public:
    EntryScene();

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
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

    const char* GetName() const { return "entry_scene"; }
};