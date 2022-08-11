#pragma once

#include "ComponentBase.h"

namespace zcom
{
    // Completely barebones component, only contains base component functionality
    class DotGrid : public Base
    {
    public:
        void SetDotColor(D2D1_COLOR_F color)
        {
            if (_dotColor == color)
                return;
            _dotColor = color;
            InvokeRedraw();
        }

        D2D1_COLOR_F DotColor() const
        {
            return _dotColor;
        }

        void SetMargins(RECT margins)
        {
            if (_margins == margins)
                return;
            _margins = margins;
            InvokeRedraw();
        }

        RECT Margins() const
        {
            return _margins;
        }

        void SetHorizontalOffset(int offset)
        {
            SetOffset(offset, _verticalOffset);
        }

        int HorizontalOffset() const
        {
            return _horizontalOffset;
        }

        void SetVerticalOffset(int offset)
        {
            SetOffset(_horizontalOffset, offset);
        }

        int VerticalOffset() const
        {
            return _verticalOffset;
        }

        void SetOffset(int horizontal, int vertical)
        {
            if (horizontal == _horizontalOffset && vertical == _verticalOffset)
                return;
            _horizontalOffset = horizontal;
            _verticalOffset = vertical;
            InvokeRedraw();
        }

    protected:
        friend class Scene;
        friend class Base;
        DotGrid(Scene* scene) : Base(scene) {}
        void Init() {}
    public:
        ~DotGrid() {}
        DotGrid(DotGrid&&) = delete;
        DotGrid& operator=(DotGrid&&) = delete;
        DotGrid(const DotGrid&) = delete;
        DotGrid& operator=(const DotGrid&) = delete;

    private:
        D2D1_COLOR_F _dotColor = D2D1::ColorF(0.5f, 0.5f, 0.5f);
        RECT _margins = { 2, 2, 2, 2 };
        int _horizontalOffset = 0;
        int _verticalOffset = 0;

#pragma region base_class
    protected:
        void _OnDraw(Graphics g)
        {
            ID2D1SolidColorBrush* brush;
            g.target->CreateSolidColorBrush(_dotColor, &brush);

            for (int x = _margins.left; x < GetWidth() - _margins.right; x++)
            {
                for (int y = _margins.top; y < GetHeight() - _margins.bottom; y++)
                {
                    // Apply offset
                    int trueX = x - _horizontalOffset;
                    int trueY = y - _verticalOffset;

                    // Since the grid repeats every 4 pixels, squish the value to [0,4)
                    // Double modulo is required to deal with negative values
                    int gridX = ((trueX % 4) + 4) % 4;
                    int gridY = ((trueY % 4) + 4) % 4;

                    if ((gridX == 0 && gridY == 0) ||
                        (gridX == 2 && gridY == 2))
                    {
                        D2D1_RECT_F rect;
                        rect.left = x;
                        rect.right = rect.left + 1.0f;
                        rect.top = y;
                        rect.bottom = rect.top + 1.0f;
                        g.target->FillRectangle(rect, brush);
                    }
                }
            }

            brush->Release();
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "dot_grid"; }
#pragma endregion
    };
}