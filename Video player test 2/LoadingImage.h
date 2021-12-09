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
    private:
        void _OnUpdate()
        {

        }

        void _OnDraw(Graphics g)
        {
            if (!_brush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::WhiteSmoke), &_brush);
                g.refs->push_back((IUnknown**)&_brush);
            }

            float phase = fmod(ztime::Main().GetTime(MILLISECONDS) / 1000.0f, _animationDuration);
            float dotHeight = 20.0f;

            { // Dot 1
                float localPhase = phase * 2.0f;
                float height = (-cosf(localPhase * Math::TAU) * 0.5f + 0.5f) * dotHeight;
                if (localPhase > 1.0f) height = 0;
                D2D1_ELLIPSE ellipse;
                ellipse.point = D2D1::Point2F(GetWidth() * 0.5f, GetHeight() * 0.5f);
                ellipse.point.x -= 15.0f;
                ellipse.point.y -= height;
                ellipse.radiusX = 5.0f;
                ellipse.radiusY = 5.0f;
                g.target->FillEllipse(ellipse, _brush);
            }

            { // Dot 2
                float localPhase = (phase - 0.15f) * 2.0f;
                float height = (-cosf(localPhase * Math::TAU) * 0.5f + 0.5f) * dotHeight;
                if (localPhase > 1.0f) height = 0;
                else if (localPhase < 0.0f) height = 0;
                D2D1_ELLIPSE ellipse;
                ellipse.point = D2D1::Point2F(GetWidth() * 0.5f, GetHeight() * 0.5f);
                ellipse.point.y -= height;
                ellipse.radiusX = 5.0f;
                ellipse.radiusY = 5.0f;
                g.target->FillEllipse(ellipse, _brush);
            }

            { // Dot 3
                float localPhase = (phase - 0.3f) * 2.0f;
                float height = (-cosf(localPhase * Math::TAU) * 0.5f + 0.5f) * dotHeight;
                if (localPhase > 1.0f) height = 0;
                else if (localPhase < 0.0f) height = 0;
                D2D1_ELLIPSE ellipse;
                ellipse.point = D2D1::Point2F(GetWidth() * 0.5f, GetHeight() * 0.5f);
                ellipse.point.x += 15.0f;
                ellipse.point.y -= height;
                ellipse.radiusX = 5.0f;
                ellipse.radiusY = 5.0f;
                g.target->FillEllipse(ellipse, _brush);
            }
        }

        void _OnResize(int width, int height)
        {

        }

        Base* _OnMouseMove(int x, int y)
        {
            return this;
        }

        void _OnMouseLeave()
        {

        }

        void _OnMouseEnter()
        {

        }

        Base* _OnLeftPressed(int x, int y)
        {
            return this;
        }

        Base* _OnLeftReleased(int x, int y)
        {
            return this;
        }

        Base* _OnRightPressed(int x, int y)
        {
            return this;
        }

        Base* _OnRightReleased(int x, int y)
        {
            return this;
        }

        Base* _OnWheelUp(int x, int y)
        {
            return this;
        }

        Base* _OnWheelDown(int x, int y)
        {
            return this;
        }

    public:
        std::list<Base*> GetChildren()
        {
            return std::list<Base*>();
        }

        std::list<Base*> GetAllChildren()
        {
            return std::list<Base*>();
        }

        Base* IterateTab()
        {
            if (!Selected())
                return this;
            else
                return nullptr;
        }

        const char* GetName() const { return "loading_image"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        float _animationDuration = 1.2f; // Seconds

    public:
        LoadingImage()
        {

        }
        ~LoadingImage()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        LoadingImage(LoadingImage&&) = delete;
        LoadingImage& operator=(LoadingImage&&) = delete;
        LoadingImage(const LoadingImage&) = delete;
        LoadingImage& operator=(const LoadingImage&) = delete;
    };
}