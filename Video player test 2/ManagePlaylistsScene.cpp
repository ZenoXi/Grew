#include "App.h" // App.h must be included first
#include "ManagePlaylistsScene.h"

#include "OverlayScene.h"
#include "SavePlaylistScene.h"

#include "TextInput.h"
#include "ResourceManager.h"
#include "Functions.h"

#include <ShlObj.h>
#include <fstream>
#include <filesystem>

ManagePlaylistsScene::ManagePlaylistsScene(App* app)
    : Scene(app)
{}

void ManagePlaylistsScene::_Init(const SceneOptionsBase* options)
{
    ManagePlaylistsSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const ManagePlaylistsSceneOptions*>(options);
    }

    zcom::PROP_Shadow shadowProps;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 0.75f);

    // Main selection
    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(600, 500);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);

    _titleLabel = Create<zcom::Label>(L"Manage playlists");
    _titleLabel->SetBaseSize(500, 30);
    _titleLabel->SetOffsetPixels(30, 30);
    _titleLabel->SetFontSize(30.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _cancelButton = Create<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 30);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("item_delete"));
    _cancelButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });

    _titleSeparator = Create<zcom::EmptyPanel>();
    _titleSeparator->SetBaseSize(-60, 1);
    _titleSeparator->SetParentWidthPercent(1.0f);
    _titleSeparator->SetVerticalOffsetPixels(80);
    _titleSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _titleSeparator->SetBorderVisibility(true);
    _titleSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _titleSeparator->SetZIndex(0);

    _savedPlaylistPanel = Create<zcom::Panel>();
    _savedPlaylistPanel->SetBaseSize(-60, 340);
    _savedPlaylistPanel->SetParentWidthPercent(1.0f);
    _savedPlaylistPanel->SetOffsetPixels(30, 80);
    _savedPlaylistPanel->VerticalScrollable(true);

    _LoadSavedPlaylists();
    _RearrangeSavedPlaylists();

    _noPlaylistsLabel = Create<zcom::Label>(L"There are currently no saved playlists");
    _noPlaylistsLabel->SetBaseSize(400, 50);
    _noPlaylistsLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _noPlaylistsLabel->SetFontSize(18.0f);
    _noPlaylistsLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _noPlaylistsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _noPlaylistsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _noPlaylistsLabel->SetVisible(_savedPlaylists.empty());

    _bottomSeparator = Create<zcom::EmptyPanel>();
    _bottomSeparator->SetBaseSize(-60, 1);
    _bottomSeparator->SetParentWidthPercent(1.0f);
    _bottomSeparator->SetVerticalOffsetPixels(-80);
    _bottomSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _bottomSeparator->SetVerticalAlignment(zcom::Alignment::END);
    _bottomSeparator->SetBorderVisibility(true);
    _bottomSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    _bottomSeparator->SetZIndex(0);

    _browseFilesButton = Create<zcom::Button>(L"Import");
    _browseFilesButton->SetBaseSize(80, 30);
    _browseFilesButton->SetOffsetPixels(30, -30);
    _browseFilesButton->SetVerticalAlignment(zcom::Alignment::END);
    _browseFilesButton->Text()->SetFontSize(18.0f);
    _browseFilesButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
    _browseFilesButton->SetTabIndex(3);
    _browseFilesButton->SetCornerRounding(5.0f);
    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;
    _browseFilesButton->SetProperty(buttonShadow);
    _browseFilesButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _browseFilesButton->SetOnActivated([&]() { _ImportClicked(); });

    _mainPanel->AddItem(_titleLabel.get());
    _mainPanel->AddItem(_cancelButton.get());
    _mainPanel->AddItem(_titleSeparator.get());
    _mainPanel->AddItem(_savedPlaylistPanel.get());
    _mainPanel->AddItem(_noPlaylistsLabel.get());
    _mainPanel->AddItem(_bottomSeparator.get());
    _mainPanel->AddItem(_browseFilesButton.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));

    _closeScene = false;
    _playlistName = L"";
}

void ManagePlaylistsScene::_Uninit()
{
    _canvas->ClearComponents();
    _mainPanel->ClearItems();

    _mainPanel = nullptr;
    _titleLabel = nullptr;
    _cancelButton = nullptr;
    _titleSeparator = nullptr;
    _savedPlaylistPanel = nullptr;
    _noPlaylistsLabel = nullptr;
    _bottomSeparator = nullptr;
    _browseFilesButton = nullptr;
}

void ManagePlaylistsScene::_Focus()
{

}

void ManagePlaylistsScene::_Unfocus()
{

}

void ManagePlaylistsScene::_Update()
{
    _canvas->Update();

    if (_savePlaylistSceneOpen)
    {
        SavePlaylistScene* scene = (SavePlaylistScene*)App::Instance()->FindScene(SavePlaylistScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Save playlist scene incorrectly marked as open" << std::endl;
            _savePlaylistSceneOpen = false;
        }
        else if (scene->CloseScene())
        {
            if (!scene->PlaylistName().empty())
            {
                _LoadSavedPlaylists();
                _RearrangeSavedPlaylists();
            }
            App::Instance()->UninitScene(SavePlaylistScene::StaticName());
            _savePlaylistSceneOpen = false;
        }
    }

    if (_fileDialog && _fileDialog->Done())
    {
        if (_importingPlaylist)
        {
            auto results = _fileDialog->ParsedResult();
            if (!results.empty())
            {
                bool someFailed = false;
                for (auto& path : results)
                {
                    if (!_ImportPlaylist(path))
                        someFailed = true;
                }

                if (someFailed)
                {
                    zcom::NotificationInfo ninfo;
                    ninfo.duration = Duration(3, SECONDS);
                    ninfo.title = L"Playlist import";
                    ninfo.text = L"Failed to copy some files";
                    ninfo.borderColor = D2D1::ColorF(0.65f, 0.4f, 0.0f);
                    _app->Overlay()->ShowNotification(ninfo);
                }

                _LoadSavedPlaylists();
                _RearrangeSavedPlaylists();
            }
        }

        _fileDialog.reset();
    }
}

void ManagePlaylistsScene::_Resize(int width, int height)
{

}

std::optional<ManagePlaylistsScene::_Playlist> ManagePlaylistsScene::_LoadPlaylistFromFile(std::wstring path)
{
    // Process playlist file
    std::wifstream fin(path);
    if (!fin)
        return std::nullopt;

    _Playlist playlist;
    playlist.path = path;

    // Get playlist name
    std::getline(fin, playlist.name);
    if (playlist.name.empty())
        return std::nullopt;

    // Get item count
    while (!fin.eof())
    {
        std::wstring file;
        std::getline(fin, file);
        if (!file.empty())
            playlist.items.push_back(file);
    }
    if (playlist.items.size() == 0)
        return std::nullopt;

    return playlist;
}

bool ManagePlaylistsScene::_ImportPlaylist(std::wstring playlistpath)
{
    // Get %appdata% folder
    wchar_t* path_c;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_c) != S_OK) {
        CoTaskMemFree(path_c);
        return false;
    }
    std::filesystem::path path(path_c);
    CoTaskMemFree(path_c);

    path /= L"Grew";
    path /= L"Playlists";

    // Attempt to copy
    std::error_code ec;
    std::filesystem::copy(playlistpath, path, ec);
    return !ec;
}

void ManagePlaylistsScene::_LoadSavedPlaylists()
{
    _savedPlaylists.clear();

    // Get %appdata% folder
    wchar_t* path_c;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_c) != S_OK) {
        CoTaskMemFree(path_c);
        return;
    }
    std::filesystem::path path(path_c);
    CoTaskMemFree(path_c);

    path /= L"Grew";
    path /= L"Playlists";
    for (auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != L".grpl")
            continue;

        auto playlist = _LoadPlaylistFromFile(entry.path());
        if (playlist.has_value())
            _savedPlaylists.push_back(playlist.value());
    }
}

void ManagePlaylistsScene::_RearrangeSavedPlaylists()
{
    _savedPlaylistPanel->ClearItems();
    for (int i = 0; i < _savedPlaylists.size(); i++)
    {
        _Playlist playlist = _savedPlaylists[i];

        auto itemPanel = Create<zcom::Panel>();
        itemPanel->SetParentWidthPercent(1.0f);
        itemPanel->SetBaseHeight(25);
        itemPanel->SetVerticalOffsetPixels(i * 25);

        auto itemNameLabel = Create<zcom::Label>(_savedPlaylists[i].name);
        itemNameLabel->SetParentWidthPercent(1.0f);
        itemNameLabel->SetBaseSize(-50, 25);
        itemNameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        itemNameLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        itemNameLabel->SetFontSize(16.0f);
        itemNameLabel->SetCutoff(L"...");
        itemNameLabel->SetMargins({ 3.0f });

        auto renameButton = Create<zcom::Button>();
        renameButton->SetBaseSize(25, 25);
        renameButton->SetHorizontalAlignment(zcom::Alignment::END);
        renameButton->SetHorizontalOffsetPixels(-25);
        renameButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
        renameButton->SetButtonHoverColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));
        renameButton->SetButtonClickColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.4f));
        renameButton->SetButtonImageAll(ResourceManager::GetImage("edit"));
        renameButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        renameButton->SetActivation(zcom::ButtonActivation::RELEASE);
        renameButton->SetOnActivated([&, i]()
        {
            SavePlaylistSceneOptions opt;
            opt.openedPlaylistName = _savedPlaylists[i].name;
            opt.openedPlaylistPath = _savedPlaylists[i].path;
            _app->InitScene(SavePlaylistScene::StaticName(), &opt);
            _app->MoveSceneToFront(SavePlaylistScene::StaticName());
            _savePlaylistSceneOpen = true;
        });
        renameButton->SetVisible(false);

        auto deleteButton = Create<zcom::Button>();
        deleteButton->SetBaseSize(25, 25);
        deleteButton->SetHorizontalAlignment(zcom::Alignment::END);
        deleteButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
        deleteButton->SetButtonHoverColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));
        deleteButton->SetButtonClickColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.4f));
        deleteButton->SetButtonImageAll(ResourceManager::GetImage("delete"));
        deleteButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        deleteButton->SetActivation(zcom::ButtonActivation::RELEASE);
        deleteButton->SetOnActivated([&, i]()
        {
            _DeletePlaylist(i);
        });
        deleteButton->SetVisible(false);

        zcom::Button* renameButtonPtr = renameButton.get();
        zcom::Button* deleteButtonPtr = deleteButton.get();
        itemPanel->AddOnMouseEnterArea([&, renameButtonPtr, deleteButtonPtr](zcom::Base* item)
        {
            renameButtonPtr->SetVisible(true);
            deleteButtonPtr->SetVisible(true);
            item->SetBackgroundColor(D2D1::ColorF(0, 0.1f));
        });
        itemPanel->AddOnMouseLeaveArea([&, renameButtonPtr, deleteButtonPtr](zcom::Base* item)
        {
            renameButtonPtr->SetVisible(false);
            deleteButtonPtr->SetVisible(false);
            item->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
        });

        itemPanel->AddItem(itemNameLabel.release(), true);
        itemPanel->AddItem(renameButton.release(), true);
        itemPanel->AddItem(deleteButton.release(), true);
        _savedPlaylistPanel->AddItem(itemPanel.release(), true);
    }
}

void ManagePlaylistsScene::_DeletePlaylist(int index)
{
    namespace fs = std::filesystem;
    fs::path path = _savedPlaylists[index].path;

    // Attempt to delete file
    std::error_code ec;
    bool result = fs::remove(path, ec);
    if (!result)
    {
        // Show notification if failed
        zcom::NotificationInfo ninfo;
        ninfo.borderColor = D2D1::ColorF(0.8f, 0.2f, 0.2f);
        ninfo.duration = Duration(5, SECONDS);
        ninfo.title = L"Remove failed";
        ninfo.text = string_to_wstring(ec.message());
        _app->Overlay()->ShowNotification(ninfo);
        return;
    }

    // If successful, rearrange panel
    _savedPlaylists.erase(_savedPlaylists.begin() + index);
    _RearrangeSavedPlaylists();
}

void ManagePlaylistsScene::_ImportClicked()
{
    if (_fileDialog)
        return;

    _importingPlaylist = true;
    _fileDialog = std::make_unique<AsyncFileDialog>();
    FileDialogOptions opt;
    opt.allowedExtensions.push_back({ L"Grew playlist files", L"*.grpl" });
    opt.allowMultipleFiles = true;
    _fileDialog->Open(opt);
}

void ManagePlaylistsScene::_CancelClicked()
{
    _closeScene = true;
}