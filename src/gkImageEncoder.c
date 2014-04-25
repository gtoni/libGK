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

void* gkEncodeImageJPEG(gkImageData* img, size_t* bufferSize);


static gkImageType getImageTypeByFilename(char *filename)
{
	char* ext = filename + (strlen(filename) - 3);
	if (stricmp(ext, "bmp") == 0)
		return GK_IMAGE_BMP;
	else if (stricmp(ext, "jpg") == 0 || stricmp(ext, "jpeg") == 0)
		return GK_IMAGE_JPEG;
	else if (stricmp(ext, "png") == 0)
		return GK_IMAGE_PNG;
	else 
		return GK_IMAGE_UNKNOWN;
}

GK_BOOL gkEncodeImage(char* filename, gkImageData* img)
{
	FILE* outFile;
	void* buffer;
	size_t bufferSize;
	gkImageType type = getImageTypeByFilename(filename);

	buffer = gkEncodeImageMem(type, img, &bufferSize);

	if (!buffer) 
		return GK_FALSE;

	outFile = fopen(filename, "wb");
	if (!outFile) {
		free(buffer);
		return GK_FALSE;
	}

	fwrite(buffer, sizeof(char), bufferSize, outFile);
	fclose(outFile);

	free(buffer);
	return GK_TRUE;
}

void* gkEncodeImageMem(gkImageType type, gkImageData* img, size_t* bufferSize)
{
	if(type == GK_IMAGE_JPEG)
		return gkEncodeImageJPEG(img, bufferSize);

	return 0;
}


/* JPEG encoder (libjpeg) */

#include <jpeglib.h>
#include <setjmp.h>

struct gkJpegError{
	struct jpeg_error_mgr base;
	jmp_buf error_jmpbuff;
};

METHODDEF(void) jpegErrorExit(j_common_ptr cinfo)
{
	struct gkJpegError* err = (struct gkJpegError*)cinfo->err;

	(*cinfo->err->output_message)(cinfo);

	longjmp(err->error_jmpbuff, 1);
}

static void* gkEncodeImageJPEG(gkImageData* img, size_t* bufferSize)
{
	struct jpeg_compress_struct cinfo;
	struct gkJpegError err;
	int srcChannels = (img->pixelFormat == GK_PIXELFORMAT_RGB ? 3 : 4);
	unsigned char* src = (unsigned char*)img->data;
	size_t stride = img->width * srcChannels;
	void* buffer = 0;

	cinfo.err = jpeg_std_error(&err.base);
	err.base.error_exit = jpegErrorExit;
	if (setjmp(err.error_jmpbuff)) {
		if (buffer) {
			free(buffer);
		}
		jpeg_destroy_compress(&cinfo);
		return 0;
	}

	jpeg_create_compress(&cinfo);

	jpeg_mem_dest(&cinfo, (unsigned char**)&buffer, bufferSize);

	cinfo.image_width = img->width;
	cinfo.image_height = img->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);

	if(srcChannels == 3) {
		while (cinfo.next_scanline < cinfo.image_height) {
			jpeg_write_scanlines(&cinfo, &src, 1);
			src += stride;
		}
	} else {
		unsigned char* tmp = (unsigned char*)malloc(img->width*3);
		uint32_t i, d, s, a;
		unsigned char bg;
		while (cinfo.next_scanline < cinfo.image_height) {
			/* Convert RGBA row to RGB row */
			for (i = 0; i<img->width; i++) {
				s = i*4;
				d = i*3;
				a = src[s + 3];
				bg = (255 - a);
				tmp[d] = ((a * src[s]) >> 8) + bg;
				tmp[d + 1] = ((a * src[s + 1]) >> 8) + bg;
				tmp[d + 2] = ((a * src[s + 2]) >> 8) + bg;
			}
			jpeg_write_scanlines(&cinfo, &tmp, 1);
			src += stride;
		}
		free(tmp);
	}

	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);

	return buffer;
}