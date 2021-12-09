#pragma once

#include "ComponentBase.h"
#include "GameTime.h"

#include <iostream>

namespace zcom
{
    class BottomControlPanel : public Base
    {
#pragma region base_class
    private:
        void _OnUpdate()
        {
            if (_hanging)
            {
                if ((ztime::Main() - _mouseLeaveTime).GetDuration() / 1000000.0f > _hangDuration)
                {
                    _hanging = false;
                    _fading = true;
                    _fadeStartTime = ztime::Main();
                    _startOpacity = GetOpacity();
                    _targetOpacity = 0.0f;
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

        void _OnDraw(Graphics g)
        {
            g.target->Clear();
            g.target->DrawBitmap(
                _panel->Draw(g),
                D2D1::RectF(
                    _panel->GetX(),
                    _panel->GetY(),
                    _panel->GetX() + _panel->GetWidth(),
                    _panel->GetY() + _panel->GetHeight()
                ),
                _panel->GetOpacity()
            );
        }

        void _OnResize(int width, int height)
        {
            int newWidth = (int)std::round(GetWidth() * _panel->GetParentWidthPercent()) + _panel->GetBaseWidth();
            int newHeight = (int)std::round(GetHeight() * _panel->GetParentHeightPercent()) + _panel->GetBaseHeight();
            // SetSize does limit checking so the resulting size and newWidth/newHeight can mismatch
            _panel->SetSize(newWidth, newHeight);

            int newPosX = 0;
            if (_panel->GetHorizontalAlignment() == Alignment::START)
            {
                newPosX += std::round((GetWidth() - _panel->GetWidth()) * _panel->GetHorizontalOffsetPercent());
            }
            else if (_panel->GetHorizontalAlignment() == Alignment::END)
            {
                newPosX = GetWidth() - _panel->GetWidth();
                newPosX -= std::round((GetWidth() - _panel->GetWidth()) * _panel->GetHorizontalOffsetPercent());
            }
            newPosX += _panel->GetHorizontalOffsetPixels();
            // Alternative (no branching):
            // int align = item->GetHorizontalAlignment() == Alignment::END;
            // newPosX += align * (_width - item->GetWidth());
            // newPosX += (-1 * align) * std::round((_width - item->GetWidth()) * item->GetHorizontalOffsetPercent());
            // newPosX += item->GetHorizontalOffsetPixels();
            int newPosY = 0;
            if (_panel->GetVerticalAlignment() == Alignment::START)
            {
                newPosY += std::round((GetHeight() - _panel->GetHeight()) * _panel->GetVerticalOffsetPercent());
            }
            else if (_panel->GetVerticalAlignment() == Alignment::END)
            {
                newPosY = GetHeight() - _panel->GetHeight();
                newPosY -= std::round((GetHeight() - _panel->GetHeight()) * _panel->GetVerticalOffsetPercent());
            }
            newPosY += _panel->GetVerticalOffsetPixels();
            _panel->SetPosition(newPosX, newPosY);

            _panel->Resize(_panel->GetWidth(), _panel->GetHeight());
            //_panel->Resize(width, height);
        }

        Base* _OnMouseMove(int x, int y)
        {
            return _panel->OnMouseMove(x, y);
        }

        void _OnMouseLeave()
        {
            _panel->OnMouseLeave();

            _hanging = true;
            _mouseLeaveTime = ztime::Main();
        }

        void _OnMouseEnter()
        {
            _panel->OnMouseEnter();

            _hanging = false;
            _fading = true;
            _fadeStartTime = ztime::Main();
            _startOpacity = GetOpacity();
            _targetOpacity = 1.0f;
        }

        Base* _OnLeftPressed(int x, int y)
        {
            return _panel->OnLeftPressed(x, y);
        }

        Base* _OnLeftReleased(int x, int y)
        {
            return _panel->OnLeftReleased(x, y);
        }

        Base* _OnRightPressed(int x, int y)
        {
            return _panel->OnRightPressed(x, y);
        }

        Base* _OnRightReleased(int x, int y)
        {
            return _panel->OnRightReleased(x, y);
        }

        Base* _OnWheelUp(int x, int y)
        {
            return _panel->OnWheelUp(x, y);
        }

        Base* _OnWheelDown(int x, int y)
        {
            return _panel->OnWheelDown(x, y);
        }

    public:
        std::list<Base*> GetChildren()
        {
            std::list<Base*> children;
            children.push_back(_panel);
            return children;
        }

        std::list<Base*> GetAllChildren()
        {
            std::list<Base*> children;
            children.push_back(_panel);
            auto panelChildren = _panel->GetAllChildren();
            if (!panelChildren.empty())
            {
                children.insert(children.end(), panelChildren.begin(), panelChildren.end());
            }
            return children;
        }

        Base* IterateTab()
        {
            return _panel->IterateTab();
        }

        const char* GetName() const { return "bottom_control_panel"; }
#pragma endregion

    private:
        Base* _panel = nullptr;

        TimePoint _mouseLeaveTime;
        bool _hanging;
        float _hangDuration = 2.0f;

        TimePoint _fadeStartTime;
        float _targetOpacity;
        float _startOpacity;
        bool _fading;
        float _fadeDuration = 0.1f;

    public:
        BottomControlPanel(Base* panel) : _panel(panel)
        {
            SetOpacity(0.0f);
            //Resize();

            _hanging = false;
            _fading = false;
        }
        ~BottomControlPanel() {}
        BottomControlPanel(BottomControlPanel&&) = delete;
        BottomControlPanel& operator=(BottomControlPanel&&) = delete;
        BottomControlPanel(const BottomControlPanel&) = delete;
        BottomControlPanel& operator=(const BottomControlPanel&) = delete;

        void Resize()
        {
            //SetWidth(_panel->GetWidth());
            //SetHeight(_panel->GetHeight());
            //SetX(_panel->GetX());
            //SetY(_panel->GetY());
            //_panel->SetPosition(0, 0);
        }
    };
}