#include $(DBG_SOC_PATH)/$(DBG_SOC)/soc.mk

EXTRA_CFLAGS += -DBUILD_CORE

ifeq ($(DBG_SOC), mt35x)
EXTRA_CFLAGS += -Wno-error=date-time
endif
