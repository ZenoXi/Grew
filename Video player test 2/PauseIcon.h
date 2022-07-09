#pragma once

#include "ComponentBase.h"

#include "GameTime.h"

#include <sstream>

namespace zcom
{
    class PauseIcon : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            Duration elapsed = ztime::Main() - _displayStart;
            float progress = elapsed.GetDuration(MILLISECONDS) / (float)_displayDuration.GetDuration(MILLISECONDS);
            if (progress >= 1.0f)
            {
                SetVisible(false);
            }
            else
            {
                if (progress > 0.5f)
                {
                    progress -= 0.5f;
                    progress *= 2.0f;
                    SetOpacity(1.0f - powf(progress, 0.5f));
                }
                else
                {
                    SetOpacity(1.0f);
                }
            }
        }

        void _OnDraw(Graphics g)
        {
            if (!_brush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(0, 0.5f), &_brush);
                g.refs->push_back((IUnknown**)&_brush);
            }

            auto size = g.target->GetSize();

            D2D1_ROUNDED_RECT roundedrect;
            roundedrect.radiusX = 5.0f;
            roundedrect.radiusY = 5.0f;
            roundedrect.rect.left = 0;
            roundedrect.rect.top = 0;
            roundedrect.rect.right = size.width;
            roundedrect.rect.bottom = size.width;

            g.target->FillRoundedRectangle(roundedrect, _brush);

            // Draw pause icon
            D2D1_POINT_2F centerPos = { size.width / 2.0f, size.height / 2.0f };
            float rectRadius = 10.0f;

            D2D1_RECT_F bar1;
            bar1.left = centerPos.x - rectRadius;
            bar1.top = centerPos.y - rectRadius;
            bar1.right = bar1.left + 7.0f;
            bar1.bottom = centerPos.y + rectRadius;

            D2D1_RECT_F bar2;
            bar2.right = centerPos.x + rectRadius;
            bar2.top = centerPos.y - rectRadius;
            bar2.left = bar2.right - 7.0f;
            bar2.bottom = centerPos.y + rectRadius;

            ID2D1SolidColorBrush* pBrush = NULL;
            g.target->CreateSolidColorBrush(
                D2D1::ColorF(0.8f, 0.8f, 0.8f),
                &pBrush
            );

            g.target->FillRectangle(bar1, pBrush);
            g.target->FillRectangle(bar2, pBrush);
            pBrush->Release();
        }

        void _OnResize(int width, int height)
        {

        }

    public:
        const char* GetName() const { return "pause_icon"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        ID2D1PathGeometry* _triangleGeometry = nullptr;
        TimePoint _displayStart = TimePoint(-1, MINUTES);
        Duration _displayDuration = Duration(800, MILLISECONDS);

    public:
        PauseIcon()
        {
            SetVisible(false);
        }
        ~PauseIcon()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        PauseIcon(PauseIcon&&) = delete;
        PauseIcon& operator=(PauseIcon&&) = delete;
        PauseIcon(const PauseIcon&) = delete;
        PauseIcon& operator=(const PauseIcon&) = delete;

        void Show()
        {
            _displayStart = ztime::Main();
            SetVisible(true);
        }
    };
}