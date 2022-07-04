#include "App.h" // App.h must be included first
#include "EntryScene.h"

#include "ResourceManager.h"
#include "Options.h"
#include "LastIpOptionAdapter.h"
#include "Functions.h"
#include "ConnectScene.h"
#include "StartServerScene.h"
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "Network.h"

EntryScene::EntryScene() {}

void EntryScene::_Init(const SceneOptionsBase* options)
{
    EntrySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const EntrySceneOptions*>(options);
    }

    zcom::PROP_Shadow shadowProps;
    shadowProps.blurStandardDeviation = 5.0f;
    shadowProps.color = D2D1::ColorF(0, 0.75f);

    // Main selection
    _mainPanel = std::make_unique<zcom::Panel>();
    {
        _mainPanel->SetHorizontalOffsetPercent(0.5f);
        _mainPanel->SetVerticalOffsetPercent(0.5f);
        _mainPanel->SetBaseWidth(380);
        _mainPanel->SetBaseHeight(180);
        _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f));
        //_mainPanel->SetBackgroundColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.25f));
        _mainPanel->VerticalScrollable(true);
        _mainPanel->HorizontalScrollable(true);
        _mainPanel->SetCornerRounding(5.0f);
        _mainPanel->SetProperty(shadowProps);

        _connectButton = std::make_unique<zcom::Button>();
        _connectButton->SetHorizontalOffsetPixels(20);
        _connectButton->SetVerticalOffsetPixels(20);
        _connectButton->SetBaseWidth(100);
        _connectButton->SetBaseHeight(100);
        _connectButton->SetButtonImage(ResourceManager::GetImage("connect_dim"));
        _connectButton->SetButtonHoverImage(ResourceManager::GetImage("connect_bright"));
        _connectButton->SetButtonClickImage(ResourceManager::GetImage("connect_bright"));
        _connectButton->SetButtonHoverColor(D2D1::ColorF(0, 0.0f));
        _connectButton->SetOnActivated([&]() { OnConnectSelected(); });
        _connectButton->SetTabIndex(0);

        _connectLabel = std::make_unique<zcom::Label>(L"Connect");
        _connectLabel->SetHorizontalOffsetPixels(20);
        _connectLabel->SetVerticalOffsetPixels(120);
        _connectLabel->SetBaseWidth(100);
        _connectLabel->SetBaseHeight(30);
        _connectLabel->SetFontSize(24.0f);
        _connectLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _connectLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _connectLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _shareButton = std::make_unique<zcom::Button>();
        _shareButton->SetHorizontalOffsetPixels(140);
        _shareButton->SetVerticalOffsetPixels(20);
        _shareButton->SetBaseWidth(100);
        _shareButton->SetBaseHeight(100);
        _shareButton->SetButtonImage(ResourceManager::GetImage("share_dim"));
        _shareButton->SetButtonHoverImage(ResourceManager::GetImage("share_bright"));
        _shareButton->SetButtonClickImage(ResourceManager::GetImage("share_bright"));
        _shareButton->SetButtonHoverColor(D2D1::ColorF(0, 0.0f));
        _shareButton->SetOnActivated([&]() { OnShareSelected(); });
        _shareButton->SetTabIndex(1);

        _shareLabel = std::make_unique<zcom::Label>(L"Share");
        _shareLabel->SetHorizontalOffsetPixels(140);
        _shareLabel->SetVerticalOffsetPixels(120);
        _shareLabel->SetBaseWidth(100);
        _shareLabel->SetBaseHeight(30);
        _shareLabel->SetFontSize(24.0f);
        _shareLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _shareLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _shareLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _fileButton = std::make_unique<zcom::Button>();
        _fileButton->SetHorizontalOffsetPixels(260);
        _fileButton->SetVerticalOffsetPixels(20);
        _fileButton->SetBaseWidth(100);
        _fileButton->SetBaseHeight(100);
        _fileButton->SetButtonImage(ResourceManager::GetImage("file_dim"));
        _fileButton->SetButtonHoverImage(ResourceManager::GetImage("file_bright"));
        _fileButton->SetButtonClickImage(ResourceManager::GetImage("file_bright"));
        _fileButton->SetButtonHoverColor(D2D1::ColorF(0, 0.0f));
        _fileButton->SetOnActivated([&]() { OnFileSelected(); });
        _fileButton->SetTabIndex(2);

        _fileLabel = std::make_unique<zcom::Label>(L"Offline");
        _fileLabel->SetHorizontalOffsetPixels(260);
        _fileLabel->SetVerticalOffsetPixels(120);
        _fileLabel->SetBaseWidth(100);
        _fileLabel->SetBaseHeight(30);
        _fileLabel->SetFontSize(24.0f);
        _fileLabel->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
        _fileLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
        _fileLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);

        _testButton = std::make_unique<zcom::Button>(L"Hi");
        _testButton->SetOffsetPixels(1000, 300);
        _testButton->SetBaseSize(100, 25);
        _testButton->SetBorderVisibility(true);
    }

    // Nesting
    {
        _mainPanel->AddItem(_connectButton.get());
        _mainPanel->AddItem(_connectLabel.get());
        _mainPanel->AddItem(_shareButton.get());
        _mainPanel->AddItem(_shareLabel.get());
        _mainPanel->AddItem(_fileButton.get());
        _mainPanel->AddItem(_fileLabel.get());
        _mainPanel->AddItem(_testButton.get());

        //_canvas->AddComponent(_testNotification.get());
        //_canvas->GetPanel()->AddItemShadow(_testNotification.get());
        _canvas->AddComponent(_mainPanel.get());
        _canvas->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));
    }
}

void EntryScene::_Uninit()
{
    _canvas->ClearComponents();

    _mainPanel = nullptr;
    _connectButton = nullptr;
    _shareButton = nullptr;
    _fileButton = nullptr;
    _connectLabel = nullptr;
    _shareLabel = nullptr;
    _fileLabel = nullptr;
    _testButton = nullptr;
}

void EntryScene::_Focus()
{

}

void EntryScene::_Unfocus()
{

}

void EntryScene::_Update()
{
    _canvas->Update();

    if (_connectPanelOpen)
    {
        ConnectScene* scene = (ConnectScene*)App::Instance()->FindScene(ConnectScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Connect panel incorrectly marked as open" << std::endl;
            _connectPanelOpen = false;
        }
        else
        {
            if (scene->CloseScene())
            {
                if (scene->ConnectionSuccessful())
                {
                    App::Instance()->UninitScene(ConnectScene::StaticName());
                    App::Instance()->UninitScene(GetName());
                    return;
                }
                else
                {
                    App::Instance()->UninitScene(ConnectScene::StaticName());
                    _connectPanelOpen = false;
                }
            }
        }
    }

    if (_startServerPanelOpen)
    {
        StartServerScene* scene = (StartServerScene*)App::Instance()->FindScene(StartServerScene::StaticName());
        if (!scene)
        {
            std::cout << "[WARN] Start server panel incorrectly marked as open" << std::endl;
            _startServerPanelOpen = false;
        }
        else
        {
            if (scene->CloseScene())
            {
                if (scene->StartSuccessful())
                {
                    App::Instance()->UninitScene(StartServerScene::StaticName());
                    App::Instance()->UninitScene(GetName());
                    return;
                }
                else
                {
                    App::Instance()->UninitScene(StartServerScene::StaticName());
                    _startServerPanelOpen = false;
                }
            }
        }
    }

    if (_fileLoading)
    {
        if (_fileDialog.Done())
        {
            _fileLoading = false;
            std::wstring filename = _fileDialog.Result();
            if (filename != L"")
            {
                auto item = std::make_unique<PlaylistItem>(filename);
                int64_t itemId = item->GetItemId();
                App::Instance()->playlist.Request_AddItem(std::move(item));
                App::Instance()->playlist.Request_PlayItem(itemId);
                App::Instance()->UninitScene(GetName());
                App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
            }
        }
    }
}

ID2D1Bitmap1* EntryScene::_Draw(Graphics g)
{
    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void EntryScene::_Resize(int width, int height)
{
    
}

void EntryScene::OnConnectSelected()
{
    App::Instance()->InitScene(ConnectScene::StaticName(), nullptr);
    App::Instance()->MoveSceneToFront(ConnectScene::StaticName());
    _connectPanelOpen = true;
    return;
}

void EntryScene::OnShareSelected()
{
    App::Instance()->InitScene(StartServerScene::StaticName(), nullptr);
    App::Instance()->MoveSceneToFront(StartServerScene::StaticName());
    _startServerPanelOpen = true;
    return;
}

void EntryScene::OnFileSelected()
{
    if (!_fileLoading)
    {
        _fileDialog.Open();
        _fileLoading = true;
    }
}