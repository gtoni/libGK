#include "gkPlatform.h"

#ifdef GK_PLATFORM_LINUX

#include <gk.h>
#include "gk_internal.h"

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <stdio.h>
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

void main()
{
	gkAppMain();
}

static GK_BOOL gkActive;

static GK_BOOL initLinux(onInitCallback onInit)
{
    int glAttribs[] = { GLX_RGBA, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 1, None };
    XSetWindowAttributes winAttribs;
    Colormap cmap;
    Window root;
    XF86VidModeModeInfo** modes;

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

	gkActive = GK_TRUE;

	onInit();

	return GK_TRUE;
}

static void runLinux(onRunCallback loop, onRunCallback cleanup)
{
	XSetWMProtocols(display, gkWindow, &wmDeleteMessage, 1);
	XMapWindow(display, gkWindow);
	XFlush(display);
	while(gkActive)
		loop();
	cleanup();
}

static void cleanupLinux()
{
	XDestroyWindow(display, gkWindow);
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, glCtx);
	if (gkIsFullscreen()) {
		XF86VidModeSwitchToMode(display, screen, &defaultVideoMode);
		XF86VidModeSetViewPort(display, screen, 0, 0);
	}
	XFlush(display);
	XCloseDisplay(display);
}

static void ExitApp()
{
	gkActive = GK_FALSE;
}

static void GetAppDirLinux(char* dst, size_t dstSize)
{
	if (readlink("/proc/self/exe", dst, GK_MAX_APPDIR_SIZE)) {
		dirname(dst);
		strcat(dst, "/");
	}
}

static void SetVSyncLinux(GK_BOOL enabled)
{
	glXSwapIntervalSGI(enabled);
}


static void ResizeScreen(gkSize size)
{
	XSizeHints sizeHints;
	if (!gkIsWindowResizable()) {
		sizeHints.flags = PMinSize|PMaxSize;
		sizeHints.max_width = sizeHints.min_width = size.width;
		sizeHints.max_height = sizeHints.min_height = size.height;
		XSetWMNormalHints(display, gkWindow, &sizeHints);
	}
	
	XResizeWindow(display, gkWindow, size.width, size.height);
	XSync(display, True);
}

static int GetSupportedSizes(gkSize* sizes)
{
	int modeCount, i, t = 0, lastW = 0, lastH = 0;
	XF86VidModeModeInfo** modes;
	XF86VidModeGetAllModeLines(display, screen, &modeCount, &modes);
	for (i = 0; i<modeCount; i++) {
		if (modes[i]->hdisplay != lastW || modes[i]->vdisplay != lastH) {
			if(sizes)
				sizes[t] = GK_SIZE(modes[i]->hdisplay, modes[i]->vdisplay);
			lastW = modes[i]->hdisplay;
			lastH = modes[i]->vdisplay;
			t++;
		}
	}
	XFree(modes);
	return t;
}

static GK_BOOL GoFullscreen()
{
	int i, modeCount;
	XSetWindowAttributes attribs;
	XWindowChanges changes;
	XF86VidModeModeInfo** modes;

	attribs.override_redirect = True;

	changes.x = 0;
	changes.y = 0;
	changes.border_width = 0;

	XF86VidModeGetAllModeLines(display, screen, &modeCount, &modes);

	for (i = 0; i<modeCount; i++) {
		if (modes[i]->hdisplay == gkScreenSize.width && modes[i]->vdisplay == gkScreenSize.height) {
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
	return GK_TRUE;
}

static void GoWindowed(GK_BOOL oldVal)
{
	XSetWindowAttributes attribs;

	if (!oldVal) 
		return;

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
	XStoreName(display, gkWindow, title);
}

static void GetWindowTitle(char* dst, size_t dstSize)
{
	XTextProperty nm;
	XGetWMName(display, gkWindow, &nm);
	strncpy(dst, nm.value, dstSize);
	dst[dstSize-1] = 0;
}

static void SetWindowResizable(GK_BOOL resizable)
{
	gkSize screenSize = gkGetScreenSize();
	XSizeHints sizeHints;
	sizeHints.flags = PMinSize|PMaxSize;
	if (resizable) {
		sizeHints.min_width = 0;
		sizeHints.min_height = 0;
		sizeHints.max_width = INT_MAX;
		sizeHints.max_height = INT_MAX;
	} else {
		sizeHints.max_width = sizeHints.min_width = screenSize.width;
		sizeHints.max_height = sizeHints.min_height = screenSize.height;
	}
	XSetWMNormalHints(display, gkWindow, &sizeHints);
}

static void SleepLinux(uint32_t ms)
{
        struct timespec restTime;
        restTime.tv_sec = ms/1000;
        restTime.tv_nsec = (ms%1000)*1000000;
        nanosleep(&restTime, 0);
}

static uint16_t PrepareKeyLinux(uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
}

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
	char keys[32];
	uint16_t keyCode = event->xkey.keycode;
	XQueryKeymap(display, keys);
	if( !(((keys[keyCode/8]>>(keyCode%8))) & 0x1) )
	        onWindowKeyUp(keyCode, event->xkey.state);		
    }else
    {
        printf("GK [DEBUG]: Unknown event %d\n", event->type);
    }
}

static void ProcessEventsLinux()
{
    int maxMsgPerFrame = 3;
    XEvent event;
    while(maxMsgPerFrame-->0 && XPending(display)>0)
    {
        XNextEvent(display, &event);
        processEvent(&event);
    }
}

static void SwapBuffersLinux()
{
	glXSwapBuffers(display, gkWindow);
}

#ifdef __cplusplus
extern "C"{
#endif

	gkPlatform gkGetPlatform()
	{
		gkPlatform platform;

		platform.Init = initLinux;
		platform.Run = runLinux;
		platform.Cleanup = cleanupLinux;
		platform.Exit = ExitApp;

		platform.GetAppDir = GetAppDirLinux;
		platform.SetVSync = SetVSyncLinux;
		platform.ResizeScreen = ResizeScreen;
		platform.GetSupportedSizes = GetSupportedSizes;
		platform.SetFullscreen = SetFullscreen;
		platform.SetWindowTitle = SetWindowTitle;
		platform.GetWindowTitle = GetWindowTitle;
		platform.SetWindowResizable = SetWindowResizable;

		platform.Sleep = SleepLinux;

		platform.PrepareKey = PrepareKeyLinux;

		platform.ProcessEvents = ProcessEventsLinux;

		platform.SwapBuffers = SwapBuffersLinux;

		return platform;
	}


#ifdef __cplusplus
}
#endif

#endif
