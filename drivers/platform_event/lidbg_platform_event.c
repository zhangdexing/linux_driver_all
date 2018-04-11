#include "lidbg.h"
LIDBG_DEFINE;

#define TAG	 "platform_event:"
#define PLATFORMEVENT_FIFO_SIZE 512

static wait_queue_head_t wait_queue;

spinlock_t fifo_lock;

static struct kfifo event_fifo;
static unsigned int *event_buffer;

struct gpio_event_data {
	struct work_struct event_work;
	void (*work_handle)(struct work_struct *);
	int data;
	int irq;
	int gpio;
};

typedef enum
{
	FLY_CARBACK,
}FLY_PLATFORM_EVENTS;

#define GPIO_EVENT_NUM 1

static struct gpio_event_data gpio_event_data_list[GPIO_EVENT_NUM];

static void update_gpio_status(int index)
{
	gpio_event_data_list[index].data = SOC_IO_Input(gpio_event_data_list[index].gpio, gpio_event_data_list[index].gpio, GPIO_CFG_PULL_UP);
	kfifo_in_spinlocked(&event_fifo, &index, sizeof(index), &fifo_lock);
	wake_up_interruptible(&wait_queue);
}

static void disable_gpio_irq(void)
{
	lidbg(TAG"carback  disable gprio irq \n");
	disable_irq(gpio_event_data_list[FLY_CARBACK].irq);
}

static void enable_gpio_irq(void)
{
	lidbg(TAG"carback  enable gprio irq \n");
	enable_irq(gpio_event_data_list[FLY_CARBACK].irq);
}


static int lidbg_platform_event_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	DUMP_FUN;

	switch (event)
	{
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
			disable_gpio_irq();
			break;

		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
			enable_gpio_irq();
			update_gpio_status(FLY_CARBACK);
			break;
		default:
			break;
	}

	return 0;
}

static struct notifier_block lidbg_platform_event_notifier =
{
	.notifier_call = lidbg_platform_event_event,
};


irqreturn_t gpio_state_isr(int irq, void *dev)
{
	lidbg(TAG"gpio irq [%d] is coming \n",irq);

	if(irq == gpio_event_data_list[FLY_CARBACK].irq)
	{
		if(!work_pending(&gpio_event_data_list[FLY_CARBACK].event_work))
			schedule_work(&gpio_event_data_list[FLY_CARBACK].event_work);
	}
	return IRQ_HANDLED;
}


static void work_carback_status_handle(struct work_struct *work)
{
	update_gpio_status(FLY_CARBACK);

	lidbg(TAG">>>>> work_carback_status_handle =======>>>[%s]\n",(gpio_event_data_list[FLY_CARBACK].data == FLY_CARBACK_ENTRY)?"FLY_CARBACK_ENTRY":"FLY_CARBACK_EXIT");
}


int platform_event_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t  platform_event_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{

	unsigned int cmd = 999;
	int data = -1;
	int ret;

	lidbg(TAG"platform_event_read enter\n");

	if(kfifo_is_empty(&event_fifo))
	{
		if(wait_event_interruptible(wait_queue, !kfifo_is_empty(&event_fifo)))
		{
			lidbg(TAG"platform_event_read ERESTARTSYS\n");
			return -ERESTARTSYS;
		}
	}

	ret = kfifo_out_spinlocked(&event_fifo, &cmd, 4,&fifo_lock);
	if(ret < 0)
		lidbg(TAG"platform_event_read kfifo_out_spinlocked failed\n");

	switch(cmd)
	{
		case FLY_CARBACK:
			data = gpio_event_data_list[FLY_CARBACK].data;
			break;
		default:
			lidbg(TAG"platform_event_read exit,no the cmd %u\n",cmd);
			return -EINVAL;
	}

	if(copy_to_user(buffer, &data,  4))
		lidbg(TAG"copy_to_user ERR\n");

	lidbg(TAG"platform_event_read exit,data = %d\n",data);


	return size;
}


ssize_t platform_event_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{

	char *cmd[32] = {NULL};
	int cmd_num  = 0;
	int i;
	char cmd_buf[512];
	unsigned int cmd_buf_list[32];
	memset(cmd_buf, '\0', 512);



	if(copy_from_user(cmd_buf, buf, size))
	{
		PM_ERR("copy_from_user ERR\n");
	}
	if(cmd_buf[size - 1] == '\n')
		cmd_buf[size - 1] = '\0';


	lidbg(TAG"platform_event_write :-------%s-------\n", cmd_buf);
	cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;

	for(i=0 ; i<cmd_num; i++)
	{
		cmd_buf_list[i] = simple_strtoul(cmd[i], 0, 0);
	}

	kfifo_in_spinlocked(&event_fifo, cmd_buf_list, 4*cmd_num, &fifo_lock);

	lidbg(TAG"platform_event_write num :%d\n", cmd_num);
	return size;
}

static  struct file_operations platform_event_fops =
{
	.owner = THIS_MODULE,
	.open = platform_event_open,
	.read = platform_event_read,
	.write = platform_event_write,
};


static int init_gpio_event_data(void)
{
	int ret = 0;

	gpio_event_data_list[FLY_CARBACK].work_handle = work_carback_status_handle;
	gpio_event_data_list[FLY_CARBACK].data = SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP);
	gpio_event_data_list[FLY_CARBACK].gpio = CARBACK_STATE_IO;
	gpio_event_data_list[FLY_CARBACK].irq = GPIO_TO_INT(CARBACK_STATE_IO);

	SOC_IO_ISR_Add(CARBACK_STATE_IO, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, gpio_state_isr, NULL);
	//enable_irq(gpio_event_data_list[FLY_CARBACK].irq); //Must be disable_irq before enable_irq

	INIT_WORK(&gpio_event_data_list[FLY_CARBACK].event_work, gpio_event_data_list[FLY_CARBACK].work_handle);

	return ret;
}

static int lidbg_platform_event_probe(struct platform_device *pdev)
{
	init_waitqueue_head(&wait_queue);
	
	register_lidbg_notifier(&lidbg_platform_event_notifier);

	event_buffer = (unsigned int *)kmalloc(PLATFORMEVENT_FIFO_SIZE, GFP_KERNEL);
	if(event_buffer == NULL)
	{
		lidbg(TAG"kmalloc event_buffer error.\n");
		return 0;
	}

	kfifo_init(&event_fifo, event_buffer, PLATFORMEVENT_FIFO_SIZE);
	spin_lock_init(&fifo_lock);

	lidbg_new_cdev(&platform_event_fops, "flyaudio_event");
	lidbg_shell_cmd("chmod 777 /dev/flyaudio_event");

	init_gpio_event_data();
	update_gpio_status(FLY_CARBACK);

	return 0;
}

#ifdef CONFIG_PM
static int platform_event_pm_suspend(struct device *dev)
{
	DUMP_FUN;

	return 0;
}

static int platform_event_pm_resume(struct device *dev)
{
	DUMP_FUN;

	return 0;
}


static struct dev_pm_ops lidbg_platform_event_ops =
{
	.suspend	= platform_event_pm_suspend,
	.resume	= platform_event_pm_resume,
};
#endif

static struct platform_device lidbg_platform_event_device =
{
	.name               = "lidbg_platform_event",
	.id                 = -1,
};

static struct platform_driver lidbg_platform_event_driver =
{
	.probe		= lidbg_platform_event_probe,
	.driver     = {
		.name = "lidbg_platform_event",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &lidbg_platform_event_ops,
#endif
	},
};

static int __init lidbg_platform_event_init(void)
{
	DUMP_BUILD_TIME;
	DUMP_FUN;
#ifndef SUSPEND_ONLINE
	return 0;
#endif
	LIDBG_GET;
	platform_device_register(&lidbg_platform_event_device);
	platform_driver_register(&lidbg_platform_event_driver);
	return 0;
}

static void __exit lidbg_platform_event_exit(void)
{}



module_init(lidbg_platform_event_init);
module_exit(lidbg_platform_event_exit);


MODULE_DESCRIPTION("lidbg.platform_event");
MODULE_LICENSE("GPL");

