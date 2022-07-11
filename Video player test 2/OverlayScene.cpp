#include "App.h" // App.h must be included first
#include "OverlayScene.h"

OverlayScene::OverlayScene(App* app)
    : Scene(app)
{

}

void OverlayScene::_Init(const SceneOptionsBase* options)
{
    OverlaySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const OverlaySceneOptions*>(options);
    }
    
    // Set up shortcut handler
    _shortcutHandler = std::make_unique<OverlayShortcutHandler>();
    _shortcutHandler->AddOnKeyDown([&](BYTE keyCode)
    {
        return _HandleKeyDown(keyCode);
    });

    // Add handlers
    auto testFunc = [&](const zcom::EventTargets* targets)
    {
        if (targets->Empty())
            return false;

        // Notification panel (not the actual notification)
        if (targets->Size() == 1 && targets->Contains(_notificationPanel.get()))
            return false;

        return true;
    };
    _canvas->AddOnMouseMove(testFunc);
    _canvas->AddOnLeftPressed(testFunc);
    _canvas->AddOnLeftReleased(testFunc);
    _canvas->AddOnRightPressed(testFunc);
    _canvas->AddOnRightReleased(testFunc);
    _canvas->AddOnWheelUp(testFunc);
    _canvas->AddOnWheelDown(testFunc);

    //
    _notificationPanel = Create<zcom::Panel>();
    _notificationPanel->SetBaseHeight(10);
    _notificationPanel->SetBaseWidth(220);
    _notificationPanel->SetHorizontalAlignment(zcom::Alignment::END);
    _notificationPanel->SetZIndex(NOTIF_PANEL_Z_INDEX);
    _notificationPanel->SetVisible(false);

    _canvas->AddComponent(_notificationPanel.get());
}

void OverlayScene::_Uninit()
{

}

void OverlayScene::_Focus()
{
    _app->mouseManager.RemoveHandler(_canvas);
    _app->mouseManager.SetOverlayHandler(_canvas);

    App::Instance()->keyboardManager.AddHandler(_shortcutHandler.get());
    GetKeyboardState(_shortcutHandler->KeyStates());
}

void OverlayScene::_Update()
{
    _canvas->Update();
    _UpdateNotifications();
}

void OverlayScene::ShowNotification(zcom::NotificationInfo info)
{
    auto notification = Create<zcom::Notification>(200, info);
    zcom::PROP_Shadow shadowProps;
    shadowProps.offsetX = 2.0f;
    shadowProps.offsetY = 2.0f;
    shadowProps.blurStandardDeviation = 2.0f;
    shadowProps.color = D2D1::ColorF(0, 1.0f);
    notification->SetProperty(shadowProps);
    //_notifications.insert(_notifications.begin(), std::move(notification));
    _notifications.push_back(std::move(notification));
    _notificationPanel->SetVisible(true);
    _RearrangeNotifications();
}

void OverlayScene::_UpdateNotifications()
{
    bool changed = false;
    for (int i = 0; i < _notifications.size(); i++)
    {
        if (_notifications[i]->Destroy())
        {
            _notifications.erase(_notifications.begin() + i);
            i--;
            changed = true;
        }
    }
    if (changed)
    {
        if (_notifications.empty())
            _notificationPanel->SetVisible(false);
        _RearrangeNotifications();
    }
}

void OverlayScene::_RearrangeNotifications()
{
    _notificationPanel->DeferLayoutUpdates();
    _notificationPanel->ClearItems();

    int offsetX = 10;
    int offsetY = 10;
    for (int i = 0; i < _notifications.size(); i++)
    {
        _notifications[i]->SetOffsetPixels(offsetX, offsetY);
        offsetY += _notifications[i]->GetHeight() + 10;
        _notificationPanel->AddItem(_notifications[i].get());
    }
    _notificationPanel->SetBaseHeight(offsetY);
    _notificationPanel->ResumeLayoutUpdates();
}

bool OverlayScene::_HandleKeyDown(BYTE keyCode)
{
    switch (keyCode)
    {
    case VK_MULTIPLY:
        zcom::NotificationInfo ninfo;
        ninfo.duration = Duration(10, SECONDS);
        ShowNotification(ninfo);
        return true;
    }
    return false;
}