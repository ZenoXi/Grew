#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "Event.h"

enum class ButtonActivation
{
    PRESS,
    RELEASE,
    PRESS_AND_RELEASE
};

class Button : public ComponentBase
{
#pragma region base_class
private:
    void _OnUpdate()
    {

    }

    void _OnDraw(Graphics g)
    {
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
    }

    ComponentBase* _OnMouseMove(int x, int y)
    {
        return this;
    }

    void _OnMouseLeave()
    {
        _hovered = false;
        SetBackgroundImage(_backgroundImageBackup);
        SetBackgroundColor(_backgroundColorBackup);
    }

    void _OnMouseEnter()
    {
        _hovered = true;
        _backgroundImageBackup = GetBackgroundImage();
        _backgroundColorBackup = GetBackgroundColor();
        SetBackgroundImage(_backgroundImageHovered);
        SetBackgroundColor(_backgroundColorHovered);
    }

    ComponentBase* _OnLeftPressed(int x, int y)
    {
        if (_activation == ButtonActivation::PRESS || _activation == ButtonActivation::PRESS_AND_RELEASE)
        {
            _activated = true;
            _OnActivated.InvokeAll();
        }

        if (!_hovered)
        {
            _backgroundImageBackup = GetBackgroundImage();
            _backgroundColorBackup = GetBackgroundColor();
        }
        SetBackgroundImage(_backgroundImageClicked);
        SetBackgroundColor(_backgroundColorClicked);
        return this;
    }

    ComponentBase* _OnLeftReleased(int x, int y)
    {
        if (_activation == ButtonActivation::RELEASE || _activation == ButtonActivation::PRESS_AND_RELEASE)
        {
            _activated = true;
            _OnActivated.InvokeAll();
        }

        if (_hovered)
        {
            SetBackgroundImage(_backgroundImageHovered);
            SetBackgroundColor(_backgroundColorHovered);
        }
        else
        {
            SetBackgroundImage(_backgroundImageBackup);
            SetBackgroundColor(_backgroundColorBackup);
        }
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

    const char* GetName() const { return "button"; }
#pragma endregion

private:
    bool _activated = false;
    ButtonActivation _activation = ButtonActivation::PRESS;
    Event<void> _OnActivated;

    bool _hovered = false;

    Label* _text = nullptr;

    ID2D1Bitmap* _backgroundImageHovered = nullptr;
    D2D1_COLOR_F _backgroundColorHovered = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
    ID2D1Bitmap* _backgroundImageClicked = nullptr;
    D2D1_COLOR_F _backgroundColorClicked = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f);
    ID2D1Bitmap* _backgroundImageBackup = nullptr;
    D2D1_COLOR_F _backgroundColorBackup = D2D1::ColorF(0, 0.0f);

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

    void SetHoverBackgroundImage(ID2D1Bitmap* image)
    {
        _backgroundImageHovered = image;
    }

    void SetHoverBackgroundColor(D2D1_COLOR_F color)
    {
        _backgroundColorHovered = color;
    }

    void SetClickBackgroundImage(ID2D1Bitmap* image)
    {
        _backgroundImageClicked = image;
    }

    void SetClickBackgroundColor(D2D1_COLOR_F color)
    {
        _backgroundColorClicked = color;
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