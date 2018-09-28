LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	src/fuse.c \
	src/fusermount.c \
	src/fuse_kern_chan.c \
	src/fuse_loop.c \
	src/fuse_lowlevel.c \
	src/fuse_opt.c \
	src/fuse_session.c \
	src/fuse_signals.c \
	src/helper.c \
	src/mount.c \
	src/mount_util.c

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/include
LOCAL_CFLAGS:= -O2 -g -W -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CONFIG_H
LOCAL_MODULE:= libfuse
LOCAL_MODULE_TAGS:=eng
LOCAL_SYSTEM_SHARED_LIBRARIES:= libc libcutils

include $(BUILD_STATIC_LIBRARY)
