#include "gkPlatform.h"

#ifdef GK_PLATFORM_TIZEN

#include <gk.h>
#include "gk_internal.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <new>
#include <FBase.h>
#include <FGraphics.h>
#include <FApp.h>
#include <FGraphicsOpengl.h>
#include <FSystem.h>
#include <FUi.h>

using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Base::Runtime;
using namespace Tizen::Graphics;
using namespace Tizen::Locales;
using namespace Tizen::System;
using namespace Tizen::App;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Graphics::Opengl;

static GK_BOOL gkActive;

int GLEE_EXT_framebuffer_object = 0;

extern "C"
{
//
// The entry function of Tizen application called by the operating system.
//
_EXPORT_ int
OspMain(int argc, char *pArgv[])
{
	gkAppMain();
	return 0;
}

}

static onInitCallback onInitCB;
static onRunCallback onLoop = 0;
static onRunCallback onExit = 0;

Timer* mainLoopTimer = 0;

Tizen::Graphics::Opengl::EGLDisplay eglDisplay;
Tizen::Graphics::Opengl::EGLSurface eglSurface;
Tizen::Graphics::Opengl::EGLConfig  eglConfig;
Tizen::Graphics::Opengl::EGLContext eglContext;

Tizen::Ui::Controls::Form* pForm;


class GlesCube11
	: public Tizen::App::Application
	, public Tizen::System::IScreenEventListener
	, public Tizen::Base::Runtime::ITimerEventListener
	, public Tizen::Ui::IKeyEventListener
	, public Tizen::Ui::ITouchEventListener
{
public:
	static Tizen::App::Application* CreateInstance(void);

	GlesCube11(void);
	~GlesCube11(void);

	virtual bool OnAppInitializing(Tizen::App::AppRegistry& appRegistry);
	virtual bool OnAppTerminating(Tizen::App::AppRegistry& appRegistry, bool forcedTermination = false);
	virtual void OnForeground(void);
	virtual void OnBackground(void);
	virtual void OnLowMemory(void);
	virtual void OnBatteryLevelChanged(Tizen::System::BatteryLevel batteryLevel);
	virtual void OnScreenOn (void);
	virtual void OnScreenOff (void);

	virtual void OnTimerExpired(Tizen::Base::Runtime::Timer& timer);

	virtual void OnKeyPressed(const Tizen::Ui::Control& source, Tizen::Ui::KeyCode keyCode);
	virtual void OnKeyReleased(const Tizen::Ui::Control& source, Tizen::Ui::KeyCode keyCode);
	virtual void OnKeyLongPressed(const Tizen::Ui::Control& source, Tizen::Ui::KeyCode keyCode);

	virtual void OnTouchCanceled (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo);
	virtual void OnTouchFocusIn (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo);
	virtual void OnTouchFocusOut (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo);
	virtual void OnTouchMoved (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo);
	virtual void OnTouchPressed (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo);
	virtual void OnTouchReleased (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo);
private:
	bool touched;
};

GlesCube11* app;


const int MAX_INT = 65536.0f;
const int TIME_OUT = 10;

class GlesForm
	: public Tizen::Ui::Controls::Form
{
public:
	GlesForm(GlesCube11* pApp)
		: __pApp(pApp)
	{
	}

	virtual ~GlesForm(void)
	{
	}

	virtual result OnDraw(void)
	{
		return E_SUCCESS;
	}

private:
	GlesCube11* __pApp;
};

GlesCube11::GlesCube11(void)
{
	eglDisplay = EGL_NO_DISPLAY;
	eglSurface = EGL_NO_SURFACE;
	eglConfig = null;
	eglContext = EGL_NO_CONTEXT;
	pForm = null;
	app = this;
	touched = false;
}

float gkFormWidth;
float gkFormHeight;

void onWindowMouseDown(int x, int y, int mb);
void onWindowMouseMove(int x, int y);
void onWindowMouseUp(int x, int y, int mb);

GlesCube11::~GlesCube11(void)
{
}

Application* GlesCube11::CreateInstance(void)
{
	return new (std::nothrow) GlesCube11();
}

GK_BOOL initScreen();
void cleanScreen();

GK_BOOL initGL();
void cleanGL();

GK_BOOL initScreen()
{
	result r = E_SUCCESS;
	Frame* pAppFrame = new (std::nothrow) Frame();
	TryReturn(pAppFrame != null, E_FAILURE, "[GlesCube11] Generating a frame failed.");

	r = pAppFrame->Construct();
	TryReturn(!IsFailed(r), E_FAILURE, "[GlesCube11] pAppFrame->Construct() failed.");

	app->AddFrame(*pAppFrame);

	pForm = new (std::nothrow) GlesForm(app);
	TryCatch(pForm != null, , "[GlesCube11] Allocation of GlesForm failed.");

	r = pForm->Construct(FORM_STYLE_NORMAL);
	TryCatch(!IsFailed(r), delete pForm, "[GlesCube11] __pForm->Construct(FORM_STYLE_NORMAL) failed.");

	r = app->GetAppFrame()->GetFrame()->AddControl(*pForm);
	TryCatch(!IsFailed(r), delete pForm, "[GlesCube11] GetAppFrame()->GetFrame()->AddControl(__pForm) failed.");

	pForm->AddKeyEventListener(*app);
	pForm->AddTouchEventListener(*app);

	mainLoopTimer = new (std::nothrow) Timer;
	TryCatch(mainLoopTimer != null, , "[GlesCube11] Failed to allocate memory.");

	r = mainLoopTimer->Construct(*app);
	TryCatch(!IsFailed(r), , "[GlesCube11] pTempTimer->Construct(*this) failed.");


	int x,y,w, h;

	Tizen::System::SystemInfo::GetValue("http://tizen.org/feature/screen.width", w);
	Tizen::System::SystemInfo::GetValue("http://tizen.org/feature/screen.height", h);
	gkScreenSize.width = (float)w;
	gkScreenSize.height = (float)h;

	pForm->GetSize(w, h);
	gkFormWidth = (float)w;
	gkFormHeight = (float)h;

	return GK_TRUE;

CATCH:
	AppLog("[GlesCube11] GlesCube11::OnAppInitializing eglError : %#x\n"
			"[GlesCube11] GlesCube11::OnAppInitializing glError : %#x\n"
			"[GlesCube11] GlesCube11::OnAppInitializing VENDOR : %s\n"
			"[GlesCube11] GlesCube11::OnAppInitializing GL_RENDERER : %s\n"
			"[GlesCube11] GlesCube11::OnAppInitializing GL_VERSION : %s\n ",
			static_cast<unsigned int>(eglGetError()),
			static_cast<unsigned int>(glGetError()),
			glGetString(GL_VENDOR),
			glGetString(GL_RENDERER),
			glGetString(GL_VERSION));

	cleanScreen();

	return GK_FALSE;
}

void cleanScreen()
{
	if (mainLoopTimer != 0)
	{
		mainLoopTimer->Cancel();
		delete mainLoopTimer;
		mainLoopTimer = 0;
	}
}

GK_BOOL initGL()
{
	EGLint numConfigs = 1;

	EGLint eglConfigList[] =
	{
		EGL_RED_SIZE,	8,
		EGL_GREEN_SIZE,	8,
		EGL_BLUE_SIZE,	8,
		EGL_ALPHA_SIZE,	0,
		EGL_DEPTH_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_NONE
	};

	EGLint eglContextList[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};

	eglBindAPI(EGL_OPENGL_ES_API);

	eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
	TryCatch(eglDisplay != EGL_NO_DISPLAY, , "[GlesCube11] eglGetDisplay() failed.");

	TryCatch(!(eglInitialize(eglDisplay, null, null) == EGL_FALSE || eglGetError() != EGL_SUCCESS), , "[GlesCube11] eglInitialize() failed.");

	TryCatch(!(eglChooseConfig(eglDisplay, eglConfigList, &eglConfig, 1, &numConfigs) == EGL_FALSE ||
			eglGetError() != EGL_SUCCESS), , "[GlesCube11] eglChooseConfig() failed.");

	TryCatch(numConfigs, , "[GlesCube11] eglChooseConfig() failed. because of matching config doesn't exist");

	eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)pForm, null);
	TryCatch(!(eglSurface == EGL_NO_SURFACE || eglGetError() != EGL_SUCCESS), , "[GlesCube11] eglCreateWindowSurface() failed.");

	eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, eglContextList);
	TryCatch(!(eglContext == EGL_NO_CONTEXT || eglGetError() != EGL_SUCCESS), , "[GlesCube11] eglCreateContext() failed.");

	TryCatch(!(eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) == EGL_FALSE ||
			eglGetError() != EGL_SUCCESS), , "[GlesCube11] eglMakeCurrent() failed.");

	GLEE_EXT_framebuffer_object = (strstr((const char*)glGetString(GL_EXTENSIONS), "OES_framebuffer_object")?GK_TRUE:GK_FALSE);

	return GK_TRUE;

CATCH:
	{
		AppLog("[GlesCube11] GlesCube11 can run on systems which supports OpenGL ES(R) 1.1.");
		AppLog("[GlesCube11] When GlesCube11 does not correctly execute, there are a few reasons.");
		AppLog("[GlesCube11]    1. The current device(real-target or emulator) does not support OpenGL ES(R) 1.1.\n"
				" Check the Release Notes.");
		AppLog("[GlesCube11]    2. The system running on emulator cannot support OpenGL(R) 1.5 or later.\n"
				" Try with other system.");
		AppLog("[GlesCube11]    3. The system running on emulator does not maintain the latest graphics driver.\n"
				" Update the graphics driver.");
	}

	cleanGL();

	return GK_FALSE;
}

void cleanGL()
{
	if (eglDisplay != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(eglDisplay, null, null, null);

		if (eglContext != EGL_NO_CONTEXT)
		{
			eglDestroyContext(eglDisplay, eglContext);
			eglContext = EGL_NO_CONTEXT;
		}

		if (eglSurface != EGL_NO_SURFACE)
		{
			eglDestroySurface(eglDisplay, eglSurface);
			eglSurface = EGL_NO_SURFACE;
		}

		eglTerminate(eglDisplay);
		eglDisplay = EGL_NO_DISPLAY;
	}

	eglConfig    = null;
}

bool GlesCube11::OnAppInitializing(AppRegistry& appRegistry)
{
	gkActive = GK_FALSE;
	AppLog("OnAppInitializing");
	if (initScreen()) {
		if (initGL()) {
			gkActive = GK_TRUE;
			AppLog("All loaded calling callback");
			onInitCB();
		}else {
			cleanScreen();
		}
	}
	return gkActive;
}


bool GlesCube11::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	AppLog("OnAppTerminating");
	gkActive = GK_FALSE;
	if (onExit)
		onExit();
	return true;
}


void GlesCube11::OnForeground(void)
{
	AppLog("OnForeground");
	if (mainLoopTimer != 0)
	{
		mainLoopTimer->Start(TIME_OUT);
	}
}


void GlesCube11::OnBackground(void)
{
	AppLog("OnBackground");
	if (mainLoopTimer != 0)
	{
		mainLoopTimer->Cancel();
	}
}


void GlesCube11::OnLowMemory(void)
{
}


void GlesCube11::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
}


void GlesCube11::OnTimerExpired(Timer& timer)
{
	if (mainLoopTimer == 0)
	{
		return;
	}

	mainLoopTimer->Start(TIME_OUT);

	if (onLoop)
		onLoop();
}

void
GlesCube11::OnKeyPressed(const Control& source, Tizen::Ui::KeyCode keyCode)
{
}

void
GlesCube11::OnKeyReleased(const Control& source, Tizen::Ui::KeyCode keyCode)
{
}

void
GlesCube11::OnKeyLongPressed(const Control& source, Tizen::Ui::KeyCode keyCode)
{
}

void GlesCube11::OnTouchCanceled (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
{

}
void GlesCube11::OnTouchFocusIn (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
{
	//nothing to do here
}
void GlesCube11::OnTouchFocusOut (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
{
	//nothing to do here
}

#define TRANSFORM_FORM_POINT(x,y) (((x)/gkFormWidth)*gkScreenSize.width),(((y)/gkFormHeight)*gkScreenSize.height)

void GlesCube11::OnTouchMoved (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
{
	onWindowMouseMove(TRANSFORM_FORM_POINT(currentPosition.x, currentPosition.y));
}

void GlesCube11::OnTouchPressed (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
{
	if(touched) return;
	touched = true;

	gkPoint cp = {TRANSFORM_FORM_POINT(currentPosition.x, currentPosition.y)};

    gkGlobalMouseState.position.x = (float)cp.x;
    gkGlobalMouseState.position.y = (float)cp.y;

    gkUpdateMouseTarget(gkMainPanel);
    gkCheckFocusedPanel();

	onWindowMouseDown(cp.x, cp.y, 0);
}
void GlesCube11::OnTouchReleased (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
{
	touched = false;
	onWindowMouseUp(TRANSFORM_FORM_POINT(currentPosition.x, currentPosition.y), 0);
}


void
GlesCube11::OnScreenOn(void)
{
}

void
GlesCube11::OnScreenOff(void)
{
}

static GK_BOOL initTizen(onInitCallback onInit)
{
	onInitCB = onInit;

	ArrayList args;
	args.Construct();

	Tizen::App::Application::Execute(GlesCube11::CreateInstance, &args);

	return GK_TRUE;
}

static void runTizen(onRunCallback loop, onRunCallback cleanup)
{
	AppLog("Run tizen");
	AppLog((char*)glGetString(GL_EXTENSIONS));
	onLoop = loop;
	onExit = cleanup;
}

static void cleanupTizen()
{
	AppLog("cleanup tizen");
	cleanGL();
	cleanScreen();
}

static void ExitAppTizen()
{
	AppLog("Exit tizen");
	gkActive = GK_FALSE;
	app->Terminate();
}

static void GetAppDirTizen(char* dst, size_t dstSize)
{
	printf("getAppDir");
	strcpy(dst, "");
}

static void SetVSyncTizen(GK_BOOL enabled)
{
	eglSwapInterval(0, enabled);
}

static void ResizeScreenTizen(gkSize size)
{
	AppLog("resizeScreen");
}

/* Not sure what's wrong with the one in gkGeometry.c */
gkSize GK_SIZE(float width, float height){ gkSize s = {width,height}; return s;};

static int GetSupportedSizesTizen(gkSize* sizes)
{
	AppLog("getSupportedSizes");
	if (sizes)
		sizes[0] = GK_SIZE(100,100);
	return 1;
}

static GK_BOOL GoFullscreenTizen()
{
	AppLog("goFullscreen");
	return GK_TRUE;
}

static void GoWindowedTizen(GK_BOOL oldVal)
{
	AppLog("goWindowed");
}

static GK_BOOL SetFullscreenTizen(GK_BOOL enable)
{
	GK_BOOL wasFullscreen = gkIsFullscreen();
	GK_BOOL success = GK_FALSE;
	if (enable)
		enable = success = GoFullscreenTizen();
	if (!enable)
		GoWindowedTizen(wasFullscreen);
	return success;
}

static void SetWindowTitleTizen(char* title)
{
	AppLog("setWindowTitle");
}

static void GetWindowTitleTizen(char* dst, size_t dstSize)
{
	AppLog("getWindowTitle");
}

static void SetWindowResizableTizen(GK_BOOL resizable)
{
	AppLog("setWindowResizable");
}

static void SleepTizen(uint32_t ms)
{
	//Not used in tizen's loop
}

static uint16_t PrepareKeyTizen(uint16_t keyCode, uint16_t scanCode, GK_BOOL keyDown)
{
    return keyCode;
}

static void ProcessEventsTizen()
{
	//TIZEN: Tizen uses event callbacks
}

static void SwapBuffersTizen()
{
	eglSwapBuffers(eglDisplay, eglSurface);
}

#ifdef __cplusplus
extern "C"{
#endif

	gkPlatform gkGetPlatform()
	{
		gkPlatform platform;

		platform.Init = initTizen;
		platform.Run = runTizen;
		platform.Cleanup = cleanupTizen;
		platform.Exit = ExitAppTizen;

		platform.GetAppDir = GetAppDirTizen;
		platform.SetVSync = SetVSyncTizen;

		platform.ResizeScreen = ResizeScreenTizen;
		platform.GetSupportedSizes = GetSupportedSizesTizen;
		platform.SetFullscreen = SetFullscreenTizen;

		platform.SetWindowTitle = SetWindowTitleTizen;
		platform.GetWindowTitle = GetWindowTitleTizen;

		platform.SetWindowResizable = SetWindowResizableTizen;

		platform.Sleep = SleepTizen;

		platform.PrepareKey = PrepareKeyTizen;

		platform.ProcessEvents = ProcessEventsTizen;

		platform.SwapBuffers = SwapBuffersTizen;

		return platform;
	}

	uint64_t gkMilliseconds(){
	    long long int ticks;
	    Tizen::System::SystemTime::GetTicks(ticks);
	    return ticks - gkAppStartTime;
	}

#ifdef __cplusplus
}
#endif

#endif
