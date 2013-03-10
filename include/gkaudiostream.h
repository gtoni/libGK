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

#ifndef _GK_AUDIOSTREAM_H_
#define _GK_AUDIOSTREAM_H_

#include <AL/al.h>

#define GK_AUDIO_FORMAT_UNSUPPORTED 0xffff;
#define GK_AUDIO_FORMAT_MONO8       AL_FORMAT_MONO8
#define GK_AUDIO_FORMAT_MONO16      AL_FORMAT_MONO16
#define GK_AUDIO_FORMAT_MONO32      GK_AUDIO_FORMAT_UNSUPPORTED
#define GK_AUDIO_FORMAT_STEREO8     AL_FORMAT_STEREO8
#define GK_AUDIO_FORMAT_STEREO16    AL_FORMAT_STEREO16
#define GK_AUDIO_FORMAT_STEREO32    GK_AUDIO_FORMAT_UNSUPPORTED

typedef struct _gkAudioStreamInfo gkAudioStreamInfo;
struct _gkAudioStreamInfo{
    int format;
    int sampleRate;
    size_t streamSize;
    float length;
};

typedef struct _gkAudioStream gkAudioStream;
struct _gkAudioStream{
    int (*read)(gkAudioStream* stream, void* buffer, size_t bytes);
    int (*seek)(gkAudioStream* stream, size_t offset, int origin);
    int (*eof)(gkAudioStream* stream);
    int (*getError)(gkAudioStream* stream);
    void (*getInfo)(gkAudioStream* stream, gkAudioStreamInfo* info);
    void (*destroy)(gkAudioStream* stream);
};

void gkInitAudioStream();
void gkCleanupAudioStream();

gkAudioStream* gkAudioStreamOpen(char* location);
void gkAudioStreamClose(gkAudioStream* stream);

#endif
