LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= fm_fm1388.c
LOCAL_MODULE := fm_fm1388
LOCAL_SHARED_LIBRARIES := libfmrec_1388 libfm1388
include $(BUILD_EXECUTABLE)

