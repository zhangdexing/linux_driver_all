SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,doc_split)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,doc_filter)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,apps)
ifeq ($(CONFIG_HAL_VOLD_8996_vold_60),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8996_vold_60)
endif

SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_uevent_cold_boot)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,flydecode_1)
ifeq ($(DBG_VENDOR),VENDOR_QCOM)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,getQcomBand)
endif
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,i2c-tools-3.1.1)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,halAndJni/hal)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,halAndJni/jni)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,halAndJni/inc)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,record_klogctl)

SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_umount)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,apk)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,Gsensor_Handler)

ifeq ($(CONFIG_HAL_ANDROID_SERVER),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_android_server)
endif

ifeq ($(CONFIG_HAL_LOAD),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_load)
endif

ifeq ($(CONFIG_HAL_USERVER),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_userver)
endif

ifeq ($(CONFIG_HAL_GPSLIB),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,gpslib)
endif

ifeq ($(CONFIG_HAL_PX3_GPS),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,px3_gps)
endif

ifeq ($(CONFIG_HAL_SERVER),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_servicer)
endif

ifeq ($(CONFIG_HAL_UART_SEND_DATA),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_uart_send_data)
endif

ifeq ($(CONFIG_HAL_PARTED),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_parted)
endif

ifeq ($(CONFIG_HAL_USB_CAMERA_PREVIEW),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,hal_camera_usb)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,hal_camera_study_v4l2)
endif

ifeq ($(CONFIG_HAL_USB_CAMERA_PREVIEW_2),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,hal_camera_usb_2)
endif

ifeq ($(CONFIG_HAL_USB_CAMERA),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,uvccam_test)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,libusb01/android/jni)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,fly_dvr)
endif

ifeq ($(CONFIG_HAL_USB_CAMERA),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,camera-fw-update)
endif

ifeq ($(CONFIG_HAL_BOOTANIMATION),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,bootanimation)
endif

ifeq ($(CONFIG_HAL_BOOTANIMATION_6),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,bootanimation_6.0)
endif

ifeq ($(CONFIG_HAL_VOLD_8x25Q),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x25q_vold)
endif

ifeq ($(CONFIG_HAL_CAMERA_8x25Q),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x25q_camera)
endif

ifeq ($(CONFIG_HAL_VOLD_8x26),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x26_vold)
endif

ifeq ($(CONFIG_HAL_VOLD_8x26_5_0),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x26_vold_5.0)
endif

ifeq ($(CONFIG_HAL_VOLD_8x26_6_0),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x26_vold_6.0)
endif

ifeq ($(CONFIG_HAL_VOLD_mt3360),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,mt3360_vold)
endif

ifeq ($(CONFIG_HAL_VOLD_PX3),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,px3_vold)
endif

ifeq ($(CONFIG_HAL_VOLD_MT3561),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,mt35x_vold)
endif

include $(SUBDIR_MAKEFILES)
