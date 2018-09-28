LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc \
		    $(KERNEL_HEADERS) 
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:=  \
	SONiX_UVC_TestAP.c \
	sonix_xu_ctrls.c \
	v4l2uvc.c \
	nalu.c \
	cap_desc_parser.c \
	cap_desc.c 
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := lidbg_testuvccam
include $(BUILD_EXECUTABLE)
