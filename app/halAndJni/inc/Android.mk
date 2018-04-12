

include $(CLEAR_VARS)
LOCAL_MODULE := lidbgsdk.jar
LOCAL_SRC_FILES := ../inc/lidbgsdk.jar
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

