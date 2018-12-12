echo off

rd /s /q .\obj\

ndk-build NDK_APP_OUT=./libs APP_BUILD_SCRIPT=./Android.mk NDK_PROJECT_PATH=./

pause