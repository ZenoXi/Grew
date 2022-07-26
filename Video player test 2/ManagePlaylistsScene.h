#pragma once

#include "Scene.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "Button.h"
#include "Label.h"
#include "EmptyPanel.h"

#include "FileDialog.h"

#include <optional>

struct ManagePlaylistsSceneOptions : public SceneOptionsBase
{

};

class ManagePlaylistsScene : public Scene
{
public:
    bool CloseScene() const { return _closeScene; };

    ManagePlaylistsScene(App* app);

    const char* GetName() const { return "manage_playlists"; }
    static const char* StaticName() { return "manage_playlists"; }

private:
    struct _Playlist
    {
        std::wstring path;
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

    bool _savePlaylistSceneOpen = false;
    std::unique_ptr<AsyncFileDialog> _fileDialog = nullptr;
    bool _importingPlaylist = false;

    bool _closeScene = false;
    std::wstring _playlistName = L"";

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);

    std::optional<ManagePlaylistsScene::_Playlist> _LoadPlaylistFromFile(std::wstring path);
    bool _ImportPlaylist(std::wstring playlistpath);
    void _LoadSavedPlaylists();
    void _RearrangeSavedPlaylists();
    void _DeletePlaylist(int index);
    void _ImportClicked();
    void _CancelClicked();
};