#include "lidbg.h"
LIDBG_DEFINE;

#define DEV_NAME	 "/dev/lidbg_pm0"
#define SCREEN_ON    "flyaudio screen_on"
#define SCREEN_OFF   "flyaudio screen_off"
#define DEVICES_ON   "flyaudio devices_up"
#define DEVICES_DOWN "flyaudio devices_down"
#define REQUEST_FASTBOOT "flyaudio request_fastboot"
#define ANDROID_UP	 "flyaudio android_up"
#define ANDROID_DOWN "flyaudio android_down"
#define GOTO_SLEEP   "flyaudio gotosleep"
#ifdef PLATFORM_msm8909
#define POWER_SUSPEND_TIME1   (jiffies + 60*HZ)//delay between screenoff->devicedown->androiddown
#else
#define POWER_SUSPEND_TIME1   (jiffies + 30*HZ)//delay between screenoff->devicedown->androiddown
#endif
#define POWER_SUSPEND_TIME2   (jiffies + 30*HZ)//delay between screenoff->devicedown->androiddown
#define AirplanMode_TIME     (jiffies + 0*HZ)//wakelock hold time between androiddown->wakelock release
#define POWERKEY_DELAY_TIME     (jiffies + 0*HZ)//wakelock hold time between androiddown->wakelock release
#define POWERKEY_FIFO_SIZE (512)

static struct notifier_block fb_notif;
static struct timer_list timer;
static atomic_t status = ATOMIC_INIT(FLY_SCREEN_ON);
static wait_queue_head_t wait_queue;
static DECLARE_COMPLETION (sleep_powerkey_wait);
static DECLARE_COMPLETION (request_fastboot_wait);
static struct semaphore powerkey_sem;
static struct kfifo powerkey_state_fifo;
static unsigned int *powerkey_state_buffer;



FLY_ACC_STATUS lpc_acc_state = FLY_ACC_ON;

void powerkey_fifo_in(void)
{
	int i;
	down(&powerkey_sem);
	i = atomic_read(&status);
	kfifo_in(&powerkey_state_fifo, (const void *)(&i), 4);
	up(&powerkey_sem);
}

static int thread_gpio_powerkey_status(void *data)
{
	int io_state = 0;
	int io_state_cnt = 0;
	while(1)
	{
		wait_for_completion(&request_fastboot_wait);
		while(1)
		{
			io_state = SOC_IO_Input(0, GPIO_FASTBOOT_REQUEST, GPIO_CFG_NO_PULL);
			if(io_state == 1){
				lidbg("Request fastboot, FASTBOOT_REQUEST_IO(%d) is set to %d, io_state_cnt = %d.\n", GPIO_FASTBOOT_REQUEST, io_state, io_state_cnt);
				mod_timer(&timer,POWERKEY_DELAY_TIME);
				break;	//break to start fastboot
			}else{
				io_state_cnt++;
				if(io_state_cnt > 10){
					lidbg("Request fastboot timeout, but continue ANDROID_DOWN\n");
					mod_timer(&timer,POWERKEY_DELAY_TIME);
					io_state_cnt = 0;
					break;	//break with timeout
				}

				if(atomic_read(&status) != FLY_FASTBOOT_REQUEST){
					lidbg("Request fastboot received acc_on state, acc_state = %d\n", atomic_read(&status));
					io_state_cnt = 0;
					break;	//break with state changed
				}

				msleep(200);
			}
		}
	}
	return 1;
}

static int thread_powerkey_func(void *data)
{
	while(1)
	{
		wait_for_completion(&sleep_powerkey_wait);
		lidbg("%s = %d\n", __FUNCTION__,atomic_read(&status));
		powerkey_fifo_in();
		wake_up_interruptible(&wait_queue);
		switch(atomic_read(&status))
		{
			case FLY_SCREEN_OFF:
				g_var.acc_flag = FLY_ACC_OFF;
				fs_file_write(DEV_NAME, false, SCREEN_OFF, 0, strlen(SCREEN_OFF));
				mod_timer(&timer,POWER_SUSPEND_TIME1);
				break;
			case FLY_DEVICE_DOWN:
				g_var.acc_flag = FLY_ACC_OFF;
				fs_file_write(DEV_NAME, false, DEVICES_DOWN, 0, strlen(DEVICES_DOWN));
				mod_timer(&timer,POWER_SUSPEND_TIME2);
				break;
			case FLY_FASTBOOT_REQUEST:
				fs_file_write(DEV_NAME, false, REQUEST_FASTBOOT, 0, strlen(REQUEST_FASTBOOT));
				complete(&request_fastboot_wait);
				break;
			case FLY_ANDROID_DOWN:
				g_var.acc_flag = FLY_ACC_OFF;
				fs_file_write(DEV_NAME, false, ANDROID_DOWN, 0, strlen(ANDROID_DOWN));
				mod_timer(&timer,AirplanMode_TIME);
				break;
			case FLY_GOTO_SLEEP:
				//fs_file_write(DEV_NAME, false, GOTO_SLEEP, 0, strlen(GOTO_SLEEP));
				//mod_timer(&timer,POWERKEY_DELAY_TIME);
				break;
			case FLY_ANDROID_UP:
				g_var.acc_flag = FLY_ACC_ON;
				fs_file_write(DEV_NAME, false, ANDROID_UP, 0, strlen(ANDROID_UP));
				mod_timer(&timer,POWERKEY_DELAY_TIME);
				break;
			case FLY_DEVICE_UP:
				g_var.acc_flag = FLY_ACC_ON;
				fs_file_write(DEV_NAME, false, DEVICES_ON, 0, strlen(DEVICES_ON));
				mod_timer(&timer,POWERKEY_DELAY_TIME);
				break;
			case FLY_SCREEN_ON:
				g_var.acc_flag = FLY_ACC_ON;
				fs_file_write(DEV_NAME, false, SCREEN_ON, 0, strlen(SCREEN_ON));
				mod_timer(&timer,POWERKEY_DELAY_TIME);
				break;
			default:
				break;
		}
	}
	return 0;
}

static void powerkey_suspend_timer(unsigned long data)
{
	if(atomic_read(&status) != FLY_SCREEN_ON)
	{
		atomic_inc(&status);
		complete(&sleep_powerkey_wait);
	}
	return;
}

static void powerkey_suspend(void)
{
	lidbg("powerkey_suspend status = %d\n", atomic_read(&status));

	if(atomic_read(&status) == FLY_SCREEN_ON )
	{
		atomic_set(&status, FLY_SCREEN_OFF);
		complete(&sleep_powerkey_wait);
	}
	return;
}

static void powerkey_resume(void)
{
	lidbg("powerkey_resume status = %d\n", atomic_read(&status));
	#if 0
	lpc_acc_state = SOC_IO_Input(0, GPIO_FASTBOOT_REQUEST, GPIO_CFG_NO_PULL);
	if(lpc_acc_state == FLY_ACC_OFF)
	{
	     lidbg("resume when accoff,return!\n");
	     return;
	}
       #endif
	if(atomic_read(&status) == FLY_SCREEN_OFF)
	{
		del_timer(&timer);
		atomic_set(&status, FLY_SCREEN_ON);
	}
	else if((atomic_read(&status) == FLY_DEVICE_DOWN) || (atomic_read(&status) == FLY_FASTBOOT_REQUEST))
	{
		del_timer(&timer);
		atomic_set(&status, FLY_DEVICE_UP);
	}
	else if((atomic_read(&status) == FLY_ANDROID_DOWN)
		|| (g_var.system_status == FLY_ANDROID_DOWN) || (g_var.system_status == FLY_GOTO_SLEEP))
    	{
		return;
    	}
	else
    	{
		atomic_set(&status, FLY_ANDROID_UP);
    	}		

	complete(&sleep_powerkey_wait);
	return;
}


static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	if (evdata && evdata->data && event == FB_EVENT_BLANK)
	{
		blank = evdata->data;

		if (*blank == FB_BLANK_UNBLANK)
		{
			lidbg_shell_cmd("setprop persist.lidbg.AccWakedupState true");
			powerkey_resume();
		}
		else if (*blank == FB_BLANK_POWERDOWN)
		{
			lidbg_shell_cmd("setprop persist.lidbg.AccWakedupState false");
			powerkey_suspend();
		}
	}

	return 0;
}
int powerkey_open (struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  powerkey_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	int bytes;
	int powerkey_state;
	
	if(kfifo_is_empty(&powerkey_state_fifo))
	{
	    if(wait_event_interruptible(wait_queue, !kfifo_is_empty(&powerkey_state_fifo)))
	        return -ERESTARTSYS;
	}

	if( kfifo_len(&powerkey_state_fifo) == 0)
	{
	    lidbg("copy_to_user ERR\n");
	    return -1;
	}
	down(&powerkey_sem);
	bytes = kfifo_out(&powerkey_state_fifo, &powerkey_state, 4);
	up(&powerkey_sem);

	if (copy_to_user(buffer, &powerkey_state,  4))
	{
		lidbg("copy_to_user ERR\n");
	}
	return size;
}

ssize_t powerkey_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}

static  struct file_operations powerkey_fops =
{
    .owner = THIS_MODULE,
    .open = powerkey_open,
    .read = powerkey_read,
    .write = powerkey_write,
};

static int lidbg_powerkey_probe(struct platform_device *pdev)
{
	int ret;

	init_timer(&timer);
	timer.function = powerkey_suspend_timer;
	timer.data = 0;
	timer.expires = 0;

	fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&fb_notif);
	if (ret)
		PM_ERR("Unable to register fb_notifier: %d\n",ret);

	powerkey_state_buffer = (unsigned int *)kmalloc(POWERKEY_FIFO_SIZE, GFP_KERNEL);
	if(powerkey_state_buffer == NULL)
	{
	    lidbg("powerkey kmalloc state buffer error.\n");
	    return 0;
	}
	init_waitqueue_head(&wait_queue);
	sema_init(&powerkey_sem, 1);
	kfifo_init(&powerkey_state_fifo, powerkey_state_buffer, POWERKEY_FIFO_SIZE);
	lidbg_new_cdev(&powerkey_fops, "flyaudio_pm0");
	lidbg_shell_cmd("chmod 777 /dev/flyaudio_pm0");
	CREATE_KTHREAD(thread_powerkey_func, NULL);
	CREATE_KTHREAD(thread_gpio_powerkey_status, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int pk_suspend(struct device *dev)
{
    DUMP_FUN;

    return 0;
}
static int pk_resume(struct device *dev)
{
    DUMP_FUN;

    return 0;
}
static struct dev_pm_ops lidbg_powerkey_ops =
{
    .suspend	= pk_suspend,
    .resume	= pk_resume,
};
#endif

static struct platform_device lidbg_powerkey =
{
    .name               = "lidbg_powerkey",
    .id                 = -1,
};

static struct platform_driver lidbg_powerkey_driver =
{
    .probe		= lidbg_powerkey_probe,
    .driver     = {
        .name = "lidbg_powerkey",
        .owner = THIS_MODULE,
 #ifdef CONFIG_PM
        .pm = &lidbg_powerkey_ops,
 #endif
    },
};

static int __init lidbg_powerkey_init(void)
{
    DUMP_FUN;
#ifdef SUSPEND_ONLINE
    return 0;
#endif
    LIDBG_GET;
    platform_device_register(&lidbg_powerkey);
    platform_driver_register(&lidbg_powerkey_driver);
    return 0;
}

static void __exit lidbg_powerkey_exit(void)
{
}
 

module_init(lidbg_powerkey_init);
module_exit(lidbg_powerkey_exit);

MODULE_DESCRIPTION("lidbg.powerkey");
MODULE_LICENSE("GPL");
