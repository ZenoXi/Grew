#pragma once

#include "ISubtitleFrame.h"

#include <vector>

class SubtitleFrame_Image : public ISubtitleFrame
{
public:
    SubtitleFrame_Image(TimePoint timestamp) : ISubtitleFrame(timestamp)
    {

    }

    void AddRect(RECT rect, std::unique_ptr<unsigned char[]> data)
    {
        _rects.push_back({ rect, std::move(data) });
    }

    void DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap, SubtitlePosition& position, int videoWidth, int videoHeight);

protected:
    struct _SubRect
    {
        RECT rect;
        std::unique_ptr<unsigned char[]> data;
    };
    std::vector<_SubRect> _rects;
};