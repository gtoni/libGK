/* Copyright (c) 2014 Toni Georgiev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _GK_AUDIO_H_
#define _GK_AUDIO_H_

#include <gkTypes.h>
#include <gkEvent.h>

/************************************
	Audio

	Types and functions for audio and sounds.
*/

#ifdef GK_INTERNAL
#include "gkAudioInternal.h"
#endif

typedef struct gkSound
{
    float length;
    GK_BOOL seekable;
#ifdef GK_INTERNAL
    struct gkSoundInternal internal;
#endif
}gkSound;


#define GK_SOUND_STATE_IDLE         0
#define GK_SOUND_STATE_PLAYING      1
#define GK_SOUND_STATE_PAUSED       2

typedef struct gkSoundSource
{
    gkDispatcher dispatcher;

GK_READONLY uint8_t state;
GK_READONLY gkSound* sound;
#ifdef GK_INTERNAL
    struct gkSoundSourceInternal internal;
#endif
}gkSoundSource;


#define GK_SOUND_EVENT_BASE    150
#define GK_ON_SOUND_STOPPED     (GK_SOUND_EVENT_BASE + 1)

#define GK_SOUND_STATIC     0x01
#define GK_SOUND_STREAM     0x02

gkSound* gkLoadSound(char* filename, int flags);
void gkDestroySound(gkSound* sound);

gkSoundSource* gkCreateSoundSource();
void gkDestroySoundSource(gkSoundSource* source);

gkSoundSource* gkPlaySound(gkSound* sound, gkSoundSource* source);
void gkPauseSound(gkSoundSource* soundSource);
void gkResumeSound(gkSoundSource* soundSource);
void gkStopSound(gkSoundSource* soundSource, GK_BOOL dispatchStopEvent);

void gkSetSoundGain(gkSoundSource* soundSource, float gain);
float gkGetSoundGain(gkSoundSource* soundSource);

void gkSetSoundPitch(gkSoundSource* soundSource, float pitch);
float gkGetSoundPitch(gkSoundSource* soundSource);

void gkSetSoundLooping(gkSoundSource* soundSource, GK_BOOL looping);
GK_BOOL gkIsSoundLooping(gkSoundSource* soundSource);

void gkSetSoundOffset(gkSoundSource* soundSource, float seconds);
float gkGetSoundOffset(gkSoundSource* soundSource);

void gkSetMasterGain(float gain);
float gkGetMasterGain();

#endif