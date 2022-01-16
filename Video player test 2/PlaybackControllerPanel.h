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
                        App::Instance()->window.SetCursorVisibility(false);
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

        //void _OnDraw(Graphics g)
        //{
        //    Panel::_OnDraw(g);
        //    g.target->Clear();
        //    g.target->DrawBitmap(
        //        _panel->Draw(g),
        //        D2D1::RectF(
        //            _panel->GetX(),
        //            _panel->GetY(),
        //            _panel->GetX() + _panel->GetWidth(),
        //            _panel->GetY() + _panel->GetHeight()
        //        ),
        //        _panel->GetOpacity()
        //    );
        //}

        //void _OnResize(int width, int height)
        //{
        //    Panel::_OnResize(width, height);
        //    int newWidth = (int)std::round(GetWidth() * _panel->GetParentWidthPercent()) + _panel->GetBaseWidth();
        //    int newHeight = (int)std::round(GetHeight() * _panel->GetParentHeightPercent()) + _panel->GetBaseHeight();
        //    // SetSize does limit checking so the resulting size and newWidth/newHeight can mismatch
        //    _panel->SetSize(newWidth, newHeight);

        //    int newPosX = 0;
        //    if (_panel->GetHorizontalAlignment() == Alignment::START)
        //    {
        //        newPosX += std::round((GetWidth() - _panel->GetWidth()) * _panel->GetHorizontalOffsetPercent());
        //    }
        //    else if (_panel->GetHorizontalAlignment() == Alignment::END)
        //    {
        //        newPosX = GetWidth() - _panel->GetWidth();
        //        newPosX -= std::round((GetWidth() - _panel->GetWidth()) * _panel->GetHorizontalOffsetPercent());
        //    }
        //    newPosX += _panel->GetHorizontalOffsetPixels();
        //    // Alternative (no branching):
        //    // int align = item->GetHorizontalAlignment() == Alignment::END;
        //    // newPosX += align * (_width - item->GetWidth());
        //    // newPosX += (-1 * align) * std::round((_width - item->GetWidth()) * item->GetHorizontalOffsetPercent());
        //    // newPosX += item->GetHorizontalOffsetPixels();
        //    int newPosY = 0;
        //    if (_panel->GetVerticalAlignment() == Alignment::START)
        //    {
        //        newPosY += std::round((GetHeight() - _panel->GetHeight()) * _panel->GetVerticalOffsetPercent());
        //    }
        //    else if (_panel->GetVerticalAlignment() == Alignment::END)
        //    {
        //        newPosY = GetHeight() - _panel->GetHeight();
        //        newPosY -= std::round((GetHeight() - _panel->GetHeight()) * _panel->GetVerticalOffsetPercent());
        //    }
        //    newPosY += _panel->GetVerticalOffsetPixels();
        //    _panel->SetPosition(newPosX, newPosY);

        //    _panel->Resize(_panel->GetWidth(), _panel->GetHeight());
        //    //_panel->Resize(width, height);
        //}

        Base* _OnMouseMove(int x, int y)
        {
            App::Instance()->window.SetCursorVisibility(true);

            if (!_panelShown)
            {
                _panelShown = true;
                _fading = true;
                _fadeStartTime = ztime::Main();
                _startOpacity = GetOpacity();
                _targetOpacity = 1.0f;
            }

            Base* target = Panel::_OnMouseMove(x, y);
            if (target == this)
            {
                _hanging = true;
                _hangStartTime = ztime::Main();
            }
            else
            {
                _hanging = false;
            }
            return target;
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

    public:
        PlaybackControllerPanel(Base* panel) : _panel(panel)
        {
            SetOpacity(0.0f);
            //Resize();

            _hanging = false;
            _fading = false;
        }
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
    };
}