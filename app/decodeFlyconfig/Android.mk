LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# our own branch needs these headers

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
#LOCAL_MODULE_RELATIVE_PATH := hw\

LOCAL_CFLAGS += -DANDROID_PLATFORM

LOCAL_SHARED_LIBRARIES := liblog \
						  libcutils \
						  libhardware \
						  libutils
									  
LOCAL_LDLIBS:=  -L$(SYSROOT)/usr/lib -llog 

LOCAL_SRC_FILES := decodeFlyconfig.c

LOCAL_C_INCLUDES += $(LOCAL_PATH) \
					$(LOCAL_PATH)/minzip

LOCAL_STATIC_LIBRARIES += libmtdutils libminzip libz
LOCAL_STATIC_LIBRARIES += libmincrypt libbz
LOCAL_STATIC_LIBRARIES += libcutils liblog libc
LOCAL_STATIC_LIBRARIES += libselinux

LOCAL_MODULE := decodeFlyconfig

LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/minzip/Android.mk