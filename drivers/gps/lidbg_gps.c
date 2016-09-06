

#include "lidbg.h"
LIDBG_DEFINE;

#define RAW_DATA_FILE "/sdcard/gps_raw.txt"
#define DEVICE_NAME "ubloxgps"

#define GPS_START	_IO('g', 1)
#define GPS_STOP	_IO('g', 2)


struct gps_device
{
    char *name;
    unsigned int counter;
    wait_queue_head_t queue;
    struct semaphore sem;
    struct cdev cdev;
};

static int started = 0;
static int  work_en = 1;
static int  sdcard_raw_data = 0;
module_param_named(work_en, work_en, int, 0644 );
#ifdef CONFIG_HAS_EARLYSUSPEND
struct early_suspend early_suspend;
#endif
#define GPS_BUF_SIZE (1024*4)
u8 *gps_data;


#define HAL_BUF_SIZE (512)
u8 *gps_data_for_hal;


#define FIFO_SIZE (1024*4)
u8 *fifo_buffer;
static struct kfifo gps_data_fifo;


static char  num_avi_gps_data[2] = { 0 };
static int    avi_gps_data_hl = 0;
int thread_gps_server(void *data);

int gps_debug_en = 0;

struct gps_device *dev;
void clean_ublox_buf(void);

static int gps_event_handle(struct notifier_block *this,
                            unsigned long event, void *ptr)
{
    DUMP_FUN;
#ifdef SOC_msm8x25

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_ACC_ON):
        lidbg("set work_en = 1\n");
        work_en = 1;
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_ACC_OFF):
        lidbg("gps set work_en = 0\n");
        work_en = 0;
        break;

    default:
        break;
    }
#else
    switch (event)
    {
#ifdef SUSPEND_ONLINE
	case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
#else
	case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_UP):
#endif
        clean_ublox_buf();
        down(&dev->sem);
        kfifo_reset(&gps_data_fifo);
        up(&dev->sem);

        lidbg("gps set work_en = 1\n");
        work_en = 1;
	 GPS_POWER_ON;
        break;

	case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
        lidbg("gps set work_en = 0\n");
        work_en = 0;
	 GPS_POWER_OFF;
        break;

     default:
        break;
    }


#endif
    return NOTIFY_DONE;
}


static struct notifier_block gps_notifier =
{
    .notifier_call = gps_event_handle,
};



int gps_open (struct inode *inode, struct file *filp)
{
    filp->private_data = dev;
    lidbg("[ublox]gps_open\n");
    return 0;
}


ssize_t gps_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct gps_device *dev = filp->private_data;
    int read_len, fifo_len, bytes;

    if (!started)
    {
        lidbg("[ublox] gps stoped but read\n");
        return -1;
    }

    if(kfifo_is_empty(&gps_data_fifo))
    {
        if(wait_event_interruptible(dev->queue, !kfifo_is_empty(&gps_data_fifo)))
            return -ERESTARTSYS;
    }

    down(&dev->sem);
    fifo_len = kfifo_len(&gps_data_fifo);

    if(fifo_len > HAL_BUF_SIZE)
        read_len = HAL_BUF_SIZE;
    else
        read_len = fifo_len;

    bytes = kfifo_out(&gps_data_fifo, gps_data_for_hal, read_len);
    up(&dev->sem);

    if(copy_to_user(buf, gps_data_for_hal, read_len))
    {
        return -1;
    }

    if(fifo_len > HAL_BUF_SIZE)
        wake_up_interruptible(&dev->queue);

    if(gps_debug_en)
        lidbg("[ublox]fifo_len: %d, jni_read: %d\n", fifo_len, read_len);
    return read_len;
}


ssize_t gps_write (struct file *filp, const char __user *buf, size_t size, loff_t *f_pos)
{
    int argc = 0;
    char *argv[32] = {NULL};
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';

    argc = lidbg_token_string(cmd_buf, " ", argv);
    if (!strcmp(argv[0], "raw"))
    {
        sdcard_raw_data = !sdcard_raw_data;
        lidbg("sdcard_raw_data:%d\n", sdcard_raw_data);
        fs_file_separator(RAW_DATA_FILE);
    }

    return size;
}

static unsigned int gps_poll(struct file *filp, struct poll_table_struct *wait)
{
    struct gps_device *dev = filp->private_data;
    unsigned int mask = 0;

    //   lidbg("ublox:gps_poll called");


    poll_wait(filp, &dev->queue, wait);
    if (!started)
    {
        //	lidbg("ublox:gps not start but poll");
        return 0;
    }

    down(&dev->sem);
    if(!kfifo_is_empty(&gps_data_fifo))
    {
        mask |= POLLIN | POLLRDNORM;
        //	lidbg("ublox:gps poll have data");
    }
    up(&dev->sem);
    return mask;
}

static long gps_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    static bool flag = 1;
    if(flag)
    {
        fs_mem_log("ublox_so_load=true\n");
        flag = 0;
    }
    switch(cmd)
    {
    case GPS_START:
        lidbg("[ublox]:ioctl GPS START\n");
        started = 1;
        break;
    case GPS_STOP:
        lidbg("[ublox]:ioctl GPS STOP\n");
        started = 0;
        break;
    default:
        return -ENOTTY;
    }

    return 0;
}

static  struct file_operations gps_fops =
{
    .owner = THIS_MODULE,
    .read = gps_read,
    .write = gps_write,
    .poll = gps_poll,
    .open = gps_open,
    .unlocked_ioctl = gps_ioctl,
};


void clean_ublox_buf(void)
{
    int ret;
    DUMP_FUN_ENTER;

    ret = SOC_I2C_Rec(GPS_I2C_BUS, 0x42, 0xfd, num_avi_gps_data, 2);
    if (ret < 0)
        return;

    avi_gps_data_hl = (num_avi_gps_data[0] << 8) + num_avi_gps_data[1];
    if(avi_gps_data_hl > 0)
        SOC_I2C_Rec_Simple(GPS_I2C_BUS, 0x42, gps_data, avi_gps_data_hl);

    DUMP_FUN_LEAVE;

}


int thread_gps_server(void *data)
{
    int ret,i;
    DUMP_FUN_ENTER;
    while(1)
    {
        if((work_en == 0) || (started == 0))
        {
            if(gps_debug_en)
                lidbg("[ublox]goto do_nothing:%d,%d\n", work_en, started);
            goto do_nothing;
        }
        ret = SOC_I2C_Rec(GPS_I2C_BUS, 0x42, 0xfd, num_avi_gps_data, 2);
        if (ret < 0)
        {
            avi_gps_data_hl = 0;
            lidbg("[ublox]get avi_gps_data_hl err!!\n");
        }
        else
            avi_gps_data_hl = (num_avi_gps_data[0] << 8) + num_avi_gps_data[1];


        //      if(gps_debug_en)
        pr_debug("[ublox]ublox_buf_len: %d\n", avi_gps_data_hl);

        if(avi_gps_data_hl > 0)
        {
            if(avi_gps_data_hl <= GPS_BUF_SIZE)
            {
                ret = SOC_I2C_Rec_Simple(GPS_I2C_BUS, 0x42, gps_data, avi_gps_data_hl);
                gps_data[avi_gps_data_hl ] = '\0';
                pr_debug("gps_data=%s\n", gps_data);
		if(gps_data[0] == 0xb5)
		{
		    pr_debug("=====start=====\n");
		    for(i = 0; i < avi_gps_data_hl; i++)
		        pr_debug("data %d=0x%x\n",   i  , gps_data[i]);
		    pr_debug("=====stop=====\n");
		}
                if (sdcard_raw_data)
                {
                    fs_file_write2(RAW_DATA_FILE, gps_data);
                }
                if (ret < 0)
                {
                    lidbg("[ublox]get gps data err!!\n");
                    goto do_nothing;
                }
                down(&dev->sem);
                if(kfifo_is_full(&gps_data_fifo))
                {
                    kfifo_reset(&gps_data_fifo);
                    lidbgerr("[ublox]kfifo_reset!!!!!\n");
                }
                kfifo_in(&gps_data_fifo, gps_data, avi_gps_data_hl);
                up(&dev->sem);
            }
            else
            {
                lidbg("[ublox]data len err!!!!!!\n");
                goto do_nothing;
            }
        }
        else
        {
            goto do_nothing;

        }

        if (started)
        {
            if (gps_debug_en)
                lidbg("ublox:have data need read\n");
            wake_up_interruptible(&dev->queue);
        }
        if(gps_debug_en)
        {
            gps_data[avi_gps_data_hl ] = '\0';
            printk(KERN_CRIT"%s\n", gps_data);
        }

do_nothing:
        msleep(250);

    }
    DUMP_FUN_LEAVE;
    return 0;
}


ssize_t read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    PROC_READ_CHECK;
    lidbg("enable ublox print\n");
    gps_debug_en = 1;

    return 1;
}

static const struct file_operations ublox_dbg_fops =
{
    .read  = read_proc,
};

void create_new_proc_entry(void)
{
    proc_create("ublox_dbg", 0, NULL, &ublox_dbg_fops);
    // /cat proc/ublox_dbg
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void gps_early_suspend(struct early_suspend *h)
{
    DUMP_FUN;
    work_en = 0;

    down(&dev->sem);
    kfifo_reset(&gps_data_fifo);
    up(&dev->sem);

}

static void gps_late_resume(struct early_suspend *h)
{
    DUMP_FUN;
    clean_ublox_buf();

    down(&dev->sem);
    kfifo_reset(&gps_data_fifo);
    up(&dev->sem);

    work_en = 1;

}

#endif

int is_ublox_exist(void)
{
    int exist, retry;
    for(retry = 0; retry < 10; retry++)
    {
        exist = SOC_I2C_Rec_Simple(GPS_I2C_BUS, 0x42, gps_data, 1 );
        if (exist < 0)
        {
            msleep(100);
            continue;
        }
        else
        {
            return 1;
        }
    }
    return -1;
}



static int  gps_probe(struct platform_device *pdev)
{
    int ret, err, result;
    static struct class *class_install;
    static int major_number = 0;
    dev_t dev_number = MKDEV(major_number, 0);
    gps_data = (u8 *)kmalloc(GPS_BUF_SIZE + 1, GFP_KERNEL);
    gps_data_for_hal = (u8 *)kmalloc(HAL_BUF_SIZE, GFP_KERNEL);
    fifo_buffer = (u8 *)kmalloc(FIFO_SIZE, GFP_KERNEL);
    if((gps_data == NULL) || (gps_data_for_hal == NULL) || (fifo_buffer == NULL))
    {
        lidbg("gps_probe kmalloc err\n");
        return 0;
    }
    DUMP_FUN;

    if(g_var.recovery_mode)
    {
        lidbg("gps_probe do nothing\n");
        return 0;
    }

#ifndef SOC_msm8x25
    if(is_ublox_exist() < 0)
    {
#if 0
        if(g_var.is_first_update)
        {
            lidbg_shell_cmd("mount -o remount /flysystem");
            lidbg_shell_cmd("rm /flysystem/lib/out/"FLY_GPS_SO);
            lidbg_shell_cmd("mount -o remount,ro /flysystem");
        }
#endif
        lidbg("[ublox]ublox.miss\n\n");
        kfree(gps_data);
        kfree(gps_data_for_hal);
        kfree(fifo_buffer);
        return 0;
    }
    else
    {
#if 0
        if(g_var.is_first_update)
        {
            lidbg_shell_cmd("mount -o remount /flysystem");
            lidbg_shell_cmd("mv /flysystem/lib/hw/gps.soc.bak  /flysystem/lib/hw/"FLY_GPS_SO);
        }
#endif
        lidbg("[ublox]ublox.exist\n\n");
        fs_mem_log("ublox_exist=true\n");

    }
#endif

    FS_REGISTER_INT(gps_debug_en, "gps_debug_en", 0, NULL);

#ifdef CONFIG_HAS_EARLYSUSPEND  //  enable/disable the gps thread 
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    early_suspend.suspend = gps_early_suspend;
    early_suspend.resume = gps_late_resume;
    register_early_suspend(&early_suspend);
#endif

    //fake suspend
    //SOC_Fake_Register_Early_Suspend(&early_suspend);

    create_new_proc_entry(); //cat /proc/ublox_dbg to enable gps_debug

    // 1creat cdev
    dev = (struct gps_device *)kmalloc( sizeof(struct gps_device), GFP_KERNEL );
    if (dev == NULL)
    {
        ret = -ENOMEM;
        lidbg("[ublox]gps_server_driver_init:kmalloc err\n");
        return ret;
    }

    if(major_number)
    {
        result = register_chrdev_region(dev_number, 1, DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
        major_number = MAJOR(dev_number);
    }
    lidbg("[ublox]alloc_chrdev_region result:%d\n", result);

    cdev_init(&dev->cdev, &gps_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &gps_fops;
    err = cdev_add(&dev->cdev, dev_number, 1);
    if (err)
        lidbg( "[ublox]Error cdev_add\n");

    //cread cdev node in /dev
    class_install = class_create(THIS_MODULE, "gpsclass");
    if(IS_ERR(class_install))
    {
        lidbg( "[ublox]class_create err\n");
        return -1;
    }
    device_create(class_install, NULL, dev_number, NULL, "%s%d", DEVICE_NAME, 0);

    // 2creat thread
    CREATE_KTHREAD(thread_gps_server, NULL);

    // 3init all the tools
    init_waitqueue_head(&dev->queue);
    sema_init(&dev->sem, 1);
    kfifo_init(&gps_data_fifo, fifo_buffer, FIFO_SIZE);
    lidbg_chmod("/dev/ubloxgps0");

    register_lidbg_notifier(&gps_notifier);
    GPS_POWER_ON;
    return 0;
}


static int  gps_remove(struct platform_device *pdev)
{
    return 0;
}


#ifdef CONFIG_PM
static int gps_resume(struct device *dev)
{
    DUMP_FUN_ENTER;
    //work_en = 1;
    return 0;

}

static int gps_suspend(struct device *dev)
{
    DUMP_FUN_ENTER;
    //work_en = 0;
    return 0;

}

static struct dev_pm_ops gps_pm_ops =
{
    .suspend	= gps_suspend,
    .resume		= gps_resume,
};
#endif

static struct platform_driver gps_driver =
{
    .probe		= gps_probe,
    .remove     = gps_remove,
    .driver         = {
        .name = "lidbg_gps",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &gps_pm_ops,
#endif
    },
};

static struct platform_device lidbg_gps_device =
{
    .name               = "lidbg_gps",
    .id                 = -1,
};


static  int gps_server_init(void)
{
    lidbg("[ublox] ==IN==gps_server_driver_init==\n");

    LIDBG_GET;
    platform_device_register(&lidbg_gps_device);

    return platform_driver_register(&gps_driver);

}

static void gps_server_exit(void)
{
    lidbg("[ublox] ==IN==gps_server_driver_exit==\n");

    // misc_deregister(&misc);
    // kfree(dev);
}

module_init(gps_server_init);
module_exit(gps_server_exit);

MODULE_LICENSE("GPL");


