LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
$(warning LOCAL_PATH = $(LOCAL_PATH))

LOCAL_MODULE := libFM1388Parameter
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_SRC_FILES := libFM1388Parameter.c 
LOCAL_SHARED_LIBRARIES := libfm1388 libfmrec_1388

include $(BUILD_SHARED_LIBRARY)

