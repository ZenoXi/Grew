#pragma once

#include "Canvas.h"

// Options for switching scenes
struct SceneOptionsBase
{
    
};

class Scene
{
protected:
    zcom::Canvas* _canvas;

public:
    Scene();
    virtual ~Scene();
    void Init(const SceneOptionsBase* options);
    void Uninit();

    void Update();
    ID2D1Bitmap1* Draw(Graphics g);
    void Resize(int width, int height);

private:
    virtual void _Init(const SceneOptionsBase* options) = 0;
    virtual void _Uninit() = 0;

    virtual void _Update() = 0;
    virtual ID2D1Bitmap1* _Draw(Graphics g) = 0;
    virtual void _Resize(int width, int height) = 0;

public:
    virtual const char* GetName() const = 0;
};