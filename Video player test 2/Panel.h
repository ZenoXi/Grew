#pragma once

#include "ComponentBase.h"

class Panel : public ComponentBase
{
#pragma region base_class
private:
    void _OnUpdate()
    {

    }

    void _OnDraw(Graphics g)
    {
        // Get bitmaps of all items
        std::vector<ID2D1Bitmap*> bitmaps;
        bitmaps.reserve(_items.size());
        for (auto& item : _items)
        {
            bitmaps.push_back(item->Draw(g));
        }

        // Draw the bitmaps
        for (int i = 0; i < _items.size(); i++)
        {
            g.target->DrawBitmap(
                bitmaps[i],
                D2D1::RectF(
                    _items[i]->GetX(),
                    _items[i]->GetY(),
                    _items[i]->GetX() + _items[i]->GetWidth(),
                    _items[i]->GetY() + _items[i]->GetHeight()
                ),
                _items[i]->GetOpacity()
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

    ComponentBase* _OnMouseMove(int x, int y)
    {
        std::vector<ComponentBase*> hoveredComponents;

        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (x >= item->GetX() && x < item->GetX() + item->GetWidth() &&
                y >= item->GetY() && y < item->GetY() + item->GetHeight())
            {
                if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                {
                    return item->OnMouseMove(x - item->GetX(), y - item->GetY());
                }
                hoveredComponents.push_back(item);
            }
            else if (item->GetMouseInside())
            {
                if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                {
                    return item->OnMouseMove(x - item->GetX(), y - item->GetY());
                }
                else
                {
                    item->OnMouseLeave();
                }
            }
        }

        //std::cout << hoveredComponents.size() << std::endl;

        if (!hoveredComponents.empty())
        {
            ComponentBase* topmost = hoveredComponents[0];
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

    ComponentBase* _OnLeftPressed(int x, int y)
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

    ComponentBase* _OnLeftReleased(int x, int y)
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

    ComponentBase* _OnRightPressed(int x, int y)
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

    ComponentBase* _OnRightReleased(int x, int y)
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

    ComponentBase* _OnWheelUp(int x, int y)
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

    ComponentBase* _OnWheelDown(int x, int y)
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

public:
    std::list<ComponentBase*> GetChildren()
    {
        std::list<ComponentBase*> children;
        for (auto& item : _items)
        {
            children.push_back(item);
        }
        return children;
    }

    std::list<ComponentBase*> GetAllChildren()
    {
        std::list<ComponentBase*> children;
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

    const char* GetName() const { return "panel"; }
#pragma endregion

private:
    std::vector<ComponentBase*> _items;

    // Scrolling
    int _contentWidth;
    int _contentHeight;
    int _horizontalScroll;
    int _verticalScroll;
    bool _horizontalScrollable = true;
    bool _verticalScrollable = true;

public:
    Panel() {}
    ~Panel()
    {
        // Release resources
        //for (auto item : _items)
        //{
        //    delete item;
        //}
    }
    Panel(Panel&&) = delete;
    Panel& operator=(Panel&&) = delete;
    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;

    void AddItem(ComponentBase* item)
    {
        _items.push_back(item);
    }

    void RemoveItem(ComponentBase* item)
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

    ComponentBase* GetItem(int index)
    {
        return _items[index];
    }

    // Scrolling

};