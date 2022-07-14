#pragma once

#include "ComponentBase.h"
#include "ComHelper.h"

#include <string_view>

namespace zcom
{
    enum class TextAlignment
    {
        LEADING,
        CENTER,
        JUSTIFIED,
        TRAILING
    };

    class Label : public Base
    {
#pragma region base_class
    protected:
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
                pos.x = 0;
            }
            else if (_hTextAlignment == TextAlignment::JUSTIFIED)
            {
                pos.x = 0;
            }
            else if (_hTextAlignment == TextAlignment::TRAILING)
            {
                pos.x = 0;
            }
            //
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

            pos.x += _margins.left;
            pos.y += _margins.top;

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
            _CreateTextLayout();
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

            InvokeRedraw();
        }

        void _CreateTextLayout()
        {
            float finalWidth = GetWidth() - _margins.left - _margins.right;
            float finalHeight = GetHeight() - _margins.top - _margins.bottom;
            if (finalWidth <= 0) finalWidth = 1.f;
            if (finalHeight <= 0) finalHeight = 1.f;

            std::wstring finalText = _text;
            size_t charactersCut = 0;

            while (true)
            {
                if (_dwriteTextLayout)
                {
                    _dwriteTextLayout->Release();
                }

                // Create the text layout
                _dwriteFactory->CreateTextLayout(
                    finalText.c_str(),
                    finalText.length(),
                    _dwriteTextFormat,
                    finalWidth,
                    finalHeight,
                    &_dwriteTextLayout
                );

                // Wrapping
                if (!_wrapText)
                {
                    _dwriteTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                }

                // If a cutoff is specified, truncate the text until it fits (including the cutoff sequence)
                if (!_cutoff.empty())
                {
                    // OPTIMIZATION: Use binary search to speed up truncation of long strings

                    DWRITE_TEXT_METRICS textMetrics;
                    _dwriteTextLayout->GetMetrics(&textMetrics);
                    if (textMetrics.width > textMetrics.layoutWidth ||
                        (textMetrics.height > textMetrics.layoutHeight && textMetrics.lineCount > 1))
                    {
                        // Stop if the entire string is cut
                        if (charactersCut == _text.length())
                            break;

                        charactersCut++;
                        finalText = _text.substr(0, _text.length() - charactersCut) + _cutoff;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

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

            InvokeRedraw();
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "label"; }
#pragma endregion

    private:
        std::wstring _text;
        TextAlignment _hTextAlignment = TextAlignment::LEADING;
        Alignment _vTextAlignment = Alignment::START;
        bool _wrapText = false;
        std::wstring _cutoff = L"";
        RECT_F _margins = { 0, 0, 0, 0 };

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

    protected:
        friend class Scene;
        friend class Base;
        Label(Scene* scene) : Base(scene) {}
        void Init(std::wstring text = L"")
        {
            _text = text;

            // Create text rendering resources
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&_dwriteFactory)
            );
            _CreateTextFormat();
            _CreateTextLayout();
        }
    public:
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

        std::wstring GetCutoff() const
        {
            return _cutoff;
        }

        RECT_F GetMargins() const
        {
            return _margins;
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

        float GetTextWidth() const
        {
            DWRITE_TEXT_METRICS textMetrics;
            _dwriteTextLayout->GetMetrics(&textMetrics);
            return textMetrics.width + _margins.left + _margins.right;
        }

        float GetTextHeight() const
        {
            DWRITE_TEXT_METRICS textMetrics;
            _dwriteTextLayout->GetMetrics(&textMetrics);
            return textMetrics.height + _margins.top + _margins.bottom;
        }

        void SetText(std::wstring text)
        {
            if (text == _text)
                return;

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
            InvokeRedraw();
        }

        void SetWordWrap(bool wrap)
        {
            if (_wrapText != wrap)
            {
                _wrapText = wrap;
                _CreateTextLayout();
            }
        }

        // If set to a non-empty string, the text will be truncated to fit within the boundaries.
        // 'cutoff' - The string appended to the end of truncated text (e.g. "trunca..." if 'cutoff' is "...").
        void SetCutoff(std::wstring cutoff)
        {
            if (_cutoff != cutoff)
            {
                _cutoff = cutoff;
                _CreateTextLayout();
            }
        }

        void SetMargins(RECT_F margins)
        {
            if (_margins != margins)
            {
                _margins = margins;
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
            SafeFullRelease((IUnknown**)&_textBrush);
            InvokeRedraw();
        }
    };
}