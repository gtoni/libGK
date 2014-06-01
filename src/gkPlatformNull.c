#include "gkPlatform.h"

#ifdef GK_PLATFORM_NULL

#include <gk.h>
#include "gk_internal.h"

#include <stdio.h>

void main()
{
	gkAppMain();
}

static GK_BOOL gkActive;

static GK_BOOL init_null(onInitCallback onInit)
{
	printf("init\n");

	gkActive = GK_TRUE;

	onInit();

	return GK_TRUE;
}

static void run_null(onRunCallback loop, onRunCallback cleanup)
{
	printf("run\n");
	while(gkActive)
		loop();
	cleanup();
}

static void cleanup_null()
{
	printf("cleanup\n");
}

static void ExitApp_null()
{
	printf("exit\n");
	gkActive = GK_FALSE;
}

static void GetAppDir_null(char* dst, size_t dstSize)
{
	printf("getAppDir\n");
	strcpy(dst, "");
}

static void SetVSync_null(GK_BOOL enabled)
{
	printf("setVSync\n");
}

static void ResizeScreen_null(gkSize size)
{
	printf("resizeScreen\n");
}

static int GetSupportedSizes_null(gkSize* sizes)
{
	printf("getSupportedSizes\n");
	if (sizes)
		sizes[0] = GK_SIZE(100,100);
	return 1;
}

static GK_BOOL GoFullscreen_null()
{
	printf("goFullscreen\n");
	return GK_TRUE;
}

static void GoWindowed_null(GK_BOOL oldVal)
{
	printf("goWindowed\n");
}

static GK_BOOL SetFullscreen_null(GK_BOOL enable)
{
	GK_BOOL wasFullscreen = gkIsFullscreen();
	GK_BOOL success = GK_FALSE;
	if (enable)
		enable = success = GoFullscreen_null();
	if (!enable)
		GoWindowed_null(wasFullscreen);
	return success;
}

static void SetWindowTitle_null(char* title)
{
	printf("setWindowTitle\n");
}

static void GetWindowTitle_null(char* dst, size_t dstSize)
{
	printf("getWindowTitle\n");
}

static void SetWindowResizable_null(GK_BOOL resizable)
{
	printf("setWindowResizable\n");
}

static void Sleep_null(uint32_t ms)
{
	printf("sleep\n");
}

static uint16_t PrepareKey_null(uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
    return keyCode;
}

static void ProcessEvents_null()
{
	printf("processEvents\n");
}

static void SwapBuffers_null()
{
	printf("swapBuffers\n");
}

#ifdef __cplusplus
extern "C"{
#endif

	gkPlatform gkGetPlatform()
	{
		gkPlatform platform;

		platform.Init = init_null;
		platform.Run = run_null;
		platform.Cleanup = cleanup_null;
		platform.Exit = ExitApp_null;

		platform.GetAppDir = GetAppDir_null;
		platform.SetVSync = SetVSync_null;

		platform.ResizeScreen = ResizeScreen_null;
		platform.GetSupportedSizes = GetSupportedSizes_null;
		platform.SetFullscreen = SetFullscreen_null;

		platform.SetWindowTitle = SetWindowTitle_null;
		platform.GetWindowTitle = GetWindowTitle_null;

		platform.SetWindowResizable = SetWindowResizable_null;

		platform.Sleep = Sleep_null;

		platform.PrepareKey = PrepareKey_null;

		platform.ProcessEvents = ProcessEvents_null;

		platform.SwapBuffers = SwapBuffers_null;

		return platform;
	}


#ifdef __cplusplus
}
#endif

#endif