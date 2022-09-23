#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>
#include "ChiliWin.h"
#include <Mmsystem.h>
#include <bitset>

#include "Options.h"
#include "NetBase2.h"
#include "ResourceManager.h"
#include "DisplayWindow.h"
#include "App.h"
#include "Network.h"
#include "EntryScene.h"

#include "Event.h"

#pragma comment( lib,"Winmm.lib" )

int WINAPI main(HINSTANCE hInst, HINSTANCE, LPWSTR pArgs, INT)
{
    // Initialize WSA 
    znet::WSAHolder holder = znet::WSAHolder();

    // Load options
    Options::Init();

    // Create window
    DisplayWindow window(hInst, pArgs, L"class");

    // Load resources
    ResourceManager::Init("Resources/Images/resources.resc", window.gfx.GetDeviceContext());

    // Initialize singleton objects
    znet::Network::Init();
    App::Init(window, EntryScene::StaticName());

    Clock msgTimer = Clock(0);

    // Main window loop
    while (true)
    {
        // Messages
        bool msgProcessed = window.ProcessMessages();
        WindowMessage wm = window.GetExitResult();
        if (!wm.handled)
            exit(0);

        window.HandleFullscreenChange();
        window.HandleCursorVisibility();

        // Limit cpu usage
        if (!msgProcessed)
        {
            // If no messages are received for 50ms or more, sleep to limit cpu usage.
            // This way we allow for full* mouse poll rate utilization when necessary.
            //
            // * the very first mouse move after a break will have a very small delay
            // which may be noticeable in certain situations (FPS games)
            msgTimer.Update();
            if (msgTimer.Now().GetTime(MILLISECONDS) >= 50)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else
        {
            msgTimer.Reset();
        }
        continue;
    }

    return 0;
}