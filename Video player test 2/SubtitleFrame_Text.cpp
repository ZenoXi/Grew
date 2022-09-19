#include "SubtitleFrame_Text.h"

#include "App.h"

class OutlinedTextRenderer : public IDWriteTextRenderer
{
public:
    OutlinedTextRenderer(Graphics g)
    {
        _gfx = g;
    }

    HRESULT DrawGlyphRun(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GLYPH_RUN const* glyphRun,
        DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        IUnknown* clientDrawingEffect
    )
    {
        HRESULT hr;

        // Create the path geometry.
        ID2D1PathGeometry* pPathGeometry = NULL;
        hr = _gfx.factory->CreatePathGeometry(&pPathGeometry);

        // Write to the path geometry using the geometry sink.
        ID2D1GeometrySink* pSink = NULL;
        if (SUCCEEDED(hr))
        {
            hr = pPathGeometry->Open(&pSink);

            // Get the glyph run outline geometries back from DirectWrite and place them within the
            // geometry sink.
            if (SUCCEEDED(hr))
            {
                hr = glyphRun->fontFace->GetGlyphRunOutline(
                    glyphRun->fontEmSize,
                    glyphRun->glyphIndices,
                    glyphRun->glyphAdvances,
                    glyphRun->glyphOffsets,
                    glyphRun->glyphCount,
                    glyphRun->isSideways,
                    glyphRun->bidiLevel % 2,
                    pSink
                );

                // Close the geometry sink
                hr = pSink->Close();

                // Initialize a matrix to translate the origin of the glyph run.
                D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
                    1.0f, 0.0f,
                    0.0f, 1.0f,
                    baselineOriginX, baselineOriginY
                );

                // Create the transformed geometry
                ID2D1TransformedGeometry* pTransformedGeometry = NULL;
                hr = _gfx.factory->CreateTransformedGeometry(
                    pPathGeometry,
                    &matrix,
                    &pTransformedGeometry
                );

                ID2D1SolidColorBrush* outlineBrush = nullptr;
                ID2D1SolidColorBrush* fillBrush = nullptr;
                _gfx.target->CreateSolidColorBrush(D2D1::ColorF(0), &outlineBrush);
                _gfx.target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &fillBrush);

                // Draw the outline of the glyph run
                _gfx.target->DrawGeometry(
                    pTransformedGeometry,
                    outlineBrush,
                    5.0f
                );

                // Fill in the glyph run
                _gfx.target->FillGeometry(
                    pTransformedGeometry,
                    fillBrush
                );

                outlineBrush->Release();
                fillBrush->Release();
                pTransformedGeometry->Release();
            }

            pPathGeometry->Release();
            pSink->Release();
        }

        return S_OK;
    }

    HRESULT DrawUnderline(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_UNDERLINE const* underline,
        IUnknown* clientDrawingEffect
    )
    {
        return S_OK;
    }

    HRESULT DrawStrikethrough(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_STRIKETHROUGH const* strikethrough,
        IUnknown* clientDrawingEffect
    )
    {
        return S_OK;
    }

    HRESULT IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled)
    {
        *isDisabled = false;
        return S_OK;
    }

    HRESULT GetCurrentTransform(void* clientDrawingContext, DWRITE_MATRIX* transform)
    {
        _gfx.target->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
        return S_OK;
    }

    HRESULT GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip)
    {
        float x, yUnused;
        _gfx.target->GetDpi(&x, &yUnused);
        *pixelsPerDip = x / 96;
        return S_OK;
    }

    HRESULT DrawInlineObject(
        void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect
    )
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) { return 0; }

private:
    Graphics _gfx;
};

void SubtitleFrame_Text::DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap, SubtitlePosition& position, int videoWidth, int videoHeight)
{
    if (_text.empty())
    {
        App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
        return;
    }

    // Create text layout
    IDWriteFactory* factory;
    DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&factory)
    );
    IDWriteTextFormat* format;
    factory->CreateTextFormat(
        L"Arial",
        NULL,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        52.0f,
        L"en-us",
        &format
    );
    IDWriteTextLayout* layout;
    factory->CreateTextLayout(
        _text.c_str(),
        _text.length(),
        format,
        (float)videoWidth,
        (float)videoHeight,
        &layout
    );

    // Get final bitmap size
    DWRITE_TEXT_METRICS metrics;
    layout->GetMetrics(&metrics);
    float textWidth = metrics.width;
    float textHeight = metrics.height;
    int finalWidth = (int)std::ceilf(textWidth);
    int finalHeight = (int)std::ceilf(textHeight);

    // Calculate bitmap placement (bottom center)
    position.x = (videoWidth - (finalWidth + 10)) / 2;
    position.y = videoHeight - (finalHeight + 10) - 60;

    // Recreate layout with new size
    layout->Release();
    factory->CreateTextLayout(
        _text.c_str(),
        _text.length(),
        format,
        (float)finalWidth,
        (float)finalHeight,
        &layout
    );
    layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    // Release current bitmap if necessary
    ID2D1Bitmap1* bitmap = *targetBitmap;
    if (bitmap)
    {
        auto bitmapSize = bitmap->GetSize();
        if ((int)bitmapSize.width != finalWidth + 10 || (int)bitmapSize.height != finalHeight + 10)
            App::Instance()->window.gfx.ReleaseResource((IUnknown**)targetBitmap);
    }

    // Create new bitmap
    bitmap = *targetBitmap;
    if (!bitmap)
    {
        g.target->CreateBitmap(
            D2D1::SizeU(finalWidth + 10, finalHeight + 10),
            nullptr,
            0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET,
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
            ),
            targetBitmap
        );
        g.refs->push_back((IUnknown**)targetBitmap);
    }
    bitmap = *targetBitmap;

    // Set output bitmap as render target
    ID2D1Image* stash = nullptr;
    g.target->GetTarget(&stash);
    g.target->SetTarget(bitmap);

    // Render text
    OutlinedTextRenderer renderer(g);
    layout->Draw(
        nullptr,
        &renderer,
        5.0f,
        5.0f
    );

    // Unstash target
    g.target->SetTarget(stash);
    stash->Release();

    // Release resources
    layout->Release();
    format->Release();
    factory->Release();
}