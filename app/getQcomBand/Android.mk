
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_CFLAGS += -Werror -Wall -Wextra
LOCAL_MODULE := getQcomBand
LOCAL_C_INCLUDES :=  \
		     $(DBG_SYSTEM_DIR)/vendor/qcom/proprietary/qmi/services \
		     $(DBG_SYSTEM_DIR)/vendor/qcom/proprietary/qcril-qmi-services \
		     $(DBG_SYSTEM_DIR)/vendor/qcom/proprietary/qmi/core/lib/inc \
		     $(DBG_SYSTEM_DIR)/vendor/qcom/proprietary/qmi/src \
		     $(DBG_SYSTEM_DIR)/vendor/qcom/proprietary/qmi/platform \
		     $(DBG_SYSTEM_DIR)/KERNEL_OBJ/usr/include \
		     $(DBG_SYSTEM_DIR)/vendor/qcom/proprietary/qmi/inc \
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_SHARED_LIBRARIES := libqmi libqcci_legacy libqmiservices libcutils
LOCAL_SRC_FILES := getQcomBand.c
LOCAL_MODULE_TAGS := optional

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
include $(BUILD_EXECUTABLE)
