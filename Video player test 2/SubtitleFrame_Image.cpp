#include "SubtitleFrame_Image.h"

#include "App.h"

void SubtitleFrame_Image::DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap, SubtitlePosition& position, int videoWidth, int videoHeight)
{
    if (_rects.empty())
    {
        App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
        return;
    }

    // Calculate enclosing rect bounds
    RECT finalRect = _rects[0].rect;
    for (int i = 1; i < _rects.size(); i++)
    {
        if (_rects[i].rect.left < finalRect.left)
            finalRect.left = _rects[i].rect.left;
        if (_rects[i].rect.top < finalRect.top)
            finalRect.top = _rects[i].rect.top;
        if (_rects[i].rect.right > finalRect.right)
            finalRect.right = _rects[i].rect.right;
        if (_rects[i].rect.bottom < finalRect.bottom)
            finalRect.bottom = _rects[i].rect.bottom;
    }
    position.x = finalRect.left;
    position.y = finalRect.top;
    int finalWidth = finalRect.right - finalRect.left;
    int finalHeight = finalRect.bottom - finalRect.top;

    // Release current bitmap if necessary
    ID2D1Bitmap1* bitmap = *targetBitmap;
    if (bitmap)
    {
        auto bitmapSize = bitmap->GetSize();
        if ((int)bitmapSize.width != finalWidth || (int)bitmapSize.height != finalHeight)
            App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
    }

    // Create new bitmap
    bitmap = *targetBitmap;
    if (!bitmap)
    {
        g.target->CreateBitmap(
            D2D1::SizeU(finalWidth, finalHeight),
            nullptr,
            0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
            ),
            targetBitmap
        );
        g.refs->push_back({ (IUnknown**)targetBitmap, "Sub frame image bitmap" });
    }

    // Copy rects to bitmap
    bitmap = *targetBitmap;
    for (int i = 0; i < _rects.size(); i++)
    {
        D2D1_RECT_U rect = D2D1::RectU(
            _rects[i].rect.left - position.x,
            _rects[i].rect.top - position.y,
            _rects[i].rect.right - position.x,
            _rects[i].rect.bottom - position.y
        );
        bitmap->CopyFromMemory(
            &rect,
            _rects[i].data.get(),
            (_rects[i].rect.right - _rects[i].rect.left) * 4
        );
    }
}