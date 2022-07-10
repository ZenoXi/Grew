#pragma once

#include "ComponentBase.h"

namespace zcom
{
    enum class ImageStretchMode
    {
        // Stretches the image to fill the entire
        // drawing area
        FILL,
        // Fits and centers the image so that nothing is cropped
        FIT,
        // Doesn't do any stretching or aligning
        NONE
    };

    class Image : public Base
    {
#pragma region base_class
    protected:
        void _OnDraw(Graphics g)
        {
            if (_mode == ImageStretchMode::NONE)
            {
                g.target->DrawBitmap(_image);
            }
            else if (_mode == ImageStretchMode::FILL)
            {
                D2D1_RECT_F destRect = { 0.0f, 0.0f, g.target->GetSize().width, g.target->GetSize().height };
                D2D1_RECT_F srcRect = { 0.0f, 0.0f, _image->GetSize().width, _image->GetSize().height };
                g.target->DrawBitmap(_image, destRect, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRect);
            }
            else if (_mode == ImageStretchMode::FIT)
            {
                D2D1_RECT_F destRect;
                D2D1_RECT_F srcRect;

                // Scale frame to preserve aspect ratio
                float imageWidth = _image->GetSize().width;
                float imageHeight = _image->GetSize().height;
                srcRect = D2D1::RectF(0.0f, 0.0f, imageWidth, imageHeight);
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

                g.target->DrawBitmap(_image, destRect, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRect);
            }
        }

    public:
        const char* GetName() const { return "image"; }
#pragma endregion

    private:
        ID2D1Bitmap* _image = nullptr;
        ImageStretchMode _mode = ImageStretchMode::NONE;

    protected:
        friend class Scene;
        friend class Base;
        Image(Scene* scene) : Base(scene) {}
        void Init() {}
    public:
        ~Image()
        {
        }
        Image(Image&&) = delete;
        Image& operator=(Image&&) = delete;
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        void SetImage(ID2D1Bitmap* image)
        {
            _image = image;
            InvokeRedraw();
        }

        void SetStretchMode(ImageStretchMode mode)
        {
            if (mode == _mode)
                return;

            _mode = mode;
            InvokeRedraw();
        }
    };
}