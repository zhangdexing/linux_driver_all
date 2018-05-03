#include "lidbg.h"
#include "lidbg_hal_comm.h"
#define TAG "FLY: lidbg_hal_comm "
#define hal_comm_debug(tag, format, args...) printk(KERN_DEBUG"%s: "format, tag, ##args)
#define hal_comm_debug_err(tag, format, args...) printk(KERN_CRIT"%s: "format, tag, ##args)

struct list_head *klisthead ;
typedef enum
    {
	MCU,
	HAL,
	MOD,
	ALL,
	INVALID
    }TYPE;
typedef struct fifo_list
{
    char type;
    char ID;
    struct kfifo data_fifo;
    struct list_head list;
    wait_queue_head_t fifo_wait_queue;
    spinlock_t fifo_lock;
    spinlock_t rw_lock;
} threads_list;

#define HAL_FIFO_SIZE (1024*16)

static DEFINE_MUTEX(list_mutex);

static struct class *new_cdev_class = NULL;
int init_thread_kfifo(struct kfifo *pkfifo, int size);

static loff_t node_default_lseek(struct file *file, loff_t offset, int origin)
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
	if(!nodename)
	{
		lidbgerr( "no nodename !!!\n");
		return -1;
	}
	
	new_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
	if (!new_cdev)
	{
		lidbgerr( "new cdev kzalloc failed !\n");
		return -1;
	}
	
	dev_number = MKDEV(major_number_ts, 0);
	if(major_number_ts)
		result = register_chrdev_region(dev_number, 1, nodename);
	else
		result = alloc_chrdev_region(&dev_number, 0, 1, nodename);
	
	if (result)
	{
		lidbgerr( "alloc_chrdev_region result:%d !\n", result);
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
		lidbgerr( "cdev_add result:%d !\n", err);
		return -1;
	}
	
	if(!new_cdev_class)
	{
		new_cdev_class = class_create(cdev_fops->owner, "lcdev_class");
		if(IS_ERR(new_cdev_class))
		{
			lidbgerr( "class_create failed !\n");
			cdev_del(new_cdev);
			kfree(new_cdev);
			new_cdev_class = NULL;
			return -1;
		}
	}
	
	new_device = device_create(new_cdev_class, NULL, dev_number, NULL, "%s", nodename);
	if (!new_device)
	{
		lidbgerr( "device_create fail !\n");
		cdev_del(new_cdev);
		kfree(new_cdev);
		return -1;
	}
	
	return 0;
}
EXPORT_SYMBOL(new_cdev_node);


int init_thread_kfifo(struct kfifo *pkfifo, int size);


void list_init(struct list_head *hlist)
{
	lidbgerr( " line : %d    func : %s \n", __LINE__, __func__);
	INIT_LIST_HEAD(hlist);
}

threads_list *add_node_list(struct list_head *hlist, char ID, char type)
{
	threads_list *listnode;
	listnode = (threads_list *)kmalloc(sizeof(threads_list), GFP_KERNEL);
	if(listnode == NULL)
	{
		lidbg( "Fail malloc list node !\n");
		return NULL;
	}
	listnode->ID = ID;
	listnode->type = type;
	if(init_thread_kfifo(&listnode->data_fifo, HAL_FIFO_SIZE) < 0)
		lidbg( "type : 0x%x ID : 0x%x init kfifo failed !\n", listnode->type, listnode->ID);
	list_add_tail(&listnode->list, hlist);
	return listnode;
}

threads_list *search_each_node(struct list_head *hlist, char ID, char type)
{
	struct list_head *pos;
	threads_list *p;
	if (list_empty(hlist))
	{
		lidbg( "enter search_each_node list is NULL !\n");
		return NULL;
	}
	
	list_for_each(pos, hlist)
	{
		p = list_entry(pos, threads_list, list);
		if(p->ID == ID && p->type == type)
		{
			return p;
		}
	}
	return NULL;
}


int init_thread_kfifo(struct kfifo *pkfifo, int size)
{
	int ret;
	ret = kfifo_alloc(pkfifo, size, GFP_KERNEL);
	if (ret < 0)
	{
		lidbg( "kfifo_alloc failed !\n");
		return ret;
	}
	return ret;
}

int put_buf_fifo(struct kfifo *pkfifo , char *buf, int bufLen, spinlock_t *fifo_lock)
{
	int len, ret;
	int data_len;
	if (bufLen+sizeof(bufLen) > HAL_FIFO_SIZE)
	{
		return 0;
	}
	len = kfifo_avail(pkfifo);
	while(len < bufLen + sizeof(bufLen))
	{
		lidbg( "fifo did't have enough space\n");
		ret = kfifo_out_spinlocked(pkfifo, &data_len, sizeof(data_len), fifo_lock);
		lidbg( "kfifo length is : %d \n", data_len);
		if(ret < 0)
		{
			lidbg( "fail to output data \n");
		}else
		{
			char out_buf[data_len];
			ret = kfifo_out_spinlocked(pkfifo, out_buf, data_len, fifo_lock);
			if(ret < 0)
			{
				lidbg( "fail to output data \n");
			}else
			{
				lidbg( "give up one data\n ");
			}
		}
		len = kfifo_avail(pkfifo);
		lidbg( "kfifo vaild len : %d \n", len);
	}
	kfifo_in_spinlocked(pkfifo, &bufLen, sizeof(bufLen), fifo_lock);
	kfifo_in_spinlocked(pkfifo, buf, bufLen, fifo_lock);
	return bufLen;
}

int get_buf_fifo(struct kfifo *pkfifo, char *buf, spinlock_t *fifo_lock)
{
	int ret;
	int len;
	if (kfifo_is_empty(pkfifo))
	{
		return 0;
	}
	if(kfifo_len(pkfifo) <= sizeof(len))
	{
		lidbg( "get_buf_fifo err : %d\n", kfifo_len(pkfifo));
		return 0;
	}
	ret = kfifo_out_spinlocked(pkfifo, &len, sizeof(len), fifo_lock);
	{
		ret = kfifo_out_spinlocked(pkfifo, buf, len, fifo_lock);
		//lidbg( "read fifo length is %d ret : %d\n", len, ret);
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

static int listWrite(threads_list *p, char *buf, int len)
{
	int ret = 0;
	spin_lock(&p->rw_lock);
	ret = put_buf_fifo(&p->data_fifo, buf, len, &p->fifo_lock);
	spin_unlock(&p->rw_lock);
	return ret;
}

static int listRead(threads_list *p, char *buf)
{
	int len = 0;
	spin_lock(&p->rw_lock);    
	len = get_buf_fifo(&p->data_fifo, buf, &p->fifo_lock);
	spin_unlock(&p->rw_lock);    
	return len;
}

static int send_all_fifo(struct list_head *hlist, const void  *buf, int size)
{
	int ret = 0;
	struct list_head *pos;
	threads_list *p  = NULL;
	if (list_empty(hlist))
	{
		lidbg( "enter send_all_fifo list is NULL !\n");
		return 0;
	}
	list_for_each(pos, hlist)
	{
		p = list_entry(pos, threads_list, list);
		ret = listWrite(p, buf, size);
		lidbg( "send_all_fifo ===== type : 0x%x ID : 0x%x size %d\n", p->type, p->ID, size);
		wake_up_interruptible(&p->fifo_wait_queue);
	}
	return ret;
}
static long hal_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	threads_list *tmp_list = NULL;
	int ret;
	char type;
	char ID;
	char direction;
	int data_size;
	char argv[1024];
	data_size = _IOCOM_SIZE(cmd);				//读写长度
	direction = _IOCOM_DIR(cmd);				//读写标记(1为写，２为读)

	if (direction == _IOCOM_WRITE)				//如果是写数据，则先把用户空间数据拷贝到内核空间
	{
		if (copy_from_user(argv, (char *)arg, data_size) > 0)
		{
			lidbg( "copy_from_user ERR type : 0x%x ID : 0x%x size : %d direction : %d\n", type , ID, data_size, direction);
			return 0;
		}
	}else if (direction == _IOCOM_READ)				//如果是读只需要把前面四个字节拷贝到内核空间
	{
		if (copy_from_user(argv, (char *)arg, 4) > 0)
		{
			lidbg( "copy_from_user ERR type : 0x%x ID : 0x%x size : %d direction : %d\n", type , ID, data_size, direction);
			return 0;
		}
	}else 
	{
		return 0;
	}

	type = (char)argv[1];
	if(type == 0x55 || type == 0x53){
		ID = (char)argv[1];
	}else if(type == 0x54){
		ID = (char)argv[3];
	}else if(type != 0xff){
		lidbg( "No Support type : 0x%x ID : 0x%x size : %d direction : %d\n", type , ID, data_size, direction);
		return -1;
	}
		
	mutex_lock(&list_mutex);
	if(type==0xff && direction == _IOCOM_WRITE)
	{
		ret = send_all_fifo(klisthead, argv, data_size);
		lidbg( "type : 0x%x ID : 0x%x size : %d ===== send_all_fifo ok\n", type , ID, data_size);
		mutex_unlock(&list_mutex);
    		return ret;
        }else{
		tmp_list = search_each_node(klisthead, ID, type);
		if (NULL == tmp_list)
		{
			tmp_list = add_node_list(klisthead, ID, type);
			if(tmp_list == NULL){
				lidbg(  "init list failed type : 0x%x ID : 0x%x size : %d direction : %d\n", type , ID, data_size, direction);
				return -1;
			}
			init_waitqueue_head(&tmp_list->fifo_wait_queue);
			spin_lock_init(&tmp_list->fifo_lock);
			spin_lock_init(&tmp_list->rw_lock);
			lidbg(  "init list kfifo ok type : 0x%x ID : 0x%x size : %d direction : %d\n", type , ID, data_size, direction);
		}
	}
	mutex_unlock(&list_mutex);

	if (direction == _IOCOM_WRITE)//write
	{
		ret = listWrite(tmp_list, argv, data_size);
		wake_up_interruptible(&tmp_list->fifo_wait_queue);
		//lidbg(  "type : 0x%x ID : 0x%x size : %d ret : %d direction : %d ===== listWrite wake_up_interruptible \n", type , ID, data_size, ret, direction);	//写入fifo大小为size_data+sizeof(data_size)
	}else if (direction == _IOCOM_READ)//read
	{
		//lidbg(  "type : 0x%x ID : 0x%x size : %d direction : %d ===== read wait_event_interruptible\n", type , ID, data_size, direction);
		if(wait_event_interruptible(tmp_list->fifo_wait_queue, !kfifo_is_empty(&tmp_list->data_fifo))){
			//lidbg(  " >>>>>>>>>>>>>>>> read wait_event_interruptible exit >>>>>>>>>>>>>>>> \n");
			return -ERESTARTSYS;
		}
		ret = listRead(tmp_list, argv);
		if (copy_to_user((char *)arg, argv, ret) > 0)
		{
			lidbg( "copy_from_user ERR type : 0x%x ID : 0x%x size : %d ret : %d direction : %d\n", type , ID, data_size, ret, direction);
			return 0;
		}
	    	//lidbg(  "type : 0x%x ID : 0x%x size : %d ret : %d direction : %d ===== listRead \n", ID , type, data_size, ret, direction);				//读出fifo大小为size_data+sizeof(data_size)
	}
    return ret;
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
	ret = new_cdev_node(&hal_comm_fops, "lidbg_hal_comm0");
	if(ret < 0)
	{
		lidbgerr( "Fail to creat cdev \n");
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


