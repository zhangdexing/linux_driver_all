LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE := gps.$(DBG_PLATFORM)
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_MULTILIB := first
LOCAL_SRC_FILES := gps_ublox.c
LOCAL_SHARED_LIBRARIES := liblog libcutils
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
