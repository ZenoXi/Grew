#pragma once

#include "Panel.h"
#include "GameTime.h"

#include <iostream>

namespace zcom
{
    class PlaybackControllerPanel : public Panel
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            Panel::_OnUpdate();

            if (_hanging)
            {
                // Reset hang timer for fixed panels.
                // It is done this way to allow SetFixed(false) to work without additionally
                // moving the mouse (if the mouse is not inside the controller panel).
                if (_fixed)
                    _hangStartTime = ztime::Main();

                if ((ztime::Main() - _hangStartTime).GetDuration() / 1000000.0f > _hangDuration)
                {
                    _panelShown = false;
                    _hanging = false;
                    _fading = true;
                    _fadeStartTime = ztime::Main();
                    _startOpacity = GetOpacity();
                    _targetOpacity = 0.0f;

                    if (GetMouseInsideArea())
                        _SetCursorVisibility(false);
                }
            }

            if (_fading)
            {
                float progress = (ztime::Main() - _fadeStartTime).GetDuration() / (1000000.0f * _fadeDuration);
                if (progress >= 1.0f)
                {
                    _fading = false;
                    SetOpacity(_targetOpacity);
                }
                else
                {
                    SetOpacity(_startOpacity + (_targetOpacity - _startOpacity) * progress);
                }
            }
        }

        EventTargets _OnMouseMove(int x, int y, bool duplicate)
        {
            if (duplicate)
                return Panel::_OnMouseMove(x, y, duplicate);

            _SetCursorVisibility(true);

            if (!_panelShown)
            {
                _panelShown = true;
                _fading = true;
                _fadeStartTime = ztime::Main();
                _startOpacity = GetOpacity();
                _targetOpacity = 1.0f;
            }

            EventTargets targets = Panel::_OnMouseMove(x, y, duplicate);
            if (targets.Size() == 1)
            {
                _hanging = true;
                _hangStartTime = ztime::Main();
            }
            else
            {
                _hanging = false;
            }
            return targets;
        }

        void _OnMouseLeaveArea()
        {
            Panel::_OnMouseLeaveArea();
            if (_panelShown)
            {
                _hanging = true;
                _hangStartTime = ztime::Main();
            }
        }

    public:
        const char* GetName() const { return "bottom_control_panel"; }
#pragma endregion

    private:
        Base* _panel = nullptr;

        bool _panelShown = false;

        TimePoint _hangStartTime;
        bool _hanging;
        float _hangDuration = 2.0f;
        bool _fixed = false;

        TimePoint _fadeStartTime;
        float _targetOpacity;
        float _startOpacity;
        bool _fading;
        float _fadeDuration = 0.1f;

    protected:
        friend class Scene;
        friend class Base;
        PlaybackControllerPanel(Scene* scene) : Panel(scene) {}
        void Init(Base* panel)
        {
            _panel = panel;

            SetOpacity(0.0f);
            //Resize();

            _hanging = false;
            _fading = false;
        }
    public:
        ~PlaybackControllerPanel() {}
        PlaybackControllerPanel(PlaybackControllerPanel&&) = delete;
        PlaybackControllerPanel& operator=(PlaybackControllerPanel&&) = delete;
        PlaybackControllerPanel(const PlaybackControllerPanel&) = delete;
        PlaybackControllerPanel& operator=(const PlaybackControllerPanel&) = delete;

        // Fixed panel doesn't disappear on cursor leave
        void SetFixed(bool fixed)
        {
            _fixed = fixed;
        }

        bool GetFixed() const
        {
            return _fixed;
        }

    private:
        void _SetCursorVisibility(bool visible);
    };
}