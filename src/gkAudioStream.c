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


#include <gk.h>
#include <stdlib.h>
#include <stdio.h>

#include "gkStream.h"
#include "gkConfig.h"
#include "gkAudioInternal.h"

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
    gkStream* handle;
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
	struct wavHeader header;
	char* subchunk = header.data;
	gkWavAudioStream* stream;
	gkStream* fileStream = gkOpenFile(location, "rb");

	if (!fileStream) {
		printf("GK [ERROR] : Couldn't open file: %s\n", location);
		return 0;
	}

	stream = (gkWavAudioStream*)malloc(sizeof(gkWavAudioStream));
	stream->handle = fileStream;

	gkStreamRead(stream->handle, &header, sizeof(header));
	
	while (strncmp(subchunk, "data", 4) != 0) {
		gkStreamSeek(stream->handle, header.dataSize, GK_SEEK_CUR);
		gkStreamRead(stream->handle, subchunk, sizeof(uint32_t)*2);
	}
	
	stream->startOffset = gkStreamTell(stream->handle);
	
	if (header.audioFormat == 1) {
		stream->info.sampleRate = header.sampleRate;
		stream->info.streamSize = header.dataSize;
		stream->info.length = (float)header.dataSize/(float)header.byteRate;
		stream->info.format = getAudioFormat(header.numChannels, header.bitsPerSample);
		stream->info.channels = header.numChannels;
		stream->info.bitsPerSample = header.bitsPerSample;
		stream->base.read = readWavStream;
		stream->base.seek = seekWavStream;
		stream->base.getInfo = getWavStreamInfo;
		stream->base.eof = eofWavStream;
		stream->base.destroy = destroyWavAudioStream;
		return (gkAudioStream*)stream;
	}

	printf("GK [ERROR] : WAV invalid audio format: %s\n", location);

	gkStreamClose(stream->handle);
	free(stream);
	return 0;
}

static void destroyWavAudioStream(gkAudioStream* s)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    gkStreamClose(stream->handle);
    free(stream);
}

static void getWavStreamInfo(gkAudioStream* s, gkAudioStreamInfo* info)
{
    memcpy(info, &((gkWavAudioStream*)s)->info, sizeof(gkAudioStreamInfo));
}

static int readWavStream(gkAudioStream* s, void* buffer, size_t bytes)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    return gkStreamRead(stream->handle, buffer, sizeof(char) * bytes);
}

static int seekWavStream(gkAudioStream* s, size_t sampleOffset, int origin)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    sampleOffset *= (stream->info.bitsPerSample/8)*stream->info.channels;
    if(origin == GK_SEEK_SET)
    {
        sampleOffset = sampleOffset + stream->startOffset;
    }
    return gkStreamSeek(stream->handle, sampleOffset, origin);
}

static int eofWavStream(gkAudioStream* s)
{
    gkWavAudioStream* stream = (gkWavAudioStream*)s;
    return gkStreamEnd(stream->handle);
}


/* MP3 stream through libmpg123 */

#ifdef GK_USE_MPG123

#define GK_MP3_SUPPORT

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

static ssize_t readMp3File(void* src, void* dst, size_t dstSize)
{
	gkStream* source = (gkStream*)src;
	return gkStreamRead(source, dst, dstSize);
}

static off_t seekMp3File(void* src, off_t offset, int origin)
{
	gkStream* source = (gkStream*)src;
	gkStreamSeek(source, offset, origin);
	return gkStreamTell(source);
}

static void closeMp3File(void* src)
{
	gkStream* source = (gkStream*)src;
	if(source)
		gkStreamClose(source);
}

static void openMp3Stream(mpg123_handle* mp3Handle, char* location)
{
	gkStream* source = gkOpenFile(location, "rb");
	mpg123_replace_reader_handle(mp3Handle, readMp3File, seekMp3File, closeMp3File);
	mpg123_open_handle(mp3Handle, source);
}

static gkAudioStream* createMp3AudioStream(char* location)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)malloc(sizeof(gkMp3AudioStream));
    stream->handle = mpg123_new(0, 0);
    stream->eof = GK_FALSE;
    openMp3Stream(stream->handle, location);
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
    info->channels = channels;
    info->bitsPerSample = bitsPerSample;
    info->sampleRate = sampleRate;
    info->length = (float)totalSamples/(float)sampleRate;
    info->streamSize = totalSamples*(bitsPerSample*channels)/8;
}

static int readMp3Stream(gkAudioStream* s, void* buffer, size_t bytes)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    size_t bytesRead;
    int res = mpg123_read(stream->handle, buffer, bytes, &bytesRead);
    if(res == MPG123_DONE) stream->eof = GK_TRUE;
    return bytesRead;
}

static int seekMp3Stream(gkAudioStream* s, size_t sampleOffset, int origin)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    off_t streamOffset = mpg123_seek(stream->handle, sampleOffset, origin);
    stream->eof = GK_FALSE;
    if(streamOffset>=0) return 0;
    return streamOffset;
}

static int eofMp3Stream(gkAudioStream* s)
{
    gkMp3AudioStream* stream = (gkMp3AudioStream*)s;
    return stream->eof;
}

#endif

/**/
/* OGG stream through libvorbisfile */

#ifdef GK_USE_OGGVORBIS

#define GK_OGG_SUPPORT

#include <vorbis/vorbisfile.h>

typedef struct _gkOggAudioStream gkOggAudioStream;
struct _gkOggAudioStream{
    gkAudioStream base;
    OggVorbis_File handle;
    GK_BOOL eof;
};

static int readOggStream(gkAudioStream* stream, void* buffer, size_t bytes);
static int seekOggStream(gkAudioStream* stream, size_t offset, int origin);
static void getOggStreamInfo(gkAudioStream* stream, gkAudioStreamInfo* info);
static int eofOggStream(gkAudioStream* stream);
static void destroyOggAudioStream(gkAudioStream* stream);


static size_t readOggFile(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	gkStream* source = (gkStream*)datasource;
	return gkStreamRead(source, ptr, size*nmemb);
}

static int seekOggFile(void *datasource, ogg_int64_t offset, int whence)
{
	gkStream* source = (gkStream*)datasource;
	return gkStreamSeek(source, offset, whence);
}

static long tellOggFile(void* datasource)
{
	gkStream* source = (gkStream*)datasource;
	return gkStreamTell(source);
}

static int closeOggFile(void* datasource)
{
	gkStream* source = (gkStream*)datasource;
	if(source)
		gkStreamClose(source);
	return 0;
}

static ov_callbacks gkOggCallbacks = {
	readOggFile,
	seekOggFile,
	closeOggFile,
	tellOggFile
};

static GK_BOOL openOggFile(char* location, OggVorbis_File* handle)
{
	gkStream* file = gkOpenFile(location, "rb");
	if (file) {
		/* ov_open_callbacks() returns 0 on success and -1 on failure */
		if(ov_open_callbacks(file, handle, 0, 0, gkOggCallbacks) == 0)
			return GK_TRUE;
		gkStreamClose(file);
	}
	return GK_FALSE;
}

static gkAudioStream* createOggAudioStream(char* location)
{
    gkOggAudioStream* stream = (gkOggAudioStream*)malloc(sizeof(gkOggAudioStream));
    if (!openOggFile(location, &stream->handle)) {
        free(stream);
        return 0;
    }
    stream->eof = GK_FALSE;
    stream->base.read = readOggStream;

    if(ov_seekable(&stream->handle) != 0)
        stream->base.seek = seekOggStream;
    else stream->base.seek = 0;

    stream->base.getInfo = getOggStreamInfo;
    stream->base.eof = eofOggStream;
    stream->base.destroy = destroyOggAudioStream;
    return (gkAudioStream*)stream;
}

static void destroyOggAudioStream(gkAudioStream* s)
{
    gkOggAudioStream* stream = (gkOggAudioStream*)s;
    ov_clear(&stream->handle);
    free(stream);
}

static void getOggStreamInfo(gkAudioStream* s, gkAudioStreamInfo* info)
{
    gkOggAudioStream* stream = (gkOggAudioStream*)s;
    vorbis_info* vinfo = ov_info(&stream->handle, -1);

	uint64_t totalSamples = ov_pcm_total(&stream->handle, -1);
    info->channels = vinfo->channels;
    info->bitsPerSample = 16;
    info->format = getAudioFormat(info->channels, info->bitsPerSample);
    info->sampleRate = vinfo->rate;
    info->length = ov_time_total(&stream->handle, -1);
    info->streamSize = totalSamples*(info->bitsPerSample*info->channels)/8;
}

static int readOggStream(gkAudioStream* s, void* buffer, size_t bytes)
{
    gkOggAudioStream* stream = (gkOggAudioStream*)s;
    int currentSection;
    size_t offset = 0;
    while(offset<bytes)
    {
        size_t bytesRead = ov_read(&stream->handle, (char*)buffer + offset, bytes - offset, 0, 2, 1, &currentSection);
        if(bytesRead == 0)
        {
            stream->eof = GK_TRUE;
            break;
        }
        if(bytesRead<0)
        {
            break;
        }
        offset += bytesRead;
    }
    return offset;
}

static int seekOggStream(gkAudioStream* s, size_t sampleOffset, int origin)
{
    gkOggAudioStream* stream = (gkOggAudioStream*)s;
    if(origin == SEEK_CUR)
        sampleOffset += ov_pcm_tell(&stream->handle);
    else if(origin == SEEK_END)
        sampleOffset = ov_pcm_total(&stream->handle, -1) - sampleOffset;
    if(ov_pcm_seek(&stream->handle, sampleOffset) == 0)
    {
        stream->eof = GK_FALSE;
        return sampleOffset;
    }
    return 0;
}

static int eofOggStream(gkAudioStream* s)
{
    gkOggAudioStream* stream = (gkOggAudioStream*)s;
    return stream->eof;
}

#endif

/* End of stream types */


void gkInitAudioStream()
{
#ifdef GK_USE_MPG123
    mpg123_init();
#endif
}

void gkCleanupAudioStream()
{
#ifdef GK_USE_MPG123
    mpg123_exit();
#endif
}

gkAudioStream* gkAudioStreamOpen(char* location)
{
	char* ext = location + (strlen(location) - 3);
	if(stricmp(ext,"wav") == 0)
		return createWavAudioStream(location);
#ifdef GK_MP3_SUPPORT
	else if(stricmp(ext, "mp3") == 0)
		return createMp3AudioStream(location);
#endif
#ifdef GK_OGG_SUPPORT
	else if(stricmp(ext, "ogg") == 0)
		return createOggAudioStream(location);
#endif
	return 0;
}

void gkAudioStreamClose(gkAudioStream* stream)
{
    stream->destroy(stream);
}
