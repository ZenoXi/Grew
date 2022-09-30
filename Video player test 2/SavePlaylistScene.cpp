#include "App.h" // App.h must be included first
#include "SavePlaylistScene.h"
#include "OverlayScene.h"

#include "ResourceManager.h"
#include "Functions.h"

#include <ShlObj.h>
#include <fstream>
#include <filesystem>

SavePlaylistScene::SavePlaylistScene(App* app)
    : Scene(app)
{}

void SavePlaylistScene::_Init(const SceneOptionsBase* options)
{
    SavePlaylistSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const SavePlaylistSceneOptions*>(options);
    }

    _renaming = false;
    if (!opt.openedPlaylistPath.empty())
    {
        _renaming = true;
        _playlistPath = opt.openedPlaylistPath;
    }
    if (!opt.selectedItemPaths.empty())
    {
        _selectedItemPaths = opt.selectedItemPaths;
    }

    zcom::PROP_Shadow shadowProps;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 0.75f);

    // Main selection
    _mainPanel = Create<zcom::Panel>();
    if (opt.showPartialSaveWarning)
        _mainPanel->SetBaseSize(500, 310);
    else
        _mainPanel->SetBaseSize(500, 280);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);

    if (_renaming)
        _titleLabel = Create<zcom::Label>(L"Rename playlist");
    else
        _titleLabel = Create<zcom::Label>(L"Save playlist");
    _titleLabel->SetBaseSize(400, 30);
    _titleLabel->SetOffsetPixels(30, 30);
    _titleLabel->SetFontSize(30.0f);
    _titleLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    _cancelButton = Create<zcom::Button>();
    _cancelButton->SetBaseSize(30, 30);
    _cancelButton->SetOffsetPixels(-30, 30);
    _cancelButton->SetHorizontalAlignment(zcom::Alignment::END);
    _cancelButton->SetBackgroundImage(ResourceManager::GetImage("close_100x100"));
    _cancelButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _cancelButton->SetOnActivated([&]() { _CancelClicked(); });
    _cancelButton->SetTabIndex(1000);

    _titleSeparator = Create<zcom::EmptyPanel>();
    _titleSeparator->SetBaseSize(-60, 1);
    _titleSeparator->SetParentWidthPercent(1.0f);
    _titleSeparator->SetVerticalOffsetPixels(80);
    _titleSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _titleSeparator->SetBorderVisibility(true);
    _titleSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _playlistNameInput = Create<zcom::TextInput>();
    _playlistNameInput->SetBaseSize(-60, 30);
    _playlistNameInput->SetParentWidthPercent(1.0f);
    _playlistNameInput->SetOffsetPixels(30, 100);
    _playlistNameInput->PlaceholderText()->SetText(L"Playlist name");
    _playlistNameInput->Text()->SetText(opt.openedPlaylistName);
    _playlistNameInput->SetCornerRounding(5.0f);
    _playlistNameInput->SetTabIndex(0);
    //_playlistNameInput->SetPattern(L"[^/\\:\*\?\"<>\|]*");

    _playlistFilenameLabel = Create<zcom::Label>();
    _playlistFilenameLabel->SetBaseSize(-60, 20);
    _playlistFilenameLabel->SetParentWidthPercent(1.0f);
    _playlistFilenameLabel->SetOffsetPixels(30, 130);
    _playlistFilenameLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _playlistFilenameLabel->SetFontSize(14.0f);
    _playlistFilenameLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    _playlistFilenameLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));
    _playlistFilenameLabel->SetCutoff(L"...");
    _playlistFilenameLabel->SetMargins({ 2.0f });

    _overwriteExistingCheckbox = Create<zcom::Checkbox>(opt.openedPlaylistName.empty() ? false : true);
    _overwriteExistingCheckbox->SetBaseSize(20, 20);
    _overwriteExistingCheckbox->SetOffsetPixels(30, 160);
    _overwriteExistingCheckbox->SetTabIndex(1);

    _overwriteExistingLabel = Create<zcom::Label>(L"Overwrite existing file");
    _overwriteExistingLabel->SetBaseSize(200, 20);
    _overwriteExistingLabel->SetOffsetPixels(60, 159);
    _overwriteExistingLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

    if (opt.showPartialSaveWarning)
    {
        _partialSaveWarningSeparator = Create<zcom::EmptyPanel>();
        _partialSaveWarningSeparator->SetBaseSize(-60, 1);
        _partialSaveWarningSeparator->SetParentWidthPercent(1.0f);
        _partialSaveWarningSeparator->SetVerticalOffsetPixels(199);
        _partialSaveWarningSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
        _partialSaveWarningSeparator->SetBorderVisibility(true);
        _partialSaveWarningSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

        _partialSaveWarningIcon = Create<zcom::Image>(ResourceManager::GetImage("warning_19x19"));
        _partialSaveWarningIcon->SetBaseSize(19, 19);
        _partialSaveWarningIcon->SetOffsetPixels(31, 205);
        _partialSaveWarningIcon->SetTintColor(D2D1::ColorF(0.8f, 0.65f, 0.0f));

        _partialSaveWarningLabel = Create<zcom::Label>(L"Items hosted by other users will not be saved");
        _partialSaveWarningLabel->SetBaseSize(-88, 20);
        _partialSaveWarningLabel->SetParentWidthPercent(1.0f);
        _partialSaveWarningLabel->SetOffsetPixels(58, 204);
        _partialSaveWarningLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _partialSaveWarningLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
    }

    _bottomSeparator = Create<zcom::EmptyPanel>();
    _bottomSeparator->SetBaseSize(-60, 1);
    _bottomSeparator->SetParentWidthPercent(1.0f);
    _bottomSeparator->SetVerticalOffsetPixels(-80);
    _bottomSeparator->SetHorizontalAlignment(zcom::Alignment::CENTER);
    _bottomSeparator->SetVerticalAlignment(zcom::Alignment::END);
    _bottomSeparator->SetBorderVisibility(true);
    _bottomSeparator->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));

    _saveButton = Create<zcom::Button>(L"Save");
    _saveButton->SetBaseSize(80, 30);
    _saveButton->SetOffsetPixels(-30, -30);
    _saveButton->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _saveButton->Text()->SetFontSize(18.0f);
    _saveButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.3f));
    _saveButton->SetTabIndex(2);
    _saveButton->SetCornerRounding(5.0f);
    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;
    _saveButton->SetProperty(buttonShadow);
    _saveButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _saveButton->SetOnActivated([&]() { _SaveClicked(); });

    _saveErrorLabel = Create<zcom::Label>(L"");
    _saveErrorLabel->SetParentWidthPercent(1.0f);
    _saveErrorLabel->SetBaseSize(-160, 30);
    _saveErrorLabel->SetOffsetPixels(-130, -30);
    _saveErrorLabel->SetAlignment(zcom::Alignment::END, zcom::Alignment::END);
    _saveErrorLabel->SetHorizontalTextAlignment(zcom::TextAlignment::TRAILING);
    _saveErrorLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _saveErrorLabel->SetFontColor(D2D1::ColorF(0.8f, 0.2f, 0.2f));
    _saveErrorLabel->SetFontSize(16.0f);
    _saveErrorLabel->SetCutoff(L"...");
    _saveErrorLabel->SetVisible(false);

    _mainPanel->AddItem(_titleLabel.get());
    _mainPanel->AddItem(_cancelButton.get());
    _mainPanel->AddItem(_titleSeparator.get());
    _mainPanel->AddItem(_playlistNameInput.get());
    _mainPanel->AddItem(_playlistFilenameLabel.get());
    _mainPanel->AddItem(_overwriteExistingCheckbox.get());
    _mainPanel->AddItem(_overwriteExistingLabel.get());
    if (opt.showPartialSaveWarning)
    {
        _mainPanel->AddItem(_partialSaveWarningSeparator.get());
        _mainPanel->AddItem(_partialSaveWarningIcon.get());
        _mainPanel->AddItem(_partialSaveWarningLabel.get());
    }
    _mainPanel->AddItem(_bottomSeparator.get());
    _mainPanel->AddItem(_saveErrorLabel.get());
    _mainPanel->AddItem(_saveButton.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));
    _UpdateFilenameLabel();

    _closeScene = false;
    _playlistName = L"";
}

void SavePlaylistScene::_Uninit()
{
    _canvas->ClearComponents();
    _mainPanel->ClearItems();

    _mainPanel = nullptr;
    _titleLabel = nullptr;
    _cancelButton = nullptr;
    _titleSeparator = nullptr;
    _playlistNameInput = nullptr;
    _playlistFilenameLabel = nullptr;
    _overwriteExistingCheckbox = nullptr;
    _overwriteExistingLabel = nullptr;
    _partialSaveWarningSeparator = nullptr;
    _partialSaveWarningIcon = nullptr;
    _partialSaveWarningLabel = nullptr;
    _bottomSeparator = nullptr;
    _saveButton = nullptr;
    _saveErrorLabel = nullptr;
}

void SavePlaylistScene::_Focus()
{

}

void SavePlaylistScene::_Unfocus()
{

}

void SavePlaylistScene::_Update()
{
    _canvas->Update();

    _UpdateFilenameLabel();
}

void SavePlaylistScene::_Resize(int width, int height)
{

}

void SavePlaylistScene::_UpdateFilenameLabel()
{
    std::wstring text = _playlistNameInput->Text()->GetText();
    if (text.empty())
    {
        _playlistFilenameLabel->SetText(L"Filename: \"Playlist {first available number}\"");
    }
    else
    {
        // TODO: use text input regex (once implemented)
        // Sanitize input
        for (int i = 0; i < text.length(); i++)
        {
            if (text[i] == L'/' ||
                text[i] == L'\\' ||
                text[i] == L':' ||
                text[i] == L'*' ||
                text[i] == L'?' ||
                text[i] == L'"' ||
                text[i] == L'<' ||
                text[i] == L'>' ||
                text[i] == L'|')
            {
                text.erase(text.begin() + i);
                i--;
            }
        }

        std::wostringstream ss(L"");
        ss << L"Filename: \"";
        ss << text;
        ss << L'"';
        _playlistFilenameLabel->SetText(ss.str());
    }
}

void SavePlaylistScene::_SaveClicked()
{
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

    // Create list of playlist files
    std::vector<std::wstring> files;
    for (auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != L".grpl")
            continue;
        files.push_back(entry.path().filename());
    }

    //////////////////
    // Create filename
    //////////////////

    std::wstring filename = _playlistNameInput->Text()->GetText();
    std::wstring playlistName;
    if (filename.empty())
    {
        // Find first available unused filename
        int counter = 0;
        while (true)
        {
            counter++;
            std::wostringstream filenamess(L"");
            filenamess << L"Playlist " << counter;
            playlistName = filenamess.str();
            filenamess << L".grpl";

            if (std::find(files.begin(), files.end(), filenamess.str()) != files.end())
                continue;

            filename = filenamess.str();
            break;
        }
    }
    else
    {
        playlistName = filename;

        // Sanitize input
        for (int i = 0; i < filename.length(); i++)
        {
            if (filename[i] == L'/' ||
                filename[i] == L'\\' ||
                filename[i] == L':' ||
                filename[i] == L'*' ||
                filename[i] == L'?' ||
                filename[i] == L'"' ||
                filename[i] == L'<' ||
                filename[i] == L'>' ||
                filename[i] == L'|')
            {
                filename.erase(filename.begin() + i);
                i--;
            }
        }
        filename = filename + L".grpl";

        // Check for name collision
        if (!_overwriteExistingCheckbox->Checked())
        {
            if (std::find(files.begin(), files.end(), filename) != files.end())
            {
                _saveErrorLabel->SetText(L"File already exists");
                _saveErrorLabel->SetVisible(true);
                return;
            }
        }
    }
    path /= filename;

    ///////////////////////
    // Write data to buffer
    ///////////////////////

    std::wostringstream outbuf;
    outbuf << playlistName << std::endl;
    if (_renaming)
    {
        std::wifstream fin(_playlistPath);
        if (!fin)
        {
            std::string errstr;
            errstr.resize(1024);
            strerror_s(errstr.data(), 1023, errno);
            while (!errstr.empty() && errstr.back() == '\0')
                errstr.erase(errstr.end() - 1);
            errstr.resize(errstr.length() + 1);
            _saveErrorLabel->SetText(string_to_wstring(errstr));
            _saveErrorLabel->SetVisible(true);
            return;
        }

        std::wstring line;
        std::getline(fin, line);

        while (!fin.eof())
        {
            std::getline(fin, line);
            if (!line.empty())
                outbuf << line << std::endl;
        }
        fin.close();
    }
    else
    {
        if (_selectedItemPaths.empty())
        {
            auto readyItems = _app->playlist.ReadyItems();
            auto loadingItems = _app->playlist.LoadingItems();

            bool pathWritten = false;
            // Write to out buf
            for (int i = 0; i < readyItems.size(); i++)
            {
                std::wstring path = readyItems[i]->GetFilePath();
                if (!path.empty())
                {
                    outbuf << path << std::endl;
                    pathWritten = true;
                }
            }
            for (int i = 0; i < loadingItems.size(); i++)
            {
                std::wstring path = loadingItems[i]->GetFilePath();
                if (!path.empty())
                {
                    outbuf << path << std::endl;
                    pathWritten = true;
                }
            }

            // If nothing was written, show error message
            if (!pathWritten)
            {
                _saveErrorLabel->SetText(L"The playlist has no items that can be saved");
                _saveErrorLabel->SetVisible(true);
                return;
            }
        }
        else
        {
            for (int i = 0; i < _selectedItemPaths.size(); i++)
                outbuf << _selectedItemPaths[i] << std::endl;
        }
    }

    /////////////////////////////
    // Write new playlist to file
    /////////////////////////////

    std::wofstream fout(path);
    if (!fout)
    {
        std::string errstr;
        errstr.resize(1024);
        strerror_s(errstr.data(), 1023, errno);
        while (!errstr.empty() && errstr.back() == '\0')
            errstr.erase(errstr.end() - 1);
        errstr.resize(errstr.length() + 1);
        _saveErrorLabel->SetText(string_to_wstring(errstr));
        _saveErrorLabel->SetVisible(true);
        return;
    }
    fout << outbuf.str();
    fout.close();

    // Attempt to delete old file
    if (_renaming && _playlistPath != path)
    {
        std::error_code ec;
        bool result = std::filesystem::remove(_playlistPath, ec);
        if (!result)
        {
            // Show notification if failed
            zcom::NotificationInfo ninfo;
            ninfo.borderColor = D2D1::ColorF(0.8f, 0.2f, 0.2f);
            ninfo.duration = Duration(5, SECONDS);
            ninfo.title = L"Failed to delete old file";
            ninfo.text = string_to_wstring(ec.message());
            _app->Overlay()->ShowNotification(ninfo);
        }
    }

    // Show success notification
    _closeScene = true;
    _playlistName = playlistName;
    zcom::NotificationInfo ninfo;
    ninfo.duration = Duration(3, SECONDS);
    ninfo.title = L"Playlist saved as:";
    ninfo.text = playlistName;
    ninfo.borderColor = D2D1::ColorF(0.2f, 0.65f, 0.1f);
    _app->Overlay()->ShowNotification(ninfo);
}

void SavePlaylistScene::_CancelClicked()
{
    _closeScene = true;
}