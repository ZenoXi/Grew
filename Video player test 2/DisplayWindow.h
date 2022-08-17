#pragma once
#pragma comment( lib,"d2d1.lib" )
#pragma comment( lib,"d3d11.lib" )
#pragma comment( lib,"dwrite.lib" )
#include "ChiliWin.h"
#pragma comment( lib,"Shell32.lib" )
#include <shellapi.h>

#ifdef _DEBUG
#define HR(expression) assert(S_OK == (expression))
#else
#define HR(expression) expression
#endif

// Direct2D
#include <d2d1_1.h>
#include <d3d11_1.h>
#include "Dwrite.h"

#include "Navigation.h"
#include "Mouse.h"
#include "KeyboardEventHandler.h"
#include "DragDropEventHandler.h"
#include "Graphics.h"
#include "ThreadController.h"

#include <string>
#include <wrl.h>
#include <thread>
#include <iostream>

using namespace Microsoft::WRL;

struct BrushInfo
{
    // Basic
    D2D1::ColorF color = D2D1::ColorF::White;
    float size = 2.0f;

    // Linear gradient brush data
    bool linearGradient = false;

    // Radial gradient brush data
    bool radialGradient = false;
    D2D1::ColorF innerColor = D2D1::ColorF::White;
    D2D1::ColorF outerColor = D2D1::ColorF::White;

    // Specific brush
    bool specific = false;
    ID2D1Brush* pBrush = NULL;

    // Simple constructor for ease of use
    BrushInfo() {}
    BrushInfo(D2D1::ColorF color) : color(color) {}
    ~BrushInfo() { delete pBrush; }
};

class WindowGraphics
{
    HWND* p_hwnd = nullptr;
    std::mutex _m_gfx;
    bool _initialized = false;

    ComPtr<ID2D1Factory1> p_D2DFactory = NULL;
    ComPtr<ID2D1Device> p_D2DDevice;
    ComPtr<ID2D1DeviceContext> p_D2DDeviceContext = NULL;
    ComPtr<ID2D1DeviceContext> p_Target = NULL;
    ID2D1RenderTarget* p_RT = NULL;
    //ID2D1HwndRenderTarget* p_RT = NULL;
    std::vector<IUnknown**> _references;

    ComPtr<IDXGISwapChain1> p_SwapChain = NULL;
    ComPtr<ID3D11Device> p_Device = NULL;
    ComPtr<IDXGIDevice> p_DXGIDevice = NULL;
    ComPtr<IDXGIAdapter> p_DXGIAdapter = NULL;
    ComPtr<IDXGIFactory2> p_DXGIFactory = NULL;
    ComPtr<ID3D11DeviceContext> p_DeviceContext = NULL;
    ID3D11RenderTargetView* p_BackBuffer = NULL;

    ComPtr<IDXGISurface> p_Surface;
    ComPtr<ID2D1Bitmap1> p_Bitmap;

    IDWriteFactory* p_DWriteFactory = NULL;
    IDWriteTextFormat* p_DebugTextFormat = NULL;

    ID2D1RadialGradientBrush* pBrush = NULL;
    
    // Pregenerated brushes
    ID2D1SolidColorBrush* pWaterBrush = NULL;
public:
    RECT _windowRect;

    WindowGraphics() {};
    void Initialize(HWND* hwnd_t);
    void Close();
    bool Initialized() const { return _initialized; };

    void BeginFrame();
    void EndFrame(bool swap);
    void SetFullscreen(bool f);
    void ResizeBuffers(int width, int height);
    void ReleaseResource(IUnknown** ref);

    void Lock();
    void Unlock();
    ID2D1DeviceContext* GetDeviceContext();
    Graphics GetGraphics();
    ID2D1Bitmap* CreateBitmap(unsigned width, unsigned height);
    void DrawBitmap(const D2D1_RECT_F& srcRect, const D2D1_RECT_F& destRect, ID2D1Bitmap* bitmap);
    void DrawText(std::string text, BrushInfo bi = BrushInfo());
    void DrawLine(Pos2D<float> spos, Pos2D<float> epos, BrushInfo bi = BrushInfo());
    void DrawCircle(Pos2D<float> cpos, FLOAT radius, BrushInfo bi = BrushInfo());
    void FillCircle(Pos2D<float> cpos, FLOAT radius, BrushInfo bi = BrushInfo());
    void DrawEllipse(Pos2D<float> cpos, FLOAT width, FLOAT height, BrushInfo bi = BrushInfo());
    void FillEllipse(Pos2D<float> cpos, FLOAT width, FLOAT height, BrushInfo bi = BrushInfo());
    void DrawTriangle(Pos2D<float> pos1, Pos2D<float> pos2, Pos2D<float> pos3, BrushInfo bi = BrushInfo());
    void FillTriangle(Pos2D<float> pos1, Pos2D<float> pos2, Pos2D<float> pos3, BrushInfo bi = BrushInfo());
    void DrawGeometry(const std::vector<Pos2D<float>>& points, BrushInfo bi = BrushInfo());
    void FillGeometry(const std::vector<Pos2D<float>>& points, BrushInfo bi = BrushInfo());
    //void DrawTerrain(Pos2D<float> pos, const Terrain& t, BrushInfo biOutline = BrushInfo(), BrushInfo biBody = BrushInfo(D2D1::ColorF::Black));

    friend class Game;
    friend class DisplayWindow;
};

struct WindowMessage
{
    bool handled = true;
    UINT msg = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;
};

enum class CursorIcon
{
    APP_STARTING,
    ARROW,
    CROSS,
    HAND,
    HELP,
    IBEAM,
    NO,
    SIZE_ALL,
    SIZE_NESW,
    SIZE_NS,
    SIZE_NWSE,
    SIZE_WE,
    UP_ARROW,
    WAIT
};

class FileDropHandler : public IDropTarget
{
public:
    void AddDragDropEventHandler(IDragDropEventHandler* handler)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _handlers.push_back(handler);

        if (_dragging)
        {
            if (handler->OnDragEnter(_draggedPaths) == DropResponse::ALLOW)
            {
                _availableHandlers.push_back(handler);
            }
        }
    }

    bool RemoveDragDropEventHandler(IDragDropEventHandler* handler)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (int i = 0; i < _handlers.size(); i++)
        {
            if (_handlers[i] == handler)
            {
                if (_dragging)
                {
                    for (int j = 0; j < _availableHandlers.size(); j++)
                    {
                        if (_availableHandlers[j] == _handlers[i])
                        {
                            _availableHandlers[j]->OnDragLeave();
                            _availableHandlers.erase(_availableHandlers.begin() + j);
                            break;
                        }
                    }
                }

                _handlers.erase(_handlers.begin() + i);
                return true;
            }
        }
        return false;
    }

    HRESULT DragEnter(
        IDataObject* pDataObj,
        DWORD grfKeyState,
        POINTL pt,
        DWORD* pdwEffect)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dragging = true;
        _availableHandlers.clear();

        IEnumFORMATETC* enumfmt;
        if (FAILED(pDataObj->EnumFormatEtc(DATADIR_GET, &enumfmt)))
            return S_OK;

        FORMATETC fmt;
        while (enumfmt->Next(1, &fmt, NULL) == S_OK)
        {
            STGMEDIUM stg;
            if (FAILED(pDataObj->GetData(&fmt, &stg)))
                continue;
            switch (stg.tymed)
            {
            case TYMED_HGLOBAL:
            {
                //HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, 0);
                //stg.hGlobal;
                if (fmt.cfFormat == CF_HDROP && fmt.dwAspect == 1)
                {
                    _draggedPaths.clear();
                    UINT filecount = DragQueryFile((HDROP)stg.hGlobal, 0xFFFFFFFF, NULL, NULL);
                    for (UINT i = 0; i < filecount; i++)
                    {
                        UINT strLen = DragQueryFile((HDROP)stg.hGlobal, i, NULL, NULL);
                        std::wstring path;
                        path.resize(strLen + 1);
                        if (DragQueryFile((HDROP)stg.hGlobal, i, path.data(), strLen + 1) != 0)
                        {
                            if (path.back() == L'\0')
                                path.erase(path.end() - 1);
                            _draggedPaths.push_back(path);
                        }
                    }

                    // Gather available handlers
                    for (int i = 0; i < _handlers.size(); i++)
                    {
                        if (_handlers[i]->OnDragEnter(_draggedPaths) == DropResponse::ALLOW)
                        {
                            _availableHandlers.push_back(_handlers[i]);
                        }
                    }
                }
                
                break;
            }
            default:
                break;
            }
            ReleaseStgMedium(&stg);
        }

        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    HRESULT DragOver(
        DWORD grfKeyState,
        POINTL pt,
        DWORD* pdwEffect)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        POINT offset;
        offset.x = 0;
        offset.y = 0;
        ClientToScreen(_hwnd, &offset);
        int clientX = pt.x - offset.x;
        int clientY = pt.y - offset.y;

        bool overHandler = false;
        for (int i = 0; i < _availableHandlers.size(); i++)
        {
            _availableHandlers[i]->OnDrag(clientX, clientY);
            RECT rect = _availableHandlers[i]->GetDropRect();
            if (clientX >= rect.left &&
                clientX < rect.right &&
                clientY >= rect.top &&
                clientY < rect.bottom)
            {
                overHandler = true;
            }
        }

        if (overHandler)
            *pdwEffect = DROPEFFECT_COPY;
        else
            *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    HRESULT DragLeave()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dragging = false;

        for (int i = 0; i < _availableHandlers.size(); i++)
            _availableHandlers[i]->OnDragLeave();

        return S_OK;
    }

    HRESULT Drop(
        IDataObject* pDataObj,
        DWORD grfKeyState,
        POINTL pt,
        DWORD* pdwEffect)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dragging = false;

        POINT offset;
        offset.x = 0;
        offset.y = 0;
        ClientToScreen(_hwnd, &offset);
        int clientX = pt.x - offset.x;
        int clientY = pt.y - offset.y;

        for (int i = 0; i < _availableHandlers.size(); i++)
        {
            RECT rect = _availableHandlers[i]->GetDropRect();
            if (clientX >= rect.left &&
                clientX < rect.right &&
                clientY >= rect.top &&
                clientY < rect.bottom)
            {
                _availableHandlers[i]->OnDrop(clientX, clientY, _draggedPaths);
            }
        }

        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) { return 0; }

    FileDropHandler(HWND hwnd)
    {
        _hwnd = hwnd;
    }

private:
    HWND _hwnd;

    std::mutex _mutex;
    bool _dragging = false;
    std::vector<IDragDropEventHandler*> _availableHandlers;
    std::vector<IDragDropEventHandler*> _handlers;
    std::vector<std::wstring> _draggedPaths;
};

class DisplayWindow
{
    HWND _hwnd = nullptr;

    LPCWSTR _wndClassName = L"wndClassName";
    HINSTANCE _hInst;
    std::wstring _args;

    bool _mouseInWindow = false;
    HCURSOR _prevCursor = 0;
    HCURSOR _cursor = 0;

    // Handlers
    std::vector<MouseEventHandler*> _mouseHandlers;
    std::vector<KeyboardEventHandler*> _keyboardHandlers;
    std::unique_ptr<FileDropHandler> _fileDropHandler;

public:
    static std::wstring WINDOW_NAME;
    //unsigned width = 640;
    //unsigned height = 360;
    unsigned width = 1280;
    unsigned height = 775;
    //unsigned width = 1920;
    //unsigned height = 1080;
    WindowGraphics gfx;

    Mouse* mouse;

    DisplayWindow(HINSTANCE hInst, wchar_t* pArgs, LPCWSTR name);
    DisplayWindow(const DisplayWindow&) = delete;
    DisplayWindow& operator=(const DisplayWindow&) = delete;
    ~DisplayWindow();

    HWND& GetHWNDRef() { return _hwnd; };

    bool ProcessMessages();
    void ProcessQueueMessages();
    WindowMessage GetSizeResult();
    WindowMessage GetMoveResult();
    WindowMessage GetExitResult();
    void AddMouseHandler(MouseEventHandler* handler);
    bool RemoveMouseHandler(MouseEventHandler* handler);
    void AddKeyboardHandler(KeyboardEventHandler* handler);
    bool RemoveKeyboardHandler(KeyboardEventHandler* handler);
    void AddDragDropHandler(IDragDropEventHandler* handler);
    bool RemoveDragDropHandler(IDragDropEventHandler* handler);

    void SetFullscreen(bool fullscreen);
    bool GetFullscreen();
    void SetWindowDimensions(unsigned w, unsigned h);
    void HandleFullscreenChange();
    void SetCursorIcon(CursorIcon cursor);
    // Shows/Hides the cursor until it is moved or this function is called
    void SetCursorVisibility(bool visible);
    void HandleCursorVisibility();
    // Resets screen shutoff timer
    void ResetScreenTimer();
private:
    static LRESULT WINAPI _HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI _HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void HandleMsgFromQueue(WindowMessage msg);
    void MsgHandleThread();

    std::thread _msgThread;
    ThreadController _msgThreadController;
    std::mutex _m_msg;
    WindowMessage _sizeResult;
    WindowMessage _moveResult;
    WindowMessage _exitResult;
    std::queue<WindowMessage> _msgQueue;

    bool _fullscreen = false;
    bool _maximized = false;
    RECT _windowedRect;
    bool _windowedMaximized;
    bool _cursorVisible = true;
    bool _cursorVisibilityChanged = false;

    RECT _last2Moves[2];
    POINT _lastMouseMove;

    bool _fullscreenChanged = false;
    std::mutex _m_fullscreen;
    //int _newWidth;
    //int _newHeight;
};