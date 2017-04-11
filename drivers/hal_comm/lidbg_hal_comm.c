#include "lidbg.h"
#include "lidbg_hal_comm.h"

LIDBG_DEFINE;

struct list_head *klisthead ;

typedef struct fifo_list
{
    int mark_type;
    struct kfifo data_fifo;
    struct list_head list;
    wait_queue_head_t fifo_wait_queue;
    spinlock_t fifo_lock;
} threads_list;

#define HAL_FIFO_SIZE (1024*16)


static DEFINE_MUTEX(list_mutex);

int init_thread_kfifo(struct kfifo *pkfifo, int size);

loff_t node_default_lseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}

void list_init(struct list_head *hlist)
{
    lidbg( " line : %d    func : %s \n", __LINE__, __func__);
    INIT_LIST_HEAD(hlist);
}

threads_list *add_node_list(struct list_head *hlist, int thread_type)
{
    threads_list *listnode;
    listnode = (threads_list *)kmalloc(sizeof(threads_list), GFP_KERNEL);
    if(listnode == NULL)
        lidbg("Fail malloc\n");
    mutex_lock(&list_mutex);
    listnode->mark_type = thread_type;
    init_thread_kfifo(&listnode->data_fifo, HAL_FIFO_SIZE);
    list_add_tail(&listnode->list, hlist);
    mutex_unlock(&list_mutex);
    return listnode;
}

threads_list *search_each_node(struct list_head *hlist, int thread_type)
{
    struct list_head *pos;
    threads_list *p , *plist = NULL;
    mutex_lock(&list_mutex);
    list_for_each(pos, hlist)
    {
        p = list_entry(pos, threads_list, list);
        if(p->mark_type == thread_type)
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
        lidbg("kfifo_alloc failed !\n");
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
        lidbg( "fifo did't have enough space\n");
        ret = kfifo_out_spinlocked(pkfifo, &data_len, 2, fifo_lock);
        pr_debug( "kfifo length is : %d \n", data_len);
        if(ret < 0)
            lidbg( "fail to output data \n");
        {
            char out_buf[data_len];
            ret = kfifo_out_spinlocked(pkfifo, out_buf, data_len, fifo_lock);
            lidbg( "give up one data\n ");
            if(ret < 0)
                lidbg( "fail to output data \n");
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

static long hal_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    threads_list *tmp_list = NULL;
    int tmp_type, ret;
    short data_size;
    tmp_type = _IOCOM_TYPE(cmd);
    data_size = _IOCOM_SIZE(cmd);
    pr_debug("kfifo = type : %d ,size : %d ,direction : %d \n", tmp_type , data_size, _IOCOM_DIR(cmd));

    if(list_empty(klisthead) || ( (tmp_list = search_each_node(klisthead, tmp_type) ) == NULL) )
    {
        tmp_list = add_node_list(klisthead, tmp_type);
        init_waitqueue_head(&tmp_list->fifo_wait_queue);
        spin_lock_init(&tmp_list->fifo_lock);
    }

    if(_IOCOM_DIR(cmd) == 1)//write
    {
    	 bool is_kfifo_empty = kfifo_is_empty(&tmp_list->data_fifo);
	 put_buf_fifo(&tmp_list->data_fifo, &data_size, 2, &tmp_list->fifo_lock);
        put_buf_fifo(&tmp_list->data_fifo, (void *)arg, data_size, &tmp_list->fifo_lock);
	 if(is_kfifo_empty)
	 	wake_up_interruptible(&tmp_list->fifo_wait_queue);
    }
    else//read
    {
        if(kfifo_is_empty(&tmp_list->data_fifo))
            ret = wait_event_interruptible(tmp_list->fifo_wait_queue, !kfifo_is_empty(&tmp_list->data_fifo));
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
        lidbg( "Fail to creat cdev \n");
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
    LIDBG_GET;
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


