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
LOCAL_MODULE            := MPG123
LOCAL_SRC_FILES := $(LIBGK_HOME)/external/libmpg123/lib/android/libMPG123.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := OGGVORBIS
LOCAL_SRC_FILES := $(LIBGK_HOME)/external/libvorbis/lib/android/libOGGVORBIS.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := GK
LOCAL_SRC_FILES := $(LIBGK_HOME)/bin/android/libGK.a
LOCAL_EXPORT_C_INCLUDES := $(LIBGK_HOME)/include/
LOCAL_EXPORT_LDLIBS := -llog -landroid -lEGL -lGLESv1_CM -lz -lOpenSLES
LOCAL_WHOLE_STATIC_LIBRARIES  := android_native_app_glue JPEG PNG freetype2 MPG123 OGGVORBIS

LOCAL_SHARED_LIBRARIES += MPG123

include $(PREBUILT_STATIC_LIBRARY)

$(call import-module,android/native_app_glue)