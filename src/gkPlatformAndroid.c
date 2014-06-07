#include "gkPlatform.h"

#ifdef GK_PLATFORM_ANDROID

#include <gk.h>
#include "gk_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android/window.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "GK", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "GK", __VA_ARGS__))


/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    int initialized;
	int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
	EGLConfig config;
}engine;

/* internalDataPath doesn't work on some (or ALL) phones */
extern char* gkAndroidAppDir;

struct android_app* gkAndroidApp;

static GK_BOOL gkActive;

int GLEE_EXT_framebuffer_object = 0;


/**
 * Initialize an EGL context for the current display.
 */
 
static void initDisplay(struct engine* engine)
{
    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
	 
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;
	
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	
	engine->display = display;
	engine->config = config;
}

static void createSurface(struct engine* engine)
{
	EGLint format;
    EGLSurface surface;
	
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */

	eglGetConfigAttrib(engine->display, engine->config, EGL_NATIVE_VISUAL_ID, &format);
	
	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	__android_log_print(ANDROID_LOG_INFO, "GK", "Choose format is %d", format);	
		
    surface = eglCreateWindowSurface(engine->display, engine->config, engine->app->window, NULL);
	
	engine->surface = surface;
}

static int bindContext(struct engine* engine, GK_BOOL bind)
{
	if (eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return 0;
	}
	return 1;
}
 
static int updateGL(struct engine* engine) {
	GK_BOOL init = engine->context == EGL_NO_CONTEXT;
    EGLint w, h;
	
	if (init) {
		initDisplay(engine);
		createSurface(engine);
		engine->context = eglCreateContext(engine->display, engine->config, NULL, NULL);
	}else {
		createSurface(engine);
	}

	if (!bindContext(engine, GK_TRUE))
		return -1;
	
	GLEE_EXT_framebuffer_object = (strstr((const char*)glGetString(GL_EXTENSIONS), "OES_framebuffer_object")?GK_TRUE:GK_FALSE);

    eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &w);
    eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &h);

	if (init) {
		gkScreenSize.width = w;
		gkScreenSize.height = h;
	} else {
		onWindowSizeChanged(GK_SIZE(w, h));
	}
	
	engine->initialized = GK_TRUE;
	engine->animating = GK_TRUE;
	
    return 0;
}

static void destroySurface(struct engine* engine)
{
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
	}
    engine->animating = GK_FALSE;
	engine->surface = EGL_NO_SURFACE;
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}


/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		int i, c = AMotionEvent_getPointerCount(event);
		for( i = 0; i<c; i++) {
			int32_t action = AMotionEvent_getAction(event);
			uint32_t index = AMotionEvent_getPointerId(event, i);
			float x = AMotionEvent_getX(event, i);
			float y = AMotionEvent_getY(event, i);
			action = action & AMOTION_EVENT_ACTION_MASK;
			if (action == AMOTION_EVENT_ACTION_DOWN) {
				onWindowMouseDown(x,y,index);
			}else if (action == AMOTION_EVENT_ACTION_MOVE) {
				onWindowMouseMove(x,y);
			}else if (action == AMOTION_EVENT_ACTION_UP) {
				onWindowMouseUp(x,y,index);
			}
			__android_log_print(ANDROID_LOG_INFO, "GK", "motion ptr: %d  x: %f y: %f", index, x, y);
		}
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
			LOGI("APP_CMD_SAVE_STATE");
            engine->app->savedState = malloc(sizeof(int));
            *((int*)engine->app->savedState) = 0x1337;
            engine->app->savedStateSize = sizeof(int);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
			LOGI("APP_CMD_INIT_WINDOW");
            if (engine->app->window != NULL) {
                updateGL(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
			LOGI("APP_CMD_TERM_WINDOW");
			destroySurface(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
			LOGI("APP_CMD_GAINED_FOCUS");
			engine->animating = engine->surface != EGL_NO_SURFACE;
            break;
        case APP_CMD_LOST_FOCUS:
            // Also stop animating.
			LOGI("APP_CMD_LOST_FOCUS");
            engine->animating = GK_FALSE;
            break;
    }
}

void android_main(struct android_app* state) {
    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    gkAndroidApp = state;

	LOGI("android_main");
			
	gkAppMain();
}

static GK_BOOL initAndroid(onInitCallback onInit)
{
	int events;
	struct android_poll_source* source;
	
	gkActive = GK_TRUE;
	
	ANativeActivity_setWindowFlags(engine.app->activity, AWINDOW_FLAG_FULLSCREEN, AWINDOW_FLAG_SHOW_WALLPAPER);
		
	while (gkActive && !engine.initialized) {
		if(ALooper_pollAll(-1, 0, &events, (void**)&source) >= 0) {
            if (source != NULL) {
                source->process(engine.app, source);
            }
		}
	}
	
	if(gkActive)
		onInit();

	return GK_TRUE;
}

static void ProcessEventsAndroid();
static void SleepAndroid(uint32_t ms);

static void runAndroid(onRunCallback loop, onRunCallback cleanup)
{
	while (gkActive) {
		if (engine.animating) {
			loop();
		} else {
			ProcessEventsAndroid();
			SleepAndroid(100);
		}
    }
	cleanup();
	exit(0);
}

static void cleanupAndroid()
{
	engine_term_display(&engine);
}

static void ExitAppAndroid()
{
	LOGI("exit");
	gkActive = GK_FALSE;
}

static void GetAppDirAndroid(char* dst, size_t dstSize)
{
	size_t dirLen;
	const char* dir = gkAndroidAppDir;
	if (engine.app->activity->internalDataPath)
		dir = engine.app->activity->internalDataPath;

	dirLen = strlen(dir);
	LOGI("AppDir is %s", dir);
	if (dirLen > dstSize) {
		LOGI("AppDir path is too long");
		return;
	}
	strcpy(dst, dir);
}

static void SetVSyncAndroid(GK_BOOL enabled)
{
    eglSwapInterval(engine.display, enabled);
}

static void ResizeScreenAndroid(gkSize size)
{
	LOGI("resizeScreen");
}

static int GetSupportedSizesAndroid(gkSize* sizes)
{
	LOGI("getSupportedSizes");
	if (sizes)
		sizes[0] = GK_SIZE(100,100);
	return 1;
}

static GK_BOOL GoFullscreenAndroid()
{
	LOGI("goFullscreen");
	return GK_TRUE;
}

static void GoWindowedAndroid(GK_BOOL oldVal)
{
	LOGI("goWindowed");
}

static GK_BOOL SetFullscreenAndroid(GK_BOOL enable)
{
	GK_BOOL wasFullscreen = gkIsFullscreen();
	GK_BOOL success = GK_FALSE;
	if (enable)
		enable = success = GoFullscreenAndroid();
	if (!enable)
		GoWindowedAndroid(wasFullscreen);
	return success;
}

static void SetWindowTitleAndroid(char* title)
{
	LOGI("setWindowTitle");
}

static void GetWindowTitleAndroid(char* dst, size_t dstSize)
{
	LOGI("getWindowTitle");
}

static void SetWindowResizableAndroid(GK_BOOL resizable)
{
	LOGI("setWindowResizable");
}

static void SleepAndroid(uint32_t ms)
{
        struct timespec restTime;
        restTime.tv_sec = ms/1000;
        restTime.tv_nsec = (ms%1000)*1000000;
        nanosleep(&restTime, 0);
}

static uint16_t PrepareKeyAndroid(uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
    return keyCode;
}

static void ProcessEventsAndroid()
{
	struct android_app* state = engine.app;
    int maxMsgPerFrame = 3;
	
    int ident;
    int events;
    struct android_poll_source* source;

	if (engine.surface != EGL_NO_SURFACE) {
		GLint w;
		GLint h;
		gkSize s;
		eglQuerySurface(engine.display, engine.surface, EGL_WIDTH, &w);
		eglQuerySurface(engine.display, engine.surface, EGL_HEIGHT, &h);
		s.width = (float)w;
		s.height = (float)h;
		if ( s.width != gkScreenSize.width || s.height != gkScreenSize.height) {
			onWindowSizeChanged(s);
		}
	}

	
	while (maxMsgPerFrame-->0 && (ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
		// Process this event.
		if (source != NULL) {
			source->process(state, source);
		}

		// Check if we are exiting.
		if (state->destroyRequested != 0) {
			gkActive = GK_FALSE;
			break;
		}
	}
}

static void SwapBuffersAndroid()
{
    eglSwapBuffers(engine.display, engine.surface);
}

#ifdef __cplusplus
extern "C"{
#endif

	gkPlatform gkGetPlatform()
	{
		gkPlatform platform;

		platform.Init = initAndroid;
		platform.Run = runAndroid;
		platform.Cleanup = cleanupAndroid;
		platform.Exit = ExitAppAndroid;

		platform.GetAppDir = GetAppDirAndroid;
		platform.SetVSync = SetVSyncAndroid;

		platform.ResizeScreen = ResizeScreenAndroid;
		platform.GetSupportedSizes = GetSupportedSizesAndroid;
		platform.SetFullscreen = SetFullscreenAndroid;

		platform.SetWindowTitle = SetWindowTitleAndroid;
		platform.GetWindowTitle = GetWindowTitleAndroid;

		platform.SetWindowResizable = SetWindowResizableAndroid;

		platform.Sleep = SleepAndroid;

		platform.PrepareKey = PrepareKeyAndroid;

		platform.ProcessEvents = ProcessEventsAndroid;

		platform.SwapBuffers = SwapBuffersAndroid;

		return platform;
	}


#ifdef __cplusplus
}
#endif

#endif