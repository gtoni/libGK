#include "gkStream.h"

size_t gkStreamRead(gkStream* stream, void *buffer, size_t size)
{
	return stream->read(stream, buffer, size);
}

int gkStreamSeek(gkStream* stream, size_t offset, int origin)
{
	return stream->seek(stream, offset, origin);
}

size_t gkStreamTell(gkStream* stream)
{
	return stream->tell(stream);
}

GK_BOOL gkStreamEnd(gkStream* stream)
{
	return stream->end(stream);
}

size_t gkStreamWrite(gkStream* stream, const void *buffer, size_t size)
{
	return stream->write(stream, buffer, size);
}

void gkStreamClose(gkStream* stream)
{
	stream->close(stream);
}

GK_BOOL gkIsPathRelative(char* path)
{
	if (path[1] == ':' || path[0] == '/')
		return GK_FALSE;
	return GK_TRUE;
}