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

#include "gkImageInternal.h"
#include <stdio.h>
#include <memory.h>

gkImageType gkGetImageType(char* filename)
{
	gkImageType res = GK_IMAGE_UNKNOWN;
	char hdr[10];
	FILE* f = fopen(filename, "rb");
	if (f) {
		size_t hdrSize = fread(hdr, sizeof(char), 10, f);
		res = gkGetImageTypeMem(hdr, hdrSize);
		fclose(f);
	}
	return res;
}

gkImageType gkGetImageTypeMem(void* buffer, size_t size)
{
	unsigned char* hdr = (unsigned char*)buffer;
	if(size<2)
		return GK_IMAGE_UNKNOWN;

	if (hdr[0] == 0x42 && hdr[1] == 0x4d) 
		return GK_IMAGE_BMP;


	if(size>10) 
		size = 10;

	if (hdr[0] == 0xff && hdr[1] == 0xd8) {
		if (size>6) {
			if (memcmp(hdr + 6, "JFIF", size - 6) == 0 
				|| memcmp(hdr + 6, "Exif", size - 6) == 0)
				return GK_IMAGE_JPEG;
		} else {
			return GK_IMAGE_JPEG;
		}
	}

	if(size>8) size = 8;

	if(memcmp(hdr, "\211PNG\r\n\032\n", size) == 0)
		return GK_IMAGE_PNG;

	return GK_IMAGE_UNKNOWN;
}