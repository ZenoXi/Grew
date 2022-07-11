#include "DisplayWindow.h"

#include "Functions.h"

#include <iostream>
#include <cassert>
#include <sstream>

#include <WinUser.h>
#pragma comment( lib,"User32.lib" )
#include <Shellapi.h>
#include <dwmapi.h>
#pragma comment( lib,"Dwmapi.lib" )

std::wstring DisplayWindow::WINDOW_NAME = L"Grew";

#define EXEC_PTR_LOOP(container, expression) for (auto item : container) item->expression
#define EXEC_LOOP(container, expression) for (auto item : container) item.expression

using Microsoft::WRL::ComPtr;

DisplayWindow::DisplayWindow(HINSTANCE hInst, wchar_t* pArgs, LPCWSTR name) : _args(pArgs), _hInst(hInst), _wndClassName(name)
{
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        _HandleMsgSetup,
        0,
        0,
        hInst,
        nullptr,
        LoadCursor(NULL, IDC_ARROW),
        nullptr,
        nullptr,
        _wndClassName,
        nullptr
    };
    
    RegisterClassEx(&wc);

    // Create window at the center of screen
    //RECT desktop;
    //HWND deskwin = GetDesktopWindow();
    //GetWindowRect(deskwin, &desktop);

    //RECT wr;
    //wr.left = (desktop.right / 2) - width / 2;
    //wr.top = (desktop.bottom / 2) - height / 2;
    //wr.right = wr.left + width;
    //wr.bottom = wr.top + height;
    //AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);


    // Create maximized window
    RECT workRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workRect, 0);
    int x = (workRect.right - width) / 2;
    int y = (workRect.bottom - height) / 2;

    // Create and show window
    _hwnd = CreateWindowEx(
        WS_EX_ACCEPTFILES,
        _wndClassName,
        WINDOW_NAME.c_str(),
        WS_OVERLAPPEDWINDOW,
        //0, 0, workRect.right, workRect.bottom,
        x, y, width, height,
        //CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInst,
        this
    );

    gfx.Initialize(&_hwnd);

    //PAINTSTRUCT ps;
    //HDC hdc = BeginPaint(_hwnd, &ps);
    //FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(128, 128, 128)));
    //EndPaint(_hwnd, &ps);

    _windowedRect.left = x;
    _windowedRect.top = y;
    _windowedRect.right = x + width;
    _windowedRect.bottom = y + height;
    _last2Moves[0] = _windowedRect;
    _last2Moves[1] = _windowedRect;
    
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
    BOOL value = TRUE;
    ::DwmSetWindowAttribute(_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

    ShowWindow(_hwnd, SW_SHOWNORMAL);
    UpdateWindow(_hwnd);

    // Start message handle thread
    //_msgThreadController.Add("stop", sizeof(bool));
    //_msgThreadController.Set("stop", false);
    //_msgThread = std::thread(&DisplayWindow::MsgHandleThread, this);
}

DisplayWindow::~DisplayWindow()
{
    gfx.Close();

    // unregister window class
    UnregisterClass(_wndClassName, _hInst);
}

void DisplayWindow::ProcessMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            exit(0);
        }
    }
}

void DisplayWindow::ProcessQueueMessages()
{
    std::lock_guard<std::mutex> lock(_m_msg);
    while (!_msgQueue.empty())
    {
        HandleMsgFromQueue(_msgQueue.front());
        _msgQueue.pop();
    }
}

WindowMessage DisplayWindow::GetSizeResult()
{
    _m_msg.lock();
    WindowMessage wm = _sizeResult;
    _sizeResult.handled = true;
    _sizeResult.lParam = 0;
    _sizeResult.wParam = 0;
    _m_msg.unlock();
    return wm;
}

WindowMessage DisplayWindow::GetMoveResult()
{
    _m_msg.lock();
    WindowMessage wm = _moveResult;
    _moveResult.handled = true;
    _moveResult.lParam = 0;
    _moveResult.wParam = 0;
    _m_msg.unlock();
    return wm;
}

WindowMessage DisplayWindow::GetExitResult()
{
    _m_msg.lock();
    WindowMessage wm = _exitResult;
    _exitResult.handled = true;
    _m_msg.unlock();
    return wm;
}

LRESULT WINAPI DisplayWindow::_HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
    if (msg == WM_NCCREATE)
    {
        // extract ptr to window class from creation data
        const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        DisplayWindow* const pWnd = reinterpret_cast<DisplayWindow*>(pCreate->lpCreateParams);
        // set WinAPI-managed user data to store ptr to window class
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
        // set message proc to normal (non-setup) handler now that setup is finished
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DisplayWindow::_HandleMsgThunk));
        // forward message to window class handler
        return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
    }
    // if we get a message before the WM_NCCREATE message, handle with default handler
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WINAPI DisplayWindow::_HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // retrieve ptr to window class
    DisplayWindow* const pWnd = reinterpret_cast<DisplayWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    // forward message to window class handler
    return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT DisplayWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //if (msg != WM_MOUSEMOVE) std::cout << msg << std::endl;

    switch (msg)
    {
        case WM_DESTROY:
        {
            _m_msg.lock();
            _exitResult.handled = false;
            _m_msg.unlock();
            break;
        }
        case WM_ACTIVATE:
        {
            //MARGINS margins = { 0 };
            //margins.cyTopHeight = 20;
            //DwmExtendFrameIntoClientArea(_hwnd, &margins);
            break;
        }
        case WM_KILLFOCUS:
        {
            break;
        }
        case WM_DROPFILES:
        {
            HDROP hDropInfo = (HDROP)wParam;
            wchar_t sItem[MAX_PATH];
            for (int i = 0; DragQueryFile(hDropInfo, i, sItem, sizeof(sItem)); i++)
            {
                std::wcout << sItem << std::endl;
            }
            DragFinish(hDropInfo);
            break;
        }
        case WM_MOUSEMOVE:
        {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            if (x != _lastMouseMove.x || y != _lastMouseMove.y)
            {
                _lastMouseMove.x = x;
                _lastMouseMove.y = y;
            }
            else
            {
                break;
            }
            //std::cout << x << "," << y << std::endl;
            _m_msg.lock();
            if (GetCapture() != _hwnd)
            {
                _mouseInWindow = false;
            }

            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                if (!_mouseInWindow)
                {
                    SetCapture(_hwnd);
                }
            }
            else
            {
                if (wParam & (MK_LBUTTON | MK_RBUTTON))
                {
                    //x = std::max(0, x);
                    //x = std::min(int(width) - 1, x);
                    //y = std::max(0, y);
                    //y = std::min(int(height) - 1, y);
                }
                else
                {
                    ReleaseCapture();
                }
            }
            _msgQueue.push({ false, WM_MOUSEMOVE, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_LBUTTONDOWN:
        {
            SetWindowPos(_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

            _m_msg.lock();
            _msgQueue.push({ false, WM_LBUTTONDOWN, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_RBUTTONDOWN:
        {
            _m_msg.lock();
            _msgQueue.push({ false, WM_RBUTTONDOWN, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_LBUTTONUP:
        {
            _m_msg.lock();
            _msgQueue.push({ false, WM_LBUTTONUP, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_RBUTTONUP:
        {
            _m_msg.lock();
            _msgQueue.push({ false, WM_RBUTTONUP, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_MOUSEWHEEL:
        {
            _m_msg.lock();
            _msgQueue.push({ false, WM_MOUSEWHEEL, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_MOVE:
        {
            if (!_fullscreen)
            {
                int x = (int)(short)LOWORD(lParam);
                int y = (int)(short)HIWORD(lParam);
                int w = _windowedRect.right - _windowedRect.left;
                int h = _windowedRect.bottom - _windowedRect.top;
                _last2Moves[0] = _last2Moves[1];
                _last2Moves[1].left = x;
                _last2Moves[1].top = y;
                _last2Moves[1].right = x + w;
                _last2Moves[1].bottom = y + h;
            }

            _m_msg.lock();
            _msgQueue.push({ false, WM_MOVE, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* info = (MINMAXINFO*)lParam;
            info->ptMinTrackSize.x = 800;
            info->ptMinTrackSize.y = 600;
            break;
        }
        case WM_SIZE:
        {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (w == width && h == height) break;

            if (wParam == SIZE_RESTORED && !_fullscreen)
            {
                _last2Moves[0] = _last2Moves[1];
                GetWindowRect(_hwnd, &_last2Moves[1]);
                //GetWindowRect(_hwnd, &_windowedRect);
                //_windowedRect.right = _windowedRect.left + w;
                //_windowedRect.bottom = _windowedRect.top + h;
            }
            if (wParam == SIZE_MAXIMIZED)
            {
                _last2Moves[1] = _last2Moves[0];
            }
            if (wParam == SIZE_MINIMIZED)
            {
                _last2Moves[1] = _last2Moves[0];
            }

            if (gfx.Initialized())
                gfx.ResizeBuffers(w, h);
            width = w;
            height = h;

            _m_msg.lock();
            _msgQueue.push({ false, WM_SIZE, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_KEYDOWN:
        {
            //std::cout << wParam << " " << lParam << std::endl;
            _m_msg.lock();
            _msgQueue.push({ false, WM_KEYDOWN, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_KEYUP:
        {
            _m_msg.lock();
            _msgQueue.push({ false, WM_KEYUP, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_SYSKEYDOWN:
        {
            std::cout << "syskeydown" << std::endl;
            _m_msg.lock();
            _msgQueue.push({ false, WM_SYSKEYDOWN, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_SYSKEYUP:
        {
            std::cout << "syskeyup" << std::endl;
            _m_msg.lock();
            _msgQueue.push({ false, WM_SYSKEYUP, wParam, lParam });
            _m_msg.unlock();
            break;
        }
        case WM_CHAR:
        {
            _m_msg.lock();
            _msgQueue.push({ false, WM_CHAR, wParam, lParam });
            _m_msg.unlock();
            //wchar_t c = wParam;
            //WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, nullptr, nullptr);
            //std::cout << " ";
            break;
        }
        default:
        {
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }

    return 0;
}

void DisplayWindow::HandleMsgFromQueue(WindowMessage msg)
{
    switch (msg.msg)
    {
        case WM_MOUSEMOVE:
        {
            int x = (short)LOWORD(msg.lParam);
            int y = (short)HIWORD(msg.lParam);
            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                if (!_mouseInWindow)
                {
                    //SetCapture(_hwnd);
                    _mouseInWindow = true;
                    for (auto h : _mouseHandlers) h->OnMouseEnter();
                }
                for (auto h : _mouseHandlers) h->OnMouseMove(x, y);
            }
            else
            {
                //std::cout << "Mouse left: " << x << "," << y << std::endl;
                if (msg.wParam & (MK_LBUTTON | MK_RBUTTON))
                {
                    //x = std::max(0, x);
                    //x = std::min(int(width) - 1, x);
                    //y = std::max(0, y);
                    //y = std::min(int(height) - 1, y);
                    for (auto h : _mouseHandlers) h->OnMouseMove(x, y);
                }
                else
                {
                    //ReleaseCapture();
                    _mouseInWindow = false;
                    for (auto h : _mouseHandlers)
                    {
                        h->OnLeftReleased(x, y);
                        h->OnRightReleased(x, y);
                        h->OnMouseLeave();
                    }
                    //std::cout << "Mouse leave\n";
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int x = LOWORD(msg.lParam);
            int y = HIWORD(msg.lParam);
            for (auto h : _mouseHandlers) h->OnLeftPressed(x, y);
            break;
        }
        case WM_RBUTTONDOWN:
        {
            int x = LOWORD(msg.lParam);
            int y = HIWORD(msg.lParam);
            for (auto h : _mouseHandlers) h->OnRightPressed(x, y);
            break;
        }
        case WM_LBUTTONUP:
        {
            int x = LOWORD(msg.lParam);
            int y = HIWORD(msg.lParam);
            for (auto h : _mouseHandlers) h->OnLeftReleased(x, y);
            break;
        }
        case WM_RBUTTONUP:
        {
            int x = LOWORD(msg.lParam);
            int y = HIWORD(msg.lParam);
            for (auto h : _mouseHandlers) h->OnRightReleased(x, y);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int x = LOWORD(msg.lParam);
            int y = HIWORD(msg.lParam);
            if (GET_WHEEL_DELTA_WPARAM(msg.wParam) > 0)
            {
                for (auto h : _mouseHandlers) h->OnWheelUp(x, y);
            }
            else if (GET_WHEEL_DELTA_WPARAM(msg.wParam) < 0)
            {
                for (auto h : _mouseHandlers) h->OnWheelDown(x, y);
            }
            break;
        }
        case WM_KEYDOWN:
        {
            for (auto& h : _keyboardHandlers) h->OnKeyDown(msg.wParam);
            break;
        }
        case WM_KEYUP:
        {
            for (auto& h : _keyboardHandlers) h->OnKeyUp(msg.wParam);
            break;
        }
        case WM_CHAR:
        {
            for (auto& h : _keyboardHandlers) h->OnChar(msg.wParam);
            break;
        }
        case WM_MOVE:
        {
            _moveResult.handled = false;
            _moveResult.msg = WM_MOVE;
            _moveResult.lParam = msg.lParam;
            _moveResult.wParam = msg.wParam;
            break;
        }
        case WM_SIZE:
        {
            _sizeResult.handled = false;
            _sizeResult.msg = WM_SIZE;
            _sizeResult.lParam = msg.lParam;
            _sizeResult.wParam = msg.wParam;
            break;
        }
    }
}

void DisplayWindow::MsgHandleThread()
{
    while (!_msgThreadController.Get<bool>("stop"))
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            std::cout << "msgh: " << msg.message << std::endl;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                _m_msg.lock();
                _exitResult.handled = false;
                _m_msg.unlock();
            }
        }

        // Limit cpu usage, since exact timing of message handling is unnecessary
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void DisplayWindow::HandleFullscreenChange()
{
    std::lock_guard<std::mutex> lock(_m_fullscreen);
    if (_fullscreenChanged)
    {
        _fullscreenChanged = false;

        if (_fullscreen)
        {
            WINDOWPLACEMENT placement;
            placement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(_hwnd, &placement);
            _windowedMaximized = (placement.showCmd == SW_SHOWMAXIMIZED);

            RECT desktop;
            HWND deskwin = GetDesktopWindow();
            GetWindowRect(deskwin, &desktop);

            int w = desktop.right - desktop.left;
            int h = desktop.bottom - desktop.top;
            SetWindowLongPtr(_hwnd, GWL_STYLE, /*WS_VISIBLE |*/ WS_POPUP);
            SetWindowPos(_hwnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);

            ShowWindow(_hwnd, SW_SHOW);
        }
        else
        {
            //int x = _windowedRect.left;
            //int y = _windowedRect.top;
            //int w = _windowedRect.right - _windowedRect.left;
            //int h = _windowedRect.bottom - _windowedRect.top;
            int x = _last2Moves[1].left;
            int y = _last2Moves[1].top;
            int w = _last2Moves[1].right - _last2Moves[1].left;
            int h = _last2Moves[1].bottom - _last2Moves[1].top;
            SetWindowLongPtr(_hwnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
            SetWindowPos(_hwnd, NULL, x, y, w, h, SWP_FRAMECHANGED);

            if (_windowedMaximized)
                ShowWindow(_hwnd, SW_SHOWMAXIMIZED);
            else
                ShowWindow(_hwnd, SW_SHOW);
        }
        //UpdateWindow(_hwnd);
    }
}

void DisplayWindow::SetCursorVisibility(bool visible)
{
    if (visible != _cursorVisible)
    {
        _cursorVisible = visible;
        _cursorVisibilityChanged = true;
    }
}

void DisplayWindow::HandleCursorVisibility()
{
    if (_cursorVisibilityChanged)
    {
        _cursorVisibilityChanged = false;

        if (!_cursorVisible)
        {
            int value;
            while ((value = ShowCursor(false)) > -1);
            while (value < -1) value = ShowCursor(true);
        }
        else
        {
            int value;
            while ((value = ShowCursor(true)) < 1);
            while (value > 1) value = ShowCursor(false);
        }
    }
}

void DisplayWindow::ResetScreenTimer()
{
    SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
}

void DisplayWindow::AddMouseHandler(MouseEventHandler* handler)
{
    _mouseHandlers.push_back(handler);
}

bool DisplayWindow::RemoveMouseHandler(MouseEventHandler* handler)
{
    for (auto it = _mouseHandlers.begin(); it != _mouseHandlers.end(); it++)
    {
        if (*it == handler)
        {
            _mouseHandlers.erase(it);
            return true;
        }
    }
    return false;
}

void DisplayWindow::AddKeyboardHandler(KeyboardEventHandler* handler)
{
    _keyboardHandlers.push_back(handler);
}

bool DisplayWindow::RemoveKeyboardHandler(KeyboardEventHandler* handler)
{
    for (auto it = _keyboardHandlers.begin(); it != _keyboardHandlers.end(); it++)
    {
        if (*it == handler)
        {
            _keyboardHandlers.erase(it);
            return true;
        }
    }
    return false;
}

void DisplayWindow::SetFullscreen(bool fullscreen)
{
    std::lock_guard<std::mutex> lock(_m_fullscreen);
    if (_fullscreen == fullscreen) return;
    _fullscreenChanged = true;
    _fullscreen = fullscreen;
}

bool DisplayWindow::GetFullscreen()
{
    return _fullscreen;
}

void WindowGraphics::Initialize(HWND* hwnd_t)
{
    p_hwnd = hwnd_t;
    HRESULT hr;
    
    // Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&p_DWriteFactory)
    );

    p_DWriteFactory->CreateTextFormat(
        L"Calibri",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en_us",
        &p_DebugTextFormat
    );

    // Obtain the size of the drawing area.
    GetClientRect(*p_hwnd, &_windowRect);

    D2D1CreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED,
        {},
        p_D2DFactory.GetAddressOf()
    );

    D3D11CreateDevice(nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0,
        D3D11_SDK_VERSION,
        p_Device.GetAddressOf(),
        nullptr,
        nullptr
    );

    HR(p_Device.As(&p_DXGIDevice));
    HR(p_DXGIDevice->GetAdapter(p_DXGIAdapter.GetAddressOf()));
    HR(p_DXGIAdapter->GetParent(__uuidof(p_DXGIFactory), reinterpret_cast<void**>(p_DXGIFactory.GetAddressOf())));

    // Allow alt-tab
    //HR(p_DXGIFactory->MakeWindowAssociation(*hwnd_t, 0));

    // Swap chain stuff
    DXGI_SWAP_CHAIN_DESC1 scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC1));

    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 1;
    //scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfd;
    scfd.RefreshRate = { 20, 1 };
    scfd.Windowed = false;

    HR(p_DXGIFactory->CreateSwapChainForHwnd(
        p_Device.Get(),
        *p_hwnd,
        &scd,
        nullptr,
        nullptr,
        p_SwapChain.GetAddressOf()
    ));

    HR(p_D2DFactory->CreateDevice(p_DXGIDevice.Get(), p_D2DDevice.GetAddressOf()));
    HR(p_D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, p_Target.GetAddressOf()));
    HR(p_SwapChain->GetBuffer(
        0, // buffer index
        __uuidof(p_Surface),
        reinterpret_cast<void**>(p_Surface.GetAddressOf())
    ));
    auto props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
    );
    HR(p_Target->CreateBitmapFromDxgiSurface(p_Surface.Get(),
        props,
        p_Bitmap.GetAddressOf()
    ));
    p_Target->SetTarget(p_Bitmap.Get());

    //float dpiX = 96.f, dpiY = 96.f;
    //p_D2DFactory->GetDesktopDpi(&dpiX, &dpiY);
    //p_Target->SetDpi(dpiX, dpiY);
    //p_Target->SetDpi(72, 72);

    // Make background gray
    //BeginFrame();
    //p_Target->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    //EndFrame();

    _initialized = true;
}

void WindowGraphics::Close()
{
    p_SwapChain->SetFullscreenState(FALSE, NULL);

    p_SwapChain->Release();
    p_Device->Release();
    p_DeviceContext->Release();
    p_BackBuffer->Release();
}

void WindowGraphics::BeginFrame()
{
    //p_Target->BeginDraw();
    //p_Target->Clear(D2D1::ColorF(0, 0, 0));
}

void WindowGraphics::EndFrame(bool swap)
{
    //p_Target->EndDraw();
    if (swap)
        p_SwapChain->Present(1, 0);
    else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void WindowGraphics::SetFullscreen(bool f)
{
    p_SwapChain->SetFullscreenState(f, NULL);
}

void WindowGraphics::ResizeBuffers(int width, int height)
{
    Lock();
    //_m_gfx.lock();
    //auto size = p_Target->GetSize();
    //std::cout << "fix crash" << std::endl;
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Release all references
    for (int i = 0; i < _references.size(); i++)
    {
        if (*_references[i])
        {
            (*_references[i])->Release();
            (*_references[i]) = nullptr;
        }
    }
    _references.clear();
    p_Bitmap->Release();
    p_Surface->Release();
    p_Target->Release();

    // Resize swapchain
    HR(p_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0/*DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH*/));

    // Recreate target reference
    HR(p_D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, p_Target.GetAddressOf()));
    HR(p_SwapChain->GetBuffer(
        0, // buffer index
        __uuidof(p_Surface),
        reinterpret_cast<void**>(p_Surface.GetAddressOf())
    ));
    auto props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
    );
    HR(p_Target->CreateBitmapFromDxgiSurface(p_Surface.Get(),
        props,
        p_Bitmap.GetAddressOf()
    ));
    p_Target->SetTarget(p_Bitmap.Get());

    //size = p_Target->GetSize();
    //std::cout << size.width << ":" << size.height << "\n";
    //_m_gfx.unlock();
    Unlock();
}

void WindowGraphics::ReleaseResource(IUnknown** res)
{
    // Release resource
    if (*res)
    {
        (*res)->Release();
        *res = nullptr;
    }

    // Remove pointer from vector
    for (auto it = _references.begin(); it != _references.end(); it++)
    {
        if (*it == res)
        {
            _references.erase(it);
            break;
        }
    }
}

void WindowGraphics::Lock()
{
    _m_gfx.lock();
}

void WindowGraphics::Unlock()
{
    _m_gfx.unlock();
}

ID2D1DeviceContext* WindowGraphics::GetDeviceContext()
{
    return p_Target.Get();
}

Graphics WindowGraphics::GetGraphics()
{
    return { p_Target.Get(), p_D2DFactory.Get(), &_references };
}

ID2D1Bitmap* WindowGraphics::CreateBitmap(unsigned width, unsigned height)
{
    D2D1_BITMAP_PROPERTIES props;
    props.dpiX = 96.0f;
    props.dpiY = 96.0f;
    props.pixelFormat = D2D1::PixelFormat(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        D2D1_ALPHA_MODE_IGNORE
    );
    ID2D1Bitmap* frame;
    p_Target->CreateBitmap(
        D2D1::SizeU(width, height),
        props,
        &frame
    );
    return frame;
}

void WindowGraphics::DrawBitmap(const D2D1_RECT_F& srcRect, const D2D1_RECT_F& destRect, ID2D1Bitmap* bitmap)
{
    p_Target->DrawBitmap(bitmap, destRect, 1.0f, D2D1_INTERPOLATION_MODE_CUBIC, srcRect);
}

void WindowGraphics::DrawText(std::string text, BrushInfo bi)
{
    D2D1_RECT_F rect = { 0.0f, 0.0f, static_cast<float>(_windowRect.right), static_cast<float>(_windowRect.bottom) };

    bi.color = D2D1::ColorF::Gray;
    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->DrawText(string_to_wstring(text).c_str(), text.length(), p_DebugTextFormat, rect, pBrush);

    pBrush->Release();
}

void WindowGraphics::DrawLine(Pos2D<float> spos, Pos2D<float> epos, BrushInfo bi)
{
    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->DrawLine(
        D2D1::Point2F(static_cast<FLOAT>(spos.x), static_cast<FLOAT>(spos.y)),
        D2D1::Point2F(static_cast<FLOAT>(epos.x), static_cast<FLOAT>(epos.y)),
        pBrush,
        bi.size
    );

    pBrush->Release();
}

void WindowGraphics::DrawCircle(Pos2D<float> cpos, FLOAT radius, BrushInfo bi)
{
    DrawEllipse(cpos, radius, radius, bi);
}

void WindowGraphics::FillCircle(Pos2D<float> cpos, FLOAT radius, BrushInfo bi)
{
    FillEllipse(cpos, radius, radius, bi);
}

void WindowGraphics::DrawEllipse(Pos2D<float> cpos, FLOAT width, FLOAT height, BrushInfo bi)
{
    D2D1_ELLIPSE ellipse;
    ellipse.point.x = cpos.x;
    ellipse.point.y = cpos.y;
    ellipse.radiusX = width;
    ellipse.radiusY = height;

    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->DrawEllipse(
        ellipse,
        pBrush,
        bi.size
    );

    pBrush->Release();
}

void WindowGraphics::FillEllipse(Pos2D<float> cpos, FLOAT width, FLOAT height, BrushInfo bi)
{
    D2D1_ELLIPSE ellipse;
    ellipse.point.x = cpos.x;
    ellipse.point.y = cpos.y;
    ellipse.radiusX = width;
    ellipse.radiusY = height;

    if (bi.radialGradient) {
        /*
        ID2D1RadialGradientBrush* pBrush = NULL;

        ID2D1GradientStopCollection *pGradientStops = NULL;

        D2D1_GRADIENT_STOP gradientStops[2];
        gradientStops[0].color = bi.innerColor;
        gradientStops[0].position = 0.0f;
        gradientStops[1].color = bi.outerColor;
        gradientStops[1].position = 1.0f;

        p_RT->CreateGradientStopCollection(
            gradientStops,
            2,
            D2D1_GAMMA_2_2,
            D2D1_EXTEND_MODE_CLAMP,
            &pGradientStops
        );

        p_RT->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(
                D2D1::Point2F(cpos.x, cpos.y),
                D2D1::Point2F(0.0f, 0.0f),
                width,
                height
            ),
            pGradientStops,
            &pBrush
        );
        */

        pBrush->SetCenter(D2D1::Point2F(cpos.x, cpos.y));

        p_Target->FillEllipse(
            ellipse,
            pBrush
        );

        pBrush->Release();

        //pBrush->Release();
        //pGradientStops->Release();
    }
    else {
        ID2D1SolidColorBrush* pBrush = NULL;

        p_Target->CreateSolidColorBrush(
            bi.color,
            &pBrush
        );
        
        p_Target->FillEllipse(
            ellipse,
            pBrush
        );

        pBrush->Release();
    }
}

void WindowGraphics::DrawTriangle(Pos2D<float> pos1, Pos2D<float> pos2, Pos2D<float> pos3, BrushInfo bi)
{
    ID2D1PathGeometry* pTriangleGeometry;

    // Create path geometry
    p_D2DFactory->CreatePathGeometry(&pTriangleGeometry);

    ID2D1GeometrySink *pSink = NULL;
    pTriangleGeometry->Open(&pSink);

    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
    pSink->BeginFigure(
        D2D1::Point2F(pos1.x, pos1.y),
        D2D1_FIGURE_BEGIN_FILLED
    );
    D2D1_POINT_2F points[3] = {
        D2D1::Point2F(pos1.x, pos1.y),
        D2D1::Point2F(pos2.x, pos2.y),
        D2D1::Point2F(pos3.x, pos3.y)
    };
    pSink->AddLines(points, ARRAYSIZE(points));
    pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
    pSink->Close();
    pSink->Release();

    // Draw triangle
    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->DrawGeometry(
        pTriangleGeometry,
        pBrush,
        bi.size
    );

    pBrush->Release();
    pTriangleGeometry->Release();
}

void WindowGraphics::FillTriangle(Pos2D<float> pos1, Pos2D<float> pos2, Pos2D<float> pos3, BrushInfo bi)
{
    ID2D1PathGeometry* pTriangleGeometry;

    // Create path geometry
    p_D2DFactory->CreatePathGeometry(&pTriangleGeometry);

    ID2D1GeometrySink *pSink = NULL;
    pTriangleGeometry->Open(&pSink);

    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
    pSink->BeginFigure(
        D2D1::Point2F(pos1.x, pos1.y),
        D2D1_FIGURE_BEGIN_FILLED
    );
    D2D1_POINT_2F points[3] = {
        D2D1::Point2F(pos1.x, pos1.y),
        D2D1::Point2F(pos2.x, pos2.y),
        D2D1::Point2F(pos3.x, pos3.y)
    };
    pSink->AddLines(points, ARRAYSIZE(points));
    pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
    pSink->Close();
    pSink->Release();

    // Fill triangle
    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->FillGeometry(
        pTriangleGeometry,
        pBrush
    );

    pBrush->Release();
    pTriangleGeometry->Release();
}

void WindowGraphics::DrawGeometry(const std::vector<Pos2D<float>>& points, BrushInfo bi)
{
    ID2D1PathGeometry* pPathGeometry;

    // Create path geometry
    p_D2DFactory->CreatePathGeometry(&pPathGeometry);

    ID2D1GeometrySink *pSink = NULL;
    pPathGeometry->Open(&pSink);

    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
    pSink->BeginFigure(
        D2D1::Point2F(points[0].x, points[0].y),
        D2D1_FIGURE_BEGIN_FILLED
    );
    D2D1_POINT_2F* pPoints = new D2D1_POINT_2F[points.size()];
    for (unsigned i = 0; i < points.size(); i++) {
        pPoints[i] = D2D1::Point2F(points[i].x, points[i].y);
    }

    pSink->AddLines(pPoints, points.size());
    pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
    pSink->Close();
    pSink->Release();

    delete[] pPoints;

    // Create stroke style
    ID2D1StrokeStyle* pStrokeStyle;
    D2D1_STROKE_STYLE_PROPERTIES properties = D2D1::StrokeStyleProperties();
    properties.lineJoin = D2D1_LINE_JOIN_ROUND;
    properties.endCap = D2D1_CAP_STYLE_ROUND;
    properties.startCap = D2D1_CAP_STYLE_ROUND;
    properties.dashCap = D2D1_CAP_STYLE_ROUND;

    p_D2DFactory->CreateStrokeStyle(
        properties,
        nullptr,
        0u,
        &pStrokeStyle
    );

    // Draw terrain geometry
    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->DrawGeometry(
        pPathGeometry,
        pBrush,
        bi.size,
        pStrokeStyle
    );
    
    pStrokeStyle->Release();
    pBrush->Release();
    pPathGeometry->Release();
}

void WindowGraphics::FillGeometry(const std::vector<Pos2D<float>>& points, BrushInfo bi)
{
    ID2D1PathGeometry* pPathGeometry;

    // Create path geometry
    p_D2DFactory->CreatePathGeometry(&pPathGeometry);

    ID2D1GeometrySink *pSink = NULL;
    pPathGeometry->Open(&pSink);

    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
    pSink->BeginFigure(
        D2D1::Point2F(points[0].x, points[0].y),
        D2D1_FIGURE_BEGIN_FILLED
    );

    D2D1_POINT_2F* pPoints = new D2D1_POINT_2F[points.size()];
    for (unsigned i = 0; i < points.size(); i++) {
        pPoints[i] = D2D1::Point2F(points[i].x, points[i].y);
    }

    pSink->AddLines(pPoints, points.size());
    pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
    pSink->Close();
    pSink->Release();

    delete[] pPoints;

    // Draw terrain geometry
    ID2D1SolidColorBrush* pBrush = NULL;
    p_Target->CreateSolidColorBrush(
        bi.color,
        &pBrush
    );

    p_Target->FillGeometry(
        pPathGeometry,
        pBrush
    );

    pBrush->Release();
    pPathGeometry->Release();
}