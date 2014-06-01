#include <gk.h>

#include "Assets.h"
#include "Controls.h"
#include "AudioControl.h"

#define PANEL(x) ((gkPanel*)x)

gkPanel* staticLabel;
gkPanel* loadStaticBtn;
gkPanel* playStaticBtn;
gkPanel* controlStaticBtn;

gkPanel* streamLabel;
gkPanel* loadStreamBtn;
gkPanel* playStreamBtn;
gkPanel* controlStreamBtn;


gkPanel* masterVolLabel;
Slider* masterVolSlider;

gkPanel* controlPanel;
gkPanel* controlLabel;
gkPanel* playBtn;
gkPanel* pauseBtn;
gkPanel* stopBtn;
gkPanel* loopOnBtn;
gkPanel* loopOffBtn;

gkPanel* pitchLabel;
Slider* pitchSlider;
gkPanel* resetPitchBtn;

gkPanel* volLabel;
Slider* volSlider;

char timeLabelText[10];
char lengthLabelText[10];
gkPanel* timeLabel;
gkPanel* lengthLabel;
Slider* timeSlider;

#define _USE_MATH_DEFINES 1
#include <math.h>
#include <stdio.h>

void printTime(char* target, float sec)
{
    float hours, minutes;
    hours = floorf(sec/3600.0f);
    sec -= hours*3600.0f;
    minutes = floorf(sec/60.0f);
    sec -= minutes*60.0f;
    sprintf(target, "%02d:%02d", (int)minutes, (int)sec);
}

void updateControlPanel(gkPanel* panel)
{
	gkSound* snd = GetSound();
	float length = snd ? snd->length : 0.0f;
	float offset = gkGetSoundOffset(GetSoundSource());
	printTime(timeLabelText, offset);
	printTime(lengthLabelText, length);
	SetSliderPos(timeSlider, offset/length);
}

GK_BOOL onPlayClicked(gkEvent* e, void *p)
{
	PlaySource();
	playBtn->visible = GK_FALSE;
	pauseBtn->visible = GK_TRUE;
	return GK_TRUE;
}

GK_BOOL onPauseClicked(gkEvent* e, void *p)
{
	PauseSource();
	playBtn->visible = GK_TRUE;
	pauseBtn->visible = GK_FALSE;
	return GK_TRUE;
}

GK_BOOL onStopClicked(gkEvent* e, void *p)
{
	StopSource();
	playBtn->visible = GK_TRUE;
	pauseBtn->visible = GK_FALSE;
	return GK_TRUE;
}

GK_BOOL onLoopOnClicked(gkEvent* e, void *p)
{
	gkSetSoundLooping(GetSoundSource(), GK_TRUE);
	loopOnBtn->visible = GK_FALSE;
	loopOffBtn->visible = GK_TRUE;
	return GK_TRUE;
}
GK_BOOL onLoopOffClicked(gkEvent* e, void *p)
{
	gkSetSoundLooping(GetSoundSource(), GK_FALSE);
	loopOnBtn->visible = GK_TRUE;
	loopOffBtn->visible = GK_FALSE;
	return GK_TRUE;
}

GK_BOOL onTimeSliderChanged(gkEvent* e, void *p)
{
	float v;
	gkSound* snd = GetSound();
	float length = snd ? snd->length : 0.0f;
	GetSliderPos(timeSlider, &v);
	gkSetSoundOffset(GetSoundSource(), v*length);
	return GK_TRUE;
}

GK_BOOL onVolumeChanged(gkEvent* e, void *p)
{
	float v;
	GetSliderPos(volSlider, &v);
	gkSetSoundGain(GetSoundSource(), v);
	return GK_TRUE;
}

GK_BOOL onPitchChanged(gkEvent* e, void *p)
{
	float v;
	GetSliderPos(pitchSlider, &v);
	gkSetSoundPitch(GetSoundSource(), v);
	return GK_TRUE;
}

GK_BOOL onResetPitchClicked(gkEvent* e, void *p)
{
	gkSetSoundPitch(GetSoundSource(), 1.0f);
	SetSliderPos(pitchSlider, 1.0f);
	return GK_TRUE;
}

void createControlUI()
{
	controlPanel = gkCreatePanel();
	controlPanel->x = 10.0f;
	controlPanel->y = 130.0f;
	controlPanel->width = 300.0f;
	controlPanel->height = 300.0f;
	controlPanel->updateFunc = updateControlPanel;
	gkAddChild(gkMainPanel, controlPanel);

	controlLabel = CreateLabel("Sound source control:");
	gkAddChild(controlPanel, controlLabel);

	playBtn = CreateButton("Play");
	playBtn->x = 10.0f;
	playBtn->y = 45.0f;
	gkAddChild(controlPanel, playBtn);
	gkAddListener(playBtn, GK_ON_MOUSE_UP, 0, onPlayClicked, 0);

	pauseBtn = CreateButton("Pause");
	pauseBtn->x = playBtn->x;
	pauseBtn->y = playBtn->y;
	pauseBtn->visible = GK_FALSE;
	gkAddChild(controlPanel, pauseBtn);
	gkAddListener(pauseBtn, GK_ON_MOUSE_UP, 0, onPauseClicked, 0);

	stopBtn = CreateButton("Stop");
	stopBtn->x = pauseBtn->x + 90.0f;
	stopBtn->y = playBtn->y;
	gkAddChild(controlPanel, stopBtn);
	gkAddListener(stopBtn, GK_ON_MOUSE_UP, 0, onStopClicked, 0);

	loopOnBtn = CreateButton("Loop: ON");
	loopOnBtn->x = stopBtn->x + 90.0f;
	loopOnBtn->y = playBtn->y;
	gkAddChild(controlPanel, loopOnBtn);
	gkAddListener(loopOnBtn, GK_ON_MOUSE_UP, 0, onLoopOnClicked, 0);

	loopOffBtn = CreateButton("Loop: OFF");
	loopOffBtn->x = stopBtn->x + 90.0f;
	loopOffBtn->y = playBtn->y;
	loopOffBtn->visible = GK_FALSE;
	gkAddChild(controlPanel, loopOffBtn);
	gkAddListener(loopOffBtn, GK_ON_MOUSE_UP, 0, onLoopOffClicked, 0);

	pitchLabel = CreateLabel("Pitch");
	pitchLabel->y = 90.0f;
	gkAddChild(controlPanel, pitchLabel);

	pitchSlider = CreateSlider();
	SetSliderRange(pitchSlider, 0.5f, 2.0f);
	PANEL(pitchSlider)->x = pitchLabel->x + 90.0f;
	PANEL(pitchSlider)->y = pitchLabel->y + 4.0f;
	PANEL(pitchSlider)->width = 270.0f;
	gkAddChild(controlPanel, PANEL(pitchSlider));
	SetSliderPos(pitchSlider, gkGetSoundPitch(GetSoundSource()));
	gkAddListener(pitchSlider, ON_SLIDER_CHANGE, 0, onPitchChanged, 0);

	resetPitchBtn = CreateButton("Reset");
	resetPitchBtn->x = PANEL(pitchSlider)->x + 285.0f;
	resetPitchBtn->y = pitchLabel->y + 2.0f;
	gkAddChild(controlPanel, resetPitchBtn);
	gkAddListener(resetPitchBtn, GK_ON_MOUSE_UP, 0, onResetPitchClicked, 0);

	volLabel = CreateLabel("Volume");
	volLabel->y = pitchLabel->y + 40.0f;
	gkAddChild(controlPanel, volLabel);

	volSlider = CreateSlider();
	PANEL(volSlider)->x = volLabel->x + 90.0f;
	PANEL(volSlider)->y = volLabel->y + 4.0f;
	PANEL(volSlider)->width = 270.0f;
	gkAddChild(controlPanel, PANEL(volSlider));
	SetSliderPos(volSlider, gkGetSoundGain(GetSoundSource()));
	gkAddListener(volSlider, ON_SLIDER_CHANGE, 0, onVolumeChanged, 0);

	strcpy(timeLabelText, "00:00");
	strcpy(lengthLabelText, "00:00");

	timeLabel = CreateLabel(timeLabelText);
	timeLabel->y = volLabel->y + 50.0f;
	gkAddChild(controlPanel, timeLabel);

	timeSlider = CreateSlider();
	PANEL(timeSlider)->x = timeLabel->x + 90.0f;
	PANEL(timeSlider)->y = timeLabel->y + 2.0f;
	PANEL(timeSlider)->width = 270.0f;
	gkAddChild(controlPanel, PANEL(timeSlider));
	gkAddListener(timeSlider, ON_SLIDER_CHANGE, 0, onTimeSliderChanged, 0);

	lengthLabel = CreateLabel(lengthLabelText);
	lengthLabel->x = PANEL(timeSlider)->x + PANEL(timeSlider)->width + 10.0f;
	lengthLabel->y = timeLabel->y;
	gkAddChild(controlPanel, lengthLabel);
}
void destroyControlUI()
{
	gkDestroyPanel(lengthLabel);
	gkDestroyPanel(timeLabel);
	DestroySlider(timeSlider);

	DestroySlider(volSlider);
	gkDestroyPanel(volLabel);

	DestroySlider(pitchSlider);
	gkDestroyPanel(pitchLabel);
	gkDestroyPanel(resetPitchBtn);

	gkDestroyPanel(loopOffBtn);
	gkDestroyPanel(loopOnBtn);
	gkDestroyPanel(stopBtn);
	gkDestroyPanel(pauseBtn);
	gkDestroyPanel(playBtn);
	gkDestroyPanel(controlLabel);
	gkDestroyPanel(controlPanel);
}

GK_BOOL onMasterVolChanged(gkEvent* evt, void* p)
{
	float vol;
	GetSliderPos(masterVolSlider, &vol);
	gkSetMasterGain(vol);
	return GK_TRUE;
}

GK_BOOL ControlStaticSound(gkEvent* e, void *p)
{
	LoadStaticSource();
	pauseBtn->visible = GK_FALSE;
	playBtn->visible = GK_TRUE;
	return GK_TRUE;
}

GK_BOOL ControlStreamSound(gkEvent* e, void *p)
{
	LoadStreamSource();
	pauseBtn->visible = GK_FALSE;
	playBtn->visible = GK_TRUE;
	return GK_TRUE;
}

GK_BOOL onSoundStopped(gkEvent* e, void *p)
{
	pauseBtn->visible = GK_FALSE;
	playBtn->visible = GK_TRUE;
	return GK_TRUE;
}

void createUI()
{
	staticLabel = (gkPanel*)CreateLabel("Static sound");
	staticLabel->x = 10.0f;
	staticLabel->y = 10.0f;
	gkAddChild(gkMainPanel, staticLabel);

	loadStaticBtn = (gkPanel*)CreateButton("Load");
	loadStaticBtn->x = staticLabel->x + staticLabel->width + 20.0f;
	loadStaticBtn->y = staticLabel->y;
	gkAddChild(gkMainPanel, loadStaticBtn);
	gkAddListener(loadStaticBtn, GK_ON_MOUSE_UP, 0, LoadStaticSound, 0);

	playStaticBtn = (gkPanel*)CreateButton("Play");
	playStaticBtn->x = loadStaticBtn->x + loadStaticBtn->width + 10.0f;
	playStaticBtn->y = staticLabel->y;
	gkAddChild(gkMainPanel, playStaticBtn);
	gkAddListener(playStaticBtn, GK_ON_MOUSE_UP, 0, PlayStaticSound, 0);

	controlStaticBtn = (gkPanel*)CreateButton("Put in control");
	controlStaticBtn->x = playStaticBtn->x + playStaticBtn->width + 10.0f;
	controlStaticBtn->y = staticLabel->y;
	gkAddChild(gkMainPanel, controlStaticBtn);
	gkAddListener(controlStaticBtn, GK_ON_MOUSE_UP, 0, ControlStaticSound, 0);


	streamLabel = (gkPanel*)CreateLabel("Stream sound");
	streamLabel->x = 10.0f;
	streamLabel->y = 60.0f;
	gkAddChild(gkMainPanel, streamLabel);

	loadStreamBtn = (gkPanel*)CreateButton("Load");
	loadStreamBtn->x = loadStaticBtn->x;
	loadStreamBtn->y = streamLabel->y;
	gkAddChild(gkMainPanel, loadStreamBtn);
	gkAddListener(loadStreamBtn, GK_ON_MOUSE_UP, 0, LoadStreamSound, 0);

	playStreamBtn = (gkPanel*)CreateButton("Play");
	playStreamBtn->x = loadStreamBtn->x + loadStreamBtn->width + 10.0f;
	playStreamBtn->y = streamLabel->y;
	gkAddChild(gkMainPanel, playStreamBtn);
	gkAddListener(playStreamBtn, GK_ON_MOUSE_UP, 0, PlayStreamSound, 0);

	controlStreamBtn = (gkPanel*)CreateButton("Put in control");
	controlStreamBtn->x = playStreamBtn->x + playStreamBtn->width + 10.0f;
	controlStreamBtn->y = streamLabel->y;
	gkAddChild(gkMainPanel, controlStreamBtn);
	gkAddListener(controlStreamBtn, GK_ON_MOUSE_UP, 0, ControlStreamSound, 0);

	//Control

	createControlUI();

	//Master
	masterVolLabel = CreateLabel("Master volume");
	masterVolLabel->x = 10.0f;
	masterVolLabel->y = 400.0f;
	gkAddChild(gkMainPanel, masterVolLabel);

	masterVolSlider = CreateSlider();
	PANEL(masterVolSlider)->x = masterVolLabel->x + masterVolLabel->width + 10.0f;
	PANEL(masterVolSlider)->y = masterVolLabel->y + 4.0f;
	PANEL(masterVolSlider)->width = 270.0f;
	SetSliderPos(masterVolSlider, gkGetMasterGain());
	gkAddChild(gkMainPanel, PANEL(masterVolSlider));
	gkAddListener(masterVolSlider, ON_SLIDER_CHANGE, 0, onMasterVolChanged, 0);
}

void destroyUI()
{
	gkDestroyPanel(staticLabel);
	gkDestroyPanel(loadStaticBtn);
	gkDestroyPanel(playStaticBtn);
	gkDestroyPanel(controlStaticBtn);

	gkDestroyPanel(streamLabel);
	gkDestroyPanel(loadStreamBtn);
	gkDestroyPanel(playStreamBtn);
	gkDestroyPanel(controlStreamBtn);

	destroyControlUI();

	gkDestroyPanel(masterVolLabel);
	DestroySlider(masterVolSlider);
}

GK_BOOL init()
{
	gkSetWindowTitle("Sound player / Audio test");

	gkSetScreenSize(GK_SIZE(470.0f, 450.0f));

	loadAssets();

	CreateSource();
	gkAddListener(GetSoundSource(), GK_ON_SOUND_STOPPED, 0, onSoundStopped, 0);

	createUI();

	/*
		Controls:
			Text Button: Play/Pause, Stop, Loop on/off
			Slider: Volume, Pitch, Song position (time)

		Interface:
			- Load static sound - 1 of 2 (alternate)
			- Play static sound instance
			- Play static sound at CTRL
			- Load streaming sound - 1 of 2 (alternate)
			- Play streaming sound instance
			- Play streaming sound at CTRL
			- CTRL:
				- pitch control
				- volume control
				- offset control + time label
				- play->pause
				- stop
				- loop on/off
			- Master volume
	*/
	return GK_TRUE;
}

void clean()
{
	destroyUI();
	cleanupAssets();
	CleanupAudioControl();
}

GK_APP(init, clean)