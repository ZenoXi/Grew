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
#include <vector>
#include <string>
#include <memory>

#include "Options.h"
#include "OptionNames.h"
#include "BoolOptionAdapter.h"
#include "NetBase2.h"
#include "ResourceManager.h"
#include "DisplayWindow.h"
#include "App.h"
#include "Network.h"
#include "EntryScene.h"
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "ConnectScene.h"

#include "Event.h"

#pragma comment( lib,"Winmm.lib" )

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR cmdLine, INT)
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
    std::wcout << "Executable path: " << dir << '\n';

    if (!SetCurrentDirectory(runDir.data()))
    {
        std::cout << "Directory set failed\n";
        return -1;
    }

    TCHAR path[MAX_PATH] = { 0 };
    DWORD a = GetCurrentDirectory(MAX_PATH, path);
    std::wcout << "New working directory: " << path << '\n';

    // Read arguments
    std::vector<std::wstring> args;
    int argCount;
    LPWSTR* pArgs = CommandLineToArgvW(cmdLine, &argCount);
    for (int i = 0; i < argCount; i++)
    {
        args.push_back(std::wstring(pArgs[i]));
        std::wcout << args[i] << '\n';
    }
    LocalFree(pArgs);

    // Find -o flag
    std::wstring openFilePath;
    for (int i = 0; i < args.size(); i++)
    {
        if (args[i] == L"-o" && i + 1 < args.size())
        {
            openFilePath = args[i + 1];
            break;
        }
    }

    // Find -c flag
    std::wstring connectURL;
    for (int i = 0; i < args.size(); i++)
    {
        if (args[i] == L"-c" && i + 1 < args.size())
        {
            if (args[i + 1].substr(0, 7) == L"grew://")
                connectURL = args[i + 1].substr(7);
            else if (args[i + 1].substr(0, 5) == L"grew:")
                connectURL = args[i + 1].substr(5);
            break;
        }
    }

    // Initialize WSA 
    znet::WSAHolder holder = znet::WSAHolder();

    // Load options
    Options::Init();

    // Create window
    DisplayWindow window(hInst, cmdLine, L"class");

    // Load resources
    ResourceManager::Init("Resources/Images/resources.resc", window.gfx.GetDeviceContext());

    // Init network
    znet::Network::Init();

    // Init app
    App::Init(window);

    // Init appropriate scenes
    if (!openFilePath.empty())
    {
        // Start playback
        auto item = std::make_unique<PlaylistItem>(openFilePath);
        int64_t itemId = item->GetItemId();
        item->StartInitializing();
        App::Instance()->playlist.Request_AddItem(std::move(item));
        App::Instance()->playlist.Request_PlayItem(itemId);

        App::Instance()->InitScene(PlaybackScene::StaticName(), nullptr);
        App::Instance()->InitScene(PlaybackOverlayScene::StaticName(), nullptr);
    }
    else if (!connectURL.empty())
    {
        ConnectSceneOptions opt;
        opt.connectionString = connectURL;
        opt.owned = false;

        App::Instance()->InitScene(ConnectScene::StaticName(), &opt);
        App::Instance()->InitScene(PlaybackOverlayScene::StaticName(), nullptr);
    }
    else
    {
        std::wstring optStr = Options::Instance()->GetValue(OPTIONS_START_IN_PLAYLIST);
        bool startInPlaylist = BoolOptionAdapter(optStr).Value();
        if (!startInPlaylist)
            App::Instance()->InitScene(EntryScene::StaticName(), nullptr);
        App::Instance()->InitScene(PlaybackOverlayScene::StaticName(), nullptr);
    }

    Clock msgTimer = Clock(0);

    // Start app thread
    App::Start();

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