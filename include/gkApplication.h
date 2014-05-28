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

#ifndef _GK_APPLICATION_H_
#define _GK_APPLICATION_H_

#include <gkTypes.h>
#include <gkGeometry.h>

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

#endif