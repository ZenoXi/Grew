#pragma once

#include "ComponentBase.h"

namespace zcom
{
    class ControlBarBackground : public Base
    {
#pragma region base_class
    protected:
        void _OnDraw(Graphics g)
        {
            ID2D1GradientStopCollection* gradientStopCollection = nullptr;
            ID2D1LinearGradientBrush* brush = nullptr;

            // The following gradient values are generated using this formula:
            //// opacity = sin(position * pi - pi/2) / 2 + 0.5
            //// Desmos format: \frac{\sin\left(x\cdot\pi-\frac{\pi}{2}\right)}{2}+0.5
            // This is done because the default interpolation doesn't
            // look linear at all, and Direct2D doesn't provide ways
            // to do this cleanly (or I dindn't find/understand them).

            constexpr int STOP_COUNT = 9;
            D2D1_GRADIENT_STOP gradientStops[STOP_COUNT];
            gradientStops[0].position = 0.0f;
            gradientStops[1].position = 0.125f;
            gradientStops[2].position = 0.25f;
            gradientStops[3].position = 0.375f;
            gradientStops[4].position = 0.5f;
            gradientStops[5].position = 0.625f;
            gradientStops[6].position = 0.75f;
            gradientStops[7].position = 0.875f;
            gradientStops[8].position = 1.0f;
            gradientStops[0].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f * 0.8f);
            gradientStops[1].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0381f * 0.8f);
            gradientStops[2].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1465f * 0.8f);
            gradientStops[3].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.3087f * 0.8f);
            gradientStops[4].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5 * 0.8f);
            gradientStops[5].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.6913f * 0.8f);
            gradientStops[6].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.8536f * 0.8f);
            gradientStops[7].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.9619f * 0.8f);
            gradientStops[8].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f * 0.8f);

            g.target->CreateGradientStopCollection(
                gradientStops,
                STOP_COUNT,
                D2D1_GAMMA_2_2,
                D2D1_EXTEND_MODE_CLAMP,
                &gradientStopCollection
            );
            g.target->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(0, 0),
                    D2D1::Point2F(0, 60)),
                gradientStopCollection,
                &brush
            );
            g.target->FillRectangle(D2D1::RectF(0.0f, 0.0f, GetWidth(), GetHeight()), brush);

            brush->Release();
            gradientStopCollection->Release();
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "control_bar_background"; }
#pragma endregion

    protected:
        friend class Scene;
        friend class Base;
        ControlBarBackground(Scene* scene) : Base(scene) {}
        void Init() {}
    public:
        ~ControlBarBackground() {}
        ControlBarBackground(ControlBarBackground&&) = delete;
        ControlBarBackground& operator=(ControlBarBackground&&) = delete;
        ControlBarBackground(const ControlBarBackground&) = delete;
        ControlBarBackground& operator=(const ControlBarBackground&) = delete;
    };
}