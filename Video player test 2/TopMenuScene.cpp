#include "App.h" // App.h must be included first
#include "TopMenuScene.h"

#include "Network.h"
#include "ClientManager.h"
#include "ServerManager.h"
#include "Network.h"

#include "OverlayScene.h"
#include "SettingsScene.h"
#include "EntryScene.h"
#include "PopupScene.h"

TopMenuScene::TopMenuScene(App* app)
    : Scene(app)
{

}

void TopMenuScene::_Init(const SceneOptionsBase* options)
{
    TopMenuSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const TopMenuSceneOptions*>(options);
    }

    int offset = 0;
    _homeButton = Create<zcom::TopMenuButton>(L"Home");
    _homeButton->SetBaseSize((int)_homeButton->Text()->GetTextWidth() + 20, _app->MenuHeight());
    _homeButton->SetHorizontalOffsetPixels(offset);
    offset += _homeButton->GetBaseWidth();
    _homeButton->SetOnActivated([&]()
    {
        // If home page already open, ignore click
        if (_app->FindActiveScene(EntryScene::StaticName()))
            return;

        // Check if server running/connected
        auto clientManager = APP_NETWORK->GetManager<znet::ClientManager>();
        if (clientManager)
        {
            PopupSceneOptions opt;
            opt.popupImage = ResourceManager::GetImage("warning_32x32");
            opt.popupImageTint = D2D1::ColorF(0.8f, 0.65f, 0.0f);
            opt.popupText = L"You are connected to a server. Going to the home page will disconnect you from the server.\n\nContinue?";
            opt.leftButtonText = L"Ok";
            opt.rightButtonText = L"Cancel";
            opt.rightButtonColor = D2D1::ColorF(0.5f, 0.2f, 0.2f);

            _homePopupOpen = true;
            _homeButton->SetActive(false);
            _settingsButton->SetActive(false);
            _toolsButton->SetActive(false);
            _helpButton->SetActive(false);

            _app->InitScene(PopupScene::StaticName(), &opt);
            _app->MoveSceneToFront(PopupScene::StaticName());

            return;
        }

        auto serverManager = APP_NETWORK->GetManager<znet::ServerManager>();
        if (serverManager)
        {
            PopupSceneOptions opt;
            opt.popupImage = ResourceManager::GetImage("warning_32x32");
            opt.popupImageTint = D2D1::ColorF(0.8f, 0.65f, 0.0f);
            opt.popupText = L"You are running a server. Going to the home page will close it.\n\nContinue?";
            opt.leftButtonText = L"Ok";
            opt.rightButtonText = L"Cancel";
            opt.rightButtonColor = D2D1::ColorF(0.5f, 0.2f, 0.2f);

            _homePopupOpen = true;
            _homeButton->SetActive(false);
            _settingsButton->SetActive(false);
            _toolsButton->SetActive(false);
            _helpButton->SetActive(false);

            _app->InitScene(PopupScene::StaticName(), &opt);
            _app->MoveSceneToFront(PopupScene::StaticName());

            return;
        }
        
        _app->InitScene(EntryScene::StaticName(), nullptr);
        _app->MoveSceneToFront(EntryScene::StaticName());
    });
    _homeButton->SetOnHovered([&]()
    {
        if (_app->Overlay()->MenuOpened())
            _app->Overlay()->ShowMenu(nullptr, RECT{});
    });

    _settingsButton = Create<zcom::TopMenuButton>(L"Settings");
    _settingsButton->SetBaseSize((int)_settingsButton->Text()->GetTextWidth() + 20, _app->MenuHeight());
    _settingsButton->SetHorizontalOffsetPixels(offset);
    offset += _settingsButton->GetBaseWidth();
    _settingsButton->SetOnActivated([&]()
    {
        _app->InitScene(SettingsScene::StaticName(), nullptr);
        _app->MoveSceneToFront(SettingsScene::StaticName());
    });
    _settingsButton->SetOnHovered([&]()
    {
        if (_app->Overlay()->MenuOpened())
            _app->Overlay()->ShowMenu(nullptr, RECT{});
    });

    _toolsButton = Create<zcom::TopMenuButton>(L"Tools");
    _toolsButton->SetBaseSize((int)_toolsButton->Text()->GetTextWidth() + 20, _app->MenuHeight());
    _toolsButton->SetHorizontalOffsetPixels(offset);
    offset += _toolsButton->GetBaseWidth();
    _toolsButton->SetOnActivated([&]()
    {
        _app->Overlay()->ShowMenu(_menuPanel.get(), { _toolsButton->GetHorizontalOffsetPixels() + 3, 0, _toolsButton->GetHorizontalOffsetPixels() + 3, 0 });
    });
    _toolsButton->SetOnHovered([&]()
    {
        if (_app->Overlay()->MenuOpened())
        {
            _app->Overlay()->ShowMenu(_menuPanel.get(), { _toolsButton->GetHorizontalOffsetPixels() + 3, 0, _toolsButton->GetHorizontalOffsetPixels() + 3, 0 });
        }
    });

    _helpButton = Create<zcom::TopMenuButton>(L"Help");
    _helpButton->SetBaseSize((int)_helpButton->Text()->GetTextWidth() + 20, _app->MenuHeight());
    _helpButton->SetHorizontalOffsetPixels(offset);
    offset += _helpButton->GetBaseWidth();
    _helpButton->SetOnActivated([&]()
    {
        _app->Overlay()->ShowMenu(_menuPanel.get(), { _helpButton->GetHorizontalOffsetPixels() + 3, 0, _helpButton->GetHorizontalOffsetPixels() + 3, 0 });
    });
    _helpButton->SetOnHovered([&]()
    {
        if (_app->Overlay()->MenuOpened())
        {
            _app->Overlay()->ShowMenu(_menuPanel.get(), { _helpButton->GetHorizontalOffsetPixels() + 3, 0, _helpButton->GetHorizontalOffsetPixels() + 3, 0 });
        }
    });

    _menuPanel = Create<zcom::MenuPanel>();
    _menuPanel->AddItem(Create<zcom::MenuItem>(L"Bruh"));
    _menuPanel->AddItem(Create<zcom::MenuItem>(L"Moment"));
    _menuPanel->AddItem(Create<zcom::MenuItem>());
    _menuPanel->AddItem(Create<zcom::MenuItem>(L"Lmao"));

    _canvas->AddComponent(_homeButton.get());
    _canvas->AddComponent(_settingsButton.get());
    _canvas->AddComponent(_toolsButton.get());
    _canvas->AddComponent(_helpButton.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0));
}

void TopMenuScene::_Uninit()
{
    _canvas->ClearComponents();
}

void TopMenuScene::_Focus()
{
    _app->mouseManager.RemoveHandler(_canvas);
    _app->mouseManager.SetTopMenuHandler(_canvas);
}

void TopMenuScene::_Update()
{
    _canvas->Update();

    if (_homePopupOpen)
    {
        PopupScene* scene = (PopupScene*)_app->FindActiveScene(PopupScene::StaticName());
        if (scene && scene->CloseScene())
        {
            _app->UninitScene(PopupScene::StaticName());
            _homePopupOpen = false;
            _homeButton->SetActive(true);
            _settingsButton->SetActive(true);
            _toolsButton->SetActive(true);
            _helpButton->SetActive(true);

            if (scene->LeftClicked())
            {
                APP_NETWORK->CloseManager();
                _app->InitScene(EntryScene::StaticName(), nullptr);
                _app->MoveSceneToFront(EntryScene::StaticName());
            }
        }
    }
}