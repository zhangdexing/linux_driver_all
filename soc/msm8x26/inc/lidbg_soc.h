#ifndef _LIGDBG_SOC__
#define _LIGDBG_SOC__

struct io_config
{
    __u32 index;
    __u32 status;
    __u32 pull;
    bool direction;
    __u32 drive_strength;
    bool disable;

} ;


struct io_int_config
{
    __u32 ext_int_num;
    unsigned long irqflags;
    pinterrupt_isr pisr;
    void *dev;
} ;

///////////////////////////////////////

void  soc_io_init(void);

void  soc_ad_init(void);
u32  soc_ad_read(u32 ch);
u32  soc_bl_set(u32 bl_level);
u32  soc_pwm_set(int pwm_id, int duty_ns, int period_ns);
void soc_bl_init(void);

int soc_io_irq(struct io_int_config *pio_int_config);
void soc_irq_disable(unsigned int irq);
void soc_irq_enable(unsigned int irq);

int soc_io_output(u32 group, u32 index, bool status);
bool soc_io_input(u32 index);
int soc_io_config(u32 index, int func, u32 direction,  u32 pull, u32 drive_strength, bool force_reconfig);
int soc_io_suspend_config(u32 index, u32 direction, u32 pull, u32 drive_strength);
int soc_io_suspend(void);
int soc_io_resume(void);
int soc_temp_get(int num);
void lidbg_soc_main(int argc, char **argv);

///////////////////////////////////////

#define IO_CONFIG_OUTPUT(group,index) do{  soc_io_config( index, GPIOMUX_FUNC_GPIO, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA, 1);}while(0)
#define IO_CONFIG_INPUT(group,index) do{  soc_io_config( index, GPIOMUX_FUNC_GPIO, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 1);}while(0)


#if ANDROID_VERSION >= 600
#define EMMC_MOUNT_POINT0  "/storage/emulated/0"
#define EMMC_MOUNT_POINT1  "/storage/sdcard1"
#else
#define EMMC_MOUNT_POINT0  "/storage/sdcard0"
#define EMMC_MOUNT_POINT1  "/storage/sdcard1"
#endif

#define SOC_KO  "lidbg_ad_msm8x26.ko","lidbg_soc_msm8x26.ko"
#define RECOVERY_USB_MOUNT_POINT "/usb"


#endif
