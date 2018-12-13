LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Prebuilt.mk
include $(CLEAR_VARS)

LOCAL_MODULE := chinadrmdecrypt
LOCAL_C_INCLUDES:= $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES :=  davinci_mpegts_prebuilt monalisa_prebuilt
LOCAL_SRC_FILES := $(LOCAL_PATH)/ChinaDrmDecrypt.cpp  ChinaDrmParser.cpp Error.cpp

LOCAL_LDLIBS:=-L$(SYSROOT)/usr/lib -llog

LOCAL_CERTIFICATE := platform
include $(BUILD_SHARED_LIBRARY)