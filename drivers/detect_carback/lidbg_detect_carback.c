#include "lidbg.h"
LIDBG_DEFINE;

#define TAG	 "detect_carback:"

static wait_queue_head_t wait_queue;
struct work_struct work_carback_status;
static int carback_state_old=-1;
static int carback_state_new=-1;

#define TRANSPOND_BUFFER_SIZE 255
static char transpond_buffer[TRANSPOND_BUFFER_SIZE];
static int can_read = 0;
static spinlock_t spinlock;
static wait_queue_head_t transpond_wait_queue;


static int lidbg_detect_carback_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	DUMP_FUN;

	switch (event)
	{
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
			lidbg(TAG"carback  disable gprio irq \n");
			disable_irq(GPIO_TO_INT(CARBACK_STATE_IO));
			break;

		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
			lidbg(TAG"carback  enable gprio irq \n");
			//carback_state_old=-1;//force wake up read	
			carback_state_new = SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP);
			enable_irq(GPIO_TO_INT(CARBACK_STATE_IO));
			wake_up_interruptible(&wait_queue);
			break;
		default:
			break;
	}

	return 0;
}

static struct notifier_block lidbg_detect_carback_notifier =
{
	.notifier_call = lidbg_detect_carback_event,
};


irqreturn_t carback_state_isr(int irq, void *dev_id)
{
	lidbg(TAG"carback  state irq is coming \n");
	if(!work_pending(&work_carback_status))
		schedule_work(&work_carback_status);
	return IRQ_HANDLED;
}


static void work_carback_status_handle(struct work_struct *work)
{
	int val = -1;

	val = SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP);

	lidbg(TAG">>>>> work_carback_status_handle =======>>>[%s]\n",(val == FLY_CARBACK_ENTRY)?"FLY_CARBACK_ENTRY":"FLY_CARBACK_EXIT");

	carback_state_new = val;
	wake_up_interruptible(&wait_queue);
}

/*
static void detect_carback_suspend(void)
{

	return;
}

static void detect_carback_resume(void)
{
	return;
}
*/

int detect_carback_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t  detect_carback_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	lidbg(TAG"detect_carback_read enter\n");

	if(carback_state_new == carback_state_old)
	{
		if(wait_event_interruptible(wait_queue, carback_state_new != carback_state_old))
		{
			lidbg(TAG"detect_carback_read error\n");
			return -ERESTARTSYS;
		}
	}

	if (copy_to_user(buffer, &carback_state_new,  4))
	{
		lidbg(TAG"copy_to_user ERR\n");
		return -EFAULT;
	}
	
	carback_state_old = carback_state_new;
	lidbg(TAG"detect_carback_read exit,status = %d\n",carback_state_new);
	return size;
}


ssize_t detect_carback_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
#if 1
    char *cmd[32] = {NULL};
    int cmd_num  = 0;
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        PM_ERR("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';

    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;
    lidbg(TAG"rmtctrl_write :-------%s-------\n", cmd[0]);

	if(!strcmp(cmd[0],"0"))
	{
		lidbg(TAG">>>>> detect_carback_write 0=======>>>>>\n");
		carback_state_new = 0;
		wake_up_interruptible(&wait_queue);

	}
	else if(!strcmp(cmd[0],"1"))
	{
		lidbg(TAG">>>>> detect_carback_write 1=======>>>>>\n");
		carback_state_new = 1;
		wake_up_interruptible(&wait_queue);
	}
	else
	{
		lidbg(TAG">>>>> detect_carback_write other=======>>>>>\n");
		lidbg(TAG">>>>> detect_carback_write read gpio %d = %d=======>>>>>\n",CARBACK_STATE_IO,SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP));
	}
#endif
	return size;
}

int carback_transpond_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t  carback_transpond_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	char local_buf[TRANSPOND_BUFFER_SIZE];
	int len=size;

	if(len>TRANSPOND_BUFFER_SIZE || len<0)
		len = TRANSPOND_BUFFER_SIZE;
	
	if(!can_read)
	{
		if(wait_event_interruptible(transpond_wait_queue, can_read))
		{
			return -ERESTARTSYS;
		}
	}

	memset(local_buf,0,TRANSPOND_BUFFER_SIZE);
	spin_lock(&spinlock);
	strncpy(local_buf,transpond_buffer,len);
	can_read = 0;
	spin_unlock(&spinlock);


	if (copy_to_user(buffer, local_buf, len))
	{
		lidbg(TAG"copy_to_user ERR\n");
		return -EFAULT;
	}
	
	lidbg(TAG"carback_transpond_read exit\n");
	return len;
}


ssize_t carback_transpond_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	char local_buf[TRANSPOND_BUFFER_SIZE];
	int len=size;

	if(len>TRANSPOND_BUFFER_SIZE || len<0)
		len = TRANSPOND_BUFFER_SIZE;
	
    if(copy_from_user(local_buf, buf, len))
    {
        PM_ERR("copy_from_user ERR\n");
		return -EFAULT;
    }

	spin_lock(&spinlock);
	memset(transpond_buffer,0,TRANSPOND_BUFFER_SIZE);
	strncpy(transpond_buffer,local_buf,len);
	can_read = 1;
	spin_unlock(&spinlock);

	wake_up_interruptible(&transpond_wait_queue);

	lidbg(TAG"carback_transpond_write exit\n");
	return len;
}


static  struct file_operations detect_carback_fops =
{
	.owner = THIS_MODULE,
	.open = detect_carback_open,
	.read = detect_carback_read,
	.write = detect_carback_write,
};

static  struct file_operations carback_transpond_fops =
{
	.owner = THIS_MODULE,
	.open = carback_transpond_open,
	.read = carback_transpond_read,
	.write = carback_transpond_write,
};


static int lidbg_detect_carback_probe(struct platform_device *pdev)
{

	init_waitqueue_head(&wait_queue);

	register_lidbg_notifier(&lidbg_detect_carback_notifier);

	lidbg_new_cdev(&detect_carback_fops, "flyaudio_detect_carback");
	lidbg_shell_cmd("chmod 777 /dev/flyaudio_detect_carback");

	carback_state_new = SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP);
	SOC_IO_ISR_Add(CARBACK_STATE_IO, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, carback_state_isr, NULL);
	
	wake_up_interruptible(&wait_queue);
	//enable_irq(GPIO_TO_INT(CARBACK_STATE_IO));  //Must disable_irq before enable_irq

	INIT_WORK(&work_carback_status, work_carback_status_handle);


	init_waitqueue_head(&transpond_wait_queue);
	spin_lock_init(&spinlock);
	lidbg_new_cdev(&carback_transpond_fops, "flyaudio_carback_transpond");
	lidbg_shell_cmd("chmod 777 /dev/flyaudio_carback_transpond");

	return 0;
}

#ifdef CONFIG_PM
static int detect_carback_pm_suspend(struct device *dev)
{
	DUMP_FUN;

	return 0;
}

static int detect_carback_pm_resume(struct device *dev)
{
	DUMP_FUN;

	return 0;
}


static struct dev_pm_ops lidbg_detect_carback_ops =
{
	.suspend	= detect_carback_pm_suspend,
	.resume	= detect_carback_pm_resume,
};
#endif

static struct platform_device lidbg_detect_carback_device =
{
	.name               = "lidbg_detect_carback",
	.id                 = -1,
};

static struct platform_driver lidbg_detect_carback_driver =
{
	.probe		= lidbg_detect_carback_probe,
	.driver     = {
		.name = "lidbg_detect_carback",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &lidbg_detect_carback_ops,
#endif
	},
};

static int __init lidbg_detect_carback_init(void)
{
	DUMP_BUILD_TIME;
	DUMP_FUN;
#ifndef SUSPEND_ONLINE
	return 0;
#endif
	LIDBG_GET;
	platform_device_register(&lidbg_detect_carback_device);
	platform_driver_register(&lidbg_detect_carback_driver);
	return 0;
}

static void __exit lidbg_detect_carback_exit(void)
{}



module_init(lidbg_detect_carback_init);
module_exit(lidbg_detect_carback_exit);


MODULE_DESCRIPTION("lidbg.detect_carback");
MODULE_LICENSE("GPL");
