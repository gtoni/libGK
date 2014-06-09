LOCAL_PATH := $(call my-dir)/../../../

include $(CLEAR_VARS)
LOCAL_MODULE            := JPEG
LOCAL_SRC_FILES := $(LIBGK_HOME)/external/jpeg/lib/android/libJPEG.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := PNG
LOCAL_SRC_FILES := $(LIBGK_HOME)/external/png/lib/android/libPNG.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := freetype2
LOCAL_SRC_FILES := $(LIBGK_HOME)/external/freetype2/lib/android/libfreetype2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := GK
LOCAL_SRC_FILES := 	src/event.c\
					src/fonts.c\
					src/gkApplication.c\
					src/gkAudio.c\
					src/gkAudioStream.c\
					src/gkAudioSystemOpenSLES.c\
					src/gkDrawToImage.c\
					src/gkGeometry.c\
					src/gkGraphics.c\
					src/gkImage.c\
					src/gkImageDecoder.c\
					src/gkImageEncoder.c\
					src/gkImageType.c\
					src/gkList.c\
					src/gkPlatformAndroid.c\
					src/gkUtf8.c\
					src/input.c\
					src/panel.c\
					src/timer.c\
					src/tween.c\
					src/gkStream.c\
					src/gkFileStream.c\
					src/gkFileStreamAndroid.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/ 
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/
LOCAL_C_INCLUDES += $(LIBGK_HOME)/external/png/include/android/
LOCAL_C_INCLUDES += $(LIBGK_HOME)/external/jpeg/include/android/
LOCAL_C_INCLUDES += $(LIBGK_HOME)/external/freetype2/include/android/
LOCAL_C_INCLUDES += $(LIBGK_HOME)/external/libmpg123/include/android/
LOCAL_C_INCLUDES += $(LIBGK_HOME)/external/libvorbis/include/android/

LOCAL_C_FLAGS := -O3 -ffast-math
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include/

LOCAL_EXPORT_LDLIBS := -llog -landroid -lEGL -lGLESv1_CM -lz -lOpenSLES

LOCAL_WHOLE_STATIC_LIBRARIES  := android_native_app_glue JPEG PNG freetype2
					
include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/native_app_glue)