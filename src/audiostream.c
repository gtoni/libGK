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


#include <gk.h>
#include <stdlib.h>
#include <stdio.h>

#include "gk_internal.h"

static int getAudioFormat(int channels, int bitsPerSample)
{
    if(channels == 1)
    {
        switch(bitsPerSample)
        {
            case 8: return GK_AUDIO_FORMAT_MONO8;
            case 16: return GK_AUDIO_FORMAT_MONO16;
            case 32: return GK_AUDIO_FORMAT_MONO32;
        }
    }else if(channels == 2)
    {
        switch(bitsPerSample)
        {
            case 8: return GK_AUDIO_FORMAT_STEREO8;
            case 16: return GK_AUDIO_FORMAT_STEREO16;
            case 32: return GK_AUDIO_FORMAT_STEREO32;
        }
    }
    return GK_AUDIO_FORMAT_UNSUPPORTED;
}

/* WAV stream */

struct wavHeader{
    char riff[4];
    uint32_t chunkSize;
    char wave[4];
    char fmt[4];
    uint32_t subchunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

typedef struct _gkWavAudioStream gkWavAudioStream;
struct _gkWavAudioStream{
    gkAudioStream base;
    FILE* handle;
    size_t startOffset;
    gkAudioStreamInfo info;
};

static int readWavStream(gkAudioStream* stream, void* buffer, size_t bytes);
static int seekWavStream(gkAudioStream* stream, size_t offset, int origin);
static void getWavStreamInfo(gkAudioStream* stream, gkAudioStreamInfo* info);
static int eofWavStream(gkAudioStream* stream);
static void destroyWavAudioStream(gkAudioStream* stream);

static gkAudioStream* createWavAudioStream(char* location)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)malloc(sizeof(gkWavAudioStream));
    struct wavHeader header;
    stream->handle = fopen(location, "rb");
    if(stream->handle)
    {
        fread(&header, sizeof(header), 1, stream->handle);
        stream->startOffset = ftell(stream->handle);
        if(header.audioFormat == 1)
        {
            stream->info.sampleRate = header.sampleRate;
            stream->info.streamSize = header.dataSize;
            stream->info.length = (float)header.dataSize/(float)header.byteRate;
            stream->info.format = getAudioFormat(header.numChannels, header.bitsPerSample);
            stream->base.read = readWavStream;
            stream->base.seek = seekWavStream;
            stream->base.getInfo = getWavStreamInfo;
            stream->base.eof = eofWavStream;
            stream->base.destroy = destroyWavAudioStream;
            return (gkAudioStream*)stream;
        }
        fclose(stream->handle);
    }
    free(stream);
    return 0;
}

static void destroyWavAudioStream(gkAudioStream* s)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    fclose(stream->handle);
    free(stream);
}

static void getWavStreamInfo(gkAudioStream* s, gkAudioStreamInfo* info)
{
    memcpy(info, &((gkWavAudioStream*)s)->info, sizeof(gkAudioStreamInfo));
}

static int readWavStream(gkAudioStream* s, void* buffer, size_t bytes)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    return fread(buffer, sizeof(char), bytes, stream->handle);
}

static int seekWavStream(gkAudioStream* s, size_t offset, int origin)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    if(origin == SEEK_SET)
    {
        offset = offset + stream->startOffset;
    }
    return fseek(stream->handle, offset, origin);
}

static int eofWavStream(gkAudioStream* s)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    int eof = feof(stream->handle);
    clearerr(stream->handle);
    return eof;
}


/* MP3 stream through libmpg123 */

#include <mpg123.h>

typedef struct _gkMp3AudioStream gkMp3AudioStream;
struct _gkMp3AudioStream{
    gkAudioStream base;
    mpg123_handle* handle;
    GK_BOOL eof;
};

static int readMp3Stream(gkAudioStream* stream, void* buffer, size_t bytes);
static int seekMp3Stream(gkAudioStream* stream, size_t offset, int origin);
static void getMp3StreamInfo(gkAudioStream* stream, gkAudioStreamInfo* info);
static int eofMp3Stream(gkAudioStream* stream);
static void destroyMp3AudioStream(gkAudioStream* stream);

static gkAudioStream* createMp3AudioStream(char* location)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)malloc(sizeof(gkMp3AudioStream));
    stream->handle = mpg123_new(0, 0);
    stream->eof = GK_FALSE;
    mpg123_open(stream->handle, location);
    stream->base.read = readMp3Stream;
    stream->base.seek = seekMp3Stream;
    stream->base.getInfo = getMp3StreamInfo;
    stream->base.eof = eofMp3Stream;
    stream->base.destroy = destroyMp3AudioStream;
    return (gkAudioStream*)stream;
}

static void destroyMp3AudioStream(gkAudioStream* s)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    mpg123_close(stream->handle);
    mpg123_delete(stream->handle);
    free(stream);
}

static void getMp3StreamInfo(gkAudioStream* s, gkAudioStreamInfo* info)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
	int channels, encoding, bitsPerSample;
	long sampleRate, totalSamples;

	mpg123_getformat(stream->handle, &sampleRate, &channels, &encoding);

	if(( encoding&MPG123_ENC_32) == MPG123_ENC_32)
		bitsPerSample = 32;
	else if( (encoding&MPG123_ENC_16) == MPG123_ENC_16)
		bitsPerSample = 16;
	else if( (encoding&MPG123_ENC_8) == MPG123_ENC_8)
		bitsPerSample = 8;

    totalSamples = mpg123_length(stream->handle);
    info->format = getAudioFormat(channels, bitsPerSample);
    info->sampleRate = sampleRate;
    info->length = (float)totalSamples/(float)sampleRate;
    info->streamSize = totalSamples*(bitsPerSample/sizeof(char));
}

static int readMp3Stream(gkAudioStream* s, void* buffer, size_t bytes)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    size_t bytesRead;
    int res = mpg123_read(stream->handle, buffer, bytes, &bytesRead);
    if(res == MPG123_DONE) stream->eof = GK_TRUE;
    return bytesRead;
}

static int seekMp3Stream(gkAudioStream* s, size_t offset, int origin)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    stream->eof = GK_FALSE;
    off_t streamOffset = mpg123_seek(stream->handle, offset, origin);
    if(streamOffset>=0) return 0;
    return streamOffset;
}

static int eofMp3Stream(gkAudioStream* s)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    return stream->eof;
}

/* End of stream types */


void gkInitAudioStream()
{
    mpg123_init();
}

void gkCleanupAudioStream()
{
    mpg123_exit();
}

gkAudioStream* gkAudioStreamOpen(char* location)
{
    char* ext = location + (strlen(location) - 3);
    if(stricmp(ext,"wav") == 0)
        return createWavAudioStream(location);
    else if(stricmp(ext, "mp3") == 0)
        return createMp3AudioStream(location);
}

void gkAudioStreamClose(gkAudioStream* stream)
{
    stream->destroy(stream);
}
