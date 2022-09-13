#pragma once

#include "ComponentBase.h"
#include "ComHelper.h"

#include "TintEffect.h"

namespace zcom
{
    enum class ImagePlacement
    {
        // Doesn't do any stretching or aligning
        NONE,
        // Stretch the image to fill the entire drawing area
        FILL,
        // Fit and center the image so that nothing is cropped
        FIT,
        // Position the image in the top left corner
        TOP_LEFT,
        // Position the image at the top edge center
        TOP_CENTER,
        // Position the image in the top right corner
        TOP_RIGHT,
        // Position the image at the left edge center
        CENTER_LEFT,
        // Centers the image
        CENTER,
        // Position the image at the right edge center
        CENTER_RIGHT,
        // Position the image in the bottom left corner
        BOTTOM_LEFT,
        // Position the image at the bottom edge center
        BOTTOM_CENTER,
        // Position the image in the bottom right corner
        BOTTOM_RIGHT,
    };

    class Image : public Base
    {
    public:
        void SetImage(ID2D1Bitmap* image)
        {
            if (image == _image)
                return;

            _image = image;
            InvokeRedraw();
        }

        ID2D1Bitmap* GetImage() const
        {
            return _image;
        }

        // Source image area to use
        void SetSourceRect(RECT_F rect)
        {
            if (rect == _sourceRect)
                return;

            _sourceRect = rect;
            InvokeRedraw();
        }

        RECT_F GetSourceRect() const
        {
            return _sourceRect;
        }

        void SetPlacement(ImagePlacement placement)
        {
            if (placement == _placement)
                return;

            _placement = placement;
            InvokeRedraw();
        }

        ImagePlacement GetPlacement() const
        {
            return _placement;
        }

        void SetImageOffsetX(float offset)
        {
            SetImageOffset(offset, GetImageOffsetY());
        }

        void SetImageOffsetY(float offset)
        {
            SetImageOffset(GetImageOffsetX(), offset);
        }

        void SetImageOffset(float offsetX, float offsetY)
        {
            if (offsetX == _offsetX && offsetY == _offsetY)
                return;

            _offsetX = offsetX;
            _offsetY = offsetY;
            InvokeRedraw();
        }

        float GetImageOffsetX() const
        {
            return _offsetX;
        }

        float GetImageOffsetY() const
        {
            return _offsetY;
        }

        // Sets whether the image should be snapped to pixels or positioned freely
        void SetPixelSnap(bool snap)
        {
            if (snap == _snap)
                return;

            _snap = snap;
            InvokeRedraw();
        }

        bool GetPixelSnap() const
        {
            return _snap;
        }

        void SetScaleX(float scale)
        {
            SetScale(scale, GetScaleY());
        }

        void SetScaleY(float scale)
        {
            SetScale(GetScaleX(), scale);
        }

        // Scales the image by the specified amounts
        // The scaling only applies to placement modes that do not size the image automatically
        // Scaled image size ignores pixel snapping (top left corner placement is still snapped)
        void SetScale(float scaleX, float scaleY)
        {
            if (scaleX == _scaleX && scaleY == _scaleY)
                return;

            _scaleX = scaleX;
            _scaleY = scaleY;
            InvokeRedraw();
        }

        float GetScaleX() const
        {
            return _scaleX;
        }

        float GetScaleY() const
        {
            return _scaleY;
        }

        void SetImageOpacity(float opacity)
        {
            if (opacity == _imageOpacity)
                return;

            _imageOpacity = opacity;
            InvokeRedraw();
        }

        float GetImageOpacity() const
        {
            return _imageOpacity;
        }

        void SetTintColor(D2D1_COLOR_F color)
        {
            if (color == _tintColor)
                return;

            _tintColor = color;
            InvokeRedraw();
        }

        D2D1_COLOR_F GetTintColor() const
        {
            return _tintColor;
        }

    protected:
        friend class Scene;
        friend class Base;
        Image(Scene* scene) : Base(scene) {}
        void Init(ID2D1Bitmap* image = nullptr)
        {
            _image = image;
        }
    public:
        ~Image()
        {
        }
        Image(Image&&) = delete;
        Image& operator=(Image&&) = delete;
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

    private:
        ID2D1Bitmap* _image = nullptr;
        RECT_F _sourceRect = {0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        ImagePlacement _placement = ImagePlacement::NONE;
        float _offsetX = 0.0f;
        float _offsetY = 0.0f;
        float _scaleX = 1.0f;
        float _scaleY = 1.0f;
        bool _snap = false;
        float _imageOpacity = 1.0f;
        D2D1_COLOR_F _tintColor = D2D1::ColorF(1.0f, 1.0f, 1.0f);

#pragma region base_class
    protected:
        void _OnDraw(Graphics g)
        {
            if (!_image)
                return;

            D2D1_RECT_F srcRect = { _sourceRect.left, _sourceRect.top, _sourceRect.right, _sourceRect.bottom };
            D2D1_RECT_F destRect = { 0.0f, 0.0f, g.target->GetSize().width, g.target->GetSize().height };

            // Bound src rect
            if (srcRect.left < 0.0f)
                srcRect.left = 0.0f;
            if (srcRect.top < 0.0f)
                srcRect.top = 0.0f;
            if (srcRect.right > _image->GetSize().width)
                srcRect.right = _image->GetSize().width;
            if (srcRect.bottom > _image->GetSize().height)
                srcRect.bottom = _image->GetSize().height;

            // Calculate dest rect
            if (_placement == ImagePlacement::NONE)
            {
                float width = (srcRect.right - srcRect.left) * _scaleX;
                float height = (srcRect.bottom - srcRect.top) * _scaleY;
                float left = 0.0f + _offsetX;
                float top = 0.0f + _offsetY;
                if (_snap)
                {
                    left = std::roundf(left);
                    top = std::roundf(top);
                }
                destRect = { left, top, left + width, top + height };
            }
            else if (_placement == ImagePlacement::FILL)
            {
                float left = 0.0f + _offsetX;
                float top = 0.0f + _offsetY;
                if (_snap)
                {
                    left = std::roundf(left);
                    top = std::roundf(top);
                }
                float width = g.target->GetSize().width;
                float height = g.target->GetSize().height;
                destRect = { left, top, left + width, top + height};
            }
            else if (_placement == ImagePlacement::FIT)
            {
                // Scale frame to preserve aspect ratio
                float imageWidth = srcRect.right - srcRect.left;
                float imageHeight = srcRect.bottom - srcRect.top;
                float targetWidth = g.target->GetSize().width;
                float targetHeight = g.target->GetSize().height;
                if (imageWidth / imageHeight < targetWidth / targetHeight)
                {
                    float scale = imageHeight / targetHeight;
                    float newWidth = imageWidth / scale;
                    destRect = D2D1::Rect
                    (
                        (targetWidth - newWidth) * 0.5f,
                        0.0f,
                        (targetWidth - newWidth) * 0.5f + newWidth,
                        targetHeight
                    );
                }
                else if (imageWidth / imageHeight > targetWidth / targetHeight)
                {
                    float scale = imageWidth / targetWidth;
                    float newHeight = imageHeight / scale;
                    destRect = D2D1::Rect
                    (
                        0.0f,
                        (targetHeight - newHeight) * 0.5f,
                        targetWidth,
                        (targetHeight - newHeight) * 0.5f + newHeight
                    );
                }
                else
                {
                    destRect = D2D1::RectF(0.0f, 0.0f, targetWidth, targetHeight);
                }

                // Apply offset
                destRect.left += _offsetX;
                destRect.right += _offsetX;
                destRect.top += _offsetY;
                destRect.bottom += _offsetY;

                // Snap to pixels
                if (_snap)
                {
                    float width = destRect.right - destRect.left;
                    float height = destRect.bottom - destRect.top;
                    destRect.left = std::roundf(destRect.left);
                    destRect.top = std::roundf(destRect.top);
                    destRect.right = destRect.left + width;
                    destRect.bottom = destRect.top + height;
                }
            }
            // Remaining placement modes involve just position placements
            else
            {
                // Scale image
                float width = (srcRect.right - srcRect.left) * _scaleX;
                float height = (srcRect.bottom - srcRect.top) * _scaleY;

                // Calculate placement
                float left = 0.0f;
                float top = 0.0f;
                switch (_placement)
                {
                case ImagePlacement::TOP_LEFT:
                {
                    break;
                }
                case ImagePlacement::TOP_CENTER:
                {
                    left = (g.target->GetSize().width - width) * 0.5f;
                    break;
                }
                case ImagePlacement::TOP_RIGHT:
                {
                    left = g.target->GetSize().width - width;
                    break;
                }
                case ImagePlacement::CENTER_LEFT:
                {
                    top = (g.target->GetSize().height - height) * 0.5f;
                    break;
                }
                case ImagePlacement::CENTER:
                {
                    top = (g.target->GetSize().height - height) * 0.5f;
                    left = (g.target->GetSize().width - width) * 0.5f;
                    break;
                }
                case ImagePlacement::CENTER_RIGHT:
                {
                    top = (g.target->GetSize().height - height) * 0.5f;
                    left = g.target->GetSize().width - width;
                    break;
                }
                case ImagePlacement::BOTTOM_LEFT:
                {
                    top = g.target->GetSize().height - height;
                    break;
                }
                case ImagePlacement::BOTTOM_CENTER:
                {
                    top = g.target->GetSize().height - height;
                    left = (g.target->GetSize().width - width) * 0.5f;
                    break;
                }
                case ImagePlacement::BOTTOM_RIGHT:
                {
                    top = g.target->GetSize().height - height;
                    left = g.target->GetSize().width - width;
                    break;
                }
                default:
                    break;
                }

                // Snap to pixels
                left += _offsetX;
                top += _offsetY;
                if (_snap)
                {
                    left = std::roundf(left);
                    top = std::roundf(top);
                }

                destRect = { left, top, left + width, top + height };
            }

            if (!(_tintColor == D2D1::ColorF(1.0f, 1.0f, 1.0f)))
            {
                // Create tint effect
                ID2D1Effect* tintEffect = nullptr;
                HRESULT hr = g.target->CreateEffect(CLSID_CustomTintEffect, &tintEffect);
                tintEffect->SetInput(0, _image);
                D2D1_VECTOR_4F premultiplied = { _tintColor.r, _tintColor.g, _tintColor.b, _tintColor.a };
                premultiplied.x *= premultiplied.w;
                premultiplied.y *= premultiplied.w;
                premultiplied.z *= premultiplied.w;
                tintEffect->SetValue(CUSTOM_TINT_PROP_COLOR, premultiplied);

                // Draw to separate render target and use 'DrawBitmap' for scaling/placement
                ID2D1Image* stash = nullptr;
                ID2D1Bitmap1* contentBitmap = nullptr;
                g.target->CreateBitmap(
                    D2D1::SizeU((UINT32)_image->GetSize().width, (UINT32)_image->GetSize().height),
                    nullptr,
                    0,
                    D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET,
                        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }
                    ),
                    &contentBitmap
                );

                g.target->GetTarget(&stash);
                g.target->SetTarget(contentBitmap);
                g.target->Clear();
                g.target->DrawImage(tintEffect);
                g.target->SetTarget(stash);
                stash->Release();
                g.target->DrawBitmap(contentBitmap, destRect, _imageOpacity, D2D1_INTERPOLATION_MODE_CUBIC, srcRect);

                contentBitmap->Release();
                tintEffect->Release();
            }
            else
            {
                // Draw image normally
                g.target->DrawBitmap(_image, destRect, _imageOpacity, D2D1_INTERPOLATION_MODE_CUBIC, srcRect);
            }
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "image"; }
#pragma endregion
    };
}