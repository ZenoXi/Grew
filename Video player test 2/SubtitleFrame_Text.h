#pragma once

#include "ISubtitleFrame.h"

#include <string>

class SubtitleFrame_Text : public ISubtitleFrame
{
public:
    SubtitleFrame_Text(TimePoint timestamp, std::wstring text) : ISubtitleFrame(timestamp)
    {
        _text = text;
    }

    void DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap, SubtitlePosition& position, int videoWidth, int videoHeight);

protected:
    std::wstring _text;
};