#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "ResourceManager.h"
#include "GameTime.h"

namespace zcom
{
    class LoadingCircle : public Base
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
                g.target->CreateSolidColorBrush(D2D1::ColorF(0, 0.5f), &_brush);
                g.refs->push_back((IUnknown**)&_brush);
            }

            auto size = g.target->GetSize();
            D2D1_RECT_F rect1;
            rect1.left = 0;
            rect1.top = 0;
            rect1.right = size.width;
            rect1.bottom = size.height;
            D2D1_RECT_F rect2;
            rect2.left = size.width / 2.0f - 32.0f;
            rect2.right = size.width / 2.0f + 32.0f;
            rect2.top = size.width / 2.0f - 32.0f;
            rect2.bottom = size.width / 2.0f + 32.0f;
            D2D1_ROUNDED_RECT rrect;
            rrect.radiusX = 5.0f;
            rrect.radiusY = 5.0f;
            rrect.rect = rect1;
            g.target->FillRoundedRectangle(rrect, _brush);
            auto rDuration = _rotationDuration.GetDuration(MILLISECONDS);
            float angle = (ztime::Main().GetTime(MILLISECONDS) % rDuration) / (float)rDuration * 360.0f;
            g.target->SetTransform(D2D1::Matrix3x2F::Rotation(angle, { size.width / 2.0f, size.width / 2.0f }));
            g.target->DrawBitmap(_circleBitmap, rect2);
            g.target->SetTransform(D2D1::Matrix3x2F::Identity());

            D2D1_RECT_F brect;
            brect.left = (size.width - _textLabel->GetWidth()) / 2;
            brect.right = brect.left + _textLabel->GetWidth();
            brect.top = size.height - 10 - _textLabel->GetHeight();
            brect.bottom = size.height - 10;
            g.target->DrawBitmap(_textLabel->Draw(g), brect);
        }

        void _OnResize(int width, int height)
        {
            _textLabel->SetSize(width - 10, 20);
            _textLabel->Resize();
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

        const char* GetName() const { return "loading_circle"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        ID2D1Bitmap* _circleBitmap = nullptr;
        Duration _rotationDuration = Duration(800, MILLISECONDS);

        std::unique_ptr<Label> _textLabel = nullptr;

    public:
        LoadingCircle()
        {
            _circleBitmap = ResourceManager::GetImage("loading_circle");
            _textLabel = std::make_unique<Label>();
            _textLabel->SetSize(GetWidth() - 10, 20);
            _textLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _textLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _textLabel->Resize();
        }
        ~LoadingCircle()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        LoadingCircle(LoadingCircle&&) = delete;
        LoadingCircle& operator=(LoadingCircle&&) = delete;
        LoadingCircle(const LoadingCircle&) = delete;
        LoadingCircle& operator=(const LoadingCircle&) = delete;

        void SetRotationDuration(Duration duration)
        {
            _rotationDuration = duration;
        }

        void SetLoadingText(std::wstring text)
        {

        }
    };
}