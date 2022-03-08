#pragma once

#include <iostream>
#include <vector>
#include <list>

#include "Graphics.h"
#include "MouseEventHandler.h"
#include "Event.h"

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

    // The base component class
    class Base
    {
        friend class Canvas;

        ID2D1Bitmap1* _canvas = nullptr;

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
        int _width = 100;
        int _height = 100;
        float _opacity = 1.0f;
        bool _active = true;
        bool _visible = true;

        // Selection
        bool _selected = false;
        int _tabIndex = -1;
        int _zIndex = -1;

        // Events
        bool _mouseInside = false;
        bool _mouseInsideArea = false;
        bool _mouseLeftClicked = false;
        bool _mouseRightClicked = false;
        int _mousePosX = 0;
        int _mousePosY = 0;

        // Border
        bool _borderVisible = false;
        float _borderWidth = 1.0f;
        D2D1_COLOR_F _borderColor = D2D1::ColorF(1.0, 1.0, 1.0);

        // Background
        D2D1_COLOR_F _backgroundColor = D2D1::ColorF(0, 0);
        ID2D1Bitmap* _background = nullptr;

    public:
        Base() {}
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
            _hPosAlign = alignment;
        }
        void SetVerticalAlignment(Alignment alignment)
        {
            _vPosAlign = alignment;
        }
        void SetAlignment(Alignment horizontal, Alignment vertical)
        {
            SetHorizontalAlignment(horizontal);
            SetVerticalAlignment(vertical);
        }
        void SetHorizontalOffsetPercent(float offset)
        {
            _hPosPercentOffset = offset;
        }
        void SetVerticalOffsetPercent(float offset)
        {
            _vPosPercentOffset = offset;
        }
        void SetOffsetPercent(float horizontal, float vertical)
        {
            SetHorizontalOffsetPercent(horizontal);
            SetVerticalOffsetPercent(vertical);
        }
        void SetHorizontalOffsetPixels(int offset)
        {
            _hPosPixelOffset = offset;
        }
        void SetVerticalOffsetPixels(int offset)
        {
            _vPosPixelOffset = offset;
        }
        void SetOffsetPixels(int horizontal, int vertical)
        {
            SetHorizontalOffsetPixels(horizontal);
            SetVerticalOffsetPixels(vertical);
        }

        // Size description
        float GetParentWidthPercent() const { return _hSizeParentPercent; }
        float GetParentHeightPercent() const { return _vSizeParentPercent; }
        int GetBaseWidth() const { return _hSize; }
        int GetBaseHeight() const { return _vSize; }

        void SetParentWidthPercent(float width)
        {
            _hSizeParentPercent = width;
        }
        void SetParentHeightPercent(float height)
        {
            _vSizeParentPercent = height;
        }
        void SetParentSizePercent(float width, float height)
        {
            SetParentWidthPercent(width);
            SetParentHeightPercent(width);
        }
        void SetBaseWidth(int width)
        {
            _hSize = width;
        }
        void SetBaseHeight(int height)
        {
            _vSize = height;
        }
        void SetBaseSize(int width, int height)
        {
            SetBaseWidth(width);
            SetBaseHeight(height);
        }

        // Common
        int GetX() const { return _x; }
        int GetY() const { return _y; }
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
            if (width <= 0) width = 1;
            if (height <= 0) height = 1;
            _width = width;
            _height = height;
            SafeFullRelease((IUnknown**)&_canvas);
            //if (_canvas)
            //{
            //    _canvas->Release();
            //    _canvas = nullptr;
            //}
        }
        void SetOpacity(float opacity)
        {
            _opacity = opacity;
        }
        void SetActive(bool active)
        {
            if (!active)
            {
                OnDeselected();
                OnLeftReleased();
                OnRightReleased();
            }
            _active = active;
        }
        void SetVisible(bool visible)
        {
            if (!visible)
            {
                OnDeselected();
                OnLeftReleased();
                OnRightReleased();
            }
            _visible = visible;
        }

        // Selection
        bool Selected() const { return _selected; }
        int GetZIndex() const { return _zIndex; }
        int GetTabIndex() const { return _tabIndex; }

        void SetZIndex(int index) { _zIndex = index; }
        void SetTabIndex(int index) { _tabIndex = index; }

        // Border
        bool GetBorderVisibility() const { return _borderVisible; }
        float GetBorderWidth() const { return _borderWidth; }
        D2D1_COLOR_F GetBorderColor() const { return _borderColor; }

        void SetBorderVisibility(bool visible)
        {
            _borderVisible = visible;
        }
        void SetBorderWidth(float width)
        {
            _borderWidth = width;
        }
        void SetBorderColor(D2D1_COLOR_F color)
        {
            _borderColor = color;
        }

        // Background
        D2D1_COLOR_F GetBackgroundColor() const { return _backgroundColor; }
        ID2D1Bitmap* GetBackgroundImage() const { return _background; }

        void SetBackgroundColor(D2D1_COLOR_F color)
        {
            _backgroundColor = color;
        }
        void SetBackgroundImage(ID2D1Bitmap* image)
        {
            _background = image;
        }

        // Events
        Base* OnMouseMove(int x, int y)
        {
            if (!_active) return nullptr;

            _mousePosX = x;
            _mousePosY = y;
            return _OnMouseMove(x, y);
        }
        void OnMouseEnter()
        {
            if (!_active) return;
            if (_mouseInside) return;

            _mouseInside = true;
            _OnMouseEnter();
        }
        void OnMouseLeave()
        {
            if (!_active) return;
            if (!_mouseInside) return;

            _mouseInside = false;
            _OnMouseLeave();
        }
        void OnMouseEnterArea()
        {
            if (!_active) return;
            if (_mouseInsideArea) return;

            _mouseInsideArea = true;
            _OnMouseEnterArea();
        }
        void OnMouseLeaveArea()
        {
            if (!_active) return;
            if (!_mouseInsideArea) return;

            _mouseInsideArea = false;
            _OnMouseLeaveArea();
        }
        Base* OnLeftPressed(int x, int y)
        {
            if (!_active) return nullptr;
            if (_mouseLeftClicked) return nullptr;

            _mouseLeftClicked = true;
            return _OnLeftPressed(x, y);
        }
        Base* OnLeftReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min())
        {
            if (!_active) return nullptr;
            if (!_mouseLeftClicked) return nullptr;

            _mouseLeftClicked = false;
            return _OnLeftReleased(x, y);
        }
        Base* OnRightPressed(int x, int y)
        {
            if (!_active) return nullptr;
            if (_mouseRightClicked) return nullptr;

            _mouseRightClicked = true;
            return _OnRightPressed(x, y);
        }
        Base* OnRightReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min())
        {
            if (!_active) return nullptr;
            if (!_mouseRightClicked) return nullptr;

            _mouseRightClicked = false;
            return _OnRightReleased(x, y);
        }
        Base* OnWheelUp(int x, int y)
        {
            if (!_active) return nullptr;

            return _OnWheelUp(x, y);
        }
        Base* OnWheelDown(int x, int y)
        {
            if (!_active) return nullptr;

            return _OnWheelDown(x, y);
        }
        void OnSelected()
        {
            if (!_active) return;
            if (_selected) return;

            _selected = true;
            _OnSelected();
        }
        void OnDeselected()
        {
            if (!_active) return;
            if (!_selected) return;

            _selected = false;
            _OnDeselected();
        }
    protected:
        virtual Base* _OnMouseMove(int x, int y) { return this; }
        virtual void _OnMouseEnter() {}
        virtual void _OnMouseLeave() {}
        virtual void _OnMouseEnterArea() {}
        virtual void _OnMouseLeaveArea() {}
        virtual Base* _OnLeftPressed(int x, int y) { return this; }
        virtual Base* _OnRightPressed(int x, int y) { return this; }
        virtual Base* _OnLeftReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min()) { return this; }
        virtual Base* _OnRightReleased(int x = std::numeric_limits<int>::min(), int y = std::numeric_limits<int>::min()) { return this; }
        virtual Base* _OnWheelUp(int x, int y) { return this; }
        virtual Base* _OnWheelDown(int x, int y) { return this; }
        virtual void _OnSelected() {}
        virtual void _OnDeselected() {}
    public:
        bool GetMouseInside() const { return _mouseInside; }
        bool GetMouseInsideArea() const { return _mouseInsideArea; }
        bool GetMouseLeftClicked() const { return _mouseLeftClicked; }
        bool GetMouseRightClicked() const { return _mouseRightClicked; }
        int GetMousePosX() const { return _mousePosX; }
        int GetMousePosY() const { return _mousePosY; }

        // Main functions
        void Update()
        {
            if (!_active) return;

            _OnUpdate();
        }

        ID2D1Bitmap* Draw(Graphics g)
        {
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
            //g.target->EndDraw();
            ID2D1Image* target;
            g.target->GetTarget(&target);

            // Set canvas as target
            g.target->SetTarget(_canvas);
            //g.target->BeginDraw();

            if (_visible)
            {
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
                        g.target->DrawRectangle(D2D1::RectF(offset, offset, _width - offset, _height - offset), borderBrush, _borderWidth);
                        borderBrush->Release();
                    }
                }
            }
            else
            {
                g.target->Clear();
            }

            //HRESULT hr = g.target->EndDraw();
            //if (hr != S_OK)
            //{
            //    std::cout << "Draw failed in '" << GetName() << "'" << std::endl;
            //}

            // Unstash target
            g.target->SetTarget(target);
            target->Release();
            //g.target->BeginDraw();

            return _canvas;
        }

        void Resize(int width, int height)
        {
            if (width != _width || height != _height)
            {
                std::cout << "[WARN]Size not set prior to resizing '" << GetName() << "'" << std::endl;
                SetSize(width, height);
            }
            _OnResize(_width, _height);
        }

        void Resize()
        {
            Resize(_width, _height);
        }

        // Additional functions
        virtual std::list<Base*> GetChildren() = 0;
        virtual std::list<Base*> GetAllChildren() = 0;
        virtual Base* IterateTab() = 0;

    protected:
        virtual void _OnUpdate() = 0;
        virtual void _OnDraw(Graphics g) = 0;
        virtual void _OnResize(int width, int height) = 0;

    public:
        virtual const char* GetName() const { return "base"; }
    };
}