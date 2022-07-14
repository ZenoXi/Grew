#pragma once

#include "ComponentBase.h"
#include "Button.h"

namespace zcom
{
    class TopMenuButton : public Button
    {
    public:
        void SetOnHovered(const std::function<void()>& func)
        {
            _OnHovered.Add(func);
        }

    protected:
        friend class Scene;
        friend class Base;
        TopMenuButton(Scene* scene) : Button(scene) {}
        void Init(std::wstring text)
        {
            Button::Init(text);
            SetActivation(ButtonActivation::PRESS);
            SetPreset(ButtonPreset::NO_EFFECTS);
            SetButtonHoverColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));
            SetButtonClickColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f));
            SetSelectable(false);
            Text()->SetFont(L"Segoe UI");
            Text()->SetFontSize(12.0f);
            Text()->SetFontColor(D2D1::ColorF(D2D1::ColorF::White));
        }
    public:
        ~TopMenuButton() {}
        TopMenuButton(TopMenuButton&&) = delete;
        TopMenuButton& operator=(TopMenuButton&&) = delete;
        TopMenuButton(const TopMenuButton&) = delete;
        TopMenuButton& operator=(const TopMenuButton&) = delete;

    private:
        Event<void> _OnHovered;

#pragma region base_class
    protected:
        void _OnMouseEnter()
        {
            _OnHovered.InvokeAll();
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "top_menu_button"; }
#pragma endregion
    };
}