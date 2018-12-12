APP_PROJECT_PATH := $(call my-dir)
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/Android.mk
#APP_ABI := all
APP_ABI := armeabi
APP_STL := gnustl_shared
APP_CFLAGS += -fexceptions -frtti