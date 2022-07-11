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

    // Add handlers
    _canvas->AddOnMouseMove([](const zcom::EventTargets* targets)
    {
        return !targets->Empty();
    });
    _canvas->AddOnLeftPressed([](const zcom::EventTargets* targets) { return !targets->Empty(); });
    _canvas->AddOnLeftReleased([](const zcom::EventTargets* targets) { return !targets->Empty(); });
    _canvas->AddOnRightPressed([](const zcom::EventTargets* targets) { return !targets->Empty(); });
    _canvas->AddOnRightReleased([](const zcom::EventTargets* targets) { return !targets->Empty(); });
    _canvas->AddOnWheelUp([](const zcom::EventTargets* targets) { return !targets->Empty(); });
    _canvas->AddOnWheelDown([](const zcom::EventTargets* targets) { return !targets->Empty(); });

    //
    //_panel = Create<zcom::Panel>();
    //_panel->SetBaseSize(300, 150);
    //_panel->SetOffsetPixels(20, 20);
    //_panel->SetBackgroundColor(D2D1::ColorF(0.05f, 0.05f, 0.05f));

    //_canvas->AddComponent(_panel.get());
}

void OverlayScene::_Uninit()
{

}

void OverlayScene::_Focus()
{
    _app->mouseManager.RemoveHandler(_canvas);
    _app->mouseManager.SetOverlayHandler(_canvas);
}

void OverlayScene::_Update()
{
    _canvas->Update();
}