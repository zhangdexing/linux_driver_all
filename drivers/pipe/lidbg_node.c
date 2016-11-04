#include "lidbg.h"
LIDBG_DEFINE;

#define FIFO_SIZE (2*1024*1024)
#define BUFF_SIZE 1024
struct kfifo app2hal_fifo;
struct kfifo hal2app_fifo;
char *app2hal_fifo_buffer = NULL;
char *hal2app_fifo_buffer = NULL;

static spinlock_t fifo_app2hal_lock;
static spinlock_t fifo_hal2appl_lock;
unsigned long flags_app2hal;
unsigned long flags_hal2app;
static DECLARE_COMPLETION(app_read_wait);
//static DECLARE_COMPLETION(hal_read_wait);
wait_queue_head_t hal_read_wait;

static struct file_operations app_fops;
static struct file_operations hal_fops;




static int hal_open(struct inode *inode, struct file *filp)
{
    lidbg("hal_open\n");
    return 0;
}

static int hal_close(struct inode *inode, struct file *filp)
{
    lidbg("hal_close\n");
    return 0;
}

ssize_t  hal_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    int  count, fifo_len;
    static char tmp[BUFF_SIZE];

    lidbg("hal_read\n");

    fifo_len = kfifo_len(&app2hal_fifo);
    if(fifo_len < BUFF_SIZE)
        count = fifo_len;
    else
        count = BUFF_SIZE;

    spin_lock_irqsave(&fifo_app2hal_lock, flags_app2hal);
    count = kfifo_out(&app2hal_fifo, tmp, count);
    spin_unlock_irqrestore(&fifo_app2hal_lock, flags_app2hal);

    if (copy_to_user(buffer, tmp, count))
    {
        lidbg("copy_to_user ERR\n");
    }

    return count;

}
static ssize_t hal_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    //static char tmp[1500];
    char tmp[size];//C99 variable length array
    char *mem = tmp;

    lidbg("hal_write size = %d\n", size);

    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    spin_lock_irqsave(&fifo_hal2appl_lock, flags_hal2app);
    kfifo_in(&hal2app_fifo, mem, size);
    spin_unlock_irqrestore(&fifo_hal2appl_lock, flags_hal2app);

    complete(&app_read_wait);
    return size;
}

static unsigned int hal_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    interruptible_sleep_on(&hal_read_wait);
    mask |= POLLIN | POLLRDNORM;
    return mask;
}

static struct file_operations hal_fops =
{
    .owner = THIS_MODULE,
    .open = hal_open,
    .read = hal_read,
    .write = hal_write,
    .poll = hal_poll,
    .release = hal_close,
};


int app_open(struct inode *inode, struct file *filp)
{
    lidbg("app_open\n");
    return 0;
}
int app_close(struct inode *inode, struct file *filp)
{
    lidbg("app_close\n");
    return 0;
}


ssize_t  app_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    int  count, fifo_len, get;
    static char tmp[BUFF_SIZE];

    fifo_len = kfifo_len(&hal2app_fifo);
    lidbg("fifo_len =%d\n", fifo_len);

    if(fifo_len <= 0)
    {
        wait_for_completion(&app_read_wait);
        lidbg ("wait_for_completion ok\n");
    }

    fifo_len = kfifo_len(&hal2app_fifo);

    if(fifo_len < BUFF_SIZE)
        count = fifo_len;
    else
        count = BUFF_SIZE;

    spin_lock_irqsave(&fifo_hal2appl_lock, flags_hal2app);
    get = kfifo_out(&hal2app_fifo, tmp, count);
    spin_unlock_irqrestore(&fifo_hal2appl_lock, flags_hal2app);

    if (copy_to_user(buffer, tmp, count))
    {
        lidbg("copy_to_user ERR\n");
    }

    lidbg("app_read finished!%d,%d\n", count, get);
    return count;

}

static ssize_t app_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    //static char tmp[BUFF_SIZE];
    char tmp[size];//C99 variable length array
    char *mem = tmp;

    lidbg("app_write\n");
    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    spin_lock_irqsave(&fifo_app2hal_lock, flags_app2hal);
    kfifo_in(&app2hal_fifo, mem, size);
    spin_unlock_irqrestore(&fifo_app2hal_lock, flags_app2hal);

    wake_up_interruptible(&hal_read_wait);
    return size;
}


static struct file_operations app_fops =
{
    .owner = THIS_MODULE,
    .open = app_open,
    .read = app_read,
    .write = app_write,
    .release = app_close,
};

static int node_remove(struct platform_device *pdev)
{
    lidbg("node_remove\n");
    return 0;
}

static int node_probe(struct platform_device *pdev)
{
    lidbg("node_probe\n");

    INIT_COMPLETION(app_read_wait);
    init_waitqueue_head(&hal_read_wait);
    app2hal_fifo_buffer = (char *)kmalloc(FIFO_SIZE, GFP_KERNEL);
    hal2app_fifo_buffer = (char *)kmalloc(FIFO_SIZE, GFP_KERNEL);
    if(NULL == app2hal_fifo_buffer || NULL == hal2app_fifo_buffer)
    {
        lidbg("malloc fifo buffer failed.\n");
        return -1;
    }

    lidbg ("kfifo_init, FIFO_SIZE=%d\n", FIFO_SIZE);
    kfifo_init(&app2hal_fifo, app2hal_fifo_buffer, FIFO_SIZE);
    kfifo_init(&hal2app_fifo, hal2app_fifo_buffer, FIFO_SIZE);
    spin_lock_init(&fifo_app2hal_lock);
    spin_lock_init(&fifo_hal2appl_lock);

    lidbg_new_cdev(&app_fops, "appNode0");
    lidbg_new_cdev(&hal_fops, "halNode0");

    return 0;

}



static struct platform_device node_device =
{
    .name = "nodes",
    .id = 0,
};


static struct platform_driver node_device_driver =
{
    .probe 	= node_probe,
    .remove	= node_remove,
    .driver 	=
    {
        .name = "nodes",
        .owner = THIS_MODULE,
    },
};

static int __init node_init(void)
{
    int ret = 0;
    lidbg("node_init\n");
    ret = platform_device_register(&node_device);
    ret = platform_driver_register(&node_device_driver);

    return ret;
}

static void __exit node_exit(void)
{
    lidbg("node_exit\n");
}

module_init(node_init);
module_exit(node_exit);


MODULE_AUTHOR("Flyaudio Inc.");
MODULE_DESCRIPTION("supply nodes for APP communicate with HAL");
MODULE_LICENSE("GPL");
