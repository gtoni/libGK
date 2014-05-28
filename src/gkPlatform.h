#ifndef _GK_PLATFORM_H_
#define _GK_PLATFORM_H_

#define GK_PLATFORM_WIN

#include <gkTypes.h>
#include <gkGeometry.h>

#define GK_METHOD(result, name, params) result (*name)params

#define GK_MAX_APPDIR_SIZE	1024
#define GK_MAX_TITLE_SIZE	256

extern gkSize gkScreenSize;

typedef void (*onRunCallback)();

typedef struct gkPlatform
{
	GK_METHOD(GK_BOOL, Init,());
	GK_METHOD(void, Run, (onRunCallback loop, onRunCallback callback));
	GK_METHOD(void, Cleanup,());
	GK_METHOD(void, Exit,());

	GK_METHOD(void, GetAppDir,(char* dst, size_t dstSize));
	GK_METHOD(void, SetVSync,(GK_BOOL));

	GK_METHOD(void, ResizeScreen, (gkSize size));
	GK_METHOD(int, GetSupportedSizes, (gkSize* sizes));
	GK_METHOD(GK_BOOL, SetFullscreen, (GK_BOOL enable));

	GK_METHOD(void, SetWindowTitle, (char* title));
	GK_METHOD(void, GetWindowTitle, (char* dst, size_t dstSize));

	GK_METHOD(void, SetWindowResizable, (GK_BOOL resizable));

	GK_METHOD(void, Sleep, (uint32_t ms));

	GK_METHOD(uint16_t, PrepareKey, (uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown));

	GK_METHOD(void, ProcessEvents,());

	GK_METHOD(void, SwapBuffers, ());
}gkPlatform;

#ifdef __cplusplus
extern "C"{
#endif

gkPlatform gkGetPlatform();

/* Called from platform specific code */
void onWindowClose();
void onWindowMouseMove(int x, int y);
void onWindowMouseWheel(int x, int y, int w);
void onWindowMouseDown(int x, int y, int mb);
void onWindowMouseUp(int x, int y, int mb);
void onWindowSizeChanged(gkSize nsize);
void onWindowKeyDown(uint16_t keyCode, uint16_t scanCode);
void onWindowKeyUp(uint16_t keyCode, uint16_t scanCode);
void onWindowCharacter(uint32_t character);

#ifdef __cplusplus
}
#endif


#endif