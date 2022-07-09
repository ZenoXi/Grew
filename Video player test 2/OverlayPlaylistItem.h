#pragma once

#include "ComponentBase.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"

#include "ResourceManager.h"

#include <sstream>

namespace zcom
{
    class OverlayPlaylistItem : public Panel
    {
    public:
        enum Buttons
        {
            BTN_PLAY = 0x1,
            BTN_STOP = 0x2,
            BTN_DELETE = 0x4
        };

    public:
        OverlayPlaylistItem(int64_t itemId)
        {
            _itemId = itemId;

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

            AddItem(_filenameLabel.get());
            AddItem(_statusLabel.get());
            AddItem(_playButton.get());
            AddItem(_deleteButton.get());
            AddItem(_stopButton.get());
            SetButtonVisibility(BTN_DELETE);
        }
        ~OverlayPlaylistItem()
        {
            ClearItems();
        }
        OverlayPlaylistItem(OverlayPlaylistItem&&) = delete;
        OverlayPlaylistItem& operator=(OverlayPlaylistItem&&) = delete;
        OverlayPlaylistItem(const OverlayPlaylistItem&) = delete;
        OverlayPlaylistItem& operator=(const OverlayPlaylistItem&) = delete;

        int64_t GetItemId() const
        {
            return _itemId;
        }

        void HostMissing(bool value)
        {
            _hostMissing = value;
        }

        bool HostMissing() const
        {
            return _hostMissing;
        }

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

        // If true, this item should be played
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
        }

    private:
        std::unique_ptr<Label> _filenameLabel = nullptr;
        std::unique_ptr<Label> _statusLabel = nullptr;
        std::unique_ptr<Button> _playButton = nullptr;
        std::unique_ptr<Button> _deleteButton = nullptr;
        std::unique_ptr<Button> _stopButton = nullptr;

        std::wstring _filenameStr = L"";
        std::wstring _durationStr = L"";
        Duration _duration = 0;
        std::wstring _customStatus = L"";

        bool _delete = false;
        bool _play = false;
        bool _stop = false;

        int64_t _itemId = -1;
        bool _hostMissing = false;

#pragma region base_class
    protected:

    public:
        const char* GetName() const { return "overlay_playlist_item"; }
#pragma endregion
    };
}