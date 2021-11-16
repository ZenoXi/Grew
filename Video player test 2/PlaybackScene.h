#pragma once

#include "Scene.h"

#include "MediaPlayer.h"

#include "ComponentBase.h"
#include "Panel.h"
#include "BottomControlPanel.h"
#include "SeekBar.h"
#include "VolumeSlider.h"
#include "PlayButton.h"

struct PlaybackSceneOptions : public SceneOptionsBase
{
    PlaybackMode mode;

    std::string fileName = "";
    std::string ip = "";
    uint16_t port = 0;
};

class PlaybackScene : public Scene
{
    MediaPlayer* _player = nullptr;

    Panel* _controlBar = nullptr;
    SeekBar* _seekBar = nullptr;
    VolumeSlider* _volumeSlider = nullptr;
    PlayButton* _playButton = nullptr;
    BottomControlPanel* _bottomControlPanel = nullptr;

public:
    PlaybackScene();

    void _Init(const SceneOptionsBase* options);
    void _Uninit();
    void _Update();
    ID2D1Bitmap1* _Draw(Graphics g);
    void _Resize(int width, int height);

    const char* GetName() const { return "playback_scene"; }
};