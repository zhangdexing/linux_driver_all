
EXTRA_CFLAGS += -DBUILD_DRIVERS

EXTRA_CFLAGS += -I$(DBG_KERNEL_SRC_DIR)/arch/arm/mach-msm

ifeq ($(DBG_SOC), mt35x)
EXTRA_CFLAGS += -Wno-error=date-time
endif

