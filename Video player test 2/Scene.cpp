#include "App.h" // App.h must be included first
#include "Scene.h"

Scene::Scene(App* app)
    : _app(app)
{

}

Scene::~Scene()
{

}

void Scene::Init(const SceneOptionsBase* options)
{
    _canvas = new zcom::Canvas(Create<zcom::Panel>(), App::Instance()->window.width, App::Instance()->window.height);
    _Init(options);
    _canvas->Resize(App::Instance()->window.width, App::Instance()->window.height);
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