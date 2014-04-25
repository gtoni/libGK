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

gkImageData* gkDecodeImageBMP(void* buffer, size_t size);
gkImageData* gkDecodeImageJPEG(void* buffer, size_t size);
gkImageData* gkDecodeImagePNG(void* buffer, size_t size);

gkImageData* gkDecodeImage(char* filename)
{
	FILE* f = fopen(filename, "rb");
	char* buffer;
	size_t fileSize;
	gkImageData* img;

	if (!f) 
		return 0;
	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	buffer = (char*)malloc(fileSize);
	fread(buffer, fileSize, sizeof(char), f);

	img = gkDecodeImageMem(buffer, fileSize);

	free(buffer);
	fclose(f);

	return img;
}

gkImageData* gkDecodeImageMem(void* buffer, size_t size)
{
	gkImageType imgType = gkGetImageTypeMem(buffer, size);

	if(imgType == GK_IMAGE_BMP)
		return gkDecodeImageBMP(buffer, size);
	else if(imgType == GK_IMAGE_JPEG)
		return gkDecodeImageJPEG(buffer, size);
	else if(imgType == GK_IMAGE_PNG)
		return gkDecodeImagePNG(buffer, size);

	return 0;
}

gkImageData* gkCreateImageData(uint32_t width, uint32_t height, gkPixelFormat format)
{
	size_t pixelSize = (format == GK_PIXELFORMAT_RGBA ? 4 : 3);
	size_t size = sizeof(gkImageData) + (width * height * pixelSize);
	gkImageData* dat = (gkImageData*)malloc(size);
	dat->width = width;
	dat->height = height;
	dat->pixelFormat = format;
	dat->data = dat + 1;
	return dat;
}

void gkDestroyImageData(gkImageData* imgData)
{
	free(imgData);
}



static gkImageData* gkDecodeImageBMP(void* buffer, size_t size)
{
	return 0;
}


/*
 * Decode JPEG images (libjpeg)
 */

#include <jpeglib.h>

static gkImageData* gkDecodeImageJPEG(void* inbuffer, size_t insize)
{
	gkImageData* imgData;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPARRAY buffer;
	char *imgBufPtr;
	size_t stride;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, (unsigned char*)inbuffer, insize);
	jpeg_read_header(&cinfo, TRUE);

	imgData = gkCreateImageData(cinfo.image_width, cinfo.image_height, GK_PIXELFORMAT_RGB);
	imgBufPtr = (char*)imgData->data;

	cinfo.out_color_space = JCS_RGB;

	jpeg_start_decompress(&cinfo);
		
	stride = cinfo.out_color_components*cinfo.output_width;
	buffer = (cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, stride, 1);

	while (cinfo.output_scanline<cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(imgBufPtr, buffer[0], stride);
		imgBufPtr += stride;
	}
	jpeg_destroy((j_common_ptr)&cinfo);

	return imgData;
}


/*
 * Decode PNG images (libpng 1.6)
 */

#include <png.h>

struct pngEncodedData{
	void* ptr;
	size_t size;
};

static void readPngEncodedData(png_structp pread, png_bytep dst, png_size_t dstSize)
{
	struct pngEncodedData* data = (struct pngEncodedData*)png_get_io_ptr(pread);
	size_t s = data->size < dstSize ? data->size : dstSize;
	memcpy(dst, data->ptr, s);
	data->ptr = (char*)data->ptr + s;
	data->size -= s;
}

static gkImageData* gkDecodeImagePNG(void* buffer, size_t size)
{
	png_structp pread;
	png_infop pinfo;
	struct pngEncodedData data = {buffer, size};

	png_uint_32 width, height, channels, colorType;
	gkImageData* img = 0;
	char* ptr;
	size_t stride;

	pread = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!pread)
		return 0;

	pinfo = png_create_info_struct(pread);
	if (!pinfo) {
		png_destroy_read_struct(&pread, 0, 0);
		return 0;
	}

	if (setjmp(png_jmpbuf(pread))) {
		if (img)
			gkDestroyImageData(img);
		png_destroy_read_struct(&pread, &pinfo, 0);
		return 0;
	}

	png_set_read_fn(pread, &data, readPngEncodedData);

	png_read_info(pread, pinfo);

	width = png_get_image_width(pread, pinfo);
	height = png_get_image_height(pread, pinfo);
	channels = png_get_channels(pread, pinfo);
	colorType = png_get_color_type(pread, pinfo);

	if (colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(pread);
		channels = 3;
	}else if (colorType == PNG_COLOR_TYPE_GRAY) {
		png_set_gray_to_rgb(pread);
		channels = 3;
	}

	if (png_get_valid(pread, pinfo, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(pread);
		channels += 1;
	}

	if (png_get_bit_depth(pread, pinfo)  == 16)
		png_set_strip_16(pread);

	img = gkCreateImageData(width, height, channels == 3 ? GK_PIXELFORMAT_RGB : GK_PIXELFORMAT_RGBA);

	stride = width * channels;
	ptr = (char*)img->data;

	while (height>0) {
		png_read_row(pread, (png_bytep)ptr, 0);
		height--;
		ptr += stride;
	}

	png_destroy_read_struct(&pread, &pinfo, 0);
	return img;
}