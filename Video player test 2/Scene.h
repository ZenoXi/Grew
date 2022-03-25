#pragma once

#include "Canvas.h"
#include "Notification.h"
#include "GameTime.h"

#include <functional>

// Options for switching scenes
struct SceneOptionsBase
{
    
};

enum class NotificationPosition
{
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};

#define NOTIF_PANEL_Z_INDEX 255

class Scene
{
protected:
    zcom::Canvas* _canvas;
    bool _focused = false;

public:
    Scene();
    virtual ~Scene();
    void Init(const SceneOptionsBase* options);
    void Uninit();
    void Focus();
    void Unfocus();
    bool Focused() const;

    void Update();
    ID2D1Bitmap1* Draw(Graphics g);
    void Resize(int width, int height);

private:
    virtual void _Init(const SceneOptionsBase* options) = 0;
    virtual void _Uninit() = 0;
    virtual void _Focus() = 0;
    virtual void _Unfocus() = 0;

    virtual void _Update() = 0;
    virtual ID2D1Bitmap1* _Draw(Graphics g) = 0;
    virtual void _Resize(int width, int height) = 0;

public:
    virtual const char* GetName() const = 0;


private: // Notifications
    NotificationPosition _notificationPosition = NotificationPosition::BOTTOM_RIGHT;
    std::unique_ptr<zcom::Panel> _notificationPanel;
    std::vector<std::unique_ptr<zcom::Notification>> _notifications;
public:
    void SetNotificationPosition(NotificationPosition position);
    void ShowNotification(zcom::NotificationInfo info);
private:
    void _UpdateNotifications();
    void _RearrangeNotifications();
};