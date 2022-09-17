#pragma once

#include "IMediaFrame.h"

#include "Graphics.h"

enum class SubtitlePlacement
{
    VIDEO_RELATIVE,
    BOTTOM
};

struct SubtitlePosition
{
    //SubtitlePlacement placement;
    bool videoRelative;
    int x;
    int y;
};

class ISubtitleFrame : public IMediaFrame
{
public:
    ISubtitleFrame(TimePoint timestamp) : IMediaFrame(timestamp)
    {

    }
    ISubtitleFrame() : IMediaFrame(-1)
    {

    }

    virtual void DrawFrame(Graphics g, ID2D1Bitmap1** targetBitmap, SubtitlePosition& position, int videoWidth, int videoHeight);
};