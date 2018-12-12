LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := chinadrmdecrypt

LOCAL_SRC_FILES := cgts_demux.c cgts_mux.c cgts_structs.c cgts_nal_adts_parse.c cgts_util.c
LOCAL_CFLAGS   += -std=c99
LOCAL_CFLAGS += -pie -fPIE
LOCAL_LDFLAGS += -pie -fPIE
include $(BUILD_STATIC_LIBRARY)