#ifndef _LIGDBG_SOC_8909__
#define _LIGDBG_SOC_8909__


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/fb.h>
#include <mach/gpiomux.h>
#include "mach/hardware.h"
#include "mach/irqs.h"
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_memtypes.h>
#include <mach/vreg.h>
#include <mach/irqs.h>
#include <linux/i2c/pca953x.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio_event.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/regulator/consumer.h>
#include <linux/memblock.h>
#include <linux/qpnp/qpnp-adc.h>
#include <linux/spmi.h>
#include <linux/msm_tsens.h>
#include <linux/usb/android.h>
#include <soc/qcom/smem.h>


#define SUSPEND_ONLINE
#define MUC_CONTROL_DSP
#define MUC_DSP7741
#define USB_HUB_SUPPORT
#define FLY_HAL_NEW_COMM
#define FLY_USB_CAMERA_SUPPORT
#define DEFAULT_SUSPEND_NO_AIRPLANE_MODE


#define IO_LOG_NUM  (117)//0~116


#define SOC_TARGET_PATH "../../soc/msm8x26/lidbg_target_msm8909.c"
#define SOC_TARGET_DEFINE_PATH "lidbg_target_msm8909.h"

#define GPIO_MAP_OFFSET  (911)
#define GPIO_TO_INT(x) gpio_to_irq(x+GPIO_MAP_OFFSET)

#endif



