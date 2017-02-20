LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc \
		    $(LOCAL_PATH)/api \
		    $(LOCAL_PATH)/ui \
		    $(LOCAL_PATH)/os/qcom \
		    $(LOCAL_PATH)/vendor/sonix/rec \
		    $(LOCAL_PATH)/vendor/sonix/ \
		    $(KERNEL_HEADERS) 
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SHARED_LIBRARIES += libcutils libutils
LOCAL_LDFLAGS += $(DBG_OUT_PATH)/libusb01.so
LOCAL_LDLIBS += -llog
LOCAL_SRC_FILES:=  \
	fly_dvr.cpp \
	api/Flydvr_General.cpp \
	api/Flydvr_ISPIF.cpp \
	api/Flydvr_Parameter.cpp \
	api/Flydvr_Version.cpp \
	api/Flydvr_OS.cpp \
	api/Flydvr_Menu.cpp \
	api/Flydvr_USB.cpp \
	api/Flydvr_Media.cpp \
	ui/StateVideoFunc.cpp \
	ui/StateACCFunc.cpp \
	ui/MenuSetting.cpp \
	os/qcom/FLY_MSM_OS_Pipe.cpp \
	os/qcom/FLY_MSM_OS_Media.cpp \
	os/qcom/FLY_MSM_OS_Fs.cpp \
	vendor/sonix/fw/BurnerApLib/BurnerApLib.cpp \
	vendor/sonix/fw/common/debug.cpp \
	vendor/sonix/fw/common/CamEnum.cpp \
	vendor/sonix/fw/common/misc.cpp \
	vendor/sonix/fw/BurnMgr/FW_File.cpp \
	vendor/sonix/fw/BurnMgr/BurnMgr.cpp \
	vendor/sonix/fw/burner_console/main.cpp \
	vendor/sonix/rec/sonix_xu_ctrls.cpp \
	vendor/sonix/rec/v4l2uvc.cpp \
	vendor/sonix/rec/nalu.cpp \
	vendor/sonix/rec/cap_desc_parser.cpp \
	vendor/sonix/rec/cap_desc.cpp \
	vendor/sonix/Sonix_ISPIF.cpp
include $(DBG_BUILD_PATH)/build_cfg.mk
LOCAL_MULTILIB := first
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := lidbg_flydvr
include $(BUILD_EXECUTABLE)
