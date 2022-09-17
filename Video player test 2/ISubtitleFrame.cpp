#include "ISubtitleFrame.h"
#include "App.h"

void ISubtitleFrame::DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap, SubtitlePosition& position, int videoWidth, int videoHeight)
{
    // Release current bitmap
    ID2D1Bitmap1* bitmap = *targetBitmap;
    if (bitmap)
        App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
}