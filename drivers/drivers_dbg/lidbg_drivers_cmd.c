
#define TEMP_LOG_PATH 	 LIDBG_LOG_DIR"log_ct.txt"
void  fake_acc_off(void);




void dump_sysinfo(void)
{
	lidbg_shell_cmd("chmod 777 /data/lidbg/*");
	lidbg_shell_cmd("mkdir -p /data/lidbg/machine");
	lidbg_shell_cmd("chmod 777 /data/lidbg/machine");
	lidbg_shell_cmd("date > /data/lidbg/machine/machine.txt");
	lidbg_shell_cmd("cat /proc/cmdline >> /data/lidbg/machine/machine.txt");
	lidbg_shell_cmd("getprop fly.version.mcu >> /data/lidbg/machine/machine.txt");
	lidbg_shell_cmd("getprop ro.release.version >> /data/lidbg/machine/machine.txt");
	lidbg_shell_cmd("top -n 1 -t -d 1 -m 25 >/data/lidbg/machine/top.txt");
	lidbg_shell_cmd("procrank > /data/lidbg/machine/procrank.txt");
	lidbg_shell_cmd("ps > /data/lidbg/machine/ps.txt");
	lidbg_shell_cmd("df > /data/lidbg/machine/df.txt");
	lidbg_shell_cmd("getprop > /data/lidbg/machine/getprop.txt");
	lidbg_shell_cmd("lsmod > /data/lidbg/machine/lsmod.txt");
	lidbg_shell_cmd("cat /proc/buddyinfo > /data/lidbg/machine/buddyinfo.txt");

	//power
	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/wakeup_sources.txt");
	lidbg_shell_cmd("cat /sys/kernel/debug/wakeup_sources >> /data/lidbg/pm_info/wakeup_sources.txt");
	lidbg_shell_cmd("cat /proc/wakelocks >> /data/lidbg/pm_info/wakeup_sources.txt");

	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/dumpsys_media.player.txt");
	lidbg_shell_cmd("dumpsys media.player >> /data/lidbg/pm_info/dumpsys_media.player.txt");
	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/dumpsys_power.txt");
	lidbg_shell_cmd("dumpsys power >> /data/lidbg/pm_info/dumpsys_power.txt");
	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/dumpsys_audio.txt");
	lidbg_shell_cmd("dumpsys audio >> /data/lidbg/pm_info/dumpsys_audio.txt");
	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/dumpsys_alarm.txt");
	lidbg_shell_cmd("dumpsys alarm >> /data/lidbg/pm_info/dumpsys_alarm.txt");
	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/location.txt");
	lidbg_shell_cmd("dumpsys location >> /data/lidbg/pm_info/location.txt");
	lidbg_shell_cmd("date  >> /data/lidbg/pm_info/meminfo.txt");
	lidbg_shell_cmd("dumpsys meminfo >> /data/lidbg/pm_info/meminfo.txt");
	lidbg_shell_cmd("date  > /data/lidbg/pm_info/iptable.txt");
	lidbg_shell_cmd("iptables -L  >> /data/lidbg/pm_info/iptable.txt");

	lidbg_shell_cmd("chmod 777 /data/lidbg/* -R");
	lidbg_shell_cmd("chmod 777 /data/lidbg/*");

}
int thread_log_temp(void *data)
{
    int tmp, cur_temp;
    while(1)
    {
        tmp = SOC_Get_CpuFreq();
        cur_temp = soc_temp_get(g_hw.cpu_sensor_num);
        lidbg_fs_log(TEMP_LOG_PATH,  "%d,%d\n", cur_temp, tmp);
        msleep(1000);
    }
}
void cb_kv_log_temp(char *key, char *value)
{
    CREATE_KTHREAD(thread_log_temp, NULL);
}


int thread_dumpsys_meminfo(void *data)
{
    lidbg_shell_cmd("rm /sdcard/meminfo.txt");
    lidbg_shell_cmd("rm /sdcard/ps.txt");
	
    ssleep(10);
    while(1)
    {
        if((fs_get_file_size("/sdcard/meminfo.txt")+fs_get_file_size("/sdcard/ps.txt")) < 100 * 1024 * 1024 )
        {
            fs_file_separator("/sdcard/meminfo.txt");
            lidbg_shell_cmd("dumpsys meminfo >>/sdcard/meminfo.txt &");
            lidbg_shell_cmd("date  >> /sdcard/ps.txt");				
            lidbg_shell_cmd("ps -t -m 10 >> /sdcard/ps.txt");
            lidbg("meminfo size:%d ps:%d\n", fs_get_file_size("/sdcard/meminfo.txt"), fs_get_file_size("/sdcard/ps.txt"));
        }
        else
        {
            lidbg("clear:meminfo size:%d ps:%d\n", fs_get_file_size("/sdcard/meminfo.txt"), fs_get_file_size("/sdcard/ps.txt"));
            lidbg_shell_cmd("echo 1 >/sdcard/meminfo.txt");
            lidbg_shell_cmd("echo 1 >/sdcard/ps.txt");
        }
        ssleep( 5 * 60 );
    }
}

int thread_format_sdcard1(void *data)
{
#ifdef PLATFORM_msm8996
    char *pdevices="/dev/block/mmcblk0p1";
#else
    char *pdevices="/dev/block/mmcblk1p1";
#endif
    lidbg_shell_cmd("echo ==thread_format_sdcard1.start==== > /dev/lidbg_msg");
    if(fs_is_file_exist(pdevices))
    {
        char shell_cmd[128] = {0};
        LIDBG_WARN("sdcard1.device:%s\n", pdevices);
        lidbg_shell_cmd("sync");
        lidbg_shell_cmd("rm -rf /storage/sdcard1/*");
		 lidbg_shell_cmd("echo ==thread_format_sdcard1.remove complete==== > /dev/lidbg_msg");
        sprintf(shell_cmd, "/flysystem/lib/out/busybox fdisk %s < /flysystem/lib/out/fdiskcmd.txt",  pdevices);
        lidbg_shell_cmd(shell_cmd);
        lidbg_shell_cmd("echo ==thread_format_sdcard1.fdiskcmd complete==== > /dev/lidbg_msg");
        sprintf(shell_cmd, "/flysystem/lib/out/busybox mkfs.vfat %s",  pdevices);
        lidbg_shell_cmd(shell_cmd);
        lidbg_shell_cmd("echo ==thread_format_sdcard1.mkfs.vfat complete==== > /dev/lidbg_msg");
        ssleep(3);
        lidbg_shell_cmd("am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 0");
        lidbg_shell_cmd("echo ==thread_format_sdcard1.system format.20S==== > /dev/lidbg_msg");
        ssleep(22);
        lidbg_shell_cmd("am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 5");
        lidbg_shell_cmd("echo ==thread_format_sdcard1.system mount sdcard1.5S==== > /dev/lidbg_msg");
        ssleep(5);
        lidbg_shell_cmd("echo \"formatcomplete=1\" > /dev/lidbg_flycam0&");
    }
    else
    {
        LIDBG_WARN("sdcard1.device:NULL");
		lidbg_shell_cmd("echo \"formatcomplete=0\" > /dev/lidbg_flycam0&");
    }
    lidbg_shell_cmd("echo ==thread_format_sdcard1.stop==== > /dev/lidbg_msg");
    return 0;
}

int thread_format_udisk(void *data)
{
    struct mounted_volume *udisk = NULL;
    lidbg_shell_cmd("echo ==thread_format_udisk.start==== > /dev/lidbg_msg");
    udisk = find_mounted_volume_by_mount_point("/mnt/media_rw/udisk") ;
    if(udisk != NULL)
    {
        char shell_cmd[128] = {0};
        LIDBG_WARN("udisk.device:%s\n", udisk->device);
        sprintf(shell_cmd, "/flysystem/lib/out/busybox fdisk %s < /flysystem/lib/out/fdiskcmd.txt",  udisk->device);
        lidbg_shell_cmd(shell_cmd);
        sprintf(shell_cmd, "/flysystem/lib/out/busybox mkfs.vfat %s",  udisk->device);
        lidbg_shell_cmd(shell_cmd);
        lidbg_shell_cmd("sync");
        ssleep(5);
        lidbg_shell_cmd("echo appcmd *158#052 > /dev/lidbg_drivers_dbg0");
    }
    else
    {
        LIDBG_WARN("udisk.device:NULL");
    }
    lidbg_shell_cmd("echo ==thread_format_udisk.stop==== > /dev/lidbg_msg");
    return 0;
}

int thread_trigge_SD_uevent(void *data)
{
    LIDBG_WARN("thread_trigge_SD_uevent");
    lidbg_shell_cmd("echo remove > /sys/block/mmcblk0/uevent");
    lidbg_shell_cmd("echo remove > /sys/block/mmcblk1/uevent");
    lidbg_shell_cmd("echo add > /sys/block/mmcblk0/uevent");
    lidbg_shell_cmd("echo add > /sys/block/mmcblk1/uevent");
    return 0;
}

int thread_antutu_test(void *data)
{
    int cnt = 0;
    ssleep(50);
#ifdef SOC_msm8x26
    set_system_performance(1);
#endif

    while(1)
    {
        cnt++;
        lidbg_fs_log(TEMP_LOG_PATH, "antutu test start: %d\n", cnt);

        //lidbg_shell_cmd("pm uninstall com.antutu.ABenchMark");
        //lidbg_pm_install("/data/antutu.apk");
        //ssleep(5);

        lidbg_shell_cmd("am start -n com.antutu.ABenchMark/com.antutu.ABenchMark.ABenchMarkStart");
        ssleep(5);
        lidbg_shell_cmd("am start -n com.antutu.ABenchMark/com.antutu.benchmark.activity.ScoreBenchActivity");
        ssleep(60 * 5); // 4 min loop
    }

}
bool set_wifi_adb_mode(bool on)
{
    LIDBG_WARN("<%d>\n", on);
    if(on)
        lidbg_setprop("service.adb.tcp.port", "5555");
    else
        lidbg_setprop("service.adb.tcp.port", "-1");
    lidbg_stop("adbd");
    lidbg_start("adbd");
    return true;
}
int thread_dump_log_cp2_udisk(void *data)
{
	dump_sysinfo();
	ssleep(10);
	lidbg_shell_cmd("am start -n com.mypftf.android.BugReport/.MainActivity &");
	ssleep(50);
//	lidbg_shell_cmd("screencap -p /data/lidbg/screenshot.png &");
	ssleep(2);
#ifdef SOC_msm8x25
    fs_cp_data_to_udisk(true);
#else
{
    char shell_cmd[128] = {0}, tbuff[128] = {0};
    lidbg_get_current_time(tbuff, NULL);
    sprintf(shell_cmd, "mkdir /sdcard/ID-%d-%s", get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);
    sprintf(shell_cmd, "cp -rf "LIDBG_LOG_DIR"* /sdcard/ID-%d-%s", get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);
    sprintf(shell_cmd, "cp -rf /sdcard/logcat*.txt /sdcard/ID-%d-%s", get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);
    sprintf(shell_cmd, "cp -rf /sdcard/kmsg*.txt /sdcard/ID-%d-%s", get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);
    sprintf(shell_cmd, "cp -rf /data/anr /sdcard/ID-%d-%s", get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);
    sprintf(shell_cmd, "cp -rf /data/tombstones /sdcard/ID-%d-%s", get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);

//for udisk
    sprintf(shell_cmd, "cp -rf /sdcard/ID-%d-%s "USB_MOUNT_POINT, get_machine_id() , tbuff);
    lidbg_shell_cmd(shell_cmd);

    ssleep(10);
    lidbg_shell_cmd("sync");
    ssleep(1);
}
#endif
    lidbg_domineering_ack();
    lidbg_toast_show("158013--", "可拔掉U盘,调试信息以ID开头");
    lidbg_shell_cmd("am start -n com.mypftf.android.BugReport/.MainActivity &");
    ssleep(60);
    lidbg_shell_cmd("pm uninstall com.mypftf.android.BugReport");
    return 0;
}
static bool logcat_enabled = false;
int thread_enable_logcat(void *data)
{
    if(logcat_enabled)
        goto out;
    logcat_enabled = true;
    lidbg_enable_logcat();
out:
    lidbg("logcat.skip\n");
    return 0;
}
int thread_enable_logcat2(void *data)
{
    if(logcat_enabled)
        goto out;
    logcat_enabled = true;
    lidbg_enable_logcat2();
out:
    lidbg("logcat.skip\n");
    return 0;
}

int thread_system_trace(void *data)
{
    lidbg_shell_cmd("top -t -m 15 >> /sdcard/logcat.txt &");

    while(1)
    {
        lidbg_shell_cmd("procrank -u >> /sdcard/logcat.txt &");
        ssleep(5);
        lidbg_shell_cmd("dumpsys meminfo >> /sdcard/logcat.txt &");
        ssleep(5);
    }

    return 0;
}


static bool start_stop_third_apk_enabled = false;
int thread_start_stop_all_third_apk(void *data)
{
    int cnt = 0;
    while(start_stop_third_apk_enabled)
    {
        cnt++;
        LIDBG_WARN("--->%d\n", cnt);
        lidbg_shell_cmd("echo appcmd *158#087 > /dev/lidbg_drivers_dbg0");
        ssleep(2 * 60);
        lidbg_shell_cmd("echo appcmd *158#086 > /dev/lidbg_drivers_dbg0");
        ssleep(30);
    }
    lidbg("start_stop_third_apk_enabled.exit\n");
    return 0;
}

static bool top_enabled = false;
int thread_enable_top(void *data)
{
    int size, sizeold = 0;
    if(top_enabled)
        goto out;
    top_enabled = true;
    lidbg("top+\n");
    lidbg_shell_cmd("rm /sdcard/top.txt");
    lidbg_shell_cmd("rm /sdcard/top_old.txt");
    ssleep(2);

    lidbg_shell_cmd("date >/sdcard/top.txt");
    ssleep(1);
    lidbg_shell_cmd("chmod 777 /sdcard/top.txt");
    ssleep(1);
    lidbg_shell_cmd("top -t -m 10 > /sdcard/top.txt &");
    while(1)
    {
        size = fs_get_file_size("/sdcard/top.txt") ;
        if(size >= MEM_SIZE_1_MB * 300)
        {
            lidbg("file_len over\n");
            lidbg_shell_cmd("rm /sdcard/top_old.txt");
            ssleep(1);
            lidbg_shell_cmd("cp -rf /sdcard/top.txt /sdcard/top_old.txt");
            ssleep(5);
            lidbg_shell_cmd("date > /sdcard/top.txt");
            ssleep(1);
            lidbg_shell_cmd("chmod 777 /sdcard/top.txt");
        }
        ssleep(60);
        if(size == sizeold)
        {
            lidbg_shell_cmd("top -t -m 10 > /sdcard/top.txt &");
            lidbg("run top again \n");
        }
        sizeold = size ;

    }
    lidbg("top-\n");
out:
    lidbg("top.skip\n");
    return 0;
}
static bool dmesg_enabled = false;
int thread_enable_dmesg(void *data)
{
    if(dmesg_enabled)
        goto out;
    dmesg_enabled = true;
    lidbg_enable_kmsg();
out:
    lidbg("kmsg.skip\n");
    return 0;
}

int thread_screenshot(void *data)
{
    SOC_Key_Report(KEY_POWER, KEY_PRESSED);
    SOC_Key_Report(KEY_VOLUMEDOWN, KEY_PRESSED);
    msleep(3000);
    SOC_Key_Report(KEY_POWER, KEY_RELEASED);
    SOC_Key_Report(KEY_VOLUMEDOWN, KEY_RELEASED);
    return 0;
}

int thread_kmsg_fifo_save(void *data)
{
    //kmsg_fifo_save();
    return 0;
}

int thread_monkey_test(void *data)
{
    u32 loop = 0;
    lidbg("monkey test start !\n");
    while(1)
    {
        if(te_is_ts_touched())
        {
            lidbg_domineering_ack();
            lidbg("thread_monkey_test:te_is_ts_touched.pause\n");
            ssleep(60);
            continue;
        }
        lidbg("monkey loop = %d\n", loop);
        loop++;
        lidbg_shell_cmd("monkey --ignore-crashes --ignore-timeouts --throttle 300 500 &");
        msleep(60 * 1000);
    }
    lidbg("thread_monkey_test end\n");
    return 0;
}

irqreturn_t TEST_isr(int irq, void *dev_id)
{
    lidbg("TEST_isr================%d ", irq);
    return IRQ_HANDLED;
}

void callback_func_test_readdir(char *dirname, char *filename)
{
    LIDBG_WARN("%s<---%s\n", dirname, filename);
}

static bool fan_enable = false;
static bool cmd_enable = true;
void parse_cmd(char *pt)
{
    int argc = 0;
    int i = 0;

    char *argv[32] = {NULL};
    argc = lidbg_token_string(pt, " ", argv);

    lidbg("cmd:");
    while(i < argc)
    {
        printk(KERN_CRIT"%s ", argv[i]);
        i++;
    }
    lidbg("\n");

    if (!strcmp(argv[0], "appcmd")&&cmd_enable)
    {
        lidbg("%s:[%s]\n", argv[0], argv[1]);
        lidbg_chmod("/data");
        lidbg_chmod("/data/lidbg");
        lidbg_shell_cmd("chmod 777 /data/lidbg/*");
        if (!strcmp(argv[1], "*158#000"))
        {
            //*#*#158999#*#*
            lidbg_chmod("/data");
            lidbg_chmod("/data/lidbg");
	     lidbg_shell_cmd("chmod 777 /data/lidbg/*");
            fs_mem_log("*158#998--install third apk\n");
            fs_mem_log("*158#999--install debug apk\n");
            fs_mem_log("*158#001--LOG_LOGCAT\n");
            fs_mem_log("*158#002--LOG_DMESG\n");
            fs_mem_log("*158#003--LOG_CLEAR_LOGCAT_KMSG\n");
            fs_mem_log("*158#004--LOG_SHELL_TOP_DF_PS\n");
            fs_mem_log("*158#010--USB_ID_LOW_HOST\n");
            fs_mem_log("*158#011--USB_ID_HIGH_DEV\n");
            fs_mem_log("*158#012--lidbg_trace_msg_disable\n");
            fs_mem_log("*158#013--dump log and copy to udisk\n");
            fs_mem_log("*158#014--origin system\n");
            fs_mem_log("*158#015--flyaudio system\n");
            fs_mem_log("*158#016--enable wifi adb\n");//adb connect ip
            fs_mem_log("*158#017--disable wifi adb\n");
            fs_mem_log("*158#018--origin gps\n");
            fs_mem_log("*158#019--enable system print\n");
            fs_mem_log("*158#020--disable system print\n");
            fs_mem_log("*158#021--save fifo msg\n");
            fs_mem_log("*158#022--log2sd to save qxdm\n");
            fs_mem_log("*158#023--show cpu temp\n");
            fs_mem_log("*158#024--!fan enalbe\n");
            fs_mem_log("*158#025--LPC_CMD_ACC_SWITCH_START\n");
            fs_mem_log("*158#026--clear acc history\n");
            fs_mem_log("*158#027--antutu auto test\n");
            fs_mem_log("*158#028--delete ublox so && reboot\n");
            fs_mem_log("*158#029--log cpu temp\n");
            fs_mem_log("*158#030--cpu top performance mode\n");
            fs_mem_log("*158#031--pr_debug GPS_val\n");
            fs_mem_log("*158#032--pr_debug AD_val\n");
            fs_mem_log("*158#033--pr_debug TS_val\n");
            fs_mem_log("*158#034--pr_debug cpu_temp\n");
            fs_mem_log("*158#035--pr_debug lowmemorykillprotecter\n");
            fs_mem_log("*158#040--monkey test\n");
            fs_mem_log("*158#041--disable uart debug\n");
            fs_mem_log("*158#042--disable adb\n");
            fs_mem_log("*158#043--enable adb\n");
            fs_mem_log("*158#044x--start SleepTest acc test,????,?*158#0448010\n");
            fs_mem_log("*158#045x--start RGB LED test,????,?*158#0451\n");
            fs_mem_log("*158#046--set cpu run in performance mode\n");
            fs_mem_log("*158#047--set cpu run in powersave mode\n");
            fs_mem_log("*158#048--flybootserver airplane disable\n");
            fs_mem_log("*158#049--flybootserver airplane enable\n");
            fs_mem_log("*158#050--enable top -t -m 10\n");
            fs_mem_log("*158#051--LOG_LOGCAT2\n");
            fs_mem_log("*158#052--udisk reset\n");
            fs_mem_log("*158#053--system trace\n");
            fs_mem_log("*158#054--uvccam recording control(1 or 0)\n");
            fs_mem_log("*158#055--disable logcat server\n");
            fs_mem_log("*158#056--ensable logcat server\n");
            fs_mem_log("*158#057--do fake acc off\n");
            fs_mem_log("*158#058--FlyaudioWhiteListInternetEnable(true)\n");
            fs_mem_log("*158#059--FlyaudioWhiteListInternetEnable(false)\n");
            fs_mem_log("*158#060--enable iptable action \n");
            fs_mem_log("*158#061--disable iptable action \n");
            fs_mem_log("*158#062--do not kill any process in flybootserver.apk\n");
            fs_mem_log("*158#063--disable all app's internet\n");
            fs_mem_log("*158#064--enable all app's internet\n");
            fs_mem_log("*158#065--disable suspend timeout protect\n");
            fs_mem_log("*158#066--disable alarmmanager protect\n");
            fs_mem_log("*158#067xx--set alarmtimer wakeup time\n");
            fs_mem_log("*158#068----update firmware for usb camera\n");
            fs_mem_log("*158#069----shut BT power\n");
            fs_mem_log("*158#070xx--set goto sleep time\n");
            fs_mem_log("*158#071--udisk stable test start/stop\n");
            fs_mem_log("*158#072--acc on/off udisk stable test\n");
            fs_mem_log("*158#073--log kmsg no screen flash\n");
            fs_mem_log("*158#074--disable cn.flyaudio.media\n");
            fs_mem_log("*158#075--enable crash detect & debug by gsensor\n");
            fs_mem_log("*158#076--enable gsensor data for android\n");
            fs_mem_log("*158#077--dumpsys meminfo\n");
            fs_mem_log("*158#078--test seven day timeout :will reboot at tomorow 6:00,Reset once per hour\n");
            fs_mem_log("*158#079--open binder debug\n");
            fs_mem_log("*158#080--disable third part apk\n");
            fs_mem_log("*158#081--enable third part apk\n");
            fs_mem_log("*158#082--nothing\n");
            fs_mem_log("*158#083--nothing\n");
            fs_mem_log("*158#084--nothing\n");
            fs_mem_log("*158#085--nothing\n");
            fs_mem_log("*158#086--origin suspend\n");
            fs_mem_log("*158#087--keep lcd on\n");
            fs_mem_log("*158#088--force stop third part apk\n");
            fs_mem_log("*158#089--start all third part apk\n");
            fs_mem_log("*158#090--start then stop all third part apk\n");
            fs_mem_log("*158#091--rm /data/out\n");
            fs_mem_log("*158#092--enable 158013 then upload the log to internet\n");
            fs_mem_log("*158#093--rm all log created by 158013\n");
            fs_mem_log("*158#094--rm flysystem/lib/hw and flysystem/lib/modules\n");
            fs_mem_log("*158#095--recovery flysystem/lib/hw and flysystem/lib/modules\n");
            fs_mem_log("*158#096--read camera FW version\n");
            fs_mem_log("*158#097--format SDCARD1\n");
            fs_mem_log("*158#098--format udisk\n");
            fs_mem_log("*158#099--adust Gsensor Sensitivity \n");
            fs_mem_log("*158#100--dump log for app upload\n");
            fs_mem_log("*158#101--trigge_SD_uevent\n");
            fs_mem_log("*158#102--copy /persist/display/* to udisk\n");
            fs_mem_log("*158#103--copy udisk QDCM.apk to system/app\n");
            fs_mem_log("*158#104--save gps raw data to sdcard\n");
            fs_mem_log("*158#105--MTK USB HOST MODE\n");
            fs_mem_log("*158#106--MTK USB SLAVE MODE\n");
            fs_mem_log("*158#107--android display Vertical screen\n");
            fs_mem_log("*158#108--android display Normal screen\n");
            fs_mem_log("*158#109--en qualcomm display tun tools \n");
            fs_mem_log("*158#110xxx--set navi policy music level 0~100\n");
            fs_mem_log("*158#111--revert is_debug_mode\n");
            fs_mem_log("*158#112--launch MTK settings \n");
            fs_mem_log("*158#113--revert sound debug \n");
            fs_mem_log("*158#114x--use shell cmd dd to test emmc extern SD udisk Stability \n");
            fs_mem_log("*158#115--auto update fup and auto install then open the red osd \n");
            fs_mem_log("*158#116--factory reset and auto install then open the red osd \n");
            fs_mem_log("*158#117--get qcom band :input just once ,when band changed ,it will tell you though toast \n");
            fs_mem_log("*158#118--enable navi stream policy \n");
            fs_mem_log("*158#119--disable navi stream policy \n");
            fs_mem_log("*158#120--enable uart print when lpm comes \n");
            fs_mem_log("*158#121--auto input test \n");
            fs_mem_log("*158#122--delete logcat black list conf \n");
            fs_mem_log("*158#123--gsensor debug switch \n");
            fs_mem_log("*158#124--enable logcat when coldboot \n");
            fs_mem_log("*158#125--enable kmsg print \n");
            fs_mem_log("*158#126--cp logcat kmsg to Udisk \n");
            fs_mem_log("*158#127--grantWhiteListPermissions \n");
            fs_mem_log("*158#128--trigger conf check \n");
            fs_mem_log("*158#129--call lidbg_uevent_cold_boot \n");
            fs_mem_log("*158#130--enable red osd ,logcat \n");
            fs_mem_log("*158#131--get kmsg log in time mode \n");

            lidbg_shell_cmd("chmod 777 /data/lidbg/ -R");
            show_password_list();
            lidbg_domineering_ack();
        }
        if (!strcmp(argv[1], "*158#998"))
        {
            char buff[50] = {0};
            lidbg_pm_install(get_lidbg_file_path(buff, "ES.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "ST.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "GPS.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "camera4hal.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "mobileTrafficstats.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "LiveSessionDemo.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "CallMessage.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "sslcapture.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "Firewall.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "app4haljni.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "MediaPlayerTest.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "GpsInfo.ko"));
            lidbg_pm_install(get_lidbg_file_path(buff, "setting.ko"));
            lidbg_domineering_ack();
        }
        if (!strcmp(argv[1], "*158#999"))
        {
            char buff[50] = {0};
            lidbg_shell_cmd("setenforce 0");
            lidbg_pm_install(get_lidbg_file_path(buff, "fileserver.apk"));
            lidbg_pm_install(get_lidbg_file_path(buff, "MobileRateFlow.apk"));
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#001"))
        {
            lidbg_chmod("/sdcard");
            CREATE_KTHREAD(thread_enable_logcat2, NULL);
	     lidbg_shell_cmd("ps -t > /data/ps.txt");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#002"))
        {
 #if 0
            lidbg_chmod("/data");
#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_DMESG);
#else
            CREATE_KTHREAD(thread_enable_dmesg, NULL);
#endif
	    if(g_var.is_fly)
           	 lidbg_shell_cmd("/flysystem/lib/out/sendsignal STORE&");
           else
               lidbg_shell_cmd("/system/lib/modules/out/sendsignal STORE &");

		 lidbg_shell_cmd("chmod 777 /data/lidbg/reckmsg/* ");
#endif
	    if(g_var.is_fly)
	    	{
		 lidbg_shell_cmd("/flysystem/lib/out/sendsignal STORE &");
           	 lidbg_shell_cmd("/flysystem/lib/out/sendsignal KILL &");
	    	}
           else
           	{
               lidbg_shell_cmd("/system/lib/modules/out/sendsignal STORE &");
               lidbg_shell_cmd("/system/lib/modules/out/sendsignal KILL &");
           	}
		   
	    msleep(500);
	    if(g_var.is_fly)
           	 lidbg_shell_cmd("/flysystem/lib/out/record_klogctl 1 &");
           else
               lidbg_shell_cmd("/system/lib/modules/out/record_klogctl 1 &");

		   
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#073"))
        {
            lidbg_chmod("/data");
	    if(g_var.is_fly)
           	 lidbg_shell_cmd("/flysystem/lib/out/sendsignal STORE_IN_TIME &");
           else
               lidbg_shell_cmd("/system/lib/modules/out/sendsignal STORE_IN_TIME &");
        }
        else if (!strcmp(argv[1], "*158#003"))
        {
#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_CLEAR_LOGCAT_KMSG);
#else
            lidbg("clear+logcat*&&kmsg*\n");
#ifdef SOC_mt3360
            lidbg_shell_cmd("rm /sdcard/logcat*");
            lidbg_shell_cmd("rm /sdcard/kmsg*");
            logcat_enabled = false;
#else
            lidbg_shell_cmd("rm /data/logcat*");
            lidbg_shell_cmd("rm /data/kmsg*");
#endif
            lidbg_shell_cmd("echo appcmd *158#093 > /dev/lidbg_drivers_dbg0");
            lidbg("clear-logcat*&&kmsg*,IDfile\n");
#endif
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#004"))
        {
#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_SHELL_TOP_DF_PS);
#else
            lidbg("\n\nLOG_SHELL_TOP_DF_PS+\n");
            lidbg_shell_cmd("date > /data/machine.txt");
            lidbg_shell_cmd("cat /proc/cmdline >> /data/machine.txt");
            lidbg_shell_cmd("getprop fly.version.mcu >> /data/machine.txt");
            lidbg_shell_cmd("top -n 3 -t >/data/top.txt &");
            lidbg_shell_cmd("screencap -p /data/screenshot.png &");
            lidbg_shell_cmd("ps > /data/ps.txt");
            lidbg_shell_cmd("df > /data/df.txt");
            lidbg_shell_cmd("lsmod > /data/lsmod.txt");
            lidbg_shell_cmd("chmod 777 /data/*.txt");
            lidbg_shell_cmd("chmod 777 /data/*.png");
            lidbg("\n\nLOG_SHELL_TOP_DF_PS-\n");
#endif
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#010"))
        {
            lidbg("USB_ID_LOW_HOST\n");
            USB_ID_LOW_HOST;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#011"))
        {
            lidbg("USB_ID_HIGH_DEV\n");
            USB_ID_HIGH_DEV;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#012"))
        {
           // lidbg_trace_msg_disable(1);
        }
        else if (!strcmp(argv[1], "*158#013"))
        	{
            lidbg_toast_show("158013--", "确认插入U盘并等待下条提醒,大约60S...");
            lidbg_shell_cmd("echo appcmd *158#001 > /dev/lidbg_drivers_dbg0");
            lidbg_shell_cmd("echo appcmd *158#021 > /dev/lidbg_drivers_dbg0");
            CREATE_KTHREAD(thread_dump_log_cp2_udisk, NULL);
        }
        else if (!strcmp(argv[1], "*158#014"))
            lidbg_system_switch(true);
        else if (!strcmp(argv[1], "*158#015"))
            lidbg_system_switch(false);
        else if (!strcmp(argv[1], "*158#016"))
        	{
            lidbg_shell_cmd("echo appcmd *158#012 > /dev/lidbg_drivers_dbg0");
            set_wifi_adb_mode(true);
        	}
        else if (!strcmp(argv[1], "*158#017"))
            set_wifi_adb_mode(false);
        else if (!strcmp(argv[1], "*158#018"))
        {
            lidbg_shell_cmd("mount -o remount /system");
            lidbg_shell_cmd("mount -o remount /flysystem");
            lidbg_shell_cmd("rm /flysystem/lib/out/"FLY_GPS_SO);
            lidbg_shell_cmd("rm /system/lib/modules/out/"FLY_GPS_SO);
            lidbg_shell_cmd("rm /flysystem/lib/hw/"FLY_GPS_SO);
            lidbg_domineering_ack();
            msleep(3000);
            lidbg_reboot();
        }
        else if (!strcmp(argv[1], "*158#019"))
        {
            g_recovery_meg->bootParam.upName.val = 1;
            if(flyparameter_info_save(g_recovery_meg))
            {
                lidbg_domineering_ack();
                msleep(3000);
                lidbg_reboot();
            }
        }
        else if (!strcmp(argv[1], "*158#020"))
        {
            g_recovery_meg->bootParam.upName.val = 2;
            if(flyparameter_info_save(g_recovery_meg))
            {
                lidbg_domineering_ack();
                msleep(3000);
                lidbg_reboot();
            }
        }
        else if (!strcmp(argv[1], "*158#021"))
        {
            lidbg_chmod("/data");
            lidbg_fifo_get(glidbg_msg_fifo, LIDBG_LOG_DIR"lidbg_mem_log.txt", 0);
            CREATE_KTHREAD(thread_kmsg_fifo_save, NULL);
	     lidbg_shell_cmd("chmod 777 /data/lidbg/*");
	    if(g_var.is_fly)
           	 lidbg_shell_cmd("/flysystem/lib/out/sendsignal STORE&");
           else
               lidbg_shell_cmd("/system/lib/modules/out/sendsignal STORE &");
		 
	     lidbg_shell_cmd("chmod 777 /data/lidbg/reckmsg/* ");
            lidbg_shell_cmd("chmod 777 /data/lidbg/ -R");
           // lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#022"))
        {
            lidbg_shell_cmd("mount -o remount /system");
            lidbg_shell_cmd("mkdir /sdcard/diag_logs");
            lidbg_shell_cmd("chmod 777 /sdcard/diag_logs");
            lidbg_shell_cmd("cp /flysystem/lib/out/DIAG.conf /sdcard/diag_logs/DIAG.cfg");
            lidbg_shell_cmd("chmod 777 /sdcard/diag_logs/DIAG.cfg");
            lidbg_shell_cmd("/system/bin/diag_mdlog &");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#023"))
        {
            // cb_kv_show_temp(NULL, NULL);
            // lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#024"))
        {
            fan_enable = !fan_enable;
            if(fan_enable)
                LPC_CMD_FAN_ON;
            else
                LPC_CMD_FAN_OFF;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#025"))
        {
            LPC_CMD_ACC_SWITCH_START;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#026"))
        {
            fs_file_write2("/dev/lidbg_pm0", "flyaudio acc_history");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#027"))
        {
            CREATE_KTHREAD(thread_antutu_test, NULL);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#028"))
        {
            lidbg_shell_cmd("mount -o remount /flysystem");
            lidbg_shell_cmd("rm /flysystem/lib/out/"FLY_GPS_SO);
            lidbg_shell_cmd("mount -o remount,ro /flysystem");
            lidbg_domineering_ack();
            msleep(3000);
            lidbg_reboot();
        }
        else if (!strcmp(argv[1], "*158#029"))
        {
            cb_kv_log_temp(NULL, NULL);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#030"))
        {
#ifdef SOC_msm8x26
            set_system_performance(3);
            lidbg_domineering_ack();
#endif
        }
        else if (!strcmp(argv[1], "*158#031"))
        {
            lidbg("gps_debug\n");
            lidbg_shell_cmd("echo -n 'file lidbg_gps.c +p' > /sys/kernel/debug/dynamic_debug/control");
        }
        else if (!strcmp(argv[1], "*158#032"))
        {
            lidbg("AD_debug\n");
            lidbg_shell_cmd("echo -n 'file lidbg_ad_msm8x26.c +p' > /sys/kernel/debug/dynamic_debug/control");
        }
        else if (!strcmp(argv[1], "*158#033"))
        {
            lidbg("ts_debug\n");
            lidbg_shell_cmd("echo -n 'file lidbg_ts.c +p' > /sys/kernel/debug/dynamic_debug/control");
            lidbg_shell_cmd("echo -n 'file lidbg_ts_probe_new.c +p' > /sys/kernel/debug/dynamic_debug/control");
        }
        else if (!strcmp(argv[1], "*158#034"))
        {
            lidbg("temp_debug\n");
            lidbg_shell_cmd("echo -n 'file lidbg_temp.c +p' > /sys/kernel/debug/dynamic_debug/control");
        }
        else if (!strcmp(argv[1], "*158#035"))
        {
            lidbg("lowmemorykill debug\n");
            lidbg_shell_cmd("echo -n 'file lowmemorykillprotecter.c +p' > /sys/kernel/debug/dynamic_debug/control");
        }
        else if (!strcmp(argv[1], "*158#040"))
        {
            CREATE_KTHREAD(thread_monkey_test, NULL);
        }
        else if (!strcmp(argv[1], "*158#041"))
        {
            lidbg_shell_cmd("echo 0 > /proc/sys/kernel/printk");
        }
        else if (!strcmp(argv[1], "*158#042"))
        {
            lidbg("disable adb\n");
            lidbg_stop("adbd");
        }
        else if (!strcmp(argv[1], "*158#043"))
        {
            lidbg("enable adb\n");
            lidbg_start("adbd");
        }
        else if (!strncmp(argv[1], "*158#044", 8))
        {
            //????,?*158#0448010,??????ACC?????,???70S,?????ACC?????,????????80,10S??
            char s[100];
            int n;
            char buff[50] = {0};
            n = strlen(argv[1]);


            lidbg_pm_install(get_lidbg_file_path(buff, "SleepTest.apk"));
            lidbg_pm_install(get_lidbg_file_path(buff, "SleepTest/SleepTest.apk"));
            msleep(5000);

            if(n != 12)
                strcpy(argv[1], "*158#0448010");

            lidbg("start SleepTest acc test %s\n", (argv[1] + 8));
            sprintf(s, "am start -n com.example.sleeptest/com.example.sleeptest.SleepTest --ei time %s", (argv[1] + 8));
            lidbg_shell_cmd(s);
            lidbg("cmd : %s", s);
        }
        else if (!strncmp(argv[1], "*158#045", 8))
        {
            //opt args,ex:*158#0450
            int n;
            n = strlen(argv[1]);
            if(n != 9)//wrong args
            {
                lidbg("wrong args!");
                return;
            }
            lidbg("--------RGB_LED MODE:%s-----------", argv[1] + 8);
            if(!strcmp((argv[1] + 8), "1"))
                fs_file_write2("/dev/lidbg_rgb_led0", "rgb 255 0 0");
            else if(!strcmp((argv[1] + 8), "2"))
                fs_file_write2("/dev/lidbg_rgb_led0", "init");
            else if(!strcmp((argv[1] + 8), "3"))
                fs_file_write2("/dev/lidbg_rgb_led0", "stop");
            else if(!strcmp((argv[1] + 8), "4"))
                fs_file_write2("/dev/lidbg_rgb_led0", "reset");
            else if(!strcmp((argv[1] + 8), "5"))
                fs_file_write2("/dev/lidbg_rgb_led0", "play");
        }
        else if (!strcmp(argv[1], "*158#046"))
        {
            lidbg("*158#046--set cpu run in performance mode\n");
#ifdef SOC_mt35x
            lidbg_shell_cmd("echo 0 > /proc/hps/enabled");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu0/online");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu1/online");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu2/online");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu3/online");
	    lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu4/online");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu5/online");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu6/online");
            lidbg_shell_cmd("echo 1 > /sys/devices/system/cpu/cpu7/online");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 1248000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 1248000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 1248000 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 1248000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq");
#else
            set_cpu_governor(1);
#endif
        }
        else if (!strcmp(argv[1], "*158#047"))
        {
            lidbg("*158#047--set cpu run in powersave mode\n");
#ifdef SOC_mt35x
            lidbg_shell_cmd("echo 1 > /proc/hps/enabled");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 637000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 637000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 637000 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("echo 637000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq");
            lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq");
#else
            set_cpu_governor(0);
#endif
        }
        else if (!strcmp(argv[1], "*158#048"))
        {
            lidbg("**********set AirplaneEnable false**********\n");
            lidbg_shell_cmd("setprop persist.lidbg.AirplaneEnable 0");
        }
        else if (!strcmp(argv[1], "*158#049"))
        {
            lidbg("**********set AirplaneEnable true**********\n");
            lidbg_shell_cmd("setprop persist.lidbg.AirplaneEnable 1");
        }
        else if (!strcmp(argv[1], "*158#050"))
        {
            lidbg_chmod("/sdcard");
            CREATE_KTHREAD(thread_enable_top, NULL);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#051"))
        {
            lidbg_chmod("/data");
#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_LOGCAT);
#else
            CREATE_KTHREAD(thread_enable_logcat, NULL);
#endif
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#052"))
        {
            lidbg("-------udisk reset -----");
            fs_file_write2("/dev/flydev0", "udisk_reset");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#053"))
        {
            lidbg_chmod("/sdcard");
            CREATE_KTHREAD(thread_system_trace, NULL);
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#054", 8))
        {
            //opt args,ex:*158#0540
            int n;
            n = strlen(argv[1]);
            if(n != 9)//wrong args
            {
                lidbg("wrong args!");
                return;
            }
            lidbg("--------UVCCAM MODE:%s-----------", argv[1] + 8);
            if(!strcmp((argv[1] + 8), "1"))//start recording
            {
                lidbg("-------uvccam recording -----");
                lidbg_shell_cmd("setprop lidbg.uvccam.dvr.recording 1");
                if(g_var.is_fly) lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video2 -b 1 -c -f H264 -r &");
                else lidbg_shell_cmd("./system/lib/modules/out/lidbg_testuvccam /dev/video2 -b 1 -c -f H264 -r &");
            }
            else if(!strcmp((argv[1] + 8), "0"))//stop recording
            {
                lidbg("-------uvccam stop_recording -----");
                lidbg_shell_cmd("setprop lidbg.uvccam.dvr.recording 0");
            }

        }
        else if (!strcmp(argv[1], "*158#055"))
        {
            lidbg("-------disable logcat server -----");
            lidbg_shell_cmd("setprop ctl.stop logd");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#056"))
        {
            lidbg("-------enable logcat server -----");
            lidbg_shell_cmd("setprop ctl.start logd");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#057"))
        {
#ifdef SUSPEND_ONLINE
            lidbg("-------fake_acc_off -----");
            fake_acc_off();
#endif
        }
        else if (!strcmp(argv[1], "*158#058"))
        {
            lidbg("*158#058--FlyaudioWhiteListInternetEnable(true)\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 5 &");
        }
        else if (!strcmp(argv[1], "*158#059"))
        {
            lidbg("*158#059--FlyaudioWhiteListInternetEnable(false)\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 4 &");
        }
        else if (!strcmp(argv[1], "*158#060"))
        {
            lidbg("*158#060--enable iptable action \n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 7 &");
        }
        else if (!strcmp(argv[1], "*158#061"))
        {
            lidbg("*158#061--disable iptable action \n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 6 &");
        }
        else if (!strcmp(argv[1], "*158#062"))
        {
            lidbg("*158#062--do not kill any process in flybootserver.apk\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 8 &");
        }
        else if (!strcmp(argv[1], "*158#063"))
        {
            lidbg("*158#063--disable all app's internet\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 9 &");
        }
        else if (!strcmp(argv[1], "*158#064"))
        {
            lidbg("*158#064--enable all app's internet\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 10 &");
        }
        else if (!strcmp(argv[1], "*158#065"))
        {
            lidbg("disable suspend timeout protect\n");
            g_var.suspend_timeout_protect = 0;
        }
        else if (!strcmp(argv[1], "*158#066"))
        {
            lidbg("*158#066--disable alarmmanager protect\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.alarmmanager.action --ei action 0 &");
        }
        else if (!strncmp(argv[1], "*158#067", 8))
        {
            g_var.alarmtimer_interval = simple_strtoul((argv[1] + 8), 0, 0);
            lidbg("set alarmtimer wakeup time:%d\n", g_var.alarmtimer_interval);
        }
        else if (!strncmp(argv[1], "*158#068", 8))
        {
            //opt args,ex:*158#0680
            int n;
            lidbg("*158#068--update firmware for camera\n");
            n = strlen(argv[1]);
            if(n != 9)//wrong args
            {
                lidbg("wrong args!");
                return;
            }
            lidbg("--------CAMERA UPDATE MODE:%s-----------", argv[1] + 8);
            if(!strcmp((argv[1] + 8), "0"))
                lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -0&");
            else if(!strcmp((argv[1] + 8), "1"))
                lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -1&");
        }
        else if (!strcmp(argv[1], "*158#069"))
        {
            lidbg("*158#069----shut BT power\n");
            lidbg_shell_cmd("echo lpc 0x02 0x5 0x00 > /dev/lidbg_drivers_dbg0 &");
        }
        else if (!strncmp(argv[1], "*158#070", 8))
        {
            g_var.acc_goto_sleep_time = simple_strtoul((argv[1] + 8), 0, 0);
            lidbg("set acc_goto_sleep_time:%d\n", g_var.acc_goto_sleep_time);
        }
        else if (!strcmp(argv[1], "*158#071"))
        {
            lidbg("udisk stable test\n");
            if(g_var.udisk_stable_test == 0)
                g_var.udisk_stable_test = 1;
            else
                g_var.udisk_stable_test = 0;
            lidbg_fs_log(USB_MOUNT_POINT"/udisk_stable_test", "udisk_stable_test\n");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#072"))
        {
            lidbg("acc on/off udisk stable test\n");
            lidbg_fs_log(USB_MOUNT_POINT"/udisk_stable_test", "udisk_stable_test\n");
            g_var.udisk_stable_test = 2;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#074"))
        {
            lidbg("pm disable cn.flyaudio.media\n");
            lidbg_shell_cmd("pm disable cn.flyaudio.media");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#075"))
        {
            lidbg("enable crash detect & debug by gsensor\n");
            lidbg_shell_cmd("echo 1 > /sys/class/sensors/mc3xxx-accel/enable");
            lidbg_shell_cmd("echo -n 'file lidbg_crash_detect.c +p' > /sys/kernel/debug/dynamic_debug/control");
        }
        else if (!strcmp(argv[1], "*158#076"))
        {
            lidbg("enable gsensor data for android\n");
            g_var.enable_gsensor_data_for_android = 1;
        }
        else if (!strcmp(argv[1], "*158#077"))
        {
            lidbg("*158#077--dumpsys meminfo\n");
            CREATE_KTHREAD(thread_dumpsys_meminfo, NULL);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#078"))
        {
            lidbg("*158#078--test seven day timeout :will reboot at tomorow 6:00,Reset once per hour\n");
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 14 &");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#079"))
        {
            lidbg("*158#079--open binder debug\n");
            lidbg_shell_cmd("echo -n 'file binder.c +p' > /sys/kernel/debug/dynamic_debug/control");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#080"))
        {
            lidbg("*158#080--disable third part apk\n");
            lidbg_shell_cmd("echo ws 3rd 10 pm disable > /dev/lidbg_pm0");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#081"))
        {
            lidbg("*158#081--enable third part apk\n");
            lidbg_shell_cmd("echo ws 3rd 10 pm enable > /dev/lidbg_pm0");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#082"))
        {
            lidbg("*158#082--nothing\n");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#083"))
        {
            lidbg("*158#083--nothing\n");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#084"))
        {
            lidbg("*158#084--nothing\n");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#085"))
        {
            lidbg("*158#085--nothing\n");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#086"))
			
        {
            lidbg("suspend no kill,disable iptable,disable alarmmanager protect,no turnoff wifi,3s goto sleep\n");
			
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 8 &");//no kill apk
	     msleep(200);
            lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 6 &");//disable iptable
	     msleep(200);
            lidbg_shell_cmd("am broadcast -a com.lidbg.alarmmanager.action --ei action 0 &");//disable alarmmanager protect
            g_var.alarmtimer_interval = 0;
			
	     lidbg_shell_cmd("echo no_wlan_ctrl > /dev/lidbg_factory_patch0");//no turnoff wifi
	     g_var.acc_goto_sleep_time = 1;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#087"))
        {
		g_var.keep_lcd_on = 1;
        }
        else if (!strcmp(argv[1], "*158#088"))
        {
            lidbg("*158#088--force stop third part apk\n");
            lidbg_shell_cmd("echo ws 3rd 10 am force-stop > /dev/lidbg_pm0");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#089"))
        {
            lidbg("*158#089--start all third part apk\n");
            lidbg_shell_cmd("echo ws 3rd 5000 am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 7 --es paraString > /dev/lidbg_pm0");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#090"))
        {
            start_stop_third_apk_enabled = !start_stop_third_apk_enabled;
            CREATE_KTHREAD(thread_start_stop_all_third_apk, NULL);
            lidbg_domineering_ack();
            lidbg("*158#090--start then stop all third part apk,%d\n", start_stop_third_apk_enabled);
        }
        else if (!strcmp(argv[1], "*158#091"))
        {
            lidbg("-------rm /data/out ----\n");
            lidbg_shell_cmd("rm -rf /data/out");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#092"))
        {
            char buff[50] = {0};
            lidbg_pm_install(get_lidbg_file_path(buff, "BugReport.ko"));
            lidbg("*158#092--enable 158013 then upload the log to internet\n");
            lidbg_shell_cmd("echo appcmd *158#013> /dev/lidbg_drivers_dbg0");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#093"))
        {
            lidbg("*158#093--rm all log created by 158013\n");
            lidbg_shell_cmd("rm -rf /sdcard/ID*");
            lidbg_shell_cmd("rm -rf /storage/udisk/ID*");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#094"))
        {
            lidbg("*158#094--rm flysystem/lib/hw and flysystem/lib/modules\n");
            lidbg_shell_cmd("mount -o remount /system");
            lidbg_shell_cmd("mv /flysystem/lib/hw /flysystem/lib/hw1");
            lidbg_shell_cmd("mv /flysystem/lib/modules /flysystem/lib/modules1");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#095"))
        {
            lidbg("*158#095--recovery flysystem/lib/hw and flysystem/lib/modules\n");
            lidbg_shell_cmd("mount -o remount /system");
            lidbg_shell_cmd("mv /flysystem/lib/hw1 /flysystem/lib/hw");
            lidbg_shell_cmd("mv /flysystem/lib/modules1 /flysystem/lib/modules");
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#096", 8))
        {
            //opt args,ex:*158#0960
            int n;
            lidbg("*158#096--read camera FW version\n");
            n = strlen(argv[1]);
            if(n != 9)//wrong args
            {
                lidbg("wrong args!");
                return;
            }
            lidbg("--------CAMERA FW MODE:%s-----------", argv[1] + 8);
            if(!strcmp((argv[1] + 8), "0"))
                lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -2&");
            else if(!strcmp((argv[1] + 8), "1"))
                lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -3&");
        }
        else if (!strcmp(argv[1], "*158#097"))
        {
            lidbg("*158#097--format SDCARD1\n");
            CREATE_KTHREAD(thread_format_sdcard1, NULL);
            //lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#098"))
        {
            lidbg("*158#098--format udisk\n");
            CREATE_KTHREAD(thread_format_udisk, NULL);
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#099", 8))
        {
            char shell_cmd[64] = {0};
            lidbg("*158#099--adust Gsensor Sensitivity\n");
            if(strlen(argv[1]) != 10)//wrong args
            {
                lidbg("Sensitivity value must be 00~99!\n");
                return;
            }
            sprintf(shell_cmd, "echo %s > /dev/mc3xxx_enable0", (argv[1] + 8));
            lidbg_shell_cmd(shell_cmd);
        }
        else if (!strcmp(argv[1], "*158#100"))
        {
            lidbg("dump log for app upload\n");
            LPC_CMD_LPC_DEBUG_REPORT;	
  	     lidbg_shell_cmd("echo appcmd *158#021 > /dev/lidbg_drivers_dbg0");
	     dump_sysinfo();
	     ssleep(3);
	     lidbg_shell_cmd("chmod 777  /sdcard/FlyLog/DriBugReport");	
	     lidbg_shell_cmd("mkdir  /sdcard/FlyLog/DriBugReport/drivers");	     
	     lidbg_shell_cmd("cp -rf /data/lidbg /sdcard/FlyLog/DriBugReport/drivers/");
	     lidbg_shell_cmd("cp -rf /data/anr /sdcard/FlyLog/DriBugReport/drivers/");
	     lidbg_shell_cmd("cp -rf /data/tombstones /sdcard/FlyLog/DriBugReport/drivers/");
            lidbg_shell_cmd("cp -rf /system/etc/build_time.txt /FlyLog/DriBugReport/drivers/system_build_time.txt");
		 lidbg_shell_cmd("chmod 777  /dev/log/DVRERR.txt");	
		 lidbg_shell_cmd("mkdir  /sdcard/FlyLog/DriBugReport/drivers/DVR");
 		 lidbg_shell_cmd("cp -rf /dev/log/DVRERR.txt /sdcard/FlyLog/DriBugReport/drivers/DVR");
		 lidbg_shell_cmd("cp -rf /dev/log/gsensor_angle.txt /sdcard/FlyLog/DriBugReport/drivers/DVR");
		 lidbg_shell_cmd("cp -rf /dev/log/build_time.txt /sdcard/FlyLog/DriBugReport/drivers/build_time.txt");
		 
        }
        else if (!strcmp(argv[1], "*158#101"))
        {
            lidbg("*158#101--trigge_SD_uevent\n");
            CREATE_KTHREAD(thread_trigge_SD_uevent, NULL);
        }
        else if (!strcmp(argv[1], "*158#102"))
        {
            lidbg("*158#102--copy /persist/display/* to udisk\n");
            lidbg_shell_cmd("cp -rf /persist/display/* /storage/udisk/");	
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#103"))
        {
            lidbg("*158#103--copy udisk QDCM.apk to flysystem/app\n");
            lidbg_shell_cmd("mount -o remount /system");	
            lidbg_shell_cmd("cp -rf /storage/udisk/app/* /system/app/");	
            lidbg_shell_cmd("chmod 777 /system/app/*");	
            lidbg_shell_cmd("reboot");	
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#104"))
        {
            lidbg("*158#104--save gps raw data to sdcard\n");
            lidbg_shell_cmd("echo raw > /dev/ubloxgps0");	
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#105"))
        {
            lidbg("*158#105-- USB HOST MODE\n");
#ifdef SOC_mt35x
            {
		lidbg_shell_cmd("chmod 777 /sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode");
		lidbg_shell_cmd("echo a_host > /sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode");
            }
#else
	    {
		USB_VBUS_POWER_DISABLE;
		msleep(500);
		USB_VBUS_POWER_ENABLE;
            }
#endif
		lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#106"))
        {
            lidbg("*158#106-- USB SLAVE MODE\n");
#ifdef SOC_mt35x
           {
		lidbg_shell_cmd("chmod 777 /sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode");
		lidbg_shell_cmd("echo b_peripheral > /sys/devices/platform/mt_usb/musb-hdrc.0.auto/mode");
            }
#else
	    {
		USB_VBUS_POWER_DISABLE;
		msleep(500);
		USB_VBUS_POWER_ENABLE;
            }
#endif
	     lidbg_domineering_ack();

        }
        else if (!strcmp(argv[1], "*158#107"))
        {
            lidbg("*158#107--android display Vertical screen\n");
            lidbg_shell_cmd("setprop persist.panel.orientation 270");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#108"))
        {
            lidbg("*158#108--android display Normal screen\n");
            lidbg_shell_cmd("setprop persist.panel.orientation 0");
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#109"))
        {
            lidbg("*158#109--en qualcomm display tun tools \n");
#ifdef SOC_mt35x
            lidbg_shell_cmd("am start -n com.mediatek.miravision.ui/.MiraVisionActivity &");
#else
            lidbg_shell_cmd("mount -o remount /system");
            lidbg_shell_cmd("cp -rf /flysystem/lib/out/QDCMMobileApp.ko /system/app/QDCMMobileApp.apk");
            lidbg_shell_cmd("chmod 777 /system/app/QDCMMobileApp.apk");
            lidbg_shell_cmd("sync");
            lidbg_shell_cmd("reboot");
#endif
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#110", 8))
        {
            char shell_cmd[64] = {0};
            lidbg("*158#110--set navi policy music level 0~100\n");
            if(strlen(argv[1]) < 9)//wrong args
            {
                lidbg("error: need para 0-100\n");
                return;
            }
            sprintf(shell_cmd, "setprop persist.lidbg.sound.dbg \"6 %s\"", (argv[1] + 8));
            lidbg_shell_cmd(shell_cmd);
            lidbg("%s\n",shell_cmd);
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#111", 8))
        {
            lidbg("*158#111--revert is_debug_mode\n");
            g_var.is_debug_mode = !g_var.is_debug_mode;
            lidbg("g_var.is_debug_mode-%d\n",g_var.is_debug_mode);
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#112", 8))
        {
            lidbg("*158#112--launch MTK settings \n");
            lidbg_shell_cmd("am start -n com.android.settings/com.ATCSetting.mainui.MainActivity &");
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#113", 8))
        {
            lidbg("*158#113--revert sound debug \n");
            lidbg_shell_cmd("echo dbg > /dev/fly_sound0");
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#114", 8))
        {
			int type = simple_strtoul((argv[1] + 8), 0, 0);
			lidbg("*158#114x--use shell cmd dd to test emmc extern SD udisk Stability \n");

			switch(type)
			{
			case 1:
			    lidbg("*158#114x--use shell cmd dd to test emmc Stability \n");
			    lidbg_shell_cmd("i=0\nwhile\ndo\nlet i++\necho test==============$i\necho test==============$i > /dev/lidbg_msg\ntime dd if=/dev/zero of=/sdcard/test.000 bs=8k count=80000\ndone &\n");
			    break;
			case 2:
			    lidbg("*158#114x--use shell cmd dd to test extern SD  Stability \n");
			    lidbg_shell_cmd("i=0\nwhile\ndo\nlet i++\necho test==============$i\necho test==============$i > /dev/lidbg_msg\ntime dd if=/dev/zero of=/storage/sdcard1/test.000 bs=8k count=80000\ndone &\n");
			    break;
			case 3:
			    lidbg("*158#114x--use shell cmd dd to test  udisk Stability \n");
			    lidbg_shell_cmd("i=0\nwhile\ndo\nlet i++\necho test==============$i\necho test==============$i > /dev/lidbg_msg\ntime dd if=/dev/zero of=/storage/udisk/test.000 bs=8k count=80000\ndone &\n");
			    break;
			default:
			    break;
			}
			lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#115"))
        {
            lidbg("*158#115--auto update fup and auto install then open the red osd \n");
            lidbg_domineering_ack();
            g_recovery_meg->bootParam.autoUp.val = 0xb;
            if(flyparameter_info_save(g_recovery_meg))
            {
                lidbg_shell_cmd("rm -rf /storage/udisk/conf");
                lidbg_shell_cmd("echo autoOps > /persist/autoOps.txt");
                lidbg_shell_cmd("sync");
                ssleep(1);
                lidbg_shell_cmd("reboot recovery");
            }
        }
        else if (!strcmp(argv[1], "*158#116"))
        {
                lidbg("*158#116--factory reset and auto install then open the red osd \n");
                lidbg_domineering_ack();
                lidbg_shell_cmd("echo autoOps > /persist/autoOps.txt");
                lidbg_shell_cmd("am broadcast -a android.intent.action.MASTER_CLEAR");
                lidbg_shell_cmd("sync");
        }
        else if (!strcmp(argv[1], "*158#117"))
        {
                lidbg("*158#117--get qcom band :input just once ,when band changed ,it will tell you though toast \n");
                lidbg_domineering_ack();
                lidbg_shell_cmd("/flysystem/lib/out/getQcomBand &");
        }
        else if (!strcmp(argv[1], "*158#118"))
        {
                lidbg("*158#118--enable navi stream policy \n");
                lidbg_shell_cmd("setprop persist.lidbg.sound.dbg \"13 1\"");
                lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#119"))
        {
                lidbg("*158#119--disable navi stream policy \n");
                lidbg_shell_cmd("setprop persist.lidbg.sound.dbg \"13 0\"");
                lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#120"))
        {
                lidbg("*158#120--enable uart print when lpm comes \n");
                lidbg_shell_cmd("echo 0 > /sys/module/printk/parameters/console_suspend");
                lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#121"))
        {
                lidbg("*158#121--auto input test \n");
                lidbg_shell_cmd("chmod 777 /data/tap_setting.sh");
                lidbg_shell_cmd("/data/tap_setting.sh &");
                lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#122"))
        {
                lidbg("*158#122--delete logcat black list conf \n");
                lidbg_shell_cmd("mount -o remount /system");
                lidbg_shell_cmd("mv /flysystem/lib/out/logcatBlackList.conf /flysystem");
                lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#123", 8))
        {
            //opt args,ex:*158#0680
            int n;
            lidbg("*158#123--gsensor debug switch\n");
            n = strlen(argv[1]);
            if(n != 9)//wrong args
            {
                lidbg("wrong args!");
                return;
            }
            lidbg("--------ISDEBUG:%s---------", argv[1] + 8);
            if(!strcmp((argv[1] + 8), "0"))
                lidbg_shell_cmd("setprop persist.gsensor.isDebug 0&");
            else if(!strcmp((argv[1] + 8), "1"))
                lidbg_shell_cmd("setprop persist.gsensor.isDebug 1&");
			lidbg_domineering_ack();
            msleep(3000);
            lidbg_reboot();
        }
        else if (!strncmp(argv[1], "*158#124", 8))
        {
            lidbg("*158#124--enable logcat when coldboot \n");
            lidbg_shell_cmd("echo coldBootLogcat > /data/coldBootLogcat.txt");
            lidbg_shell_cmd("chmod 777  /data/coldBootLogcat.txt");
            lidbg_shell_cmd("sync");
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#125", 8))
        {
            lidbg("*158#125--enable kmsg print \n");
            lidbg_shell_cmd("echo 8 > /proc/sys/kernel/printk");
            lidbg_domineering_ack();
        }
        else if (!strncmp(argv[1], "*158#126", 8))
		{
		  lidbg("*158#126--cp logcat kmsg to Udisk \n");
		  lidbg_shell_cmd("mkdir -p /storage/udisk/LogcatKmsg");
		  lidbg_shell_cmd("rm -rf /storage/udisk/LogcatKmsg/*");
		  lidbg_shell_cmd("cp -rf /data/lidbg /storage/udisk/LogcatKmsg");
		  lidbg_shell_cmd("cp -rf /sdcard/*.txt /storage/udisk/LogcatKmsg");
		  lidbg_shell_cmd("cp -rf /data/anr /storage/udisk/LogcatKmsg");
		  lidbg_shell_cmd("cp -rf /data/tombstones /storage/udisk/LogcatKmsg");
		  lidbg_shell_cmd("echo ws toast Copy.Completely 1 > /dev/lidbg_pm0");
		  lidbg_domineering_ack();
		}
        else if (!strncmp(argv[1], "*158#127", 8))
		{
		  lidbg("*158#127--grantWhiteListPermissions \n");
		  lidbg_shell_cmd("am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 8 &");
		  lidbg_domineering_ack();
		}
        else if (!strncmp(argv[1], "*158#128", 8))
		{
		  lidbg("*158#128--trigger conf check \n");
		  lidbg_shell_cmd("echo conf_check:/storage/udisk > /dev/lidbg_misc0");
		  lidbg_domineering_ack();
		}
        else if (!strncmp(argv[1], "*158#129", 8))
		{
		  lidbg("*158#129--call lidbg_uevent_cold_boot \n");
		  lidbg_shell_cmd("/flysystem/lib/out/lidbg_uevent_cold_boot &");
		}
        else if (!strncmp(argv[1], "*158#130", 8))
		{
		  lidbg("*158#130--enable red osd ,logcat \n");
		  lidbg_shell_cmd("rm -rf /sdcard/*.txt");
		  lidbg_shell_cmd("rm -rf /data/anr/*");
		  lidbg_shell_cmd("rm -rf /data/tombstones/*");
		  lidbg_shell_cmd("am broadcast -a android.provider.Telephony.SECRET_CODE -d android_secret_code://9846 &");
		  ssleep(2);
		  lidbg_shell_cmd("echo appcmd *158#001 > /dev/lidbg_drivers_dbg0");
		}
        else if (!strcmp(argv[1], "*158#131"))
        {
           	 lidbg_shell_cmd("/flysystem/lib/out/sendsignal STORE_IN_TIME &");
        }
    }
    else if(!strcmp(argv[0], "flyaudio.code.disable") )
    {
        cmd_enable = false;
    }
    else if(!strcmp(argv[0], "flyaudio.code.enable") )
    {
        cmd_enable = true;
    }
	/*
    else if(!strcmp(argv[0], "monkey") )
    {
        int enable, gpio, on_en, off_en, on_ms, off_ms;
        enable = simple_strtoul(argv[1], 0, 0);
        gpio = simple_strtoul(argv[2], 0, 0);
        on_en = simple_strtoul(argv[3], 0, 0);
        off_en = simple_strtoul(argv[4], 0, 0);
        on_ms = simple_strtoul(argv[5], 0, 0);
        off_ms = simple_strtoul(argv[6], 0, 0);
        monkey_run(enable);
        monkey_config(gpio, on_en, off_en, on_ms, off_ms);
    }
   */
    else if(!strcmp(argv[0], "recordenable") )
    {
        lidbg("-------uvccam recording -----");
        lidbg_shell_cmd("setprop lidbg.uvccam.dvr.recording 1");
        if(g_var.is_fly) lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video2 -b 1 -c -f H264 -r &");
        else lidbg_shell_cmd("./system/lib/modules/out/lidbg_testuvccam /dev/video2 -b 1 -c -f H264 -r &");
    }
    else if(!strcmp(argv[0], "recorddisable") )
    {
        lidbg("-------uvccam stop_recording -----");
        lidbg_shell_cmd("setprop lidbg.uvccam.dvr.recording 0");
    }
    else if(!strcmp(argv[0], "captureenable") )
    {
        lidbg("-------uvccam capture-----");
        if(g_var.is_fly) lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 -c -f mjpg -S &");
        else lidbg_shell_cmd("./system/lib/modules/out/lidbg_testuvccam /dev/video1 -b 1-c -f mjpg -S &");
    }
    else if(!strcmp(argv[0], "flyparameter") )
    {
        int para_count = argc - 1;
        char pre = 'N';
        for(i = 0; i < para_count; i++)
        {
            pre = g_recovery_meg->hwInfo.info[i];
            g_recovery_meg->hwInfo.info[i] = (int)simple_strtoul(argv[i + 1], 0, 0) + '0';
            lidbg("flyparameter-char.info[%d]:old,now[%d,%d]", i, pre - '0', g_recovery_meg->hwInfo.info[i] - '0');
        }
        if(flyparameter_info_save(g_recovery_meg))
        {
            lidbg_domineering_ack();
            msleep(3000);
            lidbg_reboot();
        }
    }
    else if(!strcmp(argv[0], "flyparameter_bit") )
    {
        char pre = 'N';
        int i = simple_strtoul(argv[1], 0, 0);
        i-=1;
        {
            pre = g_recovery_meg->hwInfo.info[i];
            g_recovery_meg->hwInfo.info[i] = (int)simple_strtoul(argv[2], 0, 0) + '0';
            lidbg("flyparameter-char.info[%d]:old,now[%d,%d]", i, pre - '0', g_recovery_meg->hwInfo.info[i] - '0');
        }
        if(flyparameter_info_save(g_recovery_meg))
        {
            lidbg_domineering_ack();
        }
    }

#ifndef SOC_msm8x25
    else if(!strcmp(argv[0], "pm") )
    {
        int enable;
        enable = simple_strtoul(argv[1], 0, 0);
        SOC_PM_STEP((fly_pm_stat_step)enable, NULL);
    }
    else if(!strcmp(argv[0], "pm1") )
    {
        int enable;
        enable = simple_strtoul(argv[1], 0, 0);
        LINUX_TO_LIDBG_TRANSFER((linux_to_lidbg_transfer_t)enable, NULL);
    }
    else if(!strcmp(argv[0], "fread") )
    {
        char buff[64] = {0};
        int len = fs_file_read(argv[1], buff, 0, sizeof(buff));
        buff[len - 1] = '\0';
        lidbg("%d,%s:[%s]\n", len, argv[1], buff);
        lidbg_toast_show("fread:", buff);
    }
    else if(!strcmp(argv[0], "fwrite") )
    {
        fs_file_write2(argv[1], argv[2]);
        lidbg("%s:[%s]\n", argv[1], argv[2]);
    }
    else if(!strcmp(argv[0], "exist") )
    {
        lidbg("exist:[%d,%s]\n", fs_is_file_exist(argv[1]),argv[1]);
    }
    else if (!strcmp(argv[0], "lpc"))
    {
        int para_count = argc - 1;
        u8 lpc_buf[10] = {0};
        for(i = 0; i < para_count; i++)
        {
            lpc_buf[i] = simple_strtoul(argv[i + 1], 0, 0);
            lidbg("%d ", lpc_buf[i]);
        }
        lidbg("para_count = %d\n", para_count);
        SOC_LPC_Send(lpc_buf, para_count);
    }
    else if(!strcmp(argv[0], "irq") )
    {
        int irq;
        irq = simple_strtoul(argv[1], 0, 0);
        SOC_IO_ISR_Add(irq, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT, TEST_isr, NULL);
        SOC_IO_ISR_Enable(irq);
        lidbg("SOC_IO_ISR_Add[%d]\n", irq);
    }
    else if (!strcmp(argv[0], "vol"))
    {
        //   #ifndef SOC_mt3360
        //       int vol;
        //       vol = simple_strtoul(argv[1], 0, 0);
        //       SAF7741_Volume(vol);
        //   #endif
    }
    else if (!strcmp(argv[0], "screen_shot"))
    {
        CREATE_KTHREAD(thread_screenshot, NULL);
    }
    else if (!strcmp(argv[0], "readdir"))
    {
        if(argv[1])
            LIDBG_WARN("%s:file count=%d\n", argv[1], lidbg_readdir_and_dealfile(argv[1], callback_func_test_readdir));
        else
            LIDBG_ERR("err:echo readdir /storage/udisk/conf > /dev/lidbg_drivers_dbg0\n");
    }
#endif

#ifdef SOC_msm8x25
    else if(!strcmp(argv[0], "video"))
    {
        int new_argc;
        char **new_argv;
        new_argc = argc - 1;
        new_argv = argv + 1;
        lidbg_video_main(new_argc, new_argv);
    }
#endif
}
