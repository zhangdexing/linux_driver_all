EXTRA_CFLAGS += -DBUILD_SOC
EXTRA_CFLAGS += -DNOT_USE_MEM_LOG

EXTRA_CFLAGS += -I$(DBG_SYSTEM_DIR)/kernel-3.18/arch/arm64/include/uapi
EXTRA_CFLAGS += -Iarch/arm64/include/generated
EXTRA_CFLAGS += -Wno-date-time

THERMAL_CHIP_DRIVER_DIR := $(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/thermal/$(MTK_PLATFORM)
EXTRA_CFLAGS  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/power/$(MTK_PLATFORM)
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/gpu/hal/
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/thermal/fakeHeader/
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/base/power/include/
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/auxadc/
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/include/mt-plat/mt3561/include/mach
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/include/mt-plat/
EXTRA_CFLAGS  += -I$(DBG_SYSTEM_DIR)/kernel-3.18/drivers/misc/mediatek/include/


ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt3561"))
ifeq ($(CONFIG_ARCH_MT3561),y)
EXTRA_CFLAGS  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D1
endif

ifeq ($(CONFIG_ARCH_MT3561M),y)
EXTRA_CFLAGS  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D2
endif

ifeq ($(CONFIG_ARCH_MT3562),y)
EXTRA_CFLAGS  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D3
endif
endif
