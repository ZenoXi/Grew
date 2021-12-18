#pragma once

#include "ComponentBase.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"

#include "LocalFileDataProvider.h"
#include "ResourceManager.h"

#include "functions.h"

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
            if (!_dataProvider->Initializing() && !_initDone)
            {
                _initDone = true;
                if (_dataProvider->InitFailed())
                {
                    _statusLabel->SetText(L"Initialization failed.");
                }
                else
                {
                    _initSuccess = true;

                    Duration mediaDuration = _dataProvider->MaxMediaDuration();
                    int64_t h = mediaDuration.GetDuration(HOURS);
                    int64_t m = mediaDuration.GetDuration(MINUTES) % 60;
                    int64_t s = mediaDuration.GetDuration(SECONDS) % 60;

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
                    _statusLabel->SetText(_durationStr);

                    // Show play button
                    _statusLabel->SetBaseSize(100, 25);
                    _statusLabel->SetHorizontalOffsetPixels(-50);
                    _playButton->SetVisible(true);
                    _mainPanel->Resize();
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

        Base* _OnMouseMove(int x, int y)
        {
            return _mainPanel->OnMouseMove(x, y);
        }

        void _OnMouseLeave()
        {
            _mainPanel->OnMouseLeave();
        }

        void _OnMouseEnter()
        {
            _mainPanel->OnMouseEnter();
        }

        Base* _OnLeftPressed(int x, int y)
        {
            return _mainPanel->OnLeftPressed(x, y);
        }

        Base* _OnLeftReleased(int x, int y)
        {
            return _mainPanel->OnLeftReleased(x, y);
        }

        Base* _OnRightPressed(int x, int y)
        {
            return _mainPanel->OnRightPressed(x, y);
        }

        Base* _OnRightReleased(int x, int y)
        {
            return _mainPanel->OnRightReleased(x, y);
        }

        Base* _OnWheelUp(int x, int y)
        {
            return _mainPanel->OnWheelUp(x, y);
        }

        Base* _OnWheelDown(int x, int y)
        {
            return _mainPanel->OnWheelDown(x, y);
        }

    public:
        std::list<Base*> GetChildren()
        {
            return std::list<Base*>();
        }

        std::list<Base*> GetAllChildren()
        {
            return std::list<Base*>();
        }

        Base* IterateTab()
        {
            if (!Selected())
                return this;
            else
                return nullptr;
        }

        const char* GetName() const { return "media_queue_item"; }
#pragma endregion

    private:
        std::unique_ptr<Panel> _mainPanel;
        std::unique_ptr<Label> _filenameLabel;
        std::unique_ptr<Label> _statusLabel;
        std::unique_ptr<Button> _playButton;
        std::unique_ptr<Button> _deleteButton;
        std::unique_ptr<Button> _stopButton;

        bool _initDone = false;
        bool _initSuccess = false;
        std::unique_ptr<LocalFileDataProvider> _dataProvider;
        std::wstring _durationStr = L"";
        bool _nowPlaying = false;

        bool _delete = false;
        bool _play = false;
        bool _stop = false;

    public:
        MediaQueueItem(std::wstring path)
        {
            // Extract only the filename
            std::wstring filename;
            for (int i = path.length() - 1; i >= 0; i--)
            {
                if (path[i] == '\\' || path[i] == '/')
                {
                    break;
                }
                filename += path[i];
            }
            std::reverse(filename.begin(), filename.end());

            _filenameLabel = std::make_unique<Label>(filename);
            _filenameLabel->SetParentWidthPercent(1.0f);
            _filenameLabel->SetBaseSize(-150, 25);
            _filenameLabel->SetMargins({ 5.f, 0.f, 5.f, 0.f });
            _filenameLabel->SetFont(L"Segoe UI");
            _filenameLabel->SetFontSize(14.0f);
            _filenameLabel->SetVerticalTextAlignment(Alignment::CENTER);
            _filenameLabel->SetCutoff(L"...");

            _statusLabel = std::make_unique<Label>(L"Initializing...");
            _statusLabel->SetBaseSize(125, 25);
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
            _playButton->SetHorizontalOffsetPixels(-25);
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
            _mainPanel->Resize();

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

        bool Initialized()
        {
            return !_dataProvider->Initializing();
        }

        bool InitSuccess()
        {
            return !_dataProvider->InitFailed();
        }

        LocalFileDataProvider* GetDataProvider()
        {
            return new LocalFileDataProvider(_dataProvider.get());
        }

        void SetNowPlaying(bool nowPlaying)
        {
            if (_initSuccess && (nowPlaying != _nowPlaying))
            {
                _nowPlaying = nowPlaying;
                if (nowPlaying)
                {
                    _statusLabel->SetBaseSize(125, 25);
                    _statusLabel->SetHorizontalOffsetPixels(-25);
                    _playButton->SetVisible(false);
                    _deleteButton->SetVisible(false);
                    _stopButton->SetVisible(true);
                }
                else
                {
                    _statusLabel->SetBaseSize(100, 25);
                    _statusLabel->SetHorizontalOffsetPixels(-50);
                    _playButton->SetVisible(true);
                    _deleteButton->SetVisible(true);
                    _stopButton->SetVisible(false);
                }
                _mainPanel->Resize();
            }
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
    };
}