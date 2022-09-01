#include "Label.h"
#include "App.h"

void zcom::Label::_OnSelected()
{
    _scene->GetApp()->keyboardManager.SetExclusiveHandler(this);
    GetKeyboardState(_keyStates);
}

void zcom::Label::_OnDeselected()
{
    _scene->GetApp()->keyboardManager.ResetExclusiveHandler();

    if (_textSelectable)
    {
        _selecting = false;
        _selectionStart = 0;
        _selectionLength = 0;
        InvokeRedraw();
    }
}