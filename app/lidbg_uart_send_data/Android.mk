
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SRC_FILES:= \/par    lidbg_uart_send_data.c
LOCAL_MODULE_TAGS:= optional
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE := lidbg_uart_send_data
include $(BUILD_EXECUTABLE)
