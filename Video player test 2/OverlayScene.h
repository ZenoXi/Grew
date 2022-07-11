#pragma once

#include "Scene.h"
#include "MouseEventHandler.h"

#include "Panel.h"

struct OverlaySceneOptions : public SceneOptionsBase
{

};

class OverlayScene : public Scene
{
public:
    OverlayScene(App* app);

    const char* GetName() const { return "overlay"; }
    static const char* StaticName() { return "overlay"; }

private:
    //std::unique_ptr<zcom::Panel> _panel = nullptr;

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus() {}
    void _Update();
    void _Resize(int width, int height) {}
};