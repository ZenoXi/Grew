#include "ComponentBase.h"

#include "App.h"

void SafeFullRelease(IUnknown** res)
{
    App::Instance()->window.gfx.ReleaseResource(res);
}

void SafeRelease(IUnknown** res)
{
    if (*res)
    {
        (*res)->Release();
        *res = nullptr;
    }
}