#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "Button.h"
#include "Label.h"
#include "EmptyPanel.h"

#include "FileDialog.h"

#include <optional>

struct OpenPlaylistSceneOptions : public SceneOptionsBase
{

};

class OpenPlaylistScene : public Scene
{
public:
    bool CloseScene() const { return _closeScene; };
    std::wstring PlaylistName() const { return _playlistName; }

    OpenPlaylistScene(App* app);

    const char* GetName() const { return "open_playlist"; }
    static const char* StaticName() { return "open_playlist"; }

private:
    struct _Playlist
    {
        std::wstring name;
        std::vector<std::wstring> items;
    };

    // Main selection
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Label> _titleLabel = nullptr;
    std::unique_ptr<zcom::Button> _cancelButton = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _titleSeparator = nullptr;
    std::unique_ptr<zcom::Label> _noPlaylistsLabel = nullptr;
    std::unique_ptr<zcom::Panel> _savedPlaylistPanel = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _bottomSeparator = nullptr;
    std::unique_ptr<zcom::Button> _browseFilesButton = nullptr;

    std::vector<_Playlist> _savedPlaylists;

    // File
    std::unique_ptr<AsyncFileDialog> _fileDialog;

    bool _closeScene = false;
    std::wstring _playlistName = L"";

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

    std::optional<OpenPlaylistScene::_Playlist> _LoadPlaylistFromFile(std::wstring path);
    void _LoadSavedPlaylists();
    void _BrowseClicked();
    void _OpenPlaylist(_Playlist playlist);
    void _CancelClicked();
};