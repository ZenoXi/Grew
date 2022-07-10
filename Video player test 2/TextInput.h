#pragma once

#include "App.h"
#include "ComponentBase.h"
#include "KeyboardEventHandler.h"

#include "GameTime.h"

namespace zcom
{
    class TextInput : public Base, public KeyboardEventHandler
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            // Update caret visibility
            if (!Selected())
            {
                if (_caretVisible)
                {
                    _caretVisible = false;
                    InvokeRedraw();
                }
            }
            else
            {
                _caretTimer.Update();
                if (_caretTimer.Now().GetTime(MILLISECONDS) % 1000 < 500)
                {
                    if (!_caretVisible)
                    {
                        _caretVisible = true;
                        InvokeRedraw();
                    }
                }
                else if (_caretVisible)
                {
                    _caretVisible = false;
                    InvokeRedraw();
                }
            }
        }

        void _OnDraw(Graphics g)
        {
            // Create resources
            if (!_textBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &_textBrush);
                g.refs->push_back((IUnknown**)&_textBrush);
            }
            if (!_placeholderTextBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.3f, 0.3f), &_placeholderTextBrush);
                g.refs->push_back((IUnknown**)&_placeholderTextBrush);
            }
            if (!_caretBrush)
            {
                //g.target->CreateSolidColorBrush(D2D1::ColorF(0.65f, 0.65f, 0.65f), &_caretBrush);
                g.target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &_caretBrush);
                g.refs->push_back((IUnknown**)&_caretBrush);
            }

            DWRITE_TEXT_METRICS textMetrics;
            _dwriteTextLayout->GetMetrics(&textMetrics);

            // Calculate text position
            D2D1_POINT_2F pos;
            if (_multiline)
            {
                pos = D2D1::Point2F(5.0f, 5.0f);
            }
            else
            {
                pos = D2D1::Point2F(5.0f, (GetHeight() - textMetrics.height) * 0.5f);
            }

            // Draw placeholder text
            if (!Selected() && _text.empty())
            {
                g.target->DrawTextLayout(
                    pos,
                    _dwritePlaceholderTextLayout,
                    _placeholderTextBrush
                );
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

            // Draw caret
            if (_caretVisible)
            {
                FLOAT posX, posY;
                DWRITE_HIT_TEST_METRICS hitMetrics;
                _dwriteTextLayout->HitTestTextPosition(
                    _cursorPos,
                    false,
                    &posX, &posY,
                    &hitMetrics
                );

                g.target->DrawLine(
                    D2D1::Point2F(pos.x + posX, pos.y + posY),
                    D2D1::Point2F(pos.x + posX, pos.y + posY + textMetrics.height),
                    _caretBrush
                );
            }
        }

        void _OnSelected(); // Uses 'App'

        void _OnDeselected(); // Uses 'App'

        bool _OnKeyDown(BYTE vkCode)
        {
            switch (vkCode)
            {
            case VK_LEFT:
            {
                if (_cursorPos > 0)
                {
                    _cursorPos--;
                    _caretTimer.Reset();
                    InvokeRedraw();
                }
                break;
            }
            case VK_RIGHT:
            {
                if (_cursorPos < _text.length())
                {
                    _cursorPos++;
                    _caretTimer.Reset();
                    InvokeRedraw();
                }
                break;
            }
            case VK_HOME:
            {
                _cursorPos = 0;
                _caretTimer.Reset();
                InvokeRedraw();
                break;
            }
            case VK_END:
            {
                _cursorPos = _text.length();
                _caretTimer.Reset();
                InvokeRedraw();
                break;
            }
            case VK_TAB:
            {
                if (_tabAllowed)
                {
                    break;
                }
                else
                {
                    return false;
                }
            }
            default:
                break;
            }

            return true;
        }

        bool _OnKeyUp(BYTE vkCode)
        {
            return true;
        }

        bool _OnChar(wchar_t ch)
        {
            bool handled = false;
            if (ch == L'\b')
            {
                if (_cursorPos > 0)
                {
                    _text.erase(_text.begin() + _cursorPos - 1);
                    _cursorPos--;
                    _caretTimer.Reset();
                    _CreateTextLayout();
                }
                handled = true;
            }
            else if (ch == L'\n' || ch == L'\r')
            {
                if (!_multiline)
                {
                    handled = true;
                }
            }
            else if (ch == L'\t')
            {
                if (!_tabAllowed)
                {
                    handled = true;
                }
            }
            else if (ch == L'\x16')
            {
                // Paste
                std::wstring text;
                if (OpenClipboard(nullptr))
                {
                    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                    if (hData != nullptr)
                    {
                        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                        if (pszText != nullptr)
                        {
                            text = pszText;
                        }
                        GlobalUnlock(hData);
                    }
                    CloseClipboard();
                }

                if (!text.empty())
                {
                    _text.insert(_text.begin() + _cursorPos, text.begin(), text.end());
                    _cursorPos += text.length();
                    _caretTimer.Reset();
                    _CreateTextLayout();
                }

                handled = true;
            }

            if (!handled)
            {
                _text.insert(_text.begin() + _cursorPos, ch);
                _cursorPos++;
                _caretTimer.Reset();
                _CreateTextLayout();
            }

            return true;
        }

        void _CreateTextLayout()
        {
            if (_dwriteTextLayout)
            {
                _dwriteTextLayout->Release();
            }

            float maxWidth, maxHeight;
            if (_multiline)
            {
                maxWidth = GetWidth() - 10.0f;
                maxHeight = GetHeight() - 10.0f;
            }
            else
            {
                maxWidth = (unsigned int)-1;
                maxHeight = 0;
            }

            _dwriteFactory->CreateTextLayout(
                _text.c_str(),
                _text.length(),
                _dwriteTextFormat,
                maxWidth,
                maxHeight,
                &_dwriteTextLayout
            );
            if (!_multiline)
            {
                _dwriteTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            }

            InvokeRedraw();
        }

        void _CreatePlaceholderTextLayout()
        {
            if (_dwritePlaceholderTextLayout)
            {
                _dwritePlaceholderTextLayout->Release();
            }

            float maxWidth, maxHeight;
            if (_multiline)
            {
                maxWidth = GetWidth() - 10.0f;
                maxHeight = GetHeight() - 10.0f;
            }
            else
            {
                maxWidth = (unsigned int)-1;
                maxHeight = 0;
            }

            _dwriteFactory->CreateTextLayout(
                _placeholderText.c_str(),
                _placeholderText.length(),
                _dwriteTextFormat,
                maxWidth,
                maxHeight,
                &_dwritePlaceholderTextLayout
            );
            if (!_multiline)
            {
                _dwritePlaceholderTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            }

            InvokeRedraw();
        }

    public:
        const char* GetName() const { return "text_input"; }
#pragma endregion

    private:
        std::wstring _text;
        std::wstring _placeholderText;
        bool _multiline = false;
        bool _tabAllowed = false;

        unsigned int _cursorPos = 0;
        unsigned int _selectionStart = 0;
        unsigned int _selectionLength = 0;
        bool _caretVisible = false;
        Clock _caretTimer = Clock(0);

        ID2D1SolidColorBrush* _textBrush = nullptr;
        ID2D1SolidColorBrush* _placeholderTextBrush = nullptr;
        ID2D1SolidColorBrush* _caretBrush = nullptr;

        IDWriteFactory* _dwriteFactory = nullptr;
        IDWriteTextFormat* _dwriteTextFormat = nullptr;
        IDWriteTextLayout* _dwriteTextLayout = nullptr;
        IDWriteTextLayout* _dwritePlaceholderTextLayout = nullptr;

    protected:
        friend class Scene;
        friend class Base;
        TextInput(Scene* scene) : Base(scene) {}
        void Init()
        {
            SetSelectable(true);
            SetBorderVisibility(true);
            SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
            SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));

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

            _CreateTextLayout();
            _CreatePlaceholderTextLayout();
        }
    public:
        ~TextInput()
        {
            SafeFullRelease((IUnknown**)&_textBrush);
            SafeFullRelease((IUnknown**)&_placeholderTextBrush);
            SafeFullRelease((IUnknown**)&_caretBrush);
            SafeRelease((IUnknown**)&_dwriteTextFormat);
            SafeRelease((IUnknown**)&_dwriteTextLayout);
            SafeRelease((IUnknown**)&_dwritePlaceholderTextLayout);
            SafeRelease((IUnknown**)&_dwriteFactory);
        }
        TextInput(TextInput&&) = delete;
        TextInput& operator=(TextInput&&) = delete;
        TextInput(const TextInput&) = delete;
        TextInput& operator=(const TextInput&) = delete;

        std::wstring GetText() const
        {
            return _text;
        }

        std::wstring GetPlaceholderText() const
        {
            return _placeholderText;
        }

        bool GetMultiline() const
        {
            return _multiline;
        }

        bool GetTabAllowed() const
        {
            return _tabAllowed;
        }

        void SetPlaceholderText(std::wstring text)
        {
            _placeholderText = text;
            _CreatePlaceholderTextLayout();
        }

        void SetText(std::wstring text)
        {
            _text = text;
            _cursorPos = _text.length();
            _CreateTextLayout();
        }

        void SetMultiline(bool multiline)
        {
            if (_multiline != multiline)
            {
                _multiline = multiline;
                _CreateTextLayout();
                _CreatePlaceholderTextLayout();
            }
        }

        void SetTabAllowed(bool allowed)
        {
            _tabAllowed = allowed;
        }
    };
}