include $(DBG_ROOT_PATH)/build/build.mk
include $(DBG_DRIVERS_PATH)/drivers.mk

ifeq ($(DBG_PLATFORM),msm8996)
else
obj-$(CONFIG_BMA2X2)+= bma2x2.o
obj-$(CONFIG_MC3XXX)+= mc3xxx.o
endif
