LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := fsck.exfat
LOCAL_SRC_FILES := bin/fsck.exfat
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mkfs.exfat
LOCAL_SRC_FILES := bin/mkfs.exfat
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(call all-makefiles-under, $(LOCAL_PATH))
