LOCAL_PATH:= $(call my-dir)

common_src_files := \
	lidbg_vold/Lidbg_vold.cpp\
	VolumeManager.cpp \
	CommandListener.cpp \
	VoldCommand.cpp \
	NetlinkManager.cpp \
	NetlinkHandler.cpp \
	Volume.cpp \
	DirectVolume.cpp \
	Process.cpp \
	Ext4.cpp \
	Fat.cpp \
	Ntfs.cpp \
	Loop.cpp \
	Devmapper.cpp \
	ResponseCode.cpp \
	Xwarp.cpp \
	VoldUtil.c \
	fstrim.c \
	cryptfs.c\
	lidbg_vold/Exfat.cpp 

common_c_includes := \
	$(KERNEL_HEADERS) \
	system/extras/ext4_utils \
	external/openssl/include \
	external/stlport/stlport \
	bionic \
	external/scrypt/lib/crypto \
    external/e2fsprogs/lib \
    external/icu4c/common 

common_shared_libraries := \
	libsysutils \
	libstlport \
	libcutils \
	liblog \
	libdiskconfig \
	libhardware_legacy \
	liblogwrap \
	libext4_utils \
	libcrypto \
    libicuuc 

common_static_libraries := \
	libfs_mgr \
	libscrypt_static \
	libmincrypt \
    libext2_blkid \
    libext2_uuid

include $(CLEAR_VARS)

LOCAL_MODULE := libvold

LOCAL_SRC_FILES := $(common_src_files)

LOCAL_C_INCLUDES := $(common_c_includes)

ifeq ($(strip $(BUILD_WITH_MULTI_USB_PARTITIONS)), true)
LOCAL_CFLAGS += -DSUPPORTED_MULTI_USB_PARTITIONS
endif

LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

LOCAL_STATIC_LIBRARIES := $(common_static_libraries)

LOCAL_MODULE_TAGS := eng tests

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_MODULE:= vold

LOCAL_SRC_FILES := \
	main.cpp \
	$(common_src_files)

LOCAL_C_INCLUDES := $(common_c_includes)

LOCAL_CFLAGS := -Werror=format
ifeq ($(strip $(BUILD_WITH_MULTI_USB_PARTITIONS)), true)
LOCAL_CFLAGS += -DSUPPORTED_MULTI_USB_PARTITIONS
endif

LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)
LOCAL_SRC_FILES := \
       MiscManager.cpp \
	   Misc.cpp \
	   G3Dev.cpp \
	   $(LOCAL_SRC_FILES)
LOCAL_CFLAGS += -DUSE_USB_MODE_SWITCH
LOCAL_STATIC_LIBRARIES := $(common_static_libraries)

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= vdc.c
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_MODULE:= vdc

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_CFLAGS := 

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under, $(LOCAL_PATH))


