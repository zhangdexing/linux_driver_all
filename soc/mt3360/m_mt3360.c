/*======================================================================
======================================================================*/

#include "lidbg.h"

struct fly_smem *p_fly_smem = NULL;


int soc_temp_get(int num)
{
    return 0;
}

void lidbg_soc_main(int argc, char **argv)
{

    if(argc < 1)
    {
        lidbg("Usage:\n");
        lidbg("bl value\n");
        return;
    }

    if(!strcmp(argv[0], "bl"))
    {
        u32 bl;
        bl = simple_strtoul(argv[1], 0, 0);
        soc_bl_set(bl);
    }

    if(!strcmp(argv[0], "ad"))
    {
        u32 ch;
        ch = simple_strtoul(argv[1], 0, 0);
        //lidbg("ch%d = %d\n", ch, soc_ad_read(ch));
        lidbg("fly NONE\n");
    }
}
// for mtk test flyaudio ++
int thread_gpio_ctrl(void *data)
{
    int val = 0;
    int i = 0;

    while(1)
    {
        //set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop())
            break;
        GPIO_MultiFun_Set(127, 0xFF);
        msleep(2000);
        gpio_direction_output(127, 1);
        lidbg("fly gpio[%d]==%d\r\n", 127, 1);
        //val = gpio_get_value(162);
        //soc_io_config( 162,  GPIOMUX_IN, GPIO_CFG_NO_PULL, GPIOMUX_DRV_2MA, 0);
        //val = soc_io_input(162);
        //lidbg("fly gpio[%d]==%d\r\n", 162, val);
        for(i = 0; i < 11; i++)
        {
            //msleep(2000);
            //soc_bl_set(i * 10);
            //lidbg("fly bkl==%d\r\n", i * 10);
        }
        //lidbg("fly FLY_IO_VA:0x%08X==0x%08X\r\n", FLY_IO_VA, flyReadREG(FLY_IO_VA));
        schedule_timeout(HZ);
    }
    return 0;
}
int mt3360_test(void)
{
    struct task_struct *task;
    task = kthread_create(thread_gpio_ctrl, NULL, "thread_gpio_ctrl");
    if(IS_ERR(task))
    {
        lidbg("fly start thread error!\n");
    }
    else
    {
        wake_up_process(task);
        lidbg("fly start thread ok!\n");
    }
}
// for mtk test flyaudio --

int mt3360_init(void)
{
    DUMP_BUILD_TIME;//LIDBG_MODULE_LOG;
    /*
        lidbg( "smem_alloc id = %d\n", SMEM_ID_VENDOR0);
       // p_fly_smem = (struct fly_smem *)smem_alloc(SMEM_ID_VENDOR0, sizeof(struct fly_smem));

        if (p_fly_smem == NULL)
        {
            lidbg( "smem_alloc fail,kmalloc mem!\n");
            p_fly_smem = (struct fly_smem *)kmalloc(sizeof(struct fly_smem), GFP_KERNEL);
            if(p_fly_smem == NULL)
            {
                LIDBG_ERR("<err.register_wakelock:kzalloc.name>\n");
            }
            memset(p_fly_smem, 0, sizeof(struct fly_smem));
        }

        soc_bl_init();
        */
    soc_bl_init();

    soc_io_init();
    //mt3360_test();
    return 0;
}

/*Ä£¿éÐ¶ÔØº¯Êý*/
void mt3360_exit(void)
{
    lidbg("msm8226_exit\n");

}


EXPORT_SYMBOL(lidbg_soc_main);
EXPORT_SYMBOL(p_fly_smem);
EXPORT_SYMBOL(soc_temp_get);

MODULE_AUTHOR("Lsw");
MODULE_LICENSE("GPL");

module_init(mt3360_init);
module_exit(mt3360_exit);
