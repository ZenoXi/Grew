#pragma once

#include "ComponentBase.h"
#include "Label.h"
#include "Image.h"

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
            return _text->Redraw()
                || _image->Redraw()
                || _imageHovered->Redraw()
                || _imageClicked->Redraw();
        }

        void _OnDraw(Graphics g)
        {
            // Update images
            if (_image->Redraw())
                _image->Draw(g);
            if (_imageHovered->Redraw())
                _imageHovered->Draw(g);
            if (_imageClicked->Redraw())
                _imageClicked->Draw(g);

            D2D1_COLOR_F color;
            zcom::Image* image = nullptr;
            if (GetMouseInsideArea())
            {
                if (GetMouseLeftClicked())
                {
                    color = _colorClicked;
                    image = _imageClicked.get();
                }
                else
                {
                    color = _colorHovered;
                    image = _imageHovered.get();
                }
            }
            else
            {
                color = _color;
                image = _image.get();
            }
            
            // Draw button color
            ID2D1SolidColorBrush* brush;
            g.target->CreateSolidColorBrush(color, &brush);
            g.target->FillRectangle
            (
                D2D1::RectF(0, 0, g.target->GetSize().width, g.target->GetSize().height),
                brush
            );
            brush->Release();

            // Draw button image
            if (image && image->GetImage())
                g.target->DrawBitmap(image->Base::Image());

            // Draw button text
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
            _image->Resize(width, height);
            _imageHovered->Resize(width, height);
            _imageClicked->Resize(width, height);
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

        void _OnSelected(bool reverse);

        void _OnDeselected();

        bool _OnHotkey(int id)
        {
            return false;
        }

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
        const char* GetName() const { return Name(); }
        static const char* Name() { return "button"; }
#pragma endregion

    private:
        bool _activated = false;
        ButtonActivation _activation = ButtonActivation::PRESS;
        Event<void> _OnActivated;

        bool _hovered = false;

        std::unique_ptr<Label> _text = nullptr;
        std::unique_ptr<zcom::Image> _image = nullptr;
        std::unique_ptr<zcom::Image> _imageHovered = nullptr;
        std::unique_ptr<zcom::Image> _imageClicked = nullptr;
        D2D1_COLOR_F _color = D2D1::ColorF(0, 0.0f);
        D2D1_COLOR_F _colorHovered = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
        D2D1_COLOR_F _colorClicked = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f);

    protected:
        friend class Scene;
        friend class Base;
        Button(Scene* scene) : Base(scene) {}
        void Init(std::wstring text = L"")
        {
            SetDefaultCursor(CursorIcon::HAND);
            SetSelectable(true);
            _text = Create<Label>(text);
            _text->SetSize(GetWidth(), GetHeight());
            _text->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _text->SetVerticalTextAlignment(Alignment::CENTER);

            _image = Create<zcom::Image>();
            _image->SetSize(GetWidth(), GetHeight());
            _image->SetPlacement(ImagePlacement::FIT);
            _imageHovered = Create<zcom::Image>();
            _imageHovered->SetSize(GetWidth(), GetHeight());
            _imageHovered->SetPlacement(ImagePlacement::FIT);
            _imageClicked = Create<zcom::Image>();
            _imageClicked->SetSize(GetWidth(), GetHeight());
            _imageClicked->SetPlacement(ImagePlacement::FIT);
        }
    public:
        ~Button() {}
        Button(Button&&) = delete;
        Button& operator=(Button&&) = delete;
        Button(const Button&) = delete;
        Button& operator=(const Button&) = delete;

        void SetButtonImageAll(ID2D1Bitmap* image)
        {
            _image->SetImage(image);
            _imageHovered->SetImage(image);
            _imageClicked->SetImage(image);
        }

        void SetButtonColorAll(D2D1_COLOR_F color)
        {
            SetButtonColor(color);
            SetButtonHoverColor(color);
            SetButtonClickColor(color);
        }

        zcom::Image* ButtonImage()
        {
            return _image.get();
        }

        zcom::Image* ButtonHoverImage()
        {
            return _imageHovered.get();
        }

        zcom::Image* ButtonClickImage()
        {
            return _imageClicked.get();
        }

        // Copies all parameters (except the image itself) to all button images
        void UseImageParamsForAll(zcom::Image* image)
        {
            // Copy image params before overwriting internal images
            // in case and internal image is used as the copy base
            RECT_F sourceRect = image->GetSourceRect();
            RECT_F targetRect = image->GetTargetRect();
            ImagePlacement placement = image->GetPlacement();
            float offsetX = image->GetImageOffsetX();
            float offsetY = image->GetImageOffsetY();
            float scaleX = image->GetScaleX();
            float scaleY = image->GetScaleY();
            bool snap = image->GetPixelSnap();
            float opacity = image->GetImageOpacity();
            D2D1_COLOR_F color = image->GetTintColor();

            // Apply to internal images
            _image->SetSourceRect(sourceRect);
            _image->SetTargetRect(targetRect);
            _image->SetPlacement(placement);
            _image->SetImageOffset(offsetX, offsetY);
            _image->SetScale(scaleX, scaleY);
            _image->SetPixelSnap(snap);
            _image->SetImageOpacity(opacity);
            _image->SetTintColor(color);
            _imageHovered->SetSourceRect(sourceRect);
            _imageHovered->SetTargetRect(targetRect);
            _imageHovered->SetPlacement(placement);
            _imageHovered->SetImageOffset(offsetX, offsetY);
            _imageHovered->SetScale(scaleX, scaleY);
            _imageHovered->SetPixelSnap(snap);
            _imageHovered->SetImageOpacity(opacity);
            _imageHovered->SetTintColor(color);
            _imageClicked->SetSourceRect(sourceRect);
            _imageClicked->SetTargetRect(targetRect);
            _imageClicked->SetPlacement(placement);
            _imageClicked->SetImageOffset(offsetX, offsetY);
            _imageClicked->SetScale(scaleX, scaleY);
            _imageClicked->SetPixelSnap(snap);
            _imageClicked->SetImageOpacity(opacity);
            _imageClicked->SetTintColor(color);
        }

        void SetButtonColor(D2D1_COLOR_F color)
        {
            if (color == _color)
                return;
            _color = color;
            InvokeRedraw();
        }

        void SetButtonHoverColor(D2D1_COLOR_F color)
        {
            if (color == _colorHovered)
                return;
            _colorHovered = color;
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
            return _text.get();
        }
    };
}