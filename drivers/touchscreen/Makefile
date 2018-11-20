include $(DBG_ROOT_PATH)/build/build.mk
include $(DBG_DRIVERS_PATH)/drivers.mk

ifeq ($(DBG_PLATFORM),msm7627a)
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_probe.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt801.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt80x_update.o
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_probe_new.o
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_to_recov.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt811.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt911.o
gt911-objs :=gt911_25q.o
obj-$(CONFIG_TOUCHSCREEN)	+= ft5x06_ts.o
ft5x06_ts-objs :=ft5x06_ts_25q.o

else ifeq ($(DBG_PLATFORM),msm8625)
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_probe_new.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt801.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt80x_update.o
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_to_recov.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt811.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt911.o
gt911-objs :=gt911_25q.o
obj-$(CONFIG_TOUCHSCREEN)	+= ft5x06_ts.o
ft5x06_ts-objs :=ft5x06_ts_25q.o

else ifeq ($(DBG_PLATFORM),msm8226)
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_probe_new.o
obj-$(CONFIG_TOUCHSCREEN)	+= lidbg_ts_to_recov.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt911.o
obj-$(CONFIG_TOUCHSCREEN)	+= gt811.o
gt911-objs :=gt9xx.o
obj-$(CONFIG_TOUCHSCREEN)	+= ft5x06_ts.o
ft5x06_ts-objs :=ft5406_ts.o
obj-$(CONFIG_TOUCHSCREEN)       += ts_config.o
#ft5336_ts对应26手机触摸屏驱动，仅初期验证时使用,车机不要编译进来！
#obj-$(CONFIG_TOUCHSCREEN)	+= ft5x06_ts.o
#ft5x06_ts-objs :=ft5336_ts.o     

else ifeq ($(DBG_PLATFORM),mt3360)
obj-$(CONFIG_CYTTSP4)	        += cyttsp4/

else
obj-$(CONFIG_TOUCHSCREEN)       += lidbg_ts_probe_new.o
obj-$(CONFIG_TOUCHSCREEN)       += lidbg_ts_to_recov.o
obj-$(CONFIG_TOUCHSCREEN)       += gt911.o
obj-$(CONFIG_TOUCHSCREEN)       += ts_config.o
gt911-objs :=gt9xx.o
endif
