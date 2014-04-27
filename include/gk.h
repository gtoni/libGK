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

#ifndef _GK_H_
#define _GK_H_

#ifdef __cplusplus
extern "C"{
#endif

#ifdef GK_INTERNAL
#define GK_READONLY
#else
#define GK_READONLY /*const*/
#endif

#include <memory.h>

#ifdef _WIN32
	#define GK_WIN
#else
	#define GK_LINUX
#endif

#include <gkTypes.h>
#include <gkGeometry.h>
#include <gkGraphics.h>
#include <gkImage.h>

/**********************************
	Application

	Types and functions used for initializing and run the application, work with underlying windowing system
	and setting screen properties. There are also some useful general purpose functions.
*/

#define GK_VSYNC -1

typedef GK_BOOL (*gkInitFunc)();
typedef void (*gkCleanupFunc)();

void	gkMain(gkInitFunc init, gkCleanupFunc cleanup);
void	gkExit();

char*	gkGetAppDir();
void	gkSetTargetFps(int targetFps);
int	gkGetTargetFps();
int	gkGetFps();

void	gkSetScreenSize(gkSize size);
gkSize	gkGetScreenSize();
void	gkSetFullscreen(GK_BOOL fullscreen);
GK_BOOL	gkIsFullscreen();
size_t	gkGetSupportedScreenSizes(gkSize* sizes);

void	gkSetWindowTitle(char* title);
char*	gkGetWindowTitle();

void	gkSetWindowResizable(GK_BOOL resizable);
GK_BOOL	gkIsWindowResizable();


/************************************
	Event model

	Types and functions used for dispatching and handling events.
	Events can be dispatched for various reasons, such as mouse clicks or key pressed. Objects that dispatch events
	are usually called event dispatchers. Listener functions are called when the event they listen for is dispatched,
	also they are given a data structure which describes the event (such as which mouse button was pressed).
*/

#define GK_EVENT(T)	typedef struct T T;\
			struct T{\
				uint16_t type;\
				void* target;\
				void* currentTarget;
#define GK_EVENT_END()	};

GK_EVENT(gkEvent)
GK_EVENT_END()

typedef struct gkDispatcher
{
	struct gkEventListener* listeners;
} gkDispatcher;

typedef void* gkDispatcherHandle;
typedef void* gkEventHandle;

typedef GK_BOOL (*gkEventListenerFunc)(gkEvent* eventData, void* param);

void	gkInitDispatcher(gkDispatcher* listeners);
void	gkCleanupDispatcher(gkDispatcher* listeners);
void	gkAddListener(gkDispatcherHandle listeners, int type, short priority, gkEventListenerFunc listener, void* param);
void	gkRemoveListener(gkDispatcherHandle listeners, int type, gkEventListenerFunc listener, void* param);
GK_BOOL	gkHasListeners(gkDispatcherHandle listeners, int type);
GK_BOOL	gkDispatch(gkDispatcherHandle listeners, gkEventHandle eventData);


/************************************
	Input

	Types and functions for handling user input
*/

/* Mouse device */
#define GK_MOUSE_EVENT_BASE		0
#define GK_ON_MOUSE_DOWN		GK_MOUSE_EVENT_BASE + 1
#define GK_ON_MOUSE_MOVE		GK_MOUSE_EVENT_BASE + 2
#define	GK_ON_MOUSE_UP			GK_MOUSE_EVENT_BASE + 3
#define GK_ON_MOUSE_WHEEL		GK_MOUSE_EVENT_BASE + 4
#define GK_ON_MOUSE_ENTER		GK_MOUSE_EVENT_BASE + 5
#define GK_ON_MOUSE_LEAVE		GK_MOUSE_EVENT_BASE + 6

#define GK_MAX_MOUSE_BUTTONS 3

typedef struct gkMouseState
{
	gkPoint position;
	GK_BOOL buttons[GK_MAX_MOUSE_BUTTONS];
	int32_t wheel;
} gkMouseState;

GK_EVENT(gkMouseEvent)
	uint8_t button;
	int8_t delta;
	gkPoint position;
GK_EVENT_END()

extern gkDispatcher *gkMouse;

void gkGetMouseState(gkMouseState* mouseState);
void gkSetMousePosition(float x, float y);

/* Keyboard device */
#define GK_KEYBOARD_EVENT_BASE		10
#define GK_ON_KEY_DOWN		GK_KEYBOARD_EVENT_BASE + 1
#define GK_ON_KEY_REPEAT	GK_KEYBOARD_EVENT_BASE + 2
#define GK_ON_KEY_UP		GK_KEYBOARD_EVENT_BASE + 3
#define GK_ON_CHARACTER		GK_KEYBOARD_EVENT_BASE + 4

#include <gkkeydefs.h>

#define GK_MAX_KEYBOARD_KEYS 256

typedef struct gkKey
{
	uint16_t code;
	uint16_t modifiers;
}gkKey;

typedef struct gkKeyboardState
{
	GK_BOOL keys[GK_MAX_KEYBOARD_KEYS];
}gkKeyboardState;

GK_EVENT(gkKeyboardEvent)
	gkKey key;
	uint32_t charCode;
GK_EVENT_END()

extern gkDispatcher* gkKeyboard;

void gkGetKeyboardState(gkKeyboardState* keyboardState);


/* Joystick devices */

#define GK_JOYSTICK_XBOX360		1

typedef struct gkJoystick
{
	char* name;
	uint8_t flags;
}gkJoystick;

extern gkJoystick** gkJoysticks;

uint32_t gkEnumJoysticks();

#define GK_JOYSTICK_POV_N		0
#define GK_JOYSTICK_POV_E	 9000
#define GK_JOYSTICK_POV_S	18000
#define GK_JOYSTICK_POV_W	27000
#define GK_JOYSTICK_POV_NE	((GK_JOYSTICK_POV_N + GK_JOYSTICK_POV_E)>>1)
#define GK_JOYSTICK_POV_SE	((GK_JOYSTICK_POV_S + GK_JOYSTICK_POV_E)>>1)
#define GK_JOYSTICK_POV_NW	((GK_JOYSTICK_POV_N + GK_JOYSTICK_POV_W)>>1)
#define GK_JOYSTICK_POV_SW	((GK_JOYSTICK_POV_S + GK_JOYSTICK_POV_W)>>1)
#define GK_JOYSTICK_POV_UP		GK_JOYSTICK_POV_N
#define GK_JOYSTICK_POV_DOWN	GK_JOYSTICK_POV_S
#define GK_JOYSTICK_POV_LEFT	GK_JOYSTICK_POV_W
#define GK_JOYSTICK_POV_RIGHT	GK_JOYSTICK_POV_E

typedef struct gkJoystickState
{
	struct
	{
		float x,y,z;
	}left;
	struct
	{
		float x,y,z;
	}right;
	GK_BOOL	 buttons[32];
	uint32_t pov[4];
	float	 sliders[2];
}gkJoystickState;

typedef struct gkXBox360ControllerState
{
	struct
	{
		float x,y;
	}leftStick;
	float triggerZ;
	struct
	{
		float x,y,unused;
	}rightStick;
	GK_BOOL buttonA;
	GK_BOOL buttonB;
	GK_BOOL buttonX;
	GK_BOOL buttonY;
	GK_BOOL	buttonLB;
	GK_BOOL	buttonRB;
	GK_BOOL	buttonBACK;
	GK_BOOL	buttonSTART;
	GK_BOOL unusedButtons[24];
	uint32_t pov;
	uint32_t unusedPov[3];
	float unusedSliders[2];
}gkXBox360ControllerState;

void gkGetJoystickState(gkJoystick* joystick, gkJoystickState* state);

/************************************
	Panels

	The panel structure and functions for working with panels.
*/

#define GK_PANEL_EVENT_BASE		100
#define GK_ON_PANEL_ADDED		(GK_PANEL_EVENT_BASE + 1)
#define GK_ON_PANEL_REMOVED		(GK_PANEL_EVENT_BASE + 2)
#define GK_ON_PANEL_RESIZED		(GK_PANEL_EVENT_BASE + 3)
#define GK_ON_PANEL_FOCUS_IN	(GK_PANEL_EVENT_BASE + 4)
#define GK_ON_PANEL_FOCUS_OUT	(GK_PANEL_EVENT_BASE + 5)

typedef struct gkPanel gkPanel;

typedef void (*gkPanelLayoutFunc)(gkPanel* panel, gkRect* clientRect);
typedef void (*gkPanelUpdateFunc)(gkPanel* panel);
typedef void (*gkPanelDrawFunc)(gkPanel* panel);

struct gkPanel
{
	gkDispatcher dispatcher;
	float x,y,width,height;
	gkMatrix transform;
	float anchorX, anchorY;
	gkColor colorFilter;
	GK_BOOL mouseEnabled, mouseChildren, keyboardEnabled, keyboardChildren;
	GK_BOOL visible;
	void* data;
	gkPanelLayoutFunc layoutFunc;
	gkPanelUpdateFunc updateFunc;
	gkPanelDrawFunc drawFunc;

GK_READONLY GK_BOOL mouseOver;
GK_READONLY float mouseX, mouseY;
GK_READONLY gkPanel* parent;
GK_READONLY int16_t numChildren;

/* INTERNALS */
    struct
	{
        gkPanel *first, *last;
    }mChildren;
    gkPanel* mNext;
    gkPanel* mNextChild;
    GK_BOOL mGuardDestroy;
    GK_BOOL mMustDestroy;
    GK_BOOL mViewport;
};

extern gkPanel* gkMainPanel;

gkPanel* gkCreatePanel();
gkPanel* gkCreatePanelEx(size_t panelSize);

gkPanel* gkCreateViewportPanel();
gkPanel* gkCreateViewportPanelEx(size_t panelSize);

void gkDestroyPanel(gkPanel* panel);

void gkAddChild(gkPanel* parent, gkPanel* child);
void gkAddChildAt(gkPanel* parent, gkPanel* child, int index);
void gkRemoveChild(gkPanel* child);
void gkRemoveChildAt(gkPanel* parent, int childIndex);
int  gkGetChildIndex(gkPanel* child);
gkPanel* gkGetChildAt(gkPanel* parent, int childIndex);

gkMatrix gkLocalToGlobal(gkPanel* panel);
gkMatrix gkGlobalToLocal(gkPanel* panel);

void gkSetFocus(gkPanel* panel);
gkPanel* gkGetFocus();

/************************************
	Fonts

	Types and functions for fonts.
*/

#define	GK_TEXT_ALIGN_LEFT		0
#define	GK_TEXT_ALIGN_CENTER	1
#define	GK_TEXT_ALIGN_RIGHT		2
#define	GK_TEXT_ALIGN_JUSTIFY	4
#define	GK_TEXT_VALIGN_TOP		0
#define	GK_TEXT_VALIGN_MIDDLE	1
#define	GK_TEXT_VALIGN_BOTTOM	2
#define	GK_FONT_NORMAL		0
#define	GK_FONT_ITALIC		1
#define	GK_FONT_BOLD		2
#define	GK_FONT_BOLD_ITALIC	3

typedef struct gkFontFace
{
	GK_READONLY char* fontFamily;
	GK_READONLY uint8_t style;
}gkFontFace;

typedef struct gkFontResource
{
	GK_READONLY uint8_t numFaces;
	GK_READONLY gkFontFace **GK_READONLY faces;
}gkFontResource;

typedef struct gkFont
{
	GK_READONLY gkFontFace *GK_READONLY face;
	GK_READONLY uint16_t size;
}gkFont;

typedef struct gkTextFormat
{
	uint8_t align;
	uint8_t valign;
	GK_BOOL wordWrap;
	uint8_t tabSpace;
	GK_BOOL underline;
	float width;
	float height;
	float strokeSize;
	float lineSpacing;
	gkColor textColor;
	gkColor strokeColor;
	GK_BOOL vertical;
}gkTextFormat;

extern gkTextFormat gkDefaultTextFormat;

gkFontResource* gkAddFontResource(char* filename);
void gkRemoveFontResource(gkFontResource* rc);

gkFont* gkCreateFont(char* family, uint16_t size, uint8_t style);
void gkDestroyFont(gkFont* font);

gkSize gkMeasureText(gkFont* font, char* text, gkTextFormat* format);
void gkDrawText(gkFont* font, char* text, float x, float y, gkTextFormat* format);


/************************************
	Audio

	Types and functions for audio and sounds.
*/

#ifdef GK_INTERNAL
#include "gkaudio.h"
#endif

typedef struct gkSound
{
    float length;
    GK_BOOL seekable;
#ifdef GK_INTERNAL
    struct gkSoundInternal internal;
#endif
}gkSound;


#define GK_SOUND_STATE_IDLE         0
#define GK_SOUND_STATE_PLAYING      1
#define GK_SOUND_STATE_PAUSED       2

typedef struct gkSoundSource
{
    gkDispatcher dispatcher;

GK_READONLY int id;
GK_READONLY uint8_t state;
GK_READONLY gkSound* sound;
#ifdef GK_INTERNAL
    struct gkSoundSourceInternal internal;
#endif
}gkSoundSource;


#define GK_SOUND_EVENT_BASE    150
#define GK_ON_SOUND_STOPPED     (GK_SOUND_EVENT_BASE + 1)

#define GK_SOUND_STATIC     0x01
#define GK_SOUND_STREAM     0x02

gkSound* gkLoadSound(char* filename, int flags);
void gkDestroySound(gkSound* sound);

gkSoundSource* gkCreateSoundSource();
void gkDestroySoundSource(gkSoundSource* source);

gkSoundSource* gkPlaySound(gkSound* sound, gkSoundSource* source);
void gkPauseSound(gkSoundSource* soundSource);
void gkResumeSound(gkSoundSource* soundSource);
void gkStopSound(gkSoundSource* soundSource, GK_BOOL dispatchStopEvent);

void gkSetSoundGain(gkSoundSource* soundSource, float gain);
float gkGetSoundGain(gkSoundSource* soundSource);

void gkSetSoundPitch(gkSoundSource* soundSource, float pitch);
float gkGetSoundPitch(gkSoundSource* soundSource);

void gkSetSoundLooping(gkSoundSource* soundSource, GK_BOOL looping);
GK_BOOL gkIsSoundLooping(gkSoundSource* soundSource);

void gkSetSoundOffset(gkSoundSource* soundSource, float seconds);
float gkGetSoundOffset(gkSoundSource* soundSource);

void gkSetMasterGain(float gain);
float gkGetMasterGain();

/************************************
	Timers

	Types and functions for timers.
*/

#define GK_TIMER_EVENT_BASE		200
#define GK_ON_TIMER				(GK_TIMER_EVENT_BASE + 1)
#define GK_ON_TIMER_COMPLETE	(GK_TIMER_EVENT_BASE + 2)

uint64_t gkMilliseconds();

typedef struct gkTimer
{
	gkDispatcher dispatcher;
	uint64_t delay;
	uint32_t repeats;
	uint64_t interval;
GK_READONLY GK_BOOL running;
GK_READONLY GK_BOOL autoDestroy;
}gkTimer;

gkTimer* gkCreateTimer();
void gkStartTimer(gkTimer* timer, GK_BOOL autoDestroy);
void gkStopTimer(gkTimer* timer);
void gkDestroyTimer(gkTimer* timer);


/************************************
	Tweens

	Types and functions for tweens.
*/

#define GK_TWEEN_EVENT_BASE		210
#define GK_ON_TWEEN_UPDATE		(GK_TWEEN_EVENT_BASE + 1)
#define GK_ON_TWEEN_FINISHED	(GK_TWEEN_EVENT_BASE + 2)

GK_EVENT(gkTweenEvent)
	void* var;
GK_EVENT_END()

#define GK_TWEEN_LINEAR				0
#define GK_TWEEN_EASE_IN_SINE		1
#define GK_TWEEN_EASE_OUT_SINE		2
#define GK_TWEEN_EASE_INOUT_SINE	3
#define GK_TWEEN_EASE_OUTIN_SINE	4
#define GK_TWEEN_EASE_IN_ELASTIC	5
#define GK_TWEEN_EASE_OUT_ELASTIC	6
#define GK_TWEEN_EASE_IN_BOUNCE		7
#define GK_TWEEN_EASE_OUT_BOUNCE	8

typedef gkDispatcher gkTween;

#ifdef GK_LINUX
#define CDECL
#endif

gkTween* CDECL gkAddTween(void* var, uint32_t transitionType, uint64_t transitionTime, uint32_t varType, /*startValue, endValue*/...);

void gkRemoveTweens(void* memOffset, size_t size);	/* Removes all tweens for variables in the memory range between memOffset and (memOffset + size) */

#ifdef __cplusplus
}
#endif

#endif
