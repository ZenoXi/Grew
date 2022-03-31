#pragma once

#include "ComponentBase.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"

#include "LocalFileDataProvider.h"
#include "ResourceManager.h"

#include "functions.h"

#include <sstream>

namespace zcom
{
    class MediaQueueItem : public Base
    {
#pragma region base_class
    private:
        void _OnUpdate()
        {
            _mainPanel->Update();

            // Update status text
            if (_dataProvider && !_dataProvider->Initializing() && !_initDone)
            {
                _initDone = true;
                if (_dataProvider->InitFailed())
                {
                    _statusLabel->SetText(L"Initialization failed.");
                }
                else
                {
                    _initSuccess = true;
                    SetDuration(_dataProvider->MaxMediaDuration());
                    SetButtonVisibility(BTN_PLAY | BTN_DELETE);
                }
            }
        }

        void _OnDraw(Graphics g)
        {
            g.target->DrawBitmap(
                _mainPanel->Draw(g),
                D2D1::RectF(
                    _mainPanel->GetX(),
                    _mainPanel->GetY(),
                    _mainPanel->GetX() + _mainPanel->GetWidth(),
                    _mainPanel->GetY() + _mainPanel->GetHeight()
                ),
                _mainPanel->GetOpacity()
            );
        }

        void _OnResize(int width, int height)
        {
            _mainPanel->SetSize(GetWidth(), GetHeight());
            _mainPanel->Resize();
        }

        EventTargets _OnMouseMove(int x, int y)
        {
            _mainPanel->OnMouseMove(x, y);
            return EventTargets().Add(this, x, y);
        }

        void _OnMouseLeave()
        {
            _mainPanel->OnMouseLeave();
        }

        void _OnMouseEnter()
        {
            _mainPanel->OnMouseEnter();
        }

        void _OnMouseLeaveArea()
        {
            _mainPanel->OnMouseLeaveArea();
        }

        void _OnMouseEnterArea()
        {
            _mainPanel->OnMouseEnterArea();
        }

        EventTargets _OnLeftPressed(int x, int y)
        {
            _mainPanel->OnLeftPressed(x, y);
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnLeftReleased(int x, int y)
        {
            _mainPanel->OnLeftReleased(x, y);
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnRightPressed(int x, int y)
        {
            _mainPanel->OnRightPressed(x, y);
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnRightReleased(int x, int y)
        {
            _mainPanel->OnRightReleased(x, y);
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnWheelUp(int x, int y)
        {
            _mainPanel->OnWheelUp(x, y);
            return EventTargets();
        }

        EventTargets _OnWheelDown(int x, int y)
        {
            _mainPanel->OnWheelDown(x, y);
            return EventTargets();
        }

    public:
        const char* GetName() const { return "media_queue_item"; }
#pragma endregion

    public:
        enum Buttons
        {
            BTN_PLAY = 0x1,
            BTN_STOP = 0x2,
            BTN_DELETE = 0x4
        };

    private:
        std::unique_ptr<Panel> _mainPanel = nullptr;
        std::unique_ptr<Label> _filenameLabel = nullptr;
        std::unique_ptr<Label> _statusLabel = nullptr;
        std::unique_ptr<Button> _playButton = nullptr;
        std::unique_ptr<Button> _deleteButton = nullptr;
        std::unique_ptr<Button> _stopButton = nullptr;

        bool _initDone = false;
        bool _initSuccess = false;
        std::unique_ptr<LocalFileDataProvider> _dataProvider = nullptr;
        std::wstring _filenameStr = L"";
        std::wstring _durationStr = L"";
        Duration _duration = 0;
        bool _nowPlaying = false;
        std::wstring _customStatus = L"";

        bool _delete = false;
        bool _play = false;
        bool _stop = false;

        int32_t _mediaId = -1;
        int64_t _userId = -1;

        static int32_t _ID_COUNTER;
        static int32_t _GenerateCallbackId()
        {
            return _ID_COUNTER++;
        }
        int32_t _callbackId;

    public:
        MediaQueueItem()
        {
            _callbackId = MediaQueueItem::_GenerateCallbackId();

            _filenameLabel = std::make_unique<Label>(L"");
            _filenameLabel->SetParentWidthPercent(1.0f);
            _filenameLabel->SetBaseSize(-150, 25);
            _filenameLabel->SetMargins({ 5.f, 0.f, 5.f, 0.f });
            _filenameLabel->SetFont(L"Segoe UI");
            _filenameLabel->SetFontSize(14.0f);
            _filenameLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _filenameLabel->SetCutoff(L"...");

            _statusLabel = std::make_unique<Label>(L"Initializing...");
            _statusLabel->SetBaseSize(150, 25);
            _statusLabel->SetHorizontalOffsetPixels(-25);
            _statusLabel->SetHorizontalAlignment(Alignment::END);
            _statusLabel->SetMargins({ 0.f, 0.f, 5.f, 0.f });
            _statusLabel->SetFont(L"Segoe UI");
            _statusLabel->SetFontSize(12.0f);
            _statusLabel->SetFontColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
            _statusLabel->SetHorizontalTextAlignment(TextAlignment::TRAILING);
            _statusLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _statusLabel->SetCutoff(L"...");

            _playButton = std::make_unique<Button>();
            _playButton->SetBaseSize(25, 25);
            _playButton->SetHorizontalAlignment(Alignment::END);
            _playButton->SetActivation(ButtonActivation::RELEASE);
            _playButton->SetOnActivated([&]() { _play = true; });
            _playButton->SetBackgroundImage(ResourceManager::GetImage("item_play"));
            _playButton->SetVisible(false);

            _deleteButton = std::make_unique<Button>();
            _deleteButton->SetBaseSize(25, 25);
            _deleteButton->SetHorizontalAlignment(Alignment::END);
            _deleteButton->SetActivation(ButtonActivation::RELEASE);
            _deleteButton->SetOnActivated([&]() { _delete = true; });
            _deleteButton->SetBackgroundImage(ResourceManager::GetImage("item_delete"));
            _deleteButton->SetVisible(false);

            _stopButton = std::make_unique<Button>();
            _stopButton->SetBaseSize(25, 25);
            _stopButton->SetHorizontalAlignment(Alignment::END);
            _stopButton->SetActivation(ButtonActivation::RELEASE);
            _stopButton->SetOnActivated([&]() { _stop = true; });
            _stopButton->SetBackgroundImage(ResourceManager::GetImage("item_stop"));
            _stopButton->SetVisible(false);

            _mainPanel = std::make_unique<Panel>();
            _mainPanel->SetSize(GetWidth(), GetHeight());
            _mainPanel->AddItem(_filenameLabel.get());
            _mainPanel->AddItem(_statusLabel.get());
            _mainPanel->AddItem(_playButton.get());
            _mainPanel->AddItem(_deleteButton.get());
            _mainPanel->AddItem(_stopButton.get());
            SetButtonVisibility(BTN_DELETE);
        }
        MediaQueueItem(std::wstring path) : MediaQueueItem()
        {
            // Extract only the filename
            _filenameStr = L"";
            for (int i = path.length() - 1; i >= 0; i--)
            {
                if (path[i] == '\\' || path[i] == '/')
                {
                    break;
                }
                _filenameStr += path[i];
            }
            std::reverse(_filenameStr.begin(), _filenameStr.end());
            _filenameLabel->SetText(_filenameStr);

            // Start processing the file
            _dataProvider = std::make_unique<LocalFileDataProvider>(wstring_to_string(path));
        }
        ~MediaQueueItem()
        {

        }
        MediaQueueItem(MediaQueueItem&&) = delete;
        MediaQueueItem& operator=(MediaQueueItem&&) = delete;
        MediaQueueItem(const MediaQueueItem&) = delete;
        MediaQueueItem& operator=(const MediaQueueItem&) = delete;

        int32_t GetCallbackId()
        {
            return _callbackId;
        }

        bool Initialized()
        {
            return !_dataProvider->Initializing();
        }

        bool InitSuccess()
        {
            return !_dataProvider->InitFailed();
        }

        LocalFileDataProvider* CopyDataProvider()
        {
            return new LocalFileDataProvider(_dataProvider.get());
        }

        LocalFileDataProvider* DataProvider()
        {
            return _dataProvider.get();
        }

        //void SetNowPlaying(bool nowPlaying)
        //{
        //    if (_initSuccess && (nowPlaying != _nowPlaying))
        //    {
        //        _nowPlaying = nowPlaying;
        //        if (nowPlaying)
        //        {
        //            SetButtonVisibility(BTN_STOP);
        //        }
        //        else
        //        {
        //            SetButtonVisibility(BTN_PLAY | BTN_DELETE);
        //        }
        //    }
        //}

        void SetCustomStatus(std::wstring statusString)
        {
            if (_customStatus != statusString)
            {
                _customStatus = statusString;
                if (_customStatus.empty())
                    _statusLabel->SetText(_durationStr);
                else
                    _statusLabel->SetText(_customStatus);
            }
        }

        std::wstring GetCustomStatus() const
        {
            return _customStatus;
        }
        
        // If true, this media should be played
        bool Play()
        {
            bool val = _play;
            _play = false;
            return val;
        }

        // If true, the playback should be aborted
        bool Stop()
        {
            bool val = _stop;
            _stop = false;
            return val;
        }

        // If true, this object should be destroyed
        bool Delete()
        {
            bool val = _delete;
            _delete = false;
            return val;
        }

        std::wstring GetFilename() const
        {
            return _filenameStr;
        }

        void SetFilename(std::wstring filename)
        {
            _filenameStr = filename;
            _filenameLabel->SetText(_filenameStr);
        }

        Duration GetDuration() const
        {
            return _duration;
        }

        void SetDuration(Duration duration)
        {
            _duration = duration;
            int64_t h = _duration.GetDuration(HOURS);
            int64_t m = _duration.GetDuration(MINUTES) % 60;
            int64_t s = _duration.GetDuration(SECONDS) % 60;

            // Format status label
            std::wstringstream timeStr;
            timeStr.str(L"");
            timeStr.clear();
            if (h > 0) timeStr << h << ":";
            if (m < 10) timeStr << "0" << m << ":";
            else timeStr << m << ":";
            if (s < 10) timeStr << "0" << s;
            else timeStr << s;
            _durationStr = timeStr.str();

            if (_customStatus.empty())
                _statusLabel->SetText(_durationStr);
        }

        int32_t GetMediaId() const
        {
            return _mediaId;
        }

        void SetMediaId(int32_t id)
        {
            _mediaId = id;
        }

        int64_t GetUserId() const
        {
            return _userId;
        }

        void SetUserId(int64_t id)
        {
            _userId = id;
        }

        void SetButtonVisibility(int buttons)
        {
            int offset = 0;

            // Delete button
            if (buttons & BTN_DELETE)
            {
                _deleteButton->SetVisible(true);
                _deleteButton->SetHorizontalOffsetPixels(offset);
                offset -= 25;
            }
            else
            {
                _deleteButton->SetVisible(false);
            }

            // Stop button
            if (buttons & BTN_STOP)
            {
                _stopButton->SetVisible(true);
                _stopButton->SetHorizontalOffsetPixels(offset);
                offset -= 25;
            }
            else
            {
                _stopButton->SetVisible(false);
            }

            // Play button
            if (buttons & BTN_PLAY)
            {
                _playButton->SetVisible(true);
                _playButton->SetHorizontalOffsetPixels(offset);
                offset -= 25;
            }
            else
            {
                _playButton->SetVisible(false);
            }

            // Status label
            _statusLabel->SetHorizontalOffsetPixels(offset);
            offset -= 150;

            // Filename label
            _filenameLabel->SetBaseWidth(offset);

            _mainPanel->Resize();
        }
    };
}