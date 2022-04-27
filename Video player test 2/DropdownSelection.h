#pragma once

#include "ComponentBase.h"
#include "Label.h"

#include "MenuPanel.h"

namespace zcom
{
    class DropdownSelection : public Base
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

            D2D1_SIZE_F size = g.target->GetSize();
            g.target->DrawBitmap(
                _downArrow,
                D2D1::RectF(
                    size.width - 25.0f,
                    floorf((size.height - 25.0f) * 0.5f),
                    size.width,
                    floorf((size.height - 25.0f) * 0.5f + size.height)
                )
            );
        }

        void _OnResize(int width, int height)
        {
            _text->SetSize(width - 25, height);
            _text->Resize();
        }

        EventTargets _OnLeftPressed(int x, int y);

    public:
        const char* GetName() const { return "dropdown_selection"; }
#pragma endregion

    private:
        Event<void> _OnActivated;

        bool _hovered = false;

        std::unique_ptr<Label> _text = nullptr;
        ID2D1Bitmap* _downArrow = nullptr;

        Canvas* _canvas = nullptr;
        std::unique_ptr<MenuPanel> _menuPanel = nullptr;
        bool _menuPanelEmpty = true;
        std::wstring _defaultText;
        RECT _menuBounds = {0, 0, 0, 0};

        ID2D1Bitmap* _image = nullptr;
        D2D1_COLOR_F _color = D2D1::ColorF(0, 0.0f);
        ID2D1Bitmap* _imageHovered = nullptr;
        D2D1_COLOR_F _colorHovered = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
        ID2D1Bitmap* _imageClicked = nullptr;
        D2D1_COLOR_F _colorClicked = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.1f);

    public:
        DropdownSelection(std::wstring text, Canvas* canvas, std::wstring defaultText = L"No items")
        {
            SetSelectable(true);
            _text = std::make_unique<Label>(text);
            _text->SetSize(GetWidth(), GetHeight());
            _text->SetVerticalTextAlignment(Alignment::CENTER);
            _text->SetMargins({ 5.0f });
            _downArrow = ResourceManager::GetImage("menu_expand_down");

            _canvas = canvas;
            _defaultText = defaultText;
            _menuPanel = std::make_unique<zcom::MenuPanel>(_canvas);
            _menuPanel->SetMaxWidth(300);
            _menuPanel->SetZIndex(512); // Arbitrarily chosen number that's almost guaranteed to be higher than all other components
            _menuPanelEmpty = true;
            auto defaultItem = std::make_unique<MenuItem>(_defaultText);
            defaultItem->SetDisabled(true);
            _menuPanel->AddMenuItem(std::move(defaultItem));
            _AddPanelToCanvas();
        }
        ~DropdownSelection()
        {
            _RemovePanelFromCanvas();
        }
        DropdownSelection(DropdownSelection&&) = delete;
        DropdownSelection& operator=(DropdownSelection&&) = delete;
        DropdownSelection(const DropdownSelection&) = delete;
        DropdownSelection& operator=(const DropdownSelection&) = delete;

        void AddItem(std::wstring text, std::function<void(bool)> onSelected)
        {
            if (_menuPanelEmpty)
            {
                _menuPanel->ClearMenuItems();
                _menuPanelEmpty = false;
            }
            _menuPanel->AddMenuItem(std::make_unique<MenuItem>(text, onSelected));
        }

        void ClearItems()
        {
            _menuPanel->ClearMenuItems();
            _menuPanelEmpty = true;
        }

        void SetMenuBounds(RECT bounds)
        {
            _menuBounds = bounds;
        }

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

        Label* Text()
        {
            return _text.get();
        }

    private:
        void _AddPanelToCanvas();
        void _RemovePanelFromCanvas();
    };
}