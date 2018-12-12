LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := chinadrmdecrypt

LOCAL_C_INCLUDES:= $(LOCAL_PATH)

LOCAL_SRC_FILES := $(LOCAL_PATH)/ChinaDrmDecrypt.cpp

LOCAL_STATIC_LIBRARIES := $(LOCAL_PATH)/davinci-mpegts/libs/local/$(TARGET_ARCH_ABI)/libchinadrmdecrypt.a
LOCAL_STATIC_LIBRARIES += $(LOCAL_PATH)/monalisa-android-norach/libs/$(TARGET_ARCH_ABI)/libmonalisa-android-norach.a

include $(BUILD_SHARED_LIBRARY)