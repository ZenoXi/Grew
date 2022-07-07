#include "MenuPanel.h"
#include "Canvas.h"

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
    });

    _sceneCanvas->AddOnLeftPressed([&](const EventTargets* targets)
    {
        if (!GetVisible())
            return;
        if ((ztime::Main() - _showTime).GetDuration(MILLISECONDS) < 100)
            return;
        if (_openChildPanel)
            return;

        if (!targets->Contains(this) && !targets->Contains(_parentPanel))
        {
            Hide();
            if (_parentPanel)
                _parentPanel->OnChildClosed(targets);
        }
    });
}