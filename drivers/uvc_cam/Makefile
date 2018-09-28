include $(DBG_ROOT_PATH)/build/build.mk
include $(DBG_DRIVERS_PATH)/drivers.mk

uvccam-objs	           += uvc_driver.o
uvccam-objs	           += uvc_queue.o
uvccam-objs	           += uvc_v4l2.o
uvccam-objs	           += uvc_video.o
uvccam-objs	           += uvc_ctrl.o
uvccam-objs	           += uvc_status.o
uvccam-objs	           += uvc_isight.o
uvccam-objs	           += uvc_debugfs.o
uvccam-objs	           += nalu.o
uvccam-objs		   += uvc_queue.o

#ifeq ($(CONFIG_MEDIA_CONTROLLER),y)
uvccam-objs		   += uvc_entity.o
#endif

obj-$(CONFIG_UVCCAM)       += uvccam.o
