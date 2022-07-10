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

class App;

class Scene
{
protected:
    App* _app;
    zcom::Canvas* _canvas;
    bool _focused = false;

public:
    Scene(App* app);
    virtual ~Scene();
    void Init(const SceneOptionsBase* options);
    void Uninit();
    void Focus();
    void Unfocus();
    bool Focused() const;

    void Update();
    bool Redraw();
    ID2D1Bitmap* Draw(Graphics g);
    ID2D1Bitmap* Image();
    void Resize(int width, int height);

    // Component creation
    template<class T, typename... Args>
    std::unique_ptr<T> Create(Args&&... args)
    {
        auto uptr = std::unique_ptr<T>(new T(this));
        uptr->Init(std::forward<Args>(args)...);
        return uptr;
    }

    App* GetApp() const { return _app; }

    zcom::Canvas* GetCanvas() const;

private:
    virtual void _Init(const SceneOptionsBase* options) = 0;
    virtual void _Uninit() = 0;
    virtual void _Focus() = 0;
    virtual void _Unfocus() = 0;

    virtual void _Update() = 0;
    virtual bool _Redraw() { return _canvas->Redraw(); }
    virtual ID2D1Bitmap* _Draw(Graphics g) { return _canvas->Draw(g); }
    virtual ID2D1Bitmap* _Image() { return _canvas->Image(); }
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