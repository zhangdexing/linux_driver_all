#include <../drivers/flyaudio/lidbg_interface.h>
#include "lidbg_def.h"
#include "cmn_func.h"
#include "lidbg_target.h"

#include <linux/proc_fs.h>

#include "cmn_func.c"
#if defined(PLATFORM_MSM8226) || defined(PLATFORM_MSM8974) || defined(PLATFORM_MSM8909) || defined(PLATFORM_MSM8996)
#include <linux/msm_tsens.h> //qcom
#include "boot_freq_ctrl.c"
#endif
#if defined(PLATFORM_MSM8226) || defined(PLATFORM_MSM8909)
#include "lidbg_i2c.c"
#endif
#ifdef PLATFORM_sabresd_6dq
#include "showlogo.c"
#endif


LIDBG_THREAD_DEFINE;

int __init lidbg_kernel_init(void)
{
    DUMP_BUILD_TIME;
    printk(KERN_CRIT"===lidbg_kernel_init===\n");
#if defined(PLATFORM_MSM8226) || defined(PLATFORM_MSM8909) || defined(PLATFORM_MSM8996)
    freq_ctrl_start();
#endif

#ifdef PLATFORM_sabresd_6dq
	uboot_logo_bakup();
#endif
#if defined(PLATFORM_MSM8226) || defined(PLATFORM_MSM8909)
    lidbg_i2c_start();
#endif
    LIDBG_GET_THREAD;
#if defined(PLATFORM_MSM8226) || defined(PLATFORM_MSM8909)
    proc_create("lidbg_lcd_off", 0, NULL, &lcd_p_fops);
#endif
    return 0;
}


EXPORT_SYMBOL_GPL(plidbg_dev);

core_initcall(lidbg_kernel_init);
