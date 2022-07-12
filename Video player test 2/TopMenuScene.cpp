#include "App.h" // App.h must be included first
#include "TopMenuScene.h"

#include "OverlayScene.h"

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
        _app->Overlay()->ShowMenu(_menuPanel.get(), { _homeButton->GetHorizontalOffsetPixels() + 3, 0, _homeButton->GetHorizontalOffsetPixels() + 3, 0 });
    });
    _homeButton->SetOnHovered([&]()
    {
        if (_app->Overlay()->MenuOpened())
        {
            _app->Overlay()->ShowMenu(_menuPanel.get(), { _homeButton->GetHorizontalOffsetPixels() + 3, 0, _homeButton->GetHorizontalOffsetPixels() + 3, 0 });
        }
    });

    _settingsButton = Create<zcom::TopMenuButton>(L"Settings");
    _settingsButton->SetBaseSize((int)_settingsButton->Text()->GetTextWidth() + 20, _app->MenuHeight());
    _settingsButton->SetHorizontalOffsetPixels(offset);
    offset += _settingsButton->GetBaseWidth();
    _settingsButton->SetOnActivated([&]()
    {
        _app->Overlay()->ShowMenu(_menuPanel.get(), { _settingsButton->GetHorizontalOffsetPixels() + 3, 0, _settingsButton->GetHorizontalOffsetPixels() + 3, 0 });
    });
    _settingsButton->SetOnHovered([&]()
    {
        if (_app->Overlay()->MenuOpened())
        {
            _app->Overlay()->ShowMenu(_menuPanel.get(), { _settingsButton->GetHorizontalOffsetPixels() + 3, 0, _settingsButton->GetHorizontalOffsetPixels() + 3, 0 });
        }
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
}