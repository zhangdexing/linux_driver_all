LOCAL_PATH:= $(call my-dir)
###################################################################

##For stage1, we have to make libexfat

###################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	libexfat/cluster.c \
	libexfat/io.c \
	libexfat/log.c \
	libexfat/lookup.c \
	libexfat/mount.c \
	libexfat/node.c \
	libexfat/time.c \
	libexfat/utf.c \
	libexfat/utils.c

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/libexfat system/libfuse/include
LOCAL_CFLAGS:= -O2 -g -W -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -UUSE_UBLIO
LOCAL_MODULE:= libexfat
LOCAL_MODULE_TAGS:=eng
LOCAL_SYSTEM_SHARED_LIBRARIES:= libc libcutils

include $(BUILD_STATIC_LIBRARY)

###################################################################

##For stage2, we make exfat

###################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	fuse/main.c


LOCAL_CFLAGS:= -O2 -g -W -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DUSE_UBLIO
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_MODULE:= mkfs.exfat
LOCAL_MODULE_TAGS:=eng
LOCAL_SYSTEM_SHARED_LIBRARIES:= libc
LOCAL_STATIC_LIBRARIES:= libfuse libexfat

#include $(BUILD_EXECUTABLE)
