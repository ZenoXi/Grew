#pragma once

#include "ComponentBase.h"
#include "GameTime.h"

#include <algorithm>
#include <functional>

namespace zcom
{
    class PROP_Shadow : public Property
    {
        void _CopyFields(const PROP_Shadow& other)
        {
            valid = other.valid;
            offsetX = other.offsetX;
            offsetY = other.offsetY;
            blurStandardDeviation = other.blurStandardDeviation;
            color = other.color;
        }
    public:
        static std::string _NAME_() { return "shadow"; }

        PROP_Shadow() {}
        PROP_Shadow(const PROP_Shadow& other)
        {
            _CopyFields(other);
        }
        PROP_Shadow& operator=(const PROP_Shadow& other)
        {
            _CopyFields(other);
            return *this;
        }

        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float blurStandardDeviation = 3.0f;
        D2D1_COLOR_F color = D2D1::ColorF(0, 0.75f);
    };

    class Panel : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            if (_verticalScrollBar.hangDuration == 0)
            {
                if (MaxVerticalScroll() != 0 && !_verticalScrollBar.visible)
                {
                    _verticalScrollBar.visible = true;
                    InvokeRedraw();
                }
                else if (MaxVerticalScroll() == 0 && _verticalScrollBar.visible)
                {
                    _verticalScrollBar.visible = false;
                    InvokeRedraw();
                }
            }
            if (_horizontalScrollBar.hangDuration == 0)
            {
                if (MaxHorizontalScroll() != 0 && !_horizontalScrollBar.visible)
                {
                    _horizontalScrollBar.visible = true;
                    InvokeRedraw();
                }
                else if (MaxHorizontalScroll() == 0 && _horizontalScrollBar.visible)
                {
                    _horizontalScrollBar.visible = false;
                    InvokeRedraw();
                }
            }

            // Fade vertical scrollbar
            if (_verticalScrollBar.visible)
            {
                if (_verticalScrollBar.hangDuration == 0)
                {
                    if (_verticalScrollBar.opacity != 1.0f)
                    {
                        _verticalScrollBar.opacity = 1.0f;
                        InvokeRedraw();
                    }
                }
                else
                {
                    Duration timeElapsed = ztime::Main() - _verticalScrollBar.showTime;
                    if (timeElapsed > _verticalScrollBar.hangDuration)
                    {
                        timeElapsed -= _verticalScrollBar.hangDuration;
                        if (timeElapsed > _verticalScrollBar.fadeDuration)
                        {
                            _verticalScrollBar.visible = false;
                            _verticalScrollBar.opacity = 0.0f;
                        }
                        else
                        {
                            float fadeProgress = timeElapsed.GetDuration() / (float)_verticalScrollBar.fadeDuration.GetDuration();
                            _verticalScrollBar.opacity = 1.0f - powf(fadeProgress, 0.5f);
                        }
                        InvokeRedraw();
                    }
                    else
                    {
                        _verticalScrollBar.opacity = 1.0f;
                    }
                }
            }
            // Fade horizontal scrollbar
            if (_horizontalScrollBar.visible)
            {
                if (_horizontalScrollBar.hangDuration == 0)
                {
                    if (_horizontalScrollBar.opacity != 1.0f)
                    {
                        _horizontalScrollBar.opacity = 1.0f;
                        InvokeRedraw();
                    }
                }
                else
                {
                    Duration timeElapsed = ztime::Main() - _horizontalScrollBar.showTime;
                    if (timeElapsed > _horizontalScrollBar.hangDuration)
                    {
                        timeElapsed -= _horizontalScrollBar.hangDuration;
                        if (timeElapsed > _horizontalScrollBar.fadeDuration)
                        {
                            _horizontalScrollBar.visible = false;
                            _horizontalScrollBar.opacity = 0.0f;
                        }
                        else
                        {
                            float fadeProgress = timeElapsed.GetDuration() / (float)_horizontalScrollBar.fadeDuration.GetDuration();
                            _horizontalScrollBar.opacity = 1.0f - powf(fadeProgress, 0.5f);
                        }
                        InvokeRedraw();
                    }
                    else
                    {
                        _horizontalScrollBar.opacity = 1.0f;
                    }
                }
            }

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
                _RecalculateLayout(GetWidth(), GetHeight());
                OnMouseMove(GetMousePosX(), GetMousePosY());
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
                _RecalculateLayout(GetWidth(), GetHeight());
                OnMouseMove(GetMousePosX(), GetMousePosY());
            }

            for (auto item : _items)
            {
                item.item->Update();
            }
        }

        bool _Redraw()
        {
            for (auto& item : _items)
            {
                if (item.item->Redraw())
                {
                    return true;
                }
            }
            return false;
        }

        void _OnDraw(Graphics g)
        {
            // Recalculate layout if child layouts changed
            //if (_layoutChanged)
            //{
            //    _RecalculateLayout(GetWidth(), GetHeight());
            //    OnMouseMove(GetMousePosX(), GetMousePosY());
            //    _layoutChanged = false;
            //}

            // Get bitmaps of all items
            std::list<std::pair<ID2D1Bitmap*, Item>> bitmaps;
            for (auto& item : _items)
            {
                // Order by z-index
                auto it = bitmaps.rbegin();
                for (; it != bitmaps.rend(); it++)
                {
                    if (item.item->GetZIndex() >= it->second.item->GetZIndex())
                    {
                        break;
                    }
                }
                if (item.item->Redraw())
                    item.item->Draw(g);
                bitmaps.insert(it.base(), { item.item->Image(), item });
            }

            // Draw the bitmaps
            for (auto& it : bitmaps)
            {
                if (it.second.item->GetOpacity() <= 0.0f || !it.second.item->GetVisible())
                    continue;

                // Draw shadow
                auto prop = it.second.item->GetProperty<PROP_Shadow>();
                if (prop.valid)
                {
                    ID2D1Effect* shadowEffect = nullptr;
                    g.target->CreateEffect(CLSID_D2D1Shadow, &shadowEffect);
                    shadowEffect->SetInput(0, it.first);
                    shadowEffect->SetValue(D2D1_SHADOW_PROP_COLOR, D2D1::Vector4F(prop.color.r, prop.color.g, prop.color.b, prop.color.a));
                    shadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, prop.blurStandardDeviation);

                    if (it.second.item->GetOpacity() < 1.0f)
                    {
#ifdef CLSID_D2D1Opacity
                        ID2D1Effect* opacityEffect = nullptr;
                        g.target->CreateEffect(CLSID_D2D1Opacity, &opacityEffect);
                        opacityEffect->SetValue(D2D1_OPACITY_PROP_OPACITY, it.second.item->GetOpacity());

                        ID2D1Effect* compositeEffect = nullptr;
                        g.target->CreateEffect(CLSID_D2D1Composite, &compositeEffect);
                        compositeEffect->SetInputEffect(0, shadowEffect);
                        compositeEffect->SetInputEffect(1, opacityEffect);

                        g.target->DrawImage(compositeEffect, D2D1::Point2F(it.second.item->GetX() + prop.offsetX, it.second.item->GetY() + prop.offsetY));
                        compositeEffect->Release();
                        opacityEffect->Release();
#else
                        // Draw to separate render target and use 'DrawBitmap' with opacity
                        ID2D1Image* stash = nullptr;
                        ID2D1Bitmap1* contentBitmap = nullptr;
                        g.target->CreateBitmap(
                            D2D1::SizeU(GetWidth(), GetHeight()),
                            nullptr,
                            0,
                            D2D1::BitmapProperties1(
                                D2D1_BITMAP_OPTIONS_TARGET,
                                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
                            ),
                            &contentBitmap
                        );

                        g.target->GetTarget(&stash);
                        g.target->SetTarget(contentBitmap);
                        g.target->Clear();
                        g.target->DrawImage(shadowEffect, D2D1::Point2F(it.second.item->GetX() + prop.offsetX, it.second.item->GetY() + prop.offsetY));
                        g.target->SetTarget(stash);
                        stash->Release();
                        g.target->DrawBitmap(contentBitmap, (const D2D1_RECT_F*)0, it.second.item->GetOpacity());

                        contentBitmap->Release();
#endif
                    }
                    else
                    {
                        g.target->DrawImage(shadowEffect, D2D1::Point2F(it.second.item->GetX() + prop.offsetX, it.second.item->GetY() + prop.offsetY));
                    }
                    shadowEffect->Release();
                }

                g.target->DrawBitmap(
                    it.first,
                    D2D1::RectF(
                        it.second.item->GetX() - _horizontalScroll,
                        it.second.item->GetY() - _verticalScroll,
                        it.second.item->GetX() - _horizontalScroll + it.second.item->GetWidth(),
                        it.second.item->GetY() - _verticalScroll + it.second.item->GetHeight()
                    ),
                    it.second.item->GetOpacity()
                );
            }

            // Draw vertical scrollbar
            if (MaxVerticalScroll() > 0 && _verticalScrollBar.visible)
            {
                float lengthOffset = 10.0f;
                float widthOffset = 5.0f;
                float barWidth = 5.0f;
                float heightToContentRatio = GetHeight() / (float)_contentHeight;
                float scrollBarMaxLength = GetHeight() - lengthOffset * 2;
                float scrollBarLength = scrollBarMaxLength * heightToContentRatio;
                float scrollBarPosition = (scrollBarMaxLength - scrollBarLength) * (_verticalScroll / (float)MaxVerticalScroll());

                D2D1_POINT_2F top = { GetWidth() - widthOffset, lengthOffset + scrollBarPosition };
                D2D1_POINT_2F bottom = { top.x, top.y + scrollBarLength };

                float opacity = 0.5f * _verticalScrollBar.opacity;
                ID2D1SolidColorBrush* brush = nullptr;
                g.target->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.7f, 0.7f, opacity), &brush);
                ID2D1StrokeStyle1* style = nullptr;
                g.factory->CreateStrokeStyle
                (
                    D2D1::StrokeStyleProperties1(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND),
                    nullptr,
                    0,
                    &style
                );
                g.target->DrawLine(top, bottom, brush, barWidth, style);
                style->Release();
                brush->Release();
            }
            // Draw horizontal scrollbar
            if (MaxHorizontalScroll() > 0 && _horizontalScrollBar.visible)
            {
                float lengthOffset = 10.0f;
                float widthOffset = 5.0f;
                float barWidth = 5.0f;
                float widthToContentRatio = GetWidth() / (float)_contentWidth;
                float scrollBarMaxLength = GetWidth() - lengthOffset * 2;
                float scrollBarLength = scrollBarMaxLength * widthToContentRatio;
                float scrollBarPosition = (scrollBarMaxLength - scrollBarLength) * (_horizontalScroll / (float)MaxHorizontalScroll());

                D2D1_POINT_2F left = { lengthOffset + scrollBarPosition, GetHeight() - widthOffset };
                D2D1_POINT_2F right = { left.x + scrollBarLength, left.y };

                float opacity = 0.5f * _horizontalScrollBar.opacity;
                ID2D1SolidColorBrush* brush = nullptr;
                g.target->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.7f, 0.7f, opacity), &brush);
                ID2D1StrokeStyle1* style = nullptr;
                g.factory->CreateStrokeStyle
                (
                    D2D1::StrokeStyleProperties1(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND),
                    nullptr,
                    0,
                    &style
                );
                g.target->DrawLine(left, right, brush, barWidth, style);
                style->Release();
                brush->Release();
            }
        }

        void _OnResize(int width, int height)
        {
            _RecalculateLayout(width, height);
        }

        void _OnScreenPosChange(int x, int y)
        {
            _SetScreenPositions();
        }

        EventTargets _OnMouseMove(int x, int y, bool duplicate)
        {
            std::vector<Base*> hoveredComponents;

            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            //Base* handledItem = nullptr;
            bool itemHandled = false;
            EventTargets targets;
            for (auto& _item : _items)
            {
                Base* item = _item.item;

                if (!item->GetVisible()) continue;

                if (adjX >= item->GetX() && adjX < item->GetX() + item->GetWidth() &&
                    adjY >= item->GetY() && adjY < item->GetY() + item->GetHeight())
                {
                    if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                    {
                        if (!itemHandled)
                        {
                            targets = item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
                            itemHandled = true;
                        }
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

                            if (!itemHandled)
                            {
                                targets = item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
                                itemHandled = true;
                            }
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
            if (itemHandled)
                return std::move(targets.Add(this, x, y));

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
                return topmost->OnMouseMove(adjX - topmost->GetX(), adjY - topmost->GetY()).Add(this, x, y);
            }

            return std::move(targets.Add(this, x, y));

            //for (auto& _item : _items)
            //{
            //    Base* item = _item.item;

            //    if (adjX >= item->GetX() && adjX < item->GetX() + item->GetWidth() &&
            //        adjY >= item->GetY() && adjY < item->GetY() + item->GetHeight())
            //    {
            //        if (!item->GetMouseInside())
            //        {
            //            item->OnMouseEnter();
            //        }
            //        item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
            //    }
            //    else if (item->GetMouseInside())
            //    {
            //        if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
            //        {
            //            item->OnMouseMove(adjX - item->GetX(), adjY - item->GetY());
            //        }
            //        else
            //        {
            //            item->OnMouseLeave();
            //        }
            //    }
            //}
        }

        void _OnMouseLeave()
        {
            //std::cout << "Mouse leave\n";
            for (auto& item : _items)
            {
                if (item.item->GetMouseInside())
                {
                    item.item->OnMouseLeave();
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
                if (item.item->GetMouseInsideArea())
                {
                    item.item->OnMouseLeaveArea();
                }
            }
        }

        void _OnMouseEnterArea()
        {

            //std::cout << "Mouse enter\n";
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            for (auto& item : _items)
            {
                if (!item.item->GetVisible()) continue;

                if (item.item->GetMouseInside())
                {
                    return item.item->OnLeftPressed(adjX - item.item->GetX(), adjY - item.item->GetY()).Add(this, x, y);
                }
            }
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnLeftReleased(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            EventTargets targets;
            for (auto& item : _items)
            {
                if (!item.item->GetVisible()) continue;

                if (item.item->GetMouseInside())
                {
                    targets = item.item->OnLeftReleased(adjX - item.item->GetX(), adjY - item.item->GetY()).Add(this, x, y);
                    break;
                }
            }
            OnMouseMove(GetMousePosX(), GetMousePosY());
            if (targets.Empty())
                targets.Add(this, x, y);
            return targets;
        }

        EventTargets _OnRightPressed(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            for (auto& item : _items)
            {
                if (!item.item->GetVisible()) continue;

                if (item.item->GetMouseInside())
                {
                    return item.item->OnRightPressed(adjX - item.item->GetX(), adjY - item.item->GetY()).Add(this, x, y);
                }
            }
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnRightReleased(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            EventTargets targets;
            for (auto& item : _items)
            {
                if (!item.item->GetVisible()) continue;

                if (item.item->GetMouseInside())
                {
                    targets = item.item->OnRightReleased(adjX - item.item->GetX(), adjY - item.item->GetY()).Add(this, x, y);
                    break;
                }
            }
            OnMouseMove(GetMousePosX(), GetMousePosY());
            if (targets.Empty())
                targets.Add(this, x, y);
            return targets;
        }

        EventTargets _OnWheelUp(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            EventTargets targets;
            for (auto& item : _items)
            {
                if (!item.item->GetVisible()) continue;

                if (item.item->GetMouseInside())
                {
                    targets = item.item->OnWheelUp(adjX - item.item->GetX(), adjY - item.item->GetY());
                    break;
                }
            }

            if (targets.Empty())
            {
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (_horizontalScrollable)
                    {
                        ScrollHorizontally(HorizontalScroll() - _scrollStepSize);
                        return EventTargets().Add(this, x, y);
                    }
                }
                else
                {
                    if (_verticalScrollable)
                    {
                        ScrollVertically(VerticalScroll() - _scrollStepSize);
                        return EventTargets().Add(this, x, y);
                    }
                }
            }
            return EventTargets();
        }

        EventTargets _OnWheelDown(int x, int y)
        {
            // Adjust coordinates for scroll
            int adjX = x + _horizontalScroll;
            int adjY = y + _verticalScroll;

            EventTargets targets;
            for (auto& item : _items)
            {
                if (!item.item->GetVisible()) continue;

                if (item.item->GetMouseInside())
                {
                    targets = item.item->OnWheelDown(adjX - item.item->GetX(), adjY - item.item->GetY());
                    break;
                }
            }

            if (targets.Empty())
            {
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (_horizontalScrollable)
                    {
                        ScrollHorizontally(HorizontalScroll() + _scrollStepSize);
                        return EventTargets().Add(this, x, y);
                    }
                }
                else
                {
                    if (_verticalScrollable)
                    {
                        ScrollVertically(VerticalScroll() + _scrollStepSize);
                        return EventTargets().Add(this, x, y);
                    }
                }
            }
            return EventTargets();
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
                children.push_back(item.item);
            }
            return children;
        }

        std::list<Base*> GetAllChildren()
        {
            std::list<Base*> children;
            for (auto& item : _items)
            {
                children.push_back(item.item);
                auto itemChildren = item.item->GetAllChildren();
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
                if (Selected())
                    return nullptr;
                else
                    return this;
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

        const char* GetName() const { return Name(); }
        static const char* Name() { return "panel"; }
#pragma endregion

    protected:
        virtual void _RecalculateLayout(int width, int height)
        {
            // Calculate item sizes and positions
            int maxRightEdge = 0;
            int maxBottomEdge = 0;
            for (auto& _item : _items)
            {
                Base* item = _item.item;

                int newWidth = (int)std::round(GetWidth() * item->GetParentWidthPercent()) + item->GetBaseWidth();
                int newHeight = (int)std::round(GetHeight() * item->GetParentHeightPercent()) + item->GetBaseHeight();
                if (newWidth < 1)
                    newWidth = 1;
                if (newHeight < 1)
                    newHeight = 1;

                int newPosX = 0;
                if (item->GetHorizontalAlignment() == Alignment::START)
                {
                    newPosX += std::round((GetWidth() - newWidth) * item->GetHorizontalOffsetPercent());
                }
                else if (item->GetHorizontalAlignment() == Alignment::CENTER)
                {
                    newPosX = (GetWidth() - newWidth) / 2;
                }
                else if (item->GetHorizontalAlignment() == Alignment::END)
                {
                    newPosX = GetWidth() - newWidth;
                    newPosX -= std::round((GetWidth() - newWidth) * item->GetHorizontalOffsetPercent());
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
                    newPosY += std::round((GetHeight() - newHeight) * item->GetVerticalOffsetPercent());
                }
                else if (item->GetVerticalAlignment() == Alignment::CENTER)
                {
                    newPosY = (GetHeight() - newHeight) / 2;
                }
                else if (item->GetVerticalAlignment() == Alignment::END)
                {
                    newPosY = GetHeight() - newHeight;
                    newPosY -= std::round((GetHeight() - newHeight) * item->GetVerticalOffsetPercent());
                }
                newPosY += item->GetVerticalOffsetPixels();
                newPosY += _margins.top;

                item->SetPosition(newPosX, newPosY);
                item->Resize(newWidth, newHeight);

                if (newPosX + newWidth > maxRightEdge)
                    maxRightEdge = newPosX + newWidth;
                if (newPosY + newHeight > maxBottomEdge)
                    maxBottomEdge = newPosY + newHeight;
            }
            _contentWidth = maxRightEdge + _margins.right;
            _contentHeight = maxBottomEdge + _margins.bottom;

            if (_verticalScroll > MaxVerticalScroll())
                _verticalScroll = MaxVerticalScroll();
            if (_horizontalScroll > MaxHorizontalScroll())
                _horizontalScroll = MaxHorizontalScroll();

            _SetScreenPositions();
            InvokeRedraw();
        }

        void _SetScreenPositions()
        {
            for (auto& _item : _items)
            {
                Base* item = _item.item;
                item->SetScreenPosition(GetScreenX() + item->GetX() - _horizontalScroll, GetScreenY() + item->GetY() - _verticalScroll);
            }
        }

        struct Item
        {
            Base* item;
            bool owned;
            bool hasShadow;
            //bool layoutChanged;
        };
        std::vector<Item> _items;
    private:
        //std::vector<Base*> _items;
        //std::vector<bool> _owned;
        //std::vector<bool> _hasShadow;
        std::vector<Base*> _selectableItems;

        // Margins
        RECT _margins = { 0, 0, 0, 0 };

        // Auto child resize
        //bool _layoutChanged = false;
        bool _deferUpdates = false;
        bool _updatesDeferred = false;

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
        struct _ScrollBar
        {
            bool alwaysVisible = false;
            bool visibleOnScroll = true;
            bool interactable = false;
            bool visible = false;
            TimePoint showTime = 0;
            Duration hangDuration = Duration(1, SECONDS);
            Duration fadeDuration = Duration(150, MILLISECONDS);
            float opacity = 1.0f;
        };
        int _contentWidth = 0;
        int _contentHeight = 0;
        int _horizontalScroll = 0;
        int _verticalScroll = 0;
        int _scrollStepSize = 100;
        _ScrollAnimation _horizontalScrollAnimation = {};
        _ScrollAnimation _verticalScrollAnimation = {};
        _ScrollBar _horizontalScrollBar = {};
        _ScrollBar _verticalScrollBar = {};
        bool _horizontalScrollable = false;
        bool _verticalScrollable = false;

    protected:
        friend class Scene;
        friend class Base;
        Panel(Scene* scene) : Base(scene) {}
        void Init()
        {
            // By default allow iterating nested components
            SetTabIndex(0);
        }
    public:
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
            _items.push_back({ item, transferOwnership, false });
            item->AddOnLayoutChanged([&, item]()
            {
                if (_deferUpdates)
                {
                    _updatesDeferred = true;
                    return;
                }
                _RecalculateLayout(GetWidth(), GetHeight());
                if (GetMouseInside())
                    OnMouseMove(GetMousePosX(), GetMousePosY());
            }, { this, "" });

            ReindexTabOrder();
            if (_deferUpdates)
            {
                _updatesDeferred = true;
                return;
            }
            _RecalculateLayout(GetWidth(), GetHeight());
            if (GetMouseInside())
                OnMouseMove(GetMousePosX(), GetMousePosY());
        }

        void RemoveItem(Base* item)
        {
            for (int i = 0; i < _items.size(); i++)
            {
                if (_items[i].item == item)
                {
                    _items[i].item->RemoveOnLayoutChanged({ this, "" });
                    if (_items[i].owned)
                        delete _items[i].item;
                    _items.erase(_items.begin() + i);
                    ReindexTabOrder();
                    if (_deferUpdates)
                    {
                        _updatesDeferred = true;
                        return;
                    }
                    _RecalculateLayout(GetWidth(), GetHeight());
                    if (GetMouseInsideArea())
                        OnMouseMove(GetMousePosX(), GetMousePosY());
                    return;
                }
            }
        }

        void RemoveItem(int index)
        {
            _items[index].item->RemoveOnLayoutChanged({ this, "" });
            if (_items[index].owned)
                delete _items[index].item;
            _items.erase(_items.begin() + index);
            ReindexTabOrder();
            if (_deferUpdates)
            {
                _updatesDeferred = true;
                return;
            }
            _RecalculateLayout(GetWidth(), GetHeight());
            if (GetMouseInside())
                OnMouseMove(GetMousePosX(), GetMousePosY());
        }

        int ItemCount() const
        {
            return _items.size();
        }

        Base* GetItem(int index)
        {
            return _items[index].item;
        }

        void ClearItems()
        {
            for (int i = 0; i < _items.size(); i++)
            {
                _items[i].item->RemoveOnLayoutChanged({ this, "" });
                if (_items[i].owned)
                {
                    delete _items[i].item;
                }
            }
            _items.clear();
            _selectableItems.clear();
            if (_deferUpdates)
            {
                _updatesDeferred = true;
                return;
            }
            _RecalculateLayout(GetWidth(), GetHeight());
            //OnMouseMove(GetMousePosX(), GetMousePosY());
        }

        // Calling this function suppresses layout updates on item add/remove/layout change
        // until 'ResumeLayoutUpdates()' is called, at which point the deferred updates are executed.
        // Useful when doing lots of item manipulation, which causes many layout updates
        // when only 1 is required at the end.
        void DeferLayoutUpdates()
        {
            _deferUpdates = true;
            _updatesDeferred = false;
        }

        // Enables reactive layout updates. Any deferred updates are executed, unless 'executePending' is false.
        void ResumeLayoutUpdates(bool executePending = true)
        {
            _deferUpdates = false;
            if (_updatesDeferred && executePending)
            {
                _RecalculateLayout(GetWidth(), GetHeight());
                if (GetMouseInside())
                    OnMouseMove(GetMousePosX(), GetMousePosY());
            }
            _updatesDeferred = false;
        }

        void ReindexTabOrder()
        {
            _selectableItems.clear();
            for (int i = 0; i < _items.size(); i++)
            {
                if (_items[i].item->GetTabIndex() != -1)
                {
                    _selectableItems.push_back(_items[i].item);
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
                if (_deferUpdates)
                {
                    _updatesDeferred = true;
                    return;
                }
                _RecalculateLayout(GetWidth(), GetHeight());
            }
        }

        int GetContentWidth() const
        {
            return _contentWidth;
        }

        int GetContentHeight() const
        {
            return _contentHeight;
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

        int VisualVerticalScroll() const
        {
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
            _SetScreenPositions();
            InvokeRedraw();
        }

        int HorizontalScroll() const
        {
            if (_horizontalScrollAnimation.inProgress)
                return _horizontalScrollAnimation.endPos;
            else
                return _horizontalScroll;
        }

        int VisualHorizontalScroll() const
        {
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
            InvokeRedraw();
        }

        Duration VerticalScrollbarHangDuration() const
        {
            return _verticalScrollBar.hangDuration;
        }

        void VerticalScrollbarHangDuration(Duration duration)
        {
            _verticalScrollBar.hangDuration = duration;
        }

        Duration HorizontalScrollbarHangDuration() const
        {
            return _horizontalScrollBar.hangDuration;
        }

        void HorizontalScrollbarHangDuration(Duration duration)
        {
            _horizontalScrollBar.hangDuration = duration;
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

            if (_verticalScrollBar.visibleOnScroll)
            {
                _verticalScrollBar.visible = true;
                _verticalScrollBar.showTime = ztime::Main();
            }
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

            if (_horizontalScrollBar.visibleOnScroll)
            {
                _horizontalScrollBar.visible = true;
                _horizontalScrollBar.showTime = ztime::Main();
            }
        }
    };
}