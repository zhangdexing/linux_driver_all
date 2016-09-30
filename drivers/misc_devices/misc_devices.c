
#include "lidbg.h"
//#define DISABLE_USB_WHEN_DEVICE_DOWN
#if 0//def SUSPEND_ONLINE
#define DISABLE_USB_WHEN_GOTO_SLEEP
#else
#define DISABLE_USB_WHEN_ANDROID_DOWN
#endif
//#define FORCE_UMOUNT_UDISK

LIDBG_DEFINE;

int udisk_stability_test = 0;
#define LCD_ON_DELAY (1500)//acc_on-->lcd_on
static struct wake_lock device_wakelock;
//int usb_request = 0;

#if defined(CONFIG_FB)
struct notifier_block devices_notif;
int thread_lcd_on_delay(void *data)
{
    DUMP_FUN_ENTER;
    lidbg( "misc:%d\n",LCD_ON_DELAY);
    msleep(LCD_ON_DELAY);

	if(g_var.acc_flag==FLY_ACC_ON)
	{
		lidbg("dsi83.LCD_ON2.real\n");
		LCD_ON;
		msleep(1000);
		lidbg("LCD_ON2.in.hold_bootanim2.false\n");
		lidbg_shell_cmd("setprop lidbg.hold_bootanim2 false");
#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974) 
		lidbg_shell_cmd("setprop lidbg.hold_bootanim false");
#endif
		lidbg_shell_cmd("echo echoLCD_ON2.hold_bootanim2.false > /dev/lidbg_msg");
	}
	else
        	lidbg("LCD_ON2.skip\n");
    return 0;
}
static int devices_notifier_callback(struct notifier_block *self,
                                     unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank;

    if (evdata && evdata->data && event == FB_EVENT_BLANK)
    {
        blank = evdata->data;
        if (*blank == FB_BLANK_UNBLANK)
        {
            lidbg( "misc:FB_BLANK_UNBLANK,%d/%d\n",g_var.system_status,FLY_KERNEL_UP);
            if(g_var.system_status >= FLY_KERNEL_UP)
            {
                if((g_var.led_hal_status & g_var.led_app_status)&&(g_var.acc_flag==FLY_ACC_ON)&&(g_var.flyaudio_reboot==0))
                {
        		lidbg("dsi83.LCD_ON2.thread\n");
        		//CREATE_KTHREAD(thread_lcd_on_delay,NULL);
                }
                else
        		lidbg("dsi83.LCD_ON2.skip.%d,%d,%d,%d\n",g_var.led_hal_status,g_var.led_app_status,g_var.acc_flag,g_var.flyaudio_reboot);
            }

            g_var.fb_on = 1;
        }
        else if (*blank == FB_BLANK_POWERDOWN)
        {
            lidbg( "misc:FB_BLANK_POWERDOWN\n");
           // LCD_OFF;
            g_var.fb_on = 0;
        }
    }

    return 0;
}
#endif


static u32 last_usb_off_time = 0;

void usb_camera_enable(bool enable)
{
    DUMP_FUN;

    lidbg("[%s]\n", enable ? "usb_enable" : "usb_disable");
    if(enable)
    	{ 
    	 wake_lock(&device_wakelock);
	 while(get_tick_count() - last_usb_off_time < 500 ) 
	 {
		 lidbg("%d,%d\n",get_tick_count(),last_usb_off_time);
		 msleep(100);
	 }
	 lidbg("%d,%d\n",get_tick_count(),last_usb_off_time);
        USB_FRONT_WORK_ENABLE;
    	}
    else
    {
        USB_WORK_DISENABLE;
	 last_usb_off_time = get_tick_count();
	 wake_unlock(&device_wakelock);	
    }
}

#define UDISK_LOG_PATH 	 LIDBG_LOG_DIR"udisk_stable_test.txt"
static int thread_udisk_stable(void *data)
{
    static int cnt = 0;
    static int err_cnt = 0;
    cnt ++;
    msleep(1000*10);
    lidbg_domineering_ack();
    if(g_var.usb_status == true)
    {
   	if(!fs_is_file_exist(USB_MOUNT_POINT"/udisk_stable_test"))
   	{
   		 err_cnt++;
		 lidbg_fs_log(UDISK_LOG_PATH, "udisk_not_found:%d,%d\n", cnt,err_cnt);
		 //lidbg_loop_warning();
		 lidbg_toast_show("udisk_stable_test", "udisk_not_found");
   	}
	else
	        lidbg("thread_udisk_stable:%d,%d\n", cnt,err_cnt);
    }
    return 1;
}

#ifdef USB_HUB_SUPPORT
static int thread_usb_hub_check(void *data)
{
    msleep(2000);
     if(g_var.usb_status == 1)
     {
	    if(!fs_is_file_exist("/sys/bus/usb/drivers/usb/1-1"))
	    {
	        lidbgerr("thread_usb_hub_check fail!\n");
	        USB_POWER_DISABLE;
		 msleep(500);
		 USB_ID_HIGH_DEV;
	        ssleep(2);
		 USB_ID_LOW_HOST;
		 USB_POWER_ENABLE;
	    }
	    else
	    {
	       lidbg("thread_usb_hub_check success\n");
	    }
     }
    return 1;
}
#endif


void usb_disk_enable(bool enable)
{
    DUMP_FUN;
    lidbg("%d,%d\n", g_var.usb_status,enable);

   if(g_var.usb_status == enable)
   {
   	lidbg("usb_disk_enable skip\n");
   	return;
   }
    lidbg("60.[%s]\n", enable ? "usb_enable" : "usb_disable");
    if(enable)
    {
    	 wake_lock(&device_wakelock);
        while(get_tick_count() - last_usb_off_time < 500 ) 
	 {
		 lidbg("%d,%d\n",get_tick_count(),last_usb_off_time);
		 msleep(100);
	 }
	 lidbg("%d,%d\n",get_tick_count(),last_usb_off_time);
        USB_WORK_ENABLE;
#ifdef USB_HUB_SUPPORT
	CREATE_KTHREAD(thread_usb_hub_check, NULL);
#endif
	 if(g_var.udisk_stable_test != 0)
	 	 CREATE_KTHREAD(thread_udisk_stable, NULL);
    }
    else
    {
#ifdef FORCE_UMOUNT_UDISK
        lidbg("call lidbg_umount\n");
        lidbg_shell_cmd("/system/lib/modules/out/lidbg_umount &");
        lidbg_shell_cmd("/flysystem/lib/out/lidbg_umount &");
        msleep(4000);
#else
        lidbg("USB_WORK_DISENABLE.200.unmount\n");
        lidbg_shell_cmd("umount /storage/udisk");
        msleep(200);
#endif
        USB_WORK_DISENABLE;
	 last_usb_off_time = get_tick_count();
	 wake_unlock(&device_wakelock);
    }
}
static int thread_usb_disk_enable_delay(void *data)
{
#ifndef FLY_USB_CAMERA_SUPPORT
    if(g_var.recovery_mode == 0)
        msleep(5000);
#endif

#if ANDROID_VERSION >= 600
	 if((g_var.recovery_mode == 0)&&(g_var.android_boot_completed == 0))
	 {
	   while(0==g_var.android_boot_completed)
	    {
	        ssleep(1);
		 lidbg("thread_usb_disk_enable_delay wait for android_boot_completed.\n");
	    }
	     ssleep(5);
	 }
  #endif
  
    usb_disk_enable(true);
    SET_USB_ID_SUSPEND;

    if(0)
    {
        ssleep(10);
        lidbg("usb : trigge udisk remount uevent  to vold.after sleep 10S \n");
        lidbg_shell_cmd("echo add > /sys/block/sda/uevent");
    }


    return 1;
}


static int thread_usb_disk_disable_delay(void *data)
{
#ifdef DISABLE_USB_WHEN_DEVICE_DOWN
    //msleep(2000);
#else
    //msleep(1000);
#endif

	if(( g_var.usb_request == 1)||(g_var.usb_cam_request== 1))
		lidbg("Usb still being used, don't disable it actually...\n");
	else{
		lidbg("Usb be not used,disable it...\n");
		if(g_var.platformid==ID14_MSM8974_600)
		{
			lidbg("umount udisk\n");
			lidbg_shell_cmd("am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 2");
			msleep(5000);
		}

		usb_disk_enable(false);
		if( g_var.usb_request == 1)
		{
		lidbg("\n\n\n===========disable usb but request so. enable it================\n");
		usb_disk_enable(true);
		}
		if( g_var.usb_cam_request == 1)
		{
		lidbg("\n\n\n===========disable usb but request so. enable it================\n");
		usb_camera_enable(true);
		}
	}

    return 1;
}

int thread_shutdown_bt_power(void *data)
{
    DUMP_FUN;
    ssleep(10);
    lidbg_shell_cmd("echo appcmd *158#069 > /dev/lidbg_drivers_dbg0 &");
    return 0;
}

static int lidbg_dev_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_OFF):
        //if(!g_var.is_fly)
    {
        LCD_OFF;
#ifdef PLATFORM_ID_16
	disable_irq(GPIO_TO_INT(6));
#endif
	 if(g_var.is_fly == 0)
	      CREATE_KTHREAD(thread_usb_disk_disable_delay, NULL);
        //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_OFF));
        //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_OFF));
    }
    break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
#ifdef DISABLE_USB_WHEN_DEVICE_DOWN
        CREATE_KTHREAD(thread_usb_disk_disable_delay, NULL);
#endif
#if 0//def VENDOR_QCOM
        lidbg("set uart to gpio\n");
        SOC_IO_Output_Ext(0, g_hw.gpio_dvd_tx, 1, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA);
        SOC_IO_Output_Ext(0, g_hw.gpio_dvd_rx, 1, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA);
        SOC_IO_Output_Ext(0, g_hw.gpio_bt_tx, 1, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA);
        SOC_IO_Output_Ext(0, g_hw.gpio_bt_rx, 1, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA);
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_ANDROID_DOWN):
        //MSM_DSI83_DISABLE;
#ifdef DISABLE_USB_WHEN_ANDROID_DOWN
        CREATE_KTHREAD(thread_usb_disk_disable_delay, NULL);
#endif
        		lidbg("hold_bootanim2.true\n");
		lidbg_shell_cmd("setprop lidbg.hold_bootanim2 true");
#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974) 
		lidbg_shell_cmd("setprop lidbg.hold_bootanim true");
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_GOTO_SLEEP):
#ifdef DISABLE_USB_WHEN_GOTO_SLEEP
		CREATE_KTHREAD(thread_usb_disk_disable_delay, NULL);
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_KERNEL_DOWN):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_KERNEL_UP):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_ANDROID_UP):
#if (defined DISABLE_USB_WHEN_ANDROID_DOWN) || (defined DISABLE_USB_WHEN_GOTO_SLEEP)
 	  CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_UP):
	#ifdef MUC_CONTROL_DSP
	  if(!g_var.recovery_mode && !g_var.is_fly)
	    	CREATE_KTHREAD(thread_sound_dsp_init, NULL);
	#endif
#if 0//def VENDOR_QCOM
        lidbg("set gpio to uart\n");
        SOC_IO_Config(g_hw.gpio_dvd_tx, GPIOMUX_FUNC_2, GPIOMUX_OUT_HIGH, GPIOMUX_PULL_NONE, GPIOMUX_DRV_16MA);
        SOC_IO_Config(g_hw.gpio_dvd_rx, GPIOMUX_FUNC_2, GPIOMUX_OUT_HIGH, GPIOMUX_PULL_NONE, GPIOMUX_DRV_16MA);
        SOC_IO_Config(g_hw.gpio_bt_tx, GPIOMUX_FUNC_2, GPIOMUX_OUT_HIGH, GPIOMUX_PULL_NONE, GPIOMUX_DRV_16MA);
        SOC_IO_Config(g_hw.gpio_bt_rx, GPIOMUX_FUNC_2, GPIOMUX_OUT_HIGH, GPIOMUX_PULL_NONE, GPIOMUX_DRV_16MA);
#endif
#ifdef DISABLE_USB_WHEN_DEVICE_DOWN
        CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_ON):
#ifdef PLATFORM_ID_16
	enable_irq(GPIO_TO_INT(6));
#endif
	if(g_var.keep_lcd_on) LCD_ON;
        //if(!g_var.is_fly)
    {
#ifdef SUSPEND_ONLINE
        g_var.usb_status = 0;
#endif
        if((g_var.led_hal_status & g_var.led_app_status)/*&&(g_var.fb_on == 1)*/&&(g_var.flyaudio_reboot==0))
        {
        		lidbg("LCD_ON3\n");
        		CREATE_KTHREAD(thread_lcd_on_delay,NULL);
        }
#ifdef FLY_USB_CAMERA_SUPPORT
		CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
#endif
        //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_ON));
        //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_ON));
    }
    if(g_var.platformid==ID11_MSM8909_511&&g_recovery_meg->bootParam.upName.val==1)
        CREATE_KTHREAD(thread_shutdown_bt_power, NULL);
    break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK):
	 lidbg("backlight warning\n");
        LCD_OFF;
        msleep(100);
        LCD_ON;
        break;
    default:
        break;
    }

    return NOTIFY_DONE;
}


static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_dev_event,
};

int dev_open(struct inode *inode, struct file *filp)
{
    return 0;
}
int dev_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static void parse_cmd(char *pt)
{
    int argc = 0;
    char *argv[32] = {NULL};

    lidbg("%s\n", pt);
    argc = lidbg_token_string(pt, " ", argv);

    if (!strcmp(argv[0], "lcd_on"))
    {
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_ON));
    }
    else if (!strcmp(argv[0], "lcd_off"))
    {
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_OFF));
    }
    else if (!strcmp(argv[0], "performance_high"))
    {
#ifdef SOC_msm8x26
        set_system_performance(3);
#endif
    }
    else if (!strcmp(argv[0], "performance_middle"))
    {
#ifdef SOC_msm8x26
        set_system_performance(2);
#endif
    }
    else if (!strcmp(argv[0], "performance_low"))
    {
#ifdef SOC_msm8x26
        set_system_performance(1);
#endif
    }
    else if (!strcmp(argv[0], "acc_debug_mode"))
    {
        lidbg("acc_debug_mode enable!");
        g_var.is_debug_mode = 1;
	 lidbg_shell_cmd("echo appcmd *158#077 > /dev/lidbg_drivers_dbg0");
    }

    else if (!strcmp(argv[0], "udisk_enable"))
    {
        lidbg("Misc devices ctrl: udisk_enable");
	 usb_disk_enable(true);
    }
    else if (!strcmp(argv[0], "udisk_disable"))
    {
        lidbg("Misc devices ctrl: udisk_disable");
        usb_disk_enable(false);
    }
    else if (!strcmp(argv[0], "udisk_request"))
    {
        	lidbg("Misc devices ctrl: udisk_request");
#if defined(PLATFORM_msm8909) && defined(BOARD_V1)
 		g_var.usb_request= 1;
		usb_disk_enable(true);
#else
		 g_var.usb_cam_request = 1;
		if(g_var.acc_flag == FLY_ACC_OFF)
		    usb_camera_enable(true);
#endif
		
    }
    else if (!strcmp(argv[0], "udisk_unrequest"))
    {
        	lidbg("Misc devices ctrl: udisk_unrequest");
#if defined(PLATFORM_msm8909) && defined(BOARD_V1)
		 g_var.usb_request = 0;
		 usb_disk_enable(false);
#else
		  g_var.usb_cam_request= 0;
		  if(g_var.acc_flag == FLY_ACC_OFF)
		 	usb_camera_enable(false);
#endif
    }
    else if (!strcmp(argv[0], "usb_reboot"))
    {
        lidbg("Misc devices ctrl: usb_reboot");
        USB_POWER_DISABLE;
	 msleep(500);
	 USB_ID_HIGH_DEV;
        ssleep(2);
	 USB_ID_LOW_HOST;
	 USB_POWER_ENABLE;
	}
    else if (!strcmp(argv[0], "gps_request"))
    {
        	lidbg("Misc devices ctrl: gps_request");
		if(g_var.acc_flag == FLY_ACC_OFF)
		{
	    		wake_lock(&device_wakelock);
			GPS_POWER_ON;
		}
    }
    else if (!strcmp(argv[0], "gps_unrequest"))
    {
        	lidbg("Misc devices ctrl: gps_unrequest");
		if(g_var.acc_flag == FLY_ACC_OFF)
		{
			GPS_POWER_OFF;
    			wake_unlock(&device_wakelock);
		}
    }
    else if (!strcmp(argv[0], "flyaudio_reboot"))
    {
        g_var.flyaudio_reboot=1;
	 LCD_OFF;
	 GPIO_NOT_READY;
	 //HAL_NOT_READY;
	 //MCU_WP_GPIO_OFF;
        lidbg("Misc devices ctrl: g_var.flyaudio_reboot=1\n");
    }
}


static ssize_t dev_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    char *p = NULL;
    int len = size;
    char tmp[size + 1];//C99 variable length array
    char *mem = tmp;

    memset(mem, '\0', size + 1);

    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    if((p = memchr(mem, '\n', size)))
    {
        len = p - mem;
        *p = '\0';
    }
    else
        mem[len] =  '\0';

    parse_cmd(mem);

    return size;//warn:don't forget it;
}

static struct file_operations dev_fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .write = dev_write,
    .release = dev_close,
};

int usb_enumerate_limit = 5;
void usb_enumerate_monitor(char *key_word, void *data)
{
    DUMP_FUN;
    if(!g_var.is_udisk_needreset || usb_enumerate_limit <= 0)
    {
        lidbg("find key word.return %d,%d\n", !g_var.is_udisk_needreset, usb_enumerate_limit);
        return;
    }
    lidbg("find key word.in %d\n", usb_enumerate_limit);
    g_var.is_udisk_needreset = 0;
    usb_enumerate_limit--;
#ifndef FLY_USB_CAMERA_SUPPORT
    if(g_var.system_status >= FLY_ANDROID_UP)
    {
        usb_disk_enable(0);
        ssleep(2);
        usb_disk_enable(1);
    }
#endif
}


void sd_error_monitor(char *key_word, void *data)
{
    static int sd_error_monitor_limit = 7;

    DUMP_FUN;
    lidbg("find key word.in\n");
    if( sd_error_monitor_limit <= 0)
    {
        lidbg("find key word.return %d\n", sd_error_monitor_limit);
        return;
    }
    lidbg_shell_cmd("mount -o remount /mnt/media_rw/sdcard1");
    sd_error_monitor_limit--;
    
}


int thread_udisk_stability_test(void *data)
{
    u32 cnt = 0;
    while(1)
    {
	ssleep(1);

	while(g_var.udisk_stable_test == 1)
	{
		usb_disk_enable(1);
		ssleep(15);
		usb_disk_enable(0);
		ssleep(5);
		cnt++;
		lidbg("udisk_stability_test times=%d\n", cnt);
	}
    }

}

#ifdef PLATFORM_ID_7
static bool  is_udisk_added = 0;
static int usb_nb_misc_func(struct notifier_block *nb, unsigned long action, void *data)
{
    if(!is_udisk_added)
        lidbg("stop.thread_udisk_en\n");
    is_udisk_added = 1;
    return NOTIFY_OK;
}
static struct notifier_block usb_nb_misc =
{
    .notifier_call = usb_nb_misc_func,
};
int thread_udisk_en(void *data)
{
    int loop = 0;
    DUMP_FUN_ENTER;
    ssleep(3);
    while(!is_udisk_added)
    {
        lidbg("thread_udisk_en====%d\n", ++loop);
        IO_CONFIG_OUTPUT(0, 73);
        soc_io_output(0, 73, 0);
        ssleep(2);
    }
    lidbg("thread_udisk_en===exit=%d\n", loop);
    return 0;
}
#endif


static int soc_dev_probe(struct platform_device *pdev)
{
#if defined(CONFIG_FB)
    devices_notif.notifier_call = devices_notifier_callback;
    fb_register_client(&devices_notif);
#endif

    register_lidbg_notifier(&lidbg_notifier);

    CREATE_KTHREAD(thread_led, NULL);
#ifdef MUC_CONTROL_DSP
  if(!g_var.recovery_mode && !g_var.is_fly)
    	CREATE_KTHREAD(thread_sound_dsp_init, NULL);
#endif
    //CREATE_KTHREAD(thread_thermal, NULL);

    if((g_var.is_fly == 0) || (g_var.recovery_mode == 1))
    {
        // CREATE_KTHREAD(thread_button_init, NULL);
        //CREATE_KTHREAD(thread_key, NULL);

        //LCD_ON;
    }
    LCD_ON;
    //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_ON));
    //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_ON));

    lidbg_new_cdev(&dev_fops, "flydev");

   // lidbg_trace_msg_cb_register("unable to enumerate USB device", NULL, usb_enumerate_monitor);
    lidbg_trace_msg_cb_register("clusters badly computed", NULL, sd_error_monitor);

    FS_REGISTER_INT(udisk_stability_test, "udisk_stability_test", 0, NULL);

  //  if(udisk_stability_test == 1)
    {
        CREATE_KTHREAD(thread_udisk_stability_test, NULL);
    }
     CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
   
    GPIO_IS_READY;
    SET_GPIO_READY_SUSPEND;
    //SET_HAL_READY_SUSPEND;
#ifdef PLATFORM_ID_7
    if(0)
    {
        usb_register_notify(&usb_nb_misc);
        CREATE_KTHREAD(thread_udisk_en, NULL);
    }
#endif
#ifdef PLATFORM_ID_16
	GPS_POWER_ON;
#endif
    return 0;

}
static int soc_dev_remove(struct platform_device *pdev)
{
    lidbg("soc_dev_remove\n");
   
    if(!g_var.is_fly) {}

    return 0;

}
static int  soc_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
    lidbg("soc_dev_suspend\n");

    if(!g_var.is_fly)
    {
        //button_suspend();
    }
    led_suspend();
    GPIO_NOT_READY;
    HAL_NOT_READY;
    SET_GPIO_READY_SUSPEND;
    SET_HAL_READY_SUSPEND;
    return 0;

}
static int soc_dev_resume(struct platform_device *pdev)
{
    lidbg("soc_dev_resume \n");
    if(!g_var.is_fly)
    {
        //button_resume();
        led_resume();
    }
    GPIO_IS_READY;
    HAL_NOT_READY;
    return 0;
}
struct platform_device soc_devices =
{
    .name			= "misc_devices",
    .id 			= 0,
};
static struct platform_driver soc_devices_driver =
{
    .probe = soc_dev_probe,
    .remove = soc_dev_remove,
    .suspend = soc_dev_suspend,
    .resume = soc_dev_resume,
    .driver = {
        .name = "misc_devices",
        .owner = THIS_MODULE,
    },
};

static void set_func_tbl(void)
{
    return;
}



int dev_init(void)
{
    lidbg("=======misc_dev_init========\n");
    LIDBG_GET;
    set_func_tbl();
    wake_lock_init(&device_wakelock, WAKE_LOCK_SUSPEND, "lidbg_device_wakelock");
    platform_device_register(&soc_devices);
    platform_driver_register(&soc_devices_driver);
    return 0;
}

void dev_exit(void)
{
    lidbg("dev_exit\n");
}

void lidbg_device_main(int argc, char **argv)
{
    lidbg("lidbg_device_main\n");

    if(argc < 1)
    {
        lidbg("Usage:\n");
        return;
    }
}
EXPORT_SYMBOL(lidbg_device_main);

MODULE_AUTHOR("fly, <fly@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");

module_init(dev_init);
module_exit(dev_exit);

