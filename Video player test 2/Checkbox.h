#pragma once

#include "ComponentBase.h"

#include "Event.h"
#include "KeyboardEventHandler.h"

namespace zcom
{
    class Checkbox : public Base, public KeyboardEventHandler
    {
    public:
        bool Checked() const { return _checked; }
        void Checked(bool checked);
        void SetCheckColor(D2D1_COLOR_F checkColor);
        void AddOnStateChanged(const std::function<void(bool)>& handler);

    protected:
        friend class Scene;
        friend class Base;
        Checkbox(Scene* scene) : Base(scene) {}
        void Init(bool checked = false);
    public:
        ~Checkbox() {}
        Checkbox(Checkbox&&) = delete;
        Checkbox& operator=(Checkbox&&) = delete;
        Checkbox(const Checkbox&) = delete;
        Checkbox& operator=(const Checkbox&) = delete;

    private:
        Event<void, bool> _onStateChanged;
        D2D1_COLOR_F _checkColor = {};
        bool _checked = false;

#pragma region base_class
    protected:
        void _OnDraw(Graphics g);
        EventTargets _OnLeftPressed(int x, int y);
        void _OnSelected();
        void _OnDeselected();
        bool _OnKeyDown(BYTE vkCode);
        bool _OnKeyUp(BYTE vkCode) { return false; }
        bool _OnChar(wchar_t ch) { return false; }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "checkbox"; }
#pragma endregion
    };
}