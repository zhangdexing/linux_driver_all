/*======================================================================
======================================================================*/

#include "lidbg.h"

struct fly_smem *p_fly_smem = NULL;


int soc_temp_get(int num)
{
    static long temp;
    static struct tsens_device tsens_dev;

    /*    8974
    	{"tsens_tz_sensor5", "cpu0" },
    	{"tsens_tz_sensor6", "cpu1" },
    	{"tsens_tz_sensor7", "cpu2" },
    	{"tsens_tz_sensor8", "cpu3" },
    	{"tsens_tz_sensor3", "pop_mem" },
    */

    /*   8x26
    	{"tsens_tz_sensor5", "cpu0-1" },
    	{"tsens_tz_sensor2", "cpu2-3" },
    	{"tsens_tz_sensor3", "pop_mem" },
    */


    tsens_dev.sensor_num = num;
    tsens_get_temp(&tsens_dev, &temp);
#if 0
    lidbg("\n");
    lidbg("sensor5_temp=%d\n", temp);

    tsens_dev.sensor_num = 2;
    tsens_get_temp(&tsens_dev, &temp);
    lidbg("sensor2_temp=%d\n", temp);

    tsens_dev.sensor_num = 3;
    tsens_get_temp(&tsens_dev, &temp);
    lidbg("sensor3_temp=%d\n", temp);
    lidbg("\n");
#endif
#ifdef PLATFORM_msm8996
    return (int)(temp/10);
#endif
    return (int)temp;
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
        lidbg("ch%d = %d\n", ch, soc_ad_read(ch));
    }
}


static int  lidbg_soc_probe(struct platform_device *pdev)
{
    DUMP_FUN;
    soc_io_init();
    return 0;
}
#ifdef CONFIG_PM

static int soc_suspend(struct device *dev)
{
    DUMP_FUN;
    //	soc_io_suspend();
    return 0;
}
static int soc_resume(struct device *dev)
{
    DUMP_FUN;
    //	soc_io_resume();
    return 0;
}


static struct dev_pm_ops lidbg_soc_ops =
{
    .suspend	= soc_suspend,
    .resume		= soc_resume,
};
#endif

static struct platform_device lidbg_soc =
{
    .name               = "lidbg_soc",
    .id                 = -1,
};

static struct platform_driver lidbg_soc_driver =
{
    .probe		= lidbg_soc_probe,
    .driver         = {
        .name = "lidbg_soc",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &lidbg_soc_ops,
#endif
    },
};



int lidbg_write_file(const char *filename, const char *wbuf, size_t length)
{
    int ret = 0;
    struct file *filp = (struct file *) - ENOENT;
    mm_segment_t oldfs;
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    filp = filp_open(filename, O_RDWR | O_CREAT, 0666);
    if (IS_ERR(filp) || !filp->f_op)
    {

        lidbg("kernel_write_file:filp_open Error\n");
        ret = -ENOENT;
        return ret;
    }

    ret = filp->f_op->write(filp, wbuf, length, &filp->f_pos);
    if (ret < 0)
    {
        lidbg("kernel_write_file:write Error\n");
        return ret;
    }

    if (!IS_ERR(filp))
        filp_close(filp, NULL);
    set_fs(oldfs);
    return ret;
}

int thread_get_mac_addr(void *data)
{
try_get_smem:
    if (p_fly_smem == NULL)

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        p_fly_smem = (struct fly_smem *)smem_alloc(SMEM_ID_VENDOR0, sizeof(struct fly_smem));
#else
        p_fly_smem = (struct fly_smem *)smem_alloc(SMEM_ID_VENDOR0, sizeof(struct fly_smem), 0, 1);
#endif

    if (p_fly_smem == NULL)
    {
        msleep(500);
        goto try_get_smem;
    }

    msleep(100);

    lidbg( "SMEM_ID_VENDOR0 ok!\n");


    {
        int i;
        u8 *p;
        p = (u8 *)p_fly_smem;
        lidbg( "smem_data = ");
        for(i = 0; i < sizeof(struct fly_smem); i ++ )
        {
            lidbg("%d=%d \n",i, p[i]);
        }
    }


    /*
    	p_fly_smem->mac_addr[0] = 1;
    	p_fly_smem->mac_addr[1] = 2;
    	p_fly_smem->mac_addr[2] = 3;
    	p_fly_smem->mac_addr[3] = 4;
    	p_fly_smem->mac_addr[4] = 5;
    	p_fly_smem->mac_addr[5] = 6;
    */
    lidbg_write_file("/data/lidbg/wlan_mac", p_fly_smem->mac_addr, 6);

    return 0;
}



int msm8226_init(void)
{
    DUMP_BUILD_TIME;

    CREATE_KTHREAD(thread_get_mac_addr, NULL);

    platform_device_register(&lidbg_soc);
    platform_driver_register(&lidbg_soc_driver);
    return 0;
}

/*Ä£¿éÐ¶ÔØº¯Êý*/
void msm8226_exit(void)
{
    lidbg("msm8226_exit\n");

}


EXPORT_SYMBOL(lidbg_soc_main);
EXPORT_SYMBOL(p_fly_smem);
EXPORT_SYMBOL(soc_temp_get);

MODULE_AUTHOR("Lsw");
MODULE_LICENSE("GPL");

module_init(msm8226_init);
module_exit(msm8226_exit);
