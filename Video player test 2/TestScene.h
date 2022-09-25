#pragma once

#include "Scene.h"
#include "VolumeSlider.h"

struct TestSceneOptions : public SceneOptionsBase
{

};

class TestScene : public Scene
{
private:
    std::unique_ptr<zcom::TextInput> _portInput = nullptr;
    zcom::VolumeSlider* _volumeSlider = nullptr;

public:
    TestScene() {}

    const char* GetName() const { return "test"; }
    static const char* StaticName() { return "test"; }

private:
    void _Init(const SceneOptionsBase* options)
    {
        _portInput = std::make_unique<zcom::TextInput>();
        _portInput->SetBaseSize(60, 30);
        _portInput->SetOffsetPixels(120, 90);
        //_portInput->SetCornerRounding(5.0f);
        _portInput->SetProperty(zcom::PROP_Shadow());
        _portInput->SetTabIndex(0);

        //_volumeSlider = new zcom::VolumeSlider(0.0f);
        //_volumeSlider->SetBaseWidth(30);
        //_volumeSlider->SetBaseHeight(30);
        //_volumeSlider->SetHorizontalOffsetPixels(80);
        //_volumeSlider->SetVerticalAlignment(zcom::Alignment::END);
        //_volumeSlider->SetVerticalOffsetPixels(-5);

        _canvas->AddComponent(_portInput.get());
        //_canvas->AddComponent(_volumeSlider);
    }
    void _Uninit()
    {
        delete _volumeSlider;
    }
    void _Focus() {}
    void _Unfocus() {}
    void _Update() {}
    ID2D1Bitmap1* _Draw(Graphics g)
    {
        g.target->Clear(D2D1::ColorF(0.1f, 0.1f, 0.1f));
        ID2D1Bitmap* bitmap = _canvas->Draw(g);
        g.target->DrawBitmap(bitmap);
        return nullptr;
    }
    void _Resize(int width, int height) {}
};