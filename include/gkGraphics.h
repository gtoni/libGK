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

#ifndef _GK_GRAPHICS_H_
#define _GK_GRAPHICS_H_

#include <gkTypes.h>
#include <gkGeometry.h>

/************************************
	Graphics

	Types and functions for drawing graphics
*/

typedef struct gkColor
{
	float r,g,b,a;
}gkColor;

gkColor GK_COLOR(float r, float g, float b, float a);

void gkPushColorFilter(float r, float g, float b, float a);
void gkPopColorFilter();

void gkPushTransform(gkMatrix* matrix);
void gkPopTransform();

void gkSetFillColor(float r, float g, float b, float a);
void gkSetLineColor(float r, float g, float b, float a);
void gkSetLineWidth(float width);

void gkSetClipRect(float x, float y, float w, float h);

void gkDrawPoint(float x, float y, float size);
void gkDrawLine(float x1, float y1, float x2, float y2);
void gkDrawRect(float x, float y, float width, float height);
void gkDrawRoundRect(float x, float y, float width, float height, float a, float b);
void gkDrawCircle(float x, float y, float radius);
void gkDrawPath(gkPoint* points, int count, GK_BOOL polygon);

#endif