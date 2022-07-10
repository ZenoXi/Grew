#pragma once

#include "Panel.h"
#include "MenuItem.h"

namespace zcom
{
    class Canvas;

    class MenuPanel : public Panel
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            Panel::_OnUpdate();

            // Hide panel
            if (_childShouldHide && ztime::Main() - _childHoverEndTime >= _hoverToShowDuration)
            {
                if (_openChildPanel)
                {
                    _openChildPanel->Hide();
                    _openChildPanel = nullptr;
                }
                _childShouldHide = false;
            }

            // Show panel
            if (_childToShow && ztime::Main() - _childHoverStartTime >= _hoverToShowDuration)
            {
                if (_openChildPanel)
                {
                    _openChildPanel->Hide();
                    _openChildPanel = nullptr;
                }
                _openChildPanel = _childToShow;
                _openChildPanel->Show(_bounds, _parentRect, this);
                _childToShow = nullptr;
            }
        }

        EventTargets _OnMouseMove(int x, int y, bool duplicate)
        {
            auto targets = Panel::_OnMouseMove(x, y, duplicate);
            Base* mainTarget = targets.MainTarget();
            auto it = std::find_if(_items.begin(), _items.end(), [mainTarget](Item& item) { return item.item == mainTarget; });
            if (it != _items.end())
            {
                MenuItem* item = (MenuItem*)it->item;

                if (_hoveredItem)
                    _hoveredItem->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
                _hoveredItem = item;
                if (!_hoveredItem->IsSeparator() && !_hoveredItem->Disabled())
                    _hoveredItem->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f));

                // Stop scheduled hide
                if (_openChildPanel && item->GetMenuPanel() == _openChildPanel)
                {
                    _childShouldHide = false;
                }

                // Stop scheduled show
                if (_childToShow && item->GetMenuPanel() != _childToShow)
                {
                    _childToShow = nullptr;
                }

                // Prime open panel to hide
                if (_openChildPanel && item->GetMenuPanel() != _openChildPanel)
                {
                    if (!_childShouldHide)
                    {
                        _childShouldHide = true;
                        _childHoverEndTime = ztime::Main();
                    }
                }

                // Prime panel to open
                if (item->GetMenuPanel() && item->GetMenuPanel() != _openChildPanel)
                {
                    if (!_childToShow)
                    {
                        _childToShow = item->GetMenuPanel();
                        _childHoverStartTime = ztime::Main();
                        // RECT describing this 'item' in MenuPanel plane coordinates
                        _parentRect = {
                            GetX() + 0,
                            GetY() + item->GetY(),
                            GetX() + GetWidth(),
                            GetY() + item->GetY() + item->GetHeight()
                        };
                    }
                }
            }

            return std::move(targets.Add(this, x, y));
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            auto targets = Panel::_OnLeftPressed(x, y);
            Base* mainTarget = targets.MainTarget();
            auto it = std::find_if(_items.begin(), _items.end(), [mainTarget](Item& item) { return item.item == mainTarget; });
            if (it != _items.end())
            {
                MenuItem* item = (MenuItem*)it->item;

                _childHoverEndTime = ztime::Main() - _hoverToShowDuration;
                _childHoverStartTime = ztime::Main() - _hoverToShowDuration;

                if (!item->Disabled())
                {
                    // Handle checkable items
                    if (item->Checkable())
                    {
                        if (item->CheckGroup() == -1)
                        {
                            item->Invoke(!item->Checked());
                            item->SetChecked(!item->Checked());
                        }
                        else
                        {
                            if (!item->Checked())
                            {
                                // Uncheck others from same group
                                for (int i = 0; i < _items.size(); i++)
                                {
                                    MenuItem* mItem = (MenuItem*)_items[i].item;
                                    if (mItem->CheckGroup() == item->CheckGroup() && mItem->Checked())
                                    {
                                        mItem->SetChecked(false);
                                    }
                                }

                                item->Invoke(true);
                                item->SetChecked(true);
                            }
                        }
                    }
                    else
                    {
                        item->Invoke();
                    }
                }

                if (!item->GetMenuPanel() && !item->IsSeparator() && !item->Disabled())
                    FullClose();
            }

            return std::move(targets.Add(this, x, y));
        }

    public:
        const char* GetName() const { return "menu_panel"; }
#pragma endregion

    private:
        Canvas* _sceneCanvas = nullptr;
        MenuPanel* _parentPanel = nullptr;
        MenuPanel* _openChildPanel = nullptr;
        MenuItem* _hoveredItem = nullptr;

        RECT _bounds = { 0, 0, 0, 0 };
        RECT _parentRect = { 0, 0, 0, 0 };
        int _maxWidth = 600;
        int _minWidth = 70;

        TimePoint _childHoverStartTime = 0;
        MenuPanel* _childToShow = nullptr;
        TimePoint _childHoverEndTime = 0;
        bool _childShouldHide = false;

        TimePoint _showTime = 0;
        Duration _hoverToShowDuration = Duration(250, MILLISECONDS);

    protected:
        friend class Scene;
        friend class Base;
        MenuPanel(Scene* scene) : Panel(scene) {}
        void Init(Canvas* sceneCanvas)
        {
            Panel::Init();

            _sceneCanvas = sceneCanvas;
            _AddHandlerToCanvas();

            SetBackgroundColor(D2D1::ColorF(0.05f, 0.05f, 0.05f));
            SetBorderVisibility(true);
            SetBorderColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
            zcom::PROP_Shadow shadow;
            shadow.color = D2D1::ColorF(0);
            shadow.offsetX = 2.0f;
            shadow.offsetY = 2.0f;
            SetProperty(shadow);
            SetVisible(false);
        }
    public:
        ~MenuPanel() { ClearItems(); }
        MenuPanel(MenuPanel&&) = delete;
        MenuPanel& operator=(MenuPanel&&) = delete;
        MenuPanel(const MenuPanel&) = delete;
        MenuPanel& operator=(const MenuPanel&) = delete;

        void SetMaxWidth(int maxWidth)
        {
            if (_maxWidth != maxWidth)
            {
                _maxWidth = maxWidth;
                _RearrangeMenuItems();
            }
        }

        void SetMinWidth(int minWidth)
        {
            if (_minWidth != minWidth)
            {
                _minWidth = minWidth;
                _RearrangeMenuItems();
            }
        }

        void AddItem(std::unique_ptr<MenuItem> item)
        {
            Panel::AddItem(item.release(), true);
            _RearrangeMenuItems();
        }

        void ClearItems()
        {
            Panel::ClearItems();
            _RearrangeMenuItems();
        }

        void Show(RECT bounds, RECT parentRect, MenuPanel* parentPanel = nullptr)
        {
            _bounds = bounds;
            _parentRect = parentRect;
            _parentPanel = parentPanel;

            _CalculatePlacement();
            // Place this panel above parent
            if (_parentPanel)
                SetZIndex(_parentPanel->GetZIndex() + 1);
            SetVisible(true);
            _showTime = ztime::Main();
        }

        void Hide()
        {
            if (_openChildPanel)
            {
                _openChildPanel->Hide();
                _openChildPanel = nullptr;
            }
            if (_hoveredItem)
                _hoveredItem->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
            SetVisible(false);
        }

        // Called by child MenuPanel when it closes itself
        void OnChildClosed(const EventTargets* targets)
        {
            _openChildPanel = nullptr;
            if (!targets->Contains(this))
            {
                Hide();
                if (_parentPanel)
                    _parentPanel->OnChildClosed(targets);
            }
        }

        // Iterates to the root menu and closes it, closing all child menus
        void FullClose()
        {
            if (_parentPanel)
                _parentPanel->FullClose();
            else
                Hide();
        }

    private:
        void _AddHandlerToCanvas();

        void _RearrangeMenuItems()
        {
            constexpr int MARGINS = 2;

            _RecalculateLayout(GetWidth(), GetHeight());
            int totalHeight = MARGINS;
            int maxWidth = 0;
            for (int i = 0; i < _items.size(); i++)
            {
                _items[i].item->SetOffsetPixels(MARGINS, totalHeight);
                _items[i].item->SetBaseWidth(-MARGINS * 2);
                totalHeight += _items[i].item->GetHeight();
                int width = ((MenuItem*)_items[i].item)->CalculateWidth();
                if (width > maxWidth)
                    maxWidth = width;
            }

            if (maxWidth < _minWidth)
                maxWidth = _minWidth;
            if (maxWidth > _maxWidth)
                maxWidth = _maxWidth;

            SetBaseSize(maxWidth + MARGINS * 2, totalHeight + MARGINS);
            _CalculatePlacement();
        }

        void _CalculatePlacement()
        {
            constexpr int LEFT = 1;
            constexpr int RIGHT = 2;
            constexpr int UP = 1;
            constexpr int DOWN = 2;

            // Horizontal placement
            int hPlacement;
            if (_parentRect.right - 3 + GetWidth() < _bounds.right)
                hPlacement = RIGHT;
            else if (_parentRect.left + 3 - GetWidth() > _bounds.left)
                hPlacement = LEFT;
            else
                if (_bounds.right - (_parentRect.right - 3) > (_parentRect.left + 3) - _bounds.left)
                    hPlacement = RIGHT;
                else
                    hPlacement = LEFT;

            // Vertical placement
            int vPlacement;
            if (_parentRect.top + GetHeight() < _bounds.bottom)
                vPlacement = DOWN;
            else if (_parentRect.bottom - GetHeight() > _bounds.top)
                vPlacement = UP;
            else
                if (_bounds.bottom - _parentRect.top > _parentRect.bottom - _bounds.top)
                    vPlacement = DOWN;
                else
                    vPlacement = UP;

            // Final x position
            int xPos;
            if (hPlacement == RIGHT)
                xPos = _parentRect.right - 3;
            else
                xPos = _parentRect.left + 3 - GetWidth();

            // Final y position
            int yPos;
            if (vPlacement == DOWN)
                yPos = _parentRect.top;
            else
                yPos = _parentRect.bottom - GetHeight();

            SetOffsetPixels(xPos, yPos);
        }
    };
}