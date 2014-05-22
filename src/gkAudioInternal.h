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

#ifndef _GK_AUDIO_INTERNAL_H_
#define _GK_AUDIO_INTERNAL_H_

#include <gkTypes.h>

#define GK_AUDIO_FORMAT_UNSUPPORTED 0xffff
#define GK_AUDIO_FORMAT_MONO8       0x1100
#define GK_AUDIO_FORMAT_MONO16      0x1101
#define GK_AUDIO_FORMAT_MONO32      GK_AUDIO_FORMAT_UNSUPPORTED
#define GK_AUDIO_FORMAT_STEREO8     0x1102
#define GK_AUDIO_FORMAT_STEREO16    0x1103
#define GK_AUDIO_FORMAT_STEREO32    GK_AUDIO_FORMAT_UNSUPPORTED

typedef struct gkAudioStreamInfo{
    int format;
    int sampleRate;
    int channels;
    int bitsPerSample;
    size_t streamSize;
    float length;
}gkAudioStreamInfo;

typedef struct gkAudioStream{
    int (*read)(struct gkAudioStream* stream, void* buffer, size_t bytes);
    int (*seek)(struct gkAudioStream* stream, size_t sampleOffset, int origin);
    int (*eof)(struct gkAudioStream* stream);
    int (*getError)(struct gkAudioStream* stream);
    void (*getInfo)(struct gkAudioStream* stream, gkAudioStreamInfo* info);
    void (*destroy)(struct gkAudioStream* stream);
}gkAudioStream;

void gkInitAudioStream();
void gkCleanupAudioStream();

gkAudioStream* gkAudioStreamOpen(char* location);
void gkAudioStreamClose(gkAudioStream* stream);

#define GK_NUM_STREAM_BUFFERS 10
#define GK_STREAM_BUFFER_SIZE 1024*8

#include "gkAudioSystem.h"

struct gkSoundInternal
{
	int flags;
	gkAudioBuffer buffers[GK_NUM_STREAM_BUFFERS];
	gkAudioStream* stream;
	gkAudioStreamInfo streamInfo;
};

struct gkSoundSourceInternal
{
	gkAudioPlayer player;
	uint64_t currentOffset;
	uint64_t lastOffset;
	GK_BOOL looping;
	GK_BOOL autoDestroy;
};

#endif //
