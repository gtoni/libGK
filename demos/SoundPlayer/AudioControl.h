#ifndef _AUDIO_CTRL_H_
#define _AUDIO_CTRL_H_

#include <gk.h>

GK_BOOL LoadStaticSound(gkEvent* e, void*p);
GK_BOOL PlayStaticSound(gkEvent* e, void*p);

GK_BOOL LoadStreamSound(gkEvent* e, void*p);
GK_BOOL PlayStreamSound(gkEvent* e, void*p);

void CreateSource();
void LoadStaticSource();
void LoadStreamSource();
void PlaySource();
void PauseSource();
void StopSource();
gkSoundSource* GetSoundSource();
gkSound* GetSound();

void CleanupAudioControl();

#endif