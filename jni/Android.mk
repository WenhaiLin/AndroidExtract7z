LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := AndroidExtract7z
LOCAL_MODULE_FILENAME := libAndroidExtract7z

LOCAL_SRC_FILES := ../7zAlloc.c \
                   ../7zArcIn.c \
                   ../7zBuf.c \
                   ../7zBuf2.c \
                   ../7zCrc.c \
                   ../7zCrcOpt.c \
                   ../7zDec.c \
                   ../CpuArch.c \
                   ../Delta.c \
                   ../LzmaDec.c \
                   ../Lzma2Dec.c \
                   ../Bra.c \
                   ../Bra86.c \
                   ../BraIA64.c \
                   ../Bcj2.c \
                   ../Ppmd7.c \
                   ../Ppmd7Dec.c \
                   ../7zFile.c \
                   ../7zStream.c \
                   ../Util/7z/7zMain.cpp \
                   ../Util/JniHelper.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

