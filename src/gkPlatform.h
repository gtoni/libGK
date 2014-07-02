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

#ifndef _GK_PLATFORM_H_
#define _GK_PLATFORM_H_

#include <gkConfig.h>
#include <gkTypes.h>
#include <gkGeometry.h>

#define GK_MAX_APPDIR_SIZE	1024
#define GK_MAX_TITLE_SIZE	256

extern gkSize gkScreenSize;

typedef void (*onInitCallback)();
typedef void (*onRunCallback)();

typedef struct gkPlatform
{
	GK_METHOD(GK_BOOL, Init, (onInitCallback onInit));
	GK_METHOD(void, Run, (onRunCallback onLoop, onRunCallback onExit));
	GK_METHOD(void, Cleanup,());
	GK_METHOD(void, Exit,());

	GK_METHOD(void, GetAppDir,(char* dst, size_t dstSize));
	GK_METHOD(void, SetVSync,(GK_BOOL));

	GK_METHOD(void, ResizeScreen, (gkSize size));
	GK_METHOD(int, GetSupportedSizes, (gkSize* sizes));
	GK_METHOD(GK_BOOL, SetFullscreen, (GK_BOOL enable));

	GK_METHOD(void, SetWindowTitle, (char* title));
	GK_METHOD(void, GetWindowTitle, (char* dst, size_t dstSize));

	GK_METHOD(void, SetWindowResizable, (GK_BOOL resizable));

	GK_METHOD(void, Sleep, (uint32_t ms));

	GK_METHOD(uint16_t, PrepareKey, (uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown));

	GK_METHOD(void, ProcessEvents,());

	GK_METHOD(void, SwapBuffers, ());
}gkPlatform;

#ifdef __cplusplus
extern "C"{
#endif

gkPlatform gkGetPlatform();

/* Called from platform specific code */
void onWindowClose();
void onWindowMouseMove(int x, int y);
void onWindowMouseWheel(int x, int y, int w);
void onWindowMouseDown(int x, int y, int mb);
void onWindowMouseUp(int x, int y, int mb);
void onWindowSizeChanged(gkSize nsize);
GK_BOOL onWindowKeyDown(uint16_t keyCode, uint16_t scanCode);
GK_BOOL onWindowKeyUp(uint16_t keyCode, uint16_t scanCode);
void onWindowCharacter(uint32_t character);

#ifdef __cplusplus
}
#endif


#endif