#pragma once

#include "ComponentBase.h"
#include "Panel.h"
#include "BottomControlPanel.h"
#include "SeekBar.h"
#include "VolumeSlider.h"
#include "PlayButton.h"

// Class for storing the UI elements
class UILayout
{
public:
    Canvas componentCanvas;
    Panel* controlBar = nullptr;
    SeekBar* seekBar = nullptr;
    VolumeSlider* volumeSlider = nullptr;
    PlayButton* playButton = nullptr;
    BottomControlPanel* bottomControlPanel = nullptr;

    UILayout(int width, int height) : componentCanvas(width, height) {}

    void Create(DisplayWindow& window, MediaPlayer* player)
    {
        window.AddMouseHandler(&componentCanvas);

        controlBar = new Panel();
        controlBar->SetParentWidthPercent(1.0f);
        controlBar->SetParentHeightPercent(1.0f);
        controlBar->SetBackgroundColor(D2D1::ColorF::ColorF(0.0f, 0.0f, 0.0f, 0.7f));

        seekBar = new SeekBar(player->GetDuration().GetDuration());
        seekBar->SetParentWidthPercent(1.0f);
        seekBar->SetBaseWidth(-20);
        seekBar->SetBaseHeight(50);
        seekBar->SetHorizontalOffsetPercent(0.5f);

        volumeSlider = new VolumeSlider(player->GetVolume());
        volumeSlider->SetBaseWidth(150);
        volumeSlider->SetBaseHeight(20);
        volumeSlider->SetHorizontalOffsetPixels(10);
        volumeSlider->SetVerticalOffsetPixels(50);

        playButton = new PlayButton();
        playButton->SetBaseWidth(40);
        playButton->SetBaseHeight(40);
        playButton->SetHorizontalOffsetPercent(0.5f);
        playButton->SetVerticalOffsetPixels(50);
        playButton->SetPaused(true);

        controlBar->AddItem(seekBar);
        controlBar->AddItem(volumeSlider);
        controlBar->AddItem(playButton);

        bottomControlPanel = new BottomControlPanel(controlBar);
        bottomControlPanel->SetParentWidthPercent(1.0f);
        bottomControlPanel->SetBaseHeight(100);
        bottomControlPanel->SetVerticalAlignment(Alignment::END);

        componentCanvas.AddComponent(bottomControlPanel);
        //componentCanvas.AddComponent(controlBar);
        componentCanvas.Resize(window.width, window.height);

        //Resize(window.width, window.height);
    }

    void Resize(int width, int height)
    {
        //componentCanvas.Resize(width, height);

        //controlBar->SetPosition(0, height - 100);
        //controlBar->SetSize(width, 100);

        //seekBar->SetSize(controlBar->GetWidth() - 20, 50.0f);
        //seekBar->SetPosition(10, 0);

        //volumeSlider->SetSize(150, 20);
        //volumeSlider->SetPosition(10, 50);

        //playButton->SetSize(40, 40);
        //playButton->SetPosition(controlBar->GetWidth() / 2 - 20, controlBar->GetHeight() - 50);

        //bottomControlPanel->Resize();
    }
};