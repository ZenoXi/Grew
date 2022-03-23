#include "TextInput.h"
#include "App.h"

void zcom::TextInput::_OnSelected()
{
    App::Instance()->keyboardManager.SetExclusiveHandler(this);
    GetKeyboardState(_keyStates);
}

void zcom::TextInput::_OnDeselected()
{
    App::Instance()->keyboardManager.ResetExclusiveHandler();
}