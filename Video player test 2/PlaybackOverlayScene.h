#pragma once

#include "Scene.h"
#include "PlaybackScene.h"

#include "Label.h"
#include "MediaQueueItem.h"

#include "FileDialog.h"

struct PlaybackOverlaySceneOptions : public SceneOptionsBase
{

};

class PlaybackOverlayScene : public Scene
{
    PlaybackScene* _playbackScene = nullptr;
    int _currentlyPlaying = -1;

    std::unique_ptr<zcom::Panel> _playbackQueuePanel = nullptr;
    std::vector<std::unique_ptr<zcom::MediaQueueItem>> _readyItems;
    std::vector<std::unique_ptr<zcom::MediaQueueItem>> _loadingItems;
    bool _addingFile = false;
    AsyncFileDialog* _fileDialog = nullptr;
    std::unique_ptr<zcom::Button> _addFileButton = nullptr;
    std::unique_ptr<zcom::Button> _closeOverlayButton = nullptr;

public:
    PlaybackOverlayScene();

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Focus();
    void _Unfocus();
    void _Update();
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

    const char* GetName() const { return "playback_overlay"; }
    static const char* StaticName() { return "playback_overlay"; }

private:
    void _RearrangeQueuePanel();
};