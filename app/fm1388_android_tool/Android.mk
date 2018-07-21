LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MULTILIB := 32
LOCAL_MODULE_PATH := $(LOCAL_PATH)/tool
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SRC_FILES := libfmrec_1388/libfmrec.c libfmrec_1388/fm_wav.c
LOCAL_MODULE := libfmrec_1388
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(LOCAL_PATH)/tool
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SRC_FILES := libfm1388/libfm1388.c
LOCAL_MODULE := libfm1388
LOCAL_SHARED_LIBRARIES := libfmrec_1388
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_SRC_FILES:= fm_fm1388/fm_fm1388.c
LOCAL_STRIP_MODULE :=false
LOCAL_MODULE_PATH := $(LOCAL_PATH)/tool
LOCAL_MODULE := fm_fm1388_
LOCAL_SHARED_LIBRARIES := libfmrec_1388 libfm1388
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
$(warning LOCAL_PATH = $(LOCAL_PATH))
LOCAL_MULTILIB := 32
LOCAL_MODULE := libFM1388Parameter
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(LOCAL_PATH)/tool
LOCAL_SRC_FILES := FM1388_Parameter_Lib/libFM1388Parameter.c 
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := FM1388_ADB_Tool_
LOCAL_MULTILIB := 32
LOCAL_STRIP_MODULE :=false
LOCAL_SRC_FILES := FM1388_ADB_Tool/FM1388_ADB_Tool.c FM1388_ADB_Tool/FM1388_Play_Record.c
LOCAL_SHARED_LIBRARIES := libFM1388Parameter libfm1388
LOCAL_MODULE_PATH := $(LOCAL_PATH)/tool
include $(BUILD_EXECUTABLE)
