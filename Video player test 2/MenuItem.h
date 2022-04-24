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
                g.target->FillRectangle(D2D1::RectF(25.0f, 1.0f, g.target->GetSize().width - 5.0f, 2.0f), brush);
                brush->Release();
            }
            else if (_label)
            {
                auto size = g.target->GetSize();
                if (size.width <= 50)
                    size.width = 51;

                g.target->DrawBitmap(
                    _label->Draw(g),
                    D2D1::RectF(25, 0, size.width - 25, size.height)
                );
            }

            // Draw icon/checkmark
            if (_icon && !_checked)
            {
                g.target->DrawBitmap(_icon, D2D1::RectF(0.0f, 0.0f, 25.0f, 25.0f));
            }
            if (_checkmarkIcon && _checked)
            {
                g.target->DrawBitmap(_checkmarkIcon, D2D1::RectF(0.0f, 0.0f, 25.0f, 25.0f));
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
                _label->SetWidth(width - 50);
                _label->SetHeight(height);
                _label->Resize();
            }
        }

        //EventTargets _OnLeftPressed(int x, int y)
        //{
        //    _onClick();
        //    return EventTargets().Add(this, x, y);
        //}

    public:
        const char* GetName() const { return "menu_item"; }
#pragma endregion

    private:
        std::unique_ptr<Label> _label = nullptr;
        MenuPanel* _menuPanel = nullptr;
        ID2D1Bitmap* _menuExpandArrow = nullptr;
        std::function<void(bool)> _onClick;
        bool _separator = false;
        ID2D1Bitmap* _icon = nullptr;
        ID2D1Bitmap* _checkmarkIcon = nullptr;

        // Exactly 1 item from a check group must be checked at a time
        int _checkGroup = -1;
        bool _checked = false;
        bool _checkable = false;

    public:
        // Separator
        MenuItem()
        {
            _separator = true;

            SetBaseHeight(3);
            SetParentWidthPercent(1.0f);
        }
        MenuItem(std::wstring text, std::function<void(bool)> onClick = [](bool) {})
        {
            _onClick = onClick;

            _label = std::make_unique<Label>(text);
            _label->SetWidth(GetWidth() - 50);
            _label->SetHeight(GetHeight());
            _label->SetVerticalTextAlignment(zcom::Alignment::CENTER);
            _label->SetMargins({ 5.0f });
            _label->SetCutoff(L"...");
            _label->Resize();

            _checkmarkIcon = ResourceManager::GetImage("checkmark");

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

        // For non-checkable items, 'checked' will always be true
        void Invoke(bool checked = true)
        {
            if (_onClick)
                _onClick(checked);
        }

        // Calculates the width of this item, which displays the text without cutoff
        int CalculateWidth()
        {
            if (!_label) return 0;

            _label->SetCutoff(L"");
            int totalWidth = ceilf(_label->GetTextWidth()) + 50;
            _label->SetCutoff(L"...");
            return totalWidth;
        }

        bool IsSeparator() const
        {
            return _separator;
        }

        void SetIcon(ID2D1Bitmap* icon)
        {
            _icon = icon;
        }

        void SetCheckGroup(int group)
        {
            _checkGroup = group;
        }

        int CheckGroup() const
        {
            return _checkGroup;
        }

        void SetChecked(bool checked)
        {
            _checked = checked;
        }

        bool Checked() const
        {
            return _checked;
        }

        void SetCheckable(bool checkable)
        {
            _checkable = checkable;
        }

        bool Checkable() const
        {
            return _checkable;
        }
    };
}