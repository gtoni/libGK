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

#include "gkAudioSystem.h"

#ifdef GK_USE_OPENAL

#pragma comment(lib,"OpenAL32.lib")

#include <AL/al.h>
#include <AL/alc.h>

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

/* Amplitude Ratio (volume) to Decibel */
static float VolumeToDb(float v)
{
	if (v == 0)
		return -100.0f;
	return 20.0f*log10f(v);
}

/* Decibel to Amplitude Ratio (volume)*/
static float DbToVolume(float db)
{
	if (db <= -100.0f)
		return 0.0f;
	return powf(10.0f, db/20.0f);
}

#define STR(x)	#x
#define TOSTRING(x)	STR(x)
#define CHECK_ERROR(fun)	checkError(STR(fun)" at "TOSTRING(__LINE__))

static uint32_t checkError(char* func)
{
	int err; 
	if((err = alGetError()) != AL_NO_ERROR) 
		printf("GK [ERROR]: in %s -  AL Error: %X\n", func, err);
	return err;
}

static uint32_t checkErrorNoArg()
{
	return checkError("out");
}


static ALCdevice* device;
static ALCcontext* ctx;

static void initOpenAL()
{
	int major, minor;
	alcGetIntegerv(NULL, ALC_MAJOR_VERSION, 1, &major);
	alcGetIntegerv(NULL, ALC_MAJOR_VERSION, 1, &minor);
	
	printf("GK [INFO]: ALC version: %i.%i\n", major, minor);

	if (device = alcOpenDevice(0)) {
		ctx = alcCreateContext(device, NULL);
		if (ctx) {
			alcMakeContextCurrent(ctx);
		} else {
			alcCloseDevice(device);
			device = 0;
			printf("GK [ERROR]: Failed to create audio context");
		}
	} else {
		ctx = 0;
		printf("GK [ERROR]: OpenAL: Failed to open device\n");
	}
}

static void cleanupOpenAL()
{
	if (ctx)
		alcDestroyContext(ctx);
	if (device)
		alcCloseDevice(device);
}

typedef struct gkAudioBufferData{
	ALuint alBuffer;
	int format;
	int sampleRate;
}gkAudioBufferData;

static gkAudioBuffer CreateBuffer(int format, int sampleRate, GK_BOOL streaming)
{
	gkAudioBufferData* data = (gkAudioBufferData*)malloc(sizeof(gkAudioBufferData));
	data->format = format;
	data->sampleRate = sampleRate;
	alGenBuffers(1, &data->alBuffer);
	return data;
}

static void DestroyBuffer(gkAudioBuffer buffer)
{
	alDeleteBuffers(1, &buffer->alBuffer);
	free(buffer);
}

static uint32_t SetBufferData(gkAudioBuffer buffer, const void* data, size_t dataSize)
{
	alBufferData(buffer->alBuffer, buffer->format, data, dataSize, buffer->sampleRate);
	return CHECK_ERROR(alBufferData);
}

typedef struct gkAudioPlayerData{
	ALuint alSource;
}gkAudioPlayerData;

static gkAudioPlayer CreatePlayer()
{
	gkAudioPlayerData* data = (gkAudioPlayerData*)malloc(sizeof(gkAudioPlayerData));
	alGenSources(1, &data->alSource);
	alSourcei(data->alSource, AL_SOURCE_RELATIVE, 1);
	alSource3f(data->alSource, AL_POSITION, 0,0,0);
	CHECK_ERROR(CreatePlayer);
	return data;
}

static void DestroyPlayer(gkAudioPlayer player)
{
	alDeleteSources(1, &player->alSource);
	free(player);
}

static void SetPlayerPitch(gkAudioPlayer player, float pitch)
{
	alSourcef(player->alSource, AL_PITCH, pitch);
}
static float GetPlayerPitch(gkAudioPlayer player)
{
	float pitch;
	alGetSourcef(player->alSource, AL_PITCH, &pitch);
	return pitch;
}
static void SetPlayerVolume(gkAudioPlayer player, int vol)
{
	alSourcef(player->alSource, AL_GAIN, DbToVolume(((float)vol)/100.0f));
	CHECK_ERROR(alSourcef);
}
static int GetPlayerVolume(gkAudioPlayer player)
{
	float vol;
	alGetSourcef(player->alSource, AL_GAIN, &vol);
	return (int)(VolumeToDb(vol)*100.0f);
}
static void SetPlayerLooping(gkAudioPlayer player, GK_BOOL looping)
{
    int type = AL_STATIC;
#ifndef GK_PLATFORM_WEB
	alGetSourcei(player->alSource, AL_SOURCE_TYPE, &type);
#endif
    alSourcei(player->alSource, AL_LOOPING, (type == AL_STATIC) && looping);
	CHECK_ERROR(alGetSourcei);
}
static GK_BOOL IsPlayerLooping(gkAudioPlayer player)
{
	ALint looping;
	alGetSourcei(player->alSource, AL_LOOPING, &looping);
	return looping;
}
static void SetPlayerOffset(gkAudioPlayer player, float sec)
{
#ifndef GK_PLATFORM_WEB
	alSourcef(player->alSource, AL_SEC_OFFSET, sec);
#endif
}
static float GetPlayerOffset(gkAudioPlayer player)
{
	float sec = 0.0f;
#ifndef GK_PLATFORM_WEB
	alGetSourcef(player->alSource, AL_SEC_OFFSET, &sec);
#endif
	return sec;
}
static void PlayerSetBufer(gkAudioPlayer player, gkAudioBuffer buffer)
{
	alSourcei(player->alSource, AL_BUFFER, buffer ? buffer->alBuffer : 0);
}
static uint32_t PlayerQueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	int i;
	ALint srcId = player->alSource;
	for (i = 0; i < numBuffers; i++) 
		alSourceQueueBuffers(srcId, 1, &(buffers[i]->alBuffer));
	return CHECK_ERROR(alSourceQueueBuffers);
}

static uint32_t PlayerUnqueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	int i;
	ALint srcId = player->alSource;
	for (i = 0; i < numBuffers; i++) 
		alSourceUnqueueBuffers(srcId, 1, &(buffers[i]->alBuffer));
	return CHECK_ERROR(alSourceUnqueueBuffers);
}
static uint32_t PlayerPlay(gkAudioPlayer player)
{
	alSourcePlay(player->alSource);
	return CHECK_ERROR(alSourcePlay);
}
static uint32_t PlayerPause(gkAudioPlayer player)
{
	alSourcePause(player->alSource);
	return CHECK_ERROR(alSourcePause);
}
static uint32_t PlayerStop(gkAudioPlayer player)
{
	alSourceStop(player->alSource);
	alSourcei(player->alSource, AL_BUFFER, 0);
	return CHECK_ERROR(alSourcei);
}
static gkAudioPlayerState PlayerGetState(gkAudioPlayer player)
{
	ALint alState;
	alGetSourcei(player->alSource, AL_SOURCE_STATE, &alState);

	switch (alState) {
	case AL_PAUSED:
		return GK_AUDIOPLAYER_PAUSED;
	case AL_PLAYING:
		return GK_AUDIOPLAYER_PLAYING;
	case AL_STOPPED:
		return GK_AUDIOPLAYER_STOPPED;
	}
	return GK_AUDIOPLAYER_STOPPED;
}
static void PlayerGetQueueState(gkAudioPlayer player, int* processed, int* queued)
{
	alGetSourcei(player->alSource, AL_BUFFERS_PROCESSED, processed);
	alGetSourcei(player->alSource, AL_BUFFERS_QUEUED, queued);
}

static void SetMasterVolume(int db)
{
	float r = DbToVolume(((float)db)/100.0f);
	alListenerf(AL_GAIN, r);
}
static int GetMasterVolume()
{
	float vol;
	alGetListenerf(AL_GAIN, &vol);
	return (int)(VolumeToDb(vol)*100.0f);
}

gkAudioSystem gkGetAudioSystem()
{
	gkAudioSystem audioSys;

	audioSys.minVolume = -6000.0f;
	audioSys.maxVolume = 0.0f;

	audioSys.Init = initOpenAL;
	audioSys.Cleanup = cleanupOpenAL;
	audioSys.CheckError = checkErrorNoArg;
	audioSys.CreateBuffer = CreateBuffer;
	audioSys.DestroyBuffer = DestroyBuffer;
	audioSys.Buffer.SetData = SetBufferData;

	audioSys.CreatePlayer = CreatePlayer;
	audioSys.DestroyPlayer = DestroyPlayer;

	audioSys.Player.SetPitch = SetPlayerPitch;
	audioSys.Player.GetPitch = GetPlayerPitch;
	audioSys.Player.SetVolume = SetPlayerVolume;
	audioSys.Player.GetVolume = GetPlayerVolume;
	audioSys.Player.SetLooping = SetPlayerLooping;
	audioSys.Player.IsLooping = IsPlayerLooping;
	audioSys.Player.SetOffset = SetPlayerOffset;
	audioSys.Player.GetOffset = GetPlayerOffset;
	audioSys.Player.SetBuffer = PlayerSetBufer;
	audioSys.Player.QueueBuffers = PlayerQueueBuffers;
	audioSys.Player.UnqueueBuffers = PlayerUnqueueBuffers;
	audioSys.Player.Play = PlayerPlay;
	audioSys.Player.Pause = PlayerPause;
	audioSys.Player.Stop = PlayerStop;
	audioSys.Player.GetState = PlayerGetState;
	audioSys.Player.GetQueueState = PlayerGetQueueState;

	audioSys.SetVolume = SetMasterVolume;
	audioSys.GetVolume = GetMasterVolume;
	return audioSys;
}

#endif