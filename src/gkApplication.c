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

#include "gkPlatform.h"

#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

static gkPlatform Platform;

GK_BOOL gkFullscreen;
GK_BOOL gkWindowResizable = GK_FALSE;
GK_BOOL gkInFrame;
int gkFps = 0;
int gkTargetFps = 60;
gkSize	gkScreenSize = {800,600};

gkPanel* gkMainPanel = 0;

GK_BOOL gkUpdateSize;
GK_BOOL gkFpsLimitEnabled;

char gkAppDirBuffer[GK_MAX_APPDIR_SIZE];
char windowNameBuffer[GK_MAX_TITLE_SIZE];

gkCleanupFunc cleanupFunc;

GK_BOOL gkInit();
void gkRun();
void gkCleanup();

void updateGLSize(gkSize sz);

void gkMain(gkInitFunc init, gkCleanupFunc cleanup)
{
	Platform = gkGetPlatform();

	if (!gkInit())
		return;

	if (init()) {
		cleanupFunc = cleanup;
		gkRun();
	} else {
		cleanupFunc = 0;
		gkCleanup();
	}
}

void gkExit()
{
	Platform.Exit();
}


char* gkGetAppDir()
{
	Platform.GetAppDir(gkAppDirBuffer, GK_MAX_APPDIR_SIZE);
	return gkAppDirBuffer;
}

void gkSetTargetFps(int targetFps)
{
	gkTargetFps = targetFps;
	gkFpsLimitEnabled = (gkTargetFps >= 1);
	Platform.SetVSync(targetFps == -1);
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
	if (!gkInFrame) {
		Platform.ResizeScreen(size);
		gkProcessLayoutMainPanel(gkMainPanel, size.width, size.height);
		updateGLSize(size);
		gkUpdateSize = GK_FALSE;
	} else {
	    gkUpdateSize = GK_TRUE;
	}
	gkScreenSize = size;
	gkSetFullscreen(gkFullscreen);
}

gkSize gkGetScreenSize()
{
	return gkScreenSize;
}

static GK_BOOL gkCanSwitchFullscreen()
{
	size_t i, resolutionCount = 0;
	gkSize* resolutions;
	GK_BOOL resolutionSupported = GK_FALSE;
	resolutionCount = gkGetSupportedScreenSizes(0);
	resolutions = (gkSize*)calloc(resolutionCount, sizeof(gkSize));
	gkGetSupportedScreenSizes(resolutions);
	for (i = 0; i < resolutionCount; i++) {
		if (resolutions[i].width == gkScreenSize.width && resolutions[i].height == gkScreenSize.height) {
			resolutionSupported = GK_TRUE;
			break;
		}
	}
	free(resolutions);
	return resolutionSupported;
}

void gkSetFullscreen(GK_BOOL fullscreen)
{
	gkFullscreen = Platform.SetFullscreen(fullscreen && gkCanSwitchFullscreen());
}

GK_BOOL gkIsFullscreen()
{
	return gkFullscreen;
}

size_t gkGetSupportedScreenSizes(gkSize* sizes)
{
	return Platform.GetSupportedSizes(sizes);
}

void gkSetWindowTitle(char* title)
{
	Platform.SetWindowTitle(title);
}

char* gkGetWindowTitle()
{
	Platform.GetWindowTitle(windowNameBuffer, GK_MAX_TITLE_SIZE);
	return windowNameBuffer;
}

void gkSetWindowResizable(GK_BOOL resizable)
{
	gkWindowResizable = resizable;
	Platform.SetWindowResizable(resizable);
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

void prepareKey(gkKey* key, uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
	keyCode = Platform.PrepareKey(keyCode, scanCode, keyDown);

	gkGlobalKeyboardState.keys[keyCode] = keyDown;
	key->code = keyCode;
	key->modifiers = 0;
	if(gkGlobalKeyboardState.keys[GK_KEY_NUMLOCK])	key->modifiers |= GK_KEY_MOD_NUM;
	if(gkGlobalKeyboardState.keys[GK_KEY_CAPSLOCK]) key->modifiers |= GK_KEY_MOD_CAPS;
	if(gkGlobalKeyboardState.keys[GK_KEY_LCTRL])	key->modifiers |= GK_KEY_MOD_LCTRL;
	if(gkGlobalKeyboardState.keys[GK_KEY_RCTRL])	key->modifiers |= GK_KEY_MOD_RCTRL;
	if(gkGlobalKeyboardState.keys[GK_KEY_LALT])	key->modifiers |= GK_KEY_MOD_LALT;
	if(gkGlobalKeyboardState.keys[GK_KEY_RALT])	key->modifiers |= GK_KEY_MOD_RALT;
	if(gkGlobalKeyboardState.keys[GK_KEY_LSHIFT])	key->modifiers |= GK_KEY_MOD_LSHIFT;
	if(gkGlobalKeyboardState.keys[GK_KEY_RSHIFT])	key->modifiers |= GK_KEY_MOD_RSHIFT;
}

void onWindowKeyDown(uint16_t keyCode, uint16_t scanCode)
{
	gkKeyboardEvent evt;
	gkKey key;
	prepareKey(&key, keyCode, scanCode, GK_TRUE);

	if (gkLastKeyCode == key.code && gkLastScanCode == scanCode)
		evt.type = GK_ON_KEY_REPEAT;
	else 
		evt.type = GK_ON_KEY_DOWN;

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
	if (gkLastKeyCode == key.code && gkLastScanCode == scanCode) {
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

GK_BOOL gkInit()
{
	setlocale(LC_CTYPE, "");

	Platform.Init();	

	gkFullscreen = GK_FALSE;

	gkMouse = (gkDispatcher*)malloc(sizeof(gkDispatcher));
	gkKeyboard = (gkDispatcher*)malloc(sizeof(gkDispatcher));
	gkInitDispatcher(gkMouse);
	gkInitDispatcher(gkKeyboard);

	gkMainPanel = gkCreatePanel();

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

	return GK_TRUE;
}

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
	gkUpdateMouseTarget(gkMainPanel);
	gkCheckFocusedPanel();
	Platform.ProcessEvents();
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
	if (gkFpsAccum >= gkFpsFreq) {
		gkFps = (int)(1000.0f/((float)gkFpsAccum/(float)gkFpsFrames));
		gkFpsAccum = 0;
		gkFpsFrames = 0;
	}
	gkInFrame = GK_FALSE;

	Platform.SwapBuffers();
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

	if (gkUpdateSize)
		gkSetScreenSize(gkScreenSize);

	elapsedTime = gkMilliseconds() - elapsedTime;
	rest = (int)(sleepTime - elapsedTime);
	elapsedTime = gkMilliseconds();

	if (rest>0 && gkFpsLimitEnabled)
		Platform.Sleep(rest);
}

void gkRun()
{
	Platform.Run(loop, gkCleanup);
}

void gkCleanup()
{
	if (cleanupFunc)
		cleanupFunc();

	gkCleanupAudio();
	gkCleanupImages();
   	gkCleanupFonts();
	gkCleanupTimers();
	gkCleanupTweens();
	gkCleanupJoystick();

	gkDestroyPanel(gkMainPanel);
	gkCleanupDispatcher(gkKeyboard);
	gkCleanupDispatcher(gkMouse);
	free(gkKeyboard);
	free(gkMouse);

	Platform.Cleanup();
}
