
#include "lidbg.h"
struct ad_device
{
    char *name;
    unsigned int counter;
    wait_queue_head_t queue;
    struct semaphore sem;
    struct cdev cdev;

};
static struct kfifo ad_data_fifo;
#define FIFO_SIZE (512)
u32 *fifo_buffer;
#define DATA_SIZE (512)
u32 *ad_data_for_app;

struct ad_device *dev;
struct completion ad_val;

LIDBG_DEFINE;
int find_ad_key(struct ad_key_remap *p)
{
    static int val_buf[4] = {0};
    static int count = 0;
    int val = 0;
    int i;
    if((g_hw.ad_val_mcu > 0) && (g_var.is_fly == 0))
    {
        if(LPC_ADC_Get(p->ch, &val) == 0)
            val = p->max;
    }
    else
    {
        if(SOC_ADC_Get(p->ch, &val) == 0)
            val = p->max;
    }
    if(val > p->max)
        val = p->max;
    pr_debug("ch=%d,val=%d\n", p->ch, val);
    val_buf[count++] = val;
    if(count < 4)
        return  1;

    count = 0;
    for(i = 0; i < 4; i++)
    {
        if((val_buf[i] > 0) && (val_buf[i] < p->max - p->offset))
        {
            if(kfifo_is_full(&ad_data_fifo))
            {
                lidbg("=======kfifo_reset======\n");
                kfifo_reset(&ad_data_fifo);
            }
            down(&dev->sem);
            kfifo_in(&ad_data_fifo, val_buf, 16);
            up(&dev->sem);
            wake_up_interruptible(&dev->queue);
            complete(&ad_val);
            break;
        }
    }

    return 1;
}

void key_scan(void)
{
    int key = 0;
    int i;

    for(i = 0; i < SIZE_OF_ARRAY(g_hw.ad_key); i++)
    {
        key = find_ad_key(&(g_hw.ad_key[i]));

    }
}
int thread_check_key(void *data)
{
    int ad_en;
    DUMP_FUN;
    FS_REGISTER_INT(ad_en, "ad_en", 1, NULL);
    if(ad_en)
    {
        while(1)
        {
            if(g_var.fb_on)
            {
                key_scan();
                msleep(50);
            }
            else
                msleep(1000);
        }
    }
    return 0;
}

int thread_handle_key(void *data)
{
    int i, k, bytes;
    while(1)
    {
        wait_for_completion(&ad_val);
        while(kfifo_len(&ad_data_fifo))
        {
            down(&dev->sem);
            bytes = kfifo_out(&ad_data_fifo, ad_data_for_app, 16);
            up(&dev->sem);
            for(k = 0; k < SIZE_OF_ARRAY(g_hw.ad_key); k++)
            {
                for(i = 0; i < SIZE_OF_ARRAY(g_hw.ad_key[k].key_item); i++)
                {
                    if((ad_data_for_app[k] > g_hw.ad_key[k].key_item[i].ad_value - g_hw.ad_key[k].offset) && (ad_data_for_app[k] < g_hw.ad_key[k].key_item[i].ad_value + g_hw.ad_key[k].offset))
                    {
                        lidbg("key=%d\n", g_hw.ad_key[k].key_item[i].send_key);
                        if(g_var.recovery_mode == 1)
                            SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
                        else
                            SOC_Key_Report(g_hw.ad_key[k].key_item[i].send_key, KEY_PRESSED_RELEASED);
                    }
                }
            }
        }
    }
}

ssize_t  ad_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{

    struct ad_device *dev = filp->private_data;
    int bytes, fifo_len;
    int read_len;
    if(size <= 0)
        return 0;
    down(&dev->sem);
    fifo_len = kfifo_len(&ad_data_fifo);
    if(fifo_len <= size)
        read_len = fifo_len;
    else
        read_len = size;
    bytes = kfifo_out(&ad_data_fifo, ad_data_for_app, read_len);
    up(&dev->sem);
    if (copy_to_user(buffer, ad_data_for_app, read_len))
    {
        lidbg("copy_to_user ERR\n");
    }
    if(fifo_len > bytes)
    {
        wake_up_interruptible(&dev->queue);
    }
    return read_len;
}

ssize_t  ad_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    return size;
}

int ad_open (struct inode *inode, struct file *filp)
{
    filp->private_data = dev;
    return 0;
}
static unsigned int ad_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0, fifo_len = 0;
    struct ad_device *dev = filp->private_data;
    poll_wait(filp, &dev->queue, wait);
    down(&dev->sem);
    fifo_len = kfifo_len(&ad_data_fifo);
    if(fifo_len >= 16)
    {
        mask |= POLLIN | POLLRDNORM;
        pr_debug("plc poll have data!!!\n");
    }
    up(&dev->sem);
    return mask;

}
static  struct file_operations ad_nod_fops =
{
    .owner = THIS_MODULE,
    .write = ad_write,
    .read = ad_read,
    .open =  ad_open,
    .poll =  ad_poll,
};

static int ad_ops_suspend(struct device *dev)
{
    lidbg("-----------ad_suspend------------\n");
    DUMP_FUN;

    return 0;
}

static int ad_ops_resume(struct device *dev)
{

    lidbg("-----------ad_resume------------\n");
    DUMP_FUN;

    return 0;
}
static struct dev_pm_ops ad_ops =
{
    .suspend	= ad_ops_suspend,
    .resume	= ad_ops_resume,
};
static int ad_probe(struct platform_device *pdev)
{
    lidbg("-----------ad_probe------------\n");
    dev = (struct ad_device *)kmalloc( sizeof(struct ad_device), GFP_KERNEL );
    ad_data_for_app = (u32 *)kmalloc(DATA_SIZE, GFP_KERNEL);
    fifo_buffer = (u32 *)kmalloc(FIFO_SIZE, GFP_KERNEL);
    if((ad_data_for_app == NULL) || (fifo_buffer == NULL))
    {
        lidbg("ad_probe kmalloc err\n");
        return 0;
    }
    init_waitqueue_head(&dev->queue);
    sema_init(&dev->sem, 1);
    kfifo_init(&ad_data_fifo, fifo_buffer, FIFO_SIZE);
    lidbg_new_cdev(&ad_nod_fops, "lidbg_ad0");
    init_completion(&ad_val);
    if((g_var.recovery_mode == 1) || (g_var.is_fly == 0))
    {
        CREATE_KTHREAD(thread_check_key, NULL);
        CREATE_KTHREAD(thread_handle_key, NULL);
    }
    else
    {
        //if(g_hw.ad_val_mcu==0)
        //	CREATE_KTHREAD(thread_check_key, NULL);

    }
    return 0;
}
static int ad_remove(struct platform_device *pdev)
{
    return 0;
}
static struct platform_device ad_devices =
{
    .name			= "ad",
    .id 			= 0,
};

static struct platform_driver ad_driver =
{
    .probe = ad_probe,
    .remove = ad_remove,
    .driver = {
        .name = "ad",
        .owner = THIS_MODULE,
        .pm = &ad_ops,

    },
};

static int  ad_init(void)
{
    lidbg("ad_driver\n");
    LIDBG_GET;
    platform_device_register(&ad_devices);
    platform_driver_register(&ad_driver);

    return 0;


}

static void  ad_exit(void)
{
    printk("chdrv_exit\n");

}

module_init(ad_init);
module_exit(ad_exit);



MODULE_AUTHOR("fly, <fly@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");





