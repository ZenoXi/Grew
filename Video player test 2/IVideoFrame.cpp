#include "IVideoFrame.h"
#include "App.h"

void IVideoFrame::DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap)
{
    // Release current bitmap
    ID2D1Bitmap1* bitmap = *targetBitmap;
    if (bitmap)
        App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
}