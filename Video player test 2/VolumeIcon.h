#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "ResourceManager.h"
#include "GameTime.h"

#include <sstream>

namespace zcom
{
    class VolumeIcon : public Base
    {
#pragma region base_class
    private:
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

            D2D1_RECT_F bitmaprect;
            bitmaprect.left = size.width / 2.0f - 10.5f;
            bitmaprect.right = size.width / 2.0f + 10.5f;
            bitmaprect.top = size.width / 2.0f - 10.0f - 7.0f;
            bitmaprect.bottom = size.width / 2.0f + 10.0f - 7.0f;

            D2D1_RECT_F textrect;
            textrect.left = (size.width - _textLabel->GetWidth()) / 2;
            textrect.right = textrect.left + _textLabel->GetWidth();
            textrect.top = size.height / 2.0f + 2.0f;
            textrect.bottom = textrect.top + _textLabel->GetHeight();

            D2D1_ROUNDED_RECT roundedrect;
            roundedrect.radiusX = 5.0f;
            roundedrect.radiusY = 5.0f;
            roundedrect.rect.left = 0;
            roundedrect.rect.top = 0;
            roundedrect.rect.right = size.width;
            roundedrect.rect.bottom = size.width;

            g.target->FillRoundedRectangle(roundedrect, _brush);
            g.target->DrawBitmap(_iconBitmap, bitmaprect);
            g.target->DrawBitmap(_textLabel->Draw(g), textrect);
        }

        void _OnResize(int width, int height)
        {

        }

    public:
        const char* GetName() const { return "volume_icon"; }
#pragma endregion

    private:
        ID2D1SolidColorBrush* _brush = nullptr;
        ID2D1Bitmap* _iconBitmap = nullptr;
        TimePoint _displayStart = TimePoint(-1, MINUTES);
        Duration _displayDuration = Duration(800, MILLISECONDS);

        std::unique_ptr<Label> _textLabel = nullptr;

    public:
        VolumeIcon()
        {
            SetVisible(false);

            _iconBitmap = ResourceManager::GetImage("volume_icon");
            _textLabel = std::make_unique<Label>();
            _textLabel->SetSize(60, 20);
            _textLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _textLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _textLabel->SetFontSize(16.0f);
            _textLabel->SetFontColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
            _textLabel->Resize();
        }
        ~VolumeIcon()
        {
            SafeFullRelease((IUnknown**)&_brush);
        }
        VolumeIcon(VolumeIcon&&) = delete;
        VolumeIcon& operator=(VolumeIcon&&) = delete;
        VolumeIcon(const VolumeIcon&) = delete;
        VolumeIcon& operator=(const VolumeIcon&) = delete;

        void Show(int volumePercent)
        {
            std::wstringstream wss(L"");
            wss << volumePercent << "%";
            _textLabel->SetText(wss.str());
            _displayStart = ztime::Main();
            SetVisible(true);
        }
    };
}