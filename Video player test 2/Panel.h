#pragma once

#include "ComponentBase.h"

#include <algorithm>

namespace zcom
{
    class Panel : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            for (auto item : _items)
            {
                item->Update();
            }
        }

        void _OnDraw(Graphics g)
        {
            // Get bitmaps of all items
            std::list<std::pair<ID2D1Bitmap*, Base*>> bitmaps;
            for (auto& item : _items)
            {
                // Order by z-index
                auto it = bitmaps.rbegin();
                for (; it != bitmaps.rend(); it++)
                {
                    if (item->GetZIndex() >= it->second->GetZIndex())
                    {
                        break;
                    }
                }
                bitmaps.insert(it.base(), { item->Draw(g), item });
            }

            // Draw the bitmaps
            for (auto& it : bitmaps)
            {
                g.target->DrawBitmap(
                    it.first,
                    D2D1::RectF(
                        it.second->GetX(),
                        it.second->GetY(),
                        it.second->GetX() + it.second->GetWidth(),
                        it.second->GetY() + it.second->GetHeight()
                    ),
                    it.second->GetOpacity()
                );
            }
        }

        void _OnResize(int width, int height)
        {
            // Calculate item sizes and positions
            for (auto& item : _items)
            {
                int newWidth = (int)std::round(GetWidth() * item->GetParentWidthPercent()) + item->GetBaseWidth();
                int newHeight = (int)std::round(GetHeight() * item->GetParentHeightPercent()) + item->GetBaseHeight();
                // SetSize does limit checking so the resulting size and newWidth/newHeight can mismatch
                item->SetSize(newWidth, newHeight);

                int newPosX = 0;
                if (item->GetHorizontalAlignment() == Alignment::START)
                {
                    newPosX += std::round((GetWidth() - item->GetWidth()) * item->GetHorizontalOffsetPercent());
                }
                else if (item->GetHorizontalAlignment() == Alignment::CENTER)
                {
                    newPosX = (GetWidth() - item->GetWidth()) / 2;
                }
                else if (item->GetHorizontalAlignment() == Alignment::END)
                {
                    newPosX = GetWidth() - item->GetWidth();
                    newPosX -= std::round((GetWidth() - item->GetWidth()) * item->GetHorizontalOffsetPercent());
                }
                newPosX += item->GetHorizontalOffsetPixels();
                // Alternative (no branching):
                // int align = item->GetHorizontalAlignment() == Alignment::END;
                // newPosX += align * (_width - item->GetWidth());
                // newPosX += (-1 * align) * std::round((_width - item->GetWidth()) * item->GetHorizontalOffsetPercent());
                // newPosX += item->GetHorizontalOffsetPixels();
                int newPosY = 0;
                if (item->GetVerticalAlignment() == Alignment::START)
                {
                    newPosY += std::round((GetHeight() - item->GetHeight()) * item->GetVerticalOffsetPercent());
                }
                else if (item->GetVerticalAlignment() == Alignment::CENTER)
                {
                    newPosY = (GetHeight() - item->GetHeight()) / 2;
                }
                else if (item->GetVerticalAlignment() == Alignment::END)
                {
                    newPosY = GetHeight() - item->GetHeight();
                    newPosY -= std::round((GetHeight() - item->GetHeight()) * item->GetVerticalOffsetPercent());
                }
                newPosY += item->GetVerticalOffsetPixels();
                item->SetPosition(newPosX, newPosY);

                item->Resize(item->GetWidth(), item->GetHeight());
            }
        }

        Base* _OnMouseMove(int x, int y)
        {
            std::vector<Base*> hoveredComponents;

            Base* handledItem = nullptr;
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (x >= item->GetX() && x < item->GetX() + item->GetWidth() &&
                    y >= item->GetY() && y < item->GetY() + item->GetHeight())
                {
                    if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                    {
                        if (handledItem == nullptr)
                            handledItem = item->OnMouseMove(x - item->GetX(), y - item->GetY());
                    }
                    hoveredComponents.push_back(item);
                    if (!item->GetMouseInsideArea())
                    {
                        item->OnMouseEnterArea();
                    }
                }
                else
                {
                    if (item->GetMouseInside())
                    {
                        if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                        {
                            if (handledItem == nullptr)
                                handledItem = item->OnMouseMove(x - item->GetX(), y - item->GetY());
                        }
                        else
                        {
                            item->OnMouseLeave();
                        }
                    }
                    if (item->GetMouseInsideArea())
                    {
                        item->OnMouseLeaveArea();
                    }
                }
            }
            if (handledItem)
                return handledItem;

            //std::cout << hoveredComponents.size() << std::endl;

            if (!hoveredComponents.empty())
            {
                Base* topmost = hoveredComponents[0];
                for (int i = 1; i < hoveredComponents.size(); i++)
                {
                    if (hoveredComponents[i]->GetZIndex() > topmost->GetZIndex())
                    {
                        topmost = hoveredComponents[i];
                    }
                }

                for (int i = 0; i < hoveredComponents.size(); i++)
                {
                    if (hoveredComponents[i] != topmost && hoveredComponents[i]->GetMouseInside())
                    {
                        hoveredComponents[i]->OnMouseLeave();
                    }
                }

                if (!topmost->GetMouseInside())
                {
                    topmost->OnMouseEnter();
                }
                return topmost->OnMouseMove(x - topmost->GetX(), y - topmost->GetY());
            }

            return this;

            for (auto& item : _items)
            {
                if (x >= item->GetX() && x < item->GetX() + item->GetWidth() &&
                    y >= item->GetY() && y < item->GetY() + item->GetHeight())
                {
                    if (!item->GetMouseInside())
                    {
                        item->OnMouseEnter();
                    }
                    item->OnMouseMove(x - item->GetX(), y - item->GetY());
                }
                else if (item->GetMouseInside())
                {
                    if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                    {
                        item->OnMouseMove(x - item->GetX(), y - item->GetY());
                    }
                    else
                    {
                        item->OnMouseLeave();
                    }
                }
            }
        }

        void _OnMouseLeave()
        {
            //std::cout << "Mouse leave\n";
            for (auto& item : _items)
            {
                if (item->GetMouseInside())
                {
                    item->OnMouseLeave();
                }
            }
        }

        void _OnMouseEnter()
        {

            //std::cout << "Mouse enter\n";
        }

        void _OnMouseLeaveArea()
        {
            //std::cout << "Mouse leave\n";
            for (auto& item : _items)
            {
                if (item->GetMouseInsideArea())
                {
                    item->OnMouseLeaveArea();
                }
            }
        }

        void _OnMouseEnterArea()
        {

            //std::cout << "Mouse enter\n";
        }

        Base* _OnLeftPressed(int x, int y)
        {
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnLeftPressed(x - item->GetX(), y - item->GetY());
                }
            }
            return this;
        }

        Base* _OnLeftReleased(int x, int y)
        {
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnLeftReleased(x - item->GetX(), y - item->GetY());
                }
            }
            return this;
        }

        Base* _OnRightPressed(int x, int y)
        {
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnRightPressed(x - item->GetX(), y - item->GetY());
                }
            }
            return this;
        }

        Base* _OnRightReleased(int x, int y)
        {
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnRightReleased(x - item->GetX(), y - item->GetY());
                }
            }
            return this;
        }

        Base* _OnWheelUp(int x, int y)
        {
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnWheelUp(x - item->GetX(), y - item->GetY());
                }
            }
            return this;
        }

        Base* _OnWheelDown(int x, int y)
        {
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnWheelDown(x - item->GetX(), y - item->GetY());
                }
            }
            return this;
        }

        void _OnSelected()
        {
            //if (!_selectableItems.empty())
            //{
            //    OnDeselected();
            //    _selectableItems[0]->OnSelected();
            //}
        }

    public:
        std::list<Base*> GetChildren()
        {
            std::list<Base*> children;
            for (auto& item : _items)
            {
                children.push_back(item);
            }
            return children;
        }

        std::list<Base*> GetAllChildren()
        {
            std::list<Base*> children;
            for (auto& item : _items)
            {
                children.push_back(item);
                auto itemChildren = item->GetAllChildren();
                if (!itemChildren.empty())
                {
                    children.insert(children.end(), itemChildren.begin(), itemChildren.end());
                }
            }
            return children;
        }

        Base* IterateTab()
        {
            if (_selectableItems.empty())
            {
                return nullptr;
                //if (Selected())
                //    return nullptr;
                //else
                //    return this;
            }

            // While searching == true, _selectableItems is iterated until the first
            // available to select item is found (IterateTab() returns itself). If no
            // available item is found return null to signal end of tab selection.
            // 'Available' means visible, active, can be selected (returns !nullptr).
            bool searching = false;

            for (int i = 0; i < _selectableItems.size(); i++)
            {
                if (!_selectableItems[i]->GetVisible() || !_selectableItems[i]->GetActive())
                    continue;

                Base* item = _selectableItems[i]->IterateTab();
                if (item == nullptr)
                {
                    if (!searching)
                    {
                        searching = true;
                        continue;
                    }
                    //if (_selectableItems.size() > i + 1)
                    //    return _selectableItems[i + 1];
                    //else
                    //    return nullptr;
                }
                else if (item != _selectableItems[i])
                {
                    return item;
                }
                else if (searching)
                {
                    return item;
                }
            }

            if (searching)
                return nullptr;

            return _selectableItems[0];
        }

        const char* GetName() const { return "panel"; }
#pragma endregion

    private:
        std::vector<Base*> _items;
        std::vector<Base*> _selectableItems;

        // Scrolling
        int _contentWidth = 0;
        int _contentHeight = 0;
        int _horizontalScroll = 0;
        int _verticalScroll = 0;
        bool _horizontalScrollable = true;
        bool _verticalScrollable = true;

    public:
        Panel()
        {
            // By default allow iterating nested components
            SetTabIndex(0);
        }
        ~Panel() {}
        Panel(Panel&&) = delete;
        Panel& operator=(Panel&&) = delete;
        Panel(const Panel&) = delete;
        Panel& operator=(const Panel&) = delete;

        void AddItem(Base* item)
        {
            _items.push_back(item);
            ReindexTabOrder();
        }

        void RemoveItem(Base* item)
        {
            for (int i = 0; i < _items.size(); i++)
            {
                if (_items[i] == item)
                {
                    _items.erase(_items.begin() + i);
                    return;
                }
            }
        }

        void RemoveItem(int index)
        {
            _items.erase(_items.begin() + index);
        }

        int ItemCount() const
        {
            return _items.size();
        }

        Base* GetItem(int index)
        {
            return _items[index];
        }

        void ClearItems()
        {
            _items.clear();
        }

        void ReindexTabOrder()
        {
            _selectableItems.clear();
            for (int i = 0; i < _items.size(); i++)
            {
                if (_items[i]->GetTabIndex() != -1)
                {
                    _selectableItems.push_back(_items[i]);
                }
            }

            // Sort indices
            std::sort(_selectableItems.begin(), _selectableItems.end(), [](Base* a, Base* b) { return a->GetTabIndex() < b->GetTabIndex(); });

            // Remove duplicates
            //for (int i = 1; i < _selectableItems.size(); i++)
            //{
            //    if (_selectableItems[i - 1]->GetTabIndex() == _selectableItems[i]->GetTabIndex())
            //    {
            //        _selectableItems.erase(_selectableItems.begin() + i);
            //        i--;
            //    }
            //}
        }

        // Scrolling

    };
}