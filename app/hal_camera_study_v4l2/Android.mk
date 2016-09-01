LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MULTILIB := first
LOCAL_MODULE := libImageProc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SRC_FILES := ImageProc.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libjnigraphics
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
