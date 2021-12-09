#include "ComponentBase.h"

#include "App.h"

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