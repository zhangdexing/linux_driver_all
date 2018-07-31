LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(warning LOCAL_PATH = $(LOCAL_PATH))
LOCAL_MODULE := FM1388_ADB_Tool

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := FM1388_ADB_Tool.c FM1388_Play_Record.c FM1388_CommandLine_Parser.c FM1388_Read_Write_Data.c
LOCAL_SHARED_LIBRARIES := libFM1388Parameter libfm1388 libfmrec_1388

include $(BUILD_EXECUTABLE)


