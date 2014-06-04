#ifndef _GK_GL_H_
#define _GK_GL_H_

#include <gkConfig.h>

#if defined(GK_PLATFORM_WIN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "GLee.h"
#include <GL/gl.h>

#define glClipPlaneVarType	double
#define glClipPlanef	glClipPlane

#elif defined(GK_PLATFORM_LINUX)
#include "GLee.h"
#include <GL/gl.h>

#define glClipPlaneVarType	double
#define glClipPlanef	glClipPlane

#elif defined(GK_PLATFORM_TIZEN)
#include <gl.h>
#include <glext.h>

#define GK_GLES_1

#elif defined(GK_PLATFORM_ANDROID)

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#define GK_GLES_1

#elif defined(GK_PLATFORM_WEB)

#include <GL/glew.h>

#define GLEE_EXT_framebuffer_object 1

#define glClipPlaneVarType	double
#define glClipPlanef	glClipPlane

#else

#ifdef GK_SHOW_PLATFORM_ERRORS
#error OpenGL header not defined
#endif

#endif

#ifdef GK_GLES_1

#define glClipPlaneVarType	float
#define glOrtho(a,b,c,d,e,f) {float orthoMartix[] = {2.0f/b, 0, 0, 0, 0, -2.0f/c, 0, 0,	0,0, 0, 0, -1.0f,1.0f,0,1.0f}; glLoadMatrixf(orthoMartix);}

extern int GLEE_EXT_framebuffer_object;

#define GL_DEPTH_COMPONENT16 	GL_DEPTH_COMPONENT16_OES
#define GL_FRAMEBUFFER_COMPLETE	GL_FRAMEBUFFER_COMPLETE_OES
#define GL_FRAMEBUFFER_INCOMPLETE	GL_FRAMEBUFFER_INCOMPLETE_OES
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES
#define GL_FRAMEBUFFER_UNSUPPORTED	GL_FRAMEBUFFER_UNSUPPORTED_OES
#define GL_FRAMEBUFFER	GL_FRAMEBUFFER_OES
#define GL_RENDERBUFFER	GL_RENDERBUFFER_OES
#define GL_COLOR_ATTACHMENT0	GL_COLOR_ATTACHMENT0_OES
#define GL_DEPTH_ATTACHMENT	GL_DEPTH_ATTACHMENT_OES
#define glBindFramebuffer glBindFramebufferOES
#define glBindRenderbuffer	glBindRenderbufferOES
#define glCheckFramebufferStatus	glCheckFramebufferStatusOES
#define glDeleteFramebuffers	glDeleteFramebuffersOES
#define glDeleteRenderbuffers	glDeleteRenderbuffersOES
#define glFramebufferRenderbuffer	glFramebufferRenderbufferOES
#define glGenFramebuffers	glGenFramebuffersOES
#define glGenRenderbuffers	glGenRenderbuffersOES
#define glRenderbufferStorage	glRenderbufferStorageOES

#endif

#endif
