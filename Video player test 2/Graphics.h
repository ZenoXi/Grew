#pragma once

#pragma comment( lib,"d2d1.lib" )
#pragma comment( lib,"d3d11.lib" )
#pragma comment( lib,"dxguid.lib" )
#pragma comment( lib,"dwrite.lib" )
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <d2d1effects.h>
#include <d2d1effects_2.h>
#include "Dwrite.h"

struct Graphics
{
    ID2D1DeviceContext* target = nullptr;
    ID2D1Factory1* factory = nullptr;
    std::vector<IUnknown**>* refs = nullptr;
};