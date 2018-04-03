#include "lidbg.h"
LIDBG_DEFINE;

#define TAG	 "platform_event:"

static wait_queue_head_t wait_queue;
struct work_struct work_carback_status;
static bool is_gpio_trigger = 0;
static int carback_state;

static int lidbg_platform_event_event(struct notifier_block *this,
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
			carback_state = SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP);
			is_gpio_trigger = true; //acc on before into carback
			enable_irq(GPIO_TO_INT(CARBACK_STATE_IO));
			wake_up_interruptible(&wait_queue);
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

	carback_state = val;
	is_gpio_trigger = true;
	wake_up_interruptible(&wait_queue);
}

/*
static void platform_event_suspend(void)
{

	return;
}

static void platform_event_resume(void)
{
	return;
}
*/

int platform_event_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t  platform_event_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	lidbg(TAG"platform_event_read enter\n");

	if(!is_gpio_trigger)
	{
		if(wait_event_interruptible(wait_queue, is_gpio_trigger))
		{
			lidbg(TAG"platform_event_read error\n");
			return -ERESTARTSYS;
		}
	}
	is_gpio_trigger = false;

	if (copy_to_user(buffer, &carback_state,  4))
	{
		lidbg(TAG"copy_to_user ERR\n");
	}
	lidbg(TAG"platform_event_read exit,status = %d\n",carback_state);
	return size;
}


ssize_t platform_event_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
#if 0
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
		lidbg(TAG">>>>> platform_event_write 0=======>>>>>\n");
		carback_state = 0;
		is_gpio_trigger = true;
		wake_up_interruptible(&wait_queue);

	}
	else if(!strcmp(cmd[0],"1"))
	{
		lidbg(TAG">>>>> platform_event_write 1=======>>>>>\n");
		carback_state = 1;
		is_gpio_trigger = true;
		wake_up_interruptible(&wait_queue);
	}
	else
	{
		lidbg(TAG">>>>> platform_event_write other=======>>>>>\n");
		lidbg(TAG">>>>> platform_event_write read gpio %d = %d=======>>>>>\n",CARBACK_STATE_IO,SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP));
	}
#endif
	return size;
}

static  struct file_operations platform_event_fops =
{
	.owner = THIS_MODULE,
	.open = platform_event_open,
	.read = platform_event_read,
	.write = platform_event_write,
};

static int lidbg_platform_event_probe(struct platform_device *pdev)
{

	init_waitqueue_head(&wait_queue);

	register_lidbg_notifier(&lidbg_platform_event_notifier);

	lidbg_new_cdev(&platform_event_fops, "flyaudio_event");
	lidbg_shell_cmd("chmod 777 /dev/flyaudio_event");

	carback_state = SOC_IO_Input(CARBACK_STATE_IO, CARBACK_STATE_IO, GPIO_CFG_PULL_UP);
	SOC_IO_ISR_Add(CARBACK_STATE_IO, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, carback_state_isr, NULL);
	
	is_gpio_trigger = true; //insomd before into carback
	wake_up_interruptible(&wait_queue);
	//enable_irq(GPIO_TO_INT(CARBACK_STATE_IO));  //Must disable_irq before enable_irq

	INIT_WORK(&work_carback_status, work_carback_status_handle);

	

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
