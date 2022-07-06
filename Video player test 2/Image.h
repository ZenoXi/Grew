#pragma once

#include "ComponentBase.h"

namespace zcom
{
    class Image : public Base
    {
#pragma region base_class
    private:
        void _OnUpdate()
        {

        }

        void _OnDraw(Graphics g)
        {
            g.target->DrawBitmap(_image);
        }

        void _OnResize(int width, int height)
        {

        }

    public:
        const char* GetName() const { return "image"; }
#pragma endregion

    private:
        ID2D1Bitmap* _image = nullptr;

    public:
        Image()
        {
        }
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
        }
    };
}