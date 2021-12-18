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
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));
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

    bool changed = false;
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        if (_loadingItems[i]->Delete())
        {
            _loadingItems.erase(_loadingItems.begin() + i);
            i--;
            changed = true;
            continue;
        }

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
        if (_currentlyPlaying == i)
            continue;

        if (_readyItems[i]->Play())
        {
            _currentlyPlaying = i;
            PlaybackSceneOptions options;
            options.dataProvider = _readyItems[_currentlyPlaying]->GetDataProvider();
            options.mode = PlaybackMode::OFFLINE;
            App::Instance()->UninitScene(PlaybackScene::StaticName());
            App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
            App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
            _RearrangeQueuePanel();
        }
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
    if (_playbackScene->Finished())
    {
        bool focused = App::Instance()->FindActiveScene(PlaybackScene::StaticName())->Focused();
        App::Instance()->UninitScene(PlaybackScene::StaticName());

        _currentlyPlaying++;
        if (_currentlyPlaying == _readyItems.size())
        {
            _currentlyPlaying = -1;
        }
        else
        {
            PlaybackSceneOptions options;
            options.dataProvider = _readyItems[_currentlyPlaying]->GetDataProvider();
            options.mode = PlaybackMode::OFFLINE;
            App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
            if (focused)
                App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
            else
                App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
        }

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

void PlaybackOverlayScene::_RearrangeQueuePanel()
{
    _playbackQueuePanel->ClearItems();
    for (int i = 0; i < _readyItems.size(); i++)
    {
        _playbackQueuePanel->AddItem(_readyItems[i].get());
        _readyItems[i]->SetVerticalOffsetPixels(25 * i);
        _readyItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        if (i == _currentlyPlaying)
            _readyItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.1f));
    }
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        _playbackQueuePanel->AddItem(_loadingItems[i].get());
        _loadingItems[i]->SetVerticalOffsetPixels(25 * (i + _readyItems.size()));
    }
    _playbackQueuePanel->Resize();
}