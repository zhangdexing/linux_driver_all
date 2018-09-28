
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:= \/par    lidbg_parted.c
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := lidbg_parted
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := parted
LOCAL_SRC_FILES := bin/parted
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)
