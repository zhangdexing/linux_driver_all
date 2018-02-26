#include "lidbg.h"
#include "lidbg_hal_comm.h"
#define TAG "HDJ: lidbg_hal_comm "
#define hal_comm_debug(tag, format, args...) printk(KERN_DEBUG"%s: "format, tag, ##args)

LIDBG_DEFINE;

struct list_head *klisthead ;
typedef enum
    {
	MCU,
	HAL,
	MOD,
	ALL,
    }TYPE;
typedef struct fifo_list
{
    TYPE type;
    char ID;
    struct kfifo data_fifo;
    struct list_head list;
    wait_queue_head_t fifo_wait_queue;
    spinlock_t fifo_lock;

} threads_list;

#define HAL_FIFO_SIZE (1024*16)

static DEFINE_MUTEX(list_mutex);

static struct class *new_cdev_class = NULL;
int init_thread_kfifo(struct kfifo *pkfifo, int size);

static loff_t node_default_lseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}
#if 0
int new_cdev_node(struct file_operations *cdev_fops, char *nodename)
{
    struct cdev *new_cdev = NULL;
    struct device *new_device = NULL;
    dev_t dev_number = 0;
    int major_number_ts = 0;
    int err, result;
    if(!nodename)
    {
        printk(KERN_CRIT "no nodename !!!\n");
        return -1;
    }

    new_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
    if (!new_cdev)
    {
        printk(KERN_CRIT "kzalloc \n");
        return -1;
    }

    dev_number = MKDEV(major_number_ts, 0);
    if(major_number_ts)
        result = register_chrdev_region(dev_number, 1, nodename);
    else
        result = alloc_chrdev_region(&dev_number, 0, 1, nodename);

    if (result)
    {
       printk(KERN_CRIT "alloc_chrdev_region result:%d \n", result);
        return -1;
    }
    major_number_ts = MAJOR(dev_number);

    if(!cdev_fops->llseek)
        cdev_fops->llseek = node_default_lseek;

    cdev_init(new_cdev, cdev_fops);
    new_cdev->owner = THIS_MODULE;
    new_cdev->ops = cdev_fops;
    err = cdev_add(new_cdev, dev_number, 1);
    if (err)
    {
       printk(KERN_CRIT "cdev_add result:%d \n", err);
        return -1;
    }

    if(!new_cdev_class)
    {
        new_cdev_class = class_create(cdev_fops->owner, "lcdev_class");
        if(IS_ERR(new_cdev_class))
        {
            printk(KERN_CRIT "class_create\n");
            cdev_del(new_cdev);
            kfree(new_cdev);
            new_cdev_class = NULL;
            return -1;
        }
    }

    new_device = device_create(new_cdev_class, NULL, dev_number, NULL, "%s", nodename);
    if (!new_device)
    {
        printk(KERN_CRIT "device_create fail !\n");
        cdev_del(new_cdev);
        kfree(new_cdev);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(new_cdev_node);


int init_thread_kfifo(struct kfifo *pkfifo, int size);
#endif

void list_init(struct list_head *hlist)
{
    printk(KERN_CRIT " line : %d    func : %s \n", __LINE__, __func__);
    INIT_LIST_HEAD(hlist);
}

threads_list *add_node_list(struct list_head *hlist, char ID, TYPE type)
{
    threads_list *listnode;
    listnode = (threads_list *)kmalloc(sizeof(threads_list), GFP_KERNEL);
    if(listnode == NULL)
        printk(KERN_CRIT "Fail malloc\n");
    mutex_lock(&list_mutex);
    listnode->ID = ID;
    listnode->type = type;
    init_thread_kfifo(&listnode->data_fifo, HAL_FIFO_SIZE);
    list_add_tail(&listnode->list, hlist);
    mutex_unlock(&list_mutex);
    return listnode;
}

threads_list *search_each_node(struct list_head *hlist, char ID, TYPE type)
{
    struct list_head *pos;
    threads_list *p , *plist = NULL;
    mutex_lock(&list_mutex);
    list_for_each(pos, hlist)
    {
        p = list_entry(pos, threads_list, list);
        if(p->ID == ID && p->type == type)
        {
            plist = p;
        }
    }
    mutex_unlock(&list_mutex);
    return plist;
}


int init_thread_kfifo(struct kfifo *pkfifo, int size)
{
    int ret;
    ret = kfifo_alloc(pkfifo, size, GFP_KERNEL);
    if (ret < 0)
    {
        printk(KERN_CRIT "kfifo_alloc failed !\n");
        return -EFAULT;
    }
    return ret;
}

void put_buf_fifo(struct kfifo *pkfifo , void *buf, int size, spinlock_t *fifo_lock)
{
    int len, ret;
    short data_len;
    len = kfifo_avail(pkfifo);
    while(len < size)
    {
        printk(KERN_CRIT "fifo did't have enough space\n");
        ret = kfifo_out_spinlocked(pkfifo, &data_len, 2, fifo_lock);
        pr_debug( "kfifo length is : %d \n", data_len);
        if(ret < 0)
            printk(KERN_CRIT "fail to output data \n");
        {
            char out_buf[data_len];
            ret = kfifo_out_spinlocked(pkfifo, out_buf, data_len, fifo_lock);
            printk(KERN_CRIT "give up one data\n ");
            if(ret < 0)
                printk(KERN_CRIT "fail to output data \n");
        }
        len = kfifo_avail(pkfifo);
        pr_debug( "kfifo vaild len : %d \n", len);
    }
    kfifo_in_spinlocked(pkfifo, buf, size, fifo_lock);
}

short get_buf_fifo(struct kfifo *pkfifo, void *arg, spinlock_t *fifo_lock)
{
    int ret;
    short len;
    if(kfifo_len(pkfifo) <= 2)
    {
        pr_debug("get_buf_fifo err : %d\n", kfifo_len(pkfifo));
        return 0;
    }
    ret = kfifo_out_spinlocked(pkfifo, &len, 2, fifo_lock);
    {
        char buff[len];
        ret = kfifo_out_spinlocked(pkfifo, buff, len, fifo_lock);
        ret = copy_to_user(arg, buff, len);
        pr_debug("read fifo length is %d \n", len);
    }
    return len;
}


void hal_comm_Suspend(void)
{
    return;
}

void hal_comm_Resume(void)
{
    return;
}



int hal_comm_open(struct inode *inode, struct file *filp)
{

    return 0;
}

int hal_comm_close(struct inode *inode, struct file *filp)
{

    return 0;
}

static ssize_t hal_comm_write(struct file *filp, const char __user *buf,
                              size_t size, loff_t *ppos)
{

    return 0;
}
static int send_all_fifo(struct list_head *hlist, const void  *buf,short size)
{

    bool is_kfifo_empty;
    struct list_head *pos;
	threads_list *p  = NULL;
	mutex_lock(&list_mutex);
	list_for_each(pos, hlist)
	{
		if(1)
		{
			p = list_entry(pos, threads_list, list);
			is_kfifo_empty = kfifo_is_empty(&p->data_fifo);
			put_buf_fifo(&p->data_fifo, &size, 2, &p->fifo_lock);
			put_buf_fifo(&p->data_fifo, (void *)buf, size, &p->fifo_lock);
			//printk("send_all_fifo fifo ID is =%x\n",p->ID);
			if(is_kfifo_empty)
				wake_up_interruptible(&p->fifo_wait_queue);
		}
	}
   mutex_unlock(&list_mutex);
    return 0;
}
static long hal_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    threads_list *tmp_list = NULL;
    char ID;
	int ret;
	TYPE type;
    short data_size;
    char *argv=(char *)arg;
    data_size = _IOCOM_SIZE(cmd);
	ID=(char)argv[3];
        if((char)argv[1]==0x55)
	{
		type = MOD;
		ID   = 0x55;
	}
	else if((char)argv[1]==0x53)
	{
		type=MCU;
		ID=0x53;
	}
	else if((char)argv[1]==0x54)
	{
		type=HAL;
	}
	else if((char)argv[1]==0xff)
	{
		type=ALL;
	}
	else
	{
		type=HAL;
	}
    hal_comm_debug(TAG,"enter hal_comm_ioctl ID : %x ,type : %d ,size : %d ,direction : %d \n", ID , type, data_size, _IOCOM_DIR(cmd));
	if(type==ALL && _IOCOM_DIR(cmd) == 1)
	{
		//printk("send_all_fifo\n");
		send_all_fifo(klisthead,(void *)arg,data_size);
		return data_size;
	}
    else if(list_empty(klisthead) || ( (tmp_list = search_each_node(klisthead, ID, type) ) == NULL) )
    {
        tmp_list = add_node_list(klisthead, ID, type);
        init_waitqueue_head(&tmp_list->fifo_wait_queue);
        spin_lock_init(&tmp_list->fifo_lock);
	hal_comm_debug(TAG, "init kfifo ID : %x, type : %d, size : %d, direction : %d\n", ID , type, data_size, _IOCOM_DIR(cmd));
	//pr_debug("lcrinit kfifo = ID : %x ,type : %d ,size : %d ,direction : %d \n", ID , type, data_size, _IOCOM_DIR(cmd));


    }

    if(_IOCOM_DIR(cmd) == 1)//write
    {
    	 bool is_kfifo_empty = kfifo_is_empty(&tmp_list->data_fifo);
	 put_buf_fifo(&tmp_list->data_fifo, &data_size, 2, &tmp_list->fifo_lock);
        put_buf_fifo(&tmp_list->data_fifo, (void *)arg, data_size, &tmp_list->fifo_lock);
	 if(is_kfifo_empty){
	 	wake_up_interruptible(&tmp_list->fifo_wait_queue);
		hal_comm_debug(TAG, "wake_up_interruptible ID : %x, type : %d, size : %d, direction : %d\n", ID , type, data_size, _IOCOM_DIR(cmd));
	}
    }
    else//read
    {
        if(kfifo_is_empty(&tmp_list->data_fifo))
            ret = wait_event_interruptible(tmp_list->fifo_wait_queue, !kfifo_is_empty(&tmp_list->data_fifo));
	hal_comm_debug(TAG, "wait_event_interruptible exit ID : %x, type : %d, size : %d, direction : %d\n", ID , type, data_size, _IOCOM_DIR(cmd));
        data_size = get_buf_fifo(&tmp_list->data_fifo, (void *)arg, &tmp_list->fifo_lock);
    }
    return data_size;
}


static struct file_operations hal_comm_fops =
{
    .owner = THIS_MODULE,
    .open = hal_comm_open,
    .write = hal_comm_write,
    .unlocked_ioctl = hal_comm_ioctl,
    .release = hal_comm_close,
};


static int  hal_comm_probe(struct platform_device *pdev)
{
    int ret;
    klisthead = (struct list_head *)kmalloc(sizeof(struct list_head), GFP_KERNEL);
    list_init(klisthead);

    ret = lidbg_new_cdev(&hal_comm_fops, "lidbg_hal_comm0");
    if(ret < 0)
    {
        printk(KERN_CRIT "Fail to creat cdev \n");
        return -1;
    }
    return 0;
}


static int  hal_comm_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_device lidbg_hal_comm =
{
    .name           = "lidbg_hal_comm",
    .id                 = -1,
};

static struct platform_driver hal_comm_driver =
{
    .probe		= hal_comm_probe,
    .remove     = hal_comm_remove,
    .driver         = {
        .name = "lidbg_hal_comm",
        .owner = THIS_MODULE,
    },
};



static int  hal_comm_init(void)
{
    LIDBG_DEFINE;
    platform_device_register(&lidbg_hal_comm);
    platform_driver_register(&hal_comm_driver);
    return 0;
}

static void  hal_comm_exit(void)
{
    platform_driver_unregister(&hal_comm_driver);
}


module_init(hal_comm_init);
module_exit(hal_comm_exit);


MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("lidbg hal comm driver");


