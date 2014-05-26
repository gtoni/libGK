#include "gkAudioSystem.h"

#ifdef GK_USE_NOAUDIO

#include "gkAudioInternal.h"

#include <gk.h>

#include <stdio.h>
#include <stdlib.h>

static void init()
{
}

static void cleanup()
{
}

static uint32_t checkError()
{
	return 0;
}

typedef struct gkAudioBufferData{
	int dummy;
}gkAudioBufferData;

static gkAudioBuffer CreateBuffer(int format, int sampleRate, GK_BOOL streaming)
{
	gkAudioBufferData* data = (gkAudioBufferData*)malloc(sizeof(gkAudioBufferData));

	return data;
}

static void DestroyBuffer(gkAudioBuffer buffer)
{
	free(buffer);
}

static uint32_t SetBufferData(gkAudioBuffer buffer, const void* data, size_t dataSize)
{
	return checkError();
}

typedef struct gkAudioPlayerData{
	int volume;
	int sampleRate;
	GK_BOOL looping;
	float pitch;
}gkAudioPlayerData;

static gkAudioPlayer CreatePlayer()
{
	gkAudioPlayerData* data = (gkAudioPlayerData*)malloc(sizeof(gkAudioPlayerData));
	data->volume = 1.0f;
	data->sampleRate = 0;
	data->looping = GK_FALSE;
	return data;
}

static void DestroyPlayer(gkAudioPlayer player)
{
	free(player);
}

static void SetPlayerPitch(gkAudioPlayer player, float pitch)
{
	player->pitch = pitch;
}
static float GetPlayerPitch(gkAudioPlayer player)
{
	return player->pitch;
}
static void SetPlayerVolume(gkAudioPlayer player, int vol)
{
	player->volume = vol;
}
static int GetPlayerVolume(gkAudioPlayer player)
{
	return player->volume;
}
static void SetPlayerLooping(gkAudioPlayer player, GK_BOOL looping)
{
	player->looping = looping;
}
static GK_BOOL IsPlayerLooping(gkAudioPlayer player)
{
	return player->looping;
}
static void SetPlayerOffset(gkAudioPlayer player, float sec)
{
}
static float GetPlayerOffset(gkAudioPlayer player)
{
	return 0.0f;
}

static void PlayerSetBufer(gkAudioPlayer player, gkAudioBuffer buffer)
{
}
static uint32_t PlayerQueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	return 0;
}

static uint32_t PlayerUnqueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	return 0;
}
static uint32_t PlayerPlay(gkAudioPlayer player)
{
	return 0;
}
static uint32_t PlayerPause(gkAudioPlayer player)
{
	return 0;
}
static uint32_t PlayerStop(gkAudioPlayer player)
{
	return 0;
}
static gkAudioPlayerState PlayerGetState(gkAudioPlayer player)
{
	return GK_AUDIOPLAYER_STOPPED;
}
static void PlayerGetQueueState(gkAudioPlayer player, int* processed, int* queued)
{
	*processed = 0;
	*queued = 0;
}

static int masterVol;

static void SetMasterVolume(int vol)
{
	masterVol = vol;
}

static int GetMasterVolume()
{
	return masterVol;
}

#ifdef __cplusplus
extern "C"{
#endif

	gkAudioSystem gkGetAudioSystem()
	{
		gkAudioSystem audioSys;

		audioSys.minVolume = -5000.0f;
		audioSys.maxVolume = 0.0f;

		audioSys.Init = init;
		audioSys.Cleanup = cleanup;
		audioSys.CheckError = checkError;

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
#ifdef __cplusplus
}
#endif

#endif