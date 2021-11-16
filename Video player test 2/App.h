#pragma once

#include "DisplayWindow.h"
//s#include "MediaPlayer.h"
//#include "UILayout.h"
#include "ThreadController.h"
#include "Scene.h"
#include "MouseManager.h"
#include "KeyboardManager.h"

#include <thread>
#include <mutex>

// Singleton
class App
{
public:
    DisplayWindow& window;
    Mouse mouse;
    MouseManager mouseManager;
    KeyboardManager keyboardManager;
    
private: // Singleton interface
    App(DisplayWindow& dw, std::string startScene);
    static App* _instance;
public:
    static void Init(DisplayWindow& dw, std::string startScene);
    static App* Instance();

private: // Scene control
    std::vector<Scene*> _scenes;
    int _currentSceneIndex = -1;
public:
    bool SetScene(std::string name, SceneOptionsBase* options);
    Scene* CurrentScene();

private: // Main app thread
    ThreadController _mainThreadController;
    std::thread _mainThread;
    std::mutex _m_main;
public:
    void LoopThread();
};