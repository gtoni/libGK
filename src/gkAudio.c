/* Copyright (c) 2012 Toni Georgiev
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

#define GK_INTERNAL

#include "gk.h"
#include "gk_internal.h"

#include "gkAudioSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static gkAudioSystem Audio;

/***
    Utilities
*/

/* Those were good for OpenAL only */
//#define VOLUME(x) ((expf(5.757f*(x))*3.1623e-3f)*(x<0.1f?x*10.0f:1.0f))	//For 50dB range
//#define VOLUME(x) ((expf(6.908f*(x))/1000.0f)*(x<0.1f?x*10.0f:1.0f))		//For 60dB range
//#define VOLUME(x) ((expf(8.059f*(x))*3.1623e-4f)*(x<0.1f?x*10.0f:1.0f))	//For 70dB range

static int volToDb(float vol)
{
	int db = (int)((Audio.minVolume + vol*(Audio.maxVolume - Audio.minVolume)));
	if (vol<0.05f)
		db = db - (int)(((0.05f - vol)/0.05f)*4000.0f);
	return db;
}

static float dbToVol(int db)
{
	if (db > Audio.minVolume) 
		return ((float)db - Audio.minVolume)/(Audio.maxVolume - Audio.minVolume);
	return 0.0f;
}

struct gkSoundNode
{
    gkSoundSource* source;
    struct gkSoundNode* prev, *next;
};

static struct gkSoundNode* soundNodes = 0, *lastSoundNode = 0;

static void addSoundNode(gkSoundSource* source)
{
	struct gkSoundNode* node = (struct gkSoundNode*)malloc(sizeof(struct gkSoundNode));
	node->source = source;
	node->prev = lastSoundNode;
	node->next = 0;

	if(lastSoundNode != 0)
		lastSoundNode->next = node;
	else 
		soundNodes = node;

	lastSoundNode = node;
}

static struct gkSoundNode* findSoundNode(gkSoundSource* source)
{
	struct gkSoundNode* node;
	for (node = soundNodes; node; node = node->next) {
		if(node->source == source)
			return node;
	}
	return 0;
}

static void removeSoundNode(struct gkSoundNode* node)
{
	if (node->next == 0) 
		lastSoundNode = node->prev;
	if (node->prev) {
		node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;
	} else {
		soundNodes = node->next;
		if (node->next)
			node->next->prev = 0;
	}
	free(node);
}

static int fillBuffer(gkAudioBuffer buffer, gkSound* sound)
{
	char buf[GK_STREAM_BUFFER_SIZE];
	gkAudioStream* stream = sound->internal.stream;
	gkAudioStreamInfo* info = &sound->internal.streamInfo;
	size_t readBytes = stream->read(stream, buf, GK_STREAM_BUFFER_SIZE);

	if (readBytes > 0) {
		Audio.Buffer.SetData(buffer, buf, readBytes);
	}

	return readBytes;
}

static int fillBuffers(gkAudioBuffer* buffers, int numToFill, gkSound* sound)
{
	int i = 0;

	while ((i < numToFill) && (fillBuffer(buffers[i], sound) > 0))
		i++;

	return i;
}

static void fillQueue(gkSoundSource* source, int numBuffers, gkAudioBuffer* buffers)
{
	gkSound* sound = source->sound;
	gkAudioStream* stream = sound->internal.stream;

	int filled = fillBuffers(buffers, numBuffers, sound);

	if (filled == 0 && stream->eof(stream) && source->internal.looping) {
		stream->seek(stream, 0, SEEK_SET);
		filled = fillBuffers(buffers, numBuffers, sound);
	}

	Audio.Player.QueueBuffers(source->internal.player, filled, buffers);
}

static void updateTimeOffset(gkSoundSource* source, uint64_t currentTime)
{
	if (source->sound == 0)
		return;
	if (source->sound->internal.flags & GK_SOUND_STREAM) {
		float pitch;
		uint64_t elapsed = currentTime - source->internal.lastOffset;
		uint64_t total = (uint64_t)(source->sound->length*1000.0f);

		pitch = Audio.Player.GetPitch(source->internal.player);
		source->internal.currentOffset += (uint64_t)((float)elapsed*pitch);

		if (total != 0)
			source->internal.currentOffset %= total;

		source->internal.lastOffset += elapsed;
	}
}

static gkTimer* updateAudioTimer;

static GK_BOOL updateAudio(gkEvent* event, void* param);
static void soundSourceStopped(struct gkSoundNode* node, GK_BOOL dispatchStopEvent);

/***
    Sound system initialization and cleanup
*/

void gkInitAudio()
{
	Audio = gkGetAudioSystem();
	Audio.Init();

	gkInitAudioStream();

	updateAudioTimer = gkCreateTimer();
	updateAudioTimer->interval = 50;
	gkAddListener(updateAudioTimer, GK_ON_TIMER, 0, updateAudio, 0);
	gkStartTimer(updateAudioTimer, 0);
}

void gkCleanupAudio()
{
    struct gkSoundNode *c, *p = soundNodes;

    gkCleanupAudioStream();

    gkStopTimer(updateAudioTimer);
    gkDestroyTimer(updateAudioTimer);

    /* free all SoundSources and SoundNodes */
    while(p)
    {
        c = p;
        p = p->next;
	c->source->state = GK_SOUND_STATE_IDLE;	//Workaround-ish !
        if(c->source->internal.autoDestroy)
            gkDestroySoundSource(c->source);
        removeSoundNode(c);
    }

    Audio.Cleanup();
}


/***
    Sound creation and destruction
*/

gkSound* gkLoadSound(char* filename, int flags)
{
	gkSound* sound = 0;
	gkAudioStream* stream = gkAudioStreamOpen(filename);

	if (stream) {
		gkAudioStreamInfo* info;

		sound = (gkSound*)malloc(sizeof(gkSound));
		sound->internal.flags = flags;

		info = &sound->internal.streamInfo;

		stream->getInfo(stream, info);

		sound->length = info->length;

		if ( flags & GK_SOUND_STATIC ) {
			char* buf;
			buf = (char*)malloc(info->streamSize);
			stream->read(stream, buf, info->streamSize);

			sound->internal.buffers[0] = 
				Audio.CreateBuffer(info->format, info->sampleRate, GK_FALSE);

			Audio.Buffer.SetData(sound->internal.buffers[0], 
				buf, info->streamSize);

			sound->seekable = GK_TRUE;

			free(buf);
			gkAudioStreamClose(stream);
		} else if( flags & GK_SOUND_STREAM ) {
			int i;
			sound->internal.stream = stream;
			sound->seekable = stream->seek != 0;

			for (i = 0; i < GK_NUM_STREAM_BUFFERS; i++) {
				sound->internal.buffers[i] = 
					Audio.CreateBuffer(info->format, info->sampleRate, GK_TRUE);
			}
		}
	}

	return sound;
}

void gkDestroySound(gkSound* sound)
{
	int i, delBufferCount = 1;

	if (sound->internal.flags & GK_SOUND_STREAM )
		delBufferCount = GK_NUM_STREAM_BUFFERS;

	for (i = 0; i<delBufferCount; i++)
		Audio.DestroyBuffer(sound->internal.buffers[i]);

	free(sound);
}

gkSoundSource* gkCreateSoundSource()
{
    gkSoundSource* source = (gkSoundSource*)malloc(sizeof(gkSoundSource));

    gkInitDispatcher(&source->dispatcher);

    source->internal.player = Audio.CreatePlayer();

    source->internal.autoDestroy = GK_FALSE;

    source->state = GK_SOUND_STATE_IDLE;
    source->sound = 0;
    source->internal.looping = GK_FALSE;

    return source;
}

void gkDestroySoundSource(gkSoundSource* source)
{
	if (source->state != GK_SOUND_STATE_IDLE) {
		struct gkSoundNode* node = findSoundNode(source);
		if (node)
			removeSoundNode(node);
	}
	gkCleanupDispatcher(&source->dispatcher);
	Audio.DestroyPlayer(source->internal.player);
	free(source);
}

/***
    Sound properties
*/

void gkSetSoundGain(gkSoundSource* source, float gain)
{
	Audio.Player.SetVolume(source->internal.player, volToDb(gain));
}

float gkGetSoundGain(gkSoundSource* source)
{
	return dbToVol(Audio.Player.GetVolume(source->internal.player));
}

void gkSetSoundPitch(gkSoundSource* source, float pitch)
{
	updateTimeOffset(source, gkMilliseconds());
	Audio.Player.SetPitch(source->internal.player, pitch);
}

float gkGetSoundPitch(gkSoundSource* source)
{
	return Audio.Player.GetPitch(source->internal.player);
}

void gkSetSoundLooping(gkSoundSource* source, GK_BOOL looping)
{
    source->internal.looping = looping;
    if(source->state != GK_SOUND_STATE_IDLE)
	    Audio.Player.SetLooping(source->internal.player, looping);
}

GK_BOOL gkIsSoundLooping(gkSoundSource* source)
{
    return source->internal.looping;
}

void gkSetSoundOffset(gkSoundSource* source, float seconds)
{
    if(source->sound == 0 || !source->sound->seekable) return;
    if(source->sound->internal.flags & GK_SOUND_STATIC)
    {
	    Audio.Player.SetOffset(source->internal.player, seconds);
    }else
    {
        gkAudioStream* stream = source->sound->internal.stream;
        gkAudioStreamInfo* info = &source->sound->internal.streamInfo;
        int pos = (int)((float)info->sampleRate*seconds);

        stream->seek(stream, pos, SEEK_SET);

	Audio.Player.Stop(source->internal.player);
        fillQueue(source, GK_NUM_STREAM_BUFFERS, source->sound->internal.buffers);
	Audio.Player.Play(source->internal.player);

	if (source->state == GK_SOUND_STATE_PAUSED) 
		Audio.Player.Pause(source->internal.player);

        source->internal.currentOffset = (int)(seconds*1000.0f);
        source->internal.lastOffset = gkMilliseconds();
    }
}

float gkGetSoundOffset(gkSoundSource* source)
{
    if(source->sound == 0) 
	    return 0;

    if(source->sound->internal.flags & GK_SOUND_STATIC)
	    return Audio.Player.GetOffset(source->internal.player);

    return (float)source->internal.currentOffset*0.001f;
}

/***
    Listener properties
*/

void gkSetMasterGain(float gain)
{
	Audio.SetVolume(volToDb(gain));
}

float gkGetMasterGain()
{
	return dbToVol(Audio.GetVolume());
}

/***
    Playback control
*/

gkSoundSource* gkPlaySound(gkSound* sound, gkSoundSource* source)
{
	if (source == 0) {
		source = gkCreateSoundSource();
		source->internal.autoDestroy = GK_TRUE;
	}

	source->sound = sound;
	source->internal.currentOffset = 0;
	source->internal.lastOffset = gkMilliseconds();

	if (sound->internal.flags & GK_SOUND_STREAM) {
		gkAudioStream* stream = sound->internal.stream;
		stream->seek(stream, 0, SEEK_SET);
		Audio.Player.SetLooping(source->internal.player, GK_FALSE);
		fillQueue(source, GK_NUM_STREAM_BUFFERS, sound->internal.buffers);
	} else {
		Audio.Player.SetLooping(source->internal.player, source->internal.looping);
		Audio.Player.SetBuffer(source->internal.player, sound->internal.buffers[0]);
	}
	Audio.Player.Play(source->internal.player);

	source->state = GK_SOUND_STATE_PLAYING;

	addSoundNode(source);

	return source;
}

void gkPauseSound(gkSoundSource* source)
{
    updateTimeOffset(source, gkMilliseconds());
    source->state = GK_SOUND_STATE_PAUSED;
    Audio.Player.Pause(source->internal.player);
}

void gkResumeSound(gkSoundSource* source)
{
    source->state = GK_SOUND_STATE_PLAYING;
    source->internal.lastOffset = gkMilliseconds();
    Audio.Player.Play(source->internal.player);
}

void gkStopSound(gkSoundSource* source, GK_BOOL dispatchStopEvent)
{
	struct gkSoundNode* node = soundNodes;
	gkAudioPlayerState state = Audio.Player.GetState(source->internal.player);

	if(state != GK_AUDIOPLAYER_STOPPED)
		Audio.Player.Stop(source->internal.player);
	else
		return;

	while (node) {
		if(node->source == source) break;
		node = node->next;
	}

	soundSourceStopped(node, dispatchStopEvent);
}

/***
    Sound system heartbeat
*/

static void updateStream(gkSoundSource* source)
{
	gkSound* sound = source->sound;
	int processed, queued;

	Audio.Player.GetQueueState(source->internal.player, &processed, &queued);

	if (queued<GK_NUM_STREAM_BUFFERS)
		fillQueue(source, (GK_NUM_STREAM_BUFFERS - queued), sound->internal.buffers + queued);

	if (processed > 0) {
		int i, u;
		gkAudioBuffer *buffers = sound->internal.buffers;

		Audio.Player.UnqueueBuffers(source->internal.player, 
			processed, buffers);

		/* refill */
		fillQueue(source, processed, buffers);

		/* rotate buffer */
		for (i = 0; i<processed; i++) {
			gkAudioBuffer tmp = buffers[0];
			for (u = 1; u < GK_NUM_STREAM_BUFFERS; u++) 
				buffers[u-1] = buffers[u];
			buffers[GK_NUM_STREAM_BUFFERS-1] = tmp;
		}
	}
}

static void soundSourceStopped(struct gkSoundNode* node, GK_BOOL dispatchStopEvent)
{
	gkSoundSource* source = node->source;

	source->state = GK_SOUND_STATE_IDLE;

	Audio.Player.SetBuffer(source->internal.player, 0);

	if (dispatchStopEvent) {
		gkEvent evt;
		evt.type = GK_ON_SOUND_STOPPED;
		evt.target = source;
		gkDispatch(source, &evt);
	}
	source->sound = 0;

	if(source->internal.autoDestroy)
		gkDestroySoundSource(source);

	removeSoundNode(node);
}

static GK_BOOL updateAudio(gkEvent* event, void* param)
{
    struct gkSoundNode* p, *curNode;
    gkAudioPlayerState state;
    uint64_t millis = gkMilliseconds();

    if(soundNodes != 0)
    {
        p = soundNodes;
        while(p)
        {
            gkSoundSource* source = p->source;
            curNode = p;
            p = p->next;

            if( (source->state == GK_SOUND_STATE_PLAYING) &&
               (source->sound->internal.flags & GK_SOUND_STREAM) )
            {
                updateTimeOffset(source, millis);
                updateStream(source);
            }

	    state = Audio.Player.GetState(source->internal.player);

	    if (state == GK_AUDIOPLAYER_STOPPED)
                soundSourceStopped(curNode, GK_TRUE);
        }
    }
    return GK_TRUE;
}