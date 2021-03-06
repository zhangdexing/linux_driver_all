#include "lidbg.h"
LIDBG_DEFINE;

static int reboot_delay_s = 0;
static int delete_out_dir_after_update = 1;
static int loop_warning_en = 0;

#include "system_switch.c"

#define TAG "lidbg_misc: "

char *white_udisk[] =
{
    "/storage/udisk-2E8D-9DD3",
    NULL,
};
bool is_white_udisk(char *path)
{
    int j;
    if(path == NULL)
        return false;
    for(j = 0; white_udisk[j] != NULL; j++)
    {
        if (!strcmp(path, white_udisk[j]))
        {
            lidbg(TAG"\nwhite_udisk:%s\n", path);
            return true;
        }
    }
    return false;
}

void unhandled_monitor(char *key_word, void *data)
{
    //DUMP_FUN;
    lidbg(TAG"find key word\n");

    if( !fs_is_file_exist("/dev/log/no_reboot"))
    {
        lidbg_fs_log("/dev/log/no_reboot", "unhandled find");
        lidbg_shell_cmd("chmod 777 /data");
        lidbg_loop_warning();
    }
}

void lidbgerr_monitor(char *key_word, void *data)
{
    //DUMP_FUN;
    lidbg(TAG"find key word\n");
    lidbg_loop_warning();
}

#define REBOOT_SIG_FILE LIDBG_LOG_DIR"thread_reboot.txt"

int thread_reboot(void *data)
{

    if(!reboot_delay_s )
    {
        lidbg(TAG"<reb.exit0.%d>\n", reboot_delay_s);
        return 0;
    }

    //if exist,means:the last time between current-reboot_delay_s had reboot.
    if(fs_is_file_exist(REBOOT_SIG_FILE))
    {
        lidbg(TAG"<reb.exit1.%d,%d>\n", reboot_delay_s, fs_is_file_exist(REBOOT_SIG_FILE));
        g_var.is_debug_mode = 1;
        lidbg_loop_warning();
        return 0;
    }

    //write signal file in current time
    fs_file_write2(REBOOT_SIG_FILE, "right");
    ssleep(3);
    LIDBG_WARN(TAG"reb.warn.%s:%d,%d", LIDBG_LOG_DIR"thread_reboot.txt", reboot_delay_s, fs_is_file_exist(REBOOT_SIG_FILE));

    //make sure above is succeed.
    if( !fs_is_file_exist(REBOOT_SIG_FILE))
    {
        lidbg(TAG"<reb.exit2.%d,%d>\n", reboot_delay_s, fs_is_file_exist(REBOOT_SIG_FILE));
        return 0;
    }

    //wait
    ssleep(reboot_delay_s);


    //detect after sleep
    while(1)
    {
        char shell_cmd[64] = {0};
        sprintf(shell_cmd, "rm -rf %s", REBOOT_SIG_FILE);
        lidbg_shell_cmd(shell_cmd);
        ssleep(2);
        if(!fs_is_file_exist(REBOOT_SIG_FILE))
        {
            lidbg(TAG"<reb.succeed.%d>\n", reboot_delay_s);
            lidbg_shell_cmd("reboot lidbg_reboot_test");
            ssleep(2);
            //if above way failed ,try the way below again.
            lidbg_shell_cmd("sync");
            ssleep(3);
            kernel_restart(NULL);
            return 0;
        }
        else
        {
            lidbg(TAG"<reb.exit3.%d,%d>\n", reboot_delay_s, fs_is_file_exist(REBOOT_SIG_FILE));
        }
    }
    return 0;
}

int loop_warnning(void *data)
{

    while(1)
    {
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK));
        msleep(5000);
    }
    return 0;
}

void lidbg_loop_warning(void)
{
    static bool is_loop_warning = 0;
    if(is_loop_warning == 0)
    {
        if((loop_warning_en) || (g_var.is_debug_mode == 1))
        {
            DUMP_FUN;
            CREATE_KTHREAD(loop_warnning, NULL);
            is_loop_warning = 1;
        }
    }
}
void cb_kv_reboot_recovery(char *key, char *value)
{
    if(value && *value == '1')
        lidbg_shell_cmd("reboot recovery");
    else
        fs_mem_log("cb_kv_reboot_recovery:fail,%s\n", value);
}

void cb_kv_cmd(char *key, char *value)
{
    if(value)
    {
        lidbg_shell_cmd(value);
        fs_mem_log("cb_kv_cmd:%s\n", value);
    }
}

static struct completion udisk_misc_wait;
static int thread_udisk_misc(void *data)
{
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    while(!kthread_should_stop())
    {

	    if(g_var.recovery_mode == 1)
	    {
	       // lidbg(TAG"g_var.recovery_mode == 1 \n");
			msleep(1000);
		}
        else if(!wait_for_completion_interruptible(&udisk_misc_wait))
		{
			lidbg(TAG"wait_for_completion_interruptible udisk_misc_wait \n");
		}

		
        {
            int i = 0;

			
            {
                int pos = 0;
                char buff[128] = {0};
                char *pPah[] = {"", "/storage/sdcard1/conf/lidbg_udisk_shell.conf", "/sdcard/conf/lidbg_udisk_shell.conf", NULL,};
                get_udisk_file_path(buff, "conf/lidbg_udisk_shell.conf");
                pPah[0] = buff;
                while(i < 6 )
                {
                    for(pos = 0; pPah[pos] != NULL; pos++)
                    {
                        if(fs_is_file_exist(pPah[pos]))
                            goto found;
                    }
                    msleep(200);
                    i++;
                }
found:
                if(pPah[pos] && fs_is_file_exist(pPah[pos]))
                {
                    LIST_HEAD(lidbg_udisk_shell_list);
                    LIDBG_WARN(TAG"use:%s\n", pPah[pos] );
                    fs_fill_list(pPah[pos], FS_CMD_FILE_LISTMODE, &lidbg_udisk_shell_list);
                    if(analyze_list_cmd(&lidbg_udisk_shell_list))
                        LIDBG_WARN(TAG"exe success\n" );
                }
                else
                    LIDBG_ERR(TAG"miss:lidbg_udisk_shell\n" );
            }
        }
    }
    return 1;
}
static __le16 udiskvender[2];
static int usb_nb_misc_func(struct notifier_block *nb, unsigned long action, void *data)
{
    struct usb_device *dev = data;
    int usb_class;
    switch (action)
    {
    case USB_DEVICE_ADD:
        usb_class = lidbg_get_usb_device_type(dev);//USB_CLASS_MASS_STORAGE
        if(dev && dev->product && dev->descriptor.idVendor && dev->descriptor.idProduct)
        {
            if(strstr(dev->product, "troller") == NULL && udiskvender[0] != dev->descriptor.idVendor && udiskvender[1] != dev->descriptor.idProduct)
            {
                g_var.is_udisk_needreset = 1;
                udiskvender[0] = dev->descriptor.idVendor;
                udiskvender[1] = dev->descriptor.idProduct;
            }
        }
        break;
    case USB_DEVICE_REMOVE:
        if(g_var.recovery_mode == 1)
        {
            lidbg(TAG"umount /usb \n");
            lidbg_shell_cmd("umount /usb");
        }
        if(dev->portnum == 1)
        {
            lidbg(TAG"stop fuse udisk server.fuudiskskip \n");
            //lidbg_shell_cmd("setprop persist.fuseusb.enable 0");
        }
        else
            LIDBG_WARN(TAG"stop fuse udisk server skip:%d\n", dev->portnum);
        break;
    }
    return NOTIFY_OK;
}
static struct notifier_block usb_nb_misc =
{
    .notifier_call = usb_nb_misc_func,
};

int misc_open (struct inode *inode, struct file *filp)
{
    return 0;
}
ssize_t misc_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    int argc = 0;
    char *argv[32] = {NULL};
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg(TAG"copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';

    argc = lidbg_token_string(cmd_buf, ":", argv);
    if(argc >= 2 && argv[1] != NULL && (!strcmp(argv[0], "flyaudio")))
    {
        lidbg_shell_cmd(argv[1]);
    }
    else if(argc >= 2 && argv[1] != NULL && (!strcmp(argv[0], "conf_check")))
    {
	    lidbg(TAG"conf_check\n");
	    set_udisk_path(argv[1]);
	    if(is_white_udisk(argv[1])||!g_var.is_first_update)
	        complete(&udisk_misc_wait);
	    else
	    {
	        char cmd[128] = {0};
	        sprintf(cmd, "mv -f %s/conf %s/conf_bak", get_udisk_file_path(NULL, NULL), get_udisk_file_path(NULL, NULL));
	        lidbg_shell_cmd(cmd);
	        lidbg_shell_cmd("mv -f /sdcard1/conf /sdcard1/conf_bak");
	        lidbg(TAG"conf_check.skip\n");
	    }
    }
    else
        LIDBG_ERR(TAG"%d\n", argc);
    return size;
}

static  struct file_operations misc_nod_fops =
{
    .owner = THIS_MODULE,
    .write = misc_write,
    .open = misc_open,
};
void checkif_wifiap_error(void)
{
    int size = fs_get_file_size("/data/misc/wifi/hostapd.conf");
    LIDBG_WARN(TAG"<%d>\n\n", size);
    if(size < 20000)
    {
#ifdef PLATFORM_ID_2
        LIDBG_WARN(TAG"<find error>\n\n");
        lidbg_shell_cmd("cp -rf /flysystem/lib/out/hostapd_g8_4.4.2.conf /data/misc/wifi/hostapd.conf");
#endif
#ifdef PLATFORM_ID_4
        LIDBG_WARN(TAG"<find error>\n\n");
        lidbg_shell_cmd("cp -rf /flysystem/lib/out/hostapd_g9_4.4.4.conf /data/misc/wifi/hostapd.conf");
#endif
    }
}


void check_display_mode(void)
{
    bool exist = fs_is_file_exist("/persist/display/pp_calib_data.bin");
    int HYFeature = fs_find_string(g_var.pflyhal_config_list, "HYFeatureDisplayon");
    int Israel = fs_find_string(g_var.pflyhal_config_list, "IsraelFeatureDisplayon");
    LIDBG_WARN(TAG"<lcd_type=%d HYFeatureDisplayon.in [%d,%d]Israel.%d,lcd_manufactor:%d>\n", g_var.hw_info.lcd_type, HYFeature, exist, Israel, g_var.hw_info.lcd_manufactor);
    if(HYFeature > 0)
    {
        LIDBG_WARN(TAG"<use HYFeatureDisplayon>\n");
        lidbg_shell_cmd("cp -rf /flysystem/lib/out/pp_calib_data.bin /persist/display/");
        lidbg_shell_cmd("chmod 777 /persist/display/pp_calib_data.bin");
    }
    else if(Israel > 0)
    {
        LIDBG_WARN(TAG"<use IsraelFeatureDisplayon>\n");
        lidbg_shell_cmd("cp -rf /flysystem/lib/out/pp_calib_data_Israel.bin /persist/display/pp_calib_data.bin");
        lidbg_shell_cmd("chmod 777 /persist/display/pp_calib_data.bin");
    }
    else
    {
        LIDBG_WARN(TAG"<check fly lcd Feature\n");
#ifdef PLATFORM_msm8909
        switch (g_var.hw_info.lcd_type)
        {
        case 1:
            LIDBG_WARN(TAG"<pp_calib_data7.bin>\n");
            lidbg_shell_cmd("cp -rf /flysystem/lib/out/pp_calib_data7.bin /persist/display/pp_calib_data.bin");
            break;
        case 2:
            LIDBG_WARN(TAG"<pp_calib_data8.bin>\n");
            lidbg_shell_cmd("cp -rf /flysystem/lib/out/pp_calib_data8.bin /persist/display/pp_calib_data.bin");
            break;
        case 3:
            LIDBG_WARN(TAG"<pp_calib_data10.bin>\n");
            lidbg_shell_cmd("cp -rf /flysystem/lib/out/pp_calib_data10.bin /persist/display/pp_calib_data.bin");
            break;
        case 4:
            LIDBG_WARN(TAG"<pp_calib_data9.bin>\n");
            lidbg_shell_cmd("cp -rf /flysystem/lib/out/pp_calib_data9.bin /persist/display/pp_calib_data.bin");
            break;
        default:
            LIDBG_WARN(TAG"<check fly lcd Feature,err lcd_type:%d\n", g_var.hw_info.lcd_type);
            break;
        }
        lidbg_shell_cmd("chmod 777 /persist/display/pp_calib_data.bin");
#else
        {
            char shell_cmd[256] = {0};
            sprintf(shell_cmd, "/flysystem/flytheme/config/flyaudio_lcd_calib_mode_%d_%02d.xml ",  g_var.hw_info.lcd_type, g_var.hw_info.lcd_manufactor);
            LIDBG_WARN(TAG"<others.use %s-->%d>\n", shell_cmd, fs_is_file_exist(shell_cmd));

            sprintf(shell_cmd, "cp -rf /flysystem/flytheme/config/flyaudio_lcd_calib_mode_%d_%02d.xml /data/misc/display/disp_user_calib_data_nt35596_1080p_video_mode_dsi_panel.xml",  g_var.hw_info.lcd_type, g_var.hw_info.lcd_manufactor);
            lidbg_shell_cmd(shell_cmd);
            lidbg_shell_cmd("chmod 777 /data/misc/display/disp_user_calib_data_nt35596_1080p_video_mode_dsi_panel.xml");
        }
#endif
    }
}
static int thread_check_display_mode(void *data)
{
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    check_display_mode();
    return 1;
}

int misc_init(void *data)
{
    LIDBG_WARN(TAG"<==IN==>\n");
    init_completion(&udisk_misc_wait);
    system_switch_init();
    CREATE_KTHREAD(thread_check_display_mode, NULL);
    FS_REGISTER_INT(reboot_delay_s, "reboot_delay_s", 0, NULL);
    FS_REGISTER_INT(delete_out_dir_after_update, "delete_out_dir_after_update", 0, NULL);
    FS_REGISTER_INT(loop_warning_en, "loop_warning_en", 0, NULL);
    FS_REGISTER_KEY( "cmdstring", cb_kv_cmd);
    FS_REGISTER_KEY( "reboot_recovery", cb_kv_reboot_recovery);
    FS_REGISTER_KEY( "lidbg_origin_system", cb_kv_lidbg_origin_system);
    CREATE_KTHREAD(thread_reboot, NULL);
    CREATE_KTHREAD(thread_udisk_misc, NULL);
    usb_register_notify(&usb_nb_misc);
    checkif_wifiap_error();
    LIDBG_WARN(TAG"<==OUT==>\n\n");
    LIDBG_MODULE_LOG;

    //lidbg_trace_msg_cb_register("unhandled",NULL,unhandled_monitor);
   // lidbg_trace_msg_cb_register("lidbgerr", NULL, lidbgerr_monitor);

    lidbg_new_cdev(&misc_nod_fops, "lidbg_misc0");

    while(0 == g_var.android_boot_completed)
        ssleep(5);

    return 0;
}


static int __init lidbg_misc_init(void)
{
    DUMP_FUN;
    LIDBG_GET;
    CREATE_KTHREAD(misc_init, NULL);
    return 0;
}

static void __exit lidbg_misc_exit(void)
{
}

module_init(lidbg_misc_init);
module_exit(lidbg_misc_exit);

EXPORT_SYMBOL(lidbg_loop_warning);

EXPORT_SYMBOL(lidbg_system_switch);


MODULE_AUTHOR("futengfei");
MODULE_DESCRIPTION("misc zone");
MODULE_LICENSE("GPL");


