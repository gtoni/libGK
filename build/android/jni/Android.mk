LOCAL_PATH := $(call my-dir)/../../../

include $(CLEAR_VARS)

LOCAL_MODULE    := GK
LOCAL_SRC_FILES := 	src/event.c\
					src/fonts.c\
					src/gkApplication.c\
					src/gkAudio.c\
					src/gkAudioStream.c\
					src/gkAudioSystemNull.c\
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

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv1_CM

LOCAL_EXPORT_LDLIBS := -llog -landroid -lEGL -lGLESv1_CM

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_STATIC_LIBRARIES += JPEG
LOCAL_STATIC_LIBRARIES += PNG
					
include $(BUILD_SHARED_LIBRARY)

$(call import-module,third_party/libpng)
$(call import-module,third_party/libjpeg)
$(call import-module,android/native_app_glue)
