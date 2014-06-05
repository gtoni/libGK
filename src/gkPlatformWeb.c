#include "gkPlatform.h"

#ifdef GK_PLATFORM_WEB

#include <gk.h>
#include "gk_internal.h"

#include <stdio.h>
#include <GL/glfw.h>
#include <emscripten/emscripten.h>

int main()
{
	gkAppMain();
	return 0;
}

static GK_BOOL gkActive;

static onRunCallback onCleanup;

static GK_BOOL initWeb(onInitCallback onInit)
{
	int width = 800, height = 600;
 
    if (glfwInit() != GL_TRUE) {
        printf("glfwInit() failed\n");
        return GL_FALSE;
    }
 
    if (glfwOpenWindow(width, height, 8, 8, 8, 8, 16, 0, GLFW_WINDOW) != GL_TRUE) {
        printf("glfwOpenWindow() failed\n");
        return GL_FALSE;
    }
	
	gkScreenSize.width = (float)width;
	gkScreenSize.height = (float)height;
	
	gkActive = GK_TRUE;

	onInit();

	return GK_TRUE;
}

static void runWeb(onRunCallback loop, onRunCallback cleanup)
{
	onCleanup = cleanup;
	emscripten_set_main_loop(loop, 0, 1);
}

static void cleanupWeb()
{
	glfwTerminate();
}

static void ExitAppWeb()
{
	gkActive = GK_FALSE;
	emscripten_cancel_main_loop();
	onCleanup();
}

static void GetAppDirWeb(char* dst, size_t dstSize)
{
	printf("getAppDir\n");
	strcpy(dst, "");
}

static void SetVSyncWeb(GK_BOOL enabled)
{
	printf("setVSync\n");
}

static void ResizeScreenWeb(gkSize size)
{
	printf("resizeScreen\n");
}

static int GetSupportedSizesWeb(gkSize* sizes)
{
	printf("getSupportedSizes\n");
	if (sizes)
		sizes[0] = GK_SIZE(100,100);
	return 1;
}

static GK_BOOL GoFullscreenWeb()
{
	printf("goFullscreen\n");
	return GK_TRUE;
}

static void GoWindowedWeb(GK_BOOL oldVal)
{
	printf("goWindowed\n");
}

static GK_BOOL SetFullscreenWeb(GK_BOOL enable)
{
	GK_BOOL wasFullscreen = gkIsFullscreen();
	GK_BOOL success = GK_FALSE;
	if (enable)
		enable = success = GoFullscreenWeb();
	if (!enable)
		GoWindowedWeb(wasFullscreen);
	return success;
}

static void SetWindowTitleWeb(char* title)
{
	printf("setWindowTitle\n");
}

static void GetWindowTitleWeb(char* dst, size_t dstSize)
{
	printf("getWindowTitle\n");
}

static void SetWindowResizableWeb(GK_BOOL resizable)
{
	printf("setWindowResizable\n");
}

static void SleepWeb(uint32_t ms)
{
//	printf("sleep\n");
}

static uint16_t PrepareKeyWeb(uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
    return keyCode;
}

static void ProcessEventsWeb()
{
	glfwPollEvents();
	
    int x_pos, y_pos;
    glfwGetMousePos(&x_pos, &y_pos);
	onWindowMouseMove((float)x_pos, (float)y_pos);
	
    const int left_mouse_button_state = glfwGetMouseButton(GLFW_MOUSE_BUTTON_1);
    if (left_mouse_button_state == GLFW_PRESS) {
		onWindowMouseDown((float)x_pos, (float)y_pos, 0);
    }
}

static void SwapBuffersWeb()
{
	glfwTerminate();
}

#ifdef __cplusplus
extern "C"{
#endif

	gkPlatform gkGetPlatform()
	{
		gkPlatform platform;

		platform.Init = initWeb;
		platform.Run = runWeb;
		platform.Cleanup = cleanupWeb;
		platform.Exit = ExitAppWeb;

		platform.GetAppDir = GetAppDirWeb;
		platform.SetVSync = SetVSyncWeb;

		platform.ResizeScreen = ResizeScreenWeb;
		platform.GetSupportedSizes = GetSupportedSizesWeb;
		platform.SetFullscreen = SetFullscreenWeb;

		platform.SetWindowTitle = SetWindowTitleWeb;
		platform.GetWindowTitle = GetWindowTitleWeb;

		platform.SetWindowResizable = SetWindowResizableWeb;

		platform.Sleep = SleepWeb;

		platform.PrepareKey = PrepareKeyWeb;

		platform.ProcessEvents = ProcessEventsWeb;

		platform.SwapBuffers = SwapBuffersWeb;

		return platform;
	}


#ifdef __cplusplus
}
#endif

#endif