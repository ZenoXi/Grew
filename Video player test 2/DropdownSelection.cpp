#include "DropdownSelection.h"
#include "App.h"
#include "OverlayScene.h"

zcom::EventTargets zcom::DropdownSelection::_OnLeftPressed(int x, int y)
{
    _scene->GetApp()->Overlay()->ShowMenu(
        _menuPanel.get(),
        {
            GetScreenX(),
            GetScreenY(),
            GetScreenX() + GetWidth(),
            GetScreenY() + GetHeight()
        }
    );

    return EventTargets().Add(this, x, y);
}