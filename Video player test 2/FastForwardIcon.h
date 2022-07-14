#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "GameTime.h"

#include <sstream>

namespace zcom
{
    enum class FastForwardDirection
    {
        LEFT,
        RIGHT
    };

    class FastForwardIcon : public Base
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

                // Arrow movement is calculated in 'Draw()'
                // If 'progress' < 1.0f, then Redrawing is necessary every frame;
                InvokeRedraw();
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

            D2D1_RECT_F textrect;
            textrect.left = (size.width - _textLabel->GetWidth()) / 2;
            textrect.right = textrect.left + _textLabel->GetWidth();
            textrect.top = size.height / 2.0f;
            textrect.bottom = textrect.top + _textLabel->GetHeight();

            D2D1_ROUNDED_RECT roundedrect;
            roundedrect.radiusX = 5.0f;
            roundedrect.radiusY = 5.0f;
            roundedrect.rect.left = 0;
            roundedrect.rect.top = 0;
            roundedrect.rect.right = size.width;
            roundedrect.rect.bottom = size.width;
            
            g.target->FillRoundedRectangle(roundedrect, _brush);
            if (_textLabel->Redraw())
                _textLabel->Draw(g);
            g.target->DrawBitmap(_textLabel->Image(), textrect);

            // Draw triangle
            Duration elapsed = ztime::Main() - _displayStart;
            float progress = elapsed.GetDuration(MILLISECONDS) / (float)_arrowMoveDuration.GetDuration(MILLISECONDS);
            if (progress > 1.0f)
                progress = 1.0f;
            progress = 1.0f - powf(1.0f - progress, 3.0f);
            float travelLength = 10.0f;

            D2D1_POINT_2F trianglePos = { GetWidth() * 0.5f, GetHeight() * 0.5f };
            trianglePos.y -= 7.0f;

            D2D1_POINT_2F points1[3];
            D2D1_POINT_2F points2[3];
            if (_direction == FastForwardDirection::LEFT)
            {
                trianglePos.x += travelLength * 0.5f;
                trianglePos.x -= travelLength * progress;
                points1[0] = D2D1::Point2F(trianglePos.x - 10.0f, trianglePos.y);
                points1[1] = D2D1::Point2F(trianglePos.x + 0.0f, trianglePos.y - 5.0f);
                points1[2] = D2D1::Point2F(trianglePos.x + 0.0f, trianglePos.y + 5.0f);
                points2[0] = D2D1::Point2F(trianglePos.x - 0.0f, trianglePos.y);
                points2[1] = D2D1::Point2F(trianglePos.x + 10.0f, trianglePos.y - 5.0f);
                points2[2] = D2D1::Point2F(trianglePos.x + 10.0f, trianglePos.y + 5.0f);
            }
            else if (_direction == FastForwardDirection::RIGHT)
            {
                trianglePos.x -= travelLength * 0.5f;
                trianglePos.x += travelLength * progress;
                points1[0] = D2D1::Point2F(trianglePos.x + 10.0f, trianglePos.y);
                points1[1] = D2D1::Point2F(trianglePos.x - 0.0f, trianglePos.y - 5.0f);
                points1[2] = D2D1::Point2F(trianglePos.x - 0.0f, trianglePos.y + 5.0f);
                points2[0] = D2D1::Point2F(trianglePos.x + 0.0f, trianglePos.y);
                points2[1] = D2D1::Point2F(trianglePos.x - 10.0f, trianglePos.y - 5.0f);
                points2[2] = D2D1::Point2F(trianglePos.x - 10.0f, trianglePos.y + 5.0f);
            }

            // Create path geometry
            ID2D1PathGeometry* pTriangleGeometry1;
            ID2D1PathGeometry* pTriangleGeometry2;
            g.factory->CreatePathGeometry(&pTriangleGeometry1);
            g.factory->CreatePathGeometry(&pTriangleGeometry2);

            ID2D1GeometrySink* pSink1 = NULL;
            ID2D1GeometrySink* pSink2 = NULL;
            pTriangleGeometry1->Open(&pSink1);
            pTriangleGeometry2->Open(&pSink2);

            pSink1->SetFillMode(D2D1_FILL_MODE_WINDING);
            pSink1->BeginFigure(
                points1[0],
                D2D1_FIGURE_BEGIN_FILLED
            );
            pSink1->AddLines(points1, ARRAYSIZE(points1));
            pSink1->EndFigure(D2D1_FIGURE_END_CLOSED);
            pSink1->Close();
            pSink1->Release();

            pSink2->SetFillMode(D2D1_FILL_MODE_WINDING);
            pSink2->BeginFigure(
                points2[0],
                D2D1_FIGURE_BEGIN_FILLED
            );
            pSink2->AddLines(points2, ARRAYSIZE(points2));
            pSink2->EndFigure(D2D1_FIGURE_END_CLOSED);
            pSink2->Close();
            pSink2->Release();

            // Fill triangle
            ID2D1SolidColorBrush* pBrush = NULL;
            g.target->CreateSolidColorBrush(
                D2D1::ColorF(0.8f, 0.8f, 0.8f),
                &pBrush
            );
            g.target->FillGeometry(
                pTriangleGeometry1,
                pBrush
            );
            g.target->FillGeometry(
                pTriangleGeometry2,
                pBrush
            );
            pBrush->Release();
            pTriangleGeometry1->Release();
            pTriangleGeometry2->Release();
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "fast_forward_icon"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        ID2D1PathGeometry* _triangleGeometry = nullptr;
        FastForwardDirection _direction;
        TimePoint _displayStart = TimePoint(-1, MINUTES);
        Duration _displayDuration = Duration(800, MILLISECONDS);
        Duration _arrowMoveDuration = Duration(600, MILLISECONDS);
        //Duration _fadeDuration = Duration(400, MILLISECONDS);

        std::unique_ptr<Label> _textLabel = nullptr;

    protected:
        friend class Scene;
        friend class Base;
        FastForwardIcon(Scene* scene) : Base(scene) {}
        void Init(FastForwardDirection direction)
        {
            SetVisible(false);

            _textLabel = Create<Label>();
            _textLabel->SetSize(60, 20);
            _textLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _textLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _textLabel->SetFontSize(16.0f);
            _textLabel->SetFontColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));

            _direction = direction;
        }
    public:
        ~FastForwardIcon()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        FastForwardIcon(FastForwardIcon&&) = delete;
        FastForwardIcon& operator=(FastForwardIcon&&) = delete;
        FastForwardIcon(const FastForwardIcon&) = delete;
        FastForwardIcon& operator=(const FastForwardIcon&) = delete;

        void Show(int seconds)
        {
            std::wstringstream wss(L"");
            wss << seconds << "s";
            _textLabel->SetText(wss.str());
            _displayStart = ztime::Main();
            SetVisible(true);
        }
    };
}