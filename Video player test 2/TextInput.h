#pragma once

#include "ComponentBase.h"

namespace zcom
{
    class TextInput : public Base, public KeyboardEventHandler
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
            if (ztime::Main().GetTime(MILLISECONDS) % 1000 < 500 && Selected())
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

        void _OnResize(int width, int height)
        {

        }

        Base* _OnMouseMove(int x, int y)
        {
            return this;
        }

        void _OnMouseLeave()
        {

        }

        void _OnMouseEnter()
        {

        }

        Base* _OnLeftPressed(int x, int y)
        {
            return this;
        }

        Base* _OnLeftReleased(int x, int y)
        {
            return this;
        }

        Base* _OnRightPressed(int x, int y)
        {
            return this;
        }

        Base* _OnRightReleased(int x, int y)
        {
            return this;
        }

        Base* _OnWheelUp(int x, int y)
        {
            return this;
        }

        Base* _OnWheelDown(int x, int y)
        {
            return this;
        }

        void _OnSelected()
        {
            App::Instance()->keyboardManager.SetExclusiveHandler(this);
            SetBorderColor(D2D1::ColorF(0.0f, 0.5f, 0.8f));

            GetKeyboardState(_keyStates);
        }

        void _OnDeselected()
        {
            App::Instance()->keyboardManager.ResetExclusiveHandler();
            SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        }

        bool _OnKeyDown(BYTE vkCode)
        {
            switch (vkCode)
            {
            case VK_LEFT:
            {
                if (_cursorPos > 0)
                {
                    _cursorPos--;
                }
                break;
            }
            case VK_RIGHT:
            {
                if (_cursorPos < _text.length())
                {
                    _cursorPos++;
                }
                break;
            }
            case VK_HOME:
            {
                _cursorPos = 0;
                break;
            }
            case VK_END:
            {
                _cursorPos = _text.length();
                break;
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

            if (!handled)
            {
                _text.insert(_text.begin() + _cursorPos, ch);
                _cursorPos++;
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

        const char* GetName() const { return "text_input"; }
#pragma endregion

    private:
        std::wstring _text;
        std::wstring _placeholderText;
        bool _multiline = false;

        unsigned int _cursorPos = 0;
        unsigned int _selectionStart = 0;
        unsigned int _selectionLength = 0;

        ID2D1SolidColorBrush* _textBrush = nullptr;
        ID2D1SolidColorBrush* _placeholderTextBrush = nullptr;
        ID2D1SolidColorBrush* _caretBrush = nullptr;

        IDWriteFactory* _dwriteFactory = nullptr;
        IDWriteTextFormat* _dwriteTextFormat = nullptr;
        IDWriteTextLayout* _dwriteTextLayout = nullptr;
        IDWriteTextLayout* _dwritePlaceholderTextLayout = nullptr;

    public:
        TextInput()
        {
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
            //_dwriteFactory->CreateTextLayout(
            //    L"000:00:00",
            //    9,
            //    _dwriteTextFormat,
            //    1000,
            //    0,
            //    &_dwriteTextLayout
            //);

            //DWRITE_TEXT_METRICS metrics;
            //_dwriteTextLayout->GetMetrics(&metrics);
            //_textHeight = metrics.height;
            //_maxTimeWidth = metrics.width;
        }
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

        void SetPlaceholderText(std::wstring text)
        {
            _placeholderText = text;
            _CreatePlaceholderTextLayout();
        }

        void SetText(std::wstring text)
        {
            _text = text;
            if (_cursorPos > _text.length())
            {
                _cursorPos = _text.length();
            }
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
    };
}