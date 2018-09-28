#ifndef _LIGDBG_SOC_8996__
#define _LIGDBG_SOC_8996__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/fb.h>
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
#include <soc/qcom/smem.h>

#define FLY_USB_CAMERA_SUPPORT
#define DEFAULT_SUSPEND_NO_AIRPLANE_MODE
#define USB_HUB_SUPPORT
#define SUSPEND_ONLINE
#define MUC_CONTROL_DSP
#define MUC_DSP7741
#define CHECK_MCU_BUSY

enum msm_gpiomux_setting {
	GPIOMUX_ACTIVE = 0,
	GPIOMUX_SUSPENDED,
	GPIOMUX_NSETTINGS
};

enum gpiomux_drv {
	GPIOMUX_DRV_2MA = 0,
	GPIOMUX_DRV_4MA,
	GPIOMUX_DRV_6MA,
	GPIOMUX_DRV_8MA,
	GPIOMUX_DRV_10MA,
	GPIOMUX_DRV_12MA,
	GPIOMUX_DRV_14MA,
	GPIOMUX_DRV_16MA,
};

enum gpiomux_func {
	GPIOMUX_FUNC_GPIO = 0,
	GPIOMUX_FUNC_1,
	GPIOMUX_FUNC_2,
	GPIOMUX_FUNC_3,
	GPIOMUX_FUNC_4,
	GPIOMUX_FUNC_5,
	GPIOMUX_FUNC_6,
	GPIOMUX_FUNC_7,
	GPIOMUX_FUNC_8,
	GPIOMUX_FUNC_9,
	GPIOMUX_FUNC_A,
	GPIOMUX_FUNC_B,
	GPIOMUX_FUNC_C,
	GPIOMUX_FUNC_D,
	GPIOMUX_FUNC_E,
	GPIOMUX_FUNC_F,
};

enum gpiomux_pull {
	GPIOMUX_PULL_NONE = 0,
	GPIOMUX_PULL_DOWN,
	GPIOMUX_PULL_KEEPER,
	GPIOMUX_PULL_UP,
};

/* Direction settings are only meaningful when GPIOMUX_FUNC_GPIO is selected.
 * This element is ignored for all other FUNC selections, as the output-
 * enable pin is not under software control in those cases.  See the SWI
 * for your target for more details.
 */
enum gpiomux_dir {
	GPIOMUX_IN = 0,
	GPIOMUX_OUT_HIGH,
	GPIOMUX_OUT_LOW,
};

enum
{
    GPIO_CFG_INPUT,
    GPIO_CFG_OUTPUT,
};

/* GPIO TLMM: Pullup/Pulldown */
enum
{
    GPIO_CFG_NO_PULL,
    GPIO_CFG_PULL_UP,
    GPIO_CFG_PULL_DOWN,
};

/* GPIO TLMM: Drive Strength */
enum
{
    GPIO_CFG_2MA,
    GPIO_CFG_4MA,
    GPIO_CFG_6MA,
    GPIO_CFG_8MA,
    GPIO_CFG_10MA,
    GPIO_CFG_12MA,
    GPIO_CFG_14MA,
    GPIO_CFG_16MA,
};

enum
{
    GPIO_CFG_ENABLE,
    GPIO_CFG_DISABLE,
};




#define IO_LOG_NUM  (117)//0~116

#define SOC_TARGET_PATH "../../soc/msm8x26/lidbg_target_msm8996.c"
#define SOC_TARGET_DEFINE_PATH "lidbg_target_msm8996.h"

#define GPIO_MAP_OFFSET  (0)
#define GPIO_TO_INT(x) gpio_to_irq(x+GPIO_MAP_OFFSET)

#endif

