#include "PlaylistEventHandler_None.h"
#include "Playlist.h"

PlaylistEventHandler_None::PlaylistEventHandler_None(Playlist_Internal* playlist)
    : IPlaylistEventHandler(playlist)
{
    // Reset playlist
    _playlist->Reset();
}