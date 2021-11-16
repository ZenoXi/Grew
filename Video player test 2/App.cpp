#include "App.h"

#include "EntryScene.h";
#include "PlaybackScene.h"

App* App::_instance = nullptr;

App::App(DisplayWindow& dw, std::string startScene) : window(dw)/*, layout(dw.width, dw.height)*/
{
    dw.mouse = &mouse;
    dw.AddMouseHandler(&mouseManager);
    dw.AddKeyboardHandler(&keyboardManager);

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

    // Add scenes
    Instance()->_scenes.push_back(new EntryScene());
    Instance()->_scenes.push_back(new PlaybackScene());
    Instance()->SetScene(startScene, nullptr);

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

void App::LoopThread()
{
    int framecounter = 0;

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
            //layout.Resize(w, h);
            //layout.componentCanvas.Resize(w, h);
            CurrentScene()->Resize(w, h);
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

        // Update scene
        CurrentScene()->Update();

        // Draw scene
        CurrentScene()->Draw(window.gfx.GetGraphics());

        //// Draw UI
        //layout.componentCanvas.Update();
        //ID2D1Bitmap* bitmap = layout.componentCanvas.Draw(window.gfx.GetGraphics());
        //window.gfx.GetDeviceContext()->DrawBitmap(bitmap);

        window.gfx.EndFrame();
        window.gfx.Unlock();

        // Prevent deadlock from extremely short unlock/lock cycle
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}