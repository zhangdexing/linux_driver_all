#include<linux/module.h>
#include<linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <mach/dma.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <mach/ac83xx_gpio_pinmux.h>
#include <mach/ac83xx_system.h>

#define POWER_KEY_POLLING_TIME   (jiffies + 5*(HZ/1000))  /* 5ms */
#define POWER_KEY       116   //android POWER_KEY
extern struct input_dev *get_ac83xx_keyinput(void);
void powerkey_timer(unsigned long data);

static DEFINE_TIMER(power_key_timer, powerkey_timer, 0, 0);
static  struct input_dev   *input = NULL;

static uint32_t get_powerkey_value(void)
{
    uint32_t u4Value = PDWNC_READ32(REG_RW_GPIOIN);

    return (u4Value & (1U << PDWNC_INTR_GPIO_WAKEUP_SRC));

}
unsigned int fly_pwrkey_flag = 0;
unsigned int fly_acc_step = 0;
// add by flyaudio ++
spinlock_t fly_acc_lock;
extern void lidbg_shell_cmd(char *shell_cmd);
void powerkey_timer(unsigned long data)
{
    uint32_t tmp = 0;
    static uint32_t ac8317_suspend_counts = 0;
    uint32_t delay_wakeup = 20;
    static uint32_t acc_lock = 2;
    //spin_lock_irq(&fly_acc_lock);
    tmp = get_powerkey_value();
    if(tmp == 0)
    {
        if(fly_pwrkey_flag <= 25 && fly_pwrkey_flag >= acc_lock)// 100 <= 125 ms	wakeup
        {
            printk("fly power wakeup down (%d)\r\n", fly_pwrkey_flag);
            fly_pwrkey_flag = 0;
            fly_acc_step = 0;
            acc_lock = 2;
            input_report_key(input, POWER_KEY, 1);
            input_sync(input);
            lidbg_shell_cmd("am broadcast -a com.flyaudio.devices.acc_on ");
            mdelay(250);
            delay_wakeup = 20;
            while(delay_wakeup--)
                printk("fly power wakeup %d\r\n", delay_wakeup);
            printk("fly ac8317_wakeup_counts=%u\r\n", ac8317_suspend_counts);
            printk("fly power wakeup release\r\n");
            input_report_key(input, POWER_KEY, 0);
            input_sync(input);
        }
        else if(fly_pwrkey_flag <= 45 && fly_pwrkey_flag >= 2)// 200 <= 225 ms	acc off
        {
            printk("fly power acc off ++ (%d)\r\n", fly_pwrkey_flag);
            fly_pwrkey_flag = 0;
            if(fly_acc_step == 0)
                lidbg_shell_cmd("am broadcast -a com.flyaudio.devices.acc_off ");
            else
            {
                lidbg_shell_cmd("am broadcast -a com.flyaudio.ap.broadcast ");
                fly_acc_step = 0;
                acc_lock = 0;
            }
            printk("fly power acc off --\r\n");
        }
        else if(fly_pwrkey_flag <= 65 && fly_pwrkey_flag >= 2)// 300 <= 325 ms	acc on
        {
            printk("fly power acc on ++ (%d)\r\n", fly_pwrkey_flag);
            fly_pwrkey_flag = 0;
            if(fly_acc_step == 0)
                lidbg_shell_cmd("am broadcast -a com.flyaudio.devices.acc_on ");
            else
            {
                lidbg_shell_cmd("am broadcast -a com.flyaudio.ap.broadcast ");
                fly_acc_step = 0;
                acc_lock = 0;
            }
            printk("fly power acc on --\r\n");
        }
        else if(fly_pwrkey_flag <= 85 && fly_pwrkey_flag >= 2)// 400 <= 425 ms	suspend
        {
            printk("fly power suspend down (%d)\r\n", fly_pwrkey_flag);
            fly_pwrkey_flag = 0;
            acc_lock = 0;
            lidbg_shell_cmd("am broadcast -a com.flyaudio.ap.broadcast ");
            printk("fly ac8317_suspend_counts=%u\r\n", ++ac8317_suspend_counts);
            printk("fly power suspend release\r\n");
        }
        else
        {
            printk("fly power ignore!!! (%d)\r\n", fly_pwrkey_flag);
            fly_pwrkey_flag = 0;
        }
        fly_pwrkey_flag = 0;
    }
    else
    {
        mod_timer(&power_key_timer, POWER_KEY_POLLING_TIME);
        fly_pwrkey_flag++;
    }
    //spin_unlock_irq(&fly_acc_lock);
}


static irqreturn_t powerkey_isr_handler(int irq, void *dev_id)
{
    printk("fly powerkey_isr_handler\r\n");
    mod_timer(&power_key_timer, POWER_KEY_POLLING_TIME);
    return IRQ_HANDLED;

}
// add by flyaudio --
static __init int  ac83xxpowerkey_init(void)
{
    int ret = -1;
    fly_pwrkey_flag = 0;
    fly_acc_step = 0;
    input = get_ac83xx_keyinput();
    //add_timer(&power_key_timer);
    init_timer(&power_key_timer);
    power_key_timer.data = 0;
    power_key_timer.expires = 0;
    power_key_timer.function = powerkey_timer;
    ret = request_pdwnc_irq(PDWNC_INTR_GPIO_WAKEUP_SRC, powerkey_isr_handler, 0, "POWER-KEY-ISR", NULL);
    printk("[PWRKEY]: ac83xxpowerkey_init: driver insmod ok\n");

    return ret;
}

static __exit int  ac83xxpowerkey_exit(void)
{
    printk("[PWRKEY]: ac83xxpowerkey_exit\n");

    return 0;
}
module_init(ac83xxpowerkey_init);
module_exit(ac83xxpowerkey_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AC83XX PwrKey Driver");
EXPORT_SYMBOL(fly_acc_step);

