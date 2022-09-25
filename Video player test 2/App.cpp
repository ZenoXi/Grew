#include "App.h"

#include "TopMenuScene.h"
#include "OverlayScene.h"
#include "SettingsScene.h"
#include "EntryScene.h"
#include "ConnectScene.h"
#include "StartServerScene.h"
#include "PlaybackScene.h"
#include "PlaybackOverlayScene.h"
#include "OpenPlaylistScene.h"
#include "SavePlaylistScene.h"
#include "ManagePlaylistsScene.h"

#include "PlaylistEventHandler_None.h"
#include "PlaylistEventHandler_Offline.h"
#include "PlaylistEventHandler_Client.h"
#include "PlaylistEventHandler_Server.h"

#include "UsersEventHandler_Offline.h"
#include "UsersEventHandler_Client.h"
#include "UsersEventHandler_Server.h"

#include "ClientManager.h"
#include "ServerManager.h"

#include "BoolOptionAdapter.h"

App* App::_instance = nullptr;

App::App(DisplayWindow& dw, std::string startScene)
  : window(dw),
    _networkModeEventReceiver(&events)
{
    dw.mouse = &mouse;
    dw.AddMouseHandler(&mouseManager);
    dw.AddKeyboardHandler(&keyboardManager);

    _serverPlaylist.SetEventHandler<PlaylistEventHandler_None>();

    //// Start player
    //std::cout << "Application mode ([s]erver/[c]lient/[o]ffline): ";
    //std::string mode;
    //std::cin >> mode;

    //DisplayWindow::WINDOW_NAME = L"Offline mode";
    //if (mode == "o" || mode == "offline")
    //{
    //    //player = new MediaPlayer("E:/aots4e5comp.m4v", 0);
    //    player = new MediaPlayer(0, "E:/aots4e5.mkv");
    //}
    //else if (mode == "s" || mode == "server")
    //{
    //    std::cout << "Enter the port: ";
    //    USHORT port;
    //    std::cin >> port;
    //    std::cout << "Filename: ";
    //    std::string filename;
    //    std::cin >> filename;
    //    player = new MediaPlayer(filename, port);
    //    //player = new MediaPlayer("E:/aots4e5comp.m4v", 54000);
    //    //player = new MediaPlayer("E:/blet.mp4", 54000);
    //    DisplayWindow::WINDOW_NAME = L"Server mode";
    //}
    //else if (mode == "c" || mode == "client")
    //{
    //    std::cout << "Enter the IP: ";
    //    std::string ip;
    //    std::cin >> ip;
    //    std::cout << "Enter the port: ";
    //    USHORT port;
    //    std::cin >> port;
    //    player = new MediaPlayer(ip, port);
    //    //player = new MediaPlayer("127.0.0.1", 54000, 0);
    //    DisplayWindow::WINDOW_NAME = L"Client mode";
    //}

    // Initialize layout
    //layout.Create(window, player);
}

void App::Init(DisplayWindow& dw, std::string startScene)
{
    if (!_instance)
    {
        _instance = new App(dw, startScene);
    }

    // Top menu scene is always initialized
    Instance()->_topMenuScene = new TopMenuScene(_instance);
    Instance()->_topMenuScene->Init(nullptr);
    Instance()->_topMenuScene->Focus();
    Instance()->_topMenuScene->Resize(Instance()->MenuWidth(), Instance()->MenuHeight());
    Instance()->mouseManager.SetTopMenuHeight(Instance()->MenuHeight());

    // Overlay scene is always initialized and on top
    Instance()->_overlayScene = new OverlayScene(_instance);
    Instance()->_overlayScene->Init(nullptr);
    Instance()->_overlayScene->Focus();

    // Add scenes
    Instance()->_scenes.push_back(new SettingsScene(_instance));
    Instance()->_scenes.push_back(new EntryScene(_instance));
    Instance()->_scenes.push_back(new ConnectScene(_instance));
    Instance()->_scenes.push_back(new StartServerScene(_instance));
    Instance()->_scenes.push_back(new PlaybackScene(_instance));
    Instance()->_scenes.push_back(new PlaybackOverlayScene(_instance)); // Must be after PlaybackScene()
    Instance()->_scenes.push_back(new OpenPlaylistScene(_instance));
    Instance()->_scenes.push_back(new SavePlaylistScene(_instance));
    Instance()->_scenes.push_back(new ManagePlaylistsScene(_instance));

    // Init initial scenes
    std::wstring optStr = Options::Instance()->GetValue(OPTIONS_START_IN_PLAYLIST);
    bool startInPlaylist = BoolOptionAdapter(optStr).Value();
    if (!startInPlaylist)
        Instance()->InitScene(startScene, nullptr);
    Instance()->InitScene(PlaybackOverlayScene::StaticName(), nullptr);

    // Start main loop
    Instance()->_mainThreadController.Add("stop", sizeof(true));
    Instance()->_mainThreadController.Set("stop", false);
    Instance()->_mainThread = std::thread(&App::LoopThread, Instance());
}

App* App::Instance()
{
    return _instance;
}

bool App::SetScene(std::string name, SceneOptionsBase* options)
{
    for (int i = 0; i < _scenes.size(); i++)
    {
        if (_scenes[i]->GetName() == name)
        {
            _scenes[i]->Init(options);
            if (_currentSceneIndex != -1)
            {
                CurrentScene()->Uninit();
            }
            _currentSceneIndex = i;
            return true;
        }
    }
    return false;
}

Scene* App::CurrentScene()
{
    if (_currentSceneIndex == -1)
    {
        return nullptr;
    }
    return _scenes.at(_currentSceneIndex);
}

void App::Fullscreen(bool fullscreen)
{
    if (fullscreen == _fullscreen)
        return;

    _fullscreen = fullscreen;
    if (_fullscreen)
    {
        mouseManager.SetTopMenuVisibility(false);
        window.SetFullscreen(true);
        _sceneChanged = true;
    }
    else
    {
        mouseManager.SetTopMenuVisibility(true);
        window.SetFullscreen(false);
        _sceneChanged = true;
    }

}

bool App::InitScene(std::string name, SceneOptionsBase* options)
{
    Scene* scene = FindScene(name);
    if (!scene) return false;
    if (FindActiveScene(name)) return true;

    scene->Init(options);
    _activeScenes.insert(_activeScenes.begin(), scene);
    if (_activeScenes.size() == 1)
    {
        _activeScenes.back()->Focus();
    }
    _sceneChanged = true;
    return true;
}

bool App::ReinitScene(std::string name, SceneOptionsBase* options)
{
    Scene* scene = FindActiveScene(name);
    if (scene)
    {
        // Remove scene from uninit pending
        auto it = std::find(_scenesToUninitialize.begin(), _scenesToUninitialize.end(), name);
        if (it != _scenesToUninitialize.end())
            _scenesToUninitialize.erase(it);

        bool focused = scene->Focused();
        scene->Uninit();
        scene->Init(options);
        if (focused)
            scene->Focus();
        _sceneChanged = true;
        return true;
    }
    else
    {
        return InitScene(name, options);
    }
}

void App::UninitScene(std::string name)
{
    _scenesToUninitialize.push_back(name);
}

void App::_UninitScene(std::string name)
{
    Scene* scene = FindActiveScene(name);
    if (!scene)
        return;

    bool newFocus = false;
    if (_activeScenes.back() == scene)
        newFocus = true;

    scene->Uninit();
    _activeScenes.erase(std::find(_activeScenes.begin(), _activeScenes.end(), scene));
    if (newFocus && !_activeScenes.empty())
        _activeScenes.back()->Focus();
    _sceneChanged = true;
}

bool App::MoveSceneToFront(std::string name)
{
    int index = FindActiveSceneIndex(name);
    if (index == -1) return false;
    if (index == _activeScenes.size() - 1) return true;

    _activeScenes.back()->Unfocus();
    _activeScenes.push_back(_activeScenes[index]);
    _activeScenes.erase(_activeScenes.begin() + index);
    _activeScenes.back()->Focus();
    _sceneChanged = true;
    return true;
}

bool App::MoveSceneToBack(std::string name)
{
    int index = FindActiveSceneIndex(name);
    if (index == -1) return false;
    if (index == 0) return true;

    Scene* scene = _activeScenes[index];
    _activeScenes.erase(_activeScenes.begin() + index);
    _activeScenes.insert(_activeScenes.begin(), scene);
    if (index == _activeScenes.size() - 1)
    {
        scene->Unfocus();
        _activeScenes.back()->Focus();
    }
    _sceneChanged = true;
    return true;
}

bool App::MoveSceneUp(std::string name)
{
    int index = FindActiveSceneIndex(name);
    if (index == -1) return false;
    if (index == _activeScenes.size() - 1) return true;

    if (index == _activeScenes.size() - 2)
    {
        _activeScenes[index]->Focus();
        _activeScenes[index + 1]->Unfocus();
    }
    std::swap(_activeScenes[index], _activeScenes[index + 1]);
    _sceneChanged = true;
    return true;
}

bool App::MoveSceneDown(std::string name)
{
    int index = FindActiveSceneIndex(name);
    if (index == -1) return false;
    if (index == 0) return true;

    if (index == _activeScenes.size() - 1)
    {
        _activeScenes[index]->Unfocus();
        _activeScenes[index + -1]->Focus();
    }
    std::swap(_activeScenes[index], _activeScenes[index - 1]);
    _sceneChanged = true;
    return true;
}

bool App::MoveSceneBehind(std::string name, std::string behind)
{
    int sceneIndex = FindActiveSceneIndex(name);
    if (sceneIndex == -1) return false;
    int inFrontSceneIndex = FindActiveSceneIndex(behind);
    if (inFrontSceneIndex == -1) return false;
    if (sceneIndex < inFrontSceneIndex) return true;

    Scene* scene = _activeScenes[sceneIndex];
    _activeScenes.erase(_activeScenes.begin() + sceneIndex);
    _activeScenes.insert(_activeScenes.begin() + inFrontSceneIndex, scene);
    if (sceneIndex == _activeScenes.size() - 1)
    {
        scene->Unfocus();
        _activeScenes.back()->Focus();
    }
    _sceneChanged = true;
    return true;
}

bool App::MoveSceneInFront(std::string name, std::string inFront)
{
    int sceneIndex = FindActiveSceneIndex(name);
    if (sceneIndex == -1) return false;
    int behindSceneIndex = FindActiveSceneIndex(inFront);
    if (behindSceneIndex == -1) return false;
    if (sceneIndex > behindSceneIndex) return true;

    _activeScenes.insert(_activeScenes.begin() + behindSceneIndex + 1, _activeScenes[sceneIndex]);
    _activeScenes.erase(_activeScenes.begin() + sceneIndex);
    if (behindSceneIndex == _activeScenes.size() - 1)
    {
        _activeScenes[behindSceneIndex - 1]->Unfocus();
        _activeScenes.back()->Focus();
    }
    _sceneChanged = true;
    return true;
}

std::vector<Scene*> App::ActiveScenes()
{
    return _activeScenes;
}

Scene* App::FindScene(std::string name)
{
    for (auto scene : _scenes)
    {
        if (scene->GetName() == name)
        {
            return scene;
        }
    }
    return nullptr;
}

Scene* App::FindActiveScene(std::string name)
{
    for (auto scene : _activeScenes)
    {
        if (scene->GetName() == name)
        {
            return scene;
        }
    }
    return nullptr;
}

int App::FindSceneIndex(std::string name)
{
    for (int i = 0; i < _scenes.size(); i++)
    {
        if (_scenes[i]->GetName() == name)
        {
            return i;
        }
    }
    return -1;
}

int App::FindActiveSceneIndex(std::string name)
{
    for (int i = 0; i < _activeScenes.size(); i++)
    {
        if (_activeScenes[i]->GetName() == name)
        {
            return i;
        }
    }
    return -1;
}

void App::LoopThread()
{
    int framecounter = 0;
    Clock frameTimer = Clock(0);

    while (!_mainThreadController.Get<bool>("stop"))
    {
        window.ProcessQueueMessages();
        WindowMessage wmMove = window.GetMoveResult();
        WindowMessage wmSize = window.GetSizeResult();
        WindowMessage wmExit = window.GetExitResult();

        // Check for exit message
        if (!wmExit.handled)
        {
            exit(0); // ############ VERY BAD ########## ADD PROPER EXITING ###########
        }

        ztime::clock[CLOCK_GAME].Update();
        ztime::clock[CLOCK_MAIN].Update();

        _HandleNetworkStateChanges();

        // Update objects
        playlist.Update();
        _serverPlaylist.Update();
        users.Update();
        usersServer.Update();
        playback.Update();

        // Render frame
        window.gfx.Lock();
        window.gfx.BeginFrame();

        // Resize UI
        // This part is after "Render frame" because window.gfx.Lock() needs to be called prior
        // to resizing the scene
        if (!wmSize.handled)
        {
            int w = LOWORD(wmSize.lParam);
            int h = HIWORD(wmSize.lParam);

            if (_fullscreen)
            {
                _topMenuScene->Resize(w, MenuHeight());
                _overlayScene->Resize(w, h);
                auto activeScenes = ActiveScenes();
                for (auto& scene : activeScenes)
                    scene->Resize(w, h);
            }
            else
            {
                _topMenuScene->Resize(w, MenuHeight());
                _overlayScene->Resize(w, h - MenuHeight());
                auto activeScenes = ActiveScenes();
                for (auto& scene : activeScenes)
                    scene->Resize(w, h - MenuHeight());
            }
        }

        //// Show video frame
        //const FrameData* fd = player->GetCurrentFrame();
        //ID2D1Bitmap* frame = window.gfx.CreateBitmap(fd->GetWidth(), fd->GetHeight());
        //D2D1_RECT_U rect = D2D1::RectU(0, 0, fd->GetWidth(), fd->GetHeight());
        //frame->CopyFromMemory(&rect, fd->GetBytes(), fd->GetWidth() * 4);
        //window.gfx.DrawBitmap(
        //    D2D1::RectF(0.0f, 0.0f, fd->GetWidth(), fd->GetHeight()),
        //    D2D1::RectF(0.0f, 0.0f, window.width, window.height),
        //    frame
        //);

        //frame->Release();
        //auto activeScenes = ActiveScenes();
        //for (auto& scene : activeScenes)
        //{
        //    auto updatedActiveScenes = ActiveScenes();
        //    if (std::find(updatedActiveScenes.begin(), updatedActiveScenes.end(), scene) == updatedActiveScenes.end())
        //        continue;
        //    scene->Update();
        //    
        //    // Double checking is necessary because 'scene->Update()' can uninit other scenes
        //    updatedActiveScenes = ActiveScenes();
        //    if (std::find(updatedActiveScenes.begin(), updatedActiveScenes.end(), scene) != updatedActiveScenes.end())
        //        scene->Draw(window.gfx.GetGraphics());
        //}

//#define PRINT_FRAME_TIMES
#ifdef PRINT_FRAME_TIMES
        Clock timer = Clock(0);
        TimePoint start;
        Duration update = 0;
        Duration draw = 0;
#endif

        bool redraw = false;
        if (_sceneChanged)
        {
            _sceneChanged = false;
            redraw = true;
            window.gfx.GetGraphics().target->BeginDraw();
            window.gfx.GetGraphics().target->Clear(D2D1::ColorF(0));
        }

        { // Top menu scene
            _topMenuScene->Update();
            if (_topMenuScene->Redraw())
            {
                if (!redraw)
                    window.gfx.GetGraphics().target->BeginDraw();

                _topMenuScene->Draw(window.gfx.GetGraphics());
                redraw = true;
            }
        }

        { // Overlay scene
            _overlayScene->Update();
            if (_overlayScene->Redraw())
            {
                if (!redraw)
                    window.gfx.GetGraphics().target->BeginDraw();

                _overlayScene->Draw(window.gfx.GetGraphics());
                redraw = true;
            }
        }

        auto activeScenes = ActiveScenes();
#ifdef PRINT_FRAME_TIMES
        timer.Update();
        start = timer.Now();
#endif
        // Update scenes
        for (auto& scene : activeScenes)
        {
            scene->Update();
        }
#ifdef PRINT_FRAME_TIMES
        timer.Update();
        update += timer.Now() - start;
        
        timer.Update();
        start = timer.Now();
#endif
        // Draw scenes
        for (auto& scene : activeScenes)
        {
            if (scene->Redraw())
            {
                if (!redraw)
                    window.gfx.GetGraphics().target->BeginDraw();

                scene->Draw(window.gfx.GetGraphics());
                redraw = true;
            }
        }
#ifdef PRINT_FRAME_TIMES
        timer.Update();
        draw += timer.Now() - start;
#endif
        if (redraw)
        {
            std::cout << "Redrawn (" << framecounter++ << ")\n";
#ifdef PRINT_FRAME_TIMES
            timer.Update();
            start = timer.Now();
#endif
            window.gfx.GetGraphics().target->Clear(D2D1::ColorF(0.3f, 0.3f, 0.3f));
            //window.gfx.GetGraphics().target->Clear(D2D1::ColorF(0));
            if (_fullscreen)
            {
                for (auto& scene : activeScenes)
                {
                    window.gfx.GetGraphics().target->DrawBitmap(scene->Image());
                }
                // Draw overlay scene last
                window.gfx.GetGraphics().target->DrawBitmap(_overlayScene->Image());
            }
            else
            {
                D2D1_RECT_F mainRect = { 0.0f, (float)MenuHeight(), (float)window.width, (float)window.height };
                D2D1_RECT_F menuRect = { 0.0f, 0.0f, (float)window.width, (float)MenuHeight() };

                for (auto& scene : activeScenes)
                {
                    window.gfx.GetGraphics().target->DrawBitmap(scene->Image(), mainRect);
                }
                // Draw overlay scene last
                window.gfx.GetGraphics().target->DrawBitmap(_overlayScene->Image(), mainRect);
                // Draw top menu
                window.gfx.GetGraphics().target->DrawBitmap(_topMenuScene->Image(), menuRect);
            }
            window.gfx.GetGraphics().target->EndDraw();

#ifdef PRINT_FRAME_TIMES
            timer.Update();
            draw += timer.Now() - start;
#endif
        }

        // Uninit scenes
        while (!_scenesToUninitialize.empty())
        {
            _UninitScene(_scenesToUninitialize.front());
            _scenesToUninitialize.pop_front();
        }

#ifdef PRINT_FRAME_TIMES
        std::cout << "Update: " << update.GetDuration(MICROSECONDS) << "us\n";
        std::cout << "Draw: " << draw.GetDuration(MICROSECONDS) << "us\n";
#endif

        //// Draw UI
        //layout.componentCanvas.Update();
        //ID2D1Bitmap* bitmap = layout.componentCanvas.Draw(window.gfx.GetGraphics());
        //window.gfx.GetDeviceContext()->DrawBitmap(bitmap);

        window.gfx.EndFrame(redraw);
        window.gfx.Unlock();

        // Prevent deadlock from extremely short unlock-lock cycle
        // (it doesn't make sense to me either but for some reason
        // the mutexes aren't locked in order of 'Lock()' calls)
        Clock sleepTimer = Clock(0);
        do sleepTimer.Update();
        while (sleepTimer.Now().GetTime(MICROSECONDS) < 10);
    }
}

void App::_HandleNetworkStateChanges()
{
    while (_networkModeEventReceiver.EventCount() > 0)
    {
        auto ev = _networkModeEventReceiver.GetEvent();

        if (ev.newMode == znet::NetworkMode::OFFLINE)
        {
            _serverPlaylist.SetEventHandler<PlaylistEventHandler_None>();
            playlist.SetEventHandler<PlaylistEventHandler_Offline>();

            usersServer.SetEventHandler<UsersEventHandler_Offline>();
            users.SetEventHandler<UsersEventHandler_Offline>();
        }
        else if (ev.newMode == znet::NetworkMode::CLIENT)
        {
            _serverPlaylist.SetEventHandler<PlaylistEventHandler_None>();
            playlist.SetEventHandler<PlaylistEventHandler_Client>();

            usersServer.SetEventHandler<UsersEventHandler_Offline>();
            users.SetEventHandler<UsersEventHandler_Client>();
        }
        else if (ev.newMode == znet::NetworkMode::SERVER)
        {
            // _serverPlaylist should be created first, because client handler
            // constructor synchronously sends packets to the server playlist
            _serverPlaylist.SetEventHandler<PlaylistEventHandler_Server>();
            playlist.SetEventHandler<PlaylistEventHandler_Client>();

            usersServer.SetEventHandler<UsersEventHandler_Server>();
            users.SetEventHandler<UsersEventHandler_Client>();
        }
    }
}
