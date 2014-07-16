#include "gkStream.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct gkMemoryStream{
	gkStream base;
	uint8_t* mem;
	uint8_t* pos;
	size_t memSize;
	GK_BOOL freeOnClose;
}gkMemoryStream;

static size_t memStreamRead(gkStream* stream, void* buffer, size_t size)
{
	gkMemoryStream* memStream = (gkMemoryStream*)stream;
	size_t memLeft = memStream->memSize - (memStream->pos - memStream->mem);
	size_t memToRead = (size <= memLeft ? size : memLeft);

	if (memToRead>0) {
		memcpy(buffer, memStream->pos, memToRead);
		memStream->pos += memToRead;
	}

	return memToRead;
}

static int memStreamSeek(gkStream* stream, size_t offset, int origin)
{
	gkMemoryStream* memStream = (gkMemoryStream*)stream;
	if (origin == GK_SEEK_SET) {
		memStream->pos = memStream->mem + offset;
	} else if (origin == GK_SEEK_CUR) {
		memStream->pos += offset;
	} else if (origin == GK_SEEK_END) {
		memStream->pos = memStream->mem + (memStream->memSize - offset);
	}
	/* No Error checking !! */
	return 0;
}

static size_t memStreamTell(gkStream* stream)
{
	gkMemoryStream* memStream = (gkMemoryStream*)stream;
	return memStream->pos - memStream->mem;
}

static GK_BOOL memStreamEnd(gkStream* stream)
{
	gkMemoryStream* memStream = (gkMemoryStream*)stream;
	return memStream->memSize == (memStream->pos - memStream->mem);
}

static size_t memStreamWrite(gkStream* stream, const void* buffer, size_t size)
{
	gkMemoryStream* memStream = (gkMemoryStream*)stream;
	size_t memLeft = memStream->memSize - (memStream->pos - memStream->mem);
	size_t memToWrite = (size <= memLeft ? size : memLeft);

	if (memToWrite>0) {
		memcpy(memStream->pos, buffer, memToWrite);
		memStream->pos += memToWrite;
	}

	return memToWrite;
}

static void memStreamClose(gkStream* stream)
{
	gkMemoryStream* memStream = (gkMemoryStream*)stream;
	if (memStream->freeOnClose)
		free(memStream->mem);
	free(memStream);
}

gkStream* gkOpenMemory(void* mem, size_t memSize, GK_BOOL freeOnClose)
{
	gkMemoryStream* memStream = (gkMemoryStream*)malloc(sizeof(gkMemoryStream));

	memStream->freeOnClose = freeOnClose;
	memStream->mem = (uint8_t*)mem;
	memStream->memSize = memSize;

	memStream->pos = (uint8_t*)mem;

	memStream->base.seekable = GK_TRUE;
	memStream->base.read = memStreamRead;
	memStream->base.seek = memStreamSeek;
	memStream->base.tell = memStreamTell;
	memStream->base.end = memStreamEnd;
	memStream->base.write = memStreamWrite;
	memStream->base.close = memStreamClose;

	return (gkStream*)memStream;
}