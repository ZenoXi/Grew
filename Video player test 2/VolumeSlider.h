#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "ComponentBase.h"
#include "GameTime.h"
#include "Options.h"
#include "OptionNames.h"
#include "FloatOptionAdapter.h"
#include "Functions.h"
#include "Transition.h"
#include "ResourceManager.h"

#include <sstream>

namespace zcom
{
    class VolumeSlider : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            float initialHeight = _volumeBarHeight;
            _volumeBarHeightTransition.Apply(_volumeBarHeight);
            if (_volumeBarHeight != initialHeight)
                InvokeRedraw();

            int newWidth = GetBaseWidth();
            _itemWidthTransition.Apply(newWidth);
            SetBaseWidth(newWidth);

            if (_waitingAfterMouseLeave && (ztime::Main() - _waitStart) > _waitDuration)
            {
                _itemWidthTransition.Start(GetWidth(), 30);
                _waitingAfterMouseLeave = false;
            }
        }

        void _OnDraw(Graphics g)
        {
            // Create brushes
            if (!_selectedPartBrush)
            {
                // RGB(71, 161, 244) matches the color of DodgerBlue with 75% opacity over LightGray
                // This is done to match the volume slider and time slider colors
                g.target->CreateSolidColorBrush(D2D1::ColorF(RGB(244, 161, 71) /* R and B flipped */), &_selectedPartBrush);
                g.refs->push_back({ (IUnknown**)&_selectedPartBrush, "Volume slider selected part brush" });
            }
            if (!_volumeBarMarkerBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue), &_volumeBarMarkerBrush);
                g.refs->push_back({ (IUnknown**)&_volumeBarMarkerBrush, "Volume slider marker brush" });
            }
            if (!_remainingPartBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &_remainingPartBrush);
                g.refs->push_back({ (IUnknown**)&_remainingPartBrush, "Volume slider remaining part brush" });
            }

            // Update volume icon color
            if (GetMouseInside())
                _volumeImage->SetTintColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
            else
                _volumeImage->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));

            // Draw Volume icon
            if (_volumeImage->Redraw())
                _volumeImage->Draw(g);
            g.target->DrawBitmap(_volumeImage->Base::Image(), D2D1::RectF(0.0f, 0.0f, 30.0f, 30.0f));

            // Draw the seek bar
            float progress = _value;
            int iconWidth = 30;
            int textWidth = _volumeLabel->GetWidth();
            int volumeSliderWidth = _extendedWidth - textWidth - iconWidth - _margins * 2;
            int selectedPartWidth = volumeSliderWidth * progress;
            g.target->FillRectangle(
                D2D1::RectF(
                    iconWidth + _margins,
                    GetHeight() / 2.0f - _volumeBarHeight,
                    iconWidth + _margins + selectedPartWidth,
                    GetHeight() / 2.0f + _volumeBarHeight
                ),
                _selectedPartBrush
            );
            g.target->FillRectangle(
                D2D1::RectF(
                    iconWidth + _margins + selectedPartWidth,
                    GetHeight() / 2.0f - _volumeBarHeight,
                    iconWidth + _margins + volumeSliderWidth,
                    GetHeight() / 2.0f + _volumeBarHeight
                ),
                _remainingPartBrush
            );
            if (_volumeHovered)
            {
                g.target->FillEllipse(
                    D2D1::Ellipse(
                        D2D1::Point2F(
                            iconWidth + _margins + selectedPartWidth,
                            GetHeight() / 2.0f
                        ),
                        5.0f,
                        5.0f
                    ),
                    _volumeBarMarkerBrush
                );
            }

            // Draw value string
            if (_volumeLabel->Redraw())
                _volumeLabel->Draw(g);
            g.target->DrawBitmap(
                _volumeLabel->Image(),
                D2D1::RectF(
                    _extendedWidth - _volumeLabel->GetWidth(),
                    0,
                    _extendedWidth,
                    GetHeight()
                )
            );
        }

        EventTargets _OnMouseMove(int deltaX, int deltaY)
        {
            if (deltaX == 0 && deltaY == 0)
                return EventTargets().Add(this, GetMousePosX(), GetMousePosY());

            int iconWidth = 30;
            int textWidth = _volumeLabel->GetWidth();
            int volumeSliderWidth = _extendedWidth - textWidth - iconWidth - _margins * 2;
            int xPos = GetMousePosX() - iconWidth - _margins;
            if (xPos >= 0 && xPos <= volumeSliderWidth)
            {
                if (!_volumeHovered)
                {
                    _volumeHovered = true;
                    _volumeBarHeightTransition.Start(_volumeBarHeight, 3.0f);
                }
            }
            else if (!_held)
            {
                if (_volumeHovered)
                {
                    _volumeHovered = false;
                    _volumeBarHeightTransition.Start(_volumeBarHeight, 1.0f);
                }
            }
            if (_held)
            {
                if (xPos < 0) xPos = 0;
                if (xPos > volumeSliderWidth) xPos = volumeSliderWidth;
                float xPosNorm = xPos / (float)volumeSliderWidth;
                SetValue(xPosNorm);
                _moved = true;
            }

            InvokeRedraw();
            return EventTargets().Add(this, GetMousePosX(), GetMousePosY());
        }

        void _OnMouseEnter()
        {
            _itemWidthTransition.Start(GetWidth(), _extendedWidth);
            _waitingAfterMouseLeave = false;
        }

        void _OnMouseLeave()
        {
            _waitingAfterMouseLeave = true;
            _waitStart = ztime::Main();

            if (_volumeHovered)
            {
                _volumeHovered = false;
                _volumeBarHeightTransition.Start(_volumeBarHeight, 1.0f);
            }
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            int iconWidth = 30;
            int textWidth = _volumeLabel->GetWidth();
            int volumeSliderWidth = _extendedWidth - textWidth - iconWidth - _margins * 2;
            int xPos = GetMousePosX() - iconWidth - _margins;
            if (xPos >= 0 && xPos <= volumeSliderWidth)
            {
                float xPosNorm = xPos / (float)volumeSliderWidth;
                SetValue(xPosNorm);
                _moved = true;
                _held = true;
                InvokeRedraw();
            }
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnLeftReleased(int x, int y)
        {
            _held = false;
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnWheelUp(int x, int y)
        {
            if (!_held)
            {
                SetValue(_value + 0.05f);
                _moved = true;
                InvokeRedraw();
            }
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnWheelDown(int x, int y)
        {
            if (!_held)
            {
                SetValue(_value - 0.05f);
                _moved = true;
                InvokeRedraw();
            }
            return EventTargets().Add(this, x, y);
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "volume_slider"; }
#pragma endregion

    private:
        float _value = 0;
        float _exponent = 2.0f;

        bool _moved = false;
        bool _held = false;
        float _textHeight = 0.0f;
        float _maxTextWidth = 0.0f;

        float _volumeBarHeight = 1.0f; // This is height from center to each edge. Total height is 2 * _timeBarHeight
        Transition<float> _volumeBarHeightTransition = Transition<float>(Duration(100, MILLISECONDS));
        int _itemWidth = 30;
        int _extendedWidth = 0;
        Transition<int> _itemWidthTransition = Transition<int>(Duration(100, MILLISECONDS));

        bool _waitingAfterMouseLeave = false;
        TimePoint _waitStart = 0;
        Duration _waitDuration = Duration(500, MILLISECONDS);

        float _margins = 5.0f;

        bool _volumeHovered = false;

        ID2D1SolidColorBrush* _selectedPartBrush = nullptr;
        ID2D1SolidColorBrush* _volumeBarMarkerBrush = nullptr;
        ID2D1SolidColorBrush* _remainingPartBrush = nullptr;

        std::unique_ptr<zcom::Image> _volumeImage = nullptr;
        std::unique_ptr<Label> _volumeLabel = nullptr;

        TimePoint _mouseHoverStart = 0;

    protected:
        friend class Scene;
        friend class Base;
        VolumeSlider(Scene* scene) : Base(scene) {}
        void Init(float value, int extendedWidth = 150)
        {
            _value = value;
            _extendedWidth = extendedWidth;

            _volumeImage = Create<zcom::Image>(ResourceManager::GetImage("volume_22x22"));
            _volumeImage->SetSize(30, 30);
            _volumeImage->SetPlacement(ImagePlacement::CENTER);
            _volumeImage->SetTintColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));

            _volumeLabel = Create<Label>(L"");
            _volumeLabel->SetSize(40, 30);
            _volumeLabel->SetFontSize(15.0f);
            _volumeLabel->SetFont(L"Calibri");
            _volumeLabel->SetFontColor(D2D1::ColorF(D2D1::ColorF::LightGray));
            _volumeLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _volumeLabel->SetVerticalTextAlignment(Alignment::CENTER);

            // Set initial text
            std::wstringstream valueStr;
            valueStr << (int)roundf(_value * 100.0f) << "%";
            _volumeLabel->SetText(valueStr.str());
        }
    public:
        ~VolumeSlider()
        {
            // Release resources
            SafeFullRelease((IUnknown**)&_selectedPartBrush);
            SafeFullRelease((IUnknown**)&_volumeBarMarkerBrush);
            SafeFullRelease((IUnknown**)&_remainingPartBrush);
        }
        VolumeSlider(VolumeSlider&&) = delete;
        VolumeSlider& operator=(VolumeSlider&&) = delete;
        VolumeSlider(const VolumeSlider&) = delete;
        VolumeSlider& operator=(const VolumeSlider&) = delete;

        bool Moved()
        {
            bool val = _moved;
            _moved = false;
            return val;
        }

        float GetValue() const
        {
            return _value;
        }

        void SetValue(float value)
        {
            if (value > 1.0f) value = 1.0f;
            else if (value < 0.0f) value = 0.0f;
            _value = value;

            // Set value string
            std::wstringstream valueStr;
            valueStr << (int)roundf(_value * 100.0f) << "%";
            _volumeLabel->SetText(valueStr.str());

            Options::Instance()->SetValue(OPTIONS_VOLUME, FloatOptionAdapter(GetVolume()).ToOptionString());

            InvokeRedraw();
        }

        float GetVolume() const
        {
            return powf(_value, _exponent);
        }

        void SetVolume(float volume)
        {
            if (volume > 1.0f) volume = 1.0f;
            else if (volume < 0.0f) volume = 0.0f;
            SetValue(powf(volume, 1.0f / _exponent));
        }

        float GetExponent() const
        {
            return _exponent;
        }

        // If the slider is in position X (0.0-1.0), the volume will be set to X^'exponent'
        // Default - 2.0
        void SetExponent(float exponent)
        {
            _exponent = exponent;
        }
    };
}