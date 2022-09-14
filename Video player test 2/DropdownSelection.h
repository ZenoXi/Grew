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
        void _OnResize(int width, int height)
        {
            Button::_OnResize(width, height);
            Text()->Resize(width - 25, height);
        }

        EventTargets _OnLeftPressed(int x, int y);

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "dropdown_selection"; }
#pragma endregion

    private:
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

            SetButtonImageAll(ResourceManager::GetImage("menu_arrow_down_7x7"));
            ButtonImage()->SetPlacement(ImagePlacement::CENTER_RIGHT);
            ButtonImage()->SetImageOffsetX(-9.0f);
            UseImageParamsForAll(ButtonImage());
            ButtonImage()->SetTintColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
            ButtonHoverImage()->SetTintColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));
            ButtonClickImage()->SetTintColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue));

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