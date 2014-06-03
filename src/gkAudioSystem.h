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

#ifndef _GK_AUDIO_SYSTEM_H_
#define _GK_AUDIO_SYSTEM_H_

#include <gkConfig.h>
#include <gkTypes.h>

typedef struct gkAudioBufferData *gkAudioBuffer;

typedef struct gkAudioPlayerData *gkAudioPlayer;

typedef enum gkAudioPlayerState
{
	GK_AUDIOPLAYER_PLAYING,
	GK_AUDIOPLAYER_PAUSED,
	GK_AUDIOPLAYER_STOPPED
}gkAudioPlayerState;

typedef struct gkAudioSystem
{
	float minVolume;
	float maxVolume;

	GK_METHOD(void, Init, ());
	GK_METHOD(void, Cleanup, ());
	GK_METHOD(uint32_t, CheckError, ());

	GK_METHOD(void, SetVolume, (int masterVol));
	GK_METHOD(int, GetVolume, ());

	GK_METHOD(gkAudioBuffer, CreateBuffer, (int format, int sampleRate, GK_BOOL streaming));
	GK_METHOD(void, DestroyBuffer, (gkAudioBuffer buffer));

	struct Buffer{
		GK_METHOD(uint32_t, SetData, (gkAudioBuffer buffer, 
			const void* data, size_t dataSize));
	}Buffer;

	GK_METHOD(gkAudioPlayer, CreatePlayer, ());
	GK_METHOD(void, DestroyPlayer, (gkAudioPlayer player));

	struct Player{
		GK_METHOD(void, SetPitch, (gkAudioPlayer player, float pitch));
		GK_METHOD(float, GetPitch, (gkAudioPlayer player));
		GK_METHOD(void, SetVolume, (gkAudioPlayer player, int vol));
		GK_METHOD(int, GetVolume, (gkAudioPlayer player));
		GK_METHOD(void, SetLooping, (gkAudioPlayer player, GK_BOOL));
		GK_METHOD(GK_BOOL, IsLooping, (gkAudioPlayer player));
		GK_METHOD(void, SetOffset, (gkAudioPlayer player, float sec));
		GK_METHOD(float, GetOffset, (gkAudioPlayer player));

		GK_METHOD(void, SetBuffer, (gkAudioPlayer player, gkAudioBuffer buffer));

		GK_METHOD(uint32_t, QueueBuffers, (gkAudioPlayer player, 
			int numBuffers, gkAudioBuffer* buffers));
		GK_METHOD(uint32_t, UnqueueBuffers, (gkAudioPlayer player, 
			int numBuffers, gkAudioBuffer* buffers));
		GK_METHOD(void, GetQueueState, (gkAudioPlayer player, 
			int* processed, int* queued));

		GK_METHOD(uint32_t, Play, (gkAudioPlayer player));
		GK_METHOD(uint32_t, Pause, (gkAudioPlayer player));
		GK_METHOD(uint32_t, Stop, (gkAudioPlayer player));
		GK_METHOD(gkAudioPlayerState, GetState, (gkAudioPlayer player));
	}Player;
}gkAudioSystem;


#ifdef __cplusplus
extern "C"{
#endif

gkAudioSystem gkGetAudioSystem();

#ifdef __cplusplus
}
#endif

#endif