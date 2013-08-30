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


#include "gk.h"
#include "gk_internal.h"

#ifdef GK_WIN
#define _UNICODE
#endif

#include "IL/il.h"
#include "GLee.h"

#include <stdlib.h>
#include <GL/gl.h>

#include <locale.h>

gkColor gkGetFilteredColor(gkColor c);	//implemented in graphics.c

void gkInitImages(){
	ilInit();
}

void gkCleanupImages(){
}

gkImage* gkLoadImage(wchar_t* filename){
	gkImage* image = (gkImage*)malloc(sizeof(gkImage));
	ILuint imageId = ilGenImage();
	GLint oldTexId;

#if defined(_UNICODE)
	ilBindImage(imageId);
	if(ilLoadImage(filename)){
#else
    char filenameMbs[1024];
    setlocale(LC_CTYPE, "");
    wcstombs(filenameMbs, filename, sizeof(filenameMbs));

    ilBindImage(imageId);
    if(ilLoadImage(filenameMbs)){
#endif

		glGenTextures(1, &image->id);
		ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
		image->width = ilGetInteger(IL_IMAGE_WIDTH);
		image->height = ilGetInteger(IL_IMAGE_HEIGHT);

		glEnable(GL_TEXTURE_2D);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexId);
		glBindTexture(GL_TEXTURE_2D, image->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glBindTexture(GL_TEXTURE_2D, oldTexId);
		glDisable(GL_TEXTURE_2D);
		ilDeleteImage(imageId);
	}else{
		ilDeleteImage(imageId);
		free(image);
		return 0;
	}
	return image;
}

GK_BOOL gkSaveImage(gkImage* image, wchar_t* filename){
	int img = ilGenImage();
	GK_BOOL result;
	uint8_t* buff = (uint8_t*)calloc(image->width*image->height*4, sizeof(uint8_t));

#if defined(_UNICODE)
    wchar_t* saveName = filename;
#else
    char saveName[1024];
    setlocale(LC_CTYPE, "");
    wcstombs(saveName, filename, sizeof(saveName));
#endif // _UNICODE

	gkGetImageData(image, GK_RGBA, buff);
	ilBindImage(img);
	ilEnable(IL_FILE_OVERWRITE);
	ilTexImage(image->width, image->height, 0, 4, IL_RGBA, IL_UNSIGNED_BYTE, buff);
	result = ilSaveImage(saveName) == IL_TRUE;
	ilDeleteImage(img);
	free(buff);
	return result;
}

gkImage* gkCreateImage(int width, int height){
	gkImage* image = (gkImage*)malloc(sizeof(gkImage));
	GLint oldTexId;

	glGenTextures(1, &image->id);
	image->width = width;
	image->height = height;

	glEnable(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexId);
	glBindTexture(GL_TEXTURE_2D, image->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, oldTexId);
	glDisable(GL_TEXTURE_2D);

	return image;
}

void gkSetImageData(gkImage* image, int format, void* data){
	GLint oldId;
	GLint oldUnPackAlignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnPackAlignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldId);
	glBindTexture(GL_TEXTURE_2D, image->id);
	if(format == GK_RGBA){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}else if(format == GK_RGB){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}else if(format == GK_BGRA){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	}else if(format == GK_BGR){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,0, GL_BGR, GL_UNSIGNED_BYTE, data);
	}
	glBindTexture(GL_TEXTURE_2D, oldId);
	glDisable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnPackAlignment);
}

void gkGetImageData(gkImage* image, int format, void* data){
	GLint oldId;
	GLint oldPackAlignment;
	glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldId);
	glBindTexture(GL_TEXTURE_2D, image->id);
	if(format == GK_RGBA){
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}else if(format == GK_RGB){
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}else if(format == GK_BGRA){
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	}else if(format == GK_BGR){
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	}
	glBindTexture(GL_TEXTURE_2D, oldId);
	glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
}

void gkDrawImageInternal(gkImage* img, float* v, float* c){
	gkColor color = gkGetFilteredColor(GK_COLOR(1,1,1,1));
	glColor4f(color.r, color.g, color.b, color.a);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, img->id);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, v);
	glTexCoordPointer(2, GL_FLOAT, 0, c);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void gkDrawImage(gkImage* image, float x, float y){
	float v[]= {
		0,0,
		image->width, 0,
		image->width, image->height,
		0, image->height
	};
	float c[]= {
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	glPushMatrix();
	glTranslatef(x,y,0);
	gkDrawImageInternal(image, v, c);
	glPopMatrix();
}

void gkDrawImageEx(gkImage* image, float x, float y, gkRect srcRect){
	float v[]= {
		0,0,
		srcRect.width, 0,
		srcRect.width, srcRect.height,
		0, srcRect.height
	};
	float left = (srcRect.x/image->width);
	float top = (srcRect.y/image->height);
	float right = left + (srcRect.width/image->width);
	float bottom = top + (srcRect.height/image->height);

	float c[]= {
		left, top,
		right, top,
		right, bottom,
		left, bottom
	};

	glPushMatrix();
	glTranslatef(x,y,0);
	gkDrawImageInternal(image, v, c);
	glPopMatrix();
}


void gkDestroyImage(gkImage* image){
	glDeleteTextures(1, &image->id);
	free(image);
}

////////////////////////////////
/// Draw To image related STUFF

gkImage* gkImageToDraw = 0;
gkColorNode* gkOldColorFilterTop;
gkTransformNode* gkOldTransformTop;

/* TexCopy stuff */
gkImage* screenBufferImg = 0;
gkImage* screenDepthBufferImg = 0;

/* FBO stuff */
GLuint imgFramebufferId = 0;
GLuint imgRenderbuffers[2];

GK_BOOL gkUseFBO		= GK_FALSE;
GK_BOOL gkUseAuxBuffer	= GK_FALSE;
GK_BOOL gkUseTexCopy	= GK_FALSE;

void gkInitDrawToImageBuffer(gkSize size){
	if(GLEE_EXT_framebuffer_object){	// Usage of FrameBuffer objects is disabled due to lack of multisampling at the moment.
		GLuint status;
		gkUseFBO = GK_TRUE;
		glGenFramebuffersEXT(1, &imgFramebufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, imgFramebufferId);

		glGenRenderbuffersEXT(2, imgRenderbuffers);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, imgRenderbuffers[0]);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, (int)size.width, (int)size.height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, imgRenderbuffers[0]);

		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, imgRenderbuffers[1]);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, (int)size.width, (int)size.height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, imgRenderbuffers[1]);

		status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if(status != GL_FRAMEBUFFER_COMPLETE_EXT){
			switch(status){
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
					printf("GK [ERROR]: InitDrawToImageBuffer GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
					printf("GK [ERROR]: InitDrawToImageBuffer GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
					printf("GK [ERROR]: InitDrawToImageBuffer GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
					printf("GK [ERROR]: InitDrawToImageBuffer GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT\n");
					break;
				case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
					printf("GK [ERROR]: InitDrawToImageBuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT\n");
					break;
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glDeleteRenderbuffersEXT(2, imgRenderbuffers);
			glDeleteFramebuffersEXT(1, &imgFramebufferId);
			gkUseFBO = GK_FALSE;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	if(!gkUseFBO){
		GLint auxBuffers = 0;
		glGetIntegerv(GL_AUX_BUFFERS, &auxBuffers);
		if(auxBuffers>0){
			gkUseAuxBuffer = GK_TRUE;
		}else{
			//finally if no other way do it with texture copying
			gkUseTexCopy = GK_TRUE;

			screenBufferImg = gkCreateImage((int)size.width, (int)size.height);

			screenDepthBufferImg = (gkImage*)malloc(sizeof(gkImage));
			glGenTextures(1, &screenDepthBufferImg->id);
			glBindTexture(GL_TEXTURE_2D, screenDepthBufferImg->id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (int)size.width, (int)size.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			screenDepthBufferImg->width = (uint16_t)size.width;
			screenDepthBufferImg->height = (uint16_t)size.height;
		}
	}

	if(gkUseFBO) printf("GK [INFO]: Using FBO\n");
	else if(gkUseAuxBuffer) printf("GK [INFO]: Using aux buffer\n");
	else if(gkUseTexCopy) printf("GK [INFO]: Using TexCopy\n");
}

void gkCleanupDrawToImageBuffer(){
	if(gkUseFBO && imgFramebufferId){
		glDeleteRenderbuffersEXT(2, imgRenderbuffers);
		glDeleteFramebuffersEXT(1, &imgFramebufferId);
	}
	if(gkUseTexCopy && screenBufferImg){
		gkDestroyImage(screenBufferImg);
		gkDestroyImage(screenDepthBufferImg);
	}
}

GK_BOOL gkBeginDrawToImage(gkImage* img, GK_BOOL clear){
	GLint viewportDimmension[4];

	if(gkImageToDraw != 0) return GK_FALSE;
	gkImageToDraw = img;

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glGetIntegerv(GL_VIEWPORT, viewportDimmension);

	if(gkUseFBO){
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, imgFramebufferId);
	}else if(gkUseAuxBuffer){
		glReadBuffer(GL_AUX0);
		glDrawBuffer(GL_AUX0);
	}else if(gkUseTexCopy){
		glBindTexture(GL_TEXTURE_2D, screenBufferImg->id);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0, viewportDimmension[2], viewportDimmension[3]);

		glBindTexture(GL_TEXTURE_2D, screenDepthBufferImg->id);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, viewportDimmension[2], viewportDimmension[3], 0);
	}

	glViewport(0,0, (GLsizei)img->width, (GLsizei)img->height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,(int)img->width,0,(int)img->height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0,0,0,0);
	if(GLEE_VERSION_1_4){
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	}else{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glEnable(GL_BLEND);
	glEnable(GL_LINE_STIPPLE);


	gkOldColorFilterTop = gkColorFilterTop;
	gkColorFilterTop = (gkColorNode*)malloc(sizeof(gkColorNode));
	gkColorFilterTop->parent = 0;
	gkColorFilterTop->color = GK_COLOR(1,1,1,1);

	gkOldTransformTop = gkTransformTop;
	gkTransformTop = (gkTransformNode*)malloc(sizeof(gkTransformNode));
	gkTransformTop->parent = 0;
	gkTransformTop->transform = gkMatrixCreateIdentity();

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(!clear)	gkDrawImage(gkImageToDraw, 0.0f, 0.0f);
	return GK_TRUE;
}

void gkEndDrawToImage(){
	gkColorNode *colorNode;
	gkTransformNode *transformNode;

	if(gkImageToDraw == 0){
		printf("GK [ERROR]: No target image to draw to\n");
		return;
	}

	glDisable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, gkImageToDraw->id);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (int)gkImageToDraw->width, (int)gkImageToDraw->height);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	if(gkUseFBO){
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}else if(gkUseAuxBuffer){
		glReadBuffer(GL_BACK);
		glDrawBuffer(GL_BACK);
	}else if(gkUseTexCopy){
		GLboolean blendEnabled = glIsEnabled(GL_BLEND);
		GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
		GLint viewportDimmension[4];

		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glGetIntegerv(GL_VIEWPORT, viewportDimmension);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,viewportDimmension[2], 0, viewportDimmension[3], -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		gkColorFilterTop->color = GK_COLOR(1,1,1,1);

		gkDrawImage(screenDepthBufferImg, 0.0f, 0.0f);
		gkDrawImage(screenBufferImg, 0.0f, 0.0f);

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		if(depthEnabled == GL_TRUE) glEnable(GL_DEPTH_TEST);
		if(blendEnabled == GL_TRUE) glEnable(GL_BLEND);
	}

	while(gkColorFilterTop->parent){
		gkColorFilterTop = (colorNode = gkColorFilterTop)->parent;
		free(colorNode);
	}
	free(gkColorFilterTop);
	gkColorFilterTop = gkOldColorFilterTop;

	while(gkTransformTop->parent){
		gkTransformTop = (transformNode = gkTransformTop)->parent;
		free(transformNode);
	}
	free(gkTransformTop);
	gkTransformTop = gkOldTransformTop;

	glMatrixMode(GL_MODELVIEW);

	gkImageToDraw = 0;
}
