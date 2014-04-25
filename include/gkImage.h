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

#ifndef _GK_IMAGES_H_
#define _GK_IMAGES_H_

#include <gkTypes.h>
#include <gkGeometry.h>

/************************************
	Images

	The image structure and functions for working with images.
*/

typedef enum gkPixelFormat{
	GK_PIXELFORMAT_RGBA,
	GK_PIXELFORMAT_RGB
}gkPixelFormat;

typedef struct gkImage
{
	uint32_t id;
	uint16_t width;
	uint16_t height;
}gkImage;

gkImage* gkLoadImage(char* filaname);
gkImage* gkCreateImage(int width, int height);
GK_BOOL gkSaveImage(gkImage* image, char* filename);

void gkSetImageData(gkImage* image, gkPixelFormat format, void* data);
void gkGetImageData(gkImage* image, gkPixelFormat format, void* data);

GK_BOOL gkBeginDrawToImage(gkImage* image, GK_BOOL clear);
void gkEndDrawToImage();

void gkDrawImage(gkImage* image, float x, float y);
void gkDrawImageEx(gkImage* image, float x, float y, gkRect srcRect);

void gkDestroyImage(gkImage* image);

#endif