LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(DBG_BUILD_PATH)/build_cfg.mk

LOCAL_SRC_FILES := LidbgCameraMain.cpp LidbgCameraUsb.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \
        frameworks/native/include/media/openmax \
        hardware/qcom/display/libgralloc \
        hardware/qcom/media/libstagefrighthw \
        
LOCAL_C_INCLUDES += system/media/camera/include
LOCAL_C_INCLUDES += hardware/qcom/display/libgralloc

ifeq ($(call is-platform-sdk-version-at-least,21),true)
$(warning 5.0===$(PLATFORM_SDK_VERSION))
LOCAL_CFLAGS += -DANDROID_AT_LEAST_50
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/media
else
$(warning 4.4===$(PLATFORM_SDK_VERSION))
endif

LOCAL_MULTILIB := first

LOCAL_SHARED_LIBRARIES :=  libutils liblog libcutils  libcamera_client 

LOCAL_MODULE := camera.$(DBG_PLATFORM)
#LOCAL_MODULE := camera.default
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
