#pragma once

#include "Label.h"
#include "ResourceManager.h"

namespace zcom
{
    class MenuPanel;

    class MenuItem : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {

        }

        void _OnDraw(Graphics g)
        {
            if (_separator)
            {
                ID2D1SolidColorBrush* brush = nullptr;
                g.target->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.2f, 0.2f), &brush);
                g.target->FillRectangle(D2D1::RectF(5.0f, 1.0f, g.target->GetSize().width - 5.0f, 2.0f), brush);
                brush->Release();
            }
            else if (_label)
            {
                g.target->DrawBitmap(_label->Draw(g));
            }

            // Draw expand arrow
            if (_menuPanel)
            {
                auto size = g.target->GetSize();
                g.target->DrawBitmap(_menuExpandArrow, D2D1::RectF(
                    size.width - 25,
                    0,
                    size.width,
                    size.height
                ));
            }
        }

        void _OnResize(int width, int height)
        {
            if (_label)
            {
                _label->SetWidth(width);
                _label->SetHeight(height);
            }
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            _onClick();
            return EventTargets().Add(this, x, y);
        }

    public:
        const char* GetName() const { return "menu_item"; }
#pragma endregion

    private:
        std::unique_ptr<Label> _label = nullptr;
        MenuPanel* _menuPanel = nullptr;
        ID2D1Bitmap* _menuExpandArrow = nullptr;
        std::function<void()> _onClick;
        bool _separator = false;

    public:
        // Separator
        MenuItem()
        {
            _separator = true;

            SetBaseHeight(3);
            SetParentWidthPercent(1.0f);
        }
        MenuItem(std::wstring text, std::function<void()> onClick = []() {})
        {
            _onClick = onClick;

            _label = std::make_unique<Label>(text);
            _label->SetBaseHeight(25);
            _label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
            _label->SetMargins({ 5.0f });

            SetBaseHeight(25);
            SetParentWidthPercent(1.0f);
        }
        MenuItem(MenuPanel* panel, std::wstring text) : MenuItem(text)
        {
            _menuPanel = panel;
            _menuExpandArrow = ResourceManager::GetImage("menu_expand");
        }
        ~MenuItem() {}
        MenuItem(MenuItem&&) = delete;
        MenuItem& operator=(MenuItem&&) = delete;
        MenuItem(const MenuItem&) = delete;
        MenuItem& operator=(const MenuItem&) = delete;

        MenuPanel* GetMenuPanel() const
        {
            return _menuPanel;
        }

        bool IsSeparator() const
        {
            return _separator;
        }
    };
}