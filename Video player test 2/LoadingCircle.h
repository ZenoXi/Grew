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
                g.target->CreateSolidColorBrush(D2D1::ColorF(0, 0.5f), &_brush);
                g.refs->push_back((IUnknown**)&_brush);
            }

            auto size = g.target->GetSize();

            D2D1_RECT_F circlerect;
            circlerect.left = size.width / 2.0f - 32.0f;
            circlerect.right = size.width / 2.0f + 32.0f;
            circlerect.top = size.width / 2.0f - 32.0f;
            circlerect.bottom = size.width / 2.0f + 32.0f;

            D2D1_RECT_F textrect;
            textrect.left = (size.width - _textLabel->GetWidth()) / 2;
            textrect.right = textrect.left + _textLabel->GetWidth();
            textrect.top = circlerect.bottom + 10;
            textrect.bottom = circlerect.bottom + 10 + _textLabel->GetHeight();

            D2D1_ROUNDED_RECT roundedrect;
            roundedrect.radiusX = 5.0f;
            roundedrect.radiusY = 5.0f;
            roundedrect.rect.left = 0;
            roundedrect.rect.top = 0;
            roundedrect.rect.right = size.width;
            if (_textLabel->GetText().empty())
                roundedrect.rect.bottom = size.width;
            else
                roundedrect.rect.bottom = textrect.bottom + 10;

            g.target->FillRoundedRectangle(roundedrect, _brush);
            auto rDuration = _rotationDuration.GetDuration(MILLISECONDS);
            float angle = (ztime::Main().GetTime(MILLISECONDS) % rDuration) / (float)rDuration * 360.0f;
            g.target->SetTransform(D2D1::Matrix3x2F::Rotation(angle, { size.width / 2.0f, size.width / 2.0f }));
            g.target->DrawBitmap(_circleBitmap, circlerect);
            g.target->SetTransform(D2D1::Matrix3x2F::Identity());

            if (_textLabel->Redraw())
                _textLabel->Draw(g);
            g.target->DrawBitmap(_textLabel->Image(), textrect);

        }

        void _OnResize(int width, int height)
        {
            _textLabel->Resize(width - 10, 20);
        }

    public:
        const char* GetName() const { return "loading_circle"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        ID2D1Bitmap* _circleBitmap = nullptr;
        Duration _rotationDuration = Duration(800, MILLISECONDS);

        std::unique_ptr<Label> _textLabel = nullptr;

    protected:
        friend class Scene;
        friend class Base;
        LoadingCircle(Scene* scene) : Base(scene) {}
        void Init()
        {
            _circleBitmap = ResourceManager::GetImage("loading_circle");
            _textLabel = Create<Label>();
            _textLabel->SetSize(GetWidth() - 10, 20);
            _textLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _textLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _textLabel->SetFontSize(16.0f);
            //_textLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
            _textLabel->SetWordWrap(true);
        }
    public:
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
            if (_textLabel->GetText() != text)
            {
                _textLabel->SetText(text);
                _textLabel->Resize(_textLabel->GetWidth(), ceil(_textLabel->GetTextHeight()));
            }
        }
    };
}