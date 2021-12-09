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
    App::Instance()->mouseManager.AddHandler(_canvas);
    App::Instance()->keyboardManager.AddHandler(_canvas);
    _canvas->Resize(App::Instance()->window.width, App::Instance()->window.height);
    _Init(options);
}

void Scene::Uninit()
{
    App::Instance()->mouseManager.RemoveHandler(_canvas);
    App::Instance()->keyboardManager.RemoveHandler(_canvas);
    _Uninit();
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