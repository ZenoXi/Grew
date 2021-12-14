#pragma once

#include "ComponentBase.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"

#include "LocalFileDataProvider.h"

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
        std::unique_ptr<Button> _cancelButton;

        std::unique_ptr<LocalFileDataProvider> _dataProvider;

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
            _filenameLabel->SetBaseSize(-35, 20);
            _filenameLabel->SetFont(L"Arial");
            _filenameLabel->SetFontSize(18.0f);
            _filenameLabel->SetVerticalTextAlignment(Alignment::CENTER);

            _statusLabel = std::make_unique<Label>(L"Initializing...");
            _statusLabel->SetParentWidthPercent(1.0f);
            _statusLabel->SetBaseSize(-35, 15);
            _statusLabel->SetVerticalOffsetPixels(20);
            _statusLabel->SetFont(L"Arial");
            _statusLabel->SetFontSize(12.0f);
            _statusLabel->SetFontColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
            _statusLabel->SetVerticalTextAlignment(Alignment::CENTER);

            _cancelButton = std::make_unique<Button>();
            _cancelButton->SetBaseSize(35, 35);
            _cancelButton->SetHorizontalAlignment(Alignment::END);

            _mainPanel = std::make_unique<Panel>();
            _mainPanel->SetSize(GetWidth(), GetHeight());
            _mainPanel->AddItem(_filenameLabel.get());
            _mainPanel->AddItem(_statusLabel.get());
            _mainPanel->AddItem(_cancelButton.get());
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

        LocalFileDataProvider* GetProvider()
        {
            return _dataProvider.get();
        }

        LocalFileDataProvider* ReleaseProvider()
        {
            return _dataProvider.release();
        }
    };
}