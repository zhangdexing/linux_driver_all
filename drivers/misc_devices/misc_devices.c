
#include "lidbg.h"
//#define DISABLE_USB_WHEN_DEVICE_DOWN
#if 0//def SUSPEND_ONLINE
#define DISABLE_USB_WHEN_GOTO_SLEEP
#else
#define DISABLE_USB_WHEN_ANDROID_DOWN
#endif
//#define FORCE_UMOUNT_UDISK

LIDBG_DEFINE;
static struct timer_list usb_release_timer;

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

#ifdef USB_HUB_SUPPORT
static int thread_usb_hub_check(void *data)
{
    msleep(2000);
     if(g_var.usb_status == 1)
     {
	    if(!fs_is_file_exist("/sys/bus/usb/drivers/usb/1-1"))
	    {
	    	lidbgerr("thread_usb_hub_check fail!\n");
#ifdef SOC_msm8x26
	#ifdef PLATFORM_msm8996
		 USB_POWER_FRONT_DISABLE;
		 ssleep(2);
		 USB_POWER_FRONT_ENABLE;
	#else
	         USB_POWER_DISABLE;
		 msleep(500);
		 USB_ID_HIGH_DEV;
	         ssleep(2);
		 USB_ID_LOW_HOST;
		 USB_POWER_ENABLE;
	#endif
#endif

#ifdef SOC_mt35x
	 	lidbg("set usb a_host mode\n");
       	lidbg_readwrite_file("/sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode", NULL, "a_host", strlen("a_host"));
#endif

	    }
	    else
	    {
	       lidbg("thread_usb_hub_check success\n");
	    }
     }
     if(!fs_is_file_exist("/storage/sdcard1"))
     {
          lidbg("sdcard1 reuevent\n");
          lidbg_shell_cmd("echo appcmd *158#101 > /dev/lidbg_drivers_dbg0");
     }
     else
          lidbg("sdcard1 success\n");
    return 1;
}
#endif


void usb_disk_enable(bool enable)
{
    DUMP_FUN;
    lidbg("%d,%d\n", g_var.usb_status,enable);

   if((g_var.usb_status == enable)&&(enable == 1))
   {
   	lidbg("usb_disk_enable skip \n");
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
    return 1;
}


static int thread_usb_disk_disable_delay(void *data)
{

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

static int misc_dev_dev_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_OFF):
        //if(!g_var.is_fly)
    {
        LCD_OFF;
#ifdef SOC_mt35x
	disable_irq(GPIO_TO_INT(6));
#endif
	 if(g_var.is_fly == 0)
	      CREATE_KTHREAD(thread_usb_disk_disable_delay, NULL);
        //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_OFF));
        //lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_OFF));
    }
	mod_timer(&usb_release_timer,jiffies + 180*HZ);
	lidbg_shell_cmd("echo backcar_type $(getprop fly.set.cvsb.swit) > /dev/flydev0");
	lidbg("unmount usb udisk\n");
	lidbg_shell_cmd("am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 2");
    break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
		DEVICE3_3_POWER_OFF;
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
#if 0//def SOC_mt35x
	 lidbg("set usb a_host mode\n");
        lidbg_readwrite_file("/sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode", NULL, "a_host", strlen("a_host"));
#endif
 	  CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_UP):
	 DEVICE3_3_POWER_ON;
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
#if 0//def SOC_mt35x
	 lidbg("set usb a_host mode\n");
        lidbg_readwrite_file("/sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode", NULL, "a_host", strlen("a_host"));
#endif
        CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
#endif
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_ON):
	del_timer(&usb_release_timer);
#ifdef SOC_mt35x
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
	lidbg("mount usb udisk\n");
	lidbg_shell_cmd("am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 1");
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
    .notifier_call = misc_dev_dev_event,
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
    else if (!strcmp(argv[0], "ISRDecodeAction"))
    {
        lidbg("start flydecode_1\n");
        lidbg_shell_cmd("/flysystem/lib/out/flydecode_1 -s /sdcard/updateapp -d /flyapdata/install -f 1  -m 755 -c 1");
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
	 lidbg_shell_cmd("ps > /sdcard/ps_start.txt &");
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

		 g_var.usb_cam_request = 1;
		if(g_var.acc_flag == FLY_ACC_OFF)
		{
		    usb_camera_enable(true);
		    mod_timer(&usb_release_timer,jiffies + 180*HZ);
		}
		
    }
    else if (!strcmp(argv[0], "udisk_unrequest"))
    {
        	lidbg("Misc devices ctrl: udisk_unrequest");
		  g_var.usb_cam_request= 0;
		  if(g_var.acc_flag == FLY_ACC_OFF)
		  {
		 	usb_camera_enable(false);
			del_timer(&usb_release_timer);
		  }
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
    else  if(!strcmp(argv[0], "udisk_reset"))
    {
        USB_WORK_DISENABLE;
        ssleep(2);
       USB_WORK_ENABLE;
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
    else if (!strcmp(argv[0], "backcar_type"))
    {
		if((argv[1] != NULL ) && (!strcmp(argv[1], "1")))
		{
	        	lidbg("set backcar: usb\n");
		       g_var.backcar_type = BACKCAR_TYPE_USB;

		}
		else
		{
	        	lidbg("set backcar: cvbs\n");
		       g_var.backcar_type = BACKCAR_TYPE_CVBS;

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
	else if (!strncmp(argv[0], "rear_osd", 7))
	{
		char *keyval[2] = {NULL};//key-vals
		int rear_osdVal;
		lidbg_token_string(argv[0], "=", keyval) ;
		rear_osdVal = simple_strtoul(keyval[1], 0, 0);
		if (!strcmp(keyval[1], "1"))
		{
			lidbg("Rear OSD Enable!\n");
			lidbg_shell_cmd("/flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-oe 1 1 ");
			lidbg_shell_cmd("/flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-car 0 0 0&");
		}
		else if (!strcmp(keyval[1], "0"))
		{
			lidbg("Rear OSD Disable!\n");
			lidbg_shell_cmd("/flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-oe 0 0&");
		}
		else lidbg("Rear OSD ERROR CMD!\n");
	}
	else if (!strncmp(argv[0], "rear_hmirror", 12))
	{
		char *keyval[2] = {NULL};//key-vals
		int rear_hmirrorVal;
		lidbg_token_string(argv[0], "=", keyval) ;
		rear_hmirrorVal = simple_strtoul(keyval[1], 0, 0);
		if (!strcmp(keyval[1], "1"))
		{
			lidbg("Rear Mirror Enable!\n");
			lidbg_shell_cmd("/flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-mir 1&");
		}
		else if (!strcmp(keyval[1], "0"))
		{
			lidbg("Rear Mirror Disable!\n");
			lidbg_shell_cmd("/flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-mir 0&");
		}
		else lidbg("Rear Mirror ERROR CMD!\n");
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


#define UDISK_LOG_PATH 	 LIDBG_LOG_DIR"udisk_stable_test.txt"
u32 err_cnt = 0, cnt = 0, retry_cnt = 0;
bool is_udisk_file_exist(void)
{
    if(g_var.usb_status == true)
    {
        char shell_cmd[128] = {0};
        if(!fs_is_file_exist(USB_MOUNT_POINT"/udisk_stable_test"))
        {
            if(retry_cnt ==1)
            {
                err_cnt++;
                sprintf(shell_cmd, "udisk_not_found:%d/%d\n", err_cnt, cnt);
                lidbg_toast_show("udisk_stable_test:", shell_cmd);
                fs_file_write2(UDISK_LOG_PATH, shell_cmd);
            }
            else
            {
                sprintf(shell_cmd, "more retry:%d %d\n", retry_cnt, cnt);
                lidbg_toast_show("udisk_stable_test:", shell_cmd);
                lidbg("%s", shell_cmd);
            }
            return false;
        }
        else
        {
            sprintf(shell_cmd, "udisk_found:%d/%d\n", err_cnt, cnt);
            lidbg_toast_show("udisk_stable_test:", shell_cmd);
            lidbg("%s", shell_cmd);
            return true;
        }
    }
    return false;
}
int thread_udisk_stability_test(void *data)
{
    while(1)
    {
        ssleep(3);
        while(g_var.udisk_stable_test == 1)
        {
            cnt++;
            retry_cnt = 60;

            usb_disk_enable(1);
            ssleep(20);
            while(--retry_cnt > 0 && !is_udisk_file_exist())
            {
                ssleep(1);
            }
            usb_disk_enable(0);
            ssleep(5);
        }
    }
}


int thread_usb_camera_enable(void *data)
{
	if(g_var.acc_flag == FLY_ACC_OFF)
	{
		lidbg("usb_release\n");
		g_var.usb_cam_request= 0;
		usb_camera_enable(false);
	}
	return 0;
}


static void usb_release_timer_func(unsigned long data)
{
	if((g_var.acc_flag == FLY_ACC_OFF)&&(g_var.usb_cam_request ==1))
	{
	      CREATE_KTHREAD(thread_usb_camera_enable, NULL);
	}
	return;
}


int thread_dev_init(void *data)
{
//poweron
    LCD_ON;
    GPS_POWER_ON;
    LEVEL_CONVERSION_ENABLE;

//gpio
    GPIO_IS_READY;
    SET_GPIO_READY_SUSPEND;
    if((g_var.is_fly == 0) && (g_var.recovery_mode == 0) )
	   HAL_IS_READY;
//led
     CREATE_KTHREAD(thread_led, NULL);

//sound
 #ifdef MUC_CONTROL_DSP
  if(!g_var.recovery_mode && !g_var.is_fly)
    	CREATE_KTHREAD(thread_sound_dsp_init, NULL);
#endif

 //usb
    if(g_var.recovery_mode == 1)//enable udisk asap
    {
	USB_FRONT_WORK_ENABLE;
	USB_BACK_WORK_ENABLE;
	USB_POWER_UDISK_ENABLE;
    }
   else
   {
	USB_WORK_DISENABLE;
	msleep(500);
	USB_FRONT_WORK_ENABLE;
	USB_BACK_WORK_ENABLE;
	CREATE_KTHREAD(thread_usb_disk_enable_delay, NULL);
	CREATE_KTHREAD(thread_udisk_stability_test, NULL);
   }
    return 0;
}

static int soc_dev_probe(struct platform_device *pdev)
{
#if defined(CONFIG_FB)
    devices_notif.notifier_call = devices_notifier_callback;
    fb_register_client(&devices_notif);
#endif
    wake_lock_init(&device_wakelock, WAKE_LOCK_SUSPEND, "lidbg_device_wakelock");

    init_timer(&usb_release_timer);
    usb_release_timer.function = usb_release_timer_func;
    usb_release_timer.data = 0;
    usb_release_timer.expires = 0;

    register_lidbg_notifier(&lidbg_notifier);

    lidbg_new_cdev(&dev_fops, "flydev0");

    CREATE_KTHREAD(thread_dev_init, NULL);
    return 0;

}
static int soc_dev_remove(struct platform_device *pdev)
{
    lidbg("soc_dev_remove\n");
    return 0;

}
static int  soc_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
    lidbg("soc_dev_suspend\n");
    LEVEL_CONVERSION_ENABLE;
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
     led_resume();
    GPIO_IS_READY;

    if((!g_var.is_fly) && (g_var.recovery_mode == 0) )
	   HAL_IS_READY;
    else
          HAL_NOT_READY;
	
    LEVEL_CONVERSION_ENABLE;

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

