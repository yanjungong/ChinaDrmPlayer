APP_PROJECT_PATH := $(call my-dir)
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/Android.mk
#APP_ABI := all
APP_ABI := armeabi
APP_CFLAGS += -fexceptions -frtti

#APP_STL := gnustl_shared
APP_CPPFLAGS += -Wno-error=format-security -std=c++11
APP_OPTIM := release
APP_PLATFORM := android-14