#pragma once

#include "Scene.h"
#include "MouseEventHandler.h"

#include "Notification.h"
#include "MenuPanel.h"
#include "EmptyPanel.h"

class OverlayShortcutHandler : public KeyboardEventHandler
{
    bool _OnHotkey(int id) { return false; }
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
    void ShowMenu(zcom::MenuPanel* menu, RECT parentRect = {});
    bool MenuOpened() const { return _menu; }
    void ShowHoverText(std::wstring text, int x, int y, int maxWidth = 600);
    void AddItem(zcom::Base* item);
    void RemoveItem(zcom::Base* item);

public:
    OverlayScene(App* app);

    const char* GetName() const { return "overlay"; }
    static const char* StaticName() { return "overlay"; }

private: // Notifications
    std::unique_ptr<zcom::Panel> _notificationPanel;
    std::vector<std::unique_ptr<zcom::Notification>> _notifications;
    void _UpdateNotifications();
    void _RearrangeNotifications();

private: // Menus
    zcom::MenuPanel* _menu = nullptr;
    std::unique_ptr<zcom::EmptyPanel> _occlusionPanel = nullptr;

private: // Hover text
    std::unique_ptr<zcom::Label> _hoverTextLabel = nullptr;

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