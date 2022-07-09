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
        const char* GetName() const { return "play_button"; }
#pragma endregion

    private:
        bool _paused = false;
        bool _clicked = false;

        ID2D1Bitmap* _playIcon = nullptr;
        ID2D1Bitmap* _playIconHover = nullptr;
        ID2D1Bitmap* _pauseIcon = nullptr;
        ID2D1Bitmap* _pauseIconHover = nullptr;

    public:
        PlayButton()
        {
            _playIcon = ResourceManager::GetImage("play_dim");
            _playIconHover = ResourceManager::GetImage("play");
            _pauseIcon = ResourceManager::GetImage("pause_dim");
            _pauseIconHover = ResourceManager::GetImage("pause");
            SetPreset(ButtonPreset::NO_EFFECTS);
            SetPaused(false);
            SetSelectable(false);
        }
        ~PlayButton()
        {
            //SafeFullRelease((IUnknown**)&_primaryColorBrush);
            //SafeFullRelease((IUnknown**)&_blankBrush);
            //SafeFullRelease((IUnknown**)&_opacityBrush);
            //SafeFullRelease((IUnknown**)&_playButton);
        }
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
            {
                SetButtonImage(_playIcon);
                SetButtonHoverImage(_playIconHover);
                SetButtonClickImage(_playIconHover);
            }
            else
            {
                SetButtonImage(_pauseIcon);
                SetButtonHoverImage(_pauseIconHover);
                SetButtonClickImage(_pauseIconHover);
            }
        }

        bool Clicked()
        {
            bool value = _clicked;
            _clicked = false;
            return value;
        }
    };
}