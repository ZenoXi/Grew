#pragma once

#include "ComponentBase.h"
#include "IVideoOutputAdapter.h"
#include "ISubtitleOutputAdapter.h"
#include "ISubtitleFrame.h"

class Playback;

namespace zcom
{
    class PlaybackView : public Base
    {
    protected:
        friend class Scene;
        friend class Base;
        PlaybackView(Scene* scene) : Base(scene) {}
        void Init();
    public:
        ~PlaybackView()
        {
            SafeFullRelease((IUnknown**)&_videoFrameBitmap);
            SafeFullRelease((IUnknown**)&_subtitleFrameBitmap);
        }
        PlaybackView(PlaybackView&&) = delete;
        PlaybackView& operator=(PlaybackView&&) = delete;
        PlaybackView(const PlaybackView&) = delete;
        PlaybackView& operator=(const PlaybackView&) = delete;

    private:
        Playback* _playback;

        ID2D1Bitmap1* _videoFrameBitmap = nullptr;
        ID2D1Bitmap1* _subtitleFrameBitmap = nullptr;
        bool _videoFrameChanged = false;
        bool _subtitleFrameChanged = false;
        uint64_t _videoFrameNumber = 0;
        uint64_t _subtitleFrameNumber = 0;
        SubtitlePosition _subPos;

#pragma region base_class
    protected:
        void _OnUpdate();
        void _OnDraw(Graphics g);
        void _OnResize(int width, int height);

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "playback_view"; }
#pragma endregion
    };
}