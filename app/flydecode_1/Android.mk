
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils libbinder libmedia libpowermanager
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:= \/par    flydecode_1.c

ifeq ($(call is-platform-sdk-version-at-least,21),true)
$(warning $(PLATFORM_VERSION)===$(PLATFORM_SDK_VERSION))
LOCAL_CFLAGS += -DANDROID_AT_LEAST_50
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/media
else
$(warning 4.4===$(PLATFORM_SDK_VERSION))
endif

include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := flydecode_1
include $(BUILD_EXECUTABLE)
