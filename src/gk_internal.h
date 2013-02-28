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

#ifndef _GK_INTERNAL_H_
#define _GK_INTERNAL_H_

#ifdef WIN32
#include <windows.h>
extern HWND gkWindow;
#else
#include <X11/Xlib.h>
extern Display* display;
extern Window gkWindow;
#endif

extern uint64_t gkAppStartTime;
extern gkMouseState gkGlobalMouseState;
extern gkKeyboardState gkGlobalKeyboardState;

void gkProcessUpdatePanel(gkPanel* panel);
void gkProcessDrawPanel(gkPanel* panel);

void gkCheckFocusedPanel();
void gkUpdateMouseTarget(gkPanel* mainPanel);
void gkProcessMouseEvent(gkMouseEvent* mouseEvent);
void gkProcessKeyboardEvent(gkKeyboardEvent* keyboardEvent);


void gkResetTransform();
void gkResetColorFilter();

void gkUpdateTimers();
void gkUpdateTweens();


/* Some variables used in graphics.c */

typedef struct gkColorNodeStruct gkColorNode;
struct gkColorNodeStruct{
	gkColor color;
	gkColorNode* parent;
};
extern gkColorNode* gkColorFilterTop;
gkColor gkGetFilteredColor(gkColor c);

typedef struct gkTransformNodeStruct gkTransformNode;
struct gkTransformNodeStruct{
	gkMatrix transform;
	gkTransformNode *parent;
};
extern gkTransformNode* gkTransformTop;

/* Initializers and cleanups */

void gkInitImages();
void gkCleanupImages();

void gkInitFonts();
void gkCleanupFonts();

void gkInitTimers();
void gkCleanupTimers();

void gkInitTweens();
void gkCleanupTweens();

void gkInitDrawToImageBuffer(gkSize size);
void gkCleanupDrawToImageBuffer();

void gkInitJoystick();
void gkCleanupJoystick();


#endif
