#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "ComponentBase.h"
#include "GameTime.h"

#include <sstream>

class SeekBar : public ComponentBase
{
#pragma region base_class
private:
    void _OnUpdate()
    {
        
    }

    void _OnDraw(Graphics g)
    {
        // Create brushes
        if (!_viewedPartBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue), &_viewedPartBrush);
            g.refs->push_back((IUnknown**)&_viewedPartBrush);
        }
        if (!_viewedBufferingPartBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF::ColorF(25, 120, 213), &_viewedBufferingPartBrush);
            g.refs->push_back((IUnknown**)&_viewedBufferingPartBrush);
        }
        if (!_bufferedPartBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &_bufferedPartBrush);
            g.refs->push_back((IUnknown**)&_bufferedPartBrush);
        }
        if (!_remainingPartBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray), &_remainingPartBrush);
            g.refs->push_back((IUnknown**)&_remainingPartBrush);
        }
        if (!_textBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &_textBrush);
            g.refs->push_back((IUnknown**)&_textBrush);
        }
        if (!_textBackgroundBrush)
        {
            g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(0, 0.5f)), &_textBackgroundBrush);
            g.refs->push_back((IUnknown**)&_textBackgroundBrush);
        }

        g.target->Clear();

        // Draw the seek bar
        float progress = ztime::Game().GetTime() / (double)_duration;
        float bufferProgress = _buffered / (double)_duration;
        if (progress > 1.0f) progress = 1.0f;
        int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
        int seekBarWidth = GetWidth() - timeTextWidth * 2;
        int viewedPartWidth = seekBarWidth * progress;
        int bufferedPartWidth = seekBarWidth * bufferProgress;
        // Completed part
        g.target->FillRectangle(
            D2D1::RectF(
                timeTextWidth,
                GetHeight() / 2.0f - 1.0f,
                timeTextWidth + viewedPartWidth,
                GetHeight() / 2.0f + 1.0f
            ),
            _viewedPartBrush
        );
        // Remaining part
        g.target->FillRectangle(
            D2D1::RectF(
                timeTextWidth + viewedPartWidth,
                GetHeight() / 2.0f - 1.0f,
                GetWidth() - timeTextWidth,
                GetHeight() / 2.0f + 1.0f),
            _remainingPartBrush
        );
        // Buffered part
        if (_buffered < ztime::Game().GetTime())
        {
            g.target->FillRectangle(
                D2D1::RectF(
                    timeTextWidth + bufferedPartWidth,
                    GetHeight() / 2.0f - 1.0f,
                    timeTextWidth + viewedPartWidth,
                    GetHeight() / 2.0f + 1.0f
                ),
                _bufferedPartBrush
            );
        }
        else
        {
            g.target->FillRectangle(
                D2D1::RectF(
                    timeTextWidth + viewedPartWidth,
                    GetHeight() / 2.0f - 1.0f,
                    timeTextWidth + bufferedPartWidth,
                    GetHeight() / 2.0f + 1.0f),
                _viewedBufferingPartBrush
            );
        }

        if (GetMouseInside())
        {
            g.target->FillEllipse(
                D2D1::Ellipse(
                    D2D1::Point2F(
                        timeTextWidth + viewedPartWidth,
                        GetHeight() / 2.0f
                    ),
                    5.0f,
                    5.0f
                ),
                _viewedPartBrush
            );

            //g->FillRectangle(
            //    D2D1::RectF(
            //        timeTextWidth + viewedPartWidth - 2.0f,
            //        GetHeight() / 2.0f - 10.0f,
            //        timeTextWidth + viewedPartWidth + 2.0f,
            //        GetHeight() / 2.0f + 10.0f
            //    ),
            //    _viewedPartBrush
            //);
        }

        // Create time strings
        std::wstringstream timeStr;
        std::wstring currentTimeStr;
        std::wstring durationStr;

        int h = ztime::Game().GetTime(HOURS);
        int m = ztime::Game().GetTime(MINUTES) % 60;
        int s = ztime::Game().GetTime(SECONDS) % 60;
        if (h > 0) timeStr << h << ":";
        if (m < 10) timeStr << "0" << m << ":";
        else timeStr << m << ":";
        if (s < 10) timeStr << "0" << s;
        else timeStr << s;
        currentTimeStr = timeStr.str();

        h = _duration / 3600000000LL;
        m = (_duration / 60000000LL) % 60;
        s = (_duration / 1000000LL) % 60;
        timeStr.str(L"");
        timeStr.clear();
        if (h > 0) timeStr << h << ":";
        if (m < 10) timeStr << "0" << m << ":";
        else timeStr << m << ":";
        if (s < 10) timeStr << "0" << s;
        else timeStr << s;
        durationStr = timeStr.str();

        // Draw time strings
        g.target->DrawText(
            currentTimeStr.c_str(),
            currentTimeStr.length(),
            _dwriteTextFormat,
            D2D1::RectF(
                0.0f,
                (GetHeight() - _textHeight) * 0.5f,
                timeTextWidth,
                (GetHeight() + _textHeight) * 0.5f
            ),
            _textBrush
        );

        g.target->DrawText(
            durationStr.c_str(),
            durationStr.length(),
            _dwriteTextFormat,
            D2D1::RectF(
                GetWidth() - timeTextWidth,
                (GetHeight() - _textHeight) * 0.5f,
                GetWidth(),
                (GetHeight() + _textHeight) * 0.5f
            ),
            _textBrush
        );

        // Draw hover time string
        if (GetMouseInside())
        {
            int xPos = GetMousePosX() - timeTextWidth;
            if (xPos < 0) xPos = 0;
            if (xPos > seekBarWidth) xPos = seekBarWidth;
            double xPosNorm = xPos / (double)seekBarWidth;
            long long int hoverTime = _duration * xPosNorm;

            h = (hoverTime / 3600000000LL);
            m = (hoverTime / 60000000LL) % 60;
            s = (hoverTime / 1000000LL) % 60;
            timeStr.str(L"");
            timeStr.clear();
            if (h > 0) timeStr << h << ":";
            if (m < 10) timeStr << "0" << m << ":";
            else timeStr << m << ":";
            if (s < 10) timeStr << "0" << s;
            else timeStr << s;
            std::wstring hoverTimeStr = timeStr.str();

            D2D1_RECT_F strRect = D2D1::RectF(
                (timeTextWidth + xPos) - timeTextWidth * 0.5f,
                (GetHeight() + _textHeight) * 0.5f,
                (timeTextWidth + xPos) + timeTextWidth * 0.5f,
                (GetHeight() + _textHeight) * 0.5f
            );

            g.target->DrawText(
                hoverTimeStr.c_str(),
                hoverTimeStr.length(),
                _dwriteTextFormat,
                D2D1::RectF(
                    (timeTextWidth + xPos) - timeTextWidth * 0.5f,
                    (GetHeight() * 0.5f) - _margins - _textHeight,
                    (timeTextWidth + xPos) + timeTextWidth * 0.5f,
                    (GetHeight() * 0.5f) - _margins
                ),
                _textBrush
            );
        }
    }

    void _OnResize(int width, int height)
    {

    }

    ComponentBase* _OnMouseMove(int x, int y)
    {
        return this;
    }

    void _OnMouseLeave()
    {
        
    }

    void _OnMouseEnter()
    {
        _mouseHoverStart = ztime::Main();
    }

    ComponentBase* _OnLeftPressed(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnLeftReleased(int x, int y)
    {
        int timeTextWidth = ceilf(_maxTimeWidth) + _margins * 2;
        int seekBarWidth = GetWidth() - timeTextWidth * 2;
        int xPos = GetMousePosX() - timeTextWidth;
        if (xPos >= 0 && xPos <= seekBarWidth)
        {
            double xPosNorm = xPos / (double)seekBarWidth;
            long long int hoverTime = _duration * xPosNorm;
            _selectedTime = hoverTime;
        }
        return this;
    }

    ComponentBase* _OnRightPressed(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnRightReleased(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnWheelUp(int x, int y)
    {
        return this;
    }

    ComponentBase* _OnWheelDown(int x, int y)
    {
        return this;
    }

public:
    std::list<ComponentBase*> GetChildren()
    {
        return std::list<ComponentBase*>();
    }

    std::list<ComponentBase*> GetAllChildren()
    {
        return std::list<ComponentBase*>();
    }

    const char* GetName() const { return "seek_bar"; }
#pragma endregion

private:
    long long int _duration = 0;
    long long int _buffered = 0;

    bool _held = false;
    TimePoint _selectedTime = -1;
    float _textHeight = 0.0f;
    float _maxTimeWidth = 0.0f;

    float _margins = 5.0f;

    // Resources
    ID2D1SolidColorBrush* _viewedPartBrush = nullptr;
    ID2D1SolidColorBrush* _viewedBufferingPartBrush = nullptr;
    ID2D1SolidColorBrush* _bufferedPartBrush = nullptr;
    ID2D1SolidColorBrush* _remainingPartBrush = nullptr;
    ID2D1SolidColorBrush* _textBrush = nullptr;
    ID2D1SolidColorBrush* _textBackgroundBrush = nullptr;

    IDWriteFactory* _dwriteFactory = nullptr;
    IDWriteTextFormat* _dwriteTextFormat = nullptr;
    IDWriteTextLayout* _dwriteTextLayout = nullptr;

    TimePoint _mouseHoverStart = 0;

public:
    SeekBar(long long int duration)
    {
        _duration = duration;

        // Create text rendering resources
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&_dwriteFactory)
        );

        _dwriteFactory->CreateTextFormat(
            L"Calibri",
            NULL,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            14.0f,
            L"en-us",
            &_dwriteTextFormat
        );
        _dwriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

        _dwriteFactory->CreateTextLayout(
            L"000:00:00",
            9,
            _dwriteTextFormat,
            1000,
            0,
            &_dwriteTextLayout
        );

        DWRITE_TEXT_METRICS metrics;
        _dwriteTextLayout->GetMetrics(&metrics);
        _textHeight = metrics.height;
        _maxTimeWidth = metrics.width;
    }
    ~SeekBar()
    {
        // Release resources
        SafeFullRelease((IUnknown**)&_viewedPartBrush);
        SafeFullRelease((IUnknown**)&_viewedBufferingPartBrush);
        SafeFullRelease((IUnknown**)&_bufferedPartBrush);
        SafeFullRelease((IUnknown**)&_remainingPartBrush);
        SafeFullRelease((IUnknown**)&_textBrush);
        SafeFullRelease((IUnknown**)&_textBackgroundBrush);
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

    void SetBufferedDuration(Duration duration)
    {
        _buffered = duration.GetDuration();
        if (_buffered < 0)
        {
            _buffered = 0;
        }
        else if (_buffered > _duration)
        {
            _buffered = _duration;
        }
    }

    Duration GetDuration() const
    {
        return _duration;
    }

    void SetDuration(Duration duration)
    {
        _duration = duration.GetDuration();
    }
};