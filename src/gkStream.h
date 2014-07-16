#ifndef _GK_STREAM_H_
#define _GK_STREAM_H_

#include <gkConfig.h>
#include <gkTypes.h>

#define GK_SEEK_SET	SEEK_SET
#define GK_SEEK_CUR	SEEK_CUR
#define GK_SEEK_END	SEEK_END

typedef struct gkStream{
	GK_BOOL seekable;
	
	GK_METHOD(size_t, read, (struct gkStream* stream, void* buffer, size_t size));
	GK_METHOD(int, seek, (struct gkStream* stream, size_t offset, int origin));
	GK_METHOD(size_t, tell, (struct gkStream* stream));
	GK_METHOD(GK_BOOL, end, (struct gkStream* stream));
	GK_METHOD(size_t, write, (struct gkStream* stream, const void* buffer, size_t size));
	GK_METHOD(void, close, (struct gkStream* stream));
}gkStream;

size_t	gkStreamRead(gkStream *stream, void *buffer, size_t size);
int	gkStreamSeek(gkStream *stream, size_t offset, int origin);
size_t	gkStreamTell(gkStream *stream);
GK_BOOL	gkStreamEnd(gkStream *stream);
size_t	gkStreamWrite(gkStream *stream, const void *buffer, size_t size);
void	gkStreamClose(gkStream *stream);

/* Check if a file path is relative to the app */
GK_BOOL gkIsPathRelative(char* path);

/* File stream */
gkStream* gkOpenFile(char* filename, char* mode);

GK_BOOL gkReadFile(char* filename, void** outDst, size_t* outDstSize);

/* Memory stream */
gkStream* gkOpenMemory(void* mem, size_t memSize, GK_BOOL freeOnClose);

#endif