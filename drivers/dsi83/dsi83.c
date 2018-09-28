
#include "lidbg.h"
#include <dsi83.h>

LIDBG_DEFINE;

static int thread_dsi83_check(void *data);

static bool is_dsi83_inited = false;
static struct delayed_work dsi83_work;
static struct workqueue_struct *dsi83_workqueue;
#if defined(CONFIG_FB)
struct notifier_block dsi83_fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)

#endif
char *buf_piont = NULL;
static u8 dsi83_conf_num;

static int SN65_register_read(unsigned char sub_addr, char *buf)
{
    int ret;
    ret = SOC_I2C_Rec(DSI83_I2C_BUS, DSI83_I2C_ADDR, sub_addr, buf, 1);
    return ret;
}

static int SN65_register_write( char *buf)
{
    int ret;
    ret = SOC_I2C_Send(DSI83_I2C_BUS, DSI83_I2C_ADDR, buf, 2);
    return ret;
}

static int SN65_Sequence_seq4(void)
{
    int ret = 0, i;
    char buf2[2];
    buf2[0] = 0x00;
    buf2[1] = 0x00;
    lidbg( "dsi83:Sequence 4\n");
    for(i = 0; buf_piont[i] != 0xff ; i += 2)
    {
        ret = SN65_register_write(&buf_piont[i]);
        buf2[0] = buf_piont[i];
        ret = SN65_register_read(buf2[0], &buf2[1]);
        lidbg_dsi83("register 0x%x=0x%x\n", buf_piont[i], buf_piont[i + 1]);

        if(buf2[1] != buf_piont[i + 1])
        {
            lidbg( "Warning regitster(0x%.2x),write(0x%.2x) and read back(0x%.2x) Unequal\n", \
                   buf_piont[i], buf_piont[i + 1], buf2[1]);
        }
    }
	    lidbg( "dsi83:Sequence 4.1\n");
    return ret;
}

static int SN65_Sequence_seq6(void)
{
    int ret;
    char buf2[2];
    buf2[0] = 0x0d;
    buf2[1] = 0x01;
    lidbg( "dsi83:Sequence 6\n");
    ret = SN65_register_write(buf2);
    return ret;
}
static int SN65_Sequence_seq7(void)
{
    int ret;
    char buf2[2];
    lidbg( "dsi83:Sequence 7\n");

    buf2[0] = 0x0a;
    buf2[1] = 0x00;
    ret = SN65_register_read(buf2[0], &buf2[1]);
    lidbg_dsi83("read(0x0a) = 0x%.2x\n", buf2[1]);

    {
        unsigned char k, i = 0;
        k = buf2[1] & 0x80;
        while(!k)
        {
            ret = SN65_register_read(buf2[0], &buf2[1]);
            k = buf2[1] & 0x80;
            lidbg( "dsi83:Wait for %d,r = 0x%.2x\n", i, buf2[1]);
            i++;
            if(i > 20)
            {
                lidbgerr( "dsi83:Warning wait time out .. break\n");
                is_dsi83_inited = 0;
                break;
            }
            msleep(100);
        }
    }
    return ret;
}

static int SN65_Sequence_seq8(void)  /*seq 8 the bit must be set after the CSR`s are updated*/
{
    int ret;
    char buf2[2];

#ifdef DSI83_DEBUG
    dsi83_dump_reg();
#endif
    lidbg( "dsi83:Sequence 8\n");
    buf2[0] = 0x09;
    buf2[1] = 0x01;
    ret = SN65_register_write(buf2);
    lidbg_dsi83("write(0x09) = 0x%.2x\n", buf2[1]);

#ifdef DSI83_DEBUG
    mdelay(100);
    dsi83_dump_reg();
#endif
    return ret;
}


static void dsi83_dump_reg(void)
{
    int i;
    unsigned char reg;
    lidbg("%s:enter\n", __func__);

    for (i = 0; i < 0x3d; i++)
    {
        SN65_register_read(i, &reg);
        lidbg( "dsi83:Read reg-0x%x=0x%x\n", i, reg);
    }
    SN65_register_read(0xe0, &reg);
    lidbg( "dsi83:Read reg-0xe0=0x%x\n", reg);
    SN65_register_read(0xe1, &reg);
    lidbg( "dsi83:Read reg-0xe1=0x%x\n", reg);
    SN65_register_read(0xe5, &reg);
    lidbg( "dsi83:Read reg-0xe5=0x%x\n", reg);

}


static int SN65_devices_read_id(void)
{
    // addr:0x08 - 0x00
    uint8_t id[9] = {0x01, 0x20, 0x20, 0x20, 0x44, 0x53, 0x49, 0x38, 0x35};
    uint8_t chip_id[9];
    int i, j;

    for (i = 0x8, j = 0; i >= 0; i--, j++)
    {
        if(SN65_register_read(i, &chip_id[j]) < 0)
        {

            lidbgerr("SN65_devices_read_id fail\n");
            return -1;
        }
        else
            lidbg_dsi83("%s():reg 0x%x = 0x%x", __func__, i, chip_id[j]);
    }

    return memcmp(id, chip_id, 9);

}

static void dsi83_enable(void)
{
    SOC_IO_Output(0, DSI83_GPIO_EN, 0);
    msleep(100);
    SOC_IO_Output(0, DSI83_GPIO_EN, 1);
}

static void T123_reset(void)
{
    T123_RESET;
}

static void panel_reset(void)
{
    LCD_RESET;
}

int dsi83_check(void)
{
    unsigned char reg;
    static int check_times = 0;
    char buf1[2];
    int ret = 0;
    buf1[0] = 0xe5;
    buf1[1] = 0xff;

    reg = 0xff;
    ret = SN65_register_write(buf1);
    if(ret < 0)
        lidbgerr( "[dsi83.check]:write reg 0xe5 error.\n");
    else
        msleep(100);

    ret = SN65_register_read(0xe5, &reg);
    lidbg("[dsi83.check]reg-0xe5=0x%x\n", reg);

    if(((reg != 0x00) || (ret < 0) || (is_dsi83_inited == 0) ) && (g_var.system_status >= FLY_KERNEL_UP))
    {
        check_times ++;
        lidbg( "[dsi83.check.err]reg-0xe5=0x%x,ret=%d,check_times=%d\n", reg, ret, check_times);
        is_dsi83_inited = 0;

        if(0&&check_times <= 5)
            queue_delayed_work(dsi83_workqueue, &dsi83_work, DSI83_DELAY_TIME);
        else
            lidbgerr( "[dsi83.check.err.fail]reg-0xe5=0x%x,ret=%d,check_times=%d\n", reg, ret, check_times);
    }
    else//check ok
    {
        check_times = 0;
    }
	
    msleep(3000);
    LPC_CMD_DSI83_INIT;

    return 0;
}



#if defined(CONFIG_FB)
void dsi83_suspend(void)
{

}

void dsi83_resume(void)
{
    queue_delayed_work(dsi83_workqueue, &dsi83_work, DSI83_DELAY_TIME);
}

static int dsi83_fb_notifier_callback(struct notifier_block *self,
                                      unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank;

    if (evdata && evdata->data && event == FB_EVENT_BLANK)
    {
        blank = evdata->data;

        if (*blank == FB_BLANK_UNBLANK)
        {
            lidbg( "dsi83:FB_BLANK_UNBLANK\n");
            g_var.fb_on = true;
            dsi83_resume();
        }
        else if (*blank == FB_BLANK_POWERDOWN)
        {
            lidbg( "dsi83:FB_BLANK_POWERDOWN\n");
            g_var.fb_on = false;
            dsi83_suspend();
        }
    }

    return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)

#endif

void dsi83_gpio_init(void)
{
    dsi83_enable();
    panel_reset();
    T123_reset();
}

 
void dsi83_config(void)
{
    int ret = 0;

    SN65_Sequence_seq4();

    ret = SN65_Sequence_seq6();
    if(ret < 0)
        lidbg( "dsi83:SN65_Sequence_seq6(),err,ret = %d.\n", ret);

    SN65_Sequence_seq7();

    ret = SN65_Sequence_seq8();
    if(ret < 0)
        lidbg( "dsi83:SN65_Sequence_seq8(),err,ret = %d.\n", ret);
	
}
static void dsi83_work_func(struct work_struct *work)
{
    int ret = 0;
    int i;
    DUMP_FUN;

    lidbg( "dsi83_work_func:enter %d,%d\n", g_var.system_status, is_dsi83_inited);

#ifdef SUSPEND_ONLINE
    if(is_dsi83_inited)
#else
	if(((g_var.system_status == FLY_ANDROID_DOWN) || (g_var.system_status == FLY_GOTO_SLEEP)) || is_dsi83_inited)
#endif
    {
        lidbg( "dsi83_work_func:skip %d,%d\n", g_var.system_status, is_dsi83_inited);
        return;
    }


    is_dsi83_inited = true;
    dsi83_gpio_init();
    msleep(50);

    for(i = 0; i < 5; ++i)
    {
        ret = SN65_devices_read_id();
        if (!ret)
            break;
        else
        {
            lidbg( "dsi83:DSI83 match ID falied,num:%d.\n", i + 1);
            msleep(100);
            continue;
        }
    }

    if(i == 3)
        return;
    else
        lidbg( "dsi83:DSI83 match ID success!\n");

    dsi83_config();

  //  msleep(200);//wait for sigal/reg stable
    //dsi83_check();
    //LCD_ON;
    //lidbg( "dsi83:LCD_ON\n");
        CREATE_KTHREAD(thread_dsi83_check, NULL);
}

int is_dsi83_exist(void)
{
    int exist, retry;
    for(retry = 0; retry < 5; retry++)
    {
        exist = SN65_devices_read_id();
        if (exist != 0)
        {
            msleep(50);
            continue;
        }
        else
        {
            return 1;
        }
    }
    return -1;
}


ssize_t dsi83_rst_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    PROC_READ_CHECK;
    is_dsi83_inited = false;
    fs_mem_log("call:%s\n", __func__);
    lidbg("%s:enter\n", __func__);
    dsi83_resume();
    return 1;
}
ssize_t dsi83_dump_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    PROC_READ_CHECK;
    lidbg("%s:enter\n", __func__);
    dsi83_dump_reg();
    return 1;
}


ssize_t dsi83_set_normal_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    PROC_READ_CHECK;
    lidbg("%s:enter\n", __func__);
    buf_piont = dsi83_conf[2 * dsi83_conf_num];

    dsi83_config();
    return 1;
}


ssize_t dsi83_set_pattern_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    PROC_READ_CHECK;
    lidbg("%s:enter\n", __func__);
    buf_piont = dsi83_conf[2 * dsi83_conf_num + 1];

    dsi83_config();

    return 1;
}

static const struct file_operations dsi83_rst_fops =
{
    .read  = dsi83_rst_proc,
};

static const struct file_operations dsi83_dump_fops =
{
    .read  = dsi83_dump_proc,
};

static const struct file_operations dsi83_n_fops =
{
    .read  = dsi83_set_normal_proc,
};

static const struct file_operations dsi83_p_fops =
{
    .read  = dsi83_set_pattern_proc,
};
static int thread_dsi83_check(void *data)
{
    //dsi83_dump_reg();
    dsi83_check();
    return 0;
}
int dsi83_open (struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t dsi83_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);


    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';

    if(0)
    {
        LCD_OFF;
        lidbg("dsi83.rst\n");
        is_dsi83_inited = false;
        dsi83_work_func(NULL);
        LCD_ON;
    }

    return size;

}

static  struct file_operations dsi83_nod_fops =
{
    .owner = THIS_MODULE,
    .write = dsi83_write,
    .open = dsi83_open,
};

void disable_dsi83(char *key_word, void *data)
{
    lidbg("==disable_dsi83==\n");
    SOC_IO_Output(0, DSI83_GPIO_EN, 0);
    /*
    	ssleep(4);
        lidbg("==reboot==\n");
    	lidbg_shell_cmd("reboot");
    	ssleep(2);
        kernel_restart(NULL);
     */
}

#ifdef SUSPEND_ONLINE
static int lidbg_dsi_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
        	//dsi83_resume();
        	 queue_delayed_work(dsi83_workqueue, &dsi83_work, DSI83_DELAY_TIME);
		break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
	      // is_dsi83_inited = false;
		break;
    default:
        break;
    }

    return NOTIFY_DONE;
}

static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_dsi_event,
};
#endif

static int dsi83_probe(struct platform_device *pdev)
{
    int ret;
    lidbg("%s:enter\n", __func__);

    MSM_DSI83_POWER_ON;

    if(is_dsi83_exist() < 0)
    {
        lidbg("dsi83.miss\n");
        /*
        if(g_var.is_fly)
        {
        	lidbg_insmod("/flysystem/lib/out/bx5b3a.ko");
        }
        else
        {
        	lidbg_insmod("/system/lib/modules/out/bx5b3a.ko");
        }
        return 0;
        */
    }

    proc_create("dsi83_rst", 0, NULL, &dsi83_rst_fops);
    proc_create("dsi83_dump", 0, NULL, &dsi83_dump_fops);
    proc_create("dsi83_n", 0, NULL, &dsi83_n_fops);
    proc_create("dsi83_p", 0, NULL, &dsi83_p_fops);

    INIT_DELAYED_WORK(&dsi83_work, dsi83_work_func);
    dsi83_workqueue = create_workqueue("dsi83");

    if((g_var.is_fly == 0)&&(g_var.recovery_mode == 0))
    {
        is_dsi83_inited = 0;
        queue_delayed_work(dsi83_workqueue, &dsi83_work, DSI83_DELAY_TIME);
    }
    else
    {
        is_dsi83_inited = 1;
        CREATE_KTHREAD(thread_dsi83_check, NULL);
    }

#ifdef SUSPEND_ONLINE
	register_lidbg_notifier(&lidbg_notifier);
       if(0)
#endif
	{
#if defined(CONFIG_FB)
	    dsi83_fb_notif.notifier_call = dsi83_fb_notifier_callback;
	    ret = fb_register_client(&dsi83_fb_notif);
	    if (ret)
	        lidbg("Unable to register dsi83_fb_notif: %d\n", ret);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#endif
}

    lidbg_new_cdev(&dsi83_nod_fops, "lidbg_dsi830");

    return 0;

}

static int dsi83_remove(struct platform_device *pdev)
{
    //	cancel_work_sync(dsi83_workqueue);
    //	flush_workqueue(dsi83_workqueue);
    //	destroy_workqueue(dsi83_workqueue);
    return 0;
}

#ifdef CONFIG_PM
static int dsi83_ops_suspend(struct device *dev)
{
    DUMP_FUN;
    is_dsi83_inited = false;
    SOC_IO_Output(0, DSI83_GPIO_EN, 0);
    MSM_DSI83_POWER_OFF;
    //dsi83_suspend();
    return 0;
}
static int thread_dsi83_ops_resume(void *data)
{
    MSM_DSI83_POWER_ON;

#ifndef SUSPEND_ONLINE
    msleep(200);
#endif

    dsi83_resume();
    return 1;
}
static int dsi83_ops_resume(struct device *dev)
{
    DUMP_FUN;
#ifdef SUSPEND_ONLINE
	if(0)
#endif
	CREATE_KTHREAD(thread_dsi83_ops_resume, NULL);

    return 0;
}
static struct dev_pm_ops dsi83_ops =
{
    .suspend	= dsi83_ops_suspend,
    .resume		= dsi83_ops_resume,
};
#endif

static struct platform_device dsi83_devices =
{
    .name			= "dsi83",
    .id 			= 0,
};

static struct platform_driver dsi83_driver =
{
    .probe = dsi83_probe,
    .remove = dsi83_remove,
    .driver = {
        .name = "dsi83",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &dsi83_ops,
#endif
    },
};

static int dsi83_init(void)
{
    int ret;
    static u32 screen_x, screen_y;
    DUMP_BUILD_TIME;
    LIDBG_GET;

    ret = soc_get_screen_res(&screen_x, &screen_y);
    if (!ret)
        lidbg("dsi83 get screen res ERR\n");

    if ((screen_x == 1280) && (screen_y == 400))
        dsi83_conf_num = 1;
    else if ((screen_x == 1024) && (screen_y == 768))
        dsi83_conf_num = 2;	
    else if ((screen_x == 768) && (screen_y == 1024))
        dsi83_conf_num = 3;
    else if ((screen_x == 1280) && (screen_y == 720))
        dsi83_conf_num = 4;
    else
        dsi83_conf_num = 0;		//1024x600 default

    lidbg("dsi83:x/y/num:%d/%d/%d\n",screen_x,screen_y,dsi83_conf_num);

    buf_piont = dsi83_conf[2 * dsi83_conf_num];

    platform_device_register(&dsi83_devices);
    platform_driver_register(&dsi83_driver);
    return 0;

}

static void __exit dsi83_exit(void) {}


module_init(dsi83_init);
module_exit(dsi83_exit);
MODULE_LICENSE("GPL");

