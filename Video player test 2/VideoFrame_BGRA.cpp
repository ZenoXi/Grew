#include "VideoFrame_BGRA.h"

#include "App.h"

void VideoFrame_BGRA::DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap)
{
    // Release current bitmap if necessary
    ID2D1Bitmap1* bitmap = *targetBitmap;
    if (bitmap)
    {
        auto bitmapSize = bitmap->GetSize();
        if ((int)bitmapSize.width != _width || (int)bitmapSize.height != _height)
            App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
    }

    // Create new bitmap
    bitmap = *targetBitmap;
    if (!bitmap && _data)
    {
        g.target->CreateBitmap(
            D2D1::SizeU(_width, _height),
            nullptr,
            0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
            ),
            targetBitmap
        );
        g.refs->push_back({ (IUnknown**)targetBitmap, "BGRA frame bitmap" });
    }

    // Copy data to new bitmap
    if (_data)
    {
        bitmap = *targetBitmap;
        D2D1_RECT_U rect = D2D1::RectU(0, 0, _width, _height);
        bitmap->CopyFromMemory(&rect, _data.get(), _width * 4);
    }
}