#pragma once

#include "ComponentBase.h"
#include "Button.h"

#include "MenuPanel.h"

namespace zcom
{
    class DropdownSelection : public Button
    {
#pragma region base_class
    protected:
        void _OnDraw(Graphics g)
        {
            Button::_OnDraw(g);

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
            Text()->Resize(width - 25, height);
        }

        EventTargets _OnLeftPressed(int x, int y);

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "dropdown_selection"; }
#pragma endregion

    private:
        ID2D1Bitmap* _downArrow = nullptr;

        std::unique_ptr<MenuPanel> _menuPanel = nullptr;
        bool _menuPanelEmpty = true;
        std::wstring _defaultText;
        RECT _menuBounds = {0, 0, 0, 0};

    protected:
        friend class Scene;
        friend class Base;
        DropdownSelection(Scene* scene) : Button(scene) {}
        void Init(std::wstring text, std::wstring defaultText = L"No items")
        {
            Button::Init(text);

            SetSelectable(true);
            Text()->SetSize(GetWidth(), GetHeight());
            Text()->SetVerticalTextAlignment(Alignment::CENTER);
            Text()->SetMargins({ 5.0f });
            _downArrow = ResourceManager::GetImage("menu_expand_down");

            _defaultText = defaultText;
            _menuPanel = Create<zcom::MenuPanel>();
            _menuPanel->SetMaxWidth(300);
            _menuPanel->SetZIndex(512); // Arbitrarily chosen number that's almost guaranteed to be higher than all other components
            _menuPanelEmpty = true;
            auto defaultItem = Create<MenuItem>(_defaultText);
            defaultItem->SetDisabled(true);
            _menuPanel->AddItem(std::move(defaultItem));
        }
    public:
        ~DropdownSelection() {}
        DropdownSelection(DropdownSelection&&) = delete;
        DropdownSelection& operator=(DropdownSelection&&) = delete;
        DropdownSelection(const DropdownSelection&) = delete;
        DropdownSelection& operator=(const DropdownSelection&) = delete;

        void AddItem(std::wstring text, std::function<void(bool)> onSelected)
        {
            if (_menuPanelEmpty)
            {
                _menuPanel->ClearItems();
                _menuPanelEmpty = false;
            }
            _menuPanel->AddItem(Create<MenuItem>(text, onSelected));
        }

        void ClearItems()
        {
            _menuPanel->ClearItems();
            _menuPanelEmpty = true;
        }

        void SetMenuBounds(RECT bounds)
        {
            _menuBounds = bounds;
        }
    };
}