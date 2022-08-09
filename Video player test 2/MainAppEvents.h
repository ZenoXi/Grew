#pragma once

struct PlaylistHandlerChangeEvent
{
    static const char* _NAME_() { return "playlist_handler_change"; }
};

struct UsersHandlerChangeEvent
{
    static const char* _NAME_() { return "users_handler_change"; }
};