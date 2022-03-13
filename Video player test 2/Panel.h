#pragma once

#include "ComponentBase.h"
#include "GameTime.h"

#include <algorithm>
#include <functional>

namespace zcom
{
    class Panel : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            // Animate vertical scroll
            if (_verticalScrollAnimation.inProgress)
            {
                float timeProgress = (ztime::Main() - _verticalScrollAnimation.startTime).GetDuration() / (float)_verticalScrollAnimation.duration.GetDuration();
                if (timeProgress >= 1.0f)
                {
                    _verticalScrollAnimation.inProgress = false;
                    _verticalScroll = _verticalScrollAnimation.endPos;
                }
                else
                {
                    float moveProgress;
                    if (_verticalScrollAnimation.progressFunction)
                    {
                        moveProgress = _verticalScrollAnimation.progressFunction(timeProgress);
                    }
                    else
                    {
                        moveProgress = 1.0f - powf(timeProgress - 1.0f, 2.0f);
                    }

                    int startPos = _verticalScrollAnimation.startPos;
                    int endPos = _verticalScrollAnimation.endPos;
                    _verticalScroll = startPos + (endPos - startPos) * moveProgress;
                }
                Resize();
            }
            // Animate horizontal scroll
            if (_horizontalScrollAnimation.inProgress)
            {
                float timeProgress = (ztime::Main() - _horizontalScrollAnimation.startTime).GetDuration() / (float)_horizontalScrollAnimation.duration.GetDuration();
                if (timeProgress >= 1.0f)
                {
                    _horizontalScrollAnimation.inProgress = false;
                    _horizontalScroll = _horizontalScrollAnimation.endPos;
                }
                else
                {
                    float moveProgress;
                    if (_horizontalScrollAnimation.progressFunction)
                    {
                        moveProgress = _horizontalScrollAnimation.progressFunction(timeProgress);
                    }
                    else
                    {
                        moveProgress = 1.0f - powf(timeProgress - 1.0f, 2.0f);
                    }

                    int startPos = _horizontalScrollAnimation.startPos;
                    int endPos = _horizontalScrollAnimation.endPos;
                    _horizontalScroll = startPos + (endPos - startPos) * moveProgress;
                }
                Resize();
            }

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
                        it.second->GetX() - _horizontalScroll,
                        it.second->GetY() - _verticalScroll,
                        it.second->GetX() - _horizontalScroll + it.second->GetWidth(),
                        it.second->GetY() - _verticalScroll + it.second->GetHeight()
                    ),
                    it.second->GetOpacity()
                );
            }
        }

        void _OnResize(int width, int height)
        {
            // Calculate item sizes and positions
            int maxRightEdge = 0;
            int maxBottomEdge = 0;
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
                newPosX += _margins.left;
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
                newPosY += _margins.top;

                item->SetPosition(newPosX, newPosY);
                item->Resize(item->GetWidth(), item->GetHeight());

                if (newPosX + item->GetWidth() > maxRightEdge)
                    maxRightEdge = newPosX + item->GetWidth();
                if (newPosY + item->GetHeight() > maxBottomEdge)
                    maxBottomEdge = newPosY + item->GetHeight();
            }
            _contentWidth = maxRightEdge + _margins.right;
            _contentHeight = maxBottomEdge + _margins.bottom;
        }

        Base* _OnMouseMove(int x, int y)
        {
            std::vector<Base*> hoveredComponents;

            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            Base* handledItem = nullptr;
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (adjX >= item->GetX() && adjX < item->GetX() + item->GetWidth() &&
                    adjY >= item->GetY() && adjY < item->GetY() + item->GetHeight())
                {
                    if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                    {
                        if (handledItem == nullptr)
                            handledItem = item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
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
                                handledItem = item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
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
                if (adjX >= item->GetX() && adjX < item->GetX() + item->GetWidth() &&
                    adjY >= item->GetY() && adjY < item->GetY() + item->GetHeight())
                {
                    if (!item->GetMouseInside())
                    {
                        item->OnMouseEnter();
                    }
                    item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
                }
                else if (item->GetMouseInside())
                {
                    if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                    {
                        item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
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
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnLeftPressed(adjX - item->GetX(), adjY - item->GetY());
                }
            }
            return this;
        }

        Base* _OnLeftReleased(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnLeftReleased(adjX - item->GetX(), adjY - item->GetY());
                }
            }
            return this;
        }

        Base* _OnRightPressed(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnRightPressed(adjX - item->GetX(), adjY - item->GetY());
                }
            }
            return this;
        }

        Base* _OnRightReleased(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    return item->OnRightReleased(adjX - item->GetX(), adjY - item->GetY());
                }
            }
            return this;
        }

        Base* _OnWheelUp(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            Base* target = nullptr;
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    target = item->OnWheelUp(adjX - item->GetX(), adjY - item->GetY());
                    break;
                }
            }

            if (target == nullptr)
            {
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (_horizontalScrollable)
                    {
                        ScrollHorizontally(HorizontalScroll() - _scrollStepSize);
                        return this;
                    }
                }
                else
                {
                    if (_verticalScrollable)
                    {
                        ScrollVertically(VerticalScroll() - _scrollStepSize);
                        return this;
                    }
                }
            }
            return nullptr;
        }

        Base* _OnWheelDown(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            Base* target = nullptr;
            for (auto& item : _items)
            {
                if (!item->GetVisible()) continue;

                if (item->GetMouseInside())
                {
                    target = item->OnWheelDown(adjX - item->GetX(), adjY - item->GetY());
                    break;
                }
            }

            if (target == nullptr)
            {
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (_horizontalScrollable)
                    {
                        ScrollHorizontally(HorizontalScroll() + _scrollStepSize);
                        return this;
                    }
                }
                else
                {
                    if (_verticalScrollable)
                    {
                        ScrollVertically(VerticalScroll() + _scrollStepSize);
                        return this;
                    }
                }
            }
            return nullptr;
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
        std::vector<bool> _owned;
        std::vector<Base*> _selectableItems;

        // Margins
        RECT _margins = { 0, 0, 0, 0 };

        // Scrolling
        struct _ScrollAnimation
        {
            bool inProgress = false;
            TimePoint startTime = 0;
            Duration duration = 0;
            int startPos = 0;
            int endPos = 0;
            std::function<float(float)> progressFunction;
        };

        int _contentWidth = 0;
        int _contentHeight = 0;
        int _horizontalScroll = 0;
        int _verticalScroll = 0;
        int _scrollStepSize = 100;
        _ScrollAnimation _verticalScrollAnimation = {};
        _ScrollAnimation _horizontalScrollAnimation = {};
        bool _horizontalScrollable = false;
        bool _verticalScrollable = false;

    public:
        Panel()
        {
            // By default allow iterating nested components
            SetTabIndex(0);
        }
        ~Panel()
        {
            ClearItems();
        }
        Panel(Panel&&) = delete;
        Panel& operator=(Panel&&) = delete;
        Panel(const Panel&) = delete;
        Panel& operator=(const Panel&) = delete;

        void AddItem(Base* item, bool transferOwnership = false)
        {
            _items.push_back(item);
            _owned.push_back(transferOwnership);
            ReindexTabOrder();
        }

        void RemoveItem(Base* item)
        {
            for (int i = 0; i < _items.size(); i++)
            {
                if (_items[i] == item)
                {
                    if (_owned[i])
                        delete _items[i];
                    _items.erase(_items.begin() + i);
                    return;
                }
            }
        }

        void RemoveItem(int index)
        {
            if (_owned[index])
                delete _items[index];
            _items.erase(_items.begin() + index);
            _owned.erase(_owned.begin() + index);
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
            for (int i = 0; i < _items.size(); i++)
            {
                if (_owned[i])
                {
                    delete _items[i];
                }
            }
            _items.clear();
            _owned.clear();
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

        RECT GetMargins() const
        {
            return _margins;
        }

        void SetMargins(RECT margins)
        {
            if (_margins != margins)
            {
                _margins = margins;
                Resize();
            }
        }

        // Scrolling

        bool VerticalScrollable() const
        {
            return _verticalScrollable;
        }

        void VerticalScrollable(bool scrollable)
        {
            _verticalScrollable = scrollable;
        }

        bool HorizontalScrollable() const
        {
            return _horizontalScrollable;
        }

        void HorizontalScrollable(bool scrollable)
        {
            _horizontalScrollable = scrollable;
        }

        int MaxVerticalScroll() const
        {
            int maxScroll = _contentHeight - GetHeight();
            if (maxScroll < 0)
                maxScroll = 0;
            return maxScroll;
        }

        int MaxHorizontalScroll() const
        {
            int maxScroll = _contentWidth - GetWidth();
            if (maxScroll < 0)
                maxScroll = 0;
            return maxScroll;
        }

        int VerticalScroll() const
        {
            if (_verticalScrollAnimation.inProgress)
                return _verticalScrollAnimation.endPos;
            else
                return _verticalScroll;
        }

        void VerticalScroll(int position)
        {
            if (!_verticalScrollable)
                return;

            _verticalScroll = position;
            int maxScroll = MaxVerticalScroll();
            if (_verticalScroll > maxScroll)
                _verticalScroll = maxScroll;
            else if (_verticalScroll < 0)
                _verticalScroll = 0;
        }

        int HorizontalScroll() const
        {
            if (_horizontalScrollAnimation.inProgress)
                return _horizontalScrollAnimation.endPos;
            else
                return _horizontalScroll;
        }

        void HorizontalScroll(int position)
        {
            if (!_horizontalScrollable)
                return;

            _horizontalScroll = position;
            int maxScroll = MaxHorizontalScroll();
            if (_horizontalScroll > maxScroll)
                _horizontalScroll = maxScroll;
            else if (_horizontalScroll < 0)
                _horizontalScroll = 0;
        }

        int ScrollStepSize() const
        {
            return _scrollStepSize;
        }

        void ScrollStepSize(int pixels)
        {
            _scrollStepSize = pixels;
        }

        void ScrollVertically(int to, Duration scrollDuration = Duration(150, MILLISECONDS), std::function<float(float)> progressFunction = std::function<float(float)>())
        {
            if (to < 0)
                to = 0;
            else if (to > MaxVerticalScroll())
                to = MaxVerticalScroll();

            _verticalScrollAnimation.inProgress = true;
            _verticalScrollAnimation.startTime = ztime::Main();
            _verticalScrollAnimation.duration = scrollDuration;
            _verticalScrollAnimation.startPos = _verticalScroll;
            _verticalScrollAnimation.endPos = to;
            _verticalScrollAnimation.progressFunction = progressFunction;
        }

        void ScrollHorizontally(int to, Duration scrollDuration = Duration(150, MILLISECONDS), std::function<float(float)> progressFunction = std::function<float(float)>())
        {
            if (to < 0)
                to = 0;
            else if (to > MaxHorizontalScroll())
                to = MaxHorizontalScroll();

            _horizontalScrollAnimation.inProgress = true;
            _horizontalScrollAnimation.startTime = ztime::Main();
            _horizontalScrollAnimation.duration = scrollDuration;
            _horizontalScrollAnimation.startPos = _horizontalScroll;
            _horizontalScrollAnimation.endPos = to;
            _horizontalScrollAnimation.progressFunction = progressFunction;
        }
    };
}