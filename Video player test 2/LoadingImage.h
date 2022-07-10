#pragma once

#include "ComponentBase.h"
#include "ResourceManager.h"
#include "GameTime.h"
#include "Constants.h"

namespace zcom
{
    class LoadingImage : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            if (GetVisible())
                InvokeRedraw();
        }

        void _OnDraw(Graphics g)
        {
            if (!_brush)
            {
                g.target->CreateSolidColorBrush(_dotColor, &_brush);
                g.refs->push_back((IUnknown**)&_brush);
            }

            float phase = fmod((ztime::Main() - _animationStart).GetDuration(MILLISECONDS) / 1000.0f, _animationDuration.GetDuration(MICROSECONDS) / 1000000.0f);

            { // Dot 1
                float localPhase = phase * 2.0f;
                float height = (-cosf(localPhase * Math::TAU) * 0.5f + 0.5f) * _dotHeight;
                if (localPhase > 1.0f) height = 0;
                D2D1_ELLIPSE ellipse;
                ellipse.point = D2D1::Point2F(GetWidth() * 0.5f, GetHeight() * 0.5f);
                ellipse.point.x -= _dotSpacing;
                ellipse.point.y -= height;
                ellipse.radiusX = _dotRadius;
                ellipse.radiusY = _dotRadius;
                g.target->FillEllipse(ellipse, _brush);
            }

            { // Dot 2
                float localPhase = (phase - 0.15f) * 2.0f;
                float height = (-cosf(localPhase * Math::TAU) * 0.5f + 0.5f) * _dotHeight;
                if (localPhase > 1.0f) height = 0;
                else if (localPhase < 0.0f) height = 0;
                D2D1_ELLIPSE ellipse;
                ellipse.point = D2D1::Point2F(GetWidth() * 0.5f, GetHeight() * 0.5f);
                ellipse.point.y -= height;
                ellipse.radiusX = _dotRadius;
                ellipse.radiusY = _dotRadius;
                g.target->FillEllipse(ellipse, _brush);
            }

            { // Dot 3
                float localPhase = (phase - 0.3f) * 2.0f;
                float height = (-cosf(localPhase * Math::TAU) * 0.5f + 0.5f) * _dotHeight;
                if (localPhase > 1.0f) height = 0;
                else if (localPhase < 0.0f) height = 0;
                D2D1_ELLIPSE ellipse;
                ellipse.point = D2D1::Point2F(GetWidth() * 0.5f, GetHeight() * 0.5f);
                ellipse.point.x += _dotSpacing;
                ellipse.point.y -= height;
                ellipse.radiusX = _dotRadius;
                ellipse.radiusY = _dotRadius;
                g.target->FillEllipse(ellipse, _brush);
            }
        }

        void _OnResize(int width, int height)
        {

        }

    public:
        const char* GetName() const { return "loading_image"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        D2D1_COLOR_F _dotColor = D2D1::ColorF(D2D1::ColorF::WhiteSmoke);
        Duration _animationDuration = Duration(1200, MILLISECONDS);
        TimePoint _animationStart = 0;
        float _dotHeight = 20.0f;
        float _dotRadius = 5.0f;
        float _dotSpacing = 15.0f;

    protected:
        friend class Scene;
        friend class Base;
        LoadingImage(Scene* scene) : Base(scene) {}
        void Init() {}
    public:
        ~LoadingImage()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        LoadingImage(LoadingImage&&) = delete;
        LoadingImage& operator=(LoadingImage&&) = delete;
        LoadingImage(const LoadingImage&) = delete;
        LoadingImage& operator=(const LoadingImage&) = delete;

        void SetAnimationDuration(Duration duration)
        {
            _animationDuration = duration;
        }

        void ResetAnimation()
        {
            _animationStart = ztime::Main();
        }

        void SetDotHeight(float height)
        {
            _dotHeight = height;
        }

        void SetDotRadius(float radius)
        {
            _dotRadius = radius;
        }

        void SetDotSpacing(float spacing)
        {
            _dotSpacing = spacing;
        }

        void SetDotColor(D2D1_COLOR_F color)
        {
            _dotColor = color;
            SafeFullRelease((IUnknown**)&_brush);
        }
    };
}