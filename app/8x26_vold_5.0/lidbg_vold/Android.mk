LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mkfs.exfat
LOCAL_SRC_FILES := bin/mkfs.exfat
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := chkntfs
LOCAL_SRC_FILES := bin/chkntfs
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := fsck.exfat
LOCAL_SRC_FILES := bin/fsck.exfat
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mkntfs
LOCAL_SRC_FILES := bin/mkntfs
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ntfs-3g
LOCAL_SRC_FILES := bin/ntfs-3g
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional debug eng tests samples
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_PREBUILT)

