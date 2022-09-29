#define _CRT_SECURE_NO_WARNINGS

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

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR pArgs, INT)
{
    // Create console
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    // Set working directory
    std::wstring dir;
    dir.resize(MAX_PATH);
    GetModuleFileName(NULL, dir.data(), MAX_PATH);
    auto pos = dir.find_last_of(L"\\/");
    std::wstring runDir = dir.substr(0, pos);
    std::wcout << "Executable path:" << dir << '\n';

    if (!SetCurrentDirectory(runDir.data()))
    {
        std::cout << "Directory set failed\n";
        return -1;
    }

    TCHAR path[MAX_PATH] = { 0 };
    DWORD a = GetCurrentDirectory(MAX_PATH, path);
    std::wcout << "New working directory: " << path << '\n';

    // Initialize WSA 
    znet::WSAHolder holder = znet::WSAHolder();

    // Load options
    Options::Init();

    // Create window
    DisplayWindow window(hInst, pArgs, L"class");

    // Load resources
    ResourceManager::Init("Resources/Images/resources.resc", window.gfx.GetDeviceContext());

    // Init network
    znet::Network::Init();

    // Start app
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