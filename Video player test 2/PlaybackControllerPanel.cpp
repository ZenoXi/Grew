#include "PlaybackControllerPanel.h"
#include "App.h"

void zcom::PlaybackControllerPanel::_SetCursorVisibility(bool visible)
{
    App::Instance()->window.SetCursorVisibility(visible);
}