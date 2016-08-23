include $(DBG_ROOT_PATH)/build/build.mk
include $(DBG_DRIVERS_PATH)/drivers.mk
CYTTSP4_CORE_OBJS := cyttsp4_i2c.o
CYTTSP4_OTHER_OBJS := cyttsp4_core.o  cyttsp4_mtb.o cyttsp4_mt_common.o cyttsp4_platform.o cyttsp4_proximity.o
cyttsp4-objs   := $(CYTTSP4_CORE_OBJS) $(CYTTSP4_OTHER_OBJS)
obj-m := cyttsp4.o