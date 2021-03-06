
KERNELDIR = $(DBG_KERNEL_SRC_DIR)
INSTALLDIR = $(DBG_COPY_DIR)

ifneq ($(SOC_ARCH), arm64)
SOC_ARCH = arm
endif

PWD := $(shell pwd)

CROSS_COMPILE = $(DBG_CROSS_COMPILE)
CC = $(CROSS_COMPILE)gcc
#apt-get install ccache;ccache -M 50G;ccache -s
#CC = ccache $(CROSS_COMPILE)gcc

include $(DBG_SOC_PATH)/$(DBG_SOC)/conf/soc_select_drivers
EXTRA_CFLAGS := -I$(DBG_CORE_PATH)/inc  -I$(DBG_SOC_PATH)/$(DBG_SOC)/inc  -I$(DBG_DEV_PATH)/$(DBG_SOC)

include $(DBG_BUILD_PATH)/build_cfg.mk
KBUILD_EXTRA_SYMBOLS := $(DBG_SOC_PATH)/$(DBG_SOC)/Module.symvers $(DBG_CORE_PATH)/Module.symvers

EXTRA_CFLAGS  += -I$(DBG_DRIVERS_PATH)/inc

modules:
	$(MAKE) -C $(DBG_KERNEL_OBJ_DIR) M=$(PWD) ARCH=$(SOC_ARCH)  CROSS_COMPILE=$(DBG_CROSS_COMPILE) modules
 
modules_install:
	cp *.ko $(INSTALLDIR)

clean:
	rm -rf *.o *~ *.depend *.cmd *.ko *.mod.c *.tmp_versions *.order *.symvers *.orig *.tmp

.PHONY: modules modules_install clean


