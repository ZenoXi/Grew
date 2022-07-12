#pragma once

#include "Panel.h"
#include "MouseEventHandler.h"
#include "KeyboardEventHandler.h"

#include <vector>

namespace zcom
{
    // Base storage for all components
    class Canvas : public MouseEventHandler, public KeyboardEventHandler
    {
        int _width;
        int _height;

        Event<bool, const EventTargets*> _mouseMoveHandlers;
        Event<bool, const EventTargets*> _leftPressedHandlers;
        Event<bool, const EventTargets*> _leftReleasedHandlers;
        Event<bool, const EventTargets*> _rightPressedHandlers;
        Event<bool, const EventTargets*> _rightReleasedHandlers;
        Event<bool, const EventTargets*> _wheelUpHandlers;
        Event<bool, const EventTargets*> _wheelDownHandlers;

        std::unique_ptr<Panel> _panel;

    public:
        Canvas(std::unique_ptr<Panel> panel, int width, int height) : _panel(std::move(panel)), _width(width), _height(height)
        {
            _panel->SetSize(_width, _height);
            _panel->SetBackgroundColor(D2D1::ColorF(0, 0));
            _panel->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
        }
        ~Canvas() {}
        Canvas(Canvas&&) = delete;
        Canvas& operator=(Canvas&&) = delete;
        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;

        void AddComponent(Base* comp)
        {
            _panel->AddItem(comp);
        }

        void RemoveComponent(Base* comp)
        {
            _panel->RemoveItem(comp);
        }

        void RemoveComponent(int index)
        {
            _panel->RemoveItem(index);
        }

        int ComponentCount() const
        {
            return _panel->ItemCount();
        }

        Base* GetComponent(int index)
        {
            return _panel->GetItem(index);
        }

        void ClearComponents()
        {
            _panel->ClearItems();
        }

        void Update()
        {
            _panel->Update();
        }

        bool Redraw()
        {
            return _panel->Redraw();
        }

        void InvokeRedraw()
        {
            _panel->InvokeRedraw();
        }

        ID2D1Bitmap* Draw(Graphics g)
        {
            return _panel->Draw(g);
        }

        ID2D1Bitmap* Image()
        {
            return _panel->Image();
        }

        void Resize(int width, int height)
        {
            _panel->Resize(width, height);
        }

        int GetWidth() const
        {
            return _panel->GetWidth();
        }

        int GetHeight() const
        {
            return _panel->GetHeight();
        }

        void SetBackgroundColor(D2D1_COLOR_F color)
        {
            _panel->SetBackgroundColor(color);
            //_backgroundColor = color;
        }

        void ClearSelection(Base* exception = nullptr)
        {
            auto allComponents = GetAllItems();
            for (auto& component : allComponents)
            {
                if (component != exception)
                {
                    if (component->Selected())
                    {
                        component->OnDeselected();
                    }
                }
            }
        }

        Base* GetSelectedComponent()
        {
            auto allComponents = GetAllItems();
            for (auto& component : allComponents)
            {
                if (component->Selected())
                {
                    return component;
                }
            }
            //return _selected;
        }

        //
        // MOUSE EVENT MANAGER FUNCTIONS
        //

        bool _OnMouseMove(int x, int y)
        {
            if (!_panel->GetMouseInside())
                _panel->OnMouseEnter();
            if (!_panel->GetMouseInsideArea())
                _panel->OnMouseEnterArea();

            auto targets = _panel->OnMouseMove(x, y);
            targets.Remove(_panel.get());

            bool handled = false;
            _mouseMoveHandlers.Lock();
            for (auto& handler : _mouseMoveHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _mouseMoveHandlers.Unlock();
            return handled;
        }

        void _OnMouseLeave()
        {
            _panel->OnMouseLeave();
            _panel->OnMouseLeaveArea();
        }

        void _OnMouseEnter()
        {

        }

        bool _OnLeftPressed(int x, int y)
        {
            auto targets = _panel->OnLeftPressed(x, y);
            targets.Remove(_panel.get());
            Base* mainTarget = targets.MainTarget();

            // Clear selection
            ClearSelection(mainTarget);

            if (mainTarget != nullptr)
            {
                // Set selection
                if (!mainTarget->Selected() && mainTarget->GetSelectable())
                {
                    mainTarget->OnSelected();
                }
            }

            bool handled = false;
            _leftPressedHandlers.Lock();
            for (auto& handler : _leftPressedHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _leftPressedHandlers.Unlock();
            return handled;
        }

        bool _OnLeftReleased(int x, int y)
        {
            auto targets = _panel->OnLeftReleased(x, y);
            targets.Remove(_panel.get());

            bool handled = false;
            _leftReleasedHandlers.Lock();
            for (auto& handler : _leftReleasedHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _leftReleasedHandlers.Unlock();
            return handled;
        }

        bool _OnRightPressed(int x, int y)
        {
            auto targets = _panel->OnRightPressed(x, y);
            targets.Remove(_panel.get());

            bool handled = false;
            _rightPressedHandlers.Lock();
            for (auto& handler : _rightPressedHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _rightPressedHandlers.Unlock();
            return handled;
        }

        bool _OnRightReleased(int x, int y)
        {
            auto targets = _panel->OnRightReleased(x, y);
            targets.Remove(_panel.get());

            bool handled = false;
            _rightReleasedHandlers.Lock();
            for (auto& handler : _rightReleasedHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _rightReleasedHandlers.Unlock();
            return handled;
        }

        bool _OnWheelUp(int x, int y)
        {
            auto targets = _panel->OnWheelUp(x, y);
            targets.Remove(_panel.get());

            bool handled = false;
            _wheelUpHandlers.Lock();
            for (auto& handler : _wheelUpHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _wheelUpHandlers.Unlock();
            return handled;
        }

        bool _OnWheelDown(int x, int y)
        {
            auto targets = _panel->OnWheelDown(x, y);
            targets.Remove(_panel.get());

            bool handled = false;
            _wheelDownHandlers.Lock();
            for (auto& handler : _wheelDownHandlers)
            {
                if (handler && handler(&targets))
                    handled = true;
            }
            _wheelDownHandlers.Unlock();
            return handled;
        }

        //
        // KEYBOARD EVENT MANAGER FUNCTIONS
        //

        bool _OnKeyDown(BYTE vkCode)
        {
            if (vkCode == VK_TAB)
            {
                // Advance selected element
                Base* item = _panel->IterateTab();
                ClearSelection();
                if (item != nullptr)
                {
                    item->OnSelected();
                }
            }
            return false;
        }

        bool _OnKeyUp(BYTE vkCode)
        {
            return false;
        }

        bool _OnChar(wchar_t ch)
        {
            return false;
        }


        void AddOnMouseMove(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _mouseMoveHandlers.Add(func, info);
        }

        void AddOnLeftPressed(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _leftPressedHandlers.Add(func, info);
        }

        void AddOnLeftReleased(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _leftReleasedHandlers.Add(func, info);
        }

        void AddOnRightPressed(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _rightPressedHandlers.Add(func, info);
        }

        void AddOnRightReleased(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _rightReleasedHandlers.Add(func, info);
        }

        void AddOnWheelUp(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _wheelUpHandlers.Add(func, info);
        }

        void AddOnWheelDown(std::function<bool(const EventTargets*)> func, EventInfo info = { nullptr, "" })
        {
            _wheelDownHandlers.Add(func, info);
        }

        void RemoveOnMouseMove(EventInfo info)
        {
            _mouseMoveHandlers.Remove(info);
        }

        void RemoveOnLeftPressed(EventInfo info)
        {
            _leftPressedHandlers.Remove(info);
        }

        void RemoveOnLeftReleased(EventInfo info)
        {
            _leftReleasedHandlers.Remove(info);
        }

        void RemoveOnRightPressed(EventInfo info)
        {
            _rightPressedHandlers.Remove(info);
        }

        void RemoveOnRightReleased(EventInfo info)
        {
            _rightReleasedHandlers.Remove(info);
        }

        void RemoveOnWheelUp(EventInfo info)
        {
            _wheelUpHandlers.Remove(info);
        }

        void RemoveOnWheelDown(EventInfo info)
        {
            _wheelDownHandlers.Remove(info);
        }

    private:
        // Recursively gets all components in the canvas
        // *Might* (unlikely) cause performance issues if used often
        std::list<Base*> GetAllItems()
        {
            return _panel->GetAllChildren();
        }
    };
}