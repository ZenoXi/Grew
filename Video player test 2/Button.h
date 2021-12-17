#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "Event.h"

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

    class Button : public Base
    {
#pragma region base_class
    private:
        void _OnUpdate()
        {

        }

        void _OnDraw(Graphics g)
        {
            D2D1_COLOR_F color;
            ID2D1Bitmap* image;
            if (GetMouseLeftClicked())
            {
                color = _colorClicked;
                image = _imageClicked;
            }
            else if (GetMouseInside())
            {
                color = _colorHovered;
                image = _imageHovered;
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
            _text->SetSize(width, height);
            _text->Resize();
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
            if (_activation == ButtonActivation::PRESS || _activation == ButtonActivation::PRESS_AND_RELEASE)
            {
                _activated = true;
                _OnActivated.InvokeAll();
            }
            return this;
        }

        Base* _OnLeftReleased(int x, int y)
        {
            if (_activation == ButtonActivation::RELEASE || _activation == ButtonActivation::PRESS_AND_RELEASE)
            {
                _activated = true;
                _OnActivated.InvokeAll();
            }
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

        void SetButtonImage(ID2D1Bitmap* image)
        {
            _image = image;
        }

        void SetButtonColor(D2D1_COLOR_F color)
        {
            _color = color;
        }

        void SetButtonHoverImage(ID2D1Bitmap* image)
        {
            _imageHovered = image;
        }

        void SetButtonHoverColor(D2D1_COLOR_F color)
        {
            _colorHovered = color;
        }

        void SetButtonClickImage(ID2D1Bitmap* image)
        {
            _imageClicked = image;
        }

        void SetButtonClickColor(D2D1_COLOR_F color)
        {
            _colorClicked = color;
        }

        void SetPreset(ButtonPreset preset)
        {
            switch (preset)
            {
            case ButtonPreset::NO_EFFECTS:
            {
                _image = nullptr;
                _color = D2D1::ColorF(0, 0.0f);
                _imageHovered = nullptr;
                _colorHovered = D2D1::ColorF(0, 0.0f);
                _imageClicked = nullptr;
                _colorClicked = D2D1::ColorF(0, 0.0f);
                break;
            }
            case ButtonPreset::DEFAULT:
            {
                _image = nullptr;
                _color = D2D1::ColorF(0, 0.0f);
                _imageHovered = nullptr;
                _colorHovered = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
                _imageClicked = nullptr;
                _colorClicked = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f);
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