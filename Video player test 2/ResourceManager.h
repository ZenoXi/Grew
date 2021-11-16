#pragma once

#pragma comment( lib,"d2d1.lib" )
#include <d2d1_1.h>

#include <string>
#include <vector>

struct ImageResource
{
    std::string name;
    ID2D1Bitmap* bitmap;
};

class ResourceManager
{
    ResourceManager() {}

    static std::vector<ImageResource> _images;

public:
    static void Init(std::string resourceFilePath, ID2D1DeviceContext* target);
    static ID2D1Bitmap* GetImage(std::string name);
};