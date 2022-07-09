#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "Event.h"
#include "KeyboardEventHandler.h"

namespace zcom
{
    enum class ButtonPreset
    {
        NO_EFFECTS,
        DEFAULT
    };

    enum class ButtonActivation
    {
        PRESS,
        RELEASE,
        PRESS_AND_RELEASE
    };

    class Button : public Base, public KeyboardEventHandler
    {
#pragma region base_class
    protected:
        bool _Redraw()
        {
            return _text->Redraw();
        }

        void _OnDraw(Graphics g)
        {
            D2D1_COLOR_F color;
            ID2D1Bitmap* image;
            if (GetMouseInsideArea())
            {
                if (GetMouseLeftClicked())
                {
                    color = _colorClicked;
                    image = _imageClicked;
                }
                else
                {
                    color = _colorHovered;
                    image = _imageHovered;
                }
            }
            else
            {
                color = _color;
                image = _image;
            }
            
            ID2D1SolidColorBrush* brush;
            g.target->CreateSolidColorBrush(color, &brush);
            g.target->FillRectangle
            (
                D2D1::RectF(0, 0, g.target->GetSize().width, g.target->GetSize().height),
                brush
            );
            brush->Release();
            if (image)
            {
                g.target->DrawBitmap
                (
                    image,
                    D2D1::RectF(0, 0, g.target->GetSize().width, g.target->GetSize().height)
                );
            }

            g.target->DrawBitmap(
                _text->Draw(g),
                D2D1::RectF(
                    _text->GetX(),
                    _text->GetY(),
                    _text->GetX() + _text->GetWidth(),
                    _text->GetY() + _text->GetHeight()
                )
            );
        }

        void _OnResize(int width, int height)
        {
            _text->Resize(width, height);
        }

        void _OnMouseEnterArea()
        {
            InvokeRedraw();
        }

        void _OnMouseLeaveArea()
        {
            InvokeRedraw();
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            if (_activation == ButtonActivation::PRESS || _activation == ButtonActivation::PRESS_AND_RELEASE)
            {
                _activated = true;
                _OnActivated.InvokeAll();
            }
            InvokeRedraw();
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnLeftReleased(int x, int y)
        {
            if (GetMouseInsideArea())
            {
                if (_activation == ButtonActivation::RELEASE || _activation == ButtonActivation::PRESS_AND_RELEASE)
                {
                    _activated = true;
                    _OnActivated.InvokeAll();
                }
            }
            InvokeRedraw();
            return EventTargets().Add(this, x, y);
        }

        void _OnSelected();

        void _OnDeselected();

        bool _OnKeyDown(BYTE vkCode)
        {
            if (vkCode == VK_RETURN)
            {
                _OnActivated.InvokeAll();
                return true;
            }
            return false;
        }

        bool _OnKeyUp(BYTE vkCode)
        {
            return false;
        }

        bool _OnChar(wchar_t ch)
        {
            return false;
        }

    public:
        const char* GetName() const { return "button"; }
#pragma endregion

    private:
        bool _activated = false;
        ButtonActivation _activation = ButtonActivation::PRESS;
        Event<void> _OnActivated;

        bool _hovered = false;

        Label* _text = nullptr;

        ID2D1Bitmap* _image = nullptr;
        D2D1_COLOR_F _color = D2D1::ColorF(0, 0.0f);
        ID2D1Bitmap* _imageHovered = nullptr;
        D2D1_COLOR_F _colorHovered = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
        ID2D1Bitmap* _imageClicked = nullptr;
        D2D1_COLOR_F _colorClicked = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f);

    public:
        Button(std::wstring text = L"") : _text(new Label(text))
        {
            SetSelectable(true);
            _text->SetSize(GetWidth(), GetHeight());
            _text->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _text->SetVerticalTextAlignment(Alignment::CENTER);
        }
        ~Button()
        {
            if (_text)
            {
                delete _text;
                _text = nullptr;
            }
        }
        Button(Button&&) = delete;
        Button& operator=(Button&&) = delete;
        Button(const Button&) = delete;
        Button& operator=(const Button&) = delete;

        void SetButtonImageAll(ID2D1Bitmap* image)
        {
            SetButtonImage(image);
            SetButtonHoverImage(image);
            SetButtonClickImage(image);
        }

        void SetButtonColorAll(D2D1_COLOR_F color)
        {
            SetButtonColor(color);
            SetButtonHoverColor(color);
            SetButtonClickColor(color);
        }

        void SetButtonImage(ID2D1Bitmap* image)
        {
            if (image == _image)
                return;
            _image = image;
            InvokeRedraw();
        }

        void SetButtonColor(D2D1_COLOR_F color)
        {
            if (color == _color)
                return;
            _color = color;
            InvokeRedraw();
        }

        void SetButtonHoverImage(ID2D1Bitmap* image)
        {
            if (image == _imageHovered)
                return;
            _imageHovered = image;
            InvokeRedraw();
        }

        void SetButtonHoverColor(D2D1_COLOR_F color)
        {
            if (color == _colorHovered)
                return;
            _colorHovered = color;
            InvokeRedraw();
        }

        void SetButtonClickImage(ID2D1Bitmap* image)
        {
            if (image == _imageClicked)
                return;
            _imageClicked = image;
            InvokeRedraw();
        }

        void SetButtonClickColor(D2D1_COLOR_F color)
        {
            if (color == _colorClicked)
                return;
            _colorClicked = color;
            InvokeRedraw();
        }

        void SetPreset(ButtonPreset preset)
        {
            switch (preset)
            {
            case ButtonPreset::NO_EFFECTS:
            {
                SetButtonImageAll(nullptr);
                SetButtonColorAll(D2D1::ColorF(0, 0.0f));
                break;
            }
            case ButtonPreset::DEFAULT:
            {
                SetButtonImageAll(nullptr);
                SetButtonColor(D2D1::ColorF(0, 0.0f));
                SetButtonHoverColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f));
                SetButtonClickColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f));
                break;
            }
            default:
                break;
            }
        }

        void SetOnActivated(const std::function<void()>& func)
        {
            _OnActivated.Add(func);
        }

        void SetActivation(ButtonActivation activation)
        {
            _activation = activation;
        }

        bool Activated()
        {
            bool value = _activated;
            _activated = false;
            return value;
        }

        Label* Text()
        {
            return _text;
        }
    };
}