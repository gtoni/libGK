#include "gk.h"
#include "gk_internal.h"

#include <AL/al.h>
#include <AL/alc.h>

static ALCdevice* device;

void gkInitAudio()
{
    device = alcOpenDevice(NULL);
}

void gkCleanupAudio()
{
    alcCloseDevice(device);
}
