
#include "lidbg.h"
LIDBG_DEFINE;

struct work_struct work_left_button1;
struct work_struct work_left_button2;
static int val;
static void work_left_button1_fn(struct work_struct *work)
{
    /* 1??????????? */
    if(val)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_UP, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEUP, KEY_PRESSED_RELEASED);

    }

    /* 2??????????? */
    if(val == 0)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_DOWN, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEDOWN, KEY_PRESSED_RELEASED);
    }

}

static void work_left_button2_fn(struct work_struct *work)
{
    /* 1??????????? */
    if(val)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_UP, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEUP, KEY_PRESSED_RELEASED);
    }

    /* 2??????????? */
    if(val == 0)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_DOWN, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEDOWN, KEY_PRESSED_RELEASED);
    }

}
struct work_struct work_right_button1;
static void work_right_button1_fn(struct work_struct *work)
{
    /* 1??????????? */
    if(val)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_UP, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEUP, KEY_PRESSED_RELEASED);

    }

    /* 2??????????? */
    if(val == 0)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_DOWN, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEDOWN, KEY_PRESSED_RELEASED);
    }
}

struct work_struct work_right_button2;
static void work_right_button2_fn(struct work_struct *work)
{
    /* 1??????????? */
    if(val)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_UP, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEUP, KEY_PRESSED_RELEASED);
    }

    /* 2??????????? */
    if(val == 0)
    {
        if(g_var.recovery_mode == 1)
            SOC_Key_Report(KEY_DOWN, KEY_PRESSED_RELEASED);
        else
            SOC_Key_Report(KEY_VOLUMEDOWN, KEY_PRESSED_RELEASED);
    }
}


irqreturn_t irq_left_button1(int irq, void *dev_id)
{
    //lidbg("irq_left_button1: %d\n", irq);
    val = SOC_IO_Input(BUTTON_LEFT_2, BUTTON_LEFT_2, GPIO_CFG_PULL_UP);
    if(!work_pending(&work_left_button1))
        schedule_work(&work_left_button1);
    return IRQ_HANDLED;

}
irqreturn_t irq_left_button2(int irq, void *dev_id)
{
    val = SOC_IO_Input(BUTTON_LEFT_1, BUTTON_LEFT_1, GPIO_CFG_PULL_UP);
    //lidbg("irq_left_button2: %d\n", irq);
    if(!work_pending(&work_left_button2))
        schedule_work(&work_left_button2);
    return IRQ_HANDLED;
}




irqreturn_t irq_right_button1(int irq, void *dev_id)
{
    //lidbg("irq_right_button1: %d\n", irq);

    val = SOC_IO_Input(BUTTON_RIGHT_2, BUTTON_RIGHT_2, GPIO_CFG_PULL_UP);
    if(!work_pending(&work_right_button1))
        schedule_work(&work_right_button1);
    return IRQ_HANDLED;
}
irqreturn_t irq_right_button2(int irq, void *dev_id)
{
    //lidbg("irq_right_button2: %d\n", irq);

    val = SOC_IO_Input(BUTTON_RIGHT_1, BUTTON_RIGHT_1, GPIO_CFG_PULL_UP);
    if(!work_pending(&work_right_button2))
        schedule_work(&work_right_button2);
    return IRQ_HANDLED;
}






int button_suspend(void)
{
    SOC_IO_ISR_Disable(BUTTON_LEFT_1);
    SOC_IO_ISR_Disable(BUTTON_LEFT_2);
    SOC_IO_ISR_Disable(BUTTON_RIGHT_1);
    SOC_IO_ISR_Disable(BUTTON_RIGHT_2);
    return 0;
}
int button_resume(void)
{

    IO_CONFIG_INPUT(0, BUTTON_LEFT_1);
    IO_CONFIG_INPUT(0, BUTTON_LEFT_2);
    IO_CONFIG_INPUT(0, BUTTON_RIGHT_1);
    IO_CONFIG_INPUT(0, BUTTON_RIGHT_2);

    SOC_IO_ISR_Enable(BUTTON_LEFT_1);
    SOC_IO_ISR_Enable(BUTTON_LEFT_2);
    SOC_IO_ISR_Enable(BUTTON_RIGHT_1);
    SOC_IO_ISR_Enable(BUTTON_RIGHT_2);

    return 0;
}
void button_init(void)
{
    int button_en;

    lidbg("button_init\n");
    FS_REGISTER_INT(button_en, "button_en", 1, NULL);
    if(button_en)
    {
        INIT_WORK(&work_left_button1, work_left_button1_fn);
        INIT_WORK(&work_left_button2, work_left_button2_fn);

        INIT_WORK(&work_right_button1, work_right_button1_fn);
        INIT_WORK(&work_right_button2, work_right_button2_fn);

#ifdef SOC_mt3360
#else
        SOC_IO_Input(BUTTON_LEFT_1, BUTTON_LEFT_1, GPIO_CFG_PULL_UP);
        SOC_IO_Input(BUTTON_LEFT_2, BUTTON_LEFT_2, GPIO_CFG_PULL_UP);
        SOC_IO_Input(BUTTON_RIGHT_1, BUTTON_RIGHT_1, GPIO_CFG_PULL_UP);
        SOC_IO_Input(BUTTON_RIGHT_2, BUTTON_RIGHT_2, GPIO_CFG_PULL_UP);
#endif

        SOC_IO_ISR_Add(BUTTON_LEFT_1, IRQF_TRIGGER_RISING , irq_left_button1, NULL);
        SOC_IO_ISR_Add(BUTTON_LEFT_2, IRQF_TRIGGER_FALLING, irq_left_button2, NULL);
        SOC_IO_ISR_Add(BUTTON_RIGHT_1, IRQF_TRIGGER_RISING, irq_right_button1, NULL);
        SOC_IO_ISR_Add(BUTTON_RIGHT_2, IRQF_TRIGGER_FALLING, irq_right_button2, NULL);
    }
}
int thread_button_init(void *data)
{
    button_init();
    return 0;
}








static int  button_dev_init(void)
{
    printk(KERN_WARNING "chdrv_init\n");
    lidbg("hello_button\n");

    LIDBG_GET;
    if((g_var.is_fly == 0) || (g_var.recovery_mode == 1))
    {
        CREATE_KTHREAD(thread_button_init, NULL);
    }

    return 0;


}

static void  button_dev_exit(void)
{
    printk("chdrv_exit\n");



}

EXPORT_SYMBOL(button_suspend);
EXPORT_SYMBOL(button_resume);
module_init(button_dev_init);
module_exit(button_dev_exit);



MODULE_AUTHOR("fly, <fly@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");





