#include "lidbg.h"
LIDBG_DEFINE;

#define DEV_NAME	 "/dev/lidbg_pm0"
#define rmtctrl_FIFO_SIZE (512)

#define SCREE_OFF_JIFF (30)
#define SCREE_OFF_TIME_S (jiffies + SCREE_OFF_JIFF*HZ)

#define GOTO_SLEEP_JIFF (15)
#define GOTO_SLEEP_TIME_S (jiffies + GOTO_SLEEP_JIFF*HZ)

#define AUTO_SLEEP_JIFF (15)
#define AUTO_SLEEP_TIME_S (jiffies + AUTO_SLEEP_JIFF*HZ)

#define UNORMAL_WAKEUP_TIME_MINU (30)
#define UNORMAL_WAKEUP_CNT (UNORMAL_WAKEUP_TIME_MINU*10)

#define ACC_OFF_DETECT_TIME (3)
#define ACC_OFF_DETECT_TIME_S (jiffies + ACC_OFF_DETECT_TIME*HZ)
#define ACC_ON_DETECT_TIME (0)
#define ACC_ON_DETECT_TIME_S (jiffies + ACC_ON_DETECT_TIME*HZ)

#define SCREEN_ON    "flyaudio screen_on"
#define SCREEN_OFF   "flyaudio screen_off"
#define DEVICES_ON   "flyaudio devices_up"
#define DEVICES_DOWN "flyaudio devices_down"
#define REQUEST_FASTBOOT "flyaudio request_fastboot"
#define ANDROID_UP	 "flyaudio android_up"
#define ANDROID_DOWN "flyaudio android_down"
#define GOTO_SLEEP   "flyaudio gotosleep"

static struct notifier_block fb_notif;
static atomic_t status = ATOMIC_INIT(FLY_SCREEN_ON);
static wait_queue_head_t wait_queue;
static struct semaphore rmtctrl_sem;
static struct kfifo rmtctrl_state_fifo;
static unsigned int *rmtctrl_state_buffer;
struct work_struct acc_state_work;
static struct timer_list rmtctrl_timer;
static struct timer_list acc_detect_timer;
static struct wake_lock rmtctrl_wakelock;
static u32 system_unormal_wakeuped_ms = 0;
static u32 system_wakeup_ms = 0;
static u32 repeat_times = 0;
static u32 system_unormal_wakeup_cnt = 0;
struct work_struct work_acc_status;

bool is_fake_acc_off = 0;

void check_airplane_mode(void)
{

    int ret = fs_find_string(g_var.pflyhal_config_list, "AirPlaneModeOn");
    if(ret > 0)
    {
        g_var.suspend_airplane_mode = true;
	 g_var.alarmtimer_interval = 60 ;
    }
        lidbg("<suspend_airplane_mode =%d>\n", g_var.suspend_airplane_mode);
}

static long long ktime_get_ms(void)
{
	static ktime_t k_time;
	long long time_ms;

	k_time = ktime_get_real();
	time_ms = ktime_to_ms(k_time);

	return time_ms;
}

void rmtctrl_fifo_in(void)
{
	down(&rmtctrl_sem);
	kfifo_in(&rmtctrl_state_fifo, (const void *)&atomic_read(&status), 4);
	up(&rmtctrl_sem);
}

irqreturn_t acc_state_isr(int irq, void *dev_id)
{
    lidbg("Acc state irq is coming \n");
    if(!work_pending(&work_acc_status))
        schedule_work(&work_acc_status);
    return IRQ_HANDLED;
}


static void work_acc_status_handle(struct work_struct *work)
{
	int val = -1;
	g_var.acc_counter++;

	val = SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);
	lidbg(">>>>> work_acc_status_handle =======>>>[%s]-(acc_counter:%d |sleep_counter:%d)\n",(val == FLY_ACC_OFF)?"ACC_OFF":"ACC_ON",g_var.acc_counter/2,g_var.sleep_counter);

	if(val == FLY_ACC_OFF)
	{
		LCD_OFF;
		mod_timer(&acc_detect_timer, ACC_OFF_DETECT_TIME_S);
	}
	else
	{     
              if((g_var.acc_flag == FLY_ACC_ON)&& (g_var.flyaudio_reboot == 0))
			LCD_ON;
		mod_timer(&acc_detect_timer, ACC_ON_DETECT_TIME_S);
	}

}


static void send_app_status(FLY_SYSTEM_STATUS state)
{
		lidbg("send_app_status:%d\n", state);
		atomic_set(&status, state);
		rmtctrl_fifo_in();
		wake_up_interruptible(&wait_queue);
}
static void rmtctrl_timer_func(unsigned long data)
{
	if((g_var.acc_flag == FLY_ACC_OFF)&&(is_fake_acc_off == 0)){
       lidbg("rmtctrl_timer_func: goto_sleep %ds later...[usb_request:%d,usb_cam_request:%d]\n", GOTO_SLEEP_JIFF,g_var.usb_request,g_var.usb_cam_request);
	//if(( g_var.usb_request == 0)&&(g_var.usb_cam_request == 0))
       	send_app_status(FLY_GOTO_SLEEP);
       mod_timer(&rmtctrl_timer,GOTO_SLEEP_TIME_S);
	}
	else
	{
		lidbg("g_var.acc_flag=%d\n",g_var.acc_flag);
	}
	return;
}

static void acc_detect_timer_func(unsigned long data)
{
	lidbg("acc_detect_timer completed\n");
	if(!work_pending(&acc_state_work))
	schedule_work(&acc_state_work);

	return;
}

void acc_status_handle(FLY_ACC_STATUS val)
{
	static u32 acc_count = 0;
	if(val == FLY_ACC_ON){
		wake_lock(&rmtctrl_wakelock);
		#ifdef SOC_mt35x
		MSM_DSI83_POWER_ON;
		dsi83_resume();
		#endif
		lidbg("acc_status_handle: FLY_ACC_ON:acc_count=%d\n",acc_count++);
		g_var.acc_flag = FLY_ACC_ON;

		lidbg("acc_status_handle: clear unormal wakeup count.\n");
		system_wakeup_ms = 0;
		repeat_times = 0;
		system_unormal_wakeup_cnt = 0;
		system_unormal_wakeuped_ms = 0;
		lidbg("acc_status_handle: set acc.status to 0\n");
		lidbg_shell_cmd("setprop persist.lidbg.acc.status 0");
		send_app_status(FLY_KERNEL_UP);//wakeup
		lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON));
		//if(g_var.is_fly == 0)
		//    USB_WORK_ENABLE;
		//LCD_ON;
		send_app_status(FLY_SCREEN_ON);
		fs_file_write(DEV_NAME, false, SCREEN_ON, 0, strlen(SCREEN_ON));
		lidbg("acc_status_handle: FLY_ACC_ON del rmtctrl timer.\n");
		del_timer(&rmtctrl_timer);
	}else{
		lidbg("acc_status_handle: FLY_ACC_OFF\n");
		g_var.acc_flag = FLY_ACC_OFF;

		system_unormal_wakeuped_ms = ktime_get_ms();

		lidbg("acc_status_handle: set acc.status to 1\n");
		lidbg_shell_cmd("setprop persist.lidbg.acc.status 1");
		//if(g_var.is_fly == 0)
		//	USB_WORK_DISENABLE;
		//LCD_OFF;
		if(is_fake_acc_off == 0)
			lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF));

//		send_app_status(FLY_GOTO_SLEEP);
		send_app_status(FLY_SCREEN_OFF);
		fs_file_write(DEV_NAME, false, SCREEN_OFF, 0, strlen(SCREEN_OFF));
		if(is_fake_acc_off)
			send_app_status(FLY_GOTO_SLEEP);
		else
		{
			lidbg("acc_status_handle: FLY_ACC_OFF, add rmtctrl timer.\n");
			if(g_var.acc_goto_sleep_time == 0)
				mod_timer(&rmtctrl_timer,SCREE_OFF_TIME_S);
			else
				mod_timer(&rmtctrl_timer, (jiffies + g_var.acc_goto_sleep_time*HZ));

		}
		wake_unlock(&rmtctrl_wakelock);	//ensure KERNEL_UP FB be called before sleep
	}
}

static void acc_state_work_func(struct work_struct *work)
{
	int val = -1;

	val = SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);
	if(val != g_var.acc_flag)
		acc_status_handle(val);
}

static void rmtctrl_suspend(void)
{
	lidbg("rmtctrl_suspend set MCU_APP_GPIO_OFF\n");

	lidbg("acc_state_work_func: FLY_ACC_OFF, set prop AccWakedupState false\n");
	lidbg_shell_cmd("setprop persist.lidbg.AccWakedupState false");

	send_app_status(FLY_DEVICE_DOWN);
	fs_file_write(DEV_NAME, false, DEVICES_DOWN, 0, strlen(DEVICES_DOWN));
	send_app_status(FLY_ANDROID_DOWN);
	fs_file_write(DEV_NAME, false, ANDROID_DOWN, 0, strlen(ANDROID_DOWN));

	return;
}

static void rmtctrl_resume(void)
{
	lidbg("rmtctrl_resume fb\n");

	lidbg("acc_state_work_func: FLY_ACC_ON, set prop AccWakedupState true\n");
	lidbg_shell_cmd("setprop persist.lidbg.AccWakedupState true"); //prop for stopping kill_process when ACC_ON

	send_app_status(FLY_ANDROID_UP);
	fs_file_write(DEV_NAME, false, ANDROID_UP, 0, strlen(ANDROID_UP));
	send_app_status(FLY_DEVICE_UP);
	fs_file_write(DEV_NAME, false, DEVICES_ON, 0, strlen(DEVICES_ON));
	
	if(is_fake_acc_off)
	{
		lidbg_shell_cmd("cat /proc/dsi83_rst &");
		LCD_ON;
		send_app_status(FLY_SCREEN_ON);
	}
	
	if((g_var.acc_flag == FLY_ACC_OFF)&&(is_fake_acc_off == 0)){
		lidbg("rmtctrl_resume, g_var.acc_flag is FLY_ACC_OFF, add rmtctrl timer.\n");
		mod_timer(&rmtctrl_timer,AUTO_SLEEP_TIME_S);
	}
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
			lidbg( "rmtctrl:FB_BLANK_UNBLANK.rmtctrl_resume\n");
			rmtctrl_resume();
		}
		else if (*blank == FB_BLANK_POWERDOWN)
		{
			lidbg( "rmtctrl:FB_BLANK_POWERDOWN.rmtctrl_suspend\n");
			rmtctrl_suspend();
		}
	}

	return 0;
}
int rmtctrl_open (struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  rmtctrl_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	int bytes;
	int rmtctrl_state;
	
	lidbg("rmtctrl_read\n");
	if(kfifo_is_empty(&rmtctrl_state_fifo))
	{
	    if(wait_event_interruptible(wait_queue, !kfifo_is_empty(&rmtctrl_state_fifo)))
	        return -ERESTARTSYS;
	}

	if( kfifo_len(&rmtctrl_state_fifo) == 0)
	{
	    lidbg("copy_to_user ERR\n");
	    return -1;
	}
	down(&rmtctrl_sem);
	bytes = kfifo_out(&rmtctrl_state_fifo, &rmtctrl_state, 4);
	up(&rmtctrl_sem);

	if (copy_to_user(buffer, &rmtctrl_state,  4))
	{
		lidbg("copy_to_user ERR\n");
	}
	return size;
}

ssize_t rmtctrl_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}

static int lidbg_rmtctrl_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SLEEP_TIMEOUT):
		send_app_status(FLY_SLEEP_TIMEOUT);
        break;
    default:
        break;
    }

    return NOTIFY_DONE;
}

static int unormal_wakeup_handle(void)
{
		system_unormal_wakeup_cnt++;

		system_wakeup_ms = ktime_get_ms() - system_unormal_wakeuped_ms;  //tics ms after acc_off
		lidbg("*** system wakeup %u(%u) times in %d(%u) msec, repeat_times %d***\n", system_unormal_wakeup_cnt, UNORMAL_WAKEUP_CNT, system_wakeup_ms, (UNORMAL_WAKEUP_TIME_MINU * 60 * 1000), repeat_times);
		if(system_unormal_wakeup_cnt > UNORMAL_WAKEUP_CNT){

			if(system_wakeup_ms < (UNORMAL_WAKEUP_TIME_MINU * 60 * 1000)){
				lidbgerr("System wakeup %d times in %d(%u) msec,system tics %lld, unormal\n", system_unormal_wakeup_cnt, system_wakeup_ms, (UNORMAL_WAKEUP_TIME_MINU * 60 * 1000), ktime_get_ms());

				if(g_var.acc_flag == FLY_ACC_OFF)
				{
					if( g_var.suspend_timeout_protect  == 0)
					{
						lidbgerr("%s suspend timeout,reboot!!\n",__FUNCTION__);
					}
					else
					{
						send_app_status(FLY_SLEEP_TIMEOUT);
						repeat_times++;
						if(repeat_times >= 5)
						{
							lidbgerr("%s suspend timeout,reboot!!\n",__FUNCTION__);
							ssleep(10);
							lidbg_shell_cmd("reboot");
						}
					}
				}
			}else
				lidbg("System wakeup %d times in %d(%u) msec,system tics %lld, normal\n", system_unormal_wakeup_cnt, system_wakeup_ms, (UNORMAL_WAKEUP_TIME_MINU * 60 * 1000), ktime_get_ms());

			//count again
			system_wakeup_ms = 0;
			system_unormal_wakeup_cnt = 0;
			system_unormal_wakeuped_ms = ktime_get_ms();
		}

	return 0;
}


static struct notifier_block lidbg_rmtctrl_notifier =
{
    .notifier_call = lidbg_rmtctrl_event,
};

static  struct file_operations rmtctrl_fops =
{
    .owner = THIS_MODULE,
    .open = rmtctrl_open,
    .read = rmtctrl_read,
    .write = rmtctrl_write,
};


void update_acc_status(void)
{
    g_var.acc_flag = SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);
    if(g_var.acc_flag == FLY_ACC_OFF)
    {
    	lidbg("%s: set acc.status to 1\n",__FUNCTION__);
    	lidbg_shell_cmd("setprop persist.lidbg.acc.status 1");
   }
    else
    {
       lidbg("%s: set acc.status to 0\n",__FUNCTION__);
    	lidbg_shell_cmd("setprop persist.lidbg.acc.status 0");
    }

}


static int thread_check_acc_and_response_acc_off_delay(void *data)
{
   check_airplane_mode();

   update_acc_status();
   while(0==g_var.android_boot_completed)
    {
        ssleep(1);
	 lidbg("wait for android_boot_completed.\n");
    }
    update_acc_status();

    ssleep(20);
	
    if(g_var.suspend_airplane_mode == true)
    {
	   lidbg_shell_cmd("setprop persist.lidbg.AirplaneEnable 1");
    }
	
    g_var.acc_flag = SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);

    PM_WARN("<current acc state.stop:%d,%d >\n",g_var.acc_flag,(g_var.acc_flag == FLY_ACC_OFF));
    if(g_var.acc_flag == FLY_ACC_OFF)
    {
        PM_WARN("<response_acc_off,more delay 20S :%d,%d >\n",g_var.acc_flag,(g_var.acc_flag == FLY_ACC_OFF));
        acc_status_handle(g_var.acc_flag);
    }

	SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);
	SOC_IO_ISR_Add(MCU_ACC_STATE_IO, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, acc_state_isr, NULL);
	lidbg("rmtctrl probe: enable_irq_wake %d\n",GPIO_TO_INT(MCU_ACC_STATE_IO));
	enable_irq_wake(GPIO_TO_INT(MCU_ACC_STATE_IO));
	
     return 1;
}
static int lidbg_rmtctrl_probe(struct platform_device *pdev)
{
	int ret;

       fs_mem_log("<have a deep check,is enum FLY_ACC_OFF == 0  :%d,[g_var.acc_flag:%d ,  FLY_ACC_OFF:%d] >\n",(g_var.acc_flag == FLY_ACC_OFF),g_var.acc_flag,FLY_ACC_OFF);
       FS_REGISTER_INT(g_var.alarmtimer_interval, "alarmtimer_wakeup_time", 5, NULL);
	fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&fb_notif);
	if (ret)
		PM_ERR("Unable to register fb_notifier: %d\n",ret);

	rmtctrl_state_buffer = (unsigned int *)kmalloc(rmtctrl_FIFO_SIZE, GFP_KERNEL);
	if(rmtctrl_state_buffer == NULL)
	{
	    lidbg("rmtctrl kmalloc state buffer error.\n");
	    return 0;
	}
	INIT_WORK(&acc_state_work, acc_state_work_func);

	MCU_APP_GPIO_ON;
	lidbg("rmtctrl probe: MCU_APP_GPIO_ON\n");

	init_timer(&acc_detect_timer);
	acc_detect_timer.function = acc_detect_timer_func;
	acc_detect_timer.data = 0;
	acc_detect_timer.expires = 0;

	init_timer(&rmtctrl_timer);
	rmtctrl_timer.function = rmtctrl_timer_func;
	rmtctrl_timer.data = 0;
	rmtctrl_timer.expires = 0;

	register_lidbg_notifier(&lidbg_rmtctrl_notifier);

	lidbg_shell_cmd("setprop persist.lidbg.acc.status 0");
	lidbg("rmtctrl probe: init prop AccWakedupState\n");
	lidbg_shell_cmd("setprop persist.lidbg.AccWakedupState false");

	wake_lock_init(&rmtctrl_wakelock, WAKE_LOCK_SUSPEND, "lidbg_rmtctrl");

	//lidbg_shell_cmd("svc data disable &");
	init_waitqueue_head(&wait_queue);
	sema_init(&rmtctrl_sem, 1);
	kfifo_init(&rmtctrl_state_fifo, rmtctrl_state_buffer, rmtctrl_FIFO_SIZE);
	lidbg_new_cdev(&rmtctrl_fops, "flyaudio_pm0");
	lidbg_chmod("/dev/flyaudio_pm0");
	
	//enable_irq_wake(GPIO_TO_INT(MCU_IIC_REQ_GPIO));
	 INIT_WORK(&work_acc_status, work_acc_status_handle);

        if(g_var.recovery_mode == 0)
                CREATE_KTHREAD(thread_check_acc_and_response_acc_off_delay, NULL);
	else
		g_var.acc_flag =  FLY_ACC_ON;

	return 0;
}

#ifdef CONFIG_PM
static int rmtctrl_pm_suspend(struct device *dev)
{
    DUMP_FUN;

    return 0;
}
static int rmtctrl_pm_resume(struct device *dev)
{
    DUMP_FUN;

	lidbg("rmtctrl_pm_resume, acc_io_state is %s\n", (g_var.acc_flag == FLY_ACC_ON)?"FLY_ACC_ON":"FLY_ACC_OFF");
//	if(g_var.system_status == FLY_KERNEL_DOWN)
//		send_app_status(FLY_KERNEL_UP);
	if(g_var.acc_flag == FLY_ACC_OFF)
		unormal_wakeup_handle();

	if(is_fake_acc_off)
	{
		lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON));
	}
	if((g_var.acc_flag == FLY_ACC_OFF)&&(is_fake_acc_off == 0)){
		lidbg("rmtctrl_pm_resume, acc_io_state is FLY_ACC_OFF, add rmtctrl timer.\n");
		mod_timer(&rmtctrl_timer,AUTO_SLEEP_TIME_S);
	}
    return 0;
}
static struct dev_pm_ops lidbg_rmtctrl_ops =
{
    .suspend	= rmtctrl_pm_suspend,
    .resume	= rmtctrl_pm_resume,
};
#endif

static struct platform_device lidbg_rmtctrl_device =
{
    .name               = "lidbg_rmtctrl",
    .id                 = -1,
};

static struct platform_driver lidbg_rmtctrl_driver =
{
    .probe		= lidbg_rmtctrl_probe,
    .driver     = {
        .name = "lidbg_rmtctrl",
        .owner = THIS_MODULE,
 #ifdef CONFIG_PM
        .pm = &lidbg_rmtctrl_ops,
 #endif
    },
};

static int __init lidbg_rmtctrl_init(void)
{
    DUMP_BUILD_TIME;
    DUMP_FUN;
#ifndef SUSPEND_ONLINE
    return 0;
#endif
    LIDBG_GET;
    platform_device_register(&lidbg_rmtctrl_device);
    platform_driver_register(&lidbg_rmtctrl_driver);
    return 0;
}
  
static void __exit lidbg_rmtctrl_exit(void)
{}

 void  fake_acc_off(void)
{
	g_var.acc_flag= FLY_ACC_OFF;
	is_fake_acc_off = 1;
	acc_status_handle(g_var.acc_flag);
}

module_init(lidbg_rmtctrl_init);
module_exit(lidbg_rmtctrl_exit);

EXPORT_SYMBOL(fake_acc_off);

MODULE_DESCRIPTION("lidbg.rmtctrl");
MODULE_LICENSE("GPL");
