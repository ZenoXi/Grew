#include "App.h" // App.h must be included first
#include "Scene.h"

Scene::Scene()
{
    _canvas = new zcom::Canvas(App::Instance()->window.width, App::Instance()->window.height);
}

Scene::~Scene()
{
    delete _canvas;
}

void Scene::Init(const SceneOptionsBase* options)
{
    _Init(options);
    _canvas->Resize(App::Instance()->window.width, App::Instance()->window.height);
}

void Scene::Uninit()
{
    Unfocus();
    _Uninit();
}

void Scene::Focus()
{
    _focused = true;
    App::Instance()->mouseManager.AddHandler(_canvas);
    App::Instance()->keyboardManager.AddHandler(_canvas);
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
}

bool Scene::Focused() const
{
    return _focused;
}

void Scene::Update()
{
    _Update();
}

ID2D1Bitmap1* Scene::Draw(Graphics g)
{
    return _Draw(g);
}

void Scene::Resize(int width, int height)
{
    _canvas->Resize(width, height);
    _Resize(width, height);
}