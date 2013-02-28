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

#include "gk.h"
#include "gk_internal.h"

#if defined(_WIN32)
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
#include <stdio.h>

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

#endif


#include <stdlib.h>

#define GK_MAX_APP_DIR_BUFFER 512

GK_BOOL gkActive;
GK_BOOL gkFullscreen;
GK_BOOL gkWindowResizable;
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

GK_BOOL gkInit(enum gkInitFlags flags)
{
    gkActive = GK_TRUE;
    gkFullscreen = GK_FALSE;
    gkMouse = (gkListenerList*)malloc(sizeof(gkListenerList));
    gkKeyboard = (gkListenerList*)malloc(sizeof(gkListenerList));
    gkInitListenerList(gkMouse);
    gkInitListenerList(gkKeyboard);
    initGk();
    return GK_TRUE;
}

void gkRun()
{
    runGk();
    gkCleanupListenerList(gkKeyboard);
    gkCleanupListenerList(gkMouse);
    free(gkKeyboard);
    free(gkMouse);
}

void gkExit()
{
    gkActive = GK_FALSE;
}

wchar_t gkAppDirBuffer[GK_MAX_APP_DIR_BUFFER];
wchar_t* gkGetAppDir()
{
    wchar_t* p;
#if defined(_WIN32)
    GetModuleFileNameW(0, gkAppDirBuffer, GK_MAX_APP_DIR_BUFFER);
    if((p = strrchr(gkAppDirBuffer, L'\\')) != 0) *p = 0;
    wcsncat_s(gkAppDirBuffer, GK_MAX_APP_DIR_BUFFER, L"\\", 1);
#else
    char buff[GK_MAX_APP_DIR_BUFFER];
    if(readlink("/proc/self/exe", buff, GK_MAX_APP_DIR_BUFFER*sizeof(char)))
    {
        dirname(buff);
        strcat(buff, "/");
        mbstowcs(gkAppDirBuffer, buff, sizeof(wchar_t)*GK_MAX_APP_DIR_BUFFER);
    }
#endif
    return gkAppDirBuffer;
}

void gkSetTargetFps(int targetFps)
{
    int swapIntervalCtrl = targetFps == -1?1:0;
    gkTargetFps = targetFps;
#if defined(_WIN32)
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

#if defined(_WIN32)
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
            XResizeWindow(display, gkWindow, size.width, size.height);
            XSync(display, False);
#endif
            if(gkMainPanel && gkMainPanel->resizeFunc)
            {
                gkMainPanel->resizeFunc(gkMainPanel, size.width, size.height);
            }

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
#if defined(_WIN32)
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
            XF86VidModeModeInfo** modes;
            XF86VidModeGetAllModeLines(display, screen, &modeCount, &modes);
            if(oldVal == GK_FALSE)
            {
                defaultVideoMode = *modes[0];
            }
            for(i = 0; i<modeCount; i++)
            {
                if(modes[i]->hdisplay == gkScreenSize.width && modes[i]->vdisplay == gkScreenSize.height)
                {
                    XSetWindowAttributes attribs;
                    XWindowChanges changes;
                    XF86VidModeSwitchToMode(display, screen, modes[i]);
                    XF86VidModeSetViewPort(display, screen, 0, 0);
                    XFlush(display);
                    XUnmapWindow(display, gkWindow);
                    attribs.override_redirect = True;
                    XChangeWindowAttributes(display, gkWindow, CWOverrideRedirect, &attribs);
                    XSync(display, False);
                    XMapRaised(display, gkWindow);
                    changes.x = 0;
                    changes.y = 0;
                    changes.border_width = 0;
                    XConfigureWindow(display, gkWindow, CWX|CWY|CWBorderWidth, &changes);
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
#if defined(_WIN32)
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
            XF86VidModeSwitchToMode(display, screen, &defaultVideoMode);
            XF86VidModeSetViewPort(display, screen, 0, 0);
            XUnmapWindow(display, gkWindow);
            attribs.override_redirect = GK_FALSE;
            XChangeWindowAttributes(display, gkWindow, CWOverrideRedirect, &attribs);
            XMapWindow(display, gkWindow);
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
#if defined(_WIN32)
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

void gkSetWindowTitle(wchar_t* title)
{
#if defined(_WIN32)
    SetWindowTextW(gkWindow, title);
#else
    char titleBuffer[256];
    size_t len;
    setlocale(LC_CTYPE, "");
    len = wcstombs(titleBuffer, title, sizeof(titleBuffer));
    if(len == 256) titleBuffer[255] = 0;
    XStoreName(display, gkWindow, titleBuffer);
#endif
}

wchar_t windowNameBuffer[256];
wchar_t* gkGetWindowTitle()
{
#if defined(_WIN32)
    if(gkActive)
        GetWindowTextW(gkWindow, windowNameBuffer, 256);
#else
    char titleBuffer[256];
    size_t len;
    XTextProperty nm;
    if(gkActive)
    {
        XGetWMName(display, gkWindow, &nm);
        setlocale(LC_CTYPE, "");
        len = mbstowcs(windowNameBuffer, nm.value, sizeof(windowNameBuffer));
        if(len == 256) windowNameBuffer[255] = 0;
    }
#endif
    return windowNameBuffer;
}

void gkSetWindowResizable(GK_BOOL resizable)
{
#if defined(_WIN32)
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


void gkSetMainPanel(gkPanel* panel)
{
    gkMainPanel = panel;
    if(gkMainPanel)
    {
        gkMainPanel->x = 0;
        gkMainPanel->y = 0;
        if(gkMainPanel->resizeFunc)
        {
            gkMainPanel->resizeFunc(gkMainPanel, gkScreenSize.width, gkScreenSize.height);
        }
    }
}
gkPanel* gkGetMainPanel()
{
    return gkMainPanel;
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

#ifdef WIN32
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
#ifdef _WIN32
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
void onWindowCharacter(wchar_t character)
{
    gkKeyboardEvent evt;
    evt.type = GK_ON_CHARACTER;
    evt.currentTarget = evt.target = gkKeyboard;
    evt.character = character;
    gkDispatch(gkKeyboard, &evt);
    gkProcessKeyboardEvent(&evt);
}
//

void updateGLSize(gkSize sz);
void loop();

void initGk()
{
#if defined(_WIN32)
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

    pfd.nVersion = 1;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 16;
    pfd.dwFlags = PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_DRAW_TO_WINDOW;

    hdc = GetDC(gkWindow);

    if(!(p = ChoosePixelFormat(hdc, &pfd)))
    {
        printf("Count not Choose pixel format\n");
    }
    if(!SetPixelFormat(hdc, p, &pfd))
    {
        printf("Could not set pixel format\n");
    }

    if(!(hglrc = wglCreateContext(hdc)))
    {
        printf("Count not create context\n");
    }
    if(!wglMakeCurrent(hdc, hglrc))
    {
        printf("Count not set current context\n");
    }
#else
    int glAttribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, GLX_ALPHA_SIZE, 8, GLX_SAMPLE_BUFFERS, 1, GLX_SAMPLES, 2, None };
    XSetWindowAttributes winAttribs;
    Colormap cmap;
    Window root;

    display = XOpenDisplay(NULL);
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    vi = glXChooseVisual(display, screen, glAttribs);
    if(vi == 0)
    {
        printf("No appropriate visual found\n");
    }else{
        printf("visual %p selected\n", (void*)vi->visualid);
    }

    cmap = XCreateColormap(display, root, vi->visual, AllocNone);

    winAttribs.colormap = cmap;
    winAttribs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | SubstructureNotifyMask | PointerMotionMask;

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    gkWindow = XCreateWindow(display, root, 0, 0, (int)gkScreenSize.width, (int)gkScreenSize.height, 0, vi->depth, InputOutput, vi->visual, CWColormap|CWEventMask, &winAttribs);
    XStoreName(display, gkWindow, "GKApp");

    glCtx = glXCreateContext(display, vi, NULL, GL_TRUE);
    glXMakeCurrent(display, gkWindow, glCtx);

    if(glXIsDirect(display, glCtx))
    {
        printf("Direct GLX\n");
    }else
    {
        printf("Not direct GLX\n");
    }

    XMapWindow(display, gkWindow);
#endif

    updateGLSize(gkScreenSize);

    gkSetWindowResizable(GK_FALSE);

    gkAppStartTime = gkMilliseconds();
    gkSetTargetFps(0);

#if defined(_WIN32)
    /*Antialias multi-sample 4x*/
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
                              WGL_SAMPLES_ARB, 4 ,						// Check For 4x Multisampling
                              0,0
                            };
        if(wglChoosePixelFormatARB(hdc, iAttributes, fAttributes, 1, &pixelFormat, &numFormats))
        {
            ReleaseDC(gkWindow, hdc);
            DestroyWindow(gkWindow);
            gkWindow = CreateWindowEx(WS_EX_APPWINDOW, L"GKApp", L"GKApp", WS_OVERLAPPEDWINDOW, 0, 0, (int)gkScreenSize.width, (int)gkScreenSize.height, 0, 0, hinstance, 0);
            hdc = GetDC(gkWindow);

            if(!SetPixelFormat(hdc, pixelFormat, &pfd))
            {
                printf("MS: Could not set pixel format\n");
            }

            if(!(hglrc = wglCreateContext(hdc)))
            {
                printf("MS: Count not create context\n");
            }
            if(!wglMakeCurrent(hdc, hglrc))
            {
                printf("MS: Count not set current context\n");
            }

            updateGLSize(gkScreenSize);

            gkSetWindowResizable(GK_FALSE);
            gkSetTargetFps(0);
            glEnable(GL_MULTISAMPLE);
        }
    }
#else
#endif
    gkGlobalMouseState.wheel = 0;
	gkInitImages();
   	gkInitFonts();
    gkInitTimers();
    gkInitTweens();
    gkInitJoystick();
}

#if defined(_WIN32)
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
            if(gkMainPanel && gkMainPanel->resizeFunc)
            {
                gkMainPanel->resizeFunc(gkMainPanel, sz.width, sz.height);
            }
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
    case WM_CHAR:
        onWindowCharacter((wchar_t)wParam);
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
#if defined(_WIN32)
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
    if(gkMainPanel)
    {
        gkProcessUpdatePanel(gkMainPanel);
        gkProcessDrawPanel(gkMainPanel);
    }
    gkFpsAccum += td;
    gkFpsFrames++;
    if(gkFpsAccum>=gkFpsFreq)
    {
        gkFps = (int)(1000.0f/((float)gkFpsAccum/(float)gkFpsFrames));
        gkFpsAccum = 0;
        gkFpsFrames = 0;
    }
    gkInFrame = GK_FALSE;

#if defined(_WIN32)
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
#if defined(_WIN32)
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
#if defined(_WIN32)
    ShowWindow(gkWindow, SW_SHOW);
#else
    XMapWindow(display, gkWindow);
    XSetWMProtocols(display, gkWindow, &wmDeleteMessage, 1);
    XFlush(display);
#endif
    while(gkActive)
    {
        loop();
    }
	gkCleanupImages();
   	gkCleanupFonts();
    gkCleanupTimers();
    gkCleanupTweens();
    gkCleanupJoystick();
#if defined(_WIN32)
    ReleaseDC(gkWindow, hdc);
    wglMakeCurrent(0,0);
    wglDeleteContext(hglrc);
    DestroyWindow(gkWindow);
    UnregisterClass(L"GKApp", hinstance);
#else
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
