/* Copyright (c) 2014 Toni Georgiev
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

#include "gkGL.h"

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

gkInitFunc initFunc;
gkCleanupFunc cleanupFunc;

GK_BOOL gkInit();
void loop();
void gkCleanup();

void updateGLSize(gkSize sz);

void onPlatformInit()
{
	if (!gkInit()) {
		gkExit();
		return;
	}

	if (!initFunc()) {
		cleanupFunc = 0;
		gkCleanup();
		gkExit();
		return;
	}

	Platform.Run(loop, gkCleanup);
}

void gkMain(gkInitFunc init, gkCleanupFunc cleanup)
{
	initFunc = init;
	cleanupFunc = cleanup;

	setlocale(LC_CTYPE, "");

	Platform = gkGetPlatform();
	Platform.Init(onPlatformInit);
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

GK_BOOL onWindowKeyDown(uint16_t keyCode, uint16_t scanCode)
{
	GK_BOOL result = GK_TRUE;
	gkKeyboardEvent evt;
	gkKey key;
	prepareKey(&key, keyCode, scanCode, GK_TRUE);

	if (gkLastKeyCode == key.code && gkLastScanCode == scanCode)
		evt.type = GK_ON_KEY_REPEAT;
	else 
		evt.type = GK_ON_KEY_DOWN;

	evt.currentTarget = evt.target = gkKeyboard;
	evt.key = key;

	if (!gkDispatch(gkKeyboard, &evt))
		result = GK_FALSE;

	if (!gkProcessKeyboardEvent(&evt)) 
		result = GK_FALSE;

	gkLastKeyCode = key.code;
	gkLastScanCode = scanCode;

	return result;
}

GK_BOOL onWindowKeyUp(uint16_t keyCode, uint16_t scanCode)
{
	GK_BOOL result = GK_TRUE;
	gkKeyboardEvent evt;
	gkKey key;

	prepareKey(&key, keyCode, scanCode, GK_FALSE);

	evt.type = GK_ON_KEY_UP;
	evt.currentTarget = evt.target = gkKeyboard;
	evt.key = key;

	if (!gkDispatch(gkKeyboard, &evt))
		result = GK_FALSE;

	if (!gkProcessKeyboardEvent(&evt)) 
		result = GK_FALSE;

	if (gkLastKeyCode == key.code && gkLastScanCode == scanCode) {
		gkLastKeyCode = 0;
		gkLastScanCode = 0;
	}

	return result;
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
#ifdef GK_USE_FONTS
   	gkInitFonts();
#endif
	gkInitTimers();
	gkInitTweens();
#ifdef GK_USE_JOYSTICK
	gkInitJoystick();
#endif
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
    	glClearColor(0,0,0,1.0f);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	glEnable(GL_BLEND);

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

uint64_t beginFrame()
{
	uint64_t td;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gkResetTransform();
	gkResetColorFilter();

	gkInFrame = GK_TRUE;
	td = (gkMilliseconds() - gkTimeSinceLastFrame);
	gkTimeSinceLastFrame = gkMilliseconds();

	return td;
}

void endFrame(uint64_t deltaTime)
{
	gkFpsAccum += deltaTime;
	gkFpsFrames++;
	if (gkFpsAccum >= gkFpsFreq) {
		gkFps = (int)(1000.0f / ((float)gkFpsAccum / (float)gkFpsFrames));
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
	uint64_t deltaTime;
	int rest;

	deltaTime = beginFrame();

	gkProcessLayoutMainPanel(gkMainPanel, gkScreenSize.width, gkScreenSize.height);
	gkProcessUpdatePanel(gkMainPanel);

	gkUpdateTweens();
	gkUpdateTimers();
	processEvents();

	gkProcessDrawPanel(gkMainPanel);

	endFrame(deltaTime);

	if (gkUpdateSize)
		gkSetScreenSize(gkScreenSize);

	elapsedTime = gkMilliseconds() - elapsedTime;
	rest = (int)(sleepTime - elapsedTime);
	elapsedTime = gkMilliseconds();

	if (rest>0 && gkFpsLimitEnabled)
		Platform.Sleep(rest);
}

void gkCleanup()
{
	if (cleanupFunc)
		cleanupFunc();

	gkCleanupAudio();
	gkCleanupImages();
#ifdef GK_USE_FONTS
   	gkCleanupFonts();
#endif
	gkCleanupTimers();
	gkCleanupTweens();
#ifdef GK_USE_JOYSTICK
	gkCleanupJoystick();
#endif

	gkDestroyPanel(gkMainPanel);
	gkCleanupDispatcher(gkKeyboard);
	gkCleanupDispatcher(gkMouse);
	free(gkKeyboard);
	free(gkMouse);

	Platform.Cleanup();
}

#ifdef GK_PLATFORM_TEST

float r = 0;
gkImage* bg;
gkImage* img;
gkFont* font = 0;
gkTextFormat fpsTf;
uint64_t frameTime;
gkSound* snd;
GK_BOOL playing = 0;

static void testDraw(gkPanel* p)
{
	char fps[10];
	uint64_t diff = gkMilliseconds() - frameTime;
	float ts = (float)diff * 0.001f;
	gkMatrix mat = gkMatrixCreateTranslation(-img->width*0.5f, -img->height*0.5f);
	gkMatrixMult(&mat, gkMatrixCreateRotation(r));
	gkMatrixMult(&mat, gkMatrixCreateTranslation(p->mouseX, p->mouseY));
	
	frameTime += diff;

	gkDrawImage(bg,0,0);

	gkSetFillColor(1.0f,0.0f,0.0f,0.5f);
	gkSetLineWidth(1.0f);
	gkSetLineColor(1.0f,1.0f,1.0f,1.0f);
	gkDrawCircle(p->width*0.5f, p->height*0.5f, p->width*0.5f);
	
	gkPushTransform(&mat);
	gkDrawImage(img, 0,0);
	gkPopTransform();
	
	r += ts*5.0f;

#ifdef GK_USE_FONTS
	sprintf(fps, "FPS: %d", gkGetFps());
	gkDrawText(font, fps, 5, 5, &fpsTf);
#endif
}

GK_BOOL onSoundStopped(gkEvent* e, void *p)
{
	playing = GK_FALSE;
	return GK_TRUE;
}

void playSnd()
{
	if (!playing) {
		gkSoundSource* src = gkPlaySound(snd, 0);
		gkAddListener(src, GK_ON_SOUND_STOPPED, 0, onSoundStopped, 0);
		playing = GK_TRUE;
	}
}

GK_BOOL onMouseDown(gkEvent* e, void*p){
	playSnd();
	return GK_TRUE;
}

static GK_BOOL testInit()
{
	gkSetTargetFps(GK_VSYNC);

	if( !(bg = gkLoadImage("assets/testOut.jpg"))) {
		printf("Couldn't load assets/testOut.jpg\n");
	}
	if( !(img = gkLoadImage("assets/test.png"))) {
		printf("Couldn't load assets/test.png\n");
	}
	
#ifdef GK_USE_FONTS
	gkAddFontResource("assets/chiller.TTF");
	font = gkCreateFont("Chiller", 30, GK_FONT_NORMAL);
//	gkAddFontResource("assets/meiryo.ttc");
//	font = gkCreateFont("meiryo", 30, GK_FONT_NORMAL);
	printf(font?"font created\n":"font failed\n");
	fpsTf = gkDefaultTextFormat;
	fpsTf.textColor = GK_COLOR(1,1,1,1);
	fpsTf.strokeColor = GK_COLOR(0,0,0,1);
	fpsTf.strokeSize = 6.0f;
#endif

	snd = gkLoadSound("assets/music.wav", GK_SOUND_STATIC);
	if (snd == 0) {
		printf("Couldn't load sound assets/music.wav");
	}
	playSnd();
	
	gkAddListener(gkMouse, GK_ON_MOUSE_DOWN, 0, onMouseDown, 0);

	frameTime = gkMilliseconds();
	
	gkMainPanel->drawFunc = testDraw;
	return GK_TRUE;
}
static void testCleanup()
{
	gkDestroySound(snd);
#ifdef GK_USE_FONTS
	gkDestroyFont(font);
#endif
	gkDestroyImage(img);
	gkDestroyImage(bg);
}

GK_APP(testInit, testCleanup);

#endif
