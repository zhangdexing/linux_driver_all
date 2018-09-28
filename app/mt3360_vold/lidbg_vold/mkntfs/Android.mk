LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	src/attrdef.c \
	src/boot.c \
	src/utils.c \
	src/mkntfs.c \
	src/sd.c

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/src $(LOCAL_PATH)/include
LOCAL_CFLAGS:= -O2 -g -W -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CONFIG_H
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_MODULE:= mkntfs
LOCAL_MODULE_TAGS:=eng
LOCAL_SYSTEM_SHARED_LIBRARIES:= libc
LOCAL_STATIC_LIBRARIES:= libntfs-3g libfuse

include $(BUILD_EXECUTABLE)
