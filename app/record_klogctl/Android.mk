
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:= \/par    record_klogctl.c
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := record_klogctl 
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:= \/par    sendsignal.c
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := sendsignal 
include $(BUILD_EXECUTABLE)
