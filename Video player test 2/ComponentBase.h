#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <unordered_map>

#include "DisplayWindow.h"
#include "Graphics.h"
#include "MouseEventHandler.h"
#include "Event.h"

class Scene;

namespace zcom
{
    enum class Alignment
    {
        START,
        CENTER,
        END
    };

    // Releases the resource and removes the reference
    void SafeFullRelease(IUnknown** res);

    // Releases the resource
    void SafeRelease(IUnknown** res);

    // Base class for component properties.
    // Used to give components properties which can be used by specific objects without adding the property to 'Base'.
    // Derived classes should implement (if necessary):
    //  - Default constructor
    //  - Copy assignment operator
    // MUST IMPLEMENT:
    //  - static std::string _NAME_();
    class Property
    {
    public:
        virtual ~Property() {};
        bool valid;
    };

    class Base;
    // Class that contains all components that handled an event.
    class EventTargets
    {
    public:
        struct Params
        {
            Base* target;
            int x;
            int y;
        };

    private:
        std::vector<Params> _targets;

    public:
        EventTargets()
        {
            // Reserve initial capacity to prevent reallocations in most cases
            _targets.reserve(16);
        }
        EventTargets(EventTargets&& other)
        {
            _targets = std::move(other._targets);
        }
        EventTargets& operator=(EventTargets&& other)
        {
            if (this != &other)
            {
                _targets = std::move(other._targets);
            }
            return *this;
        }
        EventTargets(const EventTargets& other) = delete;
        EventTargets& operator=(const EventTargets& other) = delete;

        EventTargets Add(Base* item, int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min()) &&
        {
            _targets.push_back({ item, x, y });
            return std::move(*this);
        }

        EventTargets& Add(Base* item, int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min()) &
        {
            _targets.push_back({ item, x, y });
            return *this;
        }

        void Remove(Base* item)
        {
            auto it = std::find_if(_targets.begin(), _targets.end(), [item](Params p) { return p.target == item; });
            if (it != _targets.end())
                _targets.erase(it);
        }

        bool Empty() const
        {
            return _targets.empty();
        }

        size_t Size() const
        {
            return _targets.size();
        }

        bool Contains(Base* item) const
        {
            return std::find_if(_targets.begin(), _targets.end(), [item](Params p) { return p.target == item; }) != _targets.end();
        }

        Base* MainTarget() const
        {
            if (!_targets.empty())
                return _targets.front().target;
            else
                return nullptr;
        }

        std::vector<Params> GetTargets() const
        {
            return _targets;
        }
    };

    // The base component class
    class Base
    {
    public:
        // Component creation
        template<class T, typename... Args>
        std::unique_ptr<T> Create(Args&&... args)
        {
            auto uptr = std::unique_ptr<T>(new T(_scene));
            uptr->Init(std::forward<Args>(args)...);
            return uptr;
        }
    protected:
        Scene* _scene = nullptr;
    private:

        ID2D1Bitmap1* _canvas = nullptr;
        bool _redraw = true;

        // Position description
        Alignment _hPosAlign = Alignment::START;
        Alignment _vPosAlign = Alignment::START;
        float _hPosPercentOffset = 0.0f;
        float _vPosPercentOffset = 0.0f;
        int _hPosPixelOffset = 0;
        int _vPosPixelOffset = 0;

        // Size description
        float _hSizeParentPercent = 0.0f;
        float _vSizeParentPercent = 0.0f;
        int _hSize = 0;
        int _vSize = 0;

        // Main
        int _x = 0;
        int _y = 0;
        int _screenX = 0;
        int _screenY = 0;
        int _width = 100;
        int _height = 100;
        float _opacity = 1.0f;
        bool _active = true;
        bool _visible = true;

        // Selection
        bool _selectable = false;
        bool _selected = false;
        int _tabIndex = -1;
        int _zIndex = -1;

        // Border
        bool _borderVisible = false;
        float _borderWidth = 1.0f;
        D2D1_COLOR_F _borderColor = D2D1::ColorF(1.0f, 1.0f, 1.0f);
        D2D1_COLOR_F _selectedBorderColor = D2D1::ColorF(0.0f, 0.5f, 0.8f);

        // Background
        D2D1_COLOR_F _backgroundColor = D2D1::ColorF(0, 0);
        ID2D1Bitmap* _background = nullptr;

        // Cursor
        CursorIcon _cursor = CursorIcon::ARROW;

        // Rounding
        float _cornerRounding = 0.0f;

        // Other properties
        std::unordered_map<std::string, std::unique_ptr<Property>> _properties;

        // Mouse events
        bool _mouseInside = false;
        bool _mouseInsideArea = false;
        bool _mouseLeftClicked = false;
        bool _mouseRightClicked = false;
        int _mousePosX = 0;
        int _mousePosY = 0;
        // Pre default handling
        Event<void, Base*, int, int, bool> _onMouseMove;
        Event<void, Base*> _onMouseEnter;
        Event<void, Base*> _onMouseEnterArea;
        Event<void, Base*> _onMouseLeave;
        Event<void, Base*> _onMouseLeaveArea;
        Event<void, Base*, int, int> _onLeftPressed;
        Event<void, Base*, int, int> _onRightPressed;
        Event<void, Base*, int, int> _onLeftReleased;
        Event<void, Base*, int, int> _onRightReleased;
        Event<void, Base*, int, int> _onWheelUp;
        Event<void, Base*, int, int> _onWheelDown;
        // Post default handling
        Event<void, Base*, std::vector<EventTargets::Params>, int, int, bool> _postMouseMove;
        Event<void, Base*, std::vector<EventTargets::Params>, int, int> _postLeftPressed;
        Event<void, Base*, std::vector<EventTargets::Params>, int, int> _postRightPressed;
        Event<void, Base*, std::vector<EventTargets::Params>, int, int> _postLeftReleased;
        Event<void, Base*, std::vector<EventTargets::Params>, int, int> _postRightReleased;
        Event<void, Base*, std::vector<EventTargets::Params>, int, int> _postWheelUp;
        Event<void, Base*, std::vector<EventTargets::Params>, int, int> _postWheelDown;

        // Layout events
        Event<void> _onLayoutChanged;

    public:
        Base(Scene* scene) : _scene(scene) {}
        virtual ~Base()
        {
            SafeFullRelease((IUnknown**)&_canvas);
        }
        Base(Base&&) = delete;
        Base& operator=(Base&&) = delete;
        Base(const Base&) = delete;
        Base& operator=(const Base&) = delete;

        // Position description
        Alignment GetHorizontalAlignment() const { return _hPosAlign; }
        Alignment GetVerticalAlignment() const { return _vPosAlign; }
        float GetHorizontalOffsetPercent() const { return _hPosPercentOffset; }
        float GetVerticalOffsetPercent() const { return _vPosPercentOffset; }
        int GetHorizontalOffsetPixels() const { return _hPosPixelOffset; }
        int GetVerticalOffsetPixels() const { return _vPosPixelOffset; }

        void SetHorizontalAlignment(Alignment alignment)
        {
            SetAlignment(alignment, GetVerticalAlignment());
        }
        void SetVerticalAlignment(Alignment alignment)
        {
            SetAlignment(GetHorizontalAlignment(), alignment);
        }
        void SetAlignment(Alignment horizontal, Alignment vertical)
        {
            bool changed = false;
            if (_hPosAlign != horizontal || _vPosAlign != vertical)
                changed = true;

            _hPosAlign = horizontal;
            _vPosAlign = vertical;

            if (changed)
                _onLayoutChanged.InvokeAll();
        }
        void SetHorizontalOffsetPercent(float offset)
        {
            SetOffsetPercent(offset, GetVerticalOffsetPercent());
        }
        void SetVerticalOffsetPercent(float offset)
        {
            SetOffsetPercent(GetHorizontalOffsetPercent(), offset);
        }
        void SetOffsetPercent(float horizontal, float vertical)
        {
            bool changed = false;
            if (_hPosPercentOffset != horizontal || _vPosPercentOffset != vertical)
                changed = true;

            _hPosPercentOffset = horizontal;
            _vPosPercentOffset = vertical;

            if (changed)
                _onLayoutChanged.InvokeAll();
        }
        void SetHorizontalOffsetPixels(int offset)
        {
            SetOffsetPixels(offset, GetVerticalOffsetPixels());
        }
        void SetVerticalOffsetPixels(int offset)
        {
            SetOffsetPixels(GetHorizontalOffsetPixels(), offset);
        }
        void SetOffsetPixels(int horizontal, int vertical)
        {
            bool changed = false;
            if (_hPosPixelOffset != horizontal || _vPosPixelOffset != vertical)
                changed = true;

            _hPosPixelOffset = horizontal;
            _vPosPixelOffset = vertical;

            if (changed)
                _onLayoutChanged.InvokeAll();
        }

        // Size description
        float GetParentWidthPercent() const { return _hSizeParentPercent; }
        float GetParentHeightPercent() const { return _vSizeParentPercent; }
        int GetBaseWidth() const { return _hSize; }
        int GetBaseHeight() const { return _vSize; }

        void SetParentWidthPercent(float width)
        {
            SetParentSizePercent(width, GetParentHeightPercent());
        }
        void SetParentHeightPercent(float height)
        {
            SetParentSizePercent(GetParentWidthPercent(), height);
        }
        void SetParentSizePercent(float width, float height)
        {
            bool changed = false;
            if (_hSizeParentPercent != width || _vSizeParentPercent != height)
                changed = true;

            _hSizeParentPercent = width;
            _vSizeParentPercent = height;

            if (changed)
                _onLayoutChanged.InvokeAll();
        }
        void SetBaseWidth(int width)
        {
            SetBaseSize(width, GetBaseHeight());
        }
        void SetBaseHeight(int height)
        {
            SetBaseSize(GetBaseWidth(), height);
        }
        void SetBaseSize(int width, int height)
        {
            bool changed = false;
            if (_hSize != width || _vSize != height)
                changed = true;

            _hSize = width;
            _vSize = height;

            if (changed)
                _onLayoutChanged.InvokeAll();
        }

        // Common
        int GetX() const { return _x; }
        int GetY() const { return _y; }
        int GetScreenX() const { return _screenX; }
        int GetScreenY() const { return _screenY; }
        int GetWidth() const { return _width; }
        int GetHeight() const { return _height; }
        float GetOpacity() const { return _opacity; }
        bool GetActive() const { return _active; }
        bool GetVisible() const { return _visible; }

        void SetX(int x)
        {
            SetPosition(x, _y);
        }
        void SetY(int y)
        {
            SetPosition(_x, y);
        }
        void SetPosition(int x, int y)
        {
            _x = x;
            _y = y;
        }
        void SetScreenX(int x)
        {
            SetScreenPosition(x, _screenY);
        }
        void SetScreenY(int y)
        {
            SetScreenPosition(_screenX, y);
        }
        void SetScreenPosition(int x, int y)
        {
            _screenX = x;
            _screenY = y;
        }
        void SetWidth(int width)
        {
            SetSize(width, _height);
        }
        void SetHeight(int height)
        {
            SetSize(_width, height);
        }
        void SetSize(int width, int height)
        {
            if (width == _width && height == _height)
                return;

            if (width <= 0) width = 1;
            if (height <= 0) height = 1;
            _width = width;
            _height = height;
            SafeFullRelease((IUnknown**)&_canvas);
            _redraw = true;
            //if (_canvas)
            //{
            //    _canvas->Release();
            //    _canvas = nullptr;
            //}
        }
        void SetOpacity(float opacity)
        {
            if (opacity == _opacity)
                return;

            _opacity = opacity;
            _redraw = true;
        }
        void SetActive(bool active)
        {
            if (active == _active)
                return;

            if (!active)
            {
                OnDeselected();
                OnLeftReleased();
                OnRightReleased();
            }
            _active = active;
            _redraw = true;
        }
        void SetVisible(bool visible)
        {
            if (visible == _visible)
                return;

            if (!visible)
            {
                OnDeselected();
                OnLeftReleased();
                OnRightReleased();
            }
            _visible = visible;
            _redraw = true;
        }

        // Selection
        bool GetSelectable() const { return _selectable; }
        bool Selected() const { return _selected; }
        int GetZIndex() const { return _zIndex; }
        int GetTabIndex() const { return _tabIndex; }

        void SetSelectable(bool selectable)
        {
            _selectable = selectable;
        }
        void SetZIndex(int index)
        {
            if (index == _zIndex)
                return;

            _zIndex = index;
            _redraw = true;
        }
        void SetTabIndex(int index)
        {
            _tabIndex = index;
        }

        // Border
        bool GetBorderVisibility() const { return _borderVisible; }
        float GetBorderWidth() const { return _borderWidth; }
        D2D1_COLOR_F GetBorderColor() const { return _borderColor; }
        D2D1_COLOR_F GetSelectedBorderColor() const { return _selectedBorderColor; }

        void SetBorderVisibility(bool visible)
        {
            if (visible == _borderVisible)
                return;

            _borderVisible = visible;
            _redraw = true;
        }
        void SetBorderWidth(float width)
        {
            if (width == _borderWidth)
                return;

            _borderWidth = width;
            _redraw = true;
        }
        void SetBorderColor(D2D1_COLOR_F color)
        {
            if (color == _borderColor)
                return;

            _borderColor = color;
            _redraw = true;
        }
        void SetSelectedBorderColor(D2D1_COLOR_F color)
        {
            if (color == _selectedBorderColor)
                return;

            _selectedBorderColor = color;
            _redraw = true;
        }

        // Background
        D2D1_COLOR_F GetBackgroundColor() const { return _backgroundColor; }
        ID2D1Bitmap* GetBackgroundImage() const { return _background; }

        void SetBackgroundColor(D2D1_COLOR_F color)
        {
            if (color == _backgroundColor)
                return;

            _backgroundColor = color;
            _redraw = true;
        }
        void SetBackgroundImage(ID2D1Bitmap* image)
        {
            if (image == _background)
                return;

            _background = image;
            _redraw = true;
        }

        // Cursor
        CursorIcon GetDefaultCursor() const { return _cursor; }

        void SetDefaultCursor(CursorIcon cursor)
        {
            _cursor = cursor;
        }

        void _ApplyCursor();

        // Corner rounding
        float GetCornerRounding() const { return _cornerRounding; }

        void SetCornerRounding(float rounding)
        {
            if (rounding == _cornerRounding)
                return;

            _cornerRounding = rounding;
            _redraw = true;
        }

        // Other properties
        template<class _Prop>
        void SetProperty(_Prop prop)
        {
            auto entry = _properties.find(_Prop::_NAME_());
            if (entry != _properties.end())
            {
                // The cast only fails when two properties have same names
                *dynamic_cast<_Prop*>(entry->second.get()) = prop;
            }
            else
            {
                auto propPtr = std::make_unique<_Prop>();
                *propPtr = prop;
                _properties.insert({ _Prop::_NAME_(), std::move(propPtr) });
            }

            // Properties might not change the visuals,
            // but optimising for that scenario is unnecessary
            _redraw = true;
        }

        template<class _Prop>
        _Prop GetProperty()
        {
            auto entry = _properties.find(_Prop::_NAME_());
            if (entry != _properties.end())
            {
                // The cast only fails when two properties have same names
                _Prop prop = *dynamic_cast<_Prop*>(entry->second.get());
                prop.valid = true;
                return prop;
            }
            else
            {
                _Prop prop;
                prop.valid = false;
                return prop;
            }
        }

        template<class _Prop>
        void RemoveProperty()
        {
            _properties.erase(_Prop::_NAME_());

            // Properties might not change the visuals,
            // but optimising for that scenario is unnecessary
            _redraw = true;
        }

        // Mouse events
        EventTargets OnMouseMove(int x, int y)
        {
            if (!_active)
                return EventTargets();

            bool duplicateMove = false;
            if (_mousePosX == x && _mousePosY == y)
                duplicateMove = true;

            _mousePosX = x;
            _mousePosY = y;
            _onMouseMove.InvokeAll(this, x, y, duplicateMove);
            auto targets = _OnMouseMove(x, y, duplicateMove);
            _postMouseMove.InvokeAll(this, targets.GetTargets(), x, y, duplicateMove);

            if (targets.Size() == 1 && targets.MainTarget() == this)
            {
                // Set cursor
                _ApplyCursor();
            }
            return targets;
        }
        void OnMouseEnter()
        {
            if (!_active) return;
            if (_mouseInside) return;

            _mouseInside = true;
            _onMouseEnter.InvokeAll(this);
            _OnMouseEnter();
        }
        void OnMouseLeave()
        {
            if (!_active) return;
            if (!_mouseInside) return;

            _mouseInside = false;
            _onMouseLeave.InvokeAll(this);
            _OnMouseLeave();
        }
        void OnMouseEnterArea()
        {
            if (!_active) return;
            if (_mouseInsideArea) return;

            _mouseInsideArea = true;
            _onMouseEnterArea.InvokeAll(this);
            _OnMouseEnterArea();
        }
        void OnMouseLeaveArea()
        {
            if (!_active) return;
            if (!_mouseInsideArea) return;

            _mouseInsideArea = false;
            _onMouseLeaveArea.InvokeAll(this);
            _OnMouseLeaveArea();
        }
        EventTargets OnLeftPressed(int x, int y)
        {
            if (!_active) return EventTargets();
            if (_mouseLeftClicked) return EventTargets();

            _mouseLeftClicked = true;
            _onLeftPressed.InvokeAll(this, x, y);
            auto targets = _OnLeftPressed(x, y);
            _postLeftPressed.InvokeAll(this, targets.GetTargets(), x, y);
            return targets;
        }
        EventTargets OnLeftReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min())
        {
            if (!_active) return EventTargets();
            if (!_mouseLeftClicked) return EventTargets();

            _mouseLeftClicked = false;
            _onLeftReleased.InvokeAll(this, x, y);
            auto targets = _OnLeftReleased(x, y);
            _postLeftReleased.InvokeAll(this, targets.GetTargets(), x, y);
            return targets;
        }
        EventTargets OnRightPressed(int x, int y)
        {
            if (!_active) return EventTargets();
            if (_mouseRightClicked) return EventTargets();

            _mouseRightClicked = true;
            _onRightPressed.InvokeAll(this, x, y);
            auto targets = _OnRightPressed(x, y);
            _postRightPressed.InvokeAll(this, targets.GetTargets(), x, y);
            return targets;
        }
        EventTargets OnRightReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min())
        {
            if (!_active) return EventTargets();
            if (!_mouseRightClicked) return EventTargets();

            _mouseRightClicked = false;
            _onRightReleased.InvokeAll(this, x, y);
            auto targets = _OnRightReleased(x, y);
            _postRightReleased.InvokeAll(this, targets.GetTargets(), x, y);
            return targets;
        }
        EventTargets OnWheelUp(int x, int y)
        {
            if (!_active) return EventTargets();

            _onWheelUp.InvokeAll(this, x, y);
            auto targets = _OnWheelUp(x, y);
            _postWheelUp.InvokeAll(this, targets.GetTargets(), x, y);
            return targets;
        }
        EventTargets OnWheelDown(int x, int y)
        {
            if (!_active) return EventTargets();

            _onWheelDown.InvokeAll(this, x, y);
            auto targets = _OnWheelDown(x, y);
            _postWheelDown.InvokeAll(this, targets.GetTargets(), x, y);
            return targets;
        }
        void OnSelected()
        {
            if (!_active) return;
            if (_selected) return;

            _selected = true;
            _redraw = true;
            _OnSelected();
        }
        void OnDeselected()
        {
            if (!_active) return;
            if (!_selected) return;

            _selected = false;
            _redraw = true;
            _OnDeselected();
        }
    protected:
        virtual EventTargets _OnMouseMove(int x, int y, bool duplicate) { return EventTargets().Add(this, x, y); }
        virtual void _OnMouseEnter() {}
        virtual void _OnMouseLeave() {}
        virtual void _OnMouseEnterArea() {}
        virtual void _OnMouseLeaveArea() {}
        virtual EventTargets _OnLeftPressed(int x, int y) { return EventTargets().Add(this, x, y); }
        virtual EventTargets _OnRightPressed(int x, int y) { return EventTargets().Add(this, x, y); }
        virtual EventTargets _OnLeftReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min()) { return EventTargets().Add(this, x, y); }
        virtual EventTargets _OnRightReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min()) { return EventTargets().Add(this, x, y); }
        virtual EventTargets _OnWheelUp(int x, int y) { return EventTargets(); }
        virtual EventTargets _OnWheelDown(int x, int y) { return EventTargets(); }
        virtual void _OnSelected() {}
        virtual void _OnDeselected() {}
    public:
        bool GetMouseInside() const { return _mouseInside; }
        bool GetMouseInsideArea() const { return _mouseInsideArea; }
        bool GetMouseLeftClicked() const { return _mouseLeftClicked; }
        bool GetMouseRightClicked() const { return _mouseRightClicked; }
        int GetMousePosX() const { return _mousePosX; }
        int GetMousePosY() const { return _mousePosY; }

        void AddOnMouseMove(std::function<void(Base*, int, int, bool)> handler, EventInfo info = { nullptr, "" })
        {
            _onMouseMove.Add(handler, info);
        }
        void AddOnMouseEnter(std::function<void(Base*)> handler, EventInfo info = { nullptr, "" })
        {
            _onMouseEnter.Add(handler, info);
        }
        void AddOnMouseLeave(std::function<void(Base*)> handler, EventInfo info = { nullptr, "" })
        {
            _onMouseLeave.Add(handler, info);
        }
        void AddOnMouseEnterArea(std::function<void(Base*)> handler, EventInfo info = { nullptr, "" })
        {
            _onMouseEnterArea.Add(handler, info);
        }
        void AddOnMouseLeaveArea(std::function<void(Base*)> handler, EventInfo info = { nullptr, "" })
        {
            _onMouseLeaveArea.Add(handler, info);
        }
        void AddOnLeftPressed(std::function<void(Base*, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _onLeftPressed.Add(handler, info);
        }
        void AddOnRightPressed(std::function<void(Base*, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _onRightPressed.Add(handler, info);
        }
        void AddOnLeftReleased(std::function<void(Base*, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _onLeftReleased.Add(handler, info);
        }
        void AddOnRightReleased(std::function<void(Base*, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _onRightReleased.Add(handler, info);
        }
        void AddOnWheelUp(std::function<void(Base*, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _onWheelUp.Add(handler, info);
        }
        void AddOnWheelDown(std::function<void(Base*, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _onWheelDown.Add(handler, info);
        }

        void AddPostMouseMove(std::function<void(Base*, std::vector<EventTargets::Params>, int, int, bool)> handler, EventInfo info = { nullptr, "" })
        {
            _postMouseMove.Add(handler, info);
        }
        void AddPostLeftPressed(std::function<void(Base*, std::vector<EventTargets::Params>, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _postLeftPressed.Add(handler, info);
        }
        void AddPostRightPressed(std::function<void(Base*, std::vector<EventTargets::Params>, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _postRightPressed.Add(handler, info);
        }
        void AddPostLeftReleased(std::function<void(Base*, std::vector<EventTargets::Params>, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _postLeftReleased.Add(handler, info);
        }
        void AddPostRightReleased(std::function<void(Base*, std::vector<EventTargets::Params>, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _postRightReleased.Add(handler, info);
        }
        void AddPostWheelUp(std::function<void(Base*, std::vector<EventTargets::Params>, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _postWheelUp.Add(handler, info);
        }
        void AddPostWheelDown(std::function<void(Base*, std::vector<EventTargets::Params>, int, int)> handler, EventInfo info = { nullptr, "" })
        {
            _postWheelDown.Add(handler, info);
        }

        void RemoveOnMouseMove(EventInfo info = { nullptr, "" })
        {
            _onMouseMove.Remove(info);
        }
        void RemoveOnMouseEnter(EventInfo info = { nullptr, "" })
        {
            _onMouseEnter.Remove(info);
        }
        void RemoveOnMouseLeave(EventInfo info = { nullptr, "" })
        {
            _onMouseLeave.Remove(info);
        }
        void RemoveOnMouseEnterArea(EventInfo info = { nullptr, "" })
        {
            _onMouseEnterArea.Remove(info);
        }
        void RemoveOnMouseLeaveArea(EventInfo info = { nullptr, "" })
        {
            _onMouseLeaveArea.Remove(info);
        }
        void RemoveOnLeftPressed(EventInfo info = { nullptr, "" })
        {
            _onLeftPressed.Remove(info);
        }
        void RemoveOnRightPressed(EventInfo info = { nullptr, "" })
        {
            _onRightPressed.Remove(info);
        }
        void RemoveOnLeftReleased(EventInfo info = { nullptr, "" })
        {
            _onLeftReleased.Remove(info);
        }
        void RemoveOnRightReleased(EventInfo info = { nullptr, "" })
        {
            _onRightReleased.Remove(info);
        }
        void RemoveOnWheelUp(EventInfo info = { nullptr, "" })
        {
            _onWheelUp.Remove(info);
        }
        void RemoveOnWheelDown(EventInfo info = { nullptr, "" })
        {
            _onWheelDown.Remove(info);
        }

        void RemovePostMouseMove(EventInfo info = { nullptr, "" })
        {
            _postMouseMove.Remove(info);
        }
        void RemovePostLeftPressed(EventInfo info = { nullptr, "" })
        {
            _postLeftPressed.Remove(info);
        }
        void RemovePostRightPressed(EventInfo info = { nullptr, "" })
        {
            _postRightPressed.Remove(info);
        }
        void RemovePostLeftReleased(EventInfo info = { nullptr, "" })
        {
            _postLeftReleased.Remove(info);
        }
        void RemovePostRightReleased(EventInfo info = { nullptr, "" })
        {
            _postRightReleased.Remove(info);
        }
        void RemovePostWheelUp(EventInfo info = { nullptr, "" })
        {
            _postWheelUp.Remove(info);
        }
        void RemovePostWheelDown(EventInfo info = { nullptr, "" })
        {
            _postWheelDown.Remove(info);
        }

        // Layout events
        void AddOnLayoutChanged(std::function<void()> handler, EventInfo info = { nullptr, "" })
        {
            _onLayoutChanged.Add(handler, info);
        }

        void RemoveOnLayoutChanged(EventInfo info = { nullptr, "" })
        {
            _onLayoutChanged.Remove(info);
        }

        // Main functions
        void Update()
        {
            if (!_active) return;

            _OnUpdate();
        }

        // If this function returns true, the 'Draw()' function should be called
        // to redraw any visual changes
        bool Redraw()
        {
            return _redraw || !_canvas || _Redraw();
        }

        void InvokeRedraw()
        {
            _redraw = true;
        }

        ID2D1Bitmap* Draw(Graphics g)
        {
            _redraw = false;

            if (!_canvas)
            {
                g.target->CreateBitmap(
                    D2D1::SizeU(_width, _height),
                    nullptr,
                    0,
                    D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET,
                        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
                    ),
                    &_canvas
                );
                g.refs->push_back((IUnknown**)&_canvas);
            }

            // Stash current target
            ID2D1Image* target;
            g.target->GetTarget(&target);

            // Set canvas as target
            g.target->SetTarget(_canvas);
            g.target->Clear();

            if (_visible)
            {
                ID2D1Image* stash = nullptr;
                ID2D1Bitmap1* contentBitmap = nullptr;
                const float rounding = _cornerRounding; // Use const value in this function in case '_cornerRouding' is modified

                if (rounding > 0.0f)
                {
                    // Create separate target for content
                    g.target->CreateBitmap(
                        D2D1::SizeU(_width, _height),
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
                }

                // Draw background
                g.target->Clear(GetBackgroundColor());
                if (_background)
                {
                    g.target->DrawBitmap
                    (
                        _background,
                        D2D1::RectF(0, 0, g.target->GetSize().width, g.target->GetSize().height)
                    );
                }

                // Draw component
                _OnDraw(g);

                // Draw border
                if (_borderVisible)
                {
                    ID2D1SolidColorBrush* borderBrush = nullptr;
                    g.target->CreateSolidColorBrush(_borderColor, &borderBrush);
                    if (borderBrush)
                    {
                        float offset = _borderWidth * 0.5f;
                        if (rounding > 0.0f)
                        {
                            D2D1_ROUNDED_RECT roundedrect;
                            roundedrect.radiusX = _cornerRounding - offset;
                            roundedrect.radiusY = _cornerRounding - offset;
                            roundedrect.rect = D2D1::RectF(offset, offset, _width - offset, _height - offset);
                            g.target->DrawRoundedRectangle(roundedrect, borderBrush, _borderWidth);
                        }
                        else
                        {
                            g.target->DrawRectangle(D2D1::RectF(offset, offset, _width - offset, _height - offset), borderBrush, _borderWidth);
                        }
                        borderBrush->Release();
                    }
                }
                if (_selected)
                {
                    ID2D1SolidColorBrush* borderBrush = nullptr;
                    g.target->CreateSolidColorBrush(_selectedBorderColor, &borderBrush);
                    if (borderBrush)
                    {
                        float offset = _borderWidth * 0.5f;
                        if (rounding > 0.0f)
                        {
                            D2D1_ROUNDED_RECT roundedrect;
                            roundedrect.radiusX = _cornerRounding - offset;
                            roundedrect.radiusY = _cornerRounding - offset;
                            roundedrect.rect = D2D1::RectF(offset, offset, _width - offset, _height - offset);
                            g.target->DrawRoundedRectangle(roundedrect, borderBrush, _borderWidth);
                        }
                        else
                        {
                            g.target->DrawRectangle(D2D1::RectF(offset, offset, _width - offset, _height - offset), borderBrush, _borderWidth);
                        }
                        borderBrush->Release();
                    }
                }

                if (rounding > 0.0f)
                {
                    // Round corners
                    ID2D1Bitmap1* opacityMask = nullptr;
                    g.target->CreateBitmap(
                        D2D1::SizeU(_width, _height),
                        nullptr,
                        0,
                        D2D1::BitmapProperties1(
                            D2D1_BITMAP_OPTIONS_TARGET,
                            { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
                        ),
                        &opacityMask
                    );
                    g.target->SetTarget(opacityMask);
                    g.target->Clear();

                    D2D1_ROUNDED_RECT roundedrect;
                    roundedrect.radiusX = _cornerRounding;
                    roundedrect.radiusY = _cornerRounding;
                    roundedrect.rect.left = 0;
                    roundedrect.rect.top = 0;
                    roundedrect.rect.right = _width;
                    roundedrect.rect.bottom = _height;
                    ID2D1SolidColorBrush* opacityBrush;
                    g.target->CreateSolidColorBrush(D2D1::ColorF(0), &opacityBrush);
                    g.target->FillRoundedRectangle(roundedrect, opacityBrush);
                    opacityBrush->Release();

                    g.target->SetTarget(stash);
                    stash->Release();

                    ID2D1BitmapBrush* bitmapBrush;
                    g.target->CreateBitmapBrush(
                        contentBitmap,
                        D2D1::BitmapBrushProperties(
                            D2D1_EXTEND_MODE_CLAMP,
                            D2D1_EXTEND_MODE_CLAMP,
                            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                        ),
                        &bitmapBrush
                    );

                    g.target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                    g.target->FillOpacityMask(opacityMask, bitmapBrush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
                    g.target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                    g.target->Flush();
                    bitmapBrush->Release();
                    opacityMask->Release();
                    contentBitmap->Release();
                }

                // If inactive, gray out the canvas
                if (!_active)
                {
                    ID2D1Bitmap1* grayscaleBitmap = nullptr;
                    g.target->CreateBitmap(
                        D2D1::SizeU(_width, _height),
                        nullptr,
                        0,
                        D2D1::BitmapProperties1(
                            D2D1_BITMAP_OPTIONS_TARGET,
                            { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
                        ),
                        &grayscaleBitmap
                    );

                    ID2D1Effect* grayscaleEffect;
                    g.target->CreateEffect(CLSID_D2D1Grayscale, &grayscaleEffect);
                    grayscaleEffect->SetInput(0, _canvas);
                    ID2D1Effect* brightnessEffect;
                    g.target->CreateEffect(CLSID_D2D1Brightness, &brightnessEffect);
                    brightnessEffect->SetInputEffect(0, grayscaleEffect);
                    brightnessEffect->SetValue(D2D1_BRIGHTNESS_PROP_WHITE_POINT, D2D1::Vector2F(1.0f, 0.5f));
                    brightnessEffect->SetValue(D2D1_BRIGHTNESS_PROP_BLACK_POINT, D2D1::Vector2F(1.0f, 0.5f));

                    g.target->GetTarget(&stash);
                    g.target->SetTarget(grayscaleBitmap);
                    g.target->Clear();
                    g.target->DrawImage(brightnessEffect);
                    g.target->SetTarget(stash);
                    g.target->Clear();
                    g.target->DrawBitmap(grayscaleBitmap);

                    stash->Release();
                    brightnessEffect->Release();
                    grayscaleEffect->Release();
                    grayscaleBitmap->Release();
                }
            }
            else if (_Redraw())
            {
                _OnDraw(g);
                g.target->Clear();
            }

            // Unstash target
            g.target->SetTarget(target);
            target->Release();

            return _canvas;
        }

        ID2D1Bitmap* Image()
        {
            return _canvas;
        }

        void Resize(int width, int height)
        {
            if (width != _width || height != _height)
            {
                SetSize(width, height);
                _OnResize(_width, _height);
            }
        }

        // Additional functions
        virtual std::list<Base*> GetChildren()
        {
            return std::list<Base*>();
        }

        virtual std::list<Base*> GetAllChildren()
        {
            return std::list<Base*>();
        }

        virtual Base* IterateTab()
        {
            if (!Selected())
                return this;
            else
                return nullptr;
        }

    protected:
        virtual void _OnUpdate() {}
        virtual bool _Redraw() { return false; }
        virtual void _OnDraw(Graphics g) {}
        virtual void _OnResize(int width, int height) {}

    public:
        virtual const char* GetName() const { return "base"; }
    };

}

//#include "Scene.h"
//
//template<class T, typename... Args>
//std::unique_ptr<T> zcom::Base::Create(Args&&... args)
//{
//    return _scene->Create(std::forward<Args>(args)...);
//}