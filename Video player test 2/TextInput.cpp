#include "TextInput.h"
#include "App.h"

void zcom::TextInput::_OnSelected()
{
    _scene->GetApp()->keyboardManager.SetExclusiveHandler(this);
    GetKeyboardState(_keyStates);

    _initialText = _textLabel->GetText();
}

void zcom::TextInput::_OnDeselected()
{
    _scene->GetApp()->keyboardManager.ResetExclusiveHandler();

    if (_TextMatches(_textLabel->GetText(), _pattern))
    //if (!_pattern.empty() && !_textLabel->GetText().empty() && !std::regex_match(_textLabel->GetText(), std::wregex(_pattern)))
        _textLabel->SetText(_initialText);

    _textLabel->SetSelectionStart(0);
    _textLabel->SetSelectionLength(0);
}