#include "App.h" // App.h must be included first
#include "PlaybackOverlayScene.h"

PlaybackOverlayScene::PlaybackOverlayScene()
{
    _playbackScene = (PlaybackScene*)App::Instance()->FindScene(PlaybackScene::StaticName());
}

void PlaybackOverlayScene::_Init(const SceneOptionsBase* options)
{
    PlaybackOverlaySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PlaybackOverlaySceneOptions*>(options);
    }
    
    _playbackQueuePanel = std::make_unique<zcom::Panel>();
    _playbackQueuePanel->SetParentHeightPercent(1.0f);
    _playbackQueuePanel->SetBaseSize(600, -110);
    _playbackQueuePanel->SetOffsetPixels(40, 40);
    _playbackQueuePanel->SetBorderVisibility(true);
    _playbackQueuePanel->SetBorderColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    _playbackQueuePanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));

    //_mediaQueueItem = new zcom::MediaQueueItem(L"E:\\aots4e5.mkv");
    //_mediaQueueItem->SetBaseSize(200, 35);
    //_mediaQueueItem->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));

    _addFileButton = std::make_unique<zcom::Button>(L"Add file");
    _addFileButton->SetBaseSize(100, 25);
    _addFileButton->SetOffsetPixels(40, -40);
    _addFileButton->SetVerticalAlignment(zcom::Alignment::END);
    _addFileButton->SetBorderVisibility(true);
    _addFileButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _addFileButton->SetOnActivated([&]()
    {
        _addingFile = true;
        _fileDialog = new AsyncFileDialog();
        _fileDialog->Open();
    });

    _closeOverlayButton = std::make_unique<zcom::Button>(L"Close");
    _closeOverlayButton->SetBaseSize(100, 25);
    _closeOverlayButton->SetOffsetPixels(-40, 40);
    _closeOverlayButton->SetHorizontalAlignment(zcom::Alignment::END);
    _closeOverlayButton->SetBorderVisibility(true);
    _closeOverlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _closeOverlayButton->SetOnActivated([&]()
    {
        App::Instance()->MoveSceneToBack(this->GetName());
    });

    _canvas->AddComponent(_playbackQueuePanel.get());
    _canvas->AddComponent(_addFileButton.get());
    _canvas->AddComponent(_closeOverlayButton.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.75f));
}

void PlaybackOverlayScene::_Uninit()
{

}

void PlaybackOverlayScene::_Focus()
{

}

void PlaybackOverlayScene::_Unfocus()
{

}

void PlaybackOverlayScene::_Update()
{
    _playbackQueuePanel->Update();

    // Check for file dialog completion
    if (_addingFile)
    {
        if (_fileDialog->Done())
        {
            if (_fileDialog->Result() != L"")
            {
                auto newItem = std::make_unique<zcom::MediaQueueItem>(_fileDialog->Result());
                newItem->SetParentWidthPercent(1.0f);
                newItem->SetBaseHeight(25);
                _loadingItems.push_back(std::move(newItem));
                _RearrangeQueuePanel();
            }
            _addingFile = false;
            delete _fileDialog;
        }
    }

    // Manage queue items
    bool changed = false;
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        // Delete
        if (_loadingItems[i]->Delete())
        {
            _loadingItems.erase(_loadingItems.begin() + i);
            i--;
            changed = true;
            continue;
        }

        // Move from loading to ready
        if (_loadingItems[i]->Initialized() && _loadingItems[i]->InitSuccess())
        {
            _readyItems.push_back(std::move(_loadingItems[i]));
            _loadingItems.erase(_loadingItems.begin() + i);
            i--;
            changed = true;
            continue;
        }
    }
    for (int i = 0; i < _readyItems.size(); i++)
    {
        // Play
        if (_readyItems[i]->Play())
        {
            _currentlyPlaying = i;
            PlaybackSceneOptions options;
            options.dataProvider = _readyItems[_currentlyPlaying]->GetDataProvider();
            options.mode = PlaybackMode::OFFLINE;
            App::Instance()->UninitScene(PlaybackScene::StaticName());
            App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
            App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
            changed = true;
        }
        // Stop
        else if (_readyItems[i]->Stop())
        {
            _currentlyPlaying = -1;
            App::Instance()->UninitScene(PlaybackScene::StaticName());
            changed = true;
        }
        // Delete
        else if (_readyItems[i]->Delete())
        {
            if (_currentlyPlaying > i)
                _currentlyPlaying--;

            _readyItems.erase(_readyItems.begin() + i);
            i--;
            changed = true;
            continue;
        }
    }
    if (changed) _RearrangeQueuePanel();

    // Play next item
    if ((_autoplay && _playbackScene->Finished()) || _waiting)
    {
        bool uninitScene = true;
        _currentlyPlaying++;
        if (_currentlyPlaying == _readyItems.size())
        {
            _currentlyPlaying = -1;
            if (_waiting)
                uninitScene = false;
        }

        Scene* scene = nullptr;
        bool focused = false;
        if (uninitScene)
        {
            scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
            if (scene)
            {
                focused = scene->Focused();
                App::Instance()->UninitScene(PlaybackScene::StaticName());
            }
        }

        if (_currentlyPlaying != -1)
        {
            _waiting = false;
            PlaybackSceneOptions options;
            options.dataProvider = _readyItems[_currentlyPlaying]->GetDataProvider();
            options.mode = PlaybackMode::OFFLINE;
            App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
            if (focused)
                App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
            else
                App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
        }

        if (uninitScene)
            _RearrangeQueuePanel();
    }
}

ID2D1Bitmap1* PlaybackOverlayScene::_Draw(Graphics g)
{
    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void PlaybackOverlayScene::_Resize(int width, int height)
{

}

void PlaybackOverlayScene::AddItem(std::wstring filepath)
{
    auto newItem = std::make_unique<zcom::MediaQueueItem>(filepath);
    newItem->SetParentWidthPercent(1.0f);
    newItem->SetBaseHeight(25);
    _loadingItems.push_back(std::move(newItem));
    _RearrangeQueuePanel();

    //_readyItems.push_back(std::move(newItem));
    //_currentlyPlaying = _readyItems.size() - 1;

    //PlaybackSceneOptions options;
    //options.fileName = wstring_to_string(filepath);
    //options.mode = PlaybackMode::OFFLINE;
    //App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
    //App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
}

void PlaybackOverlayScene::WaitForLoad(bool focus)
{
    if (_readyItems.empty())
    {
        _waiting = true;
        _currentlyPlaying = -1;

        PlaybackSceneOptions options;
        options.placeholder = true;
        App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
        if (focus)
            App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
    }
}

void PlaybackOverlayScene::SetAutoplay(bool autoplay)
{
    _autoplay = autoplay;
}

bool PlaybackOverlayScene::GetAutoplay() const
{
    return _autoplay;
}

void PlaybackOverlayScene::_RearrangeQueuePanel()
{
    _playbackQueuePanel->ClearItems();
    for (int i = 0; i < _readyItems.size(); i++)
    {
        _playbackQueuePanel->AddItem(_readyItems[i].get());
        _readyItems[i]->SetVerticalOffsetPixels(25 * i);
        _readyItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        if (i == _currentlyPlaying)
        {
            _readyItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
            _readyItems[i]->SetNowPlaying(true);
        }
        else
        {
            _readyItems[i]->SetNowPlaying(false);
        }

    }
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        _playbackQueuePanel->AddItem(_loadingItems[i].get());
        _loadingItems[i]->SetVerticalOffsetPixels(25 * (i + _readyItems.size()));
    }
    _playbackQueuePanel->Resize();
}