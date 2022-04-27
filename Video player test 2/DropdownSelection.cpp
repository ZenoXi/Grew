#include "DropdownSelection.h"
#include "Canvas.h"

void zcom::DropdownSelection::_AddPanelToCanvas()
{
    _canvas->AddComponent(_menuPanel.get());
}

void zcom::DropdownSelection::_RemovePanelFromCanvas()
{
    _canvas->RemoveComponent(_menuPanel.get());
}

zcom::EventTargets zcom::DropdownSelection::_OnLeftPressed(int x, int y)
{
    if (!_menuPanel->GetVisible())
    {
        RECT bounds = { 0, 0, _canvas->GetWidth(), _canvas->GetHeight() };
        if (_menuBounds != RECT{ 0, 0, 0, 0 })
            bounds = _menuBounds;

        _menuPanel->Show(
            bounds,
            {
                GetScreenX(),
                GetScreenY(),
                GetScreenX() + GetWidth(),
                GetScreenY() + GetHeight()
            }
        );
    }
    else
    {
        _menuPanel->Hide();
    }

    return EventTargets().Add(this, x, y);
}