#include "ResourceManager.h"

#include "Functions.h"

#include <wincodec.h>
#include <fstream>

std::vector<ImageResource> ResourceManager::_images;

void ResourceManager::Init(std::string resourceFilePath, ID2D1DeviceContext* target)
{
    // Create 
    IWICImagingFactory* WICFactory;
    CoCreateInstance
    (
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&WICFactory)
    );

    std::ifstream in(resourceFilePath);
    while (true)
    {
        std::string resourceName;
        std::string resourcePath;
        in >> resourceName;
        in.get();
        bool eof = !std::getline(in, resourcePath);

        std::wstring resourcePathW = string_to_wstring(resourcePath);

        if (!eof)
        {
            HRESULT hr;

            IWICBitmapDecoder* decoder;
            hr = WICFactory->CreateDecoderFromFilename
            (
                resourcePathW.c_str(),
                NULL,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                &decoder
            );

            IWICBitmapFrameDecode* source;
            hr = decoder->GetFrame(0, &source);

            IWICFormatConverter* converter;
            hr = WICFactory->CreateFormatConverter(&converter);
            hr = converter->Initialize
            (
                source,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
            );

            ID2D1Bitmap* bitmap;
            hr = target->CreateBitmapFromWicBitmap
            (
                converter,
                NULL,
                &bitmap
            );

            ResourceManager::_images.push_back({ resourceName, bitmap });

            decoder->Release();
            source->Release();
            converter->Release();
        }
        else
        {
            break;
        }
    }

    WICFactory->Release();
}

ID2D1Bitmap* ResourceManager::GetImage(std::string name)
{
    for (auto& resource : ResourceManager::_images)
    {
        if (resource.name == name)
        {
            return resource.bitmap;
        }
    }
    return nullptr;
}