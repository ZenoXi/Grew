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
#include "Users.h"
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
    Users users;
    Users usersServer;
    Playback playback;
    Mouse mouse;
    MouseManager mouseManager;
    KeyboardManager keyboardManager;
    AppEvents events;

private: // Singleton interface
    App(DisplayWindow& dw);
    static App* _instance;
    static bool _exited;
public:
    static void Init(DisplayWindow& dw);
    static App* Instance();
    static void Start();
    static bool Exited();

private: // Scene control
    Scene* _overlayScene = nullptr;
    Scene* _topMenuScene = nullptr;
    std::vector<Scene*> _scenes;
    int _currentSceneIndex = -1;
    std::vector<Scene*> _activeScenes;
    std::deque<std::string> _scenesToUninitialize;
    bool _fullscreen = false;
public:
    bool SetScene(std::string name, SceneOptionsBase* options);
    Scene* CurrentScene();
    bool Fullscreen() const { return _fullscreen; }
    void Fullscreen(bool fullscreen);
    int ClientWidth() const { return window.width; }
    int ClientHeight() const { return _fullscreen ? window.height : window.height - MenuHeight(); }
    int MenuWidth() const { return window.width; }
    int MenuHeight() const { return 25; }
    OverlayScene* Overlay() const { return (OverlayScene*)_overlayScene; }
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
    bool _sceneChanged = false;
public:
    void LoopThread();

private:
    bool _serverInit = false;
    bool _clientInit = false;
    EventReceiver<NetworkModeChangedEvent> _networkModeEventReceiver;
    void _HandleNetworkStateChanges();
};