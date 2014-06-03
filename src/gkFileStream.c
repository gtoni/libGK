#include "gkStream.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct gkFileStream
{
	gkStream base;
	FILE* file;
}gkFileStream;

static size_t fileStreamRead(gkStream* stream, void* buffer, size_t size)
{
	gkFileStream* fileStream = (gkFileStream*)stream;

	size_t res = fread(buffer, sizeof(char), size, fileStream->file);
	if(res == 0)
		return ferror(fileStream->file) ? -1: 0;
	return res;
}

static int fileStreamSeek(gkStream* stream, size_t offset, int origin)
{
	gkFileStream* fileStream = (gkFileStream*)stream;
	return fseek(fileStream->file, offset, origin);
}

static size_t fileStreamTell(gkStream* stream)
{
	gkFileStream* fileStream = (gkFileStream*)stream;
	return ftell(fileStream->file);
}

static GK_BOOL fileStreamEnd(gkStream* stream)
{
	gkFileStream* fileStream = (gkFileStream*)stream;
	return feof(fileStream->file);
}

static size_t fileStreamWrite(gkStream* stream, const void* buffer, size_t size)
{
	gkFileStream* fileStream = (gkFileStream*)stream;

	size_t res = fwrite(buffer, sizeof(char), size, fileStream->file);
	if(res == 0)
		return ferror(fileStream->file) ? -1: 0;
	return res;
}

static void fileStreamClose(gkStream* stream)
{
	gkFileStream* fileStream = (gkFileStream*)stream;
	fclose(fileStream->file);
	free(fileStream);
}

gkStream* gkOpenFileStd(char *filename, char *mode)
{
	FILE* file = fopen(filename, mode);
	gkFileStream* fileStream;

	if (!file) 
		return 0;

	fileStream = (gkFileStream*)malloc(sizeof(gkFileStream));
	fileStream->file = file;
	fileStream->base.seekable = GK_TRUE;
	fileStream->base.read = fileStreamRead;
	fileStream->base.seek = fileStreamSeek;
	fileStream->base.tell = fileStreamTell;
	fileStream->base.end = fileStreamEnd;
	fileStream->base.write = fileStreamWrite;
	fileStream->base.close = fileStreamClose;
	return (gkStream*)fileStream;
}

#ifdef GK_PLATFORM_ANDROID
gkStream* gkOpenFileAndroid(char *filename, char *mode);
#endif

gkStream* gkOpenFile(char *filename, char *mode)
{
#ifdef GK_PLATFORM_ANDROID
	if (gkIsPathRelative(filename))
		return gkOpenFileAndroid(filename, mode);
#endif
	return gkOpenFileStd(filename, mode);
}