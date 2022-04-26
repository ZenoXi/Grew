#pragma once

#include "ComponentBase.h"

namespace zcom
{
    // Completely barebones component, only contains base component functionality
    class EmptyPanel : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate() {}
        void _OnDraw(Graphics g) {}
        void _OnResize(int width, int height) {}

    public:
        const char* GetName() const { return "empty_panel"; }
#pragma endregion

    public:
        EmptyPanel() {}
        ~EmptyPanel() {}
        EmptyPanel(EmptyPanel&&) = delete;
        EmptyPanel& operator=(EmptyPanel&&) = delete;
        EmptyPanel(const EmptyPanel&) = delete;
        EmptyPanel& operator=(const EmptyPanel&) = delete;
    };
}