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

    zcom::PROP_Shadow shadowProps;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 0.75f);

    // Main selection
    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(500, 280);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);
    _mainPanel->SetProperty(mainPanelShadow);

    _titleLabel = Create<zcom::Label>(L"Save playlist");
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
    _playlistNameInput->SetPlaceholderText(L"Playlist name");
    _playlistNameInput->SetText(opt.openedPlaylistName);
    _playlistNameInput->SetCornerRounding(5.0f);

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

    _overwriteExistingLabel = Create<zcom::Label>(L"Overwrite existing file");
    _overwriteExistingLabel->SetBaseSize(200, 20);
    _overwriteExistingLabel->SetOffsetPixels(60, 159);
    _overwriteExistingLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);

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
    _saveButton->SetTabIndex(3);
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
    _mainPanel->AddItem(_bottomSeparator.get());
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
    std::wstring text = _playlistNameInput->GetText();
    if (text.empty())
    {
        _playlistFilenameLabel->SetText(L"Filename: \"Playlist {first available number}\"");
    }
    else
    {
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

    std::wstring filename = _playlistNameInput->GetText();
    if (filename.empty())
    {
        // Find first available unused filename
        int counter = 0;
        while (true)
        {
            counter++;
            std::wostringstream filenamess(L"");
            filenamess << L"Playlist " << counter << L".grpl";

            if (std::find(files.begin(), files.end(), filenamess.str()) != files.end())
                continue;

            // Create and write to file
            path /= filenamess.str();
            std::wofstream fout(path.wstring());
            if (!fout)
            {
                std::string errstr;
                errstr.resize(256);
                strerror_s(errstr.data(), 255, errno);
                while (errstr.back() == '\0')
                    errstr.erase(errstr.end() - 1);
                _saveErrorLabel->SetText(string_to_wstring(errstr));
                _saveErrorLabel->SetVisible(true);
                return;
            }

            fout << L"Playlist " << counter << std::endl;
            auto readyItems = _app->playlist.ReadyItems();
            auto loadingItems = _app->playlist.LoadingItems();
            for (int i = 0; i < readyItems.size(); i++)
                fout << readyItems[i]->GetFilePath() << std::endl;
            for (int i = 0; i < loadingItems.size(); i++)
                fout << loadingItems[i]->GetFilePath() << std::endl;
            fout.close();

            _closeScene = true;
            zcom::NotificationInfo ninfo;
            ninfo.duration = Duration(3, SECONDS);
            ninfo.title = L"Playlist saved as:";
            filenamess.clear();
            filenamess << L"Playlist " << counter;
            ninfo.text = filenamess.str();
            ninfo.borderColor = D2D1::ColorF(0.2f, 0.65f, 0.1f);
            _app->Overlay()->ShowNotification(ninfo);
            return;
        }
    }
    else
    {
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

        // Create and write to file
        path /= filename;
        std::wofstream fout(path.wstring());
        if (!fout)
        {
            std::string errstr;
            errstr.resize(256);
            strerror_s(errstr.data(), 255, errno);
            while (errstr.back() == '\0')
                errstr.erase(errstr.end() - 1);
            _saveErrorLabel->SetText(string_to_wstring(errstr));
            _saveErrorLabel->SetVisible(true);
            return;
        }

        fout << _playlistNameInput->GetText() << std::endl;
        auto readyItems = _app->playlist.ReadyItems();
        auto loadingItems = _app->playlist.LoadingItems();
        for (int i = 0; i < readyItems.size(); i++)
            fout << readyItems[i]->GetFilePath() << std::endl;
        for (int i = 0; i < loadingItems.size(); i++)
            fout << loadingItems[i]->GetFilePath() << std::endl;
        fout.close();

        _closeScene = true;
        _playlistName = _playlistNameInput->GetText();
        zcom::NotificationInfo ninfo;
        ninfo.duration = Duration(3, SECONDS);
        ninfo.title = L"Playlist saved as:";
        ninfo.text = _playlistNameInput->GetText();
        ninfo.borderColor = D2D1::ColorF(0.2f, 0.65f, 0.1f);
        _app->Overlay()->ShowNotification(ninfo);
        return;
    }
}

void SavePlaylistScene::_CancelClicked()
{
    _closeScene = true;
}