#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "ComponentBase.h"
#include "GameTime.h"

#include <sstream>

class VolumeSlider : public ComponentBase
{
#pragma region base_class
private:
    void _OnUpdate()
    {
        
    }

    void _OnDraw(Graphics g)
    {
        // Create brushes
        if (!_selectedPartBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue), &_selectedPartBrush);
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
        float progress = _volume;
        int textWidth = ceilf(_maxTextWidth) + _margins * 2;
        int volumeSliderWidth = GetWidth() - textWidth - _margins;
        int selectedPartWidth = volumeSliderWidth * progress;
        g.target->FillRectangle(
            D2D1::RectF(
                textWidth,
                GetHeight() / 2.0f - 1.0f,
                textWidth + selectedPartWidth,
                GetHeight() / 2.0f + 1.0f
            ),
            _selectedPartBrush
        );
        g.target->FillRectangle(
            D2D1::RectF(
                textWidth + selectedPartWidth,
                GetHeight() / 2.0f - 1.0f,
                GetWidth() - _margins,
                GetHeight() / 2.0f + 1.0f),
            _remainingPartBrush
        );
        if (GetMouseInside())
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
        valueStr << (int)(_volume * 100.0f) << "%";

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

    ComponentBase* _OnMouseMove(int x, int y)
    {
        if (_held)
        {
            int textWidth = ceilf(_maxTextWidth) + _margins * 2;
            int volumeSliderWidth = GetWidth() - textWidth - _margins;
            int xPos = GetMousePosX() - textWidth;
            if (xPos < 0) xPos = 0;
            if (xPos > volumeSliderWidth) xPos = volumeSliderWidth;
            float xPosNorm = xPos / (float)volumeSliderWidth;
            _volume = xPosNorm;
        }
        return this;
    }

    void _OnMouseLeave()
    {
        
    }

    void _OnMouseEnter()
    {
        _mouseHoverStart = ztime::Main();
    }

    ComponentBase* _OnLeftPressed(int x, int y)
    {
        int textWidth = ceilf(_maxTextWidth) + _margins * 2;
        int volumeSliderWidth = GetWidth() - textWidth - _margins;
        int xPos = GetMousePosX() - textWidth;
        if (xPos >= 0 && xPos <= volumeSliderWidth)
        {
            float xPosNorm = xPos / (float)volumeSliderWidth;
            _volume = xPosNorm;
            _held = true;
        }
        return this;
    }

    ComponentBase* _OnLeftReleased(int x, int y)
    {
        _held = false;
        return this;
    }

    ComponentBase* _OnRightPressed(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnRightReleased(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnWheelUp(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnWheelDown(int x, int y)
    {
        return this;
    }

public:
    std::list<ComponentBase*> GetChildren()
    {
        return std::list<ComponentBase*>();
    }

    std::list<ComponentBase*> GetAllChildren()
    {
        return std::list<ComponentBase*>();
    }

    const char* GetName() const { return "seek_bar"; }
#pragma endregion

private:
    float _volume = 0;

    bool _held = false;
    float _textHeight = 0.0f;
    float _maxTextWidth = 0.0f;

    float _margins = 5.0f;

    ID2D1SolidColorBrush* _selectedPartBrush = nullptr;
    ID2D1SolidColorBrush* _remainingPartBrush = nullptr;
    ID2D1SolidColorBrush* _textBrush = nullptr;

    IDWriteFactory* _dwriteFactory = nullptr;
    IDWriteTextFormat* _dwriteTextFormat = nullptr;
    IDWriteTextLayout* _dwriteTextLayout = nullptr;

    TimePoint _mouseHoverStart = 0;

public:
    VolumeSlider(float volume)
    {
        _volume = volume;

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
            14.0f,
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

    float GetVolume() const
    {
        return _volume;
    }

    void SetVolume(float volume)
    {
        if (volume > 1.0f) volume = 1.0f;
        else if (volume < 0.0f) volume = 0.0f;
        _volume = volume;
    }
};