LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(DBG_DRIVERS_PATH)/inc
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)
LOCAL_MULTILIB := first
LOCAL_SRC_FILES := lidbg_jni.cpp
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES := liblog libcutils liblog libhardware libbinder libandroid_runtime libutils libnativehelper
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := liblidbg_jni
include $(BUILD_SHARED_LIBRARY)

