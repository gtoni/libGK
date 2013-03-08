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
    struct gkSoundNode* prev, *next;
};

static struct gkSoundNode* soundNodes = 0, *lastSoundNode = 0;

static void addSoundNode(gkSound* sound, gkSoundInstance* instance)
{
    struct gkSoundNode* node = (struct gkSoundNode*)malloc(sizeof(struct gkSoundNode));

    node->sound = sound;
    node->instance = instance;
    node->prev = lastSoundNode;
    node->next = 0;

    if(lastSoundNode != 0)
        lastSoundNode->next = node;
    else soundNodes = node;

    lastSoundNode = node;
}

static void removeSoundNode(struct gkSoundNode* node)
{
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



static gkTimer* updateAudioTimer;

static GK_BOOL updateAudio(gkEvent* event, void* param);
static void DestroySoundInstance(gkSoundInstance* instance);

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
    struct gkSoundNode *c, *p = soundNodes;

    gkStopTimer(updateAudioTimer);
    gkDestroyTimer(updateAudioTimer);

    /* free all SoundInstances and SoundNodes */
    while(p)
    {
        c = p;
        p = p->next;
        DestroySoundInstance(c->instance);
        removeSoundNode(c);
    }

    alcDestroyContext(ctx);
    alcCloseDevice(device);
}


static int fillBuffer(int buffer, FILE* input)
{
    char buf[GK_STREAM_BUFFER_SIZE];
    size_t readBytes = fread(buf, sizeof(char), GK_STREAM_BUFFER_SIZE, input);
    if(readBytes>0)
    {
        alBufferData(buffer, AL_FORMAT_STEREO16, buf, readBytes, 48000);
    }
    return readBytes;
}

static int fillBuffers(int* buffers, int numToFill, FILE* input)
{
    int i = 0;

    while( (i<numToFill) && (fillBuffer(buffers[i], input)>0) )
        i++;

    return i;
}

static void fillQueue(int alSource, int numBuffers, int* buffers, FILE* input)
{
	int filled = fillBuffers(buffers, numBuffers, input);
	alSourceQueueBuffers(alSource, filled, buffers);
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

static gkSoundInstance* CreateSoundInstance()
{
    gkSoundInstance* instance = (gkSoundInstance*)malloc(sizeof(gkSoundInstance));

    gkInitListenerList(&instance->listeners);

    alGenSources(1, &instance->alSource);

    alSourcei(instance->alSource, AL_SOURCE_RELATIVE, 1);
    alSource3f(instance->alSource, AL_POSITION, 0,0,0);

    return instance;
}

static void DestroySoundInstance(gkSoundInstance* instance)
{
    gkCleanupListenerList(&instance->listeners);
    alDeleteSources(1, &instance->alSource);
    free(instance);
}

gkSoundInstance* gkPlaySound(gkSound* sound)
{
    gkSoundInstance* soundInstance = CreateSoundInstance();

    if(sound->internal.flags & GK_SOUND_STREAM)
    {
		fseek(sound->internal.streamingFile, 0, SEEK_SET);
		fillQueue(soundInstance->alSource, GK_NUM_STREAM_BUFFERS, sound->internal.alBuffers, sound->internal.streamingFile);
        alSourcePlay(soundInstance->alSource);
    }
    else
    {
        alSourcei(soundInstance->alSource, AL_BUFFER, sound->internal.alBuffers[0]);
        alSourcePlay(soundInstance->alSource);
    }

    addSoundNode(sound, soundInstance);

    return soundInstance;
}

static void updateStream(struct gkSoundNode* stream)
{
    gkSound* sound = stream->sound;
    gkSoundInstance* instance = stream->instance;

    int processed, queued;

    alGetSourcei(instance->alSource, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(instance->alSource, AL_BUFFERS_QUEUED, &queued);

	if(queued<GK_NUM_STREAM_BUFFERS)
		fillQueue(instance->alSource, (GK_NUM_STREAM_BUFFERS - queued), sound->internal.alBuffers, sound->internal.streamingFile);

    if(processed>0)
    {
        int i, u, *buffers = sound->internal.alBuffers;

        alSourceUnqueueBuffers(instance->alSource, processed, buffers);

        /* refill */
		fillQueue(instance->alSource, processed, buffers, sound->internal.streamingFile);

        /* rotate buffer */
        for(i = 0; i<processed; i++){
            int tmp = buffers[0];
            for(u = 1; u<GK_NUM_STREAM_BUFFERS; u++) buffers[u-1] = buffers[u];
            buffers[GK_NUM_STREAM_BUFFERS-1] = tmp;
        }
    }
}

static void soundInstanceStopped(struct gkSoundNode* node)
{
    gkSoundEvent evt;
    printf("Sound instance stopped\n");
    evt.type = GK_ON_SOUND_STOPPED;
    evt.target = node->instance;
    evt.sound = node->sound;
    gkDispatch(node->instance, &evt);

    DestroySoundInstance(node->instance);
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
            if( (curNode->sound->internal.flags & GK_SOUND_STREAM) )
                updateStream(curNode);

            alGetSourcei(curNode->instance->alSource, AL_SOURCE_STATE, &state);

            if( state == AL_STOPPED )
                soundInstanceStopped(curNode);
        }
    }
    return GK_TRUE;
}
