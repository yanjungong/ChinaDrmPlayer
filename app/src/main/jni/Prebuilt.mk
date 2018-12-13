LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := monalisa_prebuilt
LOCAL_SRC_FILES := monalisa-android-norach/libs/$(TARGET_ARCH_ABI)/libmonalisa-android-norach.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := davinci_mpegts_prebuilt
LOCAL_SRC_FILES := davinci-mpegts/libs/local/$(TARGET_ARCH_ABI)/libchinadrmdecrypt.a
include $(PREBUILT_STATIC_LIBRARY)
