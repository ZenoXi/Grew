#pragma once

#include "ComponentBase.h"

enum class TextAlignment
{
    LEADING,
    CENTER,
    JUSTIFIED,
    TRAILING
};

class Label : public ComponentBase
{
#pragma region base_class
private:
    void _OnUpdate()
    {

    }

    void _OnDraw(Graphics g)
    {
        // Create resources
        if (!_textBrush)
        {
            g.target->CreateSolidColorBrush(_fontColor, &_textBrush);
            g.refs->push_back((IUnknown**)&_textBrush);
        }

        DWRITE_TEXT_METRICS textMetrics;
        _dwriteTextLayout->GetMetrics(&textMetrics);

        D2D1_POINT_2F pos;
        if (_hTextAlignment == TextAlignment::LEADING)
        {
            pos.x = 0;
        }
        else if (_hTextAlignment == TextAlignment::CENTER)
        {
            pos.x = GetWidth() * 0.5f;
        }
        else if (_hTextAlignment == TextAlignment::JUSTIFIED)
        {
            pos.x = (GetWidth() - textMetrics.width) * 0.5f;
        }
        else if (_hTextAlignment == TextAlignment::TRAILING)
        {
            pos.x = GetWidth();
        }
        if (_vTextAlignment == Alignment::START)
        {
            pos.y = 0;
        }
        else if (_vTextAlignment == Alignment::CENTER)
        {
            pos.y = (GetHeight() - textMetrics.height) * 0.5f;
        }
        else if (_vTextAlignment == Alignment::END)
        {
            pos.y = GetHeight() - textMetrics.height;
        }

        // Draw text
        if (!_text.empty())
        {
            g.target->DrawTextLayout(
                pos,
                _dwriteTextLayout,
                _textBrush
            );
        }
    }

    void _OnResize(int width, int height)
    {

    }

    ComponentBase* _OnMouseMove(int x, int y)
    {
        return this;
    }

    void _OnMouseLeave()
    {

    }

    void _OnMouseEnter()
    {

    }

    ComponentBase* _OnLeftPressed(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnLeftReleased(int x, int y)
    {
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

    void _OnSelected()
    {
        
    }

    void _OnDeselected()
    {
        
    }

    void _CreateTextFormat()
    {
        if (_dwriteTextFormat)
        {
            _dwriteTextFormat->Release();
        }

        _dwriteFactory->CreateTextFormat(
            _font.c_str(),
            NULL,
            _fontWeight,
            _fontStyle,
            _fontStretch,
            _fontSize,
            L"en-us",
            &_dwriteTextFormat
        );
    }
    
    void _CreateTextLayout()
    {
        if (_dwriteTextLayout)
        {
            _dwriteTextLayout->Release();
        }

        _dwriteFactory->CreateTextLayout(
            _text.c_str(),
            _text.length(),
            _dwriteTextFormat,
            0,
            0,
            &_dwriteTextLayout
        );

        // Alignment
        if (_hTextAlignment == TextAlignment::LEADING)
        {
            _dwriteTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        }
        else if (_hTextAlignment == TextAlignment::CENTER)
        {
            _dwriteTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else if (_hTextAlignment == TextAlignment::JUSTIFIED)
        {
            _dwriteTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
        }
        else if (_hTextAlignment == TextAlignment::TRAILING)
        {
            _dwriteTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        }

        // Wrapping
        if (!_wrapText)
        {
            _dwriteTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
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

    const char* GetName() const { return "text_input"; }
#pragma endregion

private:
    std::wstring _text;
    TextAlignment _hTextAlignment = TextAlignment::LEADING;
    Alignment _vTextAlignment = Alignment::START;
    bool _wrapText = false;

    std::wstring _font = L"Calibri";
    float _fontSize = 14.0f;
    DWRITE_FONT_WEIGHT _fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    DWRITE_FONT_STYLE _fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH _fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    D2D1_COLOR_F _fontColor = D2D1::ColorF(0.8f, 0.8f, 0.8f);

    ID2D1SolidColorBrush* _textBrush = nullptr;

    IDWriteFactory* _dwriteFactory = nullptr;
    IDWriteTextFormat* _dwriteTextFormat = nullptr;
    IDWriteTextLayout* _dwriteTextLayout = nullptr;

public:
    Label(std::wstring text = L"") : _text(text)
    {
        // Create text rendering resources
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&_dwriteFactory)
        );
        _CreateTextFormat();
        _CreateTextLayout();
    }
    ~Label()
    {
        SafeFullRelease((IUnknown**)&_textBrush);
        SafeFullRelease((IUnknown**)&_dwriteTextFormat);
        SafeFullRelease((IUnknown**)&_dwriteTextLayout);
        SafeFullRelease((IUnknown**)&_dwriteFactory);
    }
    Label(Label&&) = delete;
    Label& operator=(Label&&) = delete;
    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

    std::wstring GetText() const
    {
        return _text;
    }

    TextAlignment GetHorizontalTextAlignment() const
    {
        return _hTextAlignment;
    }

    Alignment GetVerticalTextAlignment() const
    {
        return _vTextAlignment;
    }

    bool GetWordWrap() const
    {
        return _wrapText;
    }

    std::wstring GetFont() const
    {
        return _font;
    }

    float GetFontSize() const
    {
        return _fontSize;
    }

    DWRITE_FONT_WEIGHT GetFontWeight() const
    {
        return _fontWeight;
    }

    DWRITE_FONT_STYLE GetFontStyle() const
    {
        return _fontStyle;
    }

    DWRITE_FONT_STRETCH GetFontStretch() const
    {
        return _fontStretch;
    }

    D2D1_COLOR_F GetFontColor() const
    {
        return _fontColor;
    }

    void SetText(std::wstring text)
    {
        _text = text;
        _CreateTextLayout();
    }

    void SetHorizontalTextAlignment(TextAlignment alignment)
    {
        if (_hTextAlignment != alignment)
        {
            _hTextAlignment = alignment;
            _CreateTextLayout();
        }
    }

    void SetVerticalTextAlignment(Alignment alignment)
    {
        _vTextAlignment = alignment;
    }

    void SetWordWrap(bool wrap)
    {
        if (_wrapText != wrap)
        {
            _wrapText = wrap;
            _CreateTextLayout();
        }
    }

    void SetFont(std::wstring font)
    {
        if (_font != font)
        {
            _font = font;
            _CreateTextFormat();
            _CreateTextLayout();
        }
    }

    void SetFontSize(float size)
    {
        if (_fontSize != size)
        {
            _fontSize = size;
            _CreateTextFormat();
            _CreateTextLayout();
        }
    }

    void SetFontWeight(DWRITE_FONT_WEIGHT weight)
    {
        if (_fontWeight != weight)
        {
            _fontWeight = weight;
            _CreateTextFormat();
            _CreateTextLayout();
        }
    }

    void SetFontStyle(DWRITE_FONT_STYLE style)
    {
        if (_fontStyle != style)
        {
            _fontStyle = style;
            _CreateTextFormat();
            _CreateTextLayout();
        }
    }

    void SetFontStretch(DWRITE_FONT_STRETCH stretch)
    {
        if (_fontStretch != stretch)
        {
            _fontStretch = stretch;
            _CreateTextFormat();
            _CreateTextLayout();
        }
    }

    void SetFontColor(D2D1_COLOR_F color)
    {
        _fontColor = color;
        if (_textBrush)
        {
            _textBrush->Release();
        }
    }
};
