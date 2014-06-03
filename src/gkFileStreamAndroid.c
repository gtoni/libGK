#include "gkStream.h"

#ifdef GK_PLATFORM_ANDROID

#include <android/log.h>
#include <android_native_app_glue.h>

extern struct android_app* gkAndroidApp;

typedef struct gkAssetStream
{
	gkStream base;
	size_t cpos;
	int eof;
	AAsset* asset;
}gkAssetStream;

static size_t assetStreamRead(gkStream* stream, void* buffer, size_t size)
{
	gkAssetStream* assetStream = (gkAssetStream*)stream;

	int res = AAsset_read(assetStream->asset, buffer, sizeof(char)*size);
	if (res > 0)
		assetStream->cpos += res;
	if (res == 0)
		assetStream->eof = GK_TRUE;
	return res;
}

static int assetStreamSeek(gkStream* stream, size_t offset, int origin)
{
	gkAssetStream* assetStream = (gkAssetStream*)stream;
	int res = AAsset_seek(assetStream->asset, offset, origin);
	if (res >= 0) {
		assetStream->cpos = res;
		return 0;
	}
	return -1;
}

static size_t assetStreamTell(gkStream* stream)
{
	gkAssetStream* assetStream = (gkAssetStream*)stream;
	return assetStream->cpos;
}

static GK_BOOL assetStreamEnd(gkStream* stream)
{
	gkAssetStream* assetStream = (gkAssetStream*)stream;
	return assetStream->eof;
}

static size_t assetStreamWrite(gkStream* stream, const void* buffer, size_t size)
{
	return 0; //Can't write to android assets
}

static void assetStreamClose(gkStream* stream)
{
	gkAssetStream* assetStream = (gkAssetStream*)stream;
	AAsset_close(assetStream->asset);
	free(assetStream);
}

gkStream* gkOpenFileAndroid(char *filename, char *mode)
{
	char* fn = (strncmp(filename, "assets", 6) == 0)?filename+7:filename;
	AAsset* asset = AAssetManager_open(gkAndroidApp->activity->assetManager, 
		fn, AASSET_MODE_UNKNOWN);

	gkAssetStream* assetStream;

	if (!asset) 
		return 0;

	assetStream = (gkAssetStream*)malloc(sizeof(gkAssetStream));
	assetStream->asset = asset;
	assetStream->cpos = 0;
	assetStream->eof = GK_FALSE;
	assetStream->base.seekable = GK_TRUE;
	assetStream->base.read = assetStreamRead;
	assetStream->base.seek = assetStreamSeek;
	assetStream->base.tell = assetStreamTell;
	assetStream->base.end = assetStreamEnd;
	assetStream->base.write = assetStreamWrite;
	assetStream->base.close = assetStreamClose;
	return (gkStream*)assetStream;
}

#endif