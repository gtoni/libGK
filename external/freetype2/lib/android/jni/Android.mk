LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := freetype2
LOCAL_SRC_FILES := 	ftbase.c ftbbox.c ftbitmap.c ftdebug.c ftglyph.c ftinit.c ftstroke.c ftsystem.c psnames.c raster.c sfnt.c smooth.c truetype.c

LOCAL_CFLAGS := -DFT2_BUILD_LIBRARY
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../include/android/
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../include/android/
		
include $(BUILD_STATIC_LIBRARY)