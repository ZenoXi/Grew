#include "App.h" // App.h must be included first
#include "OpenPlaylistScene.h"

#include "ResourceManager.h"
#include "Functions.h"

#include <ShlObj.h>
#include <fstream>
#include <filesystem>

OpenPlaylistScene::OpenPlaylistScene(App* app)
    : Scene(app)
{}

void OpenPlaylistScene::_Init(const SceneOptionsBase* options)
{
    OpenPlaylistSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const OpenPlaylistSceneOptions*>(options);
    }

    zcom::PROP_Shadow shadowProps;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 0.75f);

    // Main selection
    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(500, 500);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);
    
    _titleLabel = Create<zcom::Label>(L"Saved playlists");
    _titleLabel->SetBaseSize(400, 30);
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
    _titleSeparator->SetBaseSize(440, 1);
    _titleSeparator->SetVerticalOffsetPixels(80);
    _titleSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _titleSeparator->SetBorderVisibility(true);
    _titleSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _LoadSavedPlaylists();

    _savedPlaylistPanel = Create<zcom::Panel>();
    _savedPlaylistPanel->SetBaseSize(440, 340);
    _savedPlaylistPanel->SetOffsetPixels(30, 80);
    _savedPlaylistPanel->VerticalScrollable(true);

    for (int i = 0; i < _savedPlaylists.size(); i++)
    {
        auto itemPanel = Create<zcom::Panel>();
        itemPanel->SetParentWidthPercent(1.0f);
        itemPanel->SetBaseHeight(25);
        itemPanel->SetVerticalOffsetPixels(i * 25);

        auto itemButton = Create<zcom::Button>();
        itemButton->SetParentWidthPercent(1.0f);
        itemButton->SetBaseHeight(30);
        itemButton->SetZIndex(0);
        itemButton->SetPreset(zcom::ButtonPreset::NO_EFFECTS);
        itemButton->SetButtonHoverColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f));
        itemButton->SetButtonClickColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));
        itemButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        itemButton->SetActivation(zcom::ButtonActivation::RELEASE);
        _Playlist playlist = _savedPlaylists[i];
        itemButton->SetOnActivated([&, playlist]()
        {
            _OpenPlaylist(playlist);
            _closeScene = true;
            _playlistName = playlist.name;
        });

        auto itemNameLabel = Create<zcom::Label>(_savedPlaylists[i].name);
        itemNameLabel->SetParentWidthPercent(1.0f);
        itemNameLabel->SetBaseSize(-70, 25);
        itemNameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        itemNameLabel->SetHorizontalTextAlignment(zcom::TextAlignment::LEADING);
        itemNameLabel->SetFontSize(16.0f);
        itemNameLabel->SetCutoff(L"...");
        itemNameLabel->SetMargins({ 3.0f });

        std::wostringstream ss(L"");
        ss << _savedPlaylists[i].items.size() << " items";
        auto itemCountLabel = Create<zcom::Label>(ss.str());
        itemCountLabel->SetBaseSize(70, 25);
        itemCountLabel->SetHorizontalAlignment(zcom::Alignment::END);
        itemCountLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        itemCountLabel->SetHorizontalTextAlignment(zcom::TextAlignment::TRAILING);
        itemCountLabel->SetFontSize(14.0f);
        itemCountLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));
        itemCountLabel->SetMargins({ 0.0f, 0.0f, 3.0f });

        itemPanel->AddItem(itemButton.release(), true);
        itemPanel->AddItem(itemNameLabel.release(), true);
        itemPanel->AddItem(itemCountLabel.release(), true);
        _savedPlaylistPanel->AddItem(itemPanel.release(), true);
    }

    _noPlaylistsLabel = Create<zcom::Label>(L"There are currently no saved playlists");
    _noPlaylistsLabel->SetBaseSize(400, 50);
    _noPlaylistsLabel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _noPlaylistsLabel->SetFontSize(18.0f);
    _noPlaylistsLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _noPlaylistsLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _noPlaylistsLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _noPlaylistsLabel->SetVisible(_savedPlaylists.empty());

    _bottomSeparator = Create<zcom::EmptyPanel>();
    _bottomSeparator->SetBaseSize(440, 1);
    _bottomSeparator->SetVerticalOffsetPixels(-80);
    _bottomSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _bottomSeparator->SetVerticalAlignment(zcom::Alignment::END);
    _bottomSeparator->SetBorderVisibility(true);
    _bottomSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _browseFilesButton = Create<zcom::Button>(L"Browse");
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
    _browseFilesButton->SetOnActivated([&]() { _BrowseClicked(); });

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

void OpenPlaylistScene::_Uninit()
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

void OpenPlaylistScene::_Focus()
{

}

void OpenPlaylistScene::_Unfocus()
{

}

void OpenPlaylistScene::_Update()
{
    _canvas->Update();

    if (_fileDialog && _fileDialog->Done())
    {
        std::wstring path = _fileDialog->Result();
        if (!path.empty())
        {
            auto playlist = _LoadPlaylistFromFile(path);
            if (playlist.has_value())
                _OpenPlaylist(playlist.value());
            _closeScene = true;
            _playlistName = playlist.value().name;
        }
        _fileDialog.reset();
    }
}

void OpenPlaylistScene::_Resize(int width, int height)
{

}

std::optional<OpenPlaylistScene::_Playlist> OpenPlaylistScene::_LoadPlaylistFromFile(std::wstring path)
{
    // Process playlist file
    std::wifstream fin(path);
    if (!fin)
        return std::nullopt;

    _Playlist playlist;

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

void OpenPlaylistScene::_LoadSavedPlaylists()
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

void OpenPlaylistScene::_BrowseClicked()
{
    if (_fileDialog)
        return;

    _fileDialog = std::make_unique<AsyncFileDialog>();
    FileDialogOptions opt;
    opt.openFolders = false;
    opt.allowMultipleFiles = false;
    opt.allowedExtensions.push_back({ L"Grew playlist files", L"*.grpl" });
    _fileDialog->Open(opt);
}

void OpenPlaylistScene::_OpenPlaylist(_Playlist playlist)
{
    for (int i = 0; i < playlist.items.size(); i++)
    {
        auto item = std::make_unique<PlaylistItem>(playlist.items[i]);
        _app->playlist.Request_AddItem(std::move(item));
    }
}

void OpenPlaylistScene::_CancelClicked()
{
    _closeScene = true;
}