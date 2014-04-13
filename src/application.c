/* Copyright (c) 2012 Toni Georgiev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define GK_INTERNAL

#include "gk.h"
#include "gk_internal.h"

#ifdef GK_WIN
#include <windows.h>

#include "GLee.h"
#include <GL/gl.h>

HWND gkWindow;
HINSTANCE hinstance;
HDC hdc;
HGLRC hglrc;
GK_BOOL inModalLoop = GK_FALSE;
GK_BOOL wasMaximized = GK_FALSE;
RECT oldRect;

#define UPDATE_TIMER_ID 0x1234

LRESULT WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

#else

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <unistd.h>
#include <libgen.h>
#include <locale.h>
#include <limits.h>

#include "GLee.h"
#include <GL/gl.h>
#include <GL/glx.h>

Display* display;
Window gkWindow;
int screen;
Atom wmDeleteMessage;
XVisualInfo *vi;
GLXContext glCtx;
XF86VidModeModeInfo defaultVideoMode;

#include "X11/imKStoUCS.c"

#endif

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#define GK_MAX_APP_DIR_BUFFER 512

GK_BOOL gkActive;
GK_BOOL gkFullscreen;
GK_BOOL gkWindowResizable = GK_FALSE;
GK_BOOL gkInFrame;
int gkFps = 0;
int gkTargetFps = 60;
gkSize	gkScreenSize = {800,600};

gkPanel* gkMainPanel = 0;

GK_BOOL gkUpdateSize;
GK_BOOL gkFpsLimitEnabled;

void initGk();
void runGk();

void updateGLSize(gkSize sz);

GK_BOOL gkInit()
{
    setlocale(LC_CTYPE, "");

    gkActive = GK_TRUE;
    gkFullscreen = GK_FALSE;
    gkMouse = (gkDispatcher*)malloc(sizeof(gkDispatcher));
    gkKeyboard = (gkDispatcher*)malloc(sizeof(gkDispatcher));
    gkInitDispatcher(gkMouse);
    gkInitDispatcher(gkKeyboard);
	gkMainPanel = gkCreatePanel();
	initGk();
	return GK_TRUE;
}

void gkRun()
{
    runGk();
	gkDestroyPanel(gkMainPanel);
    gkCleanupDispatcher(gkKeyboard);
    gkCleanupDispatcher(gkMouse);
    free(gkKeyboard);
    free(gkMouse);
}

void gkMain(gkInitFunc init, gkCleanupFunc cleanup)
{
	if (gkInit() && init())	{
		gkRun();
		cleanup();
	}
}

void gkExit()
{
    gkActive = GK_FALSE;
}

char gkAppDirBuffer[GK_MAX_APP_DIR_BUFFER];

char* gkGetAppDir()
{
#ifdef GK_WIN
	wchar_t buff[GK_MAX_APP_DIR_BUFFER], *p;
	GetModuleFileNameW(0, buff, GK_MAX_APP_DIR_BUFFER);
	if((p = wcsrchr(buff, L'\\')) != 0) *p = 0;
	wcsncat_s(buff, GK_MAX_APP_DIR_BUFFER, L"\\", 1);

	gkWcsToUtf8(gkAppDirBuffer, buff, GK_MAX_APP_DIR_BUFFER);
#else
    if(readlink("/proc/self/exe", gkAppDirBuffer, GK_MAX_APP_DIR_BUFFER))
    {
        dirname(gkAppDirBuffer);
        strcat(gkAppDirBuffer, "/");
    }
#endif
    return gkAppDirBuffer;
}

void gkSetTargetFps(int targetFps)
{
    int swapIntervalCtrl = targetFps == -1?1:0;
    gkTargetFps = targetFps;
#ifdef GK_WIN
    wglSwapIntervalEXT(swapIntervalCtrl);
#else
    glXSwapIntervalSGI(swapIntervalCtrl);
#endif
    if(gkTargetFps<1)
    {
        gkFpsLimitEnabled = GK_FALSE;
    }
    else
    {
        gkFpsLimitEnabled = GK_TRUE;
    }
}

int gkGetTargetFps()
{
    return gkTargetFps;
}

int gkGetFps()
{
    return gkFps;
}

void gkSetScreenSize(gkSize size)
{
    if(gkActive)
    {
        if(!gkInFrame)
        {

#ifdef GK_WIN
            RECT sz;
            GetClientRect(gkWindow, &sz);
            if((sz.right - sz.left) != size.width || (sz.bottom - sz.top) != size.height)
            {
                sz.right = sz.left + (LONG)size.width;
                sz.bottom = sz.top + (LONG)size.height;
                AdjustWindowRect(&sz, GetWindowLong(gkWindow, GWL_STYLE), 0);
                SetWindowPos(gkWindow, 0, 0, 0, sz.right - sz.left, sz.bottom - sz.top, SWP_NOMOVE|SWP_NOZORDER);
            }
#else

            XSizeHints sizeHints;
            if(!gkWindowResizable)
            {
                sizeHints.flags = PMinSize|PMaxSize;
                sizeHints.max_width = sizeHints.min_width = size.width;
                sizeHints.max_height = sizeHints.min_height = size.height;
                XSetWMNormalHints(display, gkWindow, &sizeHints);
            }

            XResizeWindow(display, gkWindow, size.width, size.height);
            XSync(display, True);
#endif
            gkProcessLayoutMainPanel(gkMainPanel, size.width, size.height);

            updateGLSize(size);

            gkUpdateSize = GK_FALSE;
        }
        else
        {
            gkUpdateSize = GK_TRUE;
        }
    }
    gkScreenSize = size;
    gkSetFullscreen(gkFullscreen);
}

gkSize gkGetScreenSize()
{
    return gkScreenSize;
}

void gkSetFullscreen(GK_BOOL fullscreen)
{
    GK_BOOL oldVal = gkFullscreen;
    gkFullscreen = fullscreen;
    if(fullscreen)
    {
        size_t i, resolutionCount = 0;
        gkSize* resolutions;
        GK_BOOL resolutionSupported = GK_FALSE;
        resolutionCount = gkGetSupportedScreenSizes(0);
        resolutions = (gkSize*)calloc(resolutionCount, sizeof(gkSize));
        gkGetSupportedScreenSizes(resolutions);
        for(i = 0; i<resolutionCount; i++)
        {
            if(resolutions[i].width == gkScreenSize.width && resolutions[i].height == gkScreenSize.height)
            {
                resolutionSupported = GK_TRUE;
                break;
            }
        }
        free(resolutions);
        if(resolutionSupported)
        {
#ifdef GK_WIN
            DEVMODE devmode;
            SetWindowLongPtr(gkWindow, GWL_STYLE, WS_POPUP);
            SetWindowPos(gkWindow, HWND_TOPMOST, 0, 0, gkScreenSize.width, gkScreenSize.height, SWP_FRAMECHANGED);
            ShowWindow(gkWindow, SW_RESTORE);

            devmode.dmSize = sizeof(DEVMODE);
            devmode.dmPelsWidth = gkScreenSize.width;
            devmode.dmPelsHeight = gkScreenSize.height;
            devmode.dmBitsPerPel = 32;
            devmode.dmFields = DM_BITSPERPEL|DM_PELSHEIGHT|DM_PELSWIDTH;

            if(ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                gkFullscreen = GK_FALSE;
            }
#else
            int i, modeCount;
            XSetWindowAttributes attribs;
            XWindowChanges changes;
            XF86VidModeModeInfo** modes;

            attribs.override_redirect = True;

            changes.x = 0;
            changes.y = 0;
            changes.border_width = 0;

            XF86VidModeGetAllModeLines(display, screen, &modeCount, &modes);

            for(i = 0; i<modeCount; i++)
            {
                if(modes[i]->hdisplay == gkScreenSize.width && modes[i]->vdisplay == gkScreenSize.height)
                {
                    XF86VidModeSwitchToMode(display, screen, modes[i]);
                    XF86VidModeSetViewPort(display, screen, 0, 0);

                    XUnmapWindow(display, gkWindow);
                    XSync(display, False);

                    XChangeWindowAttributes(display, gkWindow, CWOverrideRedirect, &attribs);
                    XSync(display, False);
                    XMapRaised(display, gkWindow);

                    XConfigureWindow(display, gkWindow, CWX|CWY|CWBorderWidth, &changes);
                    XGrabKeyboard(display, gkWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);
                    XGrabPointer(display, gkWindow, True, 0 , GrabModeAsync, GrabModeAsync, gkWindow, None, CurrentTime);
                    XFlush(display);
                    break;
                }
            }
            XFree(modes);
#endif
        }
        else
        {
            gkFullscreen = GK_FALSE;
        }
    }
    if(!gkFullscreen)
    {
#ifdef GK_WIN
        UINT_PTR oldStyle, nstyle;
        if(oldVal)
        {
            ChangeDisplaySettings(0,0);	//reset fullscreen
        }
        oldStyle = GetWindowLong(gkWindow, GWL_STYLE);
        nstyle = gkIsWindowResizable()?WS_OVERLAPPEDWINDOW:WS_DLGFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
        if(oldStyle != nstyle)
        {
            RECT nrect;
            SetWindowLongPtr(gkWindow, GWL_STYLE, nstyle);
            nrect.left = 0;
            nrect.right = gkScreenSize.width;
            nrect.top = 0;
            nrect.bottom = gkScreenSize.height;
            AdjustWindowRect(&nrect, nstyle, 0);
            SetWindowPos(gkWindow, HWND_NOTOPMOST, 0, 0, (nrect.right - nrect.left), (nrect.bottom - nrect.top), SWP_NOMOVE|SWP_FRAMECHANGED);
            ShowWindow(gkWindow, SW_RESTORE);
        }
#else
        if(oldVal)
        {
            XSetWindowAttributes attribs;
            attribs.override_redirect = GK_FALSE;

            XF86VidModeSwitchToMode(display, screen, &defaultVideoMode);
            XF86VidModeSetViewPort(display, screen, 0, 0);

            XUnmapWindow(display, gkWindow);

            XChangeWindowAttributes(display, gkWindow, CWOverrideRedirect, &attribs);

            XMapWindow(display, gkWindow);

            XUngrabKeyboard(display, CurrentTime);
            XUngrabPointer(display, CurrentTime);

            XFlush(display);
        }
#endif
    }
}
GK_BOOL gkIsFullscreen()
{
    return gkFullscreen;
}

size_t gkGetSupportedScreenSizes(gkSize* sizes)
{
#ifdef GK_WIN
    int i,t;
    DEVMODE devmode;
    DWORD lastW = 0, lastH = 0;
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    if(sizes)
    {
        for(i = 0, t = 0; EnumDisplaySettings(0, i, &devmode); i++)
        {
            if(devmode.dmBitsPerPel == 32 && (devmode.dmPelsWidth != lastW || devmode.dmPelsHeight != lastH))
            {
                lastW = devmode.dmPelsWidth;
                lastH = devmode.dmPelsHeight;
                sizes[t++] = GK_SIZE(devmode.dmPelsWidth, devmode.dmPelsHeight);
            }
        }
    }
    else
    {
        for(i = 0, t = 0; EnumDisplaySettings(0, i, &devmode); i++)
        {
            if(devmode.dmBitsPerPel == 32 && (devmode.dmPelsWidth != lastW || devmode.dmPelsHeight != lastH))
            {
                lastW = devmode.dmPelsWidth;
                lastH = devmode.dmPelsHeight;
                t++;
            }
        }
    }
    return t;
#else
    int modeCount, i, t = 0, lastW = 0, lastH = 0;
    XF86VidModeModeInfo** modes;
    XF86VidModeGetAllModeLines(display, screen, &modeCount, &modes);
    for(i = 0; i<modeCount; i++)
    {
        if(modes[i]->hdisplay != lastW || modes[i]->vdisplay != lastH)
        {
            if(sizes)
                sizes[t] = GK_SIZE(modes[i]->hdisplay, modes[i]->vdisplay);

            lastW = modes[i]->hdisplay;
            lastH = modes[i]->vdisplay;
            t++;
        }
    }
    XFree(modes);
    return t;
#endif
}

void gkSetWindowTitle(char* title)
{
#ifdef GK_WIN
	wchar_t *titleBuffer = gkWcsFromUtf8(title);
	SetWindowTextW(gkWindow, titleBuffer);
	free(titleBuffer);
#else
	XStoreName(display, gkWindow, title);
#endif
}

char windowNameBuffer[256];

char* gkGetWindowTitle()
{
#ifdef GK_WIN
	wchar_t titleBuffer[256];
	if(gkActive)
		GetWindowTextW(gkWindow, titleBuffer, 256);
	gkWcsToUtf8(windowNameBuffer, titleBuffer, 256);
#else
    XTextProperty nm;
    if(gkActive)
    {
        XGetWMName(display, gkWindow, &nm);
	strncpy(windowNameBuffer, nm.value, 256);
        windowNameBuffer[255] = 0;
    }
#endif
    return windowNameBuffer;
}

void gkSetWindowResizable(GK_BOOL resizable)
{
#ifdef GK_WIN
    LONG_PTR style = GetWindowLongPtr(gkWindow, GWL_STYLE);
    LONG_PTR newStyle = 0;
    gkWindowResizable = resizable;
    if(!gkFullscreen)
    {
        newStyle = gkWindowResizable?WS_OVERLAPPEDWINDOW:WS_DLGFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
        if(newStyle != style)
        {
            RECT nrect;
            SetWindowLongPtr(gkWindow, GWL_STYLE, newStyle);
            nrect.left = 0;
            nrect.right = (LONG)gkScreenSize.width;
            nrect.top = 0;
            nrect.bottom = (LONG)gkScreenSize.height;
            AdjustWindowRect(&nrect, newStyle, 0);
            SetWindowPos(gkWindow, 0, 0, 0, nrect.right - nrect.left, nrect.bottom - nrect.top, SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED);
            ShowWindow(gkWindow, SW_RESTORE);
        }
    }
#else
    XSizeHints sizeHints;
    sizeHints.flags = PMinSize|PMaxSize;
    gkWindowResizable = resizable;
    if(resizable)
    {
        sizeHints.min_width = 0;
        sizeHints.min_height = 0;
        sizeHints.max_width = INT_MAX;
        sizeHints.max_height = INT_MAX;
    }
    else
    {
        sizeHints.max_width = sizeHints.min_width = gkScreenSize.width;
        sizeHints.max_height = sizeHints.min_height = gkScreenSize.height;
    }
    XSetWMNormalHints(display, gkWindow, &sizeHints);
#endif

}

GK_BOOL gkIsWindowResizable()
{
    return gkWindowResizable;
}


///////////////////////////////

void onWindowClose()
{
    gkExit();
}

void onWindowMouseMove(int x, int y)
{
    gkMouseEvent evt;
    evt.type = GK_ON_MOUSE_MOVE;
    evt.target = evt.currentTarget = gkMouse;
    evt.position = GK_POINT(x,y);
    gkGlobalMouseState.position.x = (float)x;
    gkGlobalMouseState.position.y = (float)y;
    gkDispatch(gkMouse, &evt);
    gkProcessMouseEvent(&evt);
}

void onWindowMouseWheel(int x, int y, int w)
{
    gkMouseEvent evt;
    evt.type = GK_ON_MOUSE_WHEEL;
    evt.target = evt.currentTarget = gkMouse;
    evt.position = GK_POINT(gkGlobalMouseState.position.x,gkGlobalMouseState.position.y);
    evt.delta = w;
    gkGlobalMouseState.wheel += w;
    gkDispatch(gkMouse, &evt);
    gkProcessMouseEvent(&evt);
}

void onWindowMouseDown(int x, int y, int mb)
{
    gkMouseEvent evt;
    evt.type = GK_ON_MOUSE_DOWN;
    evt.target = evt.currentTarget = gkMouse;
    evt.button = mb;
    evt.position = GK_POINT(x,y);
    gkGlobalMouseState.position.x = (float)x;
    gkGlobalMouseState.position.y = (float)y;
    if(mb<GK_MAX_MOUSE_BUTTONS)
        gkGlobalMouseState.buttons[mb] = GK_TRUE;
    gkDispatch(gkMouse, &evt);
    gkProcessMouseEvent(&evt);
}

void onWindowMouseUp(int x, int y, int mb)
{
    gkMouseEvent evt;
    evt.type = GK_ON_MOUSE_UP;
    evt.target = evt.currentTarget = gkMouse;
    evt.button = mb;
    evt.position = GK_POINT(x,y);
    gkGlobalMouseState.position.x = (float)x;
    gkGlobalMouseState.position.y = (float)y;
    if(mb<GK_MAX_MOUSE_BUTTONS)
        gkGlobalMouseState.buttons[mb] = GK_FALSE;
    gkDispatch(gkMouse, &evt);
    gkProcessMouseEvent(&evt);
}

void onWindowSizeChanged(gkSize nsize)
{
    gkSetScreenSize(nsize);
}

uint16_t gkLastKeyCode = 0;
uint16_t gkLastScanCode = 0;

#ifdef GK_WIN
GK_BOOL gkLeftAlt = 0;
GK_BOOL gkRightAlt = 0;
uint16_t gkAlt = 0;

GK_BOOL gkLeftCtrl = 0;
GK_BOOL gkRightCtrl = 0;
uint16_t gkCtrl = 0;

GK_BOOL gkLeftShift = 0;
GK_BOOL gkRightShift = 0;
uint16_t gkShift = 0;
#endif

void prepareKey(gkKey* key, uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
#ifdef GK_WIN
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
#endif

    gkGlobalKeyboardState.keys[keyCode] = keyDown;
    key->code = keyCode;
    key->modifiers = 0;
    if(gkGlobalKeyboardState.keys[GK_KEY_NUMLOCK])	key->modifiers |= GK_KEY_MOD_NUM;
    if(gkGlobalKeyboardState.keys[GK_KEY_CAPSLOCK]) key->modifiers |= GK_KEY_MOD_CAPS;
    if(gkGlobalKeyboardState.keys[GK_KEY_LCTRL])	key->modifiers |= GK_KEY_MOD_LCTRL;
    if(gkGlobalKeyboardState.keys[GK_KEY_RCTRL])	key->modifiers |= GK_KEY_MOD_RCTRL;
    if(gkGlobalKeyboardState.keys[GK_KEY_LALT])		key->modifiers |= GK_KEY_MOD_LALT;
    if(gkGlobalKeyboardState.keys[GK_KEY_RALT])		key->modifiers |= GK_KEY_MOD_RALT;
    if(gkGlobalKeyboardState.keys[GK_KEY_LSHIFT])	key->modifiers |= GK_KEY_MOD_LSHIFT;
    if(gkGlobalKeyboardState.keys[GK_KEY_RSHIFT])	key->modifiers |= GK_KEY_MOD_RSHIFT;
}

void onWindowKeyDown(uint16_t keyCode, uint16_t scanCode)
{
    gkKeyboardEvent evt;
    gkKey key;
    prepareKey(&key, keyCode, scanCode, GK_TRUE);
    if(gkLastKeyCode == key.code && gkLastScanCode == scanCode)
    {
        evt.type = GK_ON_KEY_REPEAT;
    }
    else evt.type = GK_ON_KEY_DOWN;
    evt.currentTarget = evt.target = gkKeyboard;
    evt.key = key;
    gkDispatch(gkKeyboard, &evt);
    gkProcessKeyboardEvent(&evt);
    gkLastKeyCode = key.code;
    gkLastScanCode = scanCode;
}

void onWindowKeyUp(uint16_t keyCode, uint16_t scanCode)
{
    gkKeyboardEvent evt;
    gkKey key;
#ifdef GK_LINUX
    char keys[32];
    XQueryKeymap(display, keys);
    if(((keys[keyCode/8]>>(keyCode%8))) & 0x1)
        return; /* Key not really released */
#endif
    prepareKey(&key, keyCode, scanCode, GK_FALSE);
    evt.type = GK_ON_KEY_UP;
    evt.currentTarget = evt.target = gkKeyboard;
    evt.key = key;
    gkDispatch(gkKeyboard, &evt);
    gkProcessKeyboardEvent(&evt);
    if(gkLastKeyCode == key.code && gkLastScanCode == scanCode)
    {
        gkLastKeyCode = 0;
        gkLastScanCode = 0;
    }
}
void onWindowCharacter(uint32_t character)
{
    gkKeyboardEvent evt;
    evt.type = GK_ON_CHARACTER;
    evt.currentTarget = evt.target = gkKeyboard;
    evt.charCode = character;
    gkDispatch(gkKeyboard, &evt);
    gkProcessKeyboardEvent(&evt);
}
//

void updateGLSize(gkSize sz);
void loop();

void initGk()
{
#ifdef GK_WIN
    WNDCLASS wc;
    PIXELFORMATDESCRIPTOR pfd;
    int p;

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
#else
    int glAttribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, GLX_ALPHA_SIZE, 8, GLX_SAMPLE_BUFFERS, 1, GLX_SAMPLES, 2, None };
    XSetWindowAttributes winAttribs;
    Colormap cmap;
    Window root;
    XF86VidModeModeInfo** modes;

    setlocale(LC_CTYPE, "");

    display = XOpenDisplay(NULL);
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    XF86VidModeGetModeLine(display, screen, &defaultVideoMode.dotclock, (XF86VidModeModeLine*)&defaultVideoMode.hdisplay);


    vi = glXChooseVisual(display, screen, glAttribs);
    if(vi == 0)
    {
        printf("GK [ERROR]: (X) No appropriate visual found\n");
    }else{
        printf("GK [INFO]: (X) visual %p selected\n", (void*)vi->visualid);
    }

    cmap = XCreateColormap(display, root, vi->visual, AllocNone);

    winAttribs.colormap = cmap;
    winAttribs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | SubstructureNotifyMask | PointerMotionMask;
    winAttribs.override_redirect = False;

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    gkWindow = XCreateWindow(display, root, 0, 0, (int)gkScreenSize.width, (int)gkScreenSize.height, 0, vi->depth, InputOutput, vi->visual, CWColormap|CWEventMask|CWOverrideRedirect, &winAttribs);
    XStoreName(display, gkWindow, "GKApp");

    glCtx = glXCreateContext(display, vi, NULL, GL_TRUE);
    glXMakeCurrent(display, gkWindow, glCtx);

    if(glXIsDirect(display, glCtx))
    {
        printf("GK [INFO]: Direct GLX\n");
    }else
    {
        printf("GK [INFO]: Not direct GLX\n");
    }

    XMapWindow(display, gkWindow);
    XMoveWindow(display, gkWindow, (DisplayWidth(display, screen) - (int)gkScreenSize.width)>>1,
                (DisplayHeight(display, screen) - (int)gkScreenSize.height)>>1);
    XFlush(display);
#endif

    updateGLSize(gkScreenSize);

    gkSetWindowResizable(GK_FALSE);

    gkAppStartTime = gkMilliseconds();
    gkSetTargetFps(0);

    gkGlobalMouseState.wheel = 0;
	gkInitImages();
   	gkInitFonts();
    gkInitTimers();
    gkInitTweens();
    gkInitJoystick();
    gkInitAudio();
}

#ifdef GK_WIN
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

#else
static void processEvent(XEvent* event)
{
    if(event->type == ClientMessage && event->xclient.data.l[0] == wmDeleteMessage)
    {
        onWindowClose();
    }
    else if(event->type == ButtonPress)
    {
        if(event->xbutton.button <= 3)
            onWindowMouseDown(event->xbutton.x, event->xbutton.y, event->xbutton.button - Button1);
    }
    else if(event->type == ButtonRelease)
    {
        if(event->xbutton.button <= 3)
            onWindowMouseUp(event->xbutton.x, event->xbutton.y, event->xbutton.button - Button1);
        else if(event->xbutton.button == 4)
            onWindowMouseWheel(event->xbutton.x, event->xbutton.y, 1);
        else if(event->xbutton.button == 5)
            onWindowMouseWheel(event->xbutton.x, event->xbutton.y, -1);
    }
    else if(event->type == MotionNotify)
    {
        onWindowMouseMove(event->xmotion.x, event->xmotion.y);
    }else if(event->type == ConfigureNotify)
    {
        gkSize newSize = GK_SIZE(event->xconfigure.width, event->xconfigure.height);
        if(gkScreenSize.width != newSize.width || gkScreenSize.height != newSize.height)
            onWindowSizeChanged(newSize);
    }else if(event->type == KeyPress)
    {
        onWindowKeyDown(event->xkey.keycode, event->xkey.state);
        KeySym ksym = XLookupKeysym(&event->xkey, event->xkey.state);
        if(ksym != NoSymbol)
        {
            onWindowCharacter(X11_KeySymToUcs4(ksym));
        }
        if(event->xkey.keycode == GK_KEY_F4 && gkGlobalKeyboardState.keys[GK_KEY_LALT])
        {
            XLowerWindow(display, gkWindow);
            XSync(display, True);
            printf("GK [INFO]: Alt + F4 clicked, exiting...");
            gkExit();
        }
        else if(event->xkey.keycode == GK_KEY_TAB && gkGlobalKeyboardState.keys[GK_KEY_LALT])
        {
            XUngrabKeyboard(display, CurrentTime);
            XLowerWindow(display, gkWindow);
        }
    }else if(event->type == KeyRelease)
    {
        onWindowKeyUp(event->xkey.keycode, event->xkey.state);
    }else
    {
        printf("GK [DEBUG]: Unknown event %d\n", event->type);
    }
}
#endif

void updateGLSize(gkSize sz)
{
    	glViewport(0,0, (GLsizei)sz.width, (GLsizei)sz.height);
    	glMatrixMode(GL_PROJECTION);
    	glLoadIdentity();
    	glOrtho(0,sz.width,sz.height,0, -1, 1);
    	glMatrixMode(GL_MODELVIEW);
    	glLoadIdentity();
    	glClearColor(0,0,0,0);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	glEnable(GL_BLEND);
    	glEnable(GL_LINE_STIPPLE);

    	gkCleanupDrawToImageBuffer();
    	gkInitDrawToImageBuffer(sz);
}


void processEvents()
{
    int maxMsgPerFrame = 3;
#ifdef GK_WIN
    MSG msg;
    gkUpdateMouseTarget(gkMainPanel);
    gkCheckFocusedPanel();
    while(maxMsgPerFrame-->0 && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#else
    XEvent event;
    gkUpdateMouseTarget(gkMainPanel);
    gkCheckFocusedPanel();
    while(maxMsgPerFrame-->0 && XPending(display)>0)
    {
        XNextEvent(display, &event);
        processEvent(&event);
    }
#endif
}

uint64_t gkTimeSinceLastFrame = 0;
uint64_t gkFpsFrames = 0;
uint64_t gkFpsFreq = 250;
uint64_t gkFpsAccum = 0;

void drawFrame()
{
    uint64_t td;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    gkResetTransform();
    gkResetColorFilter();

    gkInFrame = GK_TRUE;
    td = (gkMilliseconds() - gkTimeSinceLastFrame);
    gkTimeSinceLastFrame = gkMilliseconds();

	gkProcessLayoutMainPanel(gkMainPanel, gkScreenSize.width, gkScreenSize.height);
    gkProcessUpdatePanel(gkMainPanel);
    gkProcessDrawPanel(gkMainPanel);

    gkFpsAccum += td;
    gkFpsFrames++;
    if(gkFpsAccum>=gkFpsFreq)
    {
        gkFps = (int)(1000.0f/((float)gkFpsAccum/(float)gkFpsFrames));
        gkFpsAccum = 0;
        gkFpsFrames = 0;
    }
    gkInFrame = GK_FALSE;

#ifdef GK_WIN
    SwapBuffers(hdc);
#else
    glXSwapBuffers(display, gkWindow);
#endif
}

void loop()
{
    uint64_t sleepTime = gkTargetFps?(1000/gkTargetFps):0;
    uint64_t elapsedTime = gkMilliseconds();
    int rest;

    gkUpdateTweens();
    gkUpdateTimers();
    processEvents();
	drawFrame();

    if(gkUpdateSize)
    {
        gkSetScreenSize(gkScreenSize);
    }

    elapsedTime = gkMilliseconds() - elapsedTime;
    rest = (int)(sleepTime - elapsedTime);
    elapsedTime = gkMilliseconds();
    if(rest>0 && gkFpsLimitEnabled)
    {
#ifdef GK_WIN
        if(rest<20)
        {
            while(gkMilliseconds() - elapsedTime<rest);
        }
        else
        {
            Sleep(rest);
        }
#else
        struct timespec restTime;
        restTime.tv_sec = rest/1000;
        restTime.tv_nsec = (rest%1000)*1000000;
        nanosleep(&restTime, 0);
#endif
    }
}

void runGk()
{
#ifdef GK_WIN
    ShowWindow(gkWindow, SW_SHOW);
#else
    XSetWMProtocols(display, gkWindow, &wmDeleteMessage, 1);
    XMapWindow(display, gkWindow);
    XFlush(display);
#endif
    while(gkActive)
    {
        loop();
    }
    gkCleanupAudio();
	gkCleanupImages();
   	gkCleanupFonts();
    gkCleanupTimers();
    gkCleanupTweens();
    gkCleanupJoystick();
#ifdef GK_WIN
    ReleaseDC(gkWindow, hdc);
    wglMakeCurrent(0,0);
    wglDeleteContext(hglrc);
    DestroyWindow(gkWindow);
    UnregisterClass(L"GKApp", hinstance);
#else
    XDestroyWindow(display, gkWindow);
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glCtx);
    if(gkFullscreen)
    {
        XF86VidModeSwitchToMode(display, screen, &defaultVideoMode);
        XF86VidModeSetViewPort(display, screen, 0, 0);
    }
    XFlush(display);
    XCloseDisplay(display);
#endif
}
