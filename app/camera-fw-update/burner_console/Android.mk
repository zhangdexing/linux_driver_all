LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc \
		    $(KERNEL_HEADERS)
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDFLAGS += $(DBG_OUT_PATH)/libusb01.so
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:=  \
	../BurnerApLib/BurnerApLib.cpp \
	../common/debug.cpp \
	../common/CamEnum.cpp \
	../common/misc.cpp \
	../BurnMgr/FW_File.cpp \
	../BurnMgr/BurnMgr.cpp \
	main.cpp
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := fw_update
include $(BUILD_EXECUTABLE)
