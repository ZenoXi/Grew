#pragma once

#include "ComponentBase.h"

#include "GameTime.h"

#include <sstream>

namespace zcom
{
    class ResumeIcon : public Base
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

            // Draw resume icon
            D2D1_POINT_2F centerPos = { size.width / 2.0f, size.height / 2.0f };
            float rectRadius = 10.0f;

            D2D1_POINT_2F points[3];
            points[0] = D2D1::Point2F(centerPos.x + rectRadius, centerPos.y);
            points[1] = D2D1::Point2F(centerPos.x - rectRadius, centerPos.y - rectRadius);
            points[2] = D2D1::Point2F(centerPos.x - rectRadius, centerPos.y + rectRadius);

            // Create path geometry
            ID2D1PathGeometry* pTriangleGeometry;
            g.factory->CreatePathGeometry(&pTriangleGeometry);

            ID2D1GeometrySink* pSink = NULL;
            pTriangleGeometry->Open(&pSink);

            pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
            pSink->BeginFigure(
                points[0],
                D2D1_FIGURE_BEGIN_FILLED
            );
            pSink->AddLines(points, ARRAYSIZE(points));
            pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
            pSink->Close();
            pSink->Release();

            // Fill triangle
            ID2D1SolidColorBrush* pBrush = NULL;
            g.target->CreateSolidColorBrush(
                D2D1::ColorF(0.8f, 0.8f, 0.8f),
                &pBrush
            );
            g.target->FillGeometry(
                pTriangleGeometry,
                pBrush
            );
            pBrush->Release();
            pTriangleGeometry->Release();
        }

        void _OnResize(int width, int height)
        {

        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "resume_icon"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        ID2D1PathGeometry* _triangleGeometry = nullptr;
        TimePoint _displayStart = TimePoint(-1, MINUTES);
        Duration _displayDuration = Duration(800, MILLISECONDS);

    protected:
        friend class Scene;
        friend class Base;
        ResumeIcon(Scene* scene) : Base(scene) {}
        void Init()
        {
            SetVisible(false);
        }
    public:
        ~ResumeIcon()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        ResumeIcon(ResumeIcon&&) = delete;
        ResumeIcon& operator=(ResumeIcon&&) = delete;
        ResumeIcon(const ResumeIcon&) = delete;
        ResumeIcon& operator=(const ResumeIcon&) = delete;

        void Show()
        {
            _displayStart = ztime::Main();
            SetVisible(true);
        }
    };
}