#include "Button.h"
#include "App.h"

void zcom::Button::_OnSelected()
{
    App::Instance()->keyboardManager.SetExclusiveHandler(this);
}

void zcom::Button::_OnDeselected()
{
    App::Instance()->keyboardManager.ResetExclusiveHandler();
}