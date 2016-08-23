EXTRA_CFLAGS += -DBUILD_SOC
EXTRA_CFLAGS += -DNOT_USE_MEM_LOG

EXTRA_CFLAGS += -I$(DBG_SYSTEM_DIR)/kernel-3.18/arch/arm64/include/uapi
EXTRA_CFLAGS += -Iarch/arm64/include/generated
EXTRA_CFLAGS += -Wno-date-time

