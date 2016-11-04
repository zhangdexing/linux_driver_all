
#include "lidbg.h"

struct uevent_dev
{
    struct list_head tmp_list;
    char *focus;
    void (*callback)(char *focus, char *uevent);
};

#define SHELL_ERRS_FILE "/dev/dbg_msg"
LIST_HEAD(uevent_list);
struct miscdevice lidbg_uevent_device;
int uevent_dbg = 0;
struct mutex lock;
static struct kfifo cmd_fifo;
#define SHELL_LINES (300)
#define PER_SHELL_SIZE (256)
#define FIFO_SIZE (SHELL_LINES*PER_SHELL_SIZE+SHELL_LINES*sizeof(u32))
//static DECLARE_COMPLETION(cmd_ready);
struct mutex fifo_lock;
static wait_queue_head_t wait_queue;


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
        LIDBG_ERR("cdev_fops->owner||nodename \n");
        return false;
    }

    new_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
    if (!new_cdev)
    {
        LIDBG_ERR("kzalloc \n");
        return false;
    }

    dev_number = MKDEV(major_number_ts, 0);
    if(major_number_ts)
        result = register_chrdev_region(dev_number, 1, nodename);
    else
        result = alloc_chrdev_region(&dev_number, 0, 1, nodename);

    if (result)
    {
        LIDBG_ERR("alloc_chrdev_region result:%d \n", result);
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
        LIDBG_ERR("cdev_add result:%d \n", err);
        return false;
    }

    if(!lidbg_cdev_class)
    {
        lidbg_cdev_class = class_create(cdev_fops->owner, "lidbg_cdev_class");
        if(IS_ERR(lidbg_cdev_class))
        {
            LIDBG_ERR("class_create\n");
            cdev_del(new_cdev);
            kfree(new_cdev);
            lidbg_cdev_class = NULL;
            return false;
        }
    }

    new_device = device_create(lidbg_cdev_class, NULL, dev_number, NULL, "%s", nodename);
    if (!new_device)
    {
        LIDBG_ERR("device_create\n");
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
        LIDBG_SUC("D[%s]\n", path);
        return true;
    }
    else
    {
        LIDBG_ERR("[/dev/%s]\n", nodename);
        return false;
    }
}


bool uevent_focus(char *focus, void(*callback)(char *focus, char *uevent))
{
    struct uevent_dev *add_new_dev = NULL;
    add_new_dev = kzalloc(sizeof(struct uevent_dev), GFP_KERNEL);
    if (add_new_dev != NULL && focus && callback)
    {
        add_new_dev->focus = focus;
        add_new_dev->callback = callback;
        list_add(&(add_new_dev->tmp_list), &uevent_list);
        LIDBG_SUC("%s\n", focus);
        return true;
    }
    LIDBG_ERR("add_new_dev != NULL && focus && callback?\n");
    return false;
}

void uevent_send(enum kobject_action action, char *envp_ext[])
{
    if(uevent_dbg)
        LIDBG_WARN("%s,%s\n", (envp_ext[0] == NULL ? "null" : envp_ext[0]), (envp_ext[1] == NULL ? "null" : envp_ext[1]));
    mutex_lock(&lock);
    if(kobject_uevent_env(&lidbg_uevent_device.this_device->kobj, action, envp_ext) < 0)
        LIDBG_ERR("uevent_send\n");
    mutex_unlock(&lock);
}

void uevent_shell(char *shell_cmd)
{
    char shellstring[256];
    char *envp[] = { "LIDBG_ACTION=shell", shellstring, NULL };
    if(strstr(shell_cmd, "insmod"))
        snprintf(shellstring, 256, "LIDBG_PARAMETER=%s", shell_cmd );
    else
        snprintf(shellstring, 256, "LIDBG_PARAMETER=%s 2>> "SHELL_ERRS_FILE, shell_cmd );
    lidbg_uevent_send(KOBJ_CHANGE, envp);
}

int lidbg_uevent_open(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  lidbg_uevent_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
    char *tmp;
    struct uevent_dev *pos;
    struct list_head *client_list = &uevent_list;
    tmp = memdup_user(buf, count + 1);
    if (IS_ERR(tmp))
    {
        LIDBG_ERR("<memdup_user>\n");
        return PTR_ERR(tmp);
    }
    tmp[count] = '\0';

    if(uevent_dbg)
        LIDBG_WARN("%s\n", tmp);

    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (pos->focus && pos->callback && strstr(tmp, pos->focus))
        {
            LIDBG_SUC("called: %s  %ps\n", pos->focus, pos->callback);
            pos->callback(pos->focus, tmp);
        }
    }
    kfree(tmp);
    return count;
}
ssize_t  lidbg_uevent_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    u32 len, mstrlen;
    char msg_clean_buff[PER_SHELL_SIZE] = {0};

    //wait_for_completion(&cmd_ready);
    if(kfifo_is_empty(&cmd_fifo))
        return 0;
    mutex_lock(&fifo_lock);
    if (kfifo_out(&cmd_fifo, (unsigned char *) &mstrlen, sizeof(u32)) !=  sizeof(u32))
    {
        LIDBG_WARN("\n critical error:kfifo_out.mstrlen.error.return\n");
        mutex_unlock(&fifo_lock);
        return 0;
    }
    len = kfifo_out(&cmd_fifo, msg_clean_buff , mstrlen);
    msg_clean_buff[mstrlen + 1] = '\0';
    mutex_unlock(&fifo_lock);
    if(uevent_dbg)
        LIDBG_WARN("kfifo_len:[%d byte ] strlen:%d/len:%d,shell:[%s]\n", kfifo_len(&cmd_fifo), mstrlen, len, msg_clean_buff);
    if(copy_to_user(buf, msg_clean_buff, mstrlen))
    {
        return -1;
    }
    return mstrlen;
}



static unsigned int lidbg_uevent_poll(struct file *filp, struct poll_table_struct *wait)
{

    unsigned int mask = 0;
    //LIDBG_WARN("[lidbg_uevent_poll]wait begin\n");
    poll_wait(filp, &wait_queue, wait);
    //LIDBG_WARN("[lidbg_uevent_poll]wait done\n");
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

/*
struct miscdevice lidbg_uevent_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &lidbg_uevent_fops,
};
*/

static int __init lidbg_uevent_init(void)
{
    int rc;
    //DUMP_BUILD_TIME;
/*
    if (misc_register(&lidbg_uevent_device))
        LIDBG_ERR("misc_register\n");
*/
    LIDBG_WARN(".in\n");
    rc = lidbg_new_cdev(&lidbg_uevent_fops, "lidbg_uevent");
    while(!rc )
    {
        LIDBG_ERR("lidbg_new_cdev:error\n");
	ssleep(1);
    }
    mutex_init(&lock);
    mutex_init(&fifo_lock);
    //INIT_COMPLETION(cmd_ready);
    init_waitqueue_head(&wait_queue);

    if (kfifo_alloc(&cmd_fifo, FIFO_SIZE, GFP_KERNEL) < 0)
    {
        LIDBG_WARN("kfifo_alloc fail\n");
        return -1;
    }

    LIDBG_WARN("lidbg_uevent_init,cmd_fifo:[%d KB],kfifo_avail:[%d KB] kfifo_len:[%d KB]\n", (u32)(FIFO_SIZE / 1024), kfifo_avail(&cmd_fifo) / 1024, kfifo_len(&cmd_fifo) / 1024);
    return 0;
}

static void __exit lidbg_uevent_exit(void)
{
    //misc_deregister(&lidbg_uevent_device);
}


//zone below [interface]
bool lidbg_uevent_focus(char *focus, void(*callback)(char *focus, char *uevent))
{
    return uevent_focus(focus, callback);
}
void lidbg_uevent_send(enum kobject_action action, char *envp_ext[])
{
    uevent_send(action, envp_ext);
}
void lidbg_uevent_shell(char *shell_cmd)
{

    if(0)
        uevent_shell(shell_cmd);
    else
    {
        u32 mstrlen = strlen(shell_cmd);
        if(mstrlen >= PER_SHELL_SIZE)
        {
            LIDBG_ERR("error:\n\n\n\n\n\n\n ignoreshell: size too big [%d>%d][%s]\n\n\n\n\n\n\n", mstrlen, PER_SHELL_SIZE, shell_cmd);
            return;
        }
        while(kfifo_is_full(&cmd_fifo) || kfifo_len(&cmd_fifo) >= ( FIFO_SIZE - mstrlen * 2))
        {
            LIDBG_WARN("kfifo_is_full:[%d],kfifo_len [%d byte]\n", kfifo_is_full(&cmd_fifo), kfifo_len(&cmd_fifo));
            msleep(100);
        }
        mutex_lock(&fifo_lock);
        kfifo_in(&cmd_fifo, (unsigned char *) &mstrlen, sizeof(u32));
        kfifo_in(&cmd_fifo, shell_cmd, mstrlen);
        mutex_unlock(&fifo_lock);
        if(uevent_dbg)
            LIDBG_WARN("kfifo_len:%d byte,mstrlen:%d/[%s]\n", kfifo_len(&cmd_fifo),  mstrlen, shell_cmd);
        //complete(&cmd_ready);
        wake_up_interruptible(&wait_queue);
    }
}

void lidbg_uevent_main(int argc, char **argv)
{
    if(!strcmp(argv[0], "dbg"))
    {
        uevent_dbg = !uevent_dbg;
    }
    else if(!strcmp(argv[0], "uevent"))
    {
        lidbg_uevent_shell(argv[1]);
    }
    else if(!strcmp(argv[0], "size"))
    {
        LIDBG_WARN("[%s]:cmd_fifo len: %d byte/%d KB  kfifo_avail:%d KB\n", __func__, kfifo_len(&cmd_fifo), kfifo_len(&cmd_fifo) / 1024, kfifo_avail(&cmd_fifo) / 1024);
    }
    LIDBG_WARN("lidbg_uevent_main:%d,%s\n", uevent_dbg, argv[0]);
}

EXPORT_SYMBOL(lidbg_uevent_focus);
EXPORT_SYMBOL(lidbg_uevent_send);
EXPORT_SYMBOL(lidbg_uevent_shell);
EXPORT_SYMBOL(lidbg_uevent_main);
EXPORT_SYMBOL(lidbg_new_cdev);

module_init(lidbg_uevent_init);
module_exit(lidbg_uevent_exit);

MODULE_DESCRIPTION("futengfei 2014.3.8");
MODULE_LICENSE("GPL");

