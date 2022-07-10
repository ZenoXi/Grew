#include "App.h" // App.h must be included first
#include "Scene.h"

Scene::Scene(App* app)
    : _app(app)
{
    _notificationPanel = Create<zcom::Panel>();
    _notificationPanel->SetBaseHeight(10);
    _notificationPanel->SetBaseWidth(220);
    _notificationPanel->SetZIndex(NOTIF_PANEL_Z_INDEX);
    _notificationPanel->SetVisible(false);
    _notificationPosition = NotificationPosition::BOTTOM_RIGHT;
}

Scene::~Scene()
{

}

void Scene::Init(const SceneOptionsBase* options)
{
    _canvas = new zcom::Canvas(Create<zcom::Panel>(), App::Instance()->window.width, App::Instance()->window.height);
    _Init(options);
    _canvas->AddComponent(_notificationPanel.get());
    _canvas->Resize(App::Instance()->window.width, App::Instance()->window.height);
    _RearrangeNotifications();
}

void Scene::Uninit()
{
    Unfocus();
    _Uninit();
    delete _canvas;
    _canvas = nullptr;
}

void Scene::Focus()
{
    _focused = true;
    App::Instance()->mouseManager.AddHandler(_canvas);
    App::Instance()->keyboardManager.AddHandler(_canvas);
    _Focus();
}

void Scene::Unfocus()
{
    _focused = false;
    _canvas->ClearSelection();
    _canvas->OnLeftReleased(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
    _canvas->OnRightReleased(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
    _canvas->OnMouseLeave();
    App::Instance()->mouseManager.RemoveHandler(_canvas);
    App::Instance()->keyboardManager.RemoveHandler(_canvas);
    _Unfocus();
}

bool Scene::Focused() const
{
    return _focused;
}

void Scene::Update()
{
    _Update();
    _UpdateNotifications();
}

bool Scene::Redraw()
{
    return _Redraw();
}

ID2D1Bitmap* Scene::Draw(Graphics g)
{
    return _Draw(g);
}

ID2D1Bitmap* Scene::Image()
{
    return _Image();
}

void Scene::Resize(int width, int height)
{
    _canvas->Resize(width, height);
    _Resize(width, height);
}

zcom::Canvas* Scene::GetCanvas() const
{
    return _canvas;
}

void Scene::SetNotificationPosition(NotificationPosition position)
{
    if (_notificationPosition != position)
    {
        _notificationPosition = position;
        _RearrangeNotifications();
    }
}

void Scene::ShowNotification(zcom::NotificationInfo info)
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

void Scene::_UpdateNotifications()
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

void Scene::_RearrangeNotifications()
{
    _notificationPanel->DeferLayoutUpdates();
    _notificationPanel->ClearItems();

    zcom::Alignment vAlignment;
    int vSign = 0;
    if (_notificationPosition == NotificationPosition::TOP_LEFT)
    {
        vAlignment = zcom::Alignment::START;
        vSign = 1;
        _notificationPanel->SetHorizontalAlignment(zcom::Alignment::START);
        _notificationPanel->SetVerticalAlignment(zcom::Alignment::START);
    }
    else if (_notificationPosition == NotificationPosition::TOP_RIGHT)
    {
        vAlignment = zcom::Alignment::START;
        vSign = 1;
        _notificationPanel->SetHorizontalAlignment(zcom::Alignment::END);
        _notificationPanel->SetVerticalAlignment(zcom::Alignment::START);
    }
    else if (_notificationPosition == NotificationPosition::BOTTOM_LEFT)
    {
        vAlignment = zcom::Alignment::END;
        vSign = -1;
        _notificationPanel->SetHorizontalAlignment(zcom::Alignment::START);
        _notificationPanel->SetVerticalAlignment(zcom::Alignment::END);
    }
    else if (_notificationPosition == NotificationPosition::BOTTOM_RIGHT)
    {
        vAlignment = zcom::Alignment::END;
        vSign = -1;
        _notificationPanel->SetHorizontalAlignment(zcom::Alignment::END);
        _notificationPanel->SetVerticalAlignment(zcom::Alignment::END);
    }

    int offsetX = 10;
    int offsetY = 10 * vSign;

    for (int i = 0; i < _notifications.size(); i++)
    {
        _notifications[i]->SetOffsetPixels(offsetX, offsetY);
        if (vSign == 1)
            _notifications[i]->SetVerticalAlignment(zcom::Alignment::START);
        else if (vSign == -1)
            _notifications[i]->SetVerticalAlignment(zcom::Alignment::END);
        offsetY += (_notifications[i]->GetHeight() + 10) * vSign;
        _notificationPanel->AddItem(_notifications[i].get());
    }
    _notificationPanel->SetBaseHeight(std::abs(offsetY));
    _notificationPanel->ResumeLayoutUpdates();
}