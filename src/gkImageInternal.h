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

#ifndef _GK_IMAGE_INTERNAL_H_
#define _GK_IMAGE_INTERNAL_H_

#include <gkImage.h>
#include <stdlib.h>

typedef enum gkImageType{
	GK_IMAGE_UNKNOWN,
	GK_IMAGE_BMP,
	GK_IMAGE_JPEG,
	GK_IMAGE_PNG
}gkImageType;

gkImageType gkGetImageType(char* filename);
gkImageType gkGetImageTypeMem(void* buffer, size_t size);

typedef struct gkImageData{
	uint32_t width;
	uint32_t height;
	gkPixelFormat pixelFormat;
	void* data;
}gkImageData;

gkImageData* gkDecodeImage(char* filename);
gkImageData* gkDecodeImageMem(void* buffer, size_t size);

GK_BOOL gkEncodeImage(char* filename, gkImageData* img);
void*	gkEncodeImageMem(gkImageType encoding, gkImageData* img, size_t* outSize);

gkImageData* gkCreateImageData(uint32_t width, uint32_t height, gkPixelFormat format);
void gkDestroyImageData(gkImageData* imageData);

#endif