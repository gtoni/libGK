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

#ifdef GK_USE_XAUDIO2

#include <math.h>

/* Amplitude Ratio (volume) to Decibel */
static int VolumeToDb(float v)
{
	if (v == 0)
		return -10000;
	return (int)((20.0f*log10f(v))*100.0f);
}

/* Decibel to Amplitude Ratio (volume)*/
static float DbToVolume(float db)
{
	db /= 100.0f;
	if (db <= -100.0f)
		return 0.0f;
	return powf(10.0f, db/20.0f);
}

#include "gkAudioInternal.h"

#include <gk.h>

#include <stdio.h>
#include <stdlib.h>

#include <XAudio2.h>

#pragma comment(lib, "Xaudio2.lib")

IXAudio2* engine;
IXAudio2MasteringVoice* master;

static void init()
{
	HRESULT hr = XAudio2Create(&engine, 0, XAUDIO2_DEFAULT_PROCESSOR);

	if (FAILED(hr)) {
		printf("Couldn't initialize XAudio2\n");
		return;
	}

	hr = engine->CreateMasteringVoice(&master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, 0);

	if (FAILED(hr)) {
		printf("Couldn't create mastering voice\n");
		return;
	}
}

static void cleanup()
{
	engine->Release();
	engine = 0;
}

static uint32_t checkError()
{
	return 0;
}

typedef struct gkAudioBufferData{
	WAVEFORMATEX wfx;
	XAUDIO2_BUFFER xbuf;
	int format;
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

	memset(&data->xbuf, 0, sizeof(XAUDIO2_BUFFER));
	data->xbuf.Flags = streaming ? 0 : XAUDIO2_END_OF_STREAM;

	data->format = format;

	return data;
}

static void DestroyBuffer(gkAudioBuffer buffer)
{
	if (buffer->xbuf.pAudioData) 
		free((void*)buffer->xbuf.pAudioData);

	free(buffer);
}

static uint32_t SetBufferData(gkAudioBuffer buffer, const void* data, size_t dataSize)
{
	void* buf = 0;

	if (dataSize > 0) {
		buf = malloc(dataSize);
		memcpy(buf, data, dataSize);
	}

	if (buffer->xbuf.pAudioData) 
		free((void*)buffer->xbuf.pAudioData);

	buffer->xbuf.AudioBytes = dataSize;
	buffer->xbuf.pAudioData = (BYTE*)buf;
	return checkError();
}

typedef struct gkAudioPlayerData{
	IXAudio2SourceVoice* source;
	int format;
	int volume;
	float pitch;
	int queued;
	gkAudioPlayerState playState;
	uint32_t samplesPerSec;
	uint32_t totalSamples;
	uint32_t startSamplesPlayed;
	XAUDIO2_BUFFER localXBuf;
}gkAudioPlayerData;

static gkAudioPlayer CreatePlayer()
{
	gkAudioPlayerData* data = (gkAudioPlayerData*)malloc(sizeof(gkAudioPlayerData));
	data->source = 0;
	data->format = 0;
	data->volume = VolumeToDb(1.0f);
	data->pitch = 1.0f;
	data->queued = 0;
	data->playState = GK_AUDIOPLAYER_STOPPED;
	data->totalSamples = 1;
	data->startSamplesPlayed = 0;
	memset(&data->localXBuf, 0, sizeof(XAUDIO2_BUFFER));
	return data;
}

static void DestroyPlayer(gkAudioPlayer player)
{
	if (engine && player->source) {
		player->source->DestroyVoice();
		player->source = 0;
	}
	free(player);
}

static void SetPlayerPitch(gkAudioPlayer player, float pitch)
{
	player->pitch = pitch;
	if (player->source)
		player->source->SetFrequencyRatio(pitch, XAUDIO2_COMMIT_NOW);
}
static float GetPlayerPitch(gkAudioPlayer player)
{
	if (player->source) {
		player->source->GetFrequencyRatio(&player->pitch);
	}
	return player->pitch;
}
static void SetPlayerVolume(gkAudioPlayer player, int vol)
{
	player->volume = vol;
	if (player->source)
		player->source->SetVolume(DbToVolume((float)vol));
}
static int GetPlayerVolume(gkAudioPlayer player)
{
	float vol;
	if (player->source) {
		player->source->GetVolume(&vol);
		return VolumeToDb(vol);
	}
	return player->volume;
}

static uint32_t GetSamplesPlayed(gkAudioPlayer player, GK_BOOL adjust = GK_TRUE)
{
	uint32_t samplesPlayed;
	XAUDIO2_VOICE_STATE state;
	player->source->GetState(&state);
	samplesPlayed = (uint32_t)state.SamplesPlayed + 
		(adjust ? (player->localXBuf.PlayBegin - player->startSamplesPlayed) : 0);
	return samplesPlayed % player->totalSamples;
}

static void SetPlayerLooping(gkAudioPlayer player, GK_BOOL looping)
{
	player->localXBuf.LoopCount = looping ? XAUDIO2_LOOP_INFINITE : 0;

	if (player->queued == 0 && player->playState != GK_AUDIOPLAYER_STOPPED) {
		player->localXBuf.PlayBegin = GetSamplesPlayed(player);
		player->source->Stop(0, XAUDIO2_COMMIT_NOW);
		player->source->FlushSourceBuffers();
		player->source->SubmitSourceBuffer(&player->localXBuf);
		if (player->playState == GK_AUDIOPLAYER_PLAYING) {
			player->source->Start(0, XAUDIO2_COMMIT_NOW);
			player->startSamplesPlayed = GetSamplesPlayed(player, GK_FALSE);
		}
	}
}
static GK_BOOL IsPlayerLooping(gkAudioPlayer player)
{
	return player->localXBuf.LoopCount == XAUDIO2_LOOP_INFINITE;
}
static void SetPlayerOffset(gkAudioPlayer player, float sec)
{
	player->localXBuf.PlayBegin = 
		(uint32_t)(sec * (float)player->samplesPerSec) % player->totalSamples;

	if (player->queued == 0 && player->playState != GK_AUDIOPLAYER_STOPPED) {
		player->source->Stop(0, XAUDIO2_COMMIT_NOW);
		player->source->FlushSourceBuffers();
		player->source->SubmitSourceBuffer(&player->localXBuf);
		if (player->playState == GK_AUDIOPLAYER_PLAYING) {
			player->source->Start(0, XAUDIO2_COMMIT_NOW);
		}
		player->startSamplesPlayed = GetSamplesPlayed(player, GK_FALSE);
	}
}
static float GetPlayerOffset(gkAudioPlayer player)
{
	if (player->source) {
		return (float)GetSamplesPlayed(player)/(float)player->samplesPerSec;
	}
	return 0.0f;
}

static void PlayerSetBufer(gkAudioPlayer player, gkAudioBuffer buffer)
{
	HRESULT hr;

	if (player->source != 0) {
		if (buffer == 0 || player->format != buffer->format) {
			player->source->DestroyVoice();
			player->source = 0;
		} else if (player->queued != 0)
			player->source->FlushSourceBuffers();
	}

	player->queued = 0;
	player->localXBuf.PlayBegin = 0;

	if (buffer == 0)
		return;

	if (player->source == 0) {
		hr = engine->CreateSourceVoice(&player->source, &buffer->wfx, 
			0, XAUDIO2_DEFAULT_FREQ_RATIO, 0);
		if (SUCCEEDED(hr)) {
			player->format = buffer->format;
			player->samplesPerSec = buffer->wfx.nSamplesPerSec;
			player->totalSamples = buffer->xbuf.AudioBytes/buffer->wfx.nBlockAlign;
			player->localXBuf.pAudioData = buffer->xbuf.pAudioData;
			player->localXBuf.AudioBytes = buffer->xbuf.AudioBytes;
			player->localXBuf.Flags = buffer->xbuf.Flags;
			player->source->SubmitSourceBuffer(&player->localXBuf, 0);
		}
	}

}
static uint32_t PlayerQueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	HRESULT hr;
	int i;

	if (numBuffers == 0)
		return 0;

	if (player->source != 0) {
		if (player->format != buffers[0]->format) {
			player->source->DestroyVoice();
			player->source = 0;
			player->queued = 0;
		}
	}

	player->localXBuf.PlayBegin = 0;

	if (player->source == 0) {
		hr = engine->CreateSourceVoice(&player->source, &buffers[0]->wfx, 
			0, XAUDIO2_DEFAULT_FREQ_RATIO, 0);
		if (SUCCEEDED(hr)) {
			player->format = buffers[0]->format;
			player->samplesPerSec = buffers[0]->wfx.nSamplesPerSec;
		}
	}

	player->queued += numBuffers;
	
	for (i = 0; i<numBuffers; i++)
		player->source->SubmitSourceBuffer(&buffers[i]->xbuf);

	return 0;
}

static uint32_t PlayerUnqueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	player->queued -= numBuffers;
	return 0;
}
static uint32_t PlayerPlay(gkAudioPlayer player)
{
	player->playState = GK_AUDIOPLAYER_PLAYING;
	if (player->source) {
		player->source->SetVolume(DbToVolume((float)player->volume), XAUDIO2_COMMIT_NOW);
		player->source->SetFrequencyRatio(player->pitch, XAUDIO2_COMMIT_NOW);
		player->source->Start(0, XAUDIO2_COMMIT_NOW);
		player->startSamplesPlayed = GetSamplesPlayed(player, GK_FALSE);
	}

	return checkError();
}
static uint32_t PlayerPause(gkAudioPlayer player)
{
	player->playState = GK_AUDIOPLAYER_PAUSED;
	if (player->source) {
		player->source->Stop();
	}
	return 0;
}
static uint32_t PlayerStop(gkAudioPlayer player)
{
	player->playState = GK_AUDIOPLAYER_STOPPED;
	if (player->source) {
		player->source->Stop();
		player->source->FlushSourceBuffers();
		player->queued = 0;
	}
	return 0;
}
static gkAudioPlayerState PlayerGetState(gkAudioPlayer player)
{
	XAUDIO2_VOICE_STATE state;

	if (player->source && player->playState == GK_AUDIOPLAYER_PLAYING) {
		player->source->GetState(&state);
		if (state.BuffersQueued == 0) {
			player->source->Stop();
			player->playState = GK_AUDIOPLAYER_STOPPED;
		}
	}

	return player->playState;
}
static void PlayerGetQueueState(gkAudioPlayer player, int* processed, int* queued)
{
	XAUDIO2_VOICE_STATE state;

	if (player->source) {
		player->source->GetState(&state);
		*processed = player->queued - state.BuffersQueued;
		*queued = player->queued;
	} else {
		*processed = 0;
		*queued = 0;
	}
}

static void SetMasterVolume(int vol)
{
	master->SetVolume(DbToVolume((float)vol));
}

static int GetMasterVolume()
{
	float vol;
	master->GetVolume(&vol);
	return VolumeToDb(vol);
}

#ifdef __cplusplus
extern "C"{
#endif

	gkAudioSystem gkGetAudioSystem()
	{
		gkAudioSystem audioSys;

		audioSys.minVolume = -6000.0f;
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