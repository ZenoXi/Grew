#pragma once

#include "DisplayWindow.h"
//s#include "MediaPlayer.h"
//#include "UILayout.h"
#include "ThreadController.h"
#include "Scene.h"
#include "MouseManager.h"
#include "KeyboardManager.h"
#include "AppEvents.h"
#include "NetworkEvents.h"
#include "Playlist.h"
#include "Playback.h"

#include <thread>
#include <mutex>

class OverlayScene;

// Singleton
class App
{
public:
    DisplayWindow& window;
    Playlist playlist;
private:
    Playlist _serverPlaylist;
public:
    Playback playback;
    Mouse mouse;
    MouseManager mouseManager;
    KeyboardManager keyboardManager;
    AppEvents events;

private: // Singleton interface
    App(DisplayWindow& dw, std::string startScene);
    static App* _instance;
public:
    static void Init(DisplayWindow& dw, std::string startScene);
    static App* Instance();

private: // Scene control
    Scene* _overlayScene = nullptr;
    std::vector<Scene*> _scenes;
    int _currentSceneIndex = -1;
    std::vector<Scene*> _activeScenes;
    std::queue<std::string> _scenesToUninitialize;
public:
    bool SetScene(std::string name, SceneOptionsBase* options);
    Scene* CurrentScene();

    OverlayScene* Overlay() { return (OverlayScene*)_overlayScene; }
    // Initializes the scene and places it behind all scenes, unfocused (unless no scenes are initialized)
    bool InitScene(std::string name, SceneOptionsBase* options);
    // Uninitializes and immediatelly initializes the scene with new options, keeping focus/z-order the same
    // If scene is not initialized, it just gets initialized as usual
    bool ReinitScene(std::string name, SceneOptionsBase* options);
    // Primes the scene to be uninitialized
    void UninitScene(std::string name);
private:
    void _UninitScene(std::string name);
public:
    bool MoveSceneToFront(std::string name);
    bool MoveSceneToBack(std::string name);
    bool MoveSceneUp(std::string name);
    bool MoveSceneDown(std::string name);
    // If the scene is already behind, it isn't moved
    bool MoveSceneBehind(std::string name, std::string behind);
    // If the scene is already in front, it isn't moved
    bool MoveSceneInFront(std::string name, std::string inFront);
    std::vector<Scene*> ActiveScenes();
    Scene* FindScene(std::string name);
    Scene* FindActiveScene(std::string name);
private:
    int FindSceneIndex(std::string name);
    int FindActiveSceneIndex(std::string name);

private: // Main app thread
    ThreadController _mainThreadController;
    std::thread _mainThread;
    std::mutex _m_main;
    bool sceneChanged = false;
public:
    void LoopThread();

private:
    bool _serverInit = false;
    bool _clientInit = false;
    EventReceiver<NetworkStateChangedEvent> _networkStateEventReceiver;
    void _HandleNetworkStateChanges();
};