LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libexfat.so
LOCAL_SRC_FILES := lib/libexfat.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES 
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libfuse.so
LOCAL_SRC_FILES := lib/libfuse.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES 
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

