#include "gk.h"
#include "gk_internal.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <stdio.h>
#include <stdlib.h>

static ALCdevice* device;
static ALCcontext* ctx;

struct gkSoundNode
{
    gkSound* sound;
    gkSoundInstance* instance;
    struct gkSoundNode* next;
};

static struct gkSoundNode* soundNodes = 0, *lastSoundNode = 0;

static gkTimer* updateAudioTimer;

GK_BOOL updateAudio(gkEvent* event, void* param);

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
}

void gkCleanupAudio()
{
    gkStopTimer(updateAudioTimer);
    gkDestroyTimer(updateAudioTimer);

    alcDestroyContext(ctx);
    alcCloseDevice(device);
}


static int fillBuffer(int buffer, FILE* input)
{
    char buf[GK_STREAM_BUFFER_SIZE];
    size_t readBytes = fread(buf, sizeof(char), GK_STREAM_BUFFER_SIZE, input);
    if(readBytes>0)
    {
        alBufferData(buffer, AL_FORMAT_MONO8, buf, readBytes, 11025);
    }
    return readBytes;
}

static int fillBuffers(gkSound* sound, int numToFill, FILE* input)
{
    int i = 0;

    while( (i<numToFill) && (fillBuffer(sound->internal.alBuffers[i], input)>0) )
        i++;

    return i;
}

gkSound* gkLoadSound(char* filename, int flags)
{
    gkSound* sound = 0;
    FILE* file = fopen(filename, "rb");

    if( flags & GK_SOUND_STATIC )
    {
        if(file)
        {
            size_t fileSize;
            char* buf;
            fseek(file, 0, SEEK_END);
            fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);
            buf = (char*)malloc(fileSize);
            fread(buf, sizeof(char), fileSize, file);

            sound = (gkSound*)malloc(sizeof(gkSound));
            sound->internal.flags = flags;

            alGenBuffers(1, sound->internal.alBuffers);
            alBufferData(sound->internal.alBuffers[0], AL_FORMAT_MONO8, buf, fileSize, 11025);

            free(buf);
            fclose(file);
        }
    }
    else if( flags & GK_SOUND_STREAM )
    {
        sound = (gkSound*)malloc(sizeof(gkSound));
        sound->internal.flags = flags;
        sound->internal.streamingFile = file;

        alGenBuffers(GK_NUM_STREAM_BUFFERS, sound->internal.alBuffers);

        printf("buffers filled %d\n", fillBuffers(sound, GK_NUM_STREAM_BUFFERS, file));
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

gkSoundInstance* gkPlaySound(gkSound* sound)
{
    struct gkSoundNode* node = (struct gkSoundNode*)malloc(sizeof(struct gkSoundNode));

    gkSoundInstance* soundInstance = (gkSoundInstance*)malloc(sizeof(gkSoundInstance));

	alGetError();

    alGenSources(1, &soundInstance->alSource);
    alSourcei(soundInstance->alSource, AL_SOURCE_RELATIVE, 1);
    alSource3f(soundInstance->alSource, AL_POSITION, 0,0,0);

    if(sound->internal.flags & GK_SOUND_STREAM)
    {
        alSourcei(soundInstance->alSource, AL_SOURCE_TYPE, AL_STREAMING);
		alSourceQueueBuffers(soundInstance->alSource, GK_NUM_STREAM_BUFFERS, sound->internal.alBuffers);

		if(alGetError() == AL_INVALID_OPERATION)
			printf("Invalid operation\n");

        alSourcePlay(soundInstance->alSource);
    }
    else
    {
        alSourcei(soundInstance->alSource, AL_SOURCE_TYPE, AL_STATIC);
        alSourcei(soundInstance->alSource, AL_BUFFER, sound->internal.alBuffers[0]);
        alSourcePlay(soundInstance->alSource);
    }

    node->sound = sound;
    node->instance = soundInstance;
    node->next = 0;

    if(lastSoundNode != 0)
        lastSoundNode->next = node;
    else soundNodes = lastSoundNode = node;


    return soundInstance;
}

void updateStream(struct gkSoundNode* stream)
{
    gkSound* sound = stream->sound;
    gkSoundInstance* instance = stream->instance;

    int processed, queued;

    alGetSourcei(instance->alSource, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(instance->alSource, AL_BUFFERS_QUEUED, &queued);

    if(processed>0)
    {
        int filled, i, u, *buffers = sound->internal.alBuffers;

        alSourceUnqueueBuffers(instance->alSource, processed, buffers);

        /* refill */
        filled = fillBuffers(sound, processed, sound->internal.streamingFile);

        alSourceQueueBuffers(instance->alSource, filled, buffers);

        /* rotate buffer */
        for(i = 0; i<processed; i++){
            int tmp = buffers[0];
            for(u = 1; u<GK_NUM_STREAM_BUFFERS; u++) buffers[u-1] = buffers[u];
            buffers[GK_NUM_STREAM_BUFFERS-1] = tmp;
        }
    }
}

GK_BOOL updateAudio(gkEvent* event, void* param)
{
    struct gkSoundNode* p, *curNode;
    int state;

    if(soundNodes != 0)
    {
        p = soundNodes;
        while(p)
        {
            curNode = p;
            if( (curNode->sound->internal.flags & GK_SOUND_STREAM) )
                updateStream(curNode);

            alGetSourcei(curNode->instance->alSource, AL_SOURCE_STATE, &state);

            if( state == AL_STOPPED )
            {
                /* delete sound node */
                printf("Sound instance stopped\n");
            }

            p = p->next;
        }
    }
    return GK_TRUE;
}
