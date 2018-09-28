LOCAL_PATH := $(call my-dir)

#ifneq ($(BOARD_USES_GPS_AH0808),true)
#ifeq ($(BOARD_GPS_LIBRARIES), libgps)

#ifneq ($(TARGET_PRODUCT),sim)
#ifeq ($(BOARD_USES_GPS_TYPE),mtk3329)

# HAL module implemenation, not prelinked and stored in
# # hw/<GPS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_CFLAGS += -DHAVE_GPS_HARDWARE
LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware libc libutils
LOCAL_SRC_FILES := gps_qemu.c
#LOCAL_MODULE := gps.$(TARGET_BOARD_PLATFORM)
#LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := gps.$(DBG_PLATFORM)
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
include $(BUILD_SHARED_LIBRARY)

#endif
#endif

#endif
#endif
