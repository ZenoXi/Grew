#include "MenuPanel.h"
#include "Canvas.h"

void zcom::MenuPanel::Show(Canvas* sceneCanvas, RECT parentRect, MenuPanel* parentPanel)
{
    _sceneCanvas = sceneCanvas;
    _parentRect = parentRect;
    _parentPanel = parentPanel;
    _bounds = { 0, 0, _sceneCanvas->GetWidth(), _sceneCanvas->GetHeight() };

    _sceneCanvas->AddComponent(this);
    _AddHandlerToCanvas();

    _CalculatePlacement();
    // Place this panel above parent
    if (_parentPanel)
        SetZIndex(_parentPanel->GetZIndex() + 1);
    SetVisible(true);
    _showTime = ztime::Main();
}

void zcom::MenuPanel::Hide()
{
    _onHideEvent.InvokeAll();
    _sceneCanvas->RemoveComponent(this);
    _RemoveHandlerFromCanvas();

    if (_openChildPanel)
    {
        _openChildPanel->Hide();
        _openChildPanel = nullptr;
    }
    if (_hoveredItem)
        _hoveredItem->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
    SetVisible(false);
}

void zcom::MenuPanel::_AddHandlerToCanvas()
{
    _sceneCanvas->AddOnMouseMove([&](const EventTargets* targets)
    {
        // Stop scheduled hide
        if (_openChildPanel && targets->Contains(_openChildPanel))
        {
            _childShouldHide = false;
        }

        // Stop scheduled show
        if (_childToShow && !targets->Contains(this))
        {
            _childToShow = nullptr;
        }

        // Highlight parent menu item
        if (_openChildPanel /*&& targets->Contains(_openChildPanel)*/)
        {
            if (targets->Contains(_openChildPanel))
            {
                for (auto& it : _items)
                {
                    if (((MenuItem*)it.item)->GetMenuPanel() == _openChildPanel)
                    {
                        if (_hoveredItem)
                            _hoveredItem->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
                        _hoveredItem = (MenuItem*)it.item;
                        _hoveredItem->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f));
                        break;
                    }
                }
            }
            else if (!targets->Contains(this))
            {
                bool unhighlight = true;
                for (auto& it : _items)
                {
                    if (((MenuItem*)it.item)->GetMenuPanel() == _openChildPanel)
                    {
                        if (_hoveredItem == (MenuItem*)it.item)
                        {
                            unhighlight = false;
                            break;
                        }
                    }
                }
                if (unhighlight)
                {
                    if (_hoveredItem)
                    {
                        _hoveredItem->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
                        _hoveredItem = nullptr;
                    }
                }
            }


        }
        else if (!targets->Contains(this))
        {
            if (_hoveredItem)
            {
                _hoveredItem->SetBackgroundColor(D2D1::ColorF(0, 0.0f));
                _hoveredItem = nullptr;
            }
        }
        return false;
    }, { this, "" });

    _sceneCanvas->AddOnLeftPressed([&](const EventTargets* targets)
    {
        if (!GetVisible())
            return false;
        if ((ztime::Main() - _showTime).GetDuration(MILLISECONDS) < 100)
            return false;
        if (_openChildPanel)
            return false;

        if (!targets->Contains(this) && !targets->Contains(_parentPanel))
        {
            Hide();
            if (_parentPanel)
                _parentPanel->OnChildClosed(targets);
        }
        return false;
    }, { this, "" });

    _sceneCanvas->AddOnRightPressed([&](const EventTargets* targets)
    {
        if (!GetVisible())
            return false;
        if ((ztime::Main() - _showTime).GetDuration(MILLISECONDS) < 100)
            return false;
        if (_openChildPanel)
            return false;

        if (!targets->Contains(this) && !targets->Contains(_parentPanel))
        {
            Hide();
            if (_parentPanel)
                _parentPanel->OnChildClosed(targets);
        }
        return false;
    }, { this, "" });
}

void zcom::MenuPanel::_RemoveHandlerFromCanvas()
{
    _sceneCanvas->RemoveOnMouseMove({ this, "" });
    _sceneCanvas->RemoveOnLeftPressed({ this, "" });
    _sceneCanvas->RemoveOnRightPressed({ this, "" });
}