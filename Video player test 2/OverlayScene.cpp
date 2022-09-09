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
    auto nonClickFunc = [&](const zcom::EventTargets* targets)
    {
        if (targets->Empty())
            return false;

        // Notification panel (not the actual notification)
        if (targets->Size() == 1 && targets->Contains(_notificationPanel.get()))
            return false;

        return true;
    };
    auto clickFunc = [&](const zcom::EventTargets* targets)
    {
        if (targets->Empty())
            return false;

        // Notification panel (not the actual notification)
        if (targets->Size() == 1 && targets->Contains(_notificationPanel.get()))
            return false;

        // Occlusion panel
        if (targets->MainTarget() == _occlusionPanel.get())
            return false;

        return true;
    };

    auto hideHoverText = [&](const zcom::EventTargets* targets)
    {
        _hoverTextLabel->SetVisible(false);
        return false;
    };
    _canvas->AddOnMouseMove(hideHoverText);
    _canvas->AddOnMouseMove(nonClickFunc);
    _canvas->AddOnLeftPressed(clickFunc);
    _canvas->AddOnLeftReleased(clickFunc);
    _canvas->AddOnRightPressed(clickFunc);
    _canvas->AddOnRightReleased(clickFunc);
    _canvas->AddOnWheelUp(nonClickFunc);
    _canvas->AddOnWheelDown(nonClickFunc);

    //
    _notificationPanel = Create<zcom::Panel>();
    _notificationPanel->SetBaseHeight(10);
    _notificationPanel->SetBaseWidth(220);
    _notificationPanel->SetHorizontalAlignment(zcom::Alignment::END);
    _notificationPanel->SetZIndex(NOTIF_PANEL_Z_INDEX);
    _notificationPanel->SetVisible(false);
    _notificationPanel->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));

    _occlusionPanel = Create<zcom::EmptyPanel>();
    _occlusionPanel->SetParentSizePercent(1.0f, 1.0f);
    _occlusionPanel->SetVisible(false);

    _hoverTextLabel = Create<zcom::Label>();
    _hoverTextLabel->SetMargins({ 5.0f, 3.0f, 5.0f, 3.0f });
    _hoverTextLabel->SetWordWrap(true);
    _hoverTextLabel->SetFont(L"Segoe UI");
    _hoverTextLabel->SetFontSize(13.0f);
    _hoverTextLabel->SetFontColor(D2D1::ColorF(D2D1::ColorF::White));
    _hoverTextLabel->SetBackgroundColor(D2D1::ColorF(0.05f, 0.05f, 0.05f));
    _hoverTextLabel->SetBorderVisibility(true);
    _hoverTextLabel->SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
    zcom::PROP_Shadow shadow;
    shadow.color = D2D1::ColorF(0);
    shadow.offsetX = 2.0f;
    shadow.offsetY = 2.0f;
    _hoverTextLabel->SetProperty(shadow);
    _hoverTextLabel->SetVisible(false);

    _canvas->AddComponent(_notificationPanel.get());
    _canvas->AddComponent(_occlusionPanel.get());
    _canvas->AddComponent(_hoverTextLabel.get());
}

void OverlayScene::_Uninit()
{
    _canvas->ClearComponents();
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

    if (_menu && !_menu->GetVisible())
    {
        _menu->RemoveOnDestruct({ this, "" });
        _menu = nullptr;
        _occlusionPanel->SetVisible(false);
    }
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

void OverlayScene::ShowMenu(zcom::MenuPanel* menu, RECT parentRect)
{
    if (_menu)
    {
        _menu->RemoveOnDestruct({ this, "" });
        _menu->Hide();
        _menu = nullptr;
        _occlusionPanel->SetVisible(false);
    }
    if (!menu)
        return;

    _menu = menu;
    _menu->SetZIndex(1);
    _menu->AddOnDestruct([&]()
    {
        _menu->Hide();
        _menu = nullptr;
    }, { this, "" });
    _menu->Show(_canvas, parentRect);
    _occlusionPanel->SetVisible(true);
}

void OverlayScene::ShowHoverText(std::wstring text, int x, int y, int maxWidth)
{
    if (text.empty())
    {
        _hoverTextLabel->SetVisible(false);
        return;
    }

    // Size the label
    _hoverTextLabel->SetWordWrap(false);
    _hoverTextLabel->SetText(text);
    float textWidth = _hoverTextLabel->GetTextWidth();
    int labelWidth = ceilf(textWidth);
    if (labelWidth > maxWidth)
        labelWidth = maxWidth;
    _hoverTextLabel->Resize(labelWidth, 1);
    _hoverTextLabel->SetWordWrap(true);
    textWidth = _hoverTextLabel->GetTextWidth();
    labelWidth = ceilf(textWidth);
    float textHeight = _hoverTextLabel->GetTextHeight();
    int labelHeight = ceilf(textHeight);
    _hoverTextLabel->SetBaseSize(0, 0); // Invoke layout change
    _hoverTextLabel->SetBaseSize(labelWidth, labelHeight);

    // Position the label
    int xPos = x;
    if (xPos + labelWidth > _canvas->GetWidth() - 10)
        xPos = _canvas->GetWidth() - 10 - labelWidth;
    int yPos = y + 20;
    int topSize = y - 20;
    int botSize = _canvas->GetWidth() - y - 30;
    if (labelHeight > botSize)
    {
        if (labelHeight <= topSize)
        {
            yPos = y - 10 - labelHeight;
        }
        else
        {
            yPos = _canvas->GetWidth() - 10 - labelHeight;
        }
    }
    _hoverTextLabel->SetOffsetPixels(xPos, yPos);

    // Show label
    _hoverTextLabel->SetVisible(true);
    _hoverTextLabel->InvokeRedraw();
}

void OverlayScene::AddItem(zcom::Base* item)
{
    _canvas->AddComponent(item);
}

void OverlayScene::RemoveItem(zcom::Base* item)
{
    _canvas->RemoveComponent(item);
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