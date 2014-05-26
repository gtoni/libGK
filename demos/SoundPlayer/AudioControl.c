#include "AudioControl.h"
#include "Assets.h"

gkSound* staticSnd = 0;
gkSound* streamSnd = 0;

GK_BOOL LoadStaticSound(gkEvent* e, void*p)
{
	if (staticSnd) {
		gkDestroySound(staticSnd);
		staticSnd = 0;
	}
	staticSnd = gkLoadSound(GetRandomStaticSound(), GK_SOUND_STATIC);
	return GK_TRUE;
}

GK_BOOL PlayStaticSound(gkEvent* e, void*p)
{
	if (staticSnd)
		gkPlaySound(staticSnd, 0);
	return GK_TRUE;
}

GK_BOOL LoadStreamSound(gkEvent* e, void*p)
{
	if (streamSnd) {
		gkDestroySound(streamSnd);
		streamSnd = 0;
	}
	streamSnd = gkLoadSound(GetRandomStreamSound(), GK_SOUND_STREAM);
	return GK_TRUE;
}

GK_BOOL PlayStreamSound(gkEvent* e, void*p)
{
	if (streamSnd)
		gkPlaySound(streamSnd, 0);
	return GK_TRUE;
}

gkSoundSource* soundSrc;
gkSound* soundSrcContent;

GK_BOOL paused = 0;

void CreateSource()
{
	soundSrc = gkCreateSoundSource();
	soundSrcContent = 0;
}

void PlaySource()
{
	if (paused) {
		gkResumeSound(soundSrc);
		paused = GK_FALSE;
	} else {
		gkPlaySound(soundSrcContent, soundSrc);
	}
}

void PauseSource()
{
	paused = GK_TRUE;
	gkPauseSound(soundSrc);
}

void StopSource()
{
	gkStopSound(soundSrc, GK_TRUE);
}


gkSoundSource* GetSoundSource()
{
	return soundSrc;
}

gkSound* GetSound()
{
	return soundSrcContent;
}

void LoadStaticSource()
{
	if (soundSrcContent) {
		gkDestroySound(soundSrcContent);
		soundSrcContent = 0;
	}
	soundSrcContent = staticSnd;

	staticSnd = 0;
}

void LoadStreamSource()
{
	if (soundSrcContent) {
		gkDestroySound(soundSrcContent);
		soundSrcContent = 0;
	}
	soundSrcContent = streamSnd;
	streamSnd = 0;
}


void CleanupAudioControl()
{
	if (staticSnd) {
		gkDestroySound(staticSnd);
		staticSnd = 0;
	}

	if (streamSnd) {
		gkDestroySound(streamSnd);
		streamSnd = 0;
	}

	if (soundSrcContent) {
		gkDestroySound(soundSrcContent);
		soundSrcContent = 0;
	}

	gkDestroySoundSource(soundSrc);
}