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

        Event<void, Base*, int, int> _mouseMoveHandlers;
        Event<void, Base*, int, int> _leftPressedHandlers;
        Event<void, Base*, int, int> _leftReleasedHandlers;
        Event<void, Base*, int, int> _rightPressedHandlers;
        Event<void, Base*, int, int> _rightReleasedHandlers;
        Event<void, Base*, int, int> _wheelUpHandlers;
        Event<void, Base*, int, int> _wheelDownHandlers;

        std::unique_ptr<Panel> _panel;

    public:
        Canvas(int width, int height) : _width(width), _height(height)
        {
            _panel = std::make_unique<Panel>();
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

        ID2D1Bitmap* Draw(Graphics g)
        {
            return _panel->Draw(g);
        }

        void Resize(int width, int height)
        {
            _panel->SetSize(width, height);
            _panel->Resize();
        }

        void Resize()
        {
            _panel->Resize();
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
            Base* target = _panel->OnMouseMove(x, y);
            if (target != _panel.get())
            {
                _mouseMoveHandlers.InvokeAll(target, x, y);
            }
        }

        void _OnMouseLeave()
        {
            _panel->OnMouseLeave();
        }

        void _OnMouseEnter()
        {

        }

        void _OnLeftPressed(int x, int y)
        {
            Base* target = _panel->OnLeftPressed(x, y);

            // Clear selection
            ClearSelection(target);

            if (target != _panel.get())
            {
                // Set selection
                if (!target->Selected())
                {
                    target->OnSelected();
                }

                _leftPressedHandlers.InvokeAll(target, x, y);
            }
        }

        void _OnLeftReleased(int x, int y)
        {
            Base* target = _panel->OnLeftReleased(x, y);
            if (target != _panel.get())
            {
                _leftReleasedHandlers.InvokeAll(target, x, y);
            }
        }

        void _OnRightPressed(int x, int y)
        {
            Base* target = _panel->OnRightPressed(x, y);
            if (target != _panel.get())
            {
                _rightPressedHandlers.InvokeAll(target, x, y);
            }
        }

        void _OnRightReleased(int x, int y)
        {
            Base* target = _panel->OnRightReleased(x, y);
            if (target != _panel.get())
            {
                _rightReleasedHandlers.InvokeAll(target, x, y);
            }
        }

        void _OnWheelUp(int x, int y)
        {
            Base* target = _panel->OnWheelUp(x, y);
            if (target != _panel.get())
            {
                _wheelUpHandlers.InvokeAll(target, x, y);
            }
        }

        void _OnWheelDown(int x, int y)
        {
            Base* target = _panel->OnWheelDown(x, y);
            if (target != _panel.get())
            {
                _wheelDownHandlers.InvokeAll(target, x, y);
            }
        }

        //
        // KEYBOARD EVENT MANAGER FUNCTIONS
        //

        void _OnKeyDown(BYTE vkCode)
        {

        }

        void _OnKeyUp(BYTE vkCode)
        {

        }

        void _OnChar(wchar_t ch)
        {
            // Advance selected element
            Base* item = _panel->IterateTab();
            if (item == nullptr)
            {
                ClearSelection();
            }
            else
            {
                item->OnSelected();
            }
        }


        void AddOnMouseMove(std::function<void(Base*, int, int)> func)
        {
            _mouseMoveHandlers.Add(func);
        }

        void AddOnLeftPressed(std::function<void(Base*, int, int)> func)
        {
            _leftPressedHandlers.Add(func);
        }

        void AddOnLeftReleased(std::function<void(Base*, int, int)> func)
        {
            _leftReleasedHandlers.Add(func);
        }

        void AddOnRightPressed(std::function<void(Base*, int, int)> func)
        {
            _rightPressedHandlers.Add(func);
        }

        void AddOnRightReleased(std::function<void(Base*, int, int)> func)
        {
            _rightReleasedHandlers.Add(func);
        }

        void AddOnWheelUp(std::function<void(Base*, int, int)> func)
        {
            _wheelUpHandlers.Add(func);
        }

        void AddOnWheelDown(std::function<void(Base*, int, int)> func)
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