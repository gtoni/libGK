#ifndef _GK_AUDIO_H_
#define _GK_AUDIO_H_

#include <stdio.h>

#define GK_NUM_STREAM_BUFFERS 10
#define GK_STREAM_BUFFER_SIZE 1024*8

struct gkSoundInternal
{
    int flags;
    int alBuffers[GK_NUM_STREAM_BUFFERS];
    FILE* streamingFile;
};

#endif //
