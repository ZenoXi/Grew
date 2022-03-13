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
    zcom::Panel* _mainPanel = nullptr;
    zcom::Button* _connectButton = nullptr;
    zcom::Button* _shareButton = nullptr;
    zcom::Button* _fileButton = nullptr;
    zcom::Label* _connectLabel = nullptr;
    zcom::Label* _shareLabel = nullptr;
    zcom::Label* _fileLabel = nullptr;
    zcom::Button* _testButton = nullptr;

    // Connect input
    zcom::Panel* _connectPanel = nullptr;
    zcom::Label* _connectPanelLabel = nullptr;
    zcom::TextInput* _connectIpInput = nullptr;
    zcom::TextInput* _connectPortInput = nullptr;
    zcom::Button* _connectConfirmButton = nullptr;
    zcom::Button* _connectCancelButton = nullptr;
    bool _setConnectFocus = false;
    zcom::LoadingImage* _connectLoadingImage = nullptr;
    zcom::Label* _connectLoadingInfoLabel = nullptr;
    zcom::Button* _connectLoadingCancelButton = nullptr;
    bool _connectLoading = false;

    // Share input
    zcom::Panel* _sharePanel = nullptr;
    zcom::Label* _sharePanelLabel = nullptr;
    zcom::TextInput* _sharePortInput = nullptr;
    zcom::Button* _shareConfirmButton = nullptr;
    zcom::Button* _shareCancelButton = nullptr;
    bool _setShareFocus = false;
    zcom::LoadingImage* _shareLoadingImage = nullptr;
    zcom::Label* _shareLoadingInfoLabel = nullptr;
    zcom::Button* _shareLoadingCancelButton = nullptr;
    bool _shareLoading = false;
    std::string _shareFilename = "";

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