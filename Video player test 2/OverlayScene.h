#pragma once

#include "Scene.h"
#include "MouseEventHandler.h"

#include "Panel.h"

class OverlayShortcutHandler : public KeyboardEventHandler
{
    bool _OnKeyDown(BYTE vkCode) { return false; }
    bool _OnKeyUp(BYTE vkCode) { return false; }
    bool _OnChar(wchar_t ch) { return false; }
};

struct OverlaySceneOptions : public SceneOptionsBase
{

};

class OverlayScene : public Scene
{
public:
    void ShowNotification(zcom::NotificationInfo info);

public:
    OverlayScene(App* app);

    const char* GetName() const { return "overlay"; }
    static const char* StaticName() { return "overlay"; }

private: // Notifications
    std::unique_ptr<zcom::Panel> _notificationPanel;
    std::vector<std::unique_ptr<zcom::Notification>> _notifications;
    void _UpdateNotifications();
    void _RearrangeNotifications();

private:
    std::unique_ptr<OverlayShortcutHandler> _shortcutHandler = nullptr;

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus() {}
    void _Update();
    void _Resize(int width, int height) {}

private:
    bool _HandleKeyDown(BYTE keyCode);
};