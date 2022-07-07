#pragma once

#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "GameTime.h"
#include "ResourceManager.h"

#include <iostream>

namespace zcom
{
    struct NotificationInfo
    {
        Duration duration = Duration(3, SECONDS);
        std::wstring title = L"Notification";
        std::wstring text = L"Notification body goes here, the text is long to test newlines";
        D2D1_COLOR_F borderColor = D2D1::ColorF(0.25f, 0.25f, 0.25f);
    };

    class Notification : public Panel
    {
#pragma region base_class
    protected:
        void _OnUpdate()
        {
            Duration timeElapsed = ztime::Main() - _creationTime;
            if (timeElapsed > _showDuration)
            {
                timeElapsed -= _showDuration;
                if (timeElapsed > _fadeDuration)
                {
                    SetOpacity(0.0f);
                    _destroy = true;
                }
                else
                {
                    float fadeProgress = timeElapsed.GetDuration() / (float)_fadeDuration.GetDuration();
                    SetOpacity(1.0f - powf(fadeProgress, 0.5f));
                }
            }
            else
            {
                SetOpacity(1.0f);
            }
            Panel::_OnUpdate();
        }

    public:
        const char* GetName() const { return "notification"; }
#pragma endregion

    private:
        std::unique_ptr<Label> _titleLabel = nullptr;
        std::unique_ptr<Label> _bodyLabel = nullptr;
        std::unique_ptr<Button> _closeButton = nullptr;

        TimePoint _creationTime;
        Duration _showDuration;
        Duration _fadeDuration = Duration(250, MILLISECONDS);
        //Duration _fadeDuration = Duration(5, SECONDS);

        bool _destroy = false;

    public:
        Notification(int notificationWidth, NotificationInfo info = NotificationInfo{})
        {
            int marginsHorizontal = 5;
            int marginsVertical = 2;
            int textWidth = notificationWidth - marginsHorizontal * 2;

            zcom::PROP_Shadow shadowProps;
            shadowProps.offsetX = 1.0f;
            shadowProps.offsetY = 1.0f;
            shadowProps.blurStandardDeviation = 1.0f;
            shadowProps.color = D2D1::ColorF(0, 1.0f);

            // Calculate notification size
            float textHeight = 0.0f;
            if (!info.title.empty())
            {
                _titleLabel = std::make_unique<Label>(info.title);
                _titleLabel->SetOffsetPixels(0, 0);
                _titleLabel->SetBaseSize(textWidth - 20, 20);
                _titleLabel->SetFontSize(16.0f);
                _titleLabel->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD);
                _titleLabel->SetCutoff(L"...");
                _titleLabel->SetProperty(shadowProps);
                textHeight += 20.0f;

                _closeButton = std::make_unique<Button>();
                _closeButton->SetOffsetPixels(textWidth - 20, 0);
                _closeButton->SetBaseSize(20, 20);
                _closeButton->SetActivation(ButtonActivation::RELEASE);
                _closeButton->SetOnActivated([&]() { _destroy = true; });
                _closeButton->SetBackgroundImage(ResourceManager::GetImage("item_delete"));
                _closeButton->SetSelectedBorderColor(D2D1::ColorF(0, 0.0f));
            }
            if (!info.text.empty())
            {
                _bodyLabel = std::make_unique<Label>(info.text);
                _bodyLabel->SetOffsetPixels(0, textHeight);
                _bodyLabel->SetWidth(textWidth);
                _bodyLabel->SetWordWrap(true);
                float bodyTextHeight = _bodyLabel->GetTextHeight();
                textHeight += bodyTextHeight;
                _bodyLabel->SetBaseWidth(textWidth);
                _bodyLabel->SetBaseHeight(ceil(bodyTextHeight));
                _bodyLabel->SetFontColor(D2D1::ColorF(0.6f, 0.6f, 0.6f));
                _bodyLabel->SetFontStyle(DWRITE_FONT_STYLE_ITALIC);
                _bodyLabel->SetProperty(shadowProps);
            }

            int notificationHeight = (int)ceil(textHeight) + marginsVertical * 2;

            // Set main properties
            DeferLayoutUpdates();
            SetBaseSize(notificationWidth, notificationHeight);
            SetMargins({ marginsHorizontal, marginsVertical, marginsHorizontal, marginsVertical });
            if (_titleLabel)
                AddItem(_titleLabel.get());
            if (_closeButton)
                AddItem(_closeButton.get());
            if (_bodyLabel)
                AddItem(_bodyLabel.get());
            SetSize(notificationWidth, notificationHeight);
            ResumeLayoutUpdates();
            
            // Set fadeout vars
            _creationTime = ztime::Main();
            _showDuration = info.duration;

            // Set other properties
            SetBorderVisibility(true);
            SetBorderColor(info.borderColor);
            SetBackgroundColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
            SetCornerRounding(5.0f);
        }
        ~Notification()
        {
            ClearItems();
        }
        Notification(Notification&&) = delete;
        Notification& operator=(Notification&&) = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        bool Destroy() const
        {
            return _destroy;
        }
    };
}