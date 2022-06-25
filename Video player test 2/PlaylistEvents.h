#pragma once

#include "GameTime.h"

#include <cstdint>
#include <string>

struct PlaylistChangedEvent
{
    static const char* _NAME_() { return "playlist_changed"; }
};

struct ItemAddEvent
{
    static const char* _NAME_() { return "item_add"; }

    int64_t id;
    std::wstring name;
    Duration duration;
};

struct ItemRemoveEvent
{
    static const char* _NAME_() { return "item_remove"; }

    int64_t id;
};

struct ItemPlayEvent
{
    static const char* _NAME_() { return "item_play"; }

    int64_t id;
};

struct PlaybackStopEvent
{
    static const char* _NAME_() { return "playback_stop"; }
};

struct ItemMoveEvent
{
    static const char* _NAME_() { return "item_move"; }

    int64_t id;
    int newSlot;
};