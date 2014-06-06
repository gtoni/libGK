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

#ifdef GK_USE_OPENSLES

#include "gkAudioInternal.h"

#include <gk.h>

#include <SLES/OpenSLES.h>

#include <stdio.h>
#include <stdlib.h>

SLObjectItf engineObj;
SLEngineItf engineItf;

SLObjectItf outputMixObj;
SLOutputMixItf outputMixItf;

static void init()
{
	SLresult res;

	res = slCreateEngine(&engineObj, 0, 0, 0, 0, 0);

	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Couldn't create OpenSLES engine object\n");
		return;
	}

	res = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Couldn't Realize OpenSLES engine object\n");
		return;
	}

	res = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engineItf);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Couldn't Retrieve OpenSLES engine interface\n");
		return;
	}

	res = (*engineItf)->CreateOutputMix(engineItf, &outputMixObj, 0,0,0);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Failed to create Output mix\n");
		return;
	}

	res = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Failed to realize Output mix\n");
		return;
	}

	res = (*outputMixObj)->GetInterface(outputMixObj, SL_IID_OUTPUTMIX, &outputMixItf);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Failed to get Output mix interface\n");
		return;
	}
}

static void cleanup()
{
	if (outputMixObj) {
		(*outputMixObj)->Destroy(outputMixObj);
		outputMixObj = 0;
		outputMixItf = 0;
	}

	if (engineObj) {
		(*engineObj)->Destroy(engineObj);
		engineObj = 0;
		engineItf = 0;
	}
}

static uint32_t checkError()
{
	return 0;
}

typedef struct gkAudioBufferData{
	SLDataFormat_PCM pcm;
	unsigned char* buf;
	size_t bufLength;
	int format;
}gkAudioBufferData;

static gkAudioBuffer CreateBuffer(int format, int sampleRate, GK_BOOL streaming)
{
	gkAudioBufferData* data = (gkAudioBufferData*)malloc(sizeof(gkAudioBufferData));

	memset(&data->pcm, 0, sizeof(SLDataFormat_PCM));
	data->pcm.formatType = SL_DATAFORMAT_PCM;
	if (format == GK_AUDIO_FORMAT_MONO8) {
		data->pcm.numChannels = 1;
		data->pcm.bitsPerSample = 8;
		data->pcm.containerSize = 8;
		data->pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
	} else if (format == GK_AUDIO_FORMAT_MONO16) {
		data->pcm.numChannels = 1;
		data->pcm.bitsPerSample = 16;
		data->pcm.containerSize = 16;
		data->pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
	} else if (format == GK_AUDIO_FORMAT_STEREO8) {
		data->pcm.numChannels = 2;
		data->pcm.bitsPerSample = 8;
		data->pcm.containerSize = 16;
		data->pcm.channelMask = SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT;
	} else if (format == GK_AUDIO_FORMAT_STEREO16) {
		data->pcm.numChannels = 2;
		data->pcm.bitsPerSample = 16;
		data->pcm.containerSize = 16;
		data->pcm.channelMask = SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT;
	}
	data->pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	data->pcm.samplesPerSec = sampleRate*1000;	//milliHertz

	data->buf = 0;
	data->bufLength = 0;

	data->format = format;
	return data;
}

static void DestroyBuffer(gkAudioBuffer buffer)
{
	if(buffer->buf)
		free(buffer->buf);
	free(buffer);
}

static uint32_t SetBufferData(gkAudioBuffer buffer, const void* data, size_t dataSize)
{
	void* buf = 0;

	if (dataSize > 0) {
		buf = malloc(dataSize);
		memcpy(buf, data, dataSize);
	}

	if (buffer->buf) 
		free(buffer->buf);

	buffer->bufLength = dataSize;
	buffer->buf = (unsigned char*)buf;

	return checkError();
}

typedef struct gkAudioPlayerData{
	SLObjectItf playerObj;
	SLPlayItf playItf;
	SLVolumeItf volumeItf;
	SLBufferQueueItf queueItf;
	gkAudioPlayerState playState;
	int sampleRateMilliHz;
	int format;
	int queued;
	int volume;
	GK_BOOL looping;
	float pitch;
}gkAudioPlayerData;

static gkAudioPlayer CreatePlayer()
{
	gkAudioPlayerData* data = (gkAudioPlayerData*)malloc(sizeof(gkAudioPlayerData));
	data->playState = GK_AUDIOPLAYER_STOPPED;
	data->playerObj = 0;
	data->sampleRateMilliHz = 0;
	data->format = 0;
	data->queued = 0;

	data->volume = 1.0f;
	data->looping = GK_FALSE;
	return data;
}

static void DestroyPlayer(gkAudioPlayer player)
{
	if (engineObj && player->playerObj) {
		(*player->playerObj)->Destroy(player->playerObj);
	}
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

static void preparePlayer(gkAudioPlayer player, gkAudioBuffer buffer)
{
	SLDataLocator_BufferQueue loc = {
		SL_DATALOCATOR_BUFFERQUEUE, GK_NUM_STREAM_BUFFERS};
	SLDataSource src = {&loc, &buffer->pcm}; /* This IS fine even if buffer is NULL*/
	SLDataLocator_OutputMix outMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
	SLDataSink out = {&outMix, 0};
	SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
	SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	SLresult res;

	if (player->playerObj) {
		if (buffer == 0 || player->format != buffer->format || 
			player->sampleRateMilliHz != buffer->pcm.samplesPerSec) {
			(*player->playerObj)->Destroy(player->playerObj);
			player->playerObj = 0;
			player->playItf = 0;
			player->queueItf = 0;
			player->volumeItf = 0;
		}else {
			/* player is compatible, nothing to do here */
			return;
		}
	}

	if (!buffer) {
		player->format = 0;
		player->sampleRateMilliHz = 0;
		return;
	}

	player->format = buffer->format;
	player->sampleRateMilliHz = buffer->pcm.samplesPerSec;

	res = (*engineItf)->CreateAudioPlayer(engineItf, &player->playerObj, &src, &out, 2, ids, req);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Failed to create audio player: %d\n", (int)res);
		return;
	}
	(*player->playerObj)->Realize(player->playerObj, SL_BOOLEAN_FALSE);
	(*player->playerObj)->GetInterface(player->playerObj, SL_IID_PLAY, &player->playItf);
	(*player->playerObj)->GetInterface(player->playerObj, SL_IID_BUFFERQUEUE, &player->queueItf);
	(*player->playerObj)->GetInterface(player->playerObj, SL_IID_VOLUME, &player->volumeItf);
}

static void PlayerSetBufer(gkAudioPlayer player, gkAudioBuffer buffer)
{
	preparePlayer(player, buffer);

	player->playState = GK_AUDIOPLAYER_STOPPED;
	if (player->queueItf) {
		(*player->queueItf)->Clear(player->queueItf);
	}

	player->queued = 0;
	
	if(buffer)
		(*player->queueItf)->Enqueue(player->queueItf, buffer->buf, buffer->bufLength);
}
static uint32_t PlayerQueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	SLresult res;
	int i;
	if (numBuffers == 0)
		return 0;

	preparePlayer(player, buffers[0]);

	player->queued += numBuffers;

	for (i = 0; i<numBuffers; i++) {
		(*player->queueItf)->Enqueue(player->queueItf, buffers[i]->buf, buffers[i]->bufLength);
	}
	return 0;
}

static uint32_t PlayerUnqueueBuffers(gkAudioPlayer player, int numBuffers, gkAudioBuffer* buffers)
{
	player->queued -= numBuffers;
	return 0;
}
static uint32_t PlayerPlay(gkAudioPlayer player)
{
	SLresult res;

	if (!player->playItf)
		return 0;

	player->playState = GK_AUDIOPLAYER_PLAYING;
	res = (*player->playItf)->SetPlayState(player->playItf, SL_PLAYSTATE_PLAYING);
	if (res != SL_RESULT_SUCCESS) {
		printf("GK [ERROR]: Failed to play sound\n");
	}

	return 0;
}
static uint32_t PlayerPause(gkAudioPlayer player)
{
	if (!player->playItf || player->playState != GK_AUDIOPLAYER_PLAYING)
		return 0;

	player->playState = GK_AUDIOPLAYER_PAUSED;
	(*player->playItf)->SetPlayState(player->playItf, SL_PLAYSTATE_PAUSED);

	return 0;
}
static uint32_t PlayerStop(gkAudioPlayer player)
{
	if (!player->playItf)
		return 0;

	player->playState = GK_AUDIOPLAYER_STOPPED;
	(*player->playItf)->SetPlayState(player->playItf, SL_PLAYSTATE_STOPPED);

	return 0;
}
static gkAudioPlayerState PlayerGetState(gkAudioPlayer player)
{
	if (!player->playItf)
		return player->playState;

	if (player->playState == GK_AUDIOPLAYER_PLAYING) {
		SLBufferQueueState state;
		(*player->queueItf)->GetState(player->queueItf, &state);

		if (state.count == 0) {
			(*player->playItf)->SetPlayState(player->playItf, SL_PLAYSTATE_STOPPED);
			(*player->queueItf)->Clear(player->queueItf);
			player->playState = GK_AUDIOPLAYER_STOPPED;
		}
	}

	return player->playState;
}
static void PlayerGetQueueState(gkAudioPlayer player, int* processed, int* queued)
{
	SLBufferQueueState state;

	if (!player->playerObj) {
		*processed = 0;
		*queued = 0;
		return;
	}

	(*player->queueItf)->GetState(player->queueItf, &state);
	*processed = player->queued - state.count;
	*queued = player->queued;
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