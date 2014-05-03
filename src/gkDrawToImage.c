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

#include <gk.h>

#include <gkImage.h>
#include <gkGraphics.h>
#include <stdlib.h>
#include <stdio.h>

#include "gk_internal.h"
#include "GLee.h"

/* 
	The idea behind this code is to draw anything to
	a 'buffer' and then copy the buffer content to the
	texture. 

	Different implementations using FBO, Aux buffers and Plain
	textures are available. 

	A much faster Draw To Texture is possible if using FBOs
	the "right" way (attaching target texture as color attachment) but
	that requires the FBO to be initialized specifically for the texture.
*/

gkImage* gkImageToDraw = 0;
gkColorNode* gkOldColorFilterTop;
gkTransformNode* gkOldTransformTop;

struct gkDrawToImageImplementation
{
	void (*initBuffer)(gkSize size);
	void (*cleanBuffer)();
	void (*beginDraw)();
	void (*endDraw)();
}*gkDTI = 0;

void gkInitDTI();

void gkInitDrawToImageBuffer(gkSize size)
{
	if (gkDTI == 0)
		gkInitDTI();

	gkDTI->initBuffer(size);
}

void gkCleanupDrawToImageBuffer()
{
	if (gkDTI != 0)
		gkDTI->cleanBuffer();
}

GK_BOOL gkBeginDrawToImage(gkImage* img, GK_BOOL clear)
{
	if(gkImageToDraw != 0) 
		return GK_FALSE;

	gkImageToDraw = img;

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	/* Set render target to preallocated buffer */
	gkDTI->beginDraw();

	glViewport(0,0, (GLsizei)img->width, (GLsizei)img->height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,(int)img->width,0,(int)img->height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0,0,0,0);
	if (GLEE_VERSION_1_4) {
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	} else {
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

	if(!clear)	
		gkDrawImage(gkImageToDraw, 0.0f, 0.0f);

	return GK_TRUE;
}

void gkEndDrawToImage()
{
	gkColorNode *colorNode;
	gkTransformNode *transformNode;

	if (gkImageToDraw == 0) {
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

	/* Reset render target to default frame buffer */
	gkDTI->endDraw();

	while (gkColorFilterTop->parent) {
		gkColorFilterTop = (colorNode = gkColorFilterTop)->parent;
		free(colorNode);
	}
	free(gkColorFilterTop);
	gkColorFilterTop = gkOldColorFilterTop;

	while (gkTransformTop->parent) {
		gkTransformTop = (transformNode = gkTransformTop)->parent;
		free(transformNode);
	}
	free(gkTransformTop);
	gkTransformTop = gkOldTransformTop;

	glMatrixMode(GL_MODELVIEW);

	gkImageToDraw = 0;
}

/* Begin implementations */

/*
	FBO Draw to image implementation 
*/

GLuint imgFramebufferId = 0;
GLuint imgRenderbuffers[2];

void gkDTIInitBufferFBO(gkSize size)
{
	GLuint status;
	glGenFramebuffersEXT(1, &imgFramebufferId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, imgFramebufferId);

	glGenRenderbuffersEXT(2, imgRenderbuffers);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, imgRenderbuffers[0]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, (int)size.width, (int)size.height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, imgRenderbuffers[0]);

	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, imgRenderbuffers[1]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16, (int)size.width, (int)size.height);
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
		imgFramebufferId = 0;

		printf("GK [INFO]: FBO initialization failed\n");
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void gkDTICleanBufferFBO()
{
	if(imgFramebufferId){
		glDeleteRenderbuffersEXT(2, imgRenderbuffers);
		glDeleteFramebuffersEXT(1, &imgFramebufferId);
	}
}

void gkDTIBeginDrawFBO()
{
	if(imgFramebufferId)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, imgFramebufferId);
}

void gkDTIEndDrawFBO()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

struct gkDrawToImageImplementation gkDTIFBO = {
	gkDTIInitBufferFBO, gkDTICleanBufferFBO,
	gkDTIBeginDrawFBO, gkDTIEndDrawFBO
};


/*
	AUX Buffer Draw to image implementation
*/

void gkDTIInitBufferAUX(gkSize size)
{
	/* do nothing for AUX */
}

void gkDTICleanBufferAUX()
{
	/* do nothing for AUX */
}

void gkDTIBeginDrawAUX()
{
	glReadBuffer(GL_AUX0);
	glDrawBuffer(GL_AUX0);
}

void gkDTIEndDrawAUX()
{
	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);
}

struct gkDrawToImageImplementation gkDTIAUX = {
	gkDTIInitBufferAUX, gkDTICleanBufferAUX,
	gkDTIBeginDrawAUX, gkDTIEndDrawAUX
};

/*
	TexCopy DTI implementation 
*/

gkImage* screenBufferImg = 0;
gkImage* screenDepthBufferImg = 0;


void gkDTIInitBufferTex(gkSize size)
{
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

void gkDTICleanBufferTex()
{
	if (screenBufferImg) {
		gkDestroyImage(screenBufferImg);
		gkDestroyImage(screenDepthBufferImg);
	}
}

void gkDTIBeginDrawTex()
{
	GLint viewportDimmension[4];

	glGetIntegerv(GL_VIEWPORT, viewportDimmension);

	glBindTexture(GL_TEXTURE_2D, screenBufferImg->id);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0, viewportDimmension[2], viewportDimmension[3]);

	glBindTexture(GL_TEXTURE_2D, screenDepthBufferImg->id);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, viewportDimmension[2], viewportDimmension[3], 0);
}

void gkDTIEndDrawTex()
{
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

struct gkDrawToImageImplementation gkDTITex = {
	gkDTIInitBufferTex, gkDTICleanBufferTex,
	gkDTIBeginDrawTex, gkDTIEndDrawTex
};

/* End of implementations */

void gkInitDTI()
{
	if (GLEE_EXT_framebuffer_object) {
		gkDTI = &gkDTIFBO;
		printf("GK [INFO]: Using FBO\n");
	} else {
		GLint auxBuffers = 0;
		glGetIntegerv(GL_AUX_BUFFERS, &auxBuffers);
		if (auxBuffers > 0) {
			gkDTI = &gkDTIAUX;
			printf("GK [INFO]: Using aux buffer\n");
		} else {
			gkDTI = &gkDTITex;
			printf("GK [INFO]: Using TexCopy\n");
		}
	}
}