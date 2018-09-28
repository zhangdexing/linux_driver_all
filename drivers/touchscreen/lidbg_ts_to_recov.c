#include "lidbg_recovery.h"
#include "lidbg.h"



//#include "../../globalDef.h"
#define TOUCH_MODE_MMAP    11
#define TOUCH_MODE_NORMAL  22
#define TOUCH_MODE_SEL     TOUCH_MODE_NORMAL
#define DEVICE_NAME "fenzhi0"
touch_t *touch;

touch_t *touchtemp;

#include <linux/spinlock.h>
DEFINE_SPINLOCK(touchlock);

bool bNeedRead;
wait_queue_head_t read_wait;

struct test_input_dev
{
    char *name;
    struct input_dev *input_dev;
    struct timer_list timer;
    unsigned int counter;
    struct miscdevice *miscdev;
};

bool bPress = false;
void set_touch_pos(touch_t *t)
{
    if (bPress == t->pressed)
    {
        return;
    }

    memcpy(touch, t, sizeof(touch_t));

    bPress = touch->pressed;

    spin_lock(&touchlock);
    bNeedRead = true;
    spin_unlock(&touchlock);
    wake_up_interruptible(&read_wait);
}

EXPORT_SYMBOL(set_touch_pos);

//global variable
struct test_input_dev *dev;
int fenzhi_major;
int data = 8;
#if 0
static void dev_timer_func(unsigned long arg)
{
    mod_timer(&dev->timer, jiffies + HZ);
    (dev->counter)++;
    if(dev->counter > 60)dev->counter = 0;
    //printk("touch.x:%d touch.y:%d touch.press:[%d]\n", touch.x, touch.y, touch.pressed);
    //printk("second%d\n", dev->counter);
    //input_report_abs(dev->input_dev, ABS_X, dev->counter);
    //input_report_abs(dev->input_dev, ABS_Y, dev->counter);
    //input_sync(dev->input_dev);
}
#endif

unsigned int fenzhi_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;

    if (!bNeedRead)
    {
        //printk("\n fenzhi_poll %d",bNeedRead);
        poll_wait(filp, &read_wait, wait);
        //interruptible_sleep_on(&read_wait);
    }

    if (bNeedRead)
    {
#if TOUCH_MODE_SEL == TOUCH_MODE_MMAP
        bNeedRead = false;
#endif

        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

int fenzhi_open (struct inode *inode, struct file *filp)
{
    printk("lidbg_ts_to_recov.ko  Open. \n");
    return 0;          /* success */
}

static int fenzhi_mmap(struct file *filp, struct vm_area_struct *vma)
{
    vma->vm_pgoff = ((unsigned long)virt_to_phys(touch)) >> PAGE_SHIFT;

    printk("vma->vm_pgoff = %lx\n", vma->vm_pgoff);

    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, sizeof(touch_t),  vma->vm_page_prot))
    {
        printk("remap_pfn_range error\n");
        return -EAGAIN;
    }

    return 0;
}

/*
 * Data management: read and write
 */
ssize_t fenzhi_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    //printk("\n fenzhi_read %d",bNeedRead);
    spin_lock(&touchlock);
    if (bNeedRead)
    {
        bNeedRead = false;
        spin_unlock(&touchlock);
        if(copy_to_user(buf, touch, sizeof(touch_t)) == 0)
            return sizeof(touch_t);
    }
    else
    {
        spin_unlock(&touchlock);
        //return 0;
    }
    return 0;
}



ssize_t fenzhi_write (struct file *filp, const char __user *buf, size_t count,
                      loff_t *f_pos)
{

    return 0;
}


static  struct file_operations fenzhi_fops =
{
    .owner =     THIS_MODULE,
    .read  =	 fenzhi_read,
    .write =     fenzhi_write,
    .poll  =	 fenzhi_poll,
    .open  =	 fenzhi_open,
    .mmap  =	 fenzhi_mmap,

};


int thread_recov_init(void *data)
{
    lidbg_new_cdev(&fenzhi_fops, DEVICE_NAME);

    return 0;
}



//static int __devinit
//static int initialized = 0;

static  int my_input_driver_init(void)
{
    DUMP_BUILD_TIME;
    //LIDBG_GET;
    \



    //printk("my_input_driver_init,write by hujian 2012/2/8 16:45");
    lidbg("xxxxxlidbg_ts_to_recov.ko  my_input_driver_init. \n");

    CREATE_KTHREAD(thread_recov_init, NULL);


    bNeedRead = false;
    init_waitqueue_head(&read_wait);

#if TOUCH_MODE_SEL == TOUCH_MODE_MMAP
    touchtemp = (touch_t *)kmalloc(sizeof(touch_t), GFP_KERNEL);
    touch = (touch_t *)(PAGE_ALIGN((unsigned long)touchtemp));
    memset(touch, 0, sizeof(touch_t));
#elif TOUCH_MODE_SEL == TOUCH_MODE_NORMAL
    touch = (touch_t *)kmalloc(sizeof(touch_t), GFP_KERNEL);
#endif

    lidbg("lidbg_ts_to_recov.ko  my_input_driver_init. OK\n");

    return 0;
}

//void __exit
static void my_input_driver_exit(void)
{

}
//EXPORT_SYMBOL_GPL(my_input_driver_init);
//EXPORT_SYMBOL_GPL(my_input_driver_exit);





module_init(my_input_driver_init);
module_exit(my_input_driver_exit);

MODULE_AUTHOR("hujian");
MODULE_DESCRIPTION("input driver,just for test");
MODULE_LICENSE("GPL");
