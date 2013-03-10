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

#include "gk.h"
#include "gk_internal.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <stdio.h>
#include <stdlib.h>

#include <gkaudiostream.h>

static ALCdevice* device;
static ALCcontext* ctx;

struct gkSoundNode
{
    gkSound* sound;
    gkSoundSource* source;
    struct gkSoundNode* prev, *next;
};

static struct gkSoundNode* soundNodes = 0, *lastSoundNode = 0;

static void addSoundNode(gkSound* sound, gkSoundSource* source)
{
    struct gkSoundNode* node = (struct gkSoundNode*)malloc(sizeof(struct gkSoundNode));

    node->sound = sound;
    node->source = source;
    node->prev = lastSoundNode;
    node->next = 0;

    if(lastSoundNode != 0)
        lastSoundNode->next = node;
    else soundNodes = node;

    lastSoundNode = node;
}

static void removeSoundNode(struct gkSoundNode* node)
{
    if(node->next == 0) lastSoundNode = node->prev;
    if(node->prev)
    {
        node->prev->next = node->next;
        if(node->next)
            node->next->prev = node->prev;
    }
    else
    {
        soundNodes = node->next;
        if(node->next)
            node->next->prev = 0;
    }
    free(node);
}

#define gkCheckALError() {int err; if((err = alGetError()) != AL_NO_ERROR) printf("AL Error %X\n", err); }

static gkTimer* updateAudioTimer;

static GK_BOOL updateAudio(gkEvent* event, void* param);
static void soundSourceStopped(struct gkSoundNode* node, GK_BOOL dispatchStopEvent);

void gkInitAudio()
{
    if(!(device = alcOpenDevice(NULL)))
    {
        printf("OpenAL: Failed to open device\n");
    }
    ctx = alcCreateContext(device, NULL);
    if(ctx)
    {
        alcMakeContextCurrent(ctx);
    }else
    {
        printf("Failed to create audio context");
    }

    updateAudioTimer = gkCreateTimer();
    updateAudioTimer->interval = 100;
    gkAddListener(updateAudioTimer, GK_ON_TIMER, 0, updateAudio, 0);
    gkStartTimer(updateAudioTimer, 0);

    gkInitAudioStream();
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
        if(c->source->internal.autoDestroy)
            gkDestroySoundSource(c->source);
        removeSoundNode(c);
    }

    alcDestroyContext(ctx);
    alcCloseDevice(device);
}


static int fillBuffer(int buffer, gkSound* sound)
{
    char buf[GK_STREAM_BUFFER_SIZE];
    gkAudioStream* stream = sound->internal.stream;
    gkAudioStreamInfo* info = &sound->internal.streamInfo;
    size_t readBytes = stream->read(stream, buf, GK_STREAM_BUFFER_SIZE);
    if(readBytes>0)
    {
        alBufferData(buffer, info->format, buf, readBytes, info->sampleRate);
        gkCheckALError();
    }
    return readBytes;
}

static int fillBuffers(int* buffers, int numToFill, gkSound* sound)
{
    int i = 0;

    while( (i<numToFill) && (fillBuffer(buffers[i], sound)>0) )
        i++;

    return i;
}

static void fillQueue(gkSoundSource* source, int numBuffers, int* buffers, gkSound* sound)
{
    gkAudioStream* stream = sound->internal.stream;

	int filled = fillBuffers(buffers, numBuffers, sound);

	if(filled == 0 && stream->eof(stream) && source->internal.looping)
    {
        stream->seek(stream, 0, SEEK_SET);
        filled = fillBuffers(buffers, numBuffers, sound);
    }

	alSourceQueueBuffers(source->id, filled, buffers);
}

gkSound* gkLoadSound(char* filename, int flags)
{
    gkSound* sound = 0;
    gkAudioStream* stream = gkAudioStreamOpen(filename);

    if(stream)
    {
        gkAudioStreamInfo* info;

        sound = (gkSound*)malloc(sizeof(gkSound));
        sound->internal.flags = flags;

        info = &sound->internal.streamInfo;

        stream->getInfo(stream, info);

        sound->length = info->length;

        if( flags & GK_SOUND_STATIC )
        {
            char* buf;
            buf = (char*)malloc(info->streamSize);
            stream->read(stream, buf, info->streamSize);

            alGenBuffers(1, sound->internal.alBuffers);
            alBufferData(sound->internal.alBuffers[0], info->format, buf, info->streamSize, info->sampleRate);

            free(buf);
            gkAudioStreamClose(stream);
        }else if( flags & GK_SOUND_STREAM )
        {
            sound->internal.stream = stream;

            alGenBuffers(GK_NUM_STREAM_BUFFERS, sound->internal.alBuffers);
        }
    }

    return sound;
}

void gkDestroySound(gkSound* sound)
{
    int delBufferCount = 1;

    if( sound->internal.flags & GK_SOUND_STREAM )
        delBufferCount = GK_NUM_STREAM_BUFFERS;

    alDeleteBuffers(delBufferCount, sound->internal.alBuffers);

    free(sound);
}

gkSoundSource* gkCreateSoundSource()
{
    gkSoundSource* source = (gkSoundSource*)malloc(sizeof(gkSoundSource));

    gkInitListenerList(&source->listeners);

    alGenSources(1, &source->id);

    alSourcei(source->id, AL_SOURCE_RELATIVE, 1);
    alSource3f(source->id, AL_POSITION, 0,0,0);

    source->internal.autoDestroy = GK_FALSE;

    source->state = GK_SOUNDSOURCE_IDLE;
    source->internal.looping = GK_FALSE;

    return source;
}

void gkSetSoundGain(gkSoundSource* source, float gain)
{
    alSourcef(source->id, AL_GAIN, gain);
}

float gkGetSoundGain(gkSoundSource* source)
{
    float gain;
    alGetSourcef(source->id, AL_GAIN, &gain);
    return gain;
}

void gkSetSoundPitch(gkSoundSource* source, float pitch)
{
    alSourcef(source->id, AL_PITCH, pitch);
}

float gkGetSoundPitch(gkSoundSource* source)
{
    float pitch;
    alGetSourcef(source->id, AL_PITCH, &pitch);
    return pitch;
}

void gkSetSoundLooping(gkSoundSource* source, GK_BOOL looping)
{
    source->internal.looping = looping;
    if(source->state != GK_SOUNDSOURCE_IDLE)
    {
        int type;
        alGetSourcei(source->id, AL_SOURCE_TYPE, &type);
        if(type == AL_STATIC)
            alSourcei(source->id, AL_LOOPING, looping);
    }
}

GK_BOOL gkIsSoundLooping(gkSoundSource* source)
{
    return source->internal.looping;
}

void gkDestroySoundSource(gkSoundSource* source)
{
    gkCleanupListenerList(&source->listeners);
    alDeleteSources(1, &source->id);
    free(source);
}

gkSoundSource* gkPlaySound(gkSound* sound, gkSoundSource* source)
{
    if(source == 0)
    {
        source = gkCreateSoundSource();
        source->internal.autoDestroy = GK_TRUE;
    }

    if(sound->internal.flags & GK_SOUND_STREAM)
    {
        gkAudioStream* stream = sound->internal.stream;
        stream->seek(stream, 0, SEEK_SET);
		alSourcei(source->id, AL_LOOPING, AL_FALSE);
		fillQueue(source, GK_NUM_STREAM_BUFFERS, sound->internal.alBuffers, sound);
        alSourcePlay(source->id);
    }
    else
    {
        alSourcei(source->id, AL_BUFFER, sound->internal.alBuffers[0]);
        alSourcei(source->id, AL_LOOPING, source->internal.looping);
        alSourcePlay(source->id);
    }

    source->state = GK_SOUNDSOURCE_PLAYING;

    addSoundNode(sound, source);

    return source;
}

void gkPauseSound(gkSoundSource* source)
{
    source->state = GK_SOUNDSOURCE_PAUSED;
    alSourcePause(source->id);
}

void gkResumeSound(gkSoundSource* source)
{
    source->state = GK_SOUNDSOURCE_PLAYING;
    alSourcePlay(source->id);
}

void gkStopSound(gkSoundSource* source, GK_BOOL dispatchStopEvent)
{
    int state;
    struct gkSoundNode* node = soundNodes;
    alGetSourcei(source->id, AL_SOURCE_STATE, &state);

    if(state != AL_STOPPED)
        alSourceStop(source->id);

    while(node)
    {
        if(node->source == source) break;
        node = node->next;
    }
    soundSourceStopped(node, dispatchStopEvent);
}

static void updateStream(struct gkSoundNode* stream)
{
    gkSound* sound = stream->sound;
    gkSoundSource* source = stream->source;

    int processed, queued;

    alGetSourcei(source->id, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(source->id, AL_BUFFERS_QUEUED, &queued);

	if(queued<GK_NUM_STREAM_BUFFERS)
    {
		fillQueue(source, (GK_NUM_STREAM_BUFFERS - queued), sound->internal.alBuffers + queued, sound);
    }

    if(processed>0)
    {
        int i, u, *buffers = sound->internal.alBuffers;

        alSourceUnqueueBuffers(source->id, processed, buffers);

        /* refill */
		fillQueue(source, processed, buffers, sound);

        /* rotate buffer */
        for(i = 0; i<processed; i++){
            int tmp = buffers[0];
            for(u = 1; u<GK_NUM_STREAM_BUFFERS; u++) buffers[u-1] = buffers[u];
            buffers[GK_NUM_STREAM_BUFFERS-1] = tmp;
        }
    }
}

static void soundSourceStopped(struct gkSoundNode* node, GK_BOOL dispatchStopEvent)
{
    gkSoundSource* source = node->source;

    source->state = GK_SOUNDSOURCE_IDLE;

    alSourcei(source->id, AL_BUFFER, 0);

    if(dispatchStopEvent)
    {
        gkSoundEvent evt;
        evt.type = GK_ON_SOUND_STOPPED;
        evt.target = source;
        evt.sound = node->sound;
        evt.source = source;
        gkDispatch(source, &evt);
    }
    if(source->internal.autoDestroy)
        gkDestroySoundSource(source);
    removeSoundNode(node);
}

static GK_BOOL updateAudio(gkEvent* event, void* param)
{
    struct gkSoundNode* p, *curNode;
    int state;

    if(soundNodes != 0)
    {
        p = soundNodes;
        while(p)
        {
            curNode = p;
            p = p->next;
            if( (curNode->source->state == GK_SOUNDSOURCE_PLAYING) &&
               (curNode->sound->internal.flags & GK_SOUND_STREAM) )
            {
                updateStream(curNode);
            }

            alGetSourcei(curNode->source->id, AL_SOURCE_STATE, &state);

            if( state == AL_STOPPED )
                soundSourceStopped(curNode, GK_TRUE);
        }
    }
    return GK_TRUE;
}
