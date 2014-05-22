#include "gkAudioSystem.h"

#ifdef GK_USE_DIRECTSOUND

#include "gkAudioInternal.h"

#include <windows.h>
#include <dsound.h>

#include <gk.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")

#include <stdio.h>
#include <stdlib.h>

LPDIRECTSOUND8 lpds;
LPDIRECTSOUNDBUFFER primary;

static void initDSound(HWND wnd)
{
	DSBUFFERDESC primDesc;
	HRESULT hr = DirectSoundCreate8(0, &lpds, 0);
	lpds->SetCooperativeLevel(wnd, DSSCL_PRIORITY);

	primDesc.dwSize = sizeof(DSBUFFERDESC);
	primDesc.dwFlags = DSBCAPS_CTRLVOLUME|DSBCAPS_PRIMARYBUFFER;
	primDesc.dwBufferBytes = 0;
	primDesc.dwReserved = 0;
	primDesc.guid3DAlgorithm = DS3DALG_DEFAULT;
	primDesc.lpwfxFormat = 0;

	hr = lpds->CreateSoundBuffer(&primDesc, &primary, 0);

	primary->SetVolume(0);
}

static void cleanupDSound()
{
	primary->Release();
	lpds->Release();
	lpds = 0;
}

static uint32_t checkError()
{
	return 0;
}

typedef struct gkAudioBufferData{
	WAVEFORMATEX wfx;
	DSBUFFERDESC desc;
	LPDIRECTSOUNDBUFFER8 dsBuffer;
}gkAudioBufferData;

static gkAudioBuffer CreateBuffer(int format, int sampleRate, GK_BOOL streaming)
{
	gkAudioBufferData* data = (gkAudioBufferData*)malloc(sizeof(gkAudioBufferData));

	memset(&data->wfx, 0, sizeof(WAVEFORMATEX));
	data->wfx.wFormatTag = WAVE_FORMAT_PCM;
	if (format == GK_AUDIO_FORMAT_MONO8) {
		data->wfx.nChannels = 1;
		data->wfx.wBitsPerSample = 8;
		data->wfx.nBlockAlign = 1;
	} else if (format == GK_AUDIO_FORMAT_MONO16) {
		data->wfx.nChannels = 1;
		data->wfx.wBitsPerSample = 16;
		data->wfx.nBlockAlign = 2;
	} else if (format == GK_AUDIO_FORMAT_STEREO8) {
		data->wfx.nChannels = 2;
		data->wfx.wBitsPerSample = 8;
		data->wfx.nBlockAlign = 2;
	} else if (format == GK_AUDIO_FORMAT_STEREO16) {
		data->wfx.nChannels = 2;
		data->wfx.wBitsPerSample = 16;
		data->wfx.nBlockAlign = 4;
	}
	data->wfx.nSamplesPerSec = sampleRate;
	data->wfx.nAvgBytesPerSec = sampleRate * data->wfx.nBlockAlign;

	memset(&data->desc, 0, sizeof(DSBUFFERDESC));
	data->desc.dwSize = sizeof(DSBUFFERDESC);
	data->desc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY |
		DSBCAPS_GLOBALFOCUS;
	data->desc.lpwfxFormat = &data->wfx;
	data->desc.dwBufferBytes = 0;

	data->dsBuffer = 0;

	return data;
}

static GK_BOOL createDSBuffer(gkAudioBuffer buffer, size_t size)
{
	LPDIRECTSOUNDBUFFER pds = 0;
	HRESULT hr;

	buffer->desc.dwBufferBytes = size;

	hr = lpds->CreateSoundBuffer(&buffer->desc, &pds, 0);
	if (SUCCEEDED(hr)) {
		hr = pds->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*) &buffer->dsBuffer);
		pds->Release();
	}
	return SUCCEEDED(hr);
}

static void DestroyBuffer(gkAudioBuffer buffer)
{
	if (buffer->dsBuffer && lpds) {
		buffer->dsBuffer->Release();
	}
	free(buffer);
}

static uint32_t SetBufferData(gkAudioBuffer buffer, const void* data, size_t dataSize)
{
	void* dst;
	DWORD dstLength;

	/* Check if buffer exist and is the right size */

	if (buffer->dsBuffer && buffer->desc.dwBufferBytes != dataSize) {
		buffer->dsBuffer->Release();
		buffer->dsBuffer = 0;
	}

	if (dataSize == 0)
		return 0;

	if (buffer->dsBuffer == 0)
		createDSBuffer(buffer, dataSize);

	if (buffer->dsBuffer == 0)
		return 1;

	/* Fill buffer */

	if (DS_OK == buffer->dsBuffer->Lock(0, 0, &dst, &dstLength, 0, 0, DSBLOCK_ENTIREBUFFER)) {
		memcpy(dst, data, dstLength);
		buffer->dsBuffer->Unlock(dst, dstLength, 0, 0);
	} else {
		return 1;	//some error
	}

	return checkError();
}

typedef struct gkAudioPlayerData{
	LPDIRECTSOUNDBUFFER8 playBuffer;
	int volume;
	DWORD sampleRate;
	GK_BOOL looping;
}gkAudioPlayerData;

static gkAudioPlayer CreatePlayer()
{
	gkAudioPlayerData* data = (gkAudioPlayerData*)malloc(sizeof(gkAudioPlayerData));
	data->playBuffer = 0;
	data->volume = 1.0f;
	data->sampleRate = 0;
	data->looping = GK_FALSE;
	return data;
}

static void DestroyPlayer(gkAudioPlayer player)
{
	if (player->playBuffer && lpds) {
		player->playBuffer->Release();
	}
	free(player);
}

static void SetPlayerPitch(gkAudioPlayer player, float pitch)
{
	player->playBuffer->SetFrequency(pitch * player->sampleRate);
}
static float GetPlayerPitch(gkAudioPlayer player)
{
	DWORD freq;
	player->playBuffer->GetFrequency(&freq);
	return ((float)freq)/((float)player->sampleRate);
}
static void SetPlayerVolume(gkAudioPlayer player, int vol)
{
	player->volume = vol;
	if (player->playBuffer) {
		player->playBuffer->SetVolume(vol);
	}
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
	LPDIRECTSOUNDBUFFER buf;
	HRESULT hr;

	if (player->playBuffer) {
		player->playBuffer->Release();
		player->playBuffer = 0;
	}

	if (buffer == 0)
		return;

	player->sampleRate = buffer->wfx.nSamplesPerSec;

	hr = lpds->DuplicateSoundBuffer(buffer->dsBuffer, &buf);
	if (SUCCEEDED(hr)) {
		hr = buf->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&player->playBuffer);
		player->playBuffer->SetVolume(player->volume);
	}
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
	if (player->playBuffer) {
		player->playBuffer->SetCurrentPosition(0);
		player->playBuffer->Play(0, 0, player->looping ? DSBPLAY_LOOPING : 0);
	}
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
	if (player->playBuffer) {
		DWORD status;
		player->playBuffer->GetStatus(&status);
		if (status & DSBSTATUS_PLAYING) 
			return GK_AUDIOPLAYER_PLAYING;
		//may be paused
	}
	return GK_AUDIOPLAYER_STOPPED;
}
static void PlayerGetQueueState(gkAudioPlayer player, int* processed, int* queued)
{
	*processed = 0;
	*queued = 0;
}

static void SetMasterVolume(int vol)
{
	primary->SetVolume(vol);
}

static int GetMasterVolume()
{
	LONG vol;
	primary->GetVolume(&vol);
	return vol;
}

extern "C"{

	#include "gk_internal.h"

	static void init()
	{
		initDSound(gkWindow);
	}

	gkAudioSystem gkGetAudioSystem()
	{
		gkAudioSystem audioSys;

		audioSys.minVolume = -5000.0f;
		audioSys.maxVolume = 0.0f;

		audioSys.Init = init;
		audioSys.Cleanup = cleanupDSound;
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
}

#endif