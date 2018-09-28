LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := SleepTest
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PACKAGE)

#ifeq ($(strip $(LOCAL_PACKAGE_OVERRIDES)),)
include $(call all-makefiles-under,$(LOCAL_PATH))
#endif
