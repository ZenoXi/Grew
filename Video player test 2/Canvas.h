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

        Event<void, const EventTargets*> _mouseMoveHandlers;
        Event<void, const EventTargets*> _leftPressedHandlers;
        Event<void, const EventTargets*> _leftReleasedHandlers;
        Event<void, const EventTargets*> _rightPressedHandlers;
        Event<void, const EventTargets*> _rightReleasedHandlers;
        Event<void, const EventTargets*> _wheelUpHandlers;
        Event<void, const EventTargets*> _wheelDownHandlers;

        std::unique_ptr<Panel> _panel;

    public:
        Canvas(std::unique_ptr<Panel> panel, int width, int height) : _panel(std::move(panel)), _width(width), _height(height)
        {
            _panel->SetSize(_width, _height);
            _panel->SetBackgroundColor(D2D1::ColorF(0, 0));
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

        void _OnMouseMove(int x, int y)
        {
            if (!_panel->GetMouseInside())
                _panel->OnMouseEnter();
            if (!_panel->GetMouseInsideArea())
                _panel->OnMouseEnterArea();

            auto targets = _panel->OnMouseMove(x, y);
            targets.Remove(_panel.get());
            _mouseMoveHandlers.InvokeAll(&targets);
        }

        void _OnMouseLeave()
        {
            _panel->OnMouseLeave();
            _panel->OnMouseLeaveArea();
        }

        void _OnMouseEnter()
        {

        }

        void _OnLeftPressed(int x, int y)
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

            _leftPressedHandlers.InvokeAll(&targets);
        }

        void _OnLeftReleased(int x, int y)
        {
            auto targets = _panel->OnLeftReleased(x, y);
            targets.Remove(_panel.get());
            _leftReleasedHandlers.InvokeAll(&targets);
        }

        void _OnRightPressed(int x, int y)
        {
            auto targets = _panel->OnRightPressed(x, y);
            targets.Remove(_panel.get());
            _rightPressedHandlers.InvokeAll(&targets);
        }

        void _OnRightReleased(int x, int y)
        {
            auto targets = _panel->OnRightReleased(x, y);
            targets.Remove(_panel.get());
            _rightReleasedHandlers.InvokeAll(&targets);
        }

        void _OnWheelUp(int x, int y)
        {
            auto targets = _panel->OnWheelUp(x, y);
            targets.Remove(_panel.get());
            _wheelUpHandlers.InvokeAll(&targets);
        }

        void _OnWheelDown(int x, int y)
        {
            auto targets = _panel->OnWheelDown(x, y);
            targets.Remove(_panel.get());
            _wheelDownHandlers.InvokeAll(&targets);
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


        void AddOnMouseMove(std::function<void(const EventTargets*)> func)
        {
            _mouseMoveHandlers.Add(func);
        }

        void AddOnLeftPressed(std::function<void(const EventTargets*)> func)
        {
            _leftPressedHandlers.Add(func);
        }

        void AddOnLeftReleased(std::function<void(const EventTargets*)> func)
        {
            _leftReleasedHandlers.Add(func);
        }

        void AddOnRightPressed(std::function<void(const EventTargets*)> func)
        {
            _rightPressedHandlers.Add(func);
        }

        void AddOnRightReleased(std::function<void(const EventTargets*)> func)
        {
            _rightReleasedHandlers.Add(func);
        }

        void AddOnWheelUp(std::function<void(const EventTargets*)> func)
        {
            _wheelUpHandlers.Add(func);
        }

        void AddOnWheelDown(std::function<void(const EventTargets*)> func)
        {
            _wheelDownHandlers.Add(func);
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