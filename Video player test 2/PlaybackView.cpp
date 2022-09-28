#include "PlaybackView.h"
#include "App.h"

void zcom::PlaybackView::Init()
{
    _playback = &App::Instance()->playback;
}

void zcom::PlaybackView::_OnUpdate()
{
    // Check for redraw
    if (!_playback->Initializing())
    {
        if (_playback->VideoAdapter()->FrameNumber() != _videoFrameNumber)
        {
            InvokeRedraw();
            _videoFrameChanged = true;
            _videoFrameNumber = _playback->VideoAdapter()->FrameNumber();
        }
        if (_playback->SubtitleAdapter()->FrameNumber() != _subtitleFrameNumber)
        {
            InvokeRedraw();
            _subtitleFrameChanged = true;
            _subtitleFrameNumber = _playback->SubtitleAdapter()->FrameNumber();
        }
    }
}

void zcom::PlaybackView::_OnDraw(Graphics g)
{
    if (!_playback->Initializing())
    {
        if (!_videoFrameBitmap)
            _videoFrameChanged = true;            // # // <- spot for cursor (ignore my autism)
        if (!_subtitleFrameBitmap)
            _subtitleFrameChanged = true;

        IVideoFrame* videoFrame = _playback->VideoAdapter()->GetFrameData();
        ISubtitleFrame* subtitleFrame = _playback->SubtitleAdapter()->GetFrameData();

        int subtitleDrawAreaWidth = 0;
        int subtitleDrawAreaHeight = 0;

        // Update video bitmap
        if (_videoFrameChanged)
            if (videoFrame)
                videoFrame->DrawFrame(g, &_videoFrameBitmap);

        if (_videoFrameBitmap && videoFrame)
        {
            subtitleDrawAreaWidth = videoFrame->GetWidth();
            subtitleDrawAreaHeight = videoFrame->GetHeight();
        }
        else
        {
            subtitleDrawAreaWidth = g.target->GetSize().width;
            subtitleDrawAreaHeight = g.target->GetSize().height;
        }

        // Update subtitle bitmap
        if (_subtitleFrameChanged)
        {
            _subPos.x = 0;
            _subPos.y = 0;
            if (subtitleFrame)
                subtitleFrame->DrawFrame(g, &_subtitleFrameBitmap, _subPos, subtitleDrawAreaWidth, subtitleDrawAreaHeight);
        }

        // Determine video frame destination dimensions
        D2D1_RECT_F destRectVideo;
        D2D1_RECT_F srcRectVideo;
        if (_videoFrameBitmap)
        {
            // Scale frame to preserve aspect ratio
            srcRectVideo = D2D1::RectF(0.0f, 0.0f, videoFrame->GetWidth(), videoFrame->GetHeight());
            float tWidth = g.target->GetSize().width;
            float tHeight = g.target->GetSize().height;
            if (videoFrame->GetWidth() / (float)videoFrame->GetHeight() < tWidth / tHeight)
            {
                float scale = videoFrame->GetHeight() / tHeight;
                float newWidth = videoFrame->GetWidth() / scale;
                destRectVideo = D2D1::Rect
                (
                    (tWidth - newWidth) * 0.5f,
                    0.0f,
                    (tWidth - newWidth) * 0.5f + newWidth,
                    tHeight
                );
            }
            else if (videoFrame->GetWidth() / (float)videoFrame->GetHeight() > tWidth / tHeight)
            {
                float scale = videoFrame->GetWidth() / tWidth;
                float newHeight = videoFrame->GetHeight() / scale;
                destRectVideo = D2D1::Rect
                (
                    0.0f,
                    (tHeight - newHeight) * 0.5f,
                    tWidth,
                    (tHeight - newHeight) * 0.5f + newHeight
                );
            }
            else
            {
                destRectVideo = D2D1::RectF(0.0f, 0.0f, tWidth, tHeight);
            }
        }

        // Determine subtitle frame destination dimensions
        D2D1_RECT_F destRectSubtitles;
        D2D1_RECT_F srcRectSubtitles;
        if (_subtitleFrameBitmap)
        {
            srcRectSubtitles = D2D1::RectF(
                0,
                0,
                _subtitleFrameBitmap->GetSize().width,
                _subtitleFrameBitmap->GetSize().height
            );

            if (_videoFrameBitmap)
            {
                float videoScale = (destRectVideo.right - destRectVideo.left) / (srcRectVideo.right - srcRectVideo.left);

                // Calculate destination rect size and scale it to video draw dimensions
                destRectSubtitles.left = _subPos.x * videoScale;
                destRectSubtitles.top = _subPos.y * videoScale;
                destRectSubtitles.right = (_subPos.x + srcRectSubtitles.right) * videoScale;
                destRectSubtitles.bottom = (_subPos.y + srcRectSubtitles.bottom) * videoScale;

                // Position destination rect correctly
                destRectSubtitles.left += destRectVideo.left;
                destRectSubtitles.top += destRectVideo.top;
                destRectSubtitles.right += destRectVideo.left;
                destRectSubtitles.bottom += destRectVideo.top;
            }
            else
            {
                // If no video, use absolute coordinates
                destRectSubtitles.left = _subPos.x;
                destRectSubtitles.top = _subPos.y;
                destRectSubtitles.right = _subPos.x + srcRectSubtitles.right;
                destRectSubtitles.bottom = _subPos.y + srcRectSubtitles.bottom;
            }
        }

        // Draw video
        if (_videoFrameBitmap)
            g.target->DrawBitmap(_videoFrameBitmap, destRectVideo, 1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, srcRectVideo);

        // Draw subtitles
        if (_subtitleFrameBitmap)
            g.target->DrawBitmap(_subtitleFrameBitmap, destRectSubtitles, 1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, srcRectSubtitles);

        _videoFrameChanged = false;
        _subtitleFrameChanged = false;
    }
}

void zcom::PlaybackView::_OnResize(int width, int height)
{

}