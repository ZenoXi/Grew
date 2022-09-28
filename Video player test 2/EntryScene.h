#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "EmptyPanel.h"
#include "Button.h"
#include "TextInput.h"
#include "Label.h"
#include "LoadingImage.h"
#include "PlaybackView.h"

#include "FileDialog.h"

struct EntrySceneOptions : public SceneOptionsBase
{

};

class EntryScene : public Scene
{
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Image> _logoImage = nullptr;
    std::unique_ptr<zcom::Label> _titleLabel = nullptr;
    std::unique_ptr<zcom::Button> _skipHomePageButton = nullptr;

    std::unique_ptr<zcom::Button> _connectButton = nullptr;
    std::unique_ptr<zcom::Button> _shareButton = nullptr;
    std::unique_ptr<zcom::Button> _fileButton = nullptr;
    std::unique_ptr<zcom::Button> _playlistButton = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _connectMarker = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _shareMarker = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _fileMarker = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _playlistMarker = nullptr;

    std::unique_ptr<zcom::Panel> _sidePanel = nullptr;
    std::unique_ptr<zcom::Label> _creditsLabel = nullptr;

    std::unique_ptr<zcom::Label> _connectInfoLabel = nullptr;
    std::unique_ptr<zcom::Label> _shareInfoLabel = nullptr;
    std::unique_ptr<zcom::Label> _fileInfoLabel = nullptr;
    std::unique_ptr<zcom::Label> _playlistInfoLabel = nullptr;

    std::unique_ptr<zcom::PlaybackView> _popoutPlaybackView = nullptr;
    
    bool _connectPanelOpen = false;
    bool _startServerPanelOpen = false;

    // File
    AsyncFileDialog _fileDialog;
    bool _fileLoading = false;


public:
    EntryScene(App* app);

    const char* GetName() const { return "entry"; }
    static const char* StaticName() { return "entry"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

    void OnConnectSelected();
    void OnShareSelected();
    void OnFileSelected();
    void OnPlaylistSelected();
};