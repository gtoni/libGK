#ifndef _GK_GL_H_
#define _GK_GL_H_

#include <gkConfig.h>

#if defined(GK_PLATFORM_WIN) || defined(GK_PLATFORM_TEST)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "GLee.h"
#include <GL/gl.h>
#elif defined(GK_PLATFORM_LINUX)
#include "GLee.h"
#include <GL/gl.h>
#else

#ifdef GK_SHOW_PLATFORM_ERRORS
#error OpenGL header not defined
#endif

#endif

#endif