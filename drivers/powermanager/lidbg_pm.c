#include "lidbg.h"
LIDBG_DEFINE;
//#define CONTROL_PM_IO_BY_BP

#define PM_DIR LIDBG_LOG_DIR"pm_info/"
#define PM_INFO_FILE PM_DIR"pm_info.txt"
#define PM_ACC_FILE PM_DIR"pm_acc.txt"
#define PM_WAKEUP_LOG_FILE PM_DIR"pm_wakeup_log.txt"

#if defined(CONFIG_HAS_EARLYSUSPEND)
#define WAKE_LOCK_ACTIVE                 (1U << 9)
#define WAKE_LOCK_AUTO_EXPIRE            (1U << 10)
static LIST_HEAD(inactive_locks);
#endif

#define PM_FILE_INFO_SIZE (5)
#ifdef SOC_mt3360
#define SUSPEND_KEY_POLLING_TIME   (jiffies + 100*(HZ/1000))  /* 100ms */
void suspendkey_timer_isr(unsigned long data);
static DEFINE_TIMER(suspendkey_timer, suspendkey_timer_isr, 0, 0);
#endif
static DECLARE_COMPLETION(sleep_observer_wait);
static atomic_t is_in_sleep = ATOMIC_INIT(0);
int power_on_off_test = 0;
static struct wake_lock pm_wakelock;
void observer_start(void);
void observer_stop(void);
extern int soc_io_resume_config(u32 index, u32 direction, u32 pull, u32 drive_strength);
extern void grf_backup(void);
extern void grf_restore(void);

#ifdef SUSPEND_ONLINE
#define SUSPEND_TIME_OUT_KILL_PROCESS
#define SUSPEND_TIME_OUT_FORCE_UNLOCK
#else
#define SUSPEND_TIME_OUT_KILL_PROCESS
#define SUSPEND_TIME_OUT_FORCE_UNLOCK
#endif

int have_triggerd_sleep_S = 0;

static DECLARE_COMPLETION(pm_3rd_package_wait);
static int delay_ms_3rd=1000;
static char shell_head[128];

bool is_safety_apk(char *apkname)
{
    if(strncmp(apkname, "com.fly.flybootservice", sizeof("com.fly.flybootservice") - 1) == 0)
        return true;
    else
        return false;
}

/*	0:show wakelock		1:kill has lock apk		2:save has lock apk package*/
void userspace_wakelock_action(int action_enum, char *file_path)
{
    int index = 0;
    char *p1 = NULL, *p2 = NULL, *kill = NULL;
    struct wakelock_item *pos;
    struct list_head *client_list ;

    client_list = &lidbg_wakelock_list;
    if(!list_empty(client_list))
    {
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if (pos->name && pos->cunt)
            {
                index++;
                PM_WARN("%d,MAX%d<THE%d:[%d,%d][%s][%s,%s]>\n", pos->cunt, pos->cunt_max, index, pos->pid, pos->uid, lock_type(pos->is_count_wakelock), pos->name, pos->package_name);
                switch (action_enum)
                {
                case 0:
                    break;
                case 1:
                {
                    p1 = strchr(pos->package_name, ',');
                    p2 = strchr(pos->package_name, '.');
                    kill = NULL;

                    if(p1)
                        kill = p1 + 1;
                    else if(p2)
                        kill = pos->package_name;
                    if(kill && !is_safety_apk(kill))
                        lidbg_force_stop_apk(kill);
                }
                break;
                case 2:
                    if (file_path && pos->is_count_wakelock)
                        fs_string2file(PM_FILE_INFO_SIZE, file_path, "[J]%d,[%s,%s]>\n", index, pos->name, pos->package_name);
                    break;

                default:
                    lidbg("<lidbg_show_wakelock.err:%d>\n", action_enum );
                    break;
                }

            }
        }
    }
    else
        lidbg("<err.lidbg_show_wakelock:nobody_register>\n");
}


int kernel_wakelock_print(char *info)
{
#if defined(CONFIG_HAS_EARLYSUSPEND)
    //add for px3
    struct wake_lock *lock;
#else
    struct wakeup_source *ws;
#endif
    int list_count = 0;

    if(g_var.ws_lh == NULL)
    {
        PM_ERR("g_var.ws_lh==NULL\n");
        return -1;
    }

#if defined(CONFIG_HAS_EARLYSUSPEND)
    //add for px3 wakelog print
    list_for_each_entry(lock, g_var.ws_lh, link)
    {
        PM_WARN("[%s]:%d:%s", info, list_count, lock->name);
        list_count++;
    }
#else
    PM_WARN("<%s>\n", info);
    rcu_read_lock();
    list_for_each_entry_rcu(ws, g_var.ws_lh, entry)
    {
        spin_lock_irq(&ws->lock);
        if (ws->active)
        {
            PM_WARN("[%s]:%d:%s,ac.%lu,rc%lu\n", info, list_count, ws->name, ws->active_count, ws->relax_count);
            list_count++;
        }
        spin_unlock_irq(&ws->lock);
    }
    rcu_read_unlock();
#endif
    return 1;
}

int kernel_wakelock_force_unlock(char *info)
{
#if defined(CONFIG_HAS_EARLYSUSPEND)
#else
    struct wakeup_source *ws;
#endif

    int list_count = 0;
    if(g_var.ws_lh == NULL)
    {
        PM_ERR("g_var.ws_lh==NULL\n");
        return -1;
    }
#if defined(CONFIG_HAS_EARLYSUSPEND)
    {
        struct wake_lock *lock, *n;
        list_for_each_entry_safe(lock, n, g_var.ws_lh, link)
        {
            PM_WARN("force unlock:[%s]:%d:%s\n", info, list_count, lock->name);
            lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
            list_del(&lock->link);
            list_add(&lock->link, &inactive_locks);
            list_count++;
        }
    }
#else
    PM_WARN("<%s>\n", info);
    rcu_read_lock();
    list_for_each_entry_rcu(ws, g_var.ws_lh, entry)
    {
        if ( ws->active)
        {
            PM_WARN("[%s]:%d:%s,ac.%lu,rc%lu\n", info, list_count, ws->name, ws->active_count, ws->relax_count);
            __pm_relax(ws);
            list_count++;
        }
    }
    rcu_read_unlock();
#endif
    return 1;
}

int kernel_wakelock_save_wakelock(char *info, char *path)
{
    struct wakeup_source *ws;
    int list_count = 0;
    if(g_var.ws_lh == NULL)
    {
        PM_ERR("g_var.ws_lh==NULL\n");
        return -1;
    }
    PM_WARN("<%s>\n", info);
    rcu_read_lock();
    list_for_each_entry_rcu(ws, g_var.ws_lh, entry)
    {
        if (ws->active)
        {
            rcu_read_unlock();
            fs_string2file(PM_FILE_INFO_SIZE, path, "%s[K].%d:%s\n", info, list_count, ws->name);
            list_count++;
            rcu_read_lock();
        }
    }
    rcu_read_unlock();
    return 1;
}

struct wakeup_source *kernel_wakelock_find_wakelock(char *info)
{
    struct wakeup_source *ws, *tmp = NULL;
    int exist = 0;
    if(g_var.ws_lh == NULL)
    {
        PM_ERR("g_var.ws_lh==NULL\n");
        return NULL;
    }
    rcu_read_lock();
    list_for_each_entry_rcu(ws, g_var.ws_lh, entry)
    {
        spin_lock_irq(&ws->lock);
        if (ws->active && !strcmp(ws->name, info))
            exist = 1;
        spin_unlock_irq(&ws->lock);

        if(exist == 1)
        {
            tmp = ws;
            break;
        }
    }
    rcu_read_unlock();
    return tmp;
}

void lidbg_pm_step_call(fly_pm_stat_step step, void *data)
{
    switch (step)
    {
    case PM_AUTOSLEEP_STORE1:
    {
        char *buff = data;
        if(!strcmp(buff, "mem"))
        {
            atomic_set(&is_in_sleep, 1);
        }
        else if(!strcmp(buff, "off"))
        {
            atomic_set(&is_in_sleep, 0);
        }
        PM_WARN("PM_AUTOSLEEP_STORE1:[%d,%s,%d]\n", g_var.sleep_counter, buff, atomic_read(&is_in_sleep));
    }
    break;
    case PM_AUTOSLEEP_SET_STATE2:
        break;
    case PM_QUEUE_UP_SUSPEND_WORK3:
        break;
    case PM_TRY_TO_SUSPEND4:
        //   kernel_wakelock_print("start1:");
        //   userspace_wakelock_action(0);
        break;
    case PM_TRY_TO_SUSPEND4P1:
    case PM_TRY_TO_SUSPEND4P2:
    case PM_TRY_TO_SUSPEND4P3:
    case PM_TRY_TO_SUSPEND4P4:
    case PM_TRY_TO_SUSPEND4P5:
        //   kernel_wakelock_print("stop1:");
        //   userspace_wakelock_action(0);
        break;
    case PM_SUSPEND5:
        break;
    case PM_ENTER_STATE6:
        break;
    case PM_ENTER_STATE6P1:
        break;
    case PM_ENTER_STATE6P2:
        break;
    case PM_SUSPEND_DEVICES_AND_ENTER7:
        //suspend_ops->enter.in
        break;
    case PM_SUSPEND_ENTER8:
        SOC_System_Status(FLY_KERNEL_DOWN);
	 have_triggerd_sleep_S = 0;
        //if(g_var.is_debug_mode == 1)
        MCU_WP_GPIO_OFF;
        SOC_IO_SUSPEND;
        g_var.sleep_counter++;
#ifdef SOC_rk3x88
        grf_backup();
#endif
        PM_SLEEP_DBG("SLEEP8.suspend_enter.MCU_WP_GPIO_OFF;sleep_count:%d\n", g_var.sleep_counter);
        break;
    case PM_SUSPEMD_OPS_ENTER9:
        break;
    case PM_SUSPEMD_OPS_ENTER9P1:
#ifdef SOC_rk3x88
        grf_restore();
#endif
        SOC_System_Status(FLY_KERNEL_UP);
        SOC_IO_RESUME;
        MCU_WP_GPIO_ON;
        PM_SLEEP_DBG("WAKEUP9.1.suspend_enter.wakeup.MCU_WP_GPIO_ON;\n");
        //suspend_ops->enter.out
        break;
    case PM_NULL:
        PM_ERR("PM_NULL\n");
        break;
    default:
        break;
    }
}

int linux_to_lidbg_receiver(linux_to_lidbg_transfer_t _enum, void *data)
{
    switch (_enum)
    {
    case LTL_TRANSFER_RTC:
        break;
    case LTL_TRANSFER_NULL:
        PM_ERR("LTL_TRANSFER_NULL\n");
        break;
    default:
        break;
    }
    return 1;
}

void usb_disk_enable(bool enable)
{
    lidbg("[%s]\n", enable ? "usb_enable" : "usb_disable");
    if(enable)
        USB_WORK_ENABLE;
    else
        USB_WORK_DISENABLE;
}

#ifdef LIDBG_PM_MONITOR
static struct wakeup_source *autosleep_ws;
static int thread_lidbg_pm_monitor(void *data)
{
    PM_WARN("<thread_lidbg_pm_monitor.in>\n");
    autosleep_ws = wakeup_source_register("autosleep");
    if (!autosleep_ws)
        autosleep_ws = wakeup_source_register("autosleep");
    PM_WARN("<0=====LPC_CMD_ACC_SWITCH_START>\n");
    while(1)
    {

        PM_WARN("<1=====hold the lock.usb enable>\n");
        //hold the lock
        MCU_APP_GPIO_ON;
        __pm_stay_awake(autosleep_ws);
        usb_disk_enable(true);

        PM_WARN("<2=====cold boot wait ACC start>\n");
        //cold boot wait ACC start
        while(atomic_read(&is_in_sleep) == 0)
            msleep(500);

        PM_WARN("<3=====usb disable>\n");
        usb_disk_enable(false);
        msleep(5000);

        PM_WARN("<4=====trigger LPC short and long press>\n");
        //trigger LPC short and long press
        MCU_APP_GPIO_OFF;

        PM_WARN("<5=====wait LPC short and long press>\n");
        //wait LPC short and long press
        msleep(300 + 5000 + 600 + 5000);

        PM_WARN("<6=====release lock and send power key>\n");
        //release lock and send power key
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
        __pm_relax(autosleep_ws);

        PM_WARN("<7=====wait power key enable>\n");
        //wait power key enable
        while(atomic_read(&is_in_sleep) == 0)
            msleep(500);

        PM_WARN("<8=====wait until wake up>\n");
        //wait until wake up
        observer_start();
        while(atomic_read(&is_in_sleep) == 1)
            msleep(500);

    }
    wakeup_source_unregister(autosleep_ws);
    usb_disk_enable(true);
    PM_WARN("<thread_lidbg_pm_monitor.exit>\n");
    return 1;
}
#endif

static int thread_gpio_app_status_delay(void *data)
{
    ssleep(10);
    LPC_PRINT(true, g_var.sleep_counter, "PM:MCU_WP_GPIO_ON1");
    //while(0==g_var.android_boot_completed)
    //{
    //    ssleep(10);
    //   PM_WARN("<wait android_boot_completed : %d>\n",g_var.android_boot_completed);
    //};
    PM_WARN("<set MCU_APP_GPIO_ON >\n");
    LPC_PRINT(true, g_var.sleep_counter, "PM:MCU_APP_GPIO_ON2");
    MCU_APP_GPIO_ON;

#ifdef CONTROL_PM_IO_BY_BP
    MCU_SET_APP_GPIO_SUSPEND;
#endif

#ifdef LIDBG_PM_AUTO_ACC
    LPC_CMD_ACC_SWITCH_START;
#endif

#ifdef LIDBG_PM_MONITOR
    CREATE_KTHREAD(thread_lidbg_pm_monitor, NULL);
#endif

    return 1;
}

static int test_task_flag(struct task_struct *p, int flag)
{
    struct task_struct *t = p;
    do
    {
        task_lock(t);
        if (test_tsk_thread_flag(t, flag))
        {
            task_unlock(t);
            return 1;
        }
        task_unlock(t);
    }
    while_each_thread(p, t);
    return 0;
}
struct task_struct *find_task_by_name_or_kill(bool enable_filter, bool enable_dbg, bool enable_kill, char *taskname)
{
    int total = 0;
    struct task_struct *p;
    struct task_struct *selected = NULL;
    for_each_process(p)
    {
        if(enable_filter)
        {
            if (p->flags & PF_KTHREAD || !(p->flags & PF_FORKNOEXEC))
                continue;
            if (test_task_flag(p, TIF_MEMDIE))  // TIF_MM_RELEASED is not defined on MT3360 kernel
                continue;
        }
        if(enable_dbg)
        {
            total++;
            PM_WARN("<task%d:0x%x,%ld,[%s],%d>\n", total, p->flags, p->state, p->comm, p->pid);
        }
        if(taskname && (!strncmp(taskname, p->comm, strlen(taskname))))
        {
            selected = p;
            break;
        }
    }
    if(enable_kill && selected)
    {
        send_sig(SIGKILL, selected, 0);
        PM_WARN("<killtask:0x%x,%d,%s>\n", selected->flags, selected->pid, selected->comm);
    }
    return selected;
}
int pm_open (struct inode *inode, struct file *filp)
{
    return 0;
}
#ifdef LIDBG_PM_CHECK_SLEEPED
static int thread_send_power_key(void *data)
{
    PM_WARN("PM:send power key\n");
    SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
    return 1;
}
#endif

#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974) 
static int thread_gps_handle(void *data)
{
       msleep(50*1000);
	  if(ANDROID_VERSION >= 600)
	       lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 22 &");
         else
       	lidbg_shell_cmd("settings put  secure location_providers_allowed network,gps");
       return 1;
}
#endif

#ifdef SOC_mt3360
void suspendkey_timer_isr(unsigned long data)
{
    lidbg_key_report(KEY_POWER, KEY_RELEASED);
    lidbg("fly power key gotosleep --\n");
}
#endif
ssize_t pm_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
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
    PM_WARN("-----FLYSTEP------------------[%s]---\n", cmd_buf);

    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;

    //flyaudio logic
    if(!strcmp(cmd[0], "flyaudio"))
    {

        lidbg("case:[%s]\n", cmd[1]);
        if(!strcmp(cmd[1], "screen_off"))
        {
            SOC_System_Status(FLY_SCREEN_OFF);
            if(SOC_Hal_Acc_Callback)
            {
                lidbg("hal callback 0\n");
                SOC_Hal_Acc_Callback(0);
            }
            if(!g_var.is_fly && fs_is_file_exist("/system/app/NfcNci.apk"))
                lidbg_rm("/system/app/NfcNci.apk");
            LPC_PRINT(true, g_var.sleep_counter, "PM:screen_off");
        }
        else  if(!strcmp(cmd[1], "screen_on"))
        {
            wake_lock(&pm_wakelock);
            SOC_System_Status(FLY_SCREEN_ON);
            if(SOC_Hal_Acc_Callback)
            {
                lidbg("hal callback 1\n");
                SOC_Hal_Acc_Callback(1);
            }
            LPC_PRINT(true,g_var. sleep_counter, "PM:screen_on");
        }
        else  if(!strcmp(cmd[1], "android_up"))
        {
#ifdef PLATFORM_msm8909
			lidbg_shell_cmd("/system/bin/r 0x1014000 0x1cc");
			lidbg_shell_cmd("/system/bin/r 0x1015000 0x1cc");
#endif
            PM_WARN("PM:g_var.system_status.%d,%d\n", g_var.system_status, (g_var.system_status == FLY_GOTO_SLEEP));
#ifdef LIDBG_PM_CHECK_SLEEPED
            if(g_var.system_status == FLY_GOTO_SLEEP)
                CREATE_KTHREAD(thread_send_power_key, NULL);
#endif
            MCU_WP_GPIO_ON;
            MCU_APP_GPIO_ON;
            SOC_System_Status(FLY_ANDROID_UP);

#if ANDROID_VERSION >= 600
			lidbg_shell_cmd("dumpsys deviceidle disable");
			lidbg("quit doze mode.\n");
#endif

        }
        else  if(!strcmp(cmd[1], "android_down"))
        {
        	MCU_APP_GPIO_OFF;
#ifdef PLATFORM_msm8909
			lidbg_shell_cmd("/system/bin/r 0x1014000 0x1c0");
			lidbg_shell_cmd("/system/bin/r 0x1015000 0x1c0");
#endif
            SOC_System_Status(FLY_ANDROID_DOWN);

#if ANDROID_VERSION >= 600
			lidbg_shell_cmd("dumpsys battery unplug");
			lidbg_shell_cmd("dumpsys deviceidle enable");
			lidbg_shell_cmd("dumpsys deviceidle force-idle");
			lidbg("run as doze mode.\n");
#endif

#ifdef SUSPEND_ONLINE
            observer_start();
            LPC_PRINT(true, g_var.sleep_counter, "PM:android_down");
            wake_unlock(&pm_wakelock);
#endif

#ifdef SOC_mt3360
            extern unsigned int fly_acc_step;
            fly_acc_step = 1;
#endif
		}else if(!strcmp(cmd[1], "pre_gotosleep")){
			lidbg("pre_gotosleep .\n");
		}
        else if(!strcmp(cmd[1], "request_fastboot")){
		lidbg("request_fastboot pull down gpio_app.\n");
		MCU_APP_GPIO_OFF;
        }
        else  if(!strcmp(cmd[1], "kill"))
        {
            char shellcmd[64] = {0};
            sprintf(&shellcmd[0],  "kill %s &", cmd[2]);
            lidbg_shell_cmd(shellcmd);
            PM_WARN("[%s]\n", shellcmd);
        }
        else  if(!strcmp(cmd[1], "gotosleep"))
        {
            SOC_System_Status(FLY_GOTO_SLEEP);
#ifdef SOC_mt3360
            //ssleep(1);
            lidbg("fly power key gotosleep ++\n");
            lidbg_key_report(KEY_POWER, KEY_PRESSED);
            //msleep(100);
            //lidbg_key_report(KEY_POWER, KEY_RELEASED);
            //lidbg("fly power key gotosleep --\n");
            mod_timer(&suspendkey_timer, SUSPEND_KEY_POLLING_TIME);
#else
#ifdef SUSPEND_ONLINE
#else
            observer_start();
            LPC_PRINT(true, g_var.sleep_counter, "PM:gotosleep");
#endif
#endif

#ifdef SUSPEND_ONLINE
#else
            wake_unlock(&pm_wakelock);
#endif
        }
        else if(!strcmp(cmd[1], "devices_up"))
        {
            MCU_APP_GPIO_ON;
#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974) 
	  if(ANDROID_VERSION >= 600)
            	lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 22 &");
	  else
              lidbg_shell_cmd("settings put  secure location_providers_allowed network,gps");
#endif
            SOC_System_Status(FLY_DEVICE_UP);
            PM_WARN("mediascan.en.1\n");
            lidbg_shell_cmd("setprop persist.lidbg.mediascan.en 1");

            if(SOC_Hal_Acc_Callback)
            {
                lidbg("hal callback 2\n");
                SOC_Hal_Acc_Callback(2);
            }
        }
        else if(!strcmp(cmd[1], "devices_down"))
        {
        
#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974) 
	  if(ANDROID_VERSION >= 600)
           	 lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 23 &");
	  else
               lidbg_shell_cmd("settings put  secure location_providers_allowed \"\"");
#endif
            SOC_System_Status(FLY_DEVICE_DOWN);
            PM_WARN("mediascan.en.0\n");
            lidbg_shell_cmd("setprop persist.lidbg.mediascan.en 0");

	
            if(ANDROID_VERSION >= 500)
            {
                //only 5.0 later version can't start bootanim when ACC on
                PM_WARN("<bootanim.exit>\n");
                lidbg_shell_cmd("setprop service.bootanim.exit 0");
            }
            if(SOC_Hal_Acc_Callback)
            {
                lidbg("hal callback 3\n");
                SOC_Hal_Acc_Callback(3);
            }
        }
        else if(!strcmp(cmd[1], "acc_history"))
        {
            lidbg_shell_cmd("rm -r "PM_DIR"*");
        }
         else  if(!strcmp(cmd[1], "PmServiceStar"))
         {
         /*
#ifdef SUSPEND_ONLINE
			lidbg_shell_cmd("insmod /system/lib/modules/out/lidbg_rmtctrl.ko");
			lidbg_shell_cmd("insmod /flysystem/lib/out/lidbg_rmtctrl.ko");
#else
			lidbg_shell_cmd("insmod /system/lib/modules/out/lidbg_powerkey.ko");
			lidbg_shell_cmd("insmod /flysystem/lib/out/lidbg_powerkey.ko");
#endif
*/
	  }

    }
#if 1 //pm debug
    if(!strcmp(cmd[0], "ws"))
    {
        lidbg("ws:case:[%s]\n", cmd[1]);
        if(!strcmp(cmd[1], "chmod"))
            lidbg_chmod(cmd[2]);
        else if(!strcmp(cmd[1], "rm"))
            lidbg_rm(cmd[2]);
        else  if(!strcmp(cmd[1], "list"))
        {
            int  ws_action_type = simple_strtoul(cmd[2], 0, 0);
            /*	0:show wakelock		1:force unlock		2:kill has lock apk		3:save lock		4:find wakelock*/
            switch (ws_action_type)
            {
            case 0:
                kernel_wakelock_print("test:");
                userspace_wakelock_action(0, NULL);
                break;
            case 1:
                kernel_wakelock_force_unlock("test:");
                userspace_wakelock_action(1, NULL);
                break;
            case 2:
                kernel_wakelock_print("test:");
                userspace_wakelock_action(1, NULL);
                break;
            case 3:
                fs_clear_file(LIDBG_OSD_DIR"pm_info.txt");
                kernel_wakelock_save_wakelock("test:", LIDBG_OSD_DIR"pm_info.txt");
                userspace_wakelock_action(2, LIDBG_OSD_DIR"pm_info.txt");
                break;
            case 4:
                if(cmd[3] && kernel_wakelock_find_wakelock(cmd[3]) != NULL)
                    PM_WARN("<find:%s>\n", cmd[3]);
                userspace_wakelock_action(0, NULL);
                break;

            default:
                break;
            }
        }
        else  if(!strcmp(cmd[1], "pm"))
        {
            int pm_observer_mode = simple_strtoul(cmd[2], 0, 0);
            if(pm_observer_mode)
                observer_start();
            else
                observer_stop();
        }
        else  if(!strcmp(cmd[1], "reb"))
        {
            int  ws_action_type = simple_strtoul(cmd[2], 0, 0);
            msleep(100);
            /*	0:normal reboot		1:reboot recovery		2:reboot bootloader	*/
            switch (ws_action_type)
            {
            case 0:
                lidbg_shell_cmd("reboot");
                break;
            case 1:
                lidbg_shell_cmd("reboot recovery");
                break;
            case 2:
                lidbg_shell_cmd("reboot bootloader");
                break;
            default:
                break;
            }
        }
        else  if(!strcmp(cmd[1], "apk"))
        {
            lidbg_pm_install(cmd[2]);
        }
        else  if(!strcmp(cmd[1], "kill"))
        {
            lidbg_force_stop_apk(cmd[2]);
        }
        else  if(!strcmp(cmd[1], "toast"))
        {
            //int  enum_value = simple_strtoul(cmd[3], 0, 0);
            lidbg_toast_show("lidbg:    ",cmd[2]);
        }
        else  if(!strcmp(cmd[1], "udisk_reset"))
        {
            usb_disk_enable(false);
            ssleep(2);
            usb_disk_enable(true);
        }
        else  if(!strcmp(cmd[1], "task"))
        {
            find_task_by_name_or_kill(true, false, true, "c2739.mainframe");

            if(cmd[5])
            {
                int  enable_filter = simple_strtoul(cmd[3], 0, 0);
                int  enable_dbg = simple_strtoul(cmd[4], 0, 0);
                int  enable_kill = simple_strtoul(cmd[5], 0, 0);
                PM_WARN("<kill,in,%s,%d,%d,%d>\n", cmd[2], enable_filter, enable_dbg, enable_kill);
                find_task_by_name_or_kill(enable_filter, enable_dbg, enable_kill, cmd[2]);
            }
            else
                PM_WARN("< echo ws task c2739.mainframe 1 1 1 > /dev/lidbg_pm0 >\n");
        }
        else  if(!strcmp(cmd[1], "3rd"))
        {
		delay_ms_3rd= simple_strtoul(cmd[2], 0, 0);
		snprintf(shell_head, sizeof(shell_head), "%s %s %s %s %s %s %s %s %s", cmd[3], cmd_num > 4 ? cmd[4] : "", cmd_num > 5 ? cmd[5] : ""
			, cmd_num > 6 ? cmd[6] : "", cmd_num > 7 ? cmd[7] : "", cmd_num > 8 ? cmd[8] : "", cmd_num > 9 ? cmd[9] : "", cmd_num > 10 ? cmd[10] : "", cmd_num > 11 ? cmd[11] : "");
		PM_WARN("<3rd.delay:%d  shell_head:%s>\n", delay_ms_3rd,shell_head);
		complete(&pm_3rd_package_wait);
        }
    }
#endif

    return size;
}
ssize_t  pm_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    if (copy_to_user(buffer,  &g_var.system_status,  4))
    {
        lidbg("copy_to_user ERR\n");
    }
    return size;
}

static  struct file_operations pm_nod_fops =
{
    .owner = THIS_MODULE,
    .open = pm_open,
    .read = pm_read,
    .write = pm_write,
};


int pm_state_open (struct inode *inode, struct file *filp)
{
    return 0;
}
ssize_t pm_state_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char *cmd[8] = {NULL};
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

    //flyaudio logic
    if(!strcmp(cmd[0], "flyaudio"))
    {
    }

    return size;
}
ssize_t  pm_state_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    char acc_state[128 + 8] = {0};
    sprintf(acc_state, "%d/%d",g_var.acc_counter/2, g_var.sleep_counter);
    if (copy_to_user(buffer, acc_state, strlen(acc_state)))
    {
        lidbg("copy_to_user ERR\n");
    }
	PM_WARN("%s",acc_state);
    return size;
}
static  struct file_operations pm_state_fops =
{
    .owner = THIS_MODULE,
    .open = pm_state_open,
    .read = pm_state_read,
    .write = pm_state_write,
};

void clear_pm_log(void)
{
    if((fs_get_file_size(PM_ACC_FILE)+fs_get_file_size(PM_INFO_FILE))> 5 * 1024 )
    {
        lidbg_shell_cmd("rm -r "PM_DIR"*");
        PM_WARN("clear acc history\n");
    }
}
static int thread_save_acc_times(void *data)
{
    clear_pm_log();
    fs_clear_file(PM_ACC_FILE);
    fs_string2file(0, PM_ACC_FILE, "acc_counter:%d  sleep_counter:%d",g_var.acc_counter/2, g_var.sleep_counter);
    lidbg_fs_log(PM_WAKEUP_LOG_FILE, "acc_counter:%d  sleep_counter:%d\n",g_var.acc_counter/2, g_var.sleep_counter);
    return 1;
}
#ifdef CONFIG_PM
static int pm_suspend(struct device *dev)
{
    DUMP_FUN;
#ifdef SOC_mt3360
    MCU_WP_GPIO_OFF;
#endif
    return 0;
}
static int pm_resume(struct device *dev)
{
    DUMP_FUN;
    DUMP_BUILD_TIME;
#ifdef SOC_mt3360
    MCU_WP_GPIO_ON;
    soc_io_resume_config(0, 0, 0, 0);
#endif

    CREATE_KTHREAD(thread_save_acc_times, NULL);
    return 0;
}
static struct dev_pm_ops lidbg_pm_ops =
{
    .suspend	= pm_suspend,
    .resume		= pm_resume,
};
#endif

void observer_prepare(void)
{
#ifdef VENDOR_QCOM
    char cmd[128] = {0};
    lidbg_chmod("/sys/module/msm_show_resume_irq/parameters/debug_mask");
    ssleep(1);
    fs_file_write("/sys/module/msm_show_resume_irq/parameters/debug_mask", false, "1", 0, strlen("1"));
    sprintf(cmd, "cat /proc/interrupts > %sinterrupts.txt &", LIDBG_LOG_DIR);
    lidbg_shell_cmd(cmd);
    fs_register_filename_list(LIDBG_LOG_DIR"interrupts.txt", true);
#endif
    fs_register_filename_list(PM_INFO_FILE, true);
}
void observer_start(void)
{
#ifdef SOC_mt3360
    return ;
#else
    complete(&sleep_observer_wait);
#endif
}
void observer_stop(void)
{
}

static int thread_observer(void *data)
{
    char when[64] = {0};

    observer_prepare();
    while(!kthread_should_stop())
    {
        have_triggerd_sleep_S = 0;
        if( !wait_for_completion_interruptible(&sleep_observer_wait))
        {

#ifndef SUSPEND_ONLINE
            find_task_by_name_or_kill(true, false, true, "c2739.mainframe");
#endif	   			
            //kernel_wakelock_print("start:");
            //userspace_wakelock_action(0, NULL);
            //lidbg_shell_cmd("echo msg airplane_mode_on:$(getprop persist.radio.airplane_mode_on) > /dev/lidbg_pm0");
            //lidbg_shell_cmd("echo msg wlan.driver.status:$(getprop wlan.driver.status) > /dev/lidbg_pm0");
            //find_task_by_name_or_kill(true, true, false, NULL);
            while(1) //atomic_read(&is_in_sleep) == 1
            {
                ssleep(1);
				
#ifdef SUSPEND_ONLINE
              //  if((g_var.system_status != FLY_ANDROID_DOWN) && (g_var.system_status != FLY_SLEEP_TIMEOUT) && (g_var.system_status != FLY_GOTO_SLEEP))
                    if(g_var.acc_flag == FLY_ACC_ON)
#else
		      if(g_var.system_status != FLY_GOTO_SLEEP)
#endif
                    break;

                have_triggerd_sleep_S++;
                switch (have_triggerd_sleep_S)
                {

#ifdef SUSPEND_ONLINE
                case 60*15:
#else
		  case 15:
		  case 30:
#endif

#ifdef SUSPEND_TIME_OUT_KILL_PROCESS
					if( g_var.suspend_timeout_protect  == 0) break;
					lidbgerr("Sleep timeout,  start to kill process...\n");
					#ifdef SUSPEND_ONLINE
					SOC_System_Status(FLY_SLEEP_TIMEOUT);
					#else
					 lidbg_shell_cmd("am broadcast -a com.lidbg.flybootserver.action --ei action 21 &");
					#endif
					break;
#endif

#ifdef SUSPEND_ONLINE
                case 60*10:
#else
		  case 60:
		  case 120:
		  case 150:
#endif
#ifdef SUSPEND_TIME_OUT_FORCE_UNLOCK
			//if( g_var.suspend_timeout_protect  == 0) break;
                    sprintf(when, "unlock%d,%d:", have_triggerd_sleep_S, g_var.sleep_counter);
                    kernel_wakelock_save_wakelock(when, PM_INFO_FILE);
                    kernel_wakelock_force_unlock(when);
                    userspace_wakelock_action(2, PM_INFO_FILE);
                    if(g_var.is_debug_mode == 1)
                    {
                        lidbg_loop_warning();

                    }
#endif
                    break;
                case 11:
#ifdef SUSPEND_ONLINE
					break;
#else

#endif
			    lidbg("dump system info\n");
			    lidbg_shell_cmd("date  >> /data/lidbg/pm_info/ps.txt");				
			    lidbg_shell_cmd("ps -t >> /data/lidbg/pm_info/ps.txt");
				
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
			    lidbg_shell_cmd("chmod 777 /data/lidbg/ -R");
				
 			    break;
                  case 13:
			   // lidbg_shell_cmd("pm disable cld.navi.c2739.mainframe");
			   //   lidbg_shell_cmd("pm disable cn.flyaudio.media");
                    	    break;
#ifdef SUSPEND_ONLINE
                case 60*19:
			if( g_var.suspend_timeout_protect  == 0) break;
			lidbgerr("%s suspend timeout,reboot!!\n",__FUNCTION__);
			lidbg_shell_cmd("reboot");
			 break;
                default:
                    //if(have_triggerd_sleep_S >= 5 && !(have_triggerd_sleep_S % 5) && ((g_var.system_status == FLY_ANDROID_DOWN) || (g_var.system_status == FLY_SLEEP_TIMEOUT) ||(g_var.system_status == FLY_GOTO_SLEEP)))//atomic_read(&is_in_sleep) == 1
                    if(have_triggerd_sleep_S >= 5 && !(have_triggerd_sleep_S % 5) && (g_var.acc_flag == FLY_ACC_OFF))
                    {
						//find_task_by_name_or_kill(true, false, true, "c2739.mainframe");
						//find_task_by_name_or_kill(true, false, true, "tencent.qqmusic");
						//find_task_by_name_or_kill(true, false, true, ".flyaudio.media");
						//find_task_by_name_or_kill(true, false, true, "m.android.phone");
						lidbg("+++++ Attention: %ds after gotosleep +++++\n", have_triggerd_sleep_S);
						sprintf(when, "start%d:", have_triggerd_sleep_S);
						kernel_wakelock_print(when);
						userspace_wakelock_action(0, NULL);
                    }
                    break;
#else
                default:
                    if(have_triggerd_sleep_S >= 5 && !(have_triggerd_sleep_S % 5) && (g_var.system_status == FLY_GOTO_SLEEP))//atomic_read(&is_in_sleep) == 1
                    {
						find_task_by_name_or_kill(true, false, true, "c2739.mainframe");
						find_task_by_name_or_kill(true, false, true, "tencent.qqmusic");
						find_task_by_name_or_kill(true, false, true, ".flyaudio.media");
						find_task_by_name_or_kill(true, false, true, "m.android.phone");

					       find_task_by_name_or_kill(true, false, true, "tonavi.amapauto");
						find_task_by_name_or_kill(true, false, true, "locationservice");
						find_task_by_name_or_kill(true, false, true, "i.amapauto:push");
						lidbg_shell_cmd("am force-stop com.autonavi.amapauto");
										
						lidbg("+++++ Attention: %ds after gotosleep +++++\n", have_triggerd_sleep_S);
						sprintf(when, "start%d:", have_triggerd_sleep_S);
						kernel_wakelock_print(when);
						userspace_wakelock_action(0, NULL);
                    }
                    break;
#endif 

                }
            }
	    // lidbg_shell_cmd("pm enable cld.navi.c2739.mainframe");
	     //lidbg_shell_cmd("pm enable cn.flyaudio.media");
            PM_WARN("\n<stop>\n");
        }
    }
    return 1;
}


int thread_power_press_test(void *data)
{
    u32 cnt = 0;
    struct wakeup_source *autosleep_ws;

    ssleep(30);

    autosleep_ws = wakeup_source_register("autosleep");
    if (!autosleep_ws)
        autosleep_ws = wakeup_source_register("autosleep");

    while(1)
    {
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
        ssleep(3);
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
        ssleep(3);
        cnt++;
        lidbg("power_press_test times=%d\n", cnt);
    }

}

int thread_add_whitelist_to_doze(void *data)
{
	struct string_dev *pos;
	char buff[100];

	lidbg_shell_cmd("rm /data/system/deviceidle.xml");

	while(0==g_var.android_boot_completed)
	{
		ssleep(1);
	}

	if(fs_is_file_exist("/flysystem/lib/out/appInternetProtectList.conf"))
	{
		LIST_HEAD(lidbg_appInternetProtect_list);
		fs_fill_list("/flysystem/lib/out/appInternetProtectList.conf", FS_CMD_FILE_LISTMODE, &lidbg_appInternetProtect_list);

		list_for_each_entry(pos, &lidbg_appInternetProtect_list, tmp_list)
		{
			if(pos->yourkey)
			{
				sprintf(buff, "%s%s", "dumpsys deviceidle whitelist +", pos->yourkey);
				lidbg_shell_cmd(buff);
			}
		}
	}
	if(fs_is_file_exist("/flysystem/lib/out/alarmProtectIntervalList.conf"))
	{
		LIST_HEAD(lidbg_alarmProtectInterval_list);
		fs_fill_list("/flysystem/lib/out/alarmProtectIntervalList.conf", FS_CMD_FILE_LISTMODE, &lidbg_alarmProtectInterval_list);
		list_for_each_entry(pos, &lidbg_alarmProtectInterval_list, tmp_list)
		{
			if(pos->yourkey)
			{
				sprintf(buff, "%s%s", "dumpsys deviceidle whitelist +", pos->yourkey);
				lidbg_shell_cmd(buff);
			}
		}
	}
	return 0;
}

static int  lidbg_pm_probe(struct platform_device *pdev)
{
    DUMP_FUN;
    PM_WARN("<==IN==>\n");
    fs_file_separator(PM_INFO_FILE);

#ifdef LIDBG_PM_MONITOR
    PM_WARN("<rm,FlyBootService>\n");
    lidbg_uevent_shell("mount -o remount /system");
    lidbg_uevent_shell("rm	-rf /system/app/FlyBootService.apk");
    lidbg_uevent_shell("rm	-rf /system/lib/modules/out/FlyBootService.apk");
    if(g_var.is_first_update)
    {
        lidbg_uevent_shell("sync");
        ssleep(5);
        lidbg("lidbg_pm_probe call kernel_restart\n");
        kernel_restart(NULL);
    }
#endif

    clear_pm_log();
    lidbg_new_cdev(&pm_nod_fops, "lidbg_pm");
    lidbg_new_cdev(&pm_state_fops, "lidbg_pm_states");
    kthread_run(thread_observer, NULL, "ftf_pmtask");
    LIDBG_MODULE_LOG;

    FS_REGISTER_INT(power_on_off_test, "power_on_off_test", 0, NULL);

    if(power_on_off_test == 1)
    {
        CREATE_KTHREAD(thread_power_press_test, NULL);
    }

#if ANDROID_VERSION >= 600
	CREATE_KTHREAD(thread_add_whitelist_to_doze, NULL);
#endif

    PM_WARN("<==OUT==>\n");
    return 0;
}
static struct platform_device lidbg_pm =
{
    .name               = "lidbg_pm",
    .id                 = -1,
};

static struct platform_driver lidbg_pm_driver =
{
    .probe		= lidbg_pm_probe,
    .driver         = {
        .name = "lidbg_pm",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &lidbg_pm_ops,
#endif
    },
};


static LIST_HEAD(pm_3rd_package_list);
void pm_list_action(void)
{
    char cmd[128];
    int count = 0;
    struct string_dev *pos;
    char *p = NULL;

    list_for_each_entry(pos, &pm_3rd_package_list, tmp_list)
    {
        if(pos->yourkey && (p = strchr(pos->yourkey, ':')))
        {
            ++p;
            count++;
            snprintf(cmd, sizeof(cmd), "%s %s &", shell_head, p );
            lidbg_shell_cmd(cmd);
            LIDBG_WARN("%d/%d -->%s\n", count,delay_ms_3rd, cmd);
            msleep(delay_ms_3rd);
        }
        p = NULL;
    }
}
static int thread_get_3rd_package_name_list_delay(void *data)
{
    DUMP_FUN;
    while(0 == g_var.android_boot_completed)
    {
        ssleep(1);
    };
    ssleep(10);
    lidbg_shell_cmd("pm list packages -3 > "LIDBG_LOG_DIR"pm_3.txt");
    ssleep(5);
    if(fs_fill_list(LIDBG_LOG_DIR"pm_3.txt", FS_CMD_FILE_LISTMODE, &pm_3rd_package_list) < 0)
    {
        lidbg("pm_3rd_package_list,error\n");
        return 1;
    };
    while(1)
    {
        if( !wait_for_completion_interruptible(&pm_3rd_package_wait))
        {
            pm_list_action();
        }
    }
    return 1;
}

static void set_func_tbl(void)
{
    plidbg_dev->soc_func_tbl.pfnLINUX_TO_LIDBG_TRANSFER = linux_to_lidbg_receiver;
    plidbg_dev->soc_func_tbl.pfnSOC_PM_STEP = lidbg_pm_step_call;
    plidbg_dev->soc_func_tbl.pfnHal_Acc_Callback = NULL;
}
#ifdef PLATFORM_msm8226
void find_fb_open_err(char *key_word, void *data)
{
    DUMP_FUN;
    lidbgerr("find key word:find_fb_open_err\n");
    if(g_var.system_status >= FLY_KERNEL_UP)
    {
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
        ssleep(2);
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
    }
    else
        lidbg("find key word:find_fb_open_err.drop.%d\n", g_var.system_status );
}
#endif

static int __init lidbg_pm_init(void)
{
    DUMP_FUN;
    LIDBG_GET;
    set_func_tbl();
    lidbg_mkdir(PM_DIR);

    wake_lock_init(&pm_wakelock, WAKE_LOCK_SUSPEND, "lidbg_pm");
    wake_lock(&pm_wakelock);

    MCU_WP_GPIO_ON;
#if 1//def CONTROL_PM_IO_BY_BP
    MCU_SET_WP_GPIO_SUSPEND;
#endif
    PM_WARN("<set MCU_WP_GPIO_ON>\n");

    CREATE_KTHREAD(thread_gpio_app_status_delay, NULL);
    CREATE_KTHREAD(thread_get_3rd_package_name_list_delay, NULL);

#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974) 
    CREATE_KTHREAD(thread_gps_handle, NULL);
#endif
	
    //lidbg_shell_cmd("echo 8  > /proc/sys/kernel/printk");
    PM_WARN("mediascan.en.0\n");
    lidbg_shell_cmd("setprop persist.lidbg.mediascan.en 1");

#ifdef PLATFORM_msm8226
    lidbg_trace_msg_cb_register("mdss_mdp_overlay_on: Failed to turn on fb0", NULL, find_fb_open_err);
#endif

#ifdef SOC_mt3360
    init_timer(&suspendkey_timer);
    suspendkey_timer.data = 0;
    suspendkey_timer.expires = 0;
    suspendkey_timer.function = suspendkey_timer_isr;
#endif
    platform_device_register(&lidbg_pm);
    platform_driver_register(&lidbg_pm_driver);
    return 0;
}

static void __exit lidbg_pm_exit(void)
{
}

module_init(lidbg_pm_init);
module_exit(lidbg_pm_exit);

MODULE_DESCRIPTION("lidbg.powermanager2014.04");
MODULE_LICENSE("GPL");


