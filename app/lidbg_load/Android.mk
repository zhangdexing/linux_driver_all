
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:=     lidbg_load.c

LOCAL_MODULE_TAGS:= optional
LOCAL_STRIP_MODULE :=false
LOCAL_MODULE := lidbg_load
include $(BUILD_EXECUTABLE)
