#include "ComponentBase.h"

#include "App.h"
#include "OverlayScene.h"

void zcom::SafeFullRelease(IUnknown** res)
{
    App::Instance()->window.gfx.ReleaseResource(res);
}

void zcom::SafeRelease(IUnknown** res)
{
    if (*res)
    {
        (*res)->Release();
        *res = nullptr;
    }
}

void zcom::Base::_ApplyCursor()
{
    _scene->GetApp()->window.SetCursorIcon(_cursor);
}

void zcom::Base::_ShowHoverText()
{
    if (_hoverText.empty())
        return;

    _scene->GetApp()->Overlay()->ShowHoverText(_hoverText, GetScreenX() + GetMousePosX(), GetScreenY() + GetMousePosY());
}