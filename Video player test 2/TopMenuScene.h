#pragma once

#include "Scene.h"
#include "TopMenuButton.h"
#include "MenuPanel.h"

struct TopMenuSceneOptions : public SceneOptionsBase
{

};

class TopMenuScene : public Scene
{
public:
    TopMenuScene(App* app);

    const char* GetName() const { return "top_menu"; }
    static const char* StaticName() { return "top_menu"; }

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus() {}
    void _Update();
    void _Resize(int width, int height) {}

    std::unique_ptr<zcom::TopMenuButton> _homeButton = nullptr;
    std::unique_ptr<zcom::TopMenuButton> _settingsButton = nullptr;
    std::unique_ptr<zcom::TopMenuButton> _toolsButton = nullptr;
    std::unique_ptr<zcom::TopMenuButton> _helpButton = nullptr;

    std::unique_ptr<zcom::MenuPanel> _menuPanel = nullptr;
};