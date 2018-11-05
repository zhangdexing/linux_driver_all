/*======================================================================
======================================================================*/

#include "lidbg.h"

#define REG_GRFNUM  106
#define REG_GPIONUM 8
#define GPIO_SWPORTA_DR  0x0000
#define GPIO_SWPORTA_DDR 0x0004
#define grf_readl(offset)		readl_relaxed(RK30_GRF_BASE + offset)
#define grf_writel(v, offset)	do { writel_relaxed(v, RK30_GRF_BASE + offset); dsb(); } while (0)

#if	0
#define  DBG		lidbg
#else
#define  DBG(...)	((void)0)
#endif

struct fly_smem *p_fly_smem = NULL;
static int reg_grfbak[REG_GRFNUM];
static int reg_grftmp[REG_GRFNUM] = {0};
static int reg_gpiobak[REG_GPIONUM];
static void __iomem *gpio_base[] = {RK30_GPIO0_BASE, RK30_GPIO1_BASE, RK30_GPIO2_BASE, RK30_GPIO3_BASE};

static void gpio_output(u32 group, u32 gpio, bool status)
{
    int ret = 0;

    ret = gpio_request(gpio, NULL);
    if (ret != 0)
    {
        gpio_free(gpio);
        gpio_request(gpio, NULL);
    }

    gpio_direction_output(gpio, status);
    gpio_set_value(gpio, status);
    return;
}

static void gpio_input_normal(u32 gpio, u32 value)
{
    int ret = 0;

    ret = gpio_request(gpio, NULL);
    if (ret != 0)
    {
        gpio_free(gpio);
        gpio_request(gpio, NULL);
    }

    gpio_direction_input(gpio);
    gpio_pull_updown(gpio, value);

    return;
}

void grf_backup(void)
{

    int i, j, reg_tmp[REG_GRFNUM];
    DUMP_FUN;
    for(i = 0; i < REG_GRFNUM; i++ )	//GRF?????,??GPIO?????IOMUX????
        reg_grfbak[i] = grf_readl(4 * i);

    for(i = 0, j = 0; i < 4; i++)
    {
        reg_gpiobak[j++] = readl_relaxed(gpio_base[i] + GPIO_SWPORTA_DDR);	//GPIO?????,?????????0?1?
        reg_gpiobak[j++] = readl_relaxed(gpio_base[i] + GPIO_SWPORTA_DR);
    }

    for(i = 0; i < 24; i++ )
    {
        if (4 * i < 0x40)
            grf_writel(0xFFFF0000, 4 * i);
        else if (4 * i == 0x58)
            grf_writel(0xFFFFCBFE, 4 * i);
        else
            grf_writel(0xFFFFFFFF, 4 * i);
    }

    gpio_output(0, RK30_PIN3_PA0, 0);	//WIFI_POWER
    gpio_output(0, RK30_PIN3_PB2, 0);	//VIDEO_PDN
    gpio_output(0, RK30_PIN3_PB4, 0);	//VIDEO_INT
    gpio_output(0, RK30_PIN3_PB5, 0);	//VIDEO_RST

    gpio_input_normal(RK30_PIN1_PA0, GPIO_CFG_NO_PULL);	//DVD_RXD
    gpio_input_normal(RK30_PIN1_PA1, GPIO_CFG_NO_PULL);	//DVD_TXD
    gpio_input_normal(RK30_PIN1_PA4, GPIO_CFG_NO_PULL);	//BT_RXD
    gpio_input_normal(RK30_PIN1_PA5, GPIO_CFG_NO_PULL);	//BT_TXD
    gpio_input_normal(RK30_PIN3_PB6, GPIO_CFG_NO_PULL);	//AV_SDA
    gpio_input_normal(RK30_PIN3_PB7, GPIO_CFG_NO_PULL);	//AV_SCL


    for(i = 0, j = 0; i < REG_GRFNUM; i++)
    {
        reg_tmp[i] = grf_readl(4 * i);
        if(reg_tmp[i] != reg_grfbak[i])
        {
            DBG("reg value diff offset[0x%x] -> reg_grfbak:[0x%x] REG_TMP:[0x%x] \n", 4 * i, reg_grfbak[i], reg_tmp[i]);
            reg_grftmp[j++] = i;
        }
    }
    reg_grftmp[j] = 0;

    return;
}

void grf_restore(void)
{

    int i, j, reg_tmp[REG_GRFNUM];
    DUMP_FUN;
    for(j = 0; j < REG_GRFNUM; j++)
    {
        if(reg_grftmp[j] == 0) break;
        i = reg_grftmp[j];
        grf_writel(( reg_grfbak[i] | 0xFFFF0000 ), 4 * i);
    }

    for(i = 0; i < REG_GRFNUM; i++ )
    {
        reg_tmp[i] = grf_readl(4 * i);
        if(reg_tmp[i] != reg_grfbak[i])
            DBG("reg value diff offset[0x%x] -> reg_grfbak:[0x%x] REG_TMP:[0x%x] \n", 4 * i, reg_grfbak[i], reg_tmp[i]);
    }

    for(i = 0, j = 0; i < 4; i++)
    {
        writel_relaxed(reg_gpiobak[j++], gpio_base[i] + GPIO_SWPORTA_DDR);
        writel_relaxed(reg_gpiobak[j++], gpio_base[i] + GPIO_SWPORTA_DR);
    }

    //??GPIO
    gpio_free(RK30_PIN3_PA0);	//WIFI_POWER
    gpio_free(RK30_PIN3_PB2);	//VIDEO_PDN
    gpio_free(RK30_PIN3_PB4);	//VIDEO_INT
    gpio_free(RK30_PIN3_PB5);	//VIDEO_RST
    gpio_free(RK30_PIN1_PA0);	//DVD_RXD
    gpio_free(RK30_PIN1_PA1);	//DVD_TXD
    gpio_free(RK30_PIN1_PA4);	//BT_RXD
    gpio_free(RK30_PIN1_PA5);	//BT_TXD
    gpio_free(RK30_PIN3_PB6);	//AV_SDA
    gpio_free(RK30_PIN3_PB7);	//AV_SCL

}


int soc_temp_get(int num)
{
    static long temp = -1;
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
	
	ret = vfs_write(filp, wbuf, length, &filp->f_pos);
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



int rk3x88_init(void)
{
    DUMP_BUILD_TIME;

    CREATE_KTHREAD(thread_get_mac_addr, NULL);

    platform_device_register(&lidbg_soc);
    platform_driver_register(&lidbg_soc_driver);
    return 0;
}

/*Ä£¿éÐ¶ÔØº¯Êý*/
void rk3x88_exit(void)
{

}

EXPORT_SYMBOL(grf_backup);
EXPORT_SYMBOL(grf_restore);
EXPORT_SYMBOL(lidbg_soc_main);
EXPORT_SYMBOL(p_fly_smem);
EXPORT_SYMBOL(soc_temp_get);

MODULE_AUTHOR("Lsw");
MODULE_LICENSE("GPL");

module_init(rk3x88_init);
module_exit(rk3x88_exit);
