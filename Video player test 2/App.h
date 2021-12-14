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
    std::vector<Scene*> _activeScenes;
public:
    bool SetScene(std::string name, SceneOptionsBase* options);
    Scene* CurrentScene();
    // Initializes the scene and places it behind all scenes, unfocused (unless no scenes are initialized)
    bool InitScene(std::string name, SceneOptionsBase* options);
    bool UninitScene(std::string name);
    bool MoveSceneToFront(std::string name);
    bool MoveSceneToBack(std::string name);
    bool MoveSceneUp(std::string name);
    bool MoveSceneDown(std::string name);
    // If the scene is already behind, it isn't moved
    bool MoveSceneBehind(std::string name, std::string behind);
    // If the scene is already in front, it isn't moved
    bool MoveSceneInFront(std::string name, std::string inFront);
    std::vector<Scene*> ActiveScenes();
private:
    Scene* FindScene(std::string name);
    Scene* FindActiveScene(std::string name);
    int FindSceneIndex(std::string name);
    int FindActiveSceneIndex(std::string name);

private: // Main app thread
    ThreadController _mainThreadController;
    std::thread _mainThread;
    std::mutex _m_main;
public:
    void LoopThread();
};