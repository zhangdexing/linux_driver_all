
#include "lidbg.h"
#define TAG "dlidbg_uevent:"

#define SHELL_ERRS_FILE "/dev/dbg_msg"
int uevent_dbg = 0;
struct mutex lock;
static struct kfifo cmd_fifo;
#define SHELL_LINES (300)
#define PER_SHELL_SIZE (256)
#define FIFO_SIZE (SHELL_LINES*PER_SHELL_SIZE+SHELL_LINES*sizeof(u32))
struct mutex fifo_lock;
static wait_queue_head_t wait_queue;
bool is_lidbg_uevent_ready = 0;
static atomic_t shell_count = ATOMIC_INIT(0);

static struct class *lidbg_cdev_class = NULL;
loff_t node_default_lseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}
bool new_cdev(struct file_operations *cdev_fops, char *nodename)
{
    struct cdev *new_cdev = NULL;
    struct device *new_device = NULL;
    dev_t dev_number = 0;
    int major_number_ts = 0;
    int err, result;

    if(!cdev_fops->owner || !nodename)
    {
        LIDBG_ERR(TAG"cdev_fops->owner||nodename \n");
        return false;
    }

    new_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
    if (!new_cdev)
    {
        LIDBG_ERR(TAG"kzalloc \n");
        return false;
    }

    dev_number = MKDEV(major_number_ts, 0);
    if(major_number_ts)
        result = register_chrdev_region(dev_number, 1, nodename);
    else
        result = alloc_chrdev_region(&dev_number, 0, 1, nodename);

    if (result)
    {
        LIDBG_ERR(TAG"alloc_chrdev_region result:%d \n", result);
        return false;
    }
    major_number_ts = MAJOR(dev_number);

    if(!cdev_fops->llseek)
        cdev_fops->llseek = node_default_lseek;

    cdev_init(new_cdev, cdev_fops);
    new_cdev->owner = cdev_fops->owner;
    new_cdev->ops = cdev_fops;
    err = cdev_add(new_cdev, dev_number, 1);
    if (err)
    {
        LIDBG_ERR(TAG"cdev_add result:%d \n", err);
        return false;
    }

    if(!lidbg_cdev_class)
    {
        lidbg_cdev_class = class_create(cdev_fops->owner, "lidbg_cdev_class");
        if(IS_ERR(lidbg_cdev_class))
        {
            LIDBG_ERR(TAG"class_create\n");
            cdev_del(new_cdev);
            kfree(new_cdev);
            lidbg_cdev_class = NULL;
            return false;
        }
    }

    new_device = device_create(lidbg_cdev_class, NULL, dev_number, NULL, "%s", nodename);
    if (!new_device)
    {
        LIDBG_ERR(TAG"device_create\n");
        cdev_del(new_cdev);
        kfree(new_cdev);
        return false;
    }

    return true;
}
bool lidbg_new_cdev(struct file_operations *cdev_fops, char *nodename)
{
    if(new_cdev(cdev_fops, nodename))
    {
        char path[32];
        sprintf(path, "/dev/%s", nodename);
        LIDBG_SUC(TAG"D[%s]\n", path);
        return true;
    }
    else
    {
        LIDBG_ERR(TAG"[/dev/%s]\n", nodename);
        return false;
    }
}

int lidbg_uevent_open(struct inode *inode, struct file *filp)
{
    LIDBG_WARN(TAG"in\n");
    is_lidbg_uevent_ready = 1;
    return 0;
}

ssize_t  lidbg_uevent_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
    char *tmp;
    tmp = memdup_user(buf, count + 1);
    if (IS_ERR(tmp))
    {
        LIDBG_ERR(TAG"<memdup_user>\n");
        return PTR_ERR(tmp);
    }
    tmp[count-1] = '\0';

    if(uevent_dbg)
        LIDBG_WARN(TAG"%s\n", tmp);

    if(!strcmp(tmp, "dbg"))
    {
        uevent_dbg = !uevent_dbg;
    }
    else if(!strcmp(tmp, "shell"))
    {
        lidbg_uevent_shell(tmp + 5);
    }
    else if(!strcmp(tmp, "size"))
    {
        LIDBG_WARN(TAG"[%s]:cmd_fifo len: %d byte/%d KB  kfifo_avail:%d KB\n", __func__, kfifo_len(&cmd_fifo), kfifo_len(&cmd_fifo) / 1024, kfifo_avail(&cmd_fifo) / 1024);
    }
    LIDBG_WARN(TAG"%d,%s\n", uevent_dbg, tmp);

    kfree(tmp);
    return count;
}
ssize_t  lidbg_uevent_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    u32 len, mstrlen;
    char msg_clean_buff[PER_SHELL_SIZE] = {0};

    if(kfifo_is_empty(&cmd_fifo))
        return 0;
    mutex_lock(&fifo_lock);
    if (kfifo_out(&cmd_fifo, (unsigned char *) &mstrlen, sizeof(u32)) !=  sizeof(u32))
    {
        LIDBG_WARN(TAG"\n critical error:kfifo_out.mstrlen.error.return\n");
        mutex_unlock(&fifo_lock);
        return 0;
    }
    len = kfifo_out(&cmd_fifo, msg_clean_buff , mstrlen);
    msg_clean_buff[mstrlen + 1] = '\0';
    mutex_unlock(&fifo_lock);
    //if(uevent_dbg)
    //LIDBG_WARN(TAG"kfifo_len:[%d byte ] strlen:%d/len:%d,shell:[%s]\n", kfifo_len(&cmd_fifo), mstrlen, len, msg_clean_buff);
    atomic_sub_return(1, &shell_count);
    if(copy_to_user(buf, msg_clean_buff, mstrlen))
    {
        return -1;
    }
    return mstrlen;
}

static unsigned int lidbg_uevent_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    poll_wait(filp, &wait_queue, wait);
    //if(uevent_dbg)
    //LIDBG_WARN(TAG"out.kfifo_len:[%d byte ]\n", kfifo_len(&cmd_fifo));
    mutex_lock(&fifo_lock);
    if(!kfifo_is_empty(&cmd_fifo))
    {
        mask |= POLLIN | POLLRDNORM;
    }
    mutex_unlock(&fifo_lock);
    return mask;
}


static  struct file_operations lidbg_uevent_fops =
{
    .owner = THIS_MODULE,
    .open = lidbg_uevent_open,
    .write = lidbg_uevent_write,
    .read = lidbg_uevent_read,
    .poll = lidbg_uevent_poll,
};

static int __init lidbg_uevent_init(void)
{
    int rc;
    LIDBG_WARN(TAG".in\n");
    mutex_init(&lock);
    mutex_init(&fifo_lock);
    init_waitqueue_head(&wait_queue);

    if (kfifo_alloc(&cmd_fifo, FIFO_SIZE, GFP_KERNEL) < 0)
    {
        LIDBG_WARN(TAG"kfifo_alloc fail\n");
        return -1;
    }
    rc = lidbg_new_cdev(&lidbg_uevent_fops, "lidbg_uevent");
    while(!rc )
    {
        LIDBG_ERR(TAG"lidbg_new_cdev:error\n");
        ssleep(1);
    }
    LIDBG_WARN(TAG"lidbg_uevent_init,cmd_fifo:[%d KB],kfifo_avail:[%d KB] kfifo_len:[%d KB]\n", (u32)(FIFO_SIZE / 1024), kfifo_avail(&cmd_fifo) / 1024, kfifo_len(&cmd_fifo) / 1024);
    return 0;
}

static void __exit lidbg_uevent_exit(void)
{
}

void lidbg_uevent_shell(char *shell_cmd)
{

    u32 mstrlen = strlen(shell_cmd);
    if(mstrlen >= PER_SHELL_SIZE)
    {
        LIDBG_ERR(TAG"error:\n\n\n\n\n\n\n ignoreshell: size too big [%d>%d][%s]\n\n\n\n\n\n\n", mstrlen, PER_SHELL_SIZE, shell_cmd);
        return;
    }
    while(kfifo_is_full(&cmd_fifo) || kfifo_len(&cmd_fifo) >= ( FIFO_SIZE - mstrlen * 2))
    {
        LIDBG_WARN(TAG"kfifo_is_full:[%d],kfifo_len [%d byte]\n", kfifo_is_full(&cmd_fifo), kfifo_len(&cmd_fifo));
        msleep(100);
    }
    mutex_lock(&fifo_lock);
    kfifo_in(&cmd_fifo, (unsigned char *) &mstrlen, sizeof(u32));
    kfifo_in(&cmd_fifo, shell_cmd, mstrlen);
    mutex_unlock(&fifo_lock);
    atomic_add_return(1, &shell_count);

    if(uevent_dbg)
        LIDBG_WARN(TAG"shell_count:%d kfifo_len:%d byte,mstrlen:%d/[%s]\n", atomic_read(&shell_count), kfifo_len(&cmd_fifo),  mstrlen, shell_cmd);
    wake_up_interruptible(&wait_queue);
}


EXPORT_SYMBOL(lidbg_uevent_shell);
EXPORT_SYMBOL(lidbg_new_cdev);
EXPORT_SYMBOL(is_lidbg_uevent_ready);

module_init(lidbg_uevent_init);
module_exit(lidbg_uevent_exit);

MODULE_DESCRIPTION("futengfei 2014.3.8");
MODULE_LICENSE("GPL");

