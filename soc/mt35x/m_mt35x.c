/*======================================================================
======================================================================*/
#include "lidbg.h"

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

    return 0;
}



int mt35x_init(void)
{
    DUMP_BUILD_TIME;

    CREATE_KTHREAD(thread_get_mac_addr, NULL);

    platform_device_register(&lidbg_soc);
    platform_driver_register(&lidbg_soc_driver);
    return 0;
}

/*Ä£¿éÐ¶ÔØº¯Êý*/
void mt35x_exit(void)
{
    lidbg("mx35x_exit\n");

}


EXPORT_SYMBOL(lidbg_soc_main);
EXPORT_SYMBOL(soc_temp_get);

MODULE_AUTHOR("Lsw");
MODULE_LICENSE("GPL");

module_init(mt35x_init);
module_exit(mt35x_exit);

