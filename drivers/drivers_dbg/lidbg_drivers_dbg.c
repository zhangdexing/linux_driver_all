
#include "lidbg.h"

void lidbg_video_main(int argc, char **argv);

LIDBG_DEFINE;

int drivers_dbg_open (struct inode *inode, struct file *filp)
{
    return 0;
}

#include "lidbg_drivers_cmd.c"

ssize_t drivers_dbg_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    parse_cmd(cmd_buf);
    return size;

}

static  struct file_operations drivers_dbg_nod_fops =
{
    .owner = THIS_MODULE,
    .write = drivers_dbg_write,
    .open = drivers_dbg_open,
};
int thread_drivers_dbg_init(void *data)
{
    fly_bare_data *mdata = bare_data_get("158019");
    lidbg_new_cdev(&drivers_dbg_nod_fops, "lidbg_drivers_dbg0");
	if(fs_is_file_exist("/data/coldBootLogcat.txt"))
	{
	    lidbg("enable coldBootLogcat\n");
	    lidbg_shell_cmd("echo appcmd *158#001 > /dev/lidbg_drivers_dbg0");
	}

    if( g_recovery_meg->bootParam.upName.val == 2&&mdata->bare_info.uart_dbg_en ==0)
    {
		lidbg("disable printk uart\n");
		lidbg_shell_cmd("echo appcmd *158#041 > /dev/lidbg_drivers_dbg0");
    }
	
   if((!g_var.is_fly) && (g_var.recovery_mode == 0) )
   {
   	    set_wifi_adb_mode(true);
		
	    if( !fs_is_file_exist("/system/app/GPS.apk"))
	    {
	        lidbg("enable system print\n");
	        g_recovery_meg->bootParam.upName.val = 1;
	        flyparameter_info_save(g_recovery_meg);

	        //lidbg("enable wifi adb\n");
	        //set_wifi_adb_mode(true);

	        //if(g_var.is_first_update)
	        {
	            char buff[50] = {0};
	            //lidbg_pm_install(get_lidbg_file_path(buff, "fileserver.apk"));
	            //lidbg_pm_install(get_lidbg_file_path(buff, "MobileRateFlow.apk"));
	            //lidbg_pm_install(get_lidbg_file_path(buff, "ES.ko"));

	            //lidbg_pm_install(get_lidbg_file_path(buff, "GPS.ko"));
	            //lidbg_pm_install(get_lidbg_file_path(buff, "FastBoot.apk"));
	            //lidbg_pm_install(get_lidbg_file_path(buff, "FlyBootService.apk"));
	            lidbg_shell_cmd("mount -o remount /system");

	            lidbg_shell_cmd("cp /system/lib/modules/out/fileserver.apk /system/app/fileserver.apk");
	            lidbg_shell_cmd("cp /system/lib/modules/out/MobileRateFlow.apk /system/app/MobileRateFlow.apk");
	            lidbg_shell_cmd("cp /system/lib/modules/out/ES.ko /system/app/ES.apk");
	            //lidbg_shell_cmd("cp /system/lib/modules/out/ST.ko /system/app/ST.apk");
	            lidbg_shell_cmd("cp /system/lib/modules/out/GPS.ko /system/app/GPS.apk");
#ifndef LIDBG_PM_MONITOR
	            if( !fs_is_file_exist("/system/app/FastBoot.apk"))
	                lidbg_shell_cmd("cp /system/lib/modules/out/FastBoot.apk /system/app/FastBoot.apk");

	            if( !fs_is_file_exist("/system/app/FlyBootService.apk"))
	                lidbg_shell_cmd("cp /system/lib/modules/out/FlyBootService.apk /system/app/FlyBootService.apk");
#endif
	            lidbg_shell_cmd("chmod 777 /system/app/*");
	            lidbg_shell_cmd("chmod 777 /data");
#if 0
	            lidbg_stop("adbd");//for baidu carlife
#endif
	            msleep(10 * 1000);
	            lidbg_pm_install(get_lidbg_file_path(buff, "ST.ko"));
	        }
	    }
   	}
#if 0
    msleep(60 * 1000);
    //else if (!strcmp(argv[1], "*158#025"))
    {
        LPC_CMD_ACC_SWITCH_START;
        lidbg_domineering_ack();
    }
#endif


    return 0;
}

int lidbg_drivers_dbg_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;

    CREATE_KTHREAD(thread_drivers_dbg_init, NULL);

    return 0;

}
void lidbg_drivers_dbg_deinit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");

module_init(lidbg_drivers_dbg_init);
module_exit(lidbg_drivers_dbg_deinit);


