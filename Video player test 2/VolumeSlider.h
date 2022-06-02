#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "ComponentBase.h"
#include "GameTime.h"
#include "Options.h"
#include "Functions.h"

#include <sstream>

namespace zcom
{
    class VolumeSlider : public Base
    {
#pragma region base_class
    private:
        void _OnUpdate()
        {
            _volumeBarHeightTransition.Apply(_volumeBarHeight);
        }

        void _OnDraw(Graphics g)
        {
            // Create brushes
            if (!_selectedPartBrush)
            {
                // RGB(71, 161, 244) matches the color of DodgerBlue with 75% opacity over LightGray
                // This is done to match the volume slider ant time slider colors
                g.target->CreateSolidColorBrush(D2D1::ColorF(RGB(244, 161, 71) /* R and B flipped */), &_selectedPartBrush);
                g.refs->push_back((IUnknown**)&_selectedPartBrush);
            }
            if (!_remainingPartBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray), &_remainingPartBrush);
                g.refs->push_back((IUnknown**)&_remainingPartBrush);
            }
            if (!_textBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &_textBrush);
                g.refs->push_back((IUnknown**)&_textBrush);
            }

            g.target->Clear();

            // Draw the seek bar
            float progress = _value;
            int textWidth = ceilf(_maxTextWidth) + _margins * 2;
            int volumeSliderWidth = GetWidth() - textWidth - _margins;
            int selectedPartWidth = volumeSliderWidth * progress;
            g.target->FillRectangle(
                D2D1::RectF(
                    textWidth,
                    GetHeight() / 2.0f - _volumeBarHeight,
                    textWidth + selectedPartWidth,
                    GetHeight() / 2.0f + _volumeBarHeight
                ),
                _selectedPartBrush
            );
            g.target->FillRectangle(
                D2D1::RectF(
                    textWidth + selectedPartWidth,
                    GetHeight() / 2.0f - _volumeBarHeight,
                    GetWidth() - _margins,
                    GetHeight() / 2.0f + _volumeBarHeight
                ),
                _remainingPartBrush
            );
            if (/*GetMouseInside()*/_volumeHovered)
            {
                g.target->FillEllipse(
                    D2D1::Ellipse(
                        D2D1::Point2F(
                            textWidth + selectedPartWidth,
                            GetHeight() / 2.0f
                        ),
                        5.0f,
                        5.0f
                    ),
                    _selectedPartBrush
                );
            }

            // Create value string
            std::wstringstream valueStr;
            valueStr << (int)roundf(_value * 100.0f) << "%";

            // Draw value strings
            g.target->DrawText(
                valueStr.str().c_str(),
                valueStr.str().length(),
                _dwriteTextFormat,
                D2D1::RectF(
                    0.0f,
                    (GetHeight() - _textHeight) * 0.5f,
                    textWidth,
                    (GetHeight() + _textHeight) * 0.5f
                ),
                _textBrush
            );
        }

        void _OnResize(int width, int height)
        {

        }

        EventTargets _OnMouseMove(int x, int y)
        {
            int textWidth = ceilf(_maxTextWidth) + _margins * 2;
            int volumeSliderWidth = GetWidth() - textWidth - _margins;
            int xPos = GetMousePosX() - textWidth;
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
            }
            return EventTargets().Add(this, x, y);
        }

        void _OnMouseLeave()
        {
            if (_volumeHovered)
            {
                _volumeHovered = false;
                _volumeBarHeightTransition.Start(_volumeBarHeight, 1.0f);
            }
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            int textWidth = ceilf(_maxTextWidth) + _margins * 2;
            int volumeSliderWidth = GetWidth() - textWidth - _margins;
            int xPos = GetMousePosX() - textWidth;
            if (xPos >= 0 && xPos <= volumeSliderWidth)
            {
                float xPosNorm = xPos / (float)volumeSliderWidth;
                SetValue(xPosNorm);
                _held = true;
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
            }
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnWheelDown(int x, int y)
        {
            if (!_held)
            {
                SetValue(_value - 0.05f);
            }
            return EventTargets().Add(this, x, y);
        }

    public:
        const char* GetName() const { return "volume_slider"; }
#pragma endregion

    private:
        float _value = 0;
        float _exponent = 2.0f;

        bool _held = false;
        float _textHeight = 0.0f;
        float _maxTextWidth = 0.0f;

        float _volumeBarHeight = 1.0f; // This is height from center to each edge. Total height is 2 * _timeBarHeight
        Transition<float> _volumeBarHeightTransition = Transition<float>(Duration(100, MILLISECONDS));

        float _margins = 5.0f;

        bool _volumeHovered = false;

        ID2D1SolidColorBrush* _selectedPartBrush = nullptr;
        ID2D1SolidColorBrush* _remainingPartBrush = nullptr;
        ID2D1SolidColorBrush* _textBrush = nullptr;

        IDWriteFactory* _dwriteFactory = nullptr;
        IDWriteTextFormat* _dwriteTextFormat = nullptr;
        IDWriteTextLayout* _dwriteTextLayout = nullptr;

        TimePoint _mouseHoverStart = 0;

    public:
        VolumeSlider(float value)
        {
            _value = value;

            // Create text rendering resources
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&_dwriteFactory)
            );

            _dwriteFactory->CreateTextFormat(
                L"Calibri",
                NULL,
                DWRITE_FONT_WEIGHT_REGULAR,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                15.0f,
                L"en-us",
                &_dwriteTextFormat
            );
            _dwriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

            _dwriteFactory->CreateTextLayout(
                L"000:00:00",
                9,
                _dwriteTextFormat,
                1000,
                0,
                &_dwriteTextLayout
            );

            DWRITE_TEXT_METRICS metrics;
            _dwriteTextLayout->GetMetrics(&metrics);
            _textHeight = metrics.height;
            _maxTextWidth = metrics.width;
        }
        ~VolumeSlider()
        {
            // Release resources
            SafeFullRelease((IUnknown**)&_selectedPartBrush);
            SafeFullRelease((IUnknown**)&_remainingPartBrush);
            SafeFullRelease((IUnknown**)&_textBrush);
            SafeRelease((IUnknown**)&_dwriteTextFormat);
            SafeRelease((IUnknown**)&_dwriteTextLayout);
            SafeRelease((IUnknown**)&_dwriteFactory);
        }
        VolumeSlider(VolumeSlider&&) = delete;
        VolumeSlider& operator=(VolumeSlider&&) = delete;
        VolumeSlider(const VolumeSlider&) = delete;
        VolumeSlider& operator=(const VolumeSlider&) = delete;

        float GetValue() const
        {
            return _value;
        }

        void SetValue(float value)
        {
            if (value > 1.0f) value = 1.0f;
            else if (value < 0.0f) value = 0.0f;
            _value = value;
            Options::Instance()->SetValue("volume", float_to_str(_value));
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