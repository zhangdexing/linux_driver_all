#include "lidbg_hal_comm.h"


struct list_head *klisthead ;


static DEFINE_MUTEX(list_mutex); 

static struct class *new_cdev_class = NULL;
int init_thread_kfifo(struct kfifo *pkfifo, int size);

loff_t node_default_lseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}

int new_cdev_node(struct file_operations *cdev_fops, char *nodename)
{
    struct cdev *new_cdev = NULL;
    struct device *new_device = NULL;
    dev_t dev_number = 0;
    int major_number_ts = 0;
    int err, result;

    if(!cdev_fops->owner || !nodename)
    {
        printk(KERN_CRIT "cdev_fops->owner||nodename \n");
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
    new_cdev->owner = cdev_fops->owner;
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

void list_init(struct list_head *hlist)
{
	printk(KERN_CRIT " line : %d    func : %s \n",__LINE__, __func__);
	INIT_LIST_HEAD(hlist);
      /* 
     * static inline void INIT_LIST_HEAD(struct list_head *list)
     * {
     * list->next = list;
     * list->prev = list;
     * }
     */
}

threads_list *add_node_list(struct list_head *hlist, int thread_type)
{
	threads_list *listnode;
	listnode = (threads_list *)kmalloc(sizeof(threads_list), GFP_KERNEL);
	if(listnode == NULL)
		printk("Fail malloc\n");
	mutex_lock(&list_mutex); 
	listnode->mark_type = thread_type;
	init_thread_kfifo(&listnode->data_fifo,HAL_FIFO_SIZE);
	list_add_tail(&listnode->list,hlist); 
	mutex_unlock(&list_mutex);
	
	return listnode;
}

threads_list *search_each_node(struct list_head *hlist, int thread_type)
{
	struct list_head *pos;
	threads_list *p , *plist = NULL;
	mutex_lock(&list_mutex);
	list_for_each(pos, hlist) {
		p = list_entry(pos, threads_list, list);
		if(p->mark_type == thread_type){
			plist = p;
		}
	}
	mutex_unlock(&list_mutex);
	return plist;
}
/*************************************/

/***********************kfifo func **************************/
int init_thread_kfifo(struct kfifo *pkfifo, int size)
{
	int ret;
	ret = kfifo_alloc(pkfifo, size, GFP_KERNEL);
	
	if (ret < 0) {
        	printk("kfifo_alloc failed !\n");
        	return -EFAULT;
    	}

	return ret;
}

void put_buf_fifo(struct kfifo *pkfifo , void *buf, int size, struct mutex *lock)
{
	int len, ret ,data_len;
	len = kfifo_avail(pkfifo);
	while(len < size){
		printk(KERN_CRIT "fifo did't have enough space\n");
//		mutex_lock(lock);
		ret = kfifo_out(pkfifo, &data_len,2);
//		mutex_unlock(lock);
		printk(KERN_CRIT "kfifo length is : %d \n",data_len);
		if(ret < 0)
			printk(KERN_CRIT "fail to output data \n");
		{
			char out_buf[data_len];
//			mutex_lock(lock);
			ret = kfifo_out(pkfifo,out_buf,data_len);
//			mutex_unlock(lock);
			printk(KERN_CRIT "give up one data : %s ", out_buf);
			if(ret < 0)
				printk(KERN_CRIT "fail to output data \n");
		}
		len = kfifo_avail(pkfifo);
		printk(KERN_CRIT "kfifo vaild len : %d \n",len);
	}
	mutex_lock(lock);
	kfifo_in(pkfifo, buf, size); 
	mutex_unlock(lock);
}

short get_buf_fifo(struct kfifo *pkfifo, void *arg, struct mutex *lock)
{
	int ret;
	short len;
	char buff[100];
//	mutex_lock(lock);
	ret = kfifo_out(pkfifo, &len, 2);
//	mutex_unlock(lock);
	if(ret < 0)
		printk(KERN_CRIT "fail to output data \n");
//	mutex_lock(lock);
	ret = kfifo_out(pkfifo, buff, len);
//	mutex_unlock(lock);
	if(ret < 0)
		printk(KERN_CRIT "fail to output data \n");
	
	ret = copy_to_user(arg, buff, len);
	printk(KERN_CRIT "read fifo length is %d ,context is %s", len,buff);
	return len;
}

/*************************************************************/

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
	printk(KERN_CRIT "kfifo = type : %d ,size : %d ,direction : %d \n",tmp_type , data_size, _IOCOM_DIR(cmd));
	if(_IOCOM_DIR(cmd) == 1){
		if(!list_empty(klisthead))
		{
			tmp_list= search_each_node(klisthead, tmp_type);
			if(tmp_list == NULL)
			{
				tmp_list = add_node_list(klisthead, tmp_type);
				sema_init(&tmp_list->fifo_sem,0);
				mutex_init(&tmp_list->fifo_mutex);
				put_buf_fifo(&tmp_list->data_fifo, &data_size,2,&tmp_list->fifo_mutex);
				put_buf_fifo(&tmp_list->data_fifo, (void *)arg,data_size,&tmp_list->fifo_mutex);
				return data_size;
			}else{
				if(kfifo_is_empty(&tmp_list->data_fifo))
				{
					put_buf_fifo(&tmp_list->data_fifo, &data_size,2,&tmp_list->fifo_mutex);
					put_buf_fifo(&tmp_list->data_fifo, (void *)arg,data_size,&tmp_list->fifo_mutex);
					up(&tmp_list->fifo_sem);
				}else{
					put_buf_fifo(&tmp_list->data_fifo, &data_size,2,&tmp_list->fifo_mutex);
					put_buf_fifo(&tmp_list->data_fifo, (void *)arg,data_size,&tmp_list->fifo_mutex);	
				}
				
				return data_size;
			}
		}else{
			tmp_list = add_node_list(klisthead, tmp_type);
			sema_init(&tmp_list->fifo_sem,0);
			mutex_init(&tmp_list->fifo_mutex);
			put_buf_fifo(&tmp_list->data_fifo, &data_size,2,&tmp_list->fifo_mutex);
			put_buf_fifo(&tmp_list->data_fifo, (void *)arg,data_size,&tmp_list->fifo_mutex);
			return data_size;
		}	
	}else{
		if(list_empty(klisthead))
		{
			tmp_list = add_node_list(klisthead, tmp_type);
			sema_init(&tmp_list->fifo_sem,0);
			mutex_init(&tmp_list->fifo_mutex);
			ret = down_interruptible(&tmp_list->fifo_sem);
			data_size = get_buf_fifo(&tmp_list->data_fifo,(void*)arg,&tmp_list->fifo_mutex);
			return data_size;
		}else{
			tmp_list= search_each_node(klisthead, tmp_type);
		
			if(tmp_list == NULL)
			{
				tmp_list = add_node_list(klisthead, tmp_type);
				sema_init(&tmp_list->fifo_sem,0);
				mutex_init(&tmp_list->fifo_mutex);
				ret = down_interruptible(&tmp_list->fifo_sem);
				data_size = get_buf_fifo(&tmp_list->data_fifo,(void*)arg,&tmp_list->fifo_mutex);
				return data_size;
			}else{
				if(kfifo_is_empty(&tmp_list->data_fifo))
				{
					ret = down_interruptible(&tmp_list->fifo_sem);
					data_size = get_buf_fifo(&tmp_list->data_fifo,(void*)arg,&tmp_list->fifo_mutex);
					return data_size;
				}else{
					data_size = get_buf_fifo(&tmp_list->data_fifo,(void*)arg,&tmp_list->fifo_mutex);
				}
			}
		}
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
	ret = new_cdev_node(&hal_comm_fops, "lidbg_hal_comm0");
	if(ret < 0){
		printk(KERN_CRIT "Fail to creat cdev \n");
		return -1;
	}
	list_init(klisthead);	
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


