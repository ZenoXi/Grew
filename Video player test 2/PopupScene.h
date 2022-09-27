#pragma once

#include "Scene.h"

#include "Image.h"
#include "Label.h"

struct PopupSceneOptions : public SceneOptionsBase
{
    ID2D1Bitmap* popupImage = nullptr;
    D2D1_COLOR_F popupImageTint = D2D1::ColorF(1.0f, 1.0f, 1.0f);
    std::wstring popupText = L"This is a pop up";
    // Button is only displayed is text is not empty
    std::wstring leftButtonText = L"";
    D2D1_COLOR_F leftButtonColor = D2D1::ColorF(0.25f, 0.25f, 0.25f);
    // Button is only displayed is text is not empty and left button text is also not empty
    std::wstring rightButtonText = L"";
    D2D1_COLOR_F rightButtonColor = D2D1::ColorF(0.25f, 0.25f, 0.25f);
};

class PopupScene : public Scene
{
public:
    bool CloseScene() const { return _closeScene; }
    bool LeftClicked() const { return _leftClicked; }
    bool RightClicked() const { return _rightClicked; }

    PopupScene(App* app) : Scene(app) {}

    const char* GetName() const { return "popup"; }
    static const char* StaticName() { return "popup"; }

private:
    std::unique_ptr<zcom::Panel> _mainPanel = nullptr;
    std::unique_ptr<zcom::Image> _popupImage = nullptr;
    std::unique_ptr<zcom::Label> _popupText = nullptr;
    std::unique_ptr<zcom::Button> _leftButton = nullptr;
    std::unique_ptr<zcom::Button> _rightButton = nullptr;
    std::unique_ptr<zcom::Button> _okButton = nullptr;

    bool _closeScene = false;
    bool _leftClicked = false;
    bool _rightClicked = false;

private:
    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    void _Resize(int width, int height);
};