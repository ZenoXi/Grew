#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "Button.h"
#include "GameTime.h"
#include "ResourceManager.h"

#include <sstream>

namespace zcom
{
    class PlayButton : public Button
    {
#pragma region base_class
    protected:
        EventTargets _OnLeftPressed(int x, int y)
        {
            _clicked = true;
            if (_paused)
                SetPaused(false);
            else
                SetPaused(true);

            return EventTargets().Add(this, x, y);
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "play_button"; }
#pragma endregion

    private:
        bool _paused = false;
        bool _clicked = false;

        ID2D1Bitmap* _playIcon = nullptr;
        ID2D1Bitmap* _pauseIcon = nullptr;

    protected:
        friend class Scene;
        friend class Base;
        PlayButton(Scene* scene) : Button(scene) {}
        void Init()
        {
            Button::Init();

            _playIcon = ResourceManager::GetImage("play_18x18");
            _pauseIcon = ResourceManager::GetImage("pause_18x18");
            SetPreset(ButtonPreset::NO_EFFECTS);
            SetPaused(false);
            SetSelectable(false);

            ButtonImage()->SetPlacement(zcom::ImagePlacement::CENTER);
            UseImageParamsForAll(ButtonImage());
            ButtonImage()->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
        }
    public:
        ~PlayButton() {}
        PlayButton(PlayButton&&) = delete;
        PlayButton& operator=(PlayButton&&) = delete;
        PlayButton(const PlayButton&) = delete;
        PlayButton& operator=(const PlayButton&) = delete;

        bool GetPaused() const
        {
            return _paused;
        }

        void SetPaused(bool paused)
        {
            _paused = paused;
            if (_paused)
                SetButtonImageAll(_playIcon);
            else
                SetButtonImageAll(_pauseIcon);
        }

        bool Clicked()
        {
            bool value = _clicked;
            _clicked = false;
            return value;
        }
    };
}