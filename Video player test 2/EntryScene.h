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

    bool _connectPanelOpen = false;
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
};