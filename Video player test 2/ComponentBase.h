#pragma once

#include <iostream>
#include <vector>
#include <list>

#include "Graphics.h"
#include "MouseEventHandler.h"
#include "Event.h"

//#include "App.h"

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

class ComponentBase
{
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
    int _zIndex = -1;

    // Events
    bool _mouseInside = false;
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
    ComponentBase() {}
    virtual ~ComponentBase()
    {
        SafeFullRelease((IUnknown**)&_canvas);
    }
    ComponentBase(ComponentBase&&) = delete;
    ComponentBase& operator=(ComponentBase&&) = delete;
    ComponentBase(const ComponentBase&) = delete;
    ComponentBase& operator=(const ComponentBase&) = delete;

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
    void SetHorizontalOffsetPercent(float offset)
    {
        _hPosPercentOffset = offset;
    }
    void SetVerticalOffsetPercent(float offset)
    {
        _vPosPercentOffset = offset;
    }
    void SetHorizontalOffsetPixels(int offset)
    {
        _hPosPixelOffset = offset;
    }
    void SetVerticalOffsetPixels(int offset)
    {
        _vPosPixelOffset = offset;
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
    void SetBaseWidth(int width)
    {
        _hSize = width;
    }
    void SetBaseHeight(int height)
    {
        _vSize = height;
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
        _active = active;
    }
    void SetVisible(bool visible)
    {
        _visible = visible;
    }

    // Selection
    bool Selected() const { return _selected; }
    bool GetZIndex() const { return _zIndex; }

    void SetZIndex(int index) { _zIndex = index; }

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
    ComponentBase* OnMouseMove(int x, int y)
    {
        if (!_active) return this;

        _mousePosX = x;
        _mousePosY = y;
        return _OnMouseMove(x, y);
    }
    void OnMouseEnter()
    {
        if (!_active) return;

        _mouseInside = true;
        _OnMouseEnter();
    }
    void OnMouseLeave()
    {
        if (!_active) return;

        _mouseInside = false;
        _OnMouseLeave();
    }
    ComponentBase* OnLeftPressed(int x, int y)
    {
        if (!_active) return this;

        _mouseLeftClicked = true;
        return _OnLeftPressed(x, y);
    }
    ComponentBase* OnLeftReleased(int x, int y)
    {
        if (!_active) return this;

        _mouseLeftClicked = false;
        return _OnLeftReleased(x, y);
    }
    ComponentBase* OnRightPressed(int x, int y)
    {
        if (!_active) return this;

        _mouseRightClicked = true;
        return _OnRightPressed(x, y);
    }
    ComponentBase* OnRightReleased(int x, int y)
    {
        if (!_active) return this;

        _mouseRightClicked = false;
        return _OnRightReleased(x, y);
    }
    ComponentBase* OnWheelUp(int x, int y)
    {
        if (!_active) return this;

        return _OnWheelUp(x, y);
    }
    ComponentBase* OnWheelDown(int x, int y)
    {
        if (!_active) return this;

        return _OnWheelDown(x, y);
    }
    void OnSelected()
    {
        if (!_active) return;

        _selected = true;
        _OnSelected();
    }
    void OnDeselected()
    {
        if (!_active) return;

        _selected = false;
        _OnDeselected();
    }
private:
    virtual ComponentBase* _OnMouseMove(int x, int y) { return this; }
    virtual void _OnMouseEnter() {}
    virtual void _OnMouseLeave() {}
    virtual ComponentBase* _OnLeftPressed(int x, int y) { return this; }
    virtual ComponentBase* _OnRightPressed(int x, int y) { return this; }
    virtual ComponentBase* _OnLeftReleased(int x, int y) { return this; }
    virtual ComponentBase* _OnRightReleased(int x, int y) { return this; }
    virtual ComponentBase* _OnWheelUp(int x, int y) { return this; }
    virtual ComponentBase* _OnWheelDown(int x, int y) { return this; }
    virtual void _OnSelected() {}
    virtual void _OnDeselected() {}
public:
    bool GetMouseInside() const { return _mouseInside; }
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
        g.target->EndDraw();
        ID2D1Image* target;
        g.target->GetTarget(&target);

        // Set canvas as target
        g.target->SetTarget(_canvas);
        g.target->BeginDraw();

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

        HRESULT hr = g.target->EndDraw();
        if (hr != S_OK)
        {
            std::cout << "Draw failed in '" << GetName() << "'" << std::endl;
        }

        // Unstash target
        g.target->SetTarget(target);
        target->Release();
        g.target->BeginDraw();

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
    virtual std::list<ComponentBase*> GetChildren() = 0;
    virtual std::list<ComponentBase*> GetAllChildren() = 0;

protected:
    virtual void _OnUpdate() = 0;
    virtual void _OnDraw(Graphics g) = 0;
    virtual void _OnResize(int width, int height) = 0;

public:
    virtual const char* GetName() const { return "base"; }
};

class Canvas : public MouseEventHandler
{
    int _width;
    int _height;
    std::vector<ComponentBase*> _items;
    ComponentBase* _selected = nullptr;

    Event<void, ComponentBase*, int, int> _mouseMoveHandlers;
    Event<void, ComponentBase*, int, int> _leftPressedHandlers;
    Event<void, ComponentBase*, int, int> _leftReleasedHandlers;
    Event<void, ComponentBase*, int, int> _rightPressedHandlers;
    Event<void, ComponentBase*, int, int> _rightReleasedHandlers;
    Event<void, ComponentBase*, int, int> _wheelUpHandlers;
    Event<void, ComponentBase*, int, int> _wheelDownHandlers;

    ID2D1Bitmap1* _canvas = nullptr;
    D2D1_COLOR_F _backgroundColor = D2D1::ColorF(0, 0);

public:
    Canvas(int width, int height) : _width(width), _height(height) {}
    ~Canvas()
    {
        // Release resources
        for (auto item : _items)
        {
            delete item;
        }
        if (_canvas)
        {
            _canvas->Release();
        }
    }
    Canvas(Canvas&&) = delete;
    Canvas& operator=(Canvas&&) = delete;
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    void AddComponent(ComponentBase* comp)
    {
        _items.push_back(comp);
    }

    void RemoveComponent(ComponentBase* comp)
    {
        for (auto it = _items.begin(); it != _items.end(); it++)
        {
            if (*it == comp)
            {
                _items.erase(it);
                return;
            }
        }
    }

    void ClearComponents()
    {
        _items.clear();
    }

    void Update()
    {
        for (auto& item : _items)
        {
            item->Update();
        }
    }

    ID2D1Bitmap* Draw(Graphics g)
    {
        // Create/recreate canvas here to avoid passing device context in the constructor/other functions
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

        // Get bitmaps of all items
        std::vector<ID2D1Bitmap*> bitmaps;
        bitmaps.reserve(_items.size());
        for (auto& item : _items)
        {
            bitmaps.push_back(item->Draw(g));
        }

        // Stash current target
        g.target->EndDraw();
        ID2D1Image* target;
        g.target->GetTarget(&target);

        // Draw the bitmaps
        g.target->SetTarget(_canvas);
        g.target->BeginDraw();
        g.target->Clear(_backgroundColor);
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
        g.target->EndDraw();

        // Unstash target
        g.target->SetTarget(target);
        target->Release();
        g.target->BeginDraw();

        return _canvas;
    }

    void Resize(int width, int height)
    {
        _width = width;
        _height = height;
        SafeFullRelease((IUnknown**)&_canvas);

        // Calculate item sizes and positions
        for (auto& item : _items)
        {
            int newWidth = (int)std::round(_width * item->GetParentWidthPercent()) + item->GetBaseWidth();
            int newHeight = (int)std::round(_height * item->GetParentHeightPercent()) + item->GetBaseHeight();
            // SetSize does limit checking so the resulting size and newWidth/newHeight can mismatch
            item->SetSize(newWidth, newHeight);

            int newPosX = 0;
            if (item->GetHorizontalAlignment() == Alignment::START)
            {
                newPosX += std::round((_width - item->GetWidth()) * item->GetHorizontalOffsetPercent());
            }
            else if (item->GetHorizontalAlignment() == Alignment::END)
            {
                newPosX = _width - item->GetWidth();
                newPosX -= std::round((_width - item->GetWidth()) * item->GetHorizontalOffsetPercent());
            }
            newPosX += item->GetHorizontalOffsetPixels();
            // ALTERNATIVE (no branching):
            // int align = item->GetHorizontalAlignment() == Alignment::END;
            // newPosX += align * (_width - item->GetWidth());
            // newPosX += (-1 * align) * std::round((_width - item->GetWidth()) * item->GetHorizontalOffsetPercent());
            // newPosX += item->GetHorizontalOffsetPixels();
            int newPosY = 0;
            if (item->GetVerticalAlignment() == Alignment::START)
            {
                newPosY += std::round((_height - item->GetHeight()) * item->GetVerticalOffsetPercent());
            }
            else if (item->GetVerticalAlignment() == Alignment::END)
            {
                newPosY = _height - item->GetHeight();
                newPosY -= std::round((_height - item->GetHeight()) * item->GetVerticalOffsetPercent());
            }
            newPosY += item->GetVerticalOffsetPixels();
            item->SetPosition(newPosX, newPosY);

            item->Resize(item->GetWidth(), item->GetHeight());
        }
    }

    void Resize()
    {
        Resize(_width, _height);
    }

    void SetBackgroundColor(D2D1_COLOR_F color)
    {
        _backgroundColor = color;
    }

    void ClearSelection(ComponentBase* exception = nullptr)
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

    ComponentBase* GetSelectedComponent()
    {
        return _selected;
    }
    
    void _OnMouseMove(int x, int y)
    {
        ComponentBase* target = nullptr;
        std::vector<ComponentBase*> hoveredComponents;

        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (x >= item->GetX() && x < item->GetX() + item->GetWidth() &&
                y >= item->GetY() && y < item->GetY() + item->GetHeight())
            {
                if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                {
                    target = item->OnMouseMove(x - item->GetX(), y - item->GetY());
                    hoveredComponents.clear();
                    break;
                }
                hoveredComponents.push_back(item);
            }
            else if (item->GetMouseInside())
            {
                if (item->GetMouseLeftClicked() || item->GetMouseRightClicked())
                {
                    target = item->OnMouseMove(x - item->GetX(), y - item->GetY());
                    hoveredComponents.clear();
                    break;
                }
                else
                {
                    item->OnMouseLeave();
                }
            }
        }

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
            target = topmost->OnMouseMove(x - topmost->GetX(), y - topmost->GetY());
        }

        if (target)
        {
            _mouseMoveHandlers.InvokeAll(target, x, y);
        }
    }

    void _OnMouseLeave()
    {
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

    }

    void _OnLeftPressed(int x, int y)
    {
        ComponentBase* target = nullptr;
        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (item->GetMouseInside())
            {
                target = item->OnLeftPressed(x - item->GetX(), y - item->GetY());
                break;
            }
        }

        // Clear selection
        ClearSelection(target);

        if (target)
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
        ComponentBase* target = nullptr;
        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (item->GetMouseInside())
            {
                target = item->OnLeftReleased(x - item->GetX(), y - item->GetY());
                break;
            }
        }

        if (target)
        {
            _leftReleasedHandlers.InvokeAll(target, x, y);
        }
    }

    void _OnRightPressed(int x, int y)
    {
        ComponentBase* target = nullptr;
        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (item->GetMouseInside())
            {
                target = item->OnRightPressed(x - item->GetX(), y - item->GetY());
                break;
            }
        }

        if (target)
        {
            _rightPressedHandlers.InvokeAll(target, x, y);
        }
    }

    void _OnRightReleased(int x, int y)
    {
        ComponentBase* target = nullptr;
        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (item->GetMouseInside())
            {
                target = item->OnRightReleased(x - item->GetX(), y - item->GetY());
                break;
            }
        }

        if (target)
        {
            _rightReleasedHandlers.InvokeAll(target, x, y);
        }
    }

    void _OnWheelUp(int x, int y)
    {
        ComponentBase* target = nullptr;
        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (item->GetMouseInside())
            {
                target = item->OnWheelUp(x - item->GetX(), y - item->GetY());
                break;
            }
        }

        if (target)
        {
            _wheelUpHandlers.InvokeAll(target, x, y);
        }
    }

    void _OnWheelDown(int x, int y)
    {
        ComponentBase* target = nullptr;
        for (auto& item : _items)
        {
            if (!item->GetVisible()) continue;

            if (item->GetMouseInside())
            {
                target = item->OnWheelDown(x - item->GetX(), y - item->GetY());
                break;
            }
        }

        if (target)
        {
            _wheelDownHandlers.InvokeAll(target, x, y);
        }
    }

    void AddOnMouseMove(std::function<void(ComponentBase*, int, int)> func)
    {
        _mouseMoveHandlers.Add(func);
    }

    void AddOnLeftPressed(std::function<void(ComponentBase*, int, int)> func)
    {
        _leftPressedHandlers.Add(func);
    }

    void AddOnLeftReleased(std::function<void(ComponentBase*, int, int)> func)
    {
        _leftReleasedHandlers.Add(func);
    }

    void AddOnRightPressed(std::function<void(ComponentBase*, int, int)> func)
    {
        _rightPressedHandlers.Add(func);
    }

    void AddOnRightReleased(std::function<void(ComponentBase*, int, int)> func)
    {
        _rightReleasedHandlers.Add(func);
    }

    void AddOnWheelUp(std::function<void(ComponentBase*, int, int)> func)
    {
        _wheelUpHandlers.Add(func);
    }

    void AddOnWheelDown(std::function<void(ComponentBase*, int, int)> func)
    {
        _wheelDownHandlers.Add(func);
    }

private:
    ComponentBase* HighestZIndexRef()
    {
        int index = HighestZIndex();
        if (index == -1)
        {
            return nullptr;
        }
    }

    int HighestZIndex()
    {
        if (_items.empty())
        {
            return -1;
        }

        int bestIndex = 0;
        int highestIndex = _items[0]->GetZIndex();
        for (int i = 1; i < _items.size(); i++)
        {
            if (_items[i]->GetZIndex() > highestIndex)
            {
                bestIndex = i;
                highestIndex = _items[i]->GetZIndex();
            }
        }
        return bestIndex;
    }

    // Recursively gets all components in a scene
    // Might cause performance issues if used often
    std::list<ComponentBase*> GetAllItems()
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
};