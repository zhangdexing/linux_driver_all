EXTRA_CFLAGS += -DBUILD_SOC
EXTRA_CFLAGS += -DNOT_USE_MEM_LOG

EXTRA_CFLAGS += -I$(DBG_SYSTEM_DIR)/autochips/platform/kernel/drivers/inc
EXTRA_CFLAGS += -I$(DBG_SYSTEM_DIR)/autochips/platform/kernel/drivers/input
EXTRA_CFLAGS += -I$(DBG_KERNEL_SRC_DIR)/arch/arm/mach-ac83xx/include
EXTRA_CFLAGS += -I$(DBG_KERNEL_SRC_DIR)/arch/arm/mach-ac83xx/include/mach
EXTRA_CFLAGS += -I$(DBG_KERNEL_SRC_DIR)/../autochips/kernel/arch/arm/mach-ac83xx/include/mach
