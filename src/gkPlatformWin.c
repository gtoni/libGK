#include "gkPlatform.h"

#ifdef GK_PLATFORM_WIN

#include <gk.h>
#include "gk_internal.h"

#include <windows.h>
#include <objbase.h>

#include "GLee.h"
#include <GL/gl.h>

#include <stdio.h>

HWND gkWindow;
HINSTANCE hinstance;
HDC hdc;
HGLRC hglrc;
GK_BOOL inModalLoop = GK_FALSE;
GK_BOOL wasMaximized = GK_FALSE;
RECT oldRect;

#define UPDATE_TIMER_ID 0x1234

LRESULT WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);


static GK_BOOL gkActive;

static GK_BOOL initWin()
{
    WNDCLASS wc;
    PIXELFORMATDESCRIPTOR pfd;
    int p;

    CoInitialize(0);

    hinstance = GetModuleHandle(0);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = 0;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hInstance = hinstance;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.lpszClassName = L"GKApp";
    wc.lpszMenuName = 0;
    wc.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    RegisterClass(&wc);
    gkWindow = CreateWindowEx(WS_EX_APPWINDOW, L"GKApp", L"GKApp", WS_OVERLAPPEDWINDOW, 0, 0, (int)gkScreenSize.width, (int)gkScreenSize.height, 0, 0, hinstance, 0);

    hdc = GetDC(gkWindow);

	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 16;
	pfd.dwFlags = PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_DRAW_TO_WINDOW;

	if(!(p = ChoosePixelFormat(hdc, &pfd)))
	{
		printf("GK [ERROR]: Could not Choose pixel format\n");
	}
	if(!SetPixelFormat(hdc, p, &pfd))
	{
		printf("GK [ERROR]: Could not set pixel format\n");
	}

    if(!(hglrc = wglCreateContext(hdc)))
    {
        printf("GK [ERROR]: Could not create context\n");
    }
    if(!wglMakeCurrent(hdc, hglrc))
    {
        printf("GK [ERROR]: Could not set current context\n");
    }

    if(GLEE_WGL_ARB_pixel_format)
    {
        UINT numFormats, pixelFormat;
        float fAttributes[] = {0,0};
        int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
                              WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
                              WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
                              WGL_COLOR_BITS_ARB,24,
                              WGL_ALPHA_BITS_ARB,8,
                              WGL_DEPTH_BITS_ARB,16,
                              WGL_STENCIL_BITS_ARB,0,
                              WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
                              WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
                              WGL_SAMPLES_ARB, 2 ,						// Check For 2x Multisampling
                              0,0
                            };
        if(wglChoosePixelFormatARB(hdc, iAttributes, fAttributes, 1, &pixelFormat, &numFormats))
		{
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(hglrc);
			ReleaseDC(gkWindow, hdc);
			DestroyWindow(gkWindow);
			gkWindow = CreateWindowEx(WS_EX_APPWINDOW, L"GKApp", L"GKApp", WS_OVERLAPPEDWINDOW, 0, 0, (int)gkScreenSize.width, (int)gkScreenSize.height, 0, 0, hinstance, 0);
			hdc = GetDC(gkWindow);

			if(!SetPixelFormat(hdc, pixelFormat, &pfd))
			{
				printf("GK [ERROR]: (WGL) Could not set pixel format\n");
			}

			if(!(hglrc = wglCreateContext(hdc)))
			{
				printf("GK [ERROR]: (WGL) Count not create context\n");
			}
			if(!wglMakeCurrent(hdc, hglrc))
			{
				printf("GK [ERROR]: (WGL) Count not set current context\n");
			}
			printf("GK [INFO]: Use multisampling\n");
			glEnable(GL_MULTISAMPLE);
		}
		else
		{
			printf("GK [INFO]: No multisampling!\n");
		}
	}else
	{
		printf("GK [INFO]: WGL_ARB_pixel_format extension not supported!\n");
	}

	gkActive = GK_TRUE;

	return GK_TRUE;
}

static void runWin(onRunCallback loop, onRunCallback cleanup)
{
	ShowWindow(gkWindow, SW_SHOW);
	while(gkActive)
		loop();
	cleanup();
}

static void cleanupWin()
{
	ReleaseDC(gkWindow, hdc);
	wglMakeCurrent(0,0);
	wglDeleteContext(hglrc);
	DestroyWindow(gkWindow);
	UnregisterClass(L"GKApp", hinstance);
	CoUninitialize();
}

static void ExitApp()
{
	gkActive = GK_FALSE;
}

static void GetAppDirWin(char* dst, size_t dstSize)
{
	wchar_t buff[GK_MAX_APPDIR_SIZE], *p;
	GetModuleFileNameW(0, buff, GK_MAX_APPDIR_SIZE);
	if((p = wcsrchr(buff, L'\\')) != 0) *p = 0;
	wcsncat_s(buff, GK_MAX_APPDIR_SIZE, L"\\", 1);

	gkWcsToUtf8(dst, buff, dstSize);
}

static void SetVSyncWin(GK_BOOL enabled)
{
	wglSwapIntervalEXT(enabled);
}

static void ResizeScreenWin(gkSize size)
{
	RECT sz;
	GetClientRect(gkWindow, &sz);
	if ((sz.right - sz.left) != size.width || (sz.bottom - sz.top) != size.height) {
		sz.right = sz.left + (LONG)size.width;
		sz.bottom = sz.top + (LONG)size.height;
		AdjustWindowRect(&sz, GetWindowLongPtr(gkWindow, GWL_STYLE), 0);
		SetWindowPos(gkWindow, 0, 0, 0, sz.right - sz.left, sz.bottom - sz.top, SWP_NOMOVE|SWP_NOZORDER);
	}
}

static int GetSupportedSizes(gkSize* sizes)
{
	int i,t;
	DEVMODE devmode;
	DWORD lastW = 0, lastH = 0;
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	for (i = 0, t = 0; EnumDisplaySettings(0, i, &devmode); i++) {
		if (devmode.dmBitsPerPel == 32 && (devmode.dmPelsWidth != lastW || devmode.dmPelsHeight != lastH)) {
			if (sizes)
				sizes[t] = GK_SIZE(devmode.dmPelsWidth, devmode.dmPelsHeight);
			lastW = devmode.dmPelsWidth;
			lastH = devmode.dmPelsHeight;
			t++;
		}
	}
	return t;
}

static GK_BOOL GoFullscreen()
{
	DEVMODE devmode;
	gkSize screenSize = gkGetScreenSize();

	SetWindowLongPtr(gkWindow, GWL_STYLE, WS_POPUP);
	SetWindowPos(gkWindow, HWND_TOPMOST, 0, 0, (int)screenSize.width, (int)screenSize.height, SWP_FRAMECHANGED);
	ShowWindow(gkWindow, SW_RESTORE);

	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmPelsWidth = screenSize.width;
	devmode.dmPelsHeight = screenSize.height;
	devmode.dmBitsPerPel = 32;
	devmode.dmFields = DM_BITSPERPEL|DM_PELSHEIGHT|DM_PELSWIDTH;

	return (ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL);
}

static void GoWindowed(GK_BOOL oldVal)
{
	UINT_PTR oldStyle, nstyle;
	gkSize screenSize = gkGetScreenSize();
	if (oldVal) {
		ChangeDisplaySettings(0,0);	//reset fullscreen
	}
	oldStyle = GetWindowLongPtr(gkWindow, GWL_STYLE);
	nstyle = gkIsWindowResizable() ? WS_OVERLAPPEDWINDOW : WS_DLGFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
	if (oldStyle != nstyle) {
		RECT nrect;
		SetWindowLongPtr(gkWindow, GWL_STYLE, nstyle);
		nrect.left = 0;
		nrect.right = screenSize.width;
		nrect.top = 0;
		nrect.bottom = screenSize.height;
		AdjustWindowRect(&nrect, nstyle, 0);
		SetWindowPos(gkWindow, HWND_NOTOPMOST, 0, 0, (nrect.right - nrect.left), (nrect.bottom - nrect.top), SWP_NOMOVE|SWP_FRAMECHANGED);
		ShowWindow(gkWindow, SW_RESTORE);
	}
}

static GK_BOOL SetFullscreen(GK_BOOL enable)
{
	GK_BOOL wasFullscreen = gkIsFullscreen();
	GK_BOOL success = GK_FALSE;
	if (enable)
		enable = success = GoFullscreen();
	if (!enable)
		GoWindowed(wasFullscreen);
	return success;
}

static void SetWindowTitle(char* title)
{
	wchar_t *titleBuffer = gkWcsFromUtf8(title);
	SetWindowTextW(gkWindow, titleBuffer);
	free(titleBuffer);
}

static void GetWindowTitle(char* dst, size_t dstSize)
{
	wchar_t titleBuffer[GK_MAX_TITLE_SIZE];
	GetWindowTextW(gkWindow, titleBuffer, GK_MAX_TITLE_SIZE);
	gkWcsToUtf8(dst, titleBuffer, dstSize);
}

static void SetWindowResizable(GK_BOOL resizable)
{
	LONG_PTR style, newStyle;
	gkSize screenSize = gkGetScreenSize();

	if (gkIsFullscreen())
		return;

	style = GetWindowLongPtr(gkWindow, GWL_STYLE);
	newStyle = resizable?WS_OVERLAPPEDWINDOW:WS_DLGFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;

	if(newStyle != style) {
		RECT nrect;
		SetWindowLongPtr(gkWindow, GWL_STYLE, newStyle);
		nrect.left = 0;
		nrect.right = (LONG)screenSize.width;
		nrect.top = 0;
		nrect.bottom = (LONG)screenSize.height;
		AdjustWindowRect(&nrect, newStyle, 0);
		SetWindowPos(gkWindow, 0, 0, 0, nrect.right - nrect.left, nrect.bottom - nrect.top, SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED);
		ShowWindow(gkWindow, SW_RESTORE);
	}
}

static void SleepWin(uint32_t ms)
{
	Sleep(ms);
}

GK_BOOL gkLeftAlt = 0;
GK_BOOL gkRightAlt = 0;
uint16_t gkAlt = 0;

GK_BOOL gkLeftCtrl = 0;
GK_BOOL gkRightCtrl = 0;
uint16_t gkCtrl = 0;

GK_BOOL gkLeftShift = 0;
GK_BOOL gkRightShift = 0;
uint16_t gkShift = 0;

static uint16_t PrepareKeyWin(uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
    if(keyCode == VK_MENU)
    {
        GK_BOOL gkLeftAltNew = (GetKeyState(VK_LMENU)&0x80) != 0;
        GK_BOOL gkRightAltNew = (GetKeyState(VK_RMENU)&0x80) != 0;
        if(keyDown)
        {
            if(gkLeftAltNew && !gkLeftAlt)
            {
                gkAlt = GK_KEY_LALT;
            }
            else if(gkRightAltNew && !gkRightAlt)
            {
                gkAlt = GK_KEY_RALT;
            }
            keyCode = gkAlt;
        }
        else
        {
            if(!gkLeftAltNew && gkLeftAlt)
            {
                keyCode = GK_KEY_LALT;
            }
            else if(!gkRightAltNew && gkRightAlt)
            {
                keyCode = GK_KEY_RALT;
            }
        }
        gkLeftAlt = gkLeftAltNew;
        gkRightAlt = gkRightAltNew;
    }
    else if(keyCode == VK_CONTROL)
    {
        GK_BOOL gkLeftCtrlNew = (GetKeyState(VK_LCONTROL)&0x80) != 0;
        GK_BOOL gkRightCtrlNew = (GetKeyState(VK_RCONTROL)&0x80) != 0;
        if(keyDown)
        {
            if(gkLeftCtrlNew && !gkLeftCtrl)
            {
                gkCtrl = GK_KEY_LCTRL;
            }
            else if(gkRightCtrlNew && !gkRightCtrl)
            {
                gkCtrl = GK_KEY_RCTRL;
            }
            keyCode = gkCtrl;
        }
        else
        {
            if(!gkLeftCtrlNew && gkLeftCtrl)
            {
                keyCode = GK_KEY_LCTRL;
            }
            else if(!gkRightCtrlNew && gkRightCtrl)
            {
                keyCode = GK_KEY_RCTRL;
            }
        }
        gkLeftCtrl = gkLeftCtrlNew;
        gkRightCtrl = gkRightCtrlNew;
    }
    else if(keyCode == VK_SHIFT)
    {
        GK_BOOL gkLeftShiftNew = (GetKeyState(VK_LSHIFT)&0x80) != 0;
        GK_BOOL gkRightShiftNew = (GetKeyState(VK_RSHIFT)&0x80) != 0;
        if(keyDown)
        {
            if(gkLeftShiftNew && !gkLeftShift)
            {
                gkShift = GK_KEY_LSHIFT;
            }
            else if(gkRightShiftNew && !gkRightShift)
            {
                gkShift = GK_KEY_RSHIFT;
            }
            keyCode = gkShift;
        }
        else
        {
            if(!gkLeftShiftNew && gkLeftShift)
            {
                keyCode = GK_KEY_LSHIFT;
            }
            else if(!gkRightShiftNew && gkRightShift)
            {
                keyCode = GK_KEY_RSHIFT;
            }
        }
        gkLeftShift = gkLeftShiftNew;
        gkRightShift = gkRightShiftNew;
    }
    return keyCode;
}

void TimerUpdate(HWND wnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
    if(msg == WM_TIMER)
    {
        RECT rect;
        gkSize sz;
        GetClientRect(gkWindow, &rect);
        sz.width = (float)(rect.right - rect.left);
        sz.height = (float)(rect.bottom - rect.top);
        if(sz.width != gkScreenSize.width || sz.height != gkScreenSize.height)
        {
            updateGLSize(sz);
            gkProcessLayoutMainPanel(gkMainPanel, sz.width, sz.height);
            gkScreenSize = sz;
        }
        loop();
    }
}

LRESULT WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_CLOSE:
        onWindowClose();
        break;
    case WM_MOUSEMOVE:
        onWindowMouseMove(LOWORD(lParam),HIWORD(lParam));
        return 0;
        break;
    case WM_MOUSEWHEEL:
        onWindowMouseWheel(LOWORD(lParam), HIWORD(lParam), GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA);
        return 0;
        break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        int mb;
        if(msg == WM_LBUTTONDOWN)	mb = 0;
        else if(msg == WM_MBUTTONDOWN)	mb = 1;
        else if(msg == WM_RBUTTONDOWN)	mb = 2;
        onWindowMouseDown(LOWORD(lParam), HIWORD(lParam), mb);
        return 0;
    }
    break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    {
        int mb;
        if(msg == WM_LBUTTONUP)	mb = 0;
        else if(msg == WM_MBUTTONUP)	mb = 1;
        else if(msg == WM_RBUTTONUP)	mb = 2;
        onWindowMouseUp(LOWORD(lParam), HIWORD(lParam), mb);
        return 0;
    }
    break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        onWindowKeyDown(wParam, (lParam>>16)&0x7F);
        return 0;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        onWindowKeyUp(wParam, (lParam>>16)&0x7F);
        return 0;
        break;
    case WM_UNICHAR:
	    if(wParam == UNICODE_NOCHAR)
		    return 1;
        onWindowCharacter((uint32_t)wParam);
	return 0;
	break;
    case WM_CHAR:
        onWindowCharacter((uint32_t)LOWORD(wParam));
        break;
    case WM_SIZE:
        if(wParam == SIZE_MAXIMIZED || (wParam == SIZE_RESTORED && wasMaximized))
        {
            if(!wasMaximized) wasMaximized = GK_TRUE;
            if(!inModalLoop)
            {
                gkSize sz = {LOWORD(lParam), HIWORD(lParam)};
                if(sz.width != gkScreenSize.width || sz.height != gkScreenSize.height)
                {
                    onWindowSizeChanged(sz);
                }
            }
        }
        break;
    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
        if(!inModalLoop)
        {
            GetClientRect(gkWindow, &oldRect);
            SetTimer(gkWindow,UPDATE_TIMER_ID, 30, (TIMERPROC)TimerUpdate);
            inModalLoop = GK_TRUE;
        }
        break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
    {
        if(inModalLoop)
        {
            RECT rect;
            KillTimer(gkWindow, UPDATE_TIMER_ID);
            GetClientRect(gkWindow, &rect);
            if((rect.right - rect.left) != (oldRect.right - oldRect.left) ||
                    (rect.bottom - rect.top) != (oldRect.bottom - oldRect.top))
            {
                gkSize s = {(float)(rect.right - rect.left), (float)(rect.bottom - rect.top)};
                oldRect = rect;
                onWindowSizeChanged(s);
            }
            inModalLoop = GK_FALSE;
        }
    }
    break;
    }
    return DefWindowProc(wnd, msg, wParam, lParam);
}

static void ProcessEventsWin()
{
    int maxMsgPerFrame = 3;
    MSG msg;
    while(maxMsgPerFrame-->0 && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void SwapBuffersWin()
{
	SwapBuffers(hdc);
}

#ifdef __cplusplus
extern "C"{
#endif

	gkPlatform gkGetPlatform()
	{
		gkPlatform platform;

		platform.Init = initWin;
		platform.Run = runWin;
		platform.Cleanup = cleanupWin;
		platform.Exit = ExitApp;

		platform.GetAppDir = GetAppDirWin;
		platform.SetVSync = SetVSyncWin;

		platform.ResizeScreen = ResizeScreenWin;
		platform.GetSupportedSizes = GetSupportedSizes;
		platform.SetFullscreen = SetFullscreen;

		platform.SetWindowTitle = SetWindowTitle;
		platform.GetWindowTitle = GetWindowTitle;

		platform.SetWindowResizable = SetWindowResizable;

		platform.Sleep = SleepWin;

		platform.PrepareKey = PrepareKeyWin;

		platform.ProcessEvents = ProcessEventsWin;

		platform.SwapBuffers = SwapBuffersWin;

		return platform;
	}


#ifdef __cplusplus
}
#endif

#endif