#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "ComponentBase.h"
#include "Label.h"
#include "GameTime.h"
#include "Functions.h"
#include "Event.h"
#include "Transition.h"
#include "MediaChapter.h"

#include <sstream>

namespace zcom
{
    class SeekBar : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            float initialValue = _timeBarHeight;
            _timeBarHeightTransition.Apply(_timeBarHeight);
            if (_timeBarHeight != initialValue)
                InvokeRedraw();
        }

        bool _Redraw()
        {
            return _currentTimeLabel->Redraw() || _durationLabel->Redraw();
        }

        void _OnDraw(Graphics g)
        {
            // Create brushes
            if (!_viewedPartBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue, 0.75f), &_viewedPartBrush);
                g.refs->push_back((IUnknown**)&_viewedPartBrush);
            }
            if (!_seekbarMarkerBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue), &_seekbarMarkerBrush);
                g.refs->push_back((IUnknown**)&_seekbarMarkerBrush);
            }
            if (!_bufferedPartBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &_bufferedPartBrush);
                g.refs->push_back((IUnknown**)&_bufferedPartBrush);
            }
            if (!_remainingPartBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &_remainingPartBrush);
                g.refs->push_back((IUnknown**)&_remainingPartBrush);
            }

            // Draw the seek bar
            float progress = _currentTime.GetTicks() / (double)_duration.GetTicks();
            float bufferProgress = _buffered.GetTicks() / (double)_duration.GetTicks();
            if (progress > 1.0f) progress = 1.0f;
            int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
            int seekBarWidth = GetWidth() - timeTextWidth * 2;
            int viewedPartWidth = seekBarWidth * progress;
            int bufferedPartWidth = seekBarWidth * bufferProgress;
            // Background part
            g.target->FillRectangle(
                D2D1::RectF(
                    timeTextWidth,
                    GetHeight() / 2.0f - _timeBarHeight,
                    GetWidth() - timeTextWidth,
                    GetHeight() / 2.0f + _timeBarHeight),
                _remainingPartBrush
            );
            // Buffered part
            if (_buffered > 0)
            {
                g.target->FillRectangle(
                    D2D1::RectF(
                        timeTextWidth,
                        GetHeight() / 2.0f - _timeBarHeight,
                        timeTextWidth + bufferedPartWidth,
                        GetHeight() / 2.0f + _timeBarHeight
                    ),
                    _bufferedPartBrush
                );
            }
            //else
            //{
            //    g.target->FillRectangle(
            //        D2D1::RectF(
            //            timeTextWidth + viewedPartWidth,
            //            GetHeight() / 2.0f - 1.0f,
            //            timeTextWidth + bufferedPartWidth,
            //            GetHeight() / 2.0f + 1.0f),
            //        _viewedBufferingPartBrush
            //    );
            //}
            // Completed part
            g.target->FillRectangle(
                D2D1::RectF(
                    timeTextWidth,
                    GetHeight() / 2.0f - _timeBarHeight,
                    timeTextWidth + viewedPartWidth,
                    GetHeight() / 2.0f + _timeBarHeight
                ),
                _viewedPartBrush
            );
            // Chapter markers
            for (int i = 0; i < _chapters.size(); i++)
            {
                // Start marker
                int startXPos = (_chapters[i].start.GetTicks() / (double)_duration.GetTicks()) * seekBarWidth;
                g.target->FillRectangle(
                    D2D1::RectF(
                        timeTextWidth + startXPos,
                        GetHeight() / 2.0f + _timeBarHeight,
                        timeTextWidth + startXPos + 1.0f,
                        GetHeight() / 2.0f + _timeBarHeight + 4.0f
                    ),
                    _remainingPartBrush
                );

                // End marker
                if (_chapters[i].end.GetTicks() != AV_NOPTS_VALUE)
                {
                    int endXPos = (_chapters[i].end.GetTicks() / (double)_duration.GetTicks()) * seekBarWidth;
                    g.target->FillRectangle(
                        D2D1::RectF(
                            timeTextWidth + startXPos,
                            GetHeight() / 2.0f + _timeBarHeight,
                            timeTextWidth + startXPos + 1.0f,
                            GetHeight() / 2.0f + _timeBarHeight + 4.0f
                        ),
                        _remainingPartBrush
                    );
                }
            }

            // 
            if (/*GetMouseInside()*/_timeHovered)
            {
                int markerPosition = viewedPartWidth;
                if (_held)
                    markerPosition = _heldPosition;

                g.target->FillEllipse(
                    D2D1::Ellipse(
                        D2D1::Point2F(
                            timeTextWidth + markerPosition,
                            GetHeight() / 2.0f
                        ),
                        5.0f,
                        5.0f
                    ),
                    _seekbarMarkerBrush
                );
            }

            // Create time strings
            _currentTimeLabel->SetText(string_to_wstring(TimeToString(_currentTime)));
            _durationLabel->SetText(string_to_wstring(TimeToString(_duration.GetTicks())));

            // Draw time strings
            auto currentTimeRect = D2D1::RectF(
                _margins,
                (GetHeight() - _textHeight) * 0.5f,
                _margins + _maxTimeWidth,
                (GetHeight() + _textHeight) * 0.5f
            );
            if (_currentTimeLabel->Redraw())
                _currentTimeLabel->Draw(g);
            g.target->DrawBitmap(_currentTimeLabel->Image(), currentTimeRect);

            auto durationRect = D2D1::RectF(
                GetWidth() - _margins - _maxTimeWidth,
                (GetHeight() - _textHeight) * 0.5f,
                GetWidth() - _margins,
                (GetHeight() + _textHeight) * 0.5f
            );
            if (_durationLabel->Redraw())
                _durationLabel->Draw(g);
            g.target->DrawBitmap(_durationLabel->Image(), durationRect);
        }

        EventTargets _OnMouseMove(int x, int y, bool duplicate)
        {
            if (duplicate)
                return EventTargets().Add(this, x, y);

            int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
            int seekBarWidth = GetWidth() - timeTextWidth * 2;
            int xPos = GetMousePosX() - timeTextWidth;
            if (xPos >= 0 && xPos <= seekBarWidth)
            {
                double xPosNorm = xPos / (double)seekBarWidth;
                TimePoint time = _duration.GetTicks() * xPosNorm;

                std::wstring title = L"";
                for (int i = _chapters.size() - 1; i >= 0; i--)
                {
                    if (time >= _chapters[i].start)
                    {
                        if (_chapters[i].end.GetTicks() != AV_NOPTS_VALUE)
                        {
                            if (time > _chapters[i].end)
                            {
                                break;
                            }
                        }
                        else
                        {
                            title = string_to_wstring(_chapters[i].title);
                            break;
                        }
                    }
                }

                _onTimeHovered.InvokeAll(x, time, title);
                if (!_timeHovered)
                {
                    // Start height change transition
                    _timeBarHeightTransition.Start(_timeBarHeight, 3.0f);
                }
                _timeHovered = true;
            }
            else if (!_held)
            {
                if (_timeHovered)
                {
                    _onHoverEnded.InvokeAll();
                    _timeHovered = false;

                    // Start height change transition
                    _timeBarHeightTransition.Start(_timeBarHeight, 1.0f);
                }
            }
            if (_held)
            {
                if (xPos < 0) xPos = 0;
                if (xPos > seekBarWidth) xPos = seekBarWidth;
                _heldPosition = xPos;
            }
            InvokeRedraw();
            return EventTargets().Add(this, x, y);
        }

        void _OnMouseLeave()
        {
            if (_timeHovered)
            {
                _onHoverEnded.InvokeAll();
                _timeHovered = false;

                // Start height change transition
                _timeBarHeightTransition.Start(_timeBarHeight, 1.0f);
            }
        }

        void _OnMouseEnter()
        {
            _mouseHoverStart = ztime::Main();
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
            int seekBarWidth = GetWidth() - timeTextWidth * 2;
            int xPos = GetMousePosX() - timeTextWidth;
            if (xPos >= 0 && xPos <= seekBarWidth)
            {
                float xPosNorm = xPos / (float)seekBarWidth;
                _held = true;
                _heldPosition = xPos;
                InvokeRedraw();
            }
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnLeftReleased(int x, int y)
        {
            int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
            int seekBarWidth = GetWidth() - timeTextWidth * 2;
            _selectedTime = _duration.GetTicks() * _heldPosition / (double)seekBarWidth;
            _held = false;
            return EventTargets().Add(this, x, y);
        }

    public:
        const char* GetName() const { return "seek_bar"; }
#pragma endregion

    private:
        Duration _duration = 0;
        Duration _buffered = 0;
        TimePoint _currentTime = 0;

        std::unique_ptr<Label> _currentTimeLabel = nullptr;
        std::unique_ptr<Label> _durationLabel = nullptr;

        std::vector<MediaChapter> _chapters;

        bool _held = false;
        int _heldPosition = 0;
        TimePoint _selectedTime = -1;
        float _textHeight = 0.0f;
        float _maxTimeWidth = 0.0f;

        float _timeBarHeight = 1.0f; // This is height from center to each edge. Total height is 2 * _timeBarHeight
        Transition<float> _timeBarHeightTransition = Transition<float>(Duration(100, MILLISECONDS));

        float _margins = 5.0f;

        bool _timeHovered = false;
        Event<void, int, TimePoint, std::wstring> _onTimeHovered;
        Event<void> _onHoverEnded;

        // Resources
        ID2D1SolidColorBrush* _viewedPartBrush = nullptr;
        ID2D1SolidColorBrush* _seekbarMarkerBrush = nullptr;
        ID2D1SolidColorBrush* _bufferedPartBrush = nullptr;
        ID2D1SolidColorBrush* _remainingPartBrush = nullptr;

        IDWriteFactory* _dwriteFactory = nullptr;
        IDWriteTextFormat* _dwriteTextFormat = nullptr;
        IDWriteTextLayout* _dwriteTextLayout = nullptr;

        TimePoint _mouseHoverStart = 0;

    protected:
        friend class Scene;
        friend class Base;
        SeekBar(Scene* scene) : Base(scene) {}
        void Init(long long int duration)
        {
            _duration = duration;

            _maxTimeWidth = 62.0f;
            _textHeight = 20.0f;

            _currentTimeLabel = Create<Label>(L"--:--");
            _currentTimeLabel->SetSize(_maxTimeWidth, _textHeight);
            _currentTimeLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _currentTimeLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _currentTimeLabel->SetFontSize(15.0f);
            _currentTimeLabel->SetFontColor(D2D1::ColorF(D2D1::ColorF::LightGray));

            _durationLabel = Create<Label>(L"--:--");
            _durationLabel->SetSize(_maxTimeWidth, _textHeight);
            _durationLabel->SetHorizontalTextAlignment(TextAlignment::CENTER);
            _durationLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _durationLabel->SetFontSize(15.0f);
            _durationLabel->SetFontColor(D2D1::ColorF(D2D1::ColorF::LightGray));
        }
    public:
        ~SeekBar()
        {
            // Release resources
            SafeFullRelease((IUnknown**)&_viewedPartBrush);
            SafeFullRelease((IUnknown**)&_seekbarMarkerBrush);
            SafeFullRelease((IUnknown**)&_bufferedPartBrush);
            SafeFullRelease((IUnknown**)&_remainingPartBrush);
            SafeRelease((IUnknown**)&_dwriteTextFormat);
            SafeRelease((IUnknown**)&_dwriteTextLayout);
            SafeRelease((IUnknown**)&_dwriteFactory);
        }
        SeekBar(SeekBar&&) = delete;
        SeekBar& operator=(SeekBar&&) = delete;
        SeekBar(const SeekBar&) = delete;
        SeekBar& operator=(const SeekBar&) = delete;

        TimePoint SeekTime()
        {
            TimePoint timepoint = _selectedTime;
            _selectedTime = -1;
            return timepoint;
        }

        void SetCurrentTime(TimePoint time)
        {
            if (time == _currentTime)
                return;

            // Check if seekbar visually changes
            float progressBefore = _currentTime.GetTicks() / (double)_duration.GetTicks();
            float progressAfter = time.GetTicks() / (double)_duration.GetTicks();
            int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
            int seekBarWidth = GetWidth() - timeTextWidth * 2;
            int viewedPartWidthBefore = seekBarWidth * progressBefore;
            int viewedPartWidthAfter = seekBarWidth * progressAfter;
            if (viewedPartWidthBefore != viewedPartWidthAfter)
                InvokeRedraw();

            _currentTime = time;
            _currentTimeLabel->SetText(string_to_wstring(TimeToString(_currentTime)));
        }

        void SetBufferedDuration(Duration duration)
        {
            if (duration == _buffered)
                return;

            if (duration < 0)
            {
                duration = 0;
            }
            else if (duration > _duration)
            {
                duration = _duration;
            }

            // Check if seekbar visually changes
            float progressBefore = _buffered.GetTicks() / (double)_duration.GetTicks();
            float progressAfter = duration.GetTicks() / (double)_duration.GetTicks();
            int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
            int seekBarWidth = GetWidth() - timeTextWidth * 2;
            int bufferedPartWidthBefore = seekBarWidth * progressBefore;
            int bufferedPartWidthAfter = seekBarWidth * progressAfter;
            if (bufferedPartWidthBefore != bufferedPartWidthAfter)
                InvokeRedraw();

            _buffered = duration;
        }

        Duration GetDuration() const
        {
            return _duration;
        }

        void SetDuration(Duration duration)
        {
            if (duration == _duration)
                return;

            _duration = duration;
            _durationLabel->SetText(string_to_wstring(TimeToString(_duration.GetTicks())));
        }

        void AddOnTimeHovered(std::function<void(int, TimePoint, std::wstring)> func)
        {
            _onTimeHovered.Add(func);
        }

        void AddOnHoverEnded(std::function<void()> func)
        {
            _onHoverEnded.Add(func);
        }

        void SetChapters(std::vector<MediaChapter> chapters)
        {
            _chapters = chapters;

            // Process times
            for (int i = 0; i < _chapters.size(); i++)
            {
                if (_chapters[i].start.GetTicks() == AV_NOPTS_VALUE)
                {
                    _chapters.erase(_chapters.begin() + i);
                    i--;
                    continue;
                }

                if (i == 0)
                    continue;

                if (_chapters[i - 1].end > _chapters[i].start)
                {
                    _chapters[i].start = _chapters[i - 1].end;
                    if (_chapters[i].start >= _chapters[i].end)
                    {
                        _chapters.erase(_chapters.begin() + i);
                        i--;
                        continue;
                    }
                }
                //else if (_chapters[i - 1].end.GetTicks() == AV_NOPTS_VALUE)
                //{
                //    _chapters[i - 1].end = _chapters[i].start;
                //}
            }

            InvokeRedraw();
        }
    };
}