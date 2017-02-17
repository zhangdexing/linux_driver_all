#include "lidbg.h"

LIDBG_DEFINE;

#include "lidbg.h"
int temp_log_freq = 10;
//static int fan_onoff_temp;
static int cpu_temp_time_minute = 20;
static bool is_cpu_temp_enabled = false;
int cpu_temp_show = 0;
static int temp_offset = 0;
int antutu_test = 0;
int antutu_temp_offset = 0;
int normal_temp_offset = 0;
int ctrl_max_freq = 0;
int mt35xx_old_state_cold = 0;
bool fan_run_status = false;

#define FREQ_MAX_NODE    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define CPU_MAX_NODE    "/sys/devices/system/cpu/cpu0/core_ctl/max_cpus"
#define CPU_MIN_NODE    "/sys/devices/system/cpu/cpu0/core_ctl/min_cpus"
#define TEMP_LOG_PATH 	 LIDBG_LOG_DIR"log_ct.txt"
#define TEMP_FREQ_TEST_RESULT LIDBG_LOG_DIR"lidbg_temp_freq.txt"
#define TEMP_FREQ_COUNTER LIDBG_LOG_DIR"freq_tmp.txt"
u32 get_scaling_max_freq(void);
char *get_cpu_status(void);

int thread_limit_temp(void *data)
{
    int mem_temp, cpu_temp;
    lidbg_chmod(CPU_MAX_NODE);

    while(g_hw.thermal_ctrl_en == 0)
    {
        mem_temp = soc_temp_get(g_hw.mem_sensor_num);
        cpu_temp = soc_temp_get(g_hw.cpu_sensor_num);

        pr_debug( "%s:%d,%d,%d,%d,%s\n", __FUNCTION__, mem_temp, cpu_temp, get_scaling_max_freq(), SOC_Get_CpuFreq(), get_cpu_status());
#ifdef PLATFORM_msm8226
        lidbg_readwrite_file(FREQ_MAX_NODE, NULL, "600000", strlen("600000"));
#elif defined(PLATFORM_msm8909)
        lidbg_readwrite_file(FREQ_MAX_NODE, NULL, "533333", strlen("533333"));
#elif defined(PLATFORM_msm8974)
        lidbg_readwrite_file(FREQ_MAX_NODE, NULL, "652800", strlen("652800"));
#endif
        msleep(1000);
    }
    return 0;
}



static int lidbg_temp_event(struct notifier_block *this,
                            unsigned long event, void *ptr)
{
    int cpu_temp;
    DUMP_FUN;

    cpu_temp = soc_temp_get(g_hw.cpu_sensor_num);


    switch (event)
    {

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_OFF):
    {
        //cpufreq_update_policy(0);
        if(cpu_temp > 90)
            break;

        g_hw.thermal_ctrl_en = 0;
        CREATE_KTHREAD(thread_limit_temp, NULL);
        break;

    }
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_ON):
    {
        g_hw.thermal_ctrl_en = 1;
#ifdef PLATFORM_msm8974
        lidbg_readwrite_file(CPU_MAX_NODE, NULL, "2265600", strlen("2265600"));
#endif
        //cpufreq_update_policy(0);
        break;
    }
    }

    return NOTIFY_DONE;
}

int get_file_int(char *file)
{
    char cpu_temp[3];
    int temp = -1;
    fs_file_read(file, cpu_temp, 0, sizeof(cpu_temp));
    temp = simple_strtoul(cpu_temp, 0, 0);
    return temp;
}

#if 0
int soc_temp_get(void)
{
#ifdef SOC_msm8x26
    static long temp;
    static struct tsens_device tsens_dev;
    tsens_dev.sensor_num = g_hw.sensor_num;
    tsens_get_temp(&tsens_dev, &temp);
    return (int)temp;
#else
    if(g_hw.cpu_freq_temp_node != NULL)
        return get_file_int(g_hw.cpu_freq_temp_node);
    else
        return 0;
#endif
}
#endif

u32 get_scaling_max_freq(void)
{
    static char max_freq[32];
    static u32 tmp;
    memset(max_freq, 0, sizeof(max_freq));
    lidbg_readwrite_file(FREQ_MAX_NODE, max_freq, NULL, 32);
    tmp = simple_strtoul(max_freq, 0, 0);
    //lidbg("scaling_max_freq=%d,%s\n", tmp,max_freq);
    return tmp;
}

char *get_cpu_status(void)
{
    static char cpu_status[16];
    memset(cpu_status, 0, sizeof(cpu_status));
    lidbg_readwrite_file("/sys/devices/system/cpu/online", cpu_status, NULL, 16);
    return cpu_status;
}


void log_temp(void)
{

    static int old_temp = 0, cur_temp = 0;
    int tmp;
    g_var.temp = cur_temp = soc_temp_get(g_hw.mem_sensor_num);
    tmp = cur_temp - old_temp;

    if(
        ((temp_log_freq != 0) && (ABS(tmp) >= temp_log_freq))
        || ((g_var.temp > 100) && (ABS(tmp) >= 2))
        || ((g_var.temp > 90) && (ABS(tmp) >= 3))
        || ((g_var.temp > 80) && (ABS(tmp) >= 5))
    )
    {

        lidbg_fs_log(TEMP_LOG_PATH, "%d,%d,%d\n", cur_temp, get_scaling_max_freq(), SOC_Get_CpuFreq());
        old_temp = cur_temp;
    }


}

int thread_show_temp(void *data)
{
    while(1)
    {
        int cur_temp = soc_temp_get(g_hw.mem_sensor_num);
        lidbg( "%d,%d,%d,%s\n", cur_temp, get_scaling_max_freq(), SOC_Get_CpuFreq(), get_cpu_status());
        msleep(1000);
    }
}
void cb_kv_show_temp(char *key, char *value)
{
    CREATE_KTHREAD(thread_show_temp, NULL);
}
//EXPORT_SYMBOL(cb_kv_show_temp);





void set_system_performance(int type)
{
    fs_mem_log("set_system_performance:%d\n", type);

    if(type == 3)//top performance
    {
#ifdef PLATFORM_msm8974
        //lidbg_readwrite_file(FREQ_MAX_NODE, NULL, "2265600", strlen("2265600"));
#endif
        set_cpu_governor(1);
        temp_offset = - antutu_temp_offset;
    }
    else if(type == 2)
    {
#ifdef PLATFORM_msm8974
        //lidbg_readwrite_file(FREQ_MAX_NODE, NULL, "1958400", strlen("1958400"));
#endif
        set_cpu_governor(0);
        temp_offset = -15;
    }
    else if(type == 1)//low performance
    {
#ifdef PLATFORM_msm8974
        //   lidbg_readwrite_file(FREQ_MAX_NODE, NULL, "2265600", strlen("2265600"));
#endif
        set_cpu_governor(0);
        temp_offset = -normal_temp_offset;
        //temp_offset = -25;//better for some machine
    }
}

int thread_stop_boot_freq_ctrl(void *data)
{
    while(0 == g_var.android_boot_completed)
    {
        ssleep(1);
    }
    ssleep(10); //wait boot_freq_ctrl finish
    lidbg("cat /proc/interrupt_mode_init.10\n");
    if(g_hw.thermal_ctrl_en == 1)
        lidbg_shell_cmd("cat /proc/freq_ctrl_stop &");
    if(g_var.recovery_mode == 0)
        lidbg_shell_cmd("cat /proc/interrupt_mode_init &");
}

int thread_thermal(void *data)
{
    int cur_temp, i, max_freq, maxcpu, mincpu, cpu_temp, cpufreq;
    DUMP_FUN;
    lidbg_shell_cmd("chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq &");
    //set_cpu_governor(0);

    if(g_var.recovery_mode == 1)
    {
        while(1)
        {
            //set_cpu_governor(1);
            max_freq = get_scaling_max_freq();
            cur_temp = soc_temp_get(g_hw.cpu_sensor_num);
            lidbg("cpu_temp=%d,freq=%d,maxfreq=%d,%s\n", cur_temp, SOC_Get_CpuFreq(), max_freq, get_cpu_status());
            ssleep(2);
        }
    }

    temp_init();

    if(0)//if(g_var.recovery_mode == 1)
    {
        int cpu_freq;
        while(1)
        {
            if(g_hw.cpu_freq_recovery_limit != NULL)
                lidbg_readwrite_file(FREQ_MAX_NODE, NULL, g_hw.cpu_freq_recovery_limit, strlen(g_hw.cpu_freq_recovery_limit));
            else
                lidbg("g_hw.fly_parameter_node == NULL,return\n");
            set_cpu_governor(0);
            ssleep(5);
            cpu_freq = SOC_Get_CpuFreq();
            cur_temp = soc_temp_get(g_hw.mem_sensor_num);
            lidbg("cpufreq=%d,mem_temp=%d\n", cpu_freq, cur_temp);
        }
    }

    while(is_cpu_temp_enabled)
    {
        lidbg("set max freq to: disabled\n");
        ssleep(10);
    }

#if defined(PLATFORM_msm8226) || defined(PLATFORM_msm8974)
    set_system_performance(1);
#ifdef PLATFORM_ID_6
    set_system_performance(2);
#endif
#else
    temp_offset = 0;
#endif

    CREATE_KTHREAD(thread_stop_boot_freq_ctrl, NULL);

    cur_temp = soc_temp_get(g_hw.mem_sensor_num);
    lidbg("lidbg freq ctrl start,%d,%d\n", cur_temp, get_scaling_max_freq());

    if(cpu_temp_show == 1)
        CREATE_KTHREAD(thread_show_temp, NULL);

    while(!kthread_should_stop())
    {
        if(g_hw.thermal_ctrl_en == 1)
            msleep(250);
        else
            msleep(1000);

        log_temp();
        cur_temp = soc_temp_get(g_hw.mem_sensor_num);
        cpu_temp = soc_temp_get(g_hw.cpu_sensor_num);
        maxcpu = get_file_int(CPU_MAX_NODE);
        mincpu = get_file_int(CPU_MIN_NODE);
        max_freq = get_scaling_max_freq();
        cpufreq = SOC_Get_CpuFreq();

        if(0 == g_var.android_boot_completed)
            lidbg("max_freq=%d,maxcpu=%d,mincpu=%d,mem_temp=%d,cpu_temp=%d,freq=%d,status=%s", max_freq, maxcpu, mincpu, cur_temp, cpu_temp, cpufreq, get_cpu_status());
        pr_debug("max_freq=%d,maxcpu=%d,mincpu=%d,mem_temp=%d,cpu_temp=%d,freq=%d,status=%s", max_freq, maxcpu, mincpu, cur_temp, cpu_temp, cpufreq, get_cpu_status());

        if(0)
            //fan ctrl
        {
            if( (cur_temp > g_hw.fan_onoff_temp)  &&
                    (g_var.system_status != FLY_DEVICE_DOWN) &&
                    (g_var.system_status != FLY_ANDROID_DOWN) &&
                    (g_var.system_status != FLY_GOTO_SLEEP) &&
                    (g_var.system_status != FLY_KERNEL_DOWN)

              )//on
            {
                if(fan_run_status == false)
                {
                    fan_run_status = true;
                    LPC_CMD_FAN_ON;
                    lidbg("AIR_ON:%d\n", cur_temp);
                }
            }
            else //off
            {
                if(fan_run_status)
                {
                    fan_run_status = false;
                    LPC_CMD_FAN_OFF;
                    lidbg( "AIR_OFF:%d\n", cur_temp);
                }
            }


        }

        //temp_offset = -25;

        if(g_hw.thermal_ctrl_en == 0)
        {
            if(cpu_temp > 85)
                lidbg("temp>85:temp:%d,freq:%d,cpu:%s\n", cpu_temp, SOC_Get_CpuFreq(), get_cpu_status());


#ifdef SOC_mt35x
            {
                if( (0 == g_var.android_boot_completed))
                    lidbg("mt3561temp:temp:%d,freq:%d,cpu:%s\n", cpu_temp, SOC_Get_CpuFreq(), get_cpu_status());
                if( (1 == g_var.android_boot_completed))
                {
                    if(cpu_temp > 90)
                    {
                        if(mt35xx_old_state_cold)
                        {
                            //lidbg("*158#047--set cpu run in powersave mode\n");
                            lidbg("mt3561temp.powersave:temp:%d,freq:%d,cpu:%s\n", cpu_temp, SOC_Get_CpuFreq(), get_cpu_status());
                            lidbg_shell_cmd("echo appcmd *158#047 > /dev/lidbg_drivers_dbg0");
                            mt35xx_old_state_cold = 0;
                        }
                    }
                    if(cpu_temp < 85)
                    {
                        if(!mt35xx_old_state_cold)
                        {
                            lidbg("mt3561temp.performance:temp:%d,freq:%d,cpu:%s\n", cpu_temp, SOC_Get_CpuFreq(), get_cpu_status());
                            lidbg_shell_cmd("echo appcmd *158#046 > /dev/lidbg_drivers_dbg0");
                            mt35xx_old_state_cold = 1;
                        }
                    }
                }
            }
#endif
            //continue
            continue;
        }

        if(0)goto thermal_ctrl;


thermal_ctrl:

        //max_freq = get_scaling_max_freq();
        //lidbg("MSM_THERM: %d\n",cur_temp);
        for(i = 0; i < SIZE_OF_ARRAY(g_hw.cpu_freq_thermal); i++)
        {
            if((g_hw.cpu_freq_thermal[i].temp_low == 0) || (g_hw.cpu_freq_thermal[i].temp_high == 0))
                break;

            if((cur_temp >= (g_hw.cpu_freq_thermal[i].temp_low + temp_offset) ) && (cur_temp <= (g_hw.cpu_freq_thermal[i].temp_high + temp_offset) ))
            {
                if(max_freq != g_hw.cpu_freq_thermal[i].limit_freq)
                {
                    lidbg_readwrite_file(FREQ_MAX_NODE, NULL, g_hw.cpu_freq_thermal[i].limit_freq_string, strlen(g_hw.cpu_freq_thermal[i].limit_freq_string));
                    if(g_hw.gpu_max_freq_node != NULL)
                        lidbg_readwrite_file(g_hw.gpu_max_freq_node, NULL, g_hw.cpu_freq_thermal[i].limit_gpu_freq_string, strlen(g_hw.cpu_freq_thermal[i].limit_gpu_freq_string));
                    lidbg("set max freq to: %d,mem_temp:%d,cpu_temp:%d,temp_offset:%d,cpufreq=%d\n", g_hw.cpu_freq_thermal[i].limit_freq, cur_temp, cpu_temp, temp_offset, SOC_Get_CpuFreq());
                }
                if(g_hw.cpu_freq_thermal[i].max_cpu > 0)
                {
                    if(maxcpu != g_hw.cpu_freq_thermal[i].max_cpu)
                    {
                        char max_cpu[10];
                        sprintf(max_cpu, "%d", g_hw.cpu_freq_thermal[i].max_cpu);
                        lidbg_readwrite_file(CPU_MAX_NODE, NULL, max_cpu, strlen(max_cpu));
                        lidbg("set cpus max to:%d/%d\n", g_hw.cpu_freq_thermal[i].max_cpu, get_file_int(CPU_MAX_NODE));
                    }
                    if(mincpu != g_hw.cpu_freq_thermal[i].max_cpu)
                    {
                        char max_cpu[10];
                        sprintf(max_cpu, "%d", g_hw.cpu_freq_thermal[i].max_cpu);
                        lidbg_readwrite_file(CPU_MIN_NODE, NULL, max_cpu, strlen(max_cpu));
                        lidbg("set cpus min to:%d/%d\n", g_hw.cpu_freq_thermal[i].max_cpu, get_file_int(CPU_MIN_NODE));
                    }

                }
                break;
            }
        }
    }
    return 0;
}

static char temp_freq_test_str[256];
int thread_start_cpu_tmp_test(void *data)
{
    char *group[15] = {NULL}, buff[56] = {0};
    int group_num  = 0, freq_pos = 0, int_time_count = 0;

    if (g_hw.cpu_freq_list == NULL)
    {
        lidbg("g_hw.cpu_freq_list == NULL,return\n");
        return 0;
    }

    ssleep(60);//waritting for system boot complete
    strcpy(temp_freq_test_str, g_hw.cpu_freq_list);
    group_num = lidbg_token_string(temp_freq_test_str, ",", group) ;

    if((freq_pos = get_file_int(TEMP_FREQ_COUNTER)) < 0)
        freq_pos = 0;
    if(freq_pos >= group_num)
        goto err;
    fs_clear_file(TEMP_FREQ_COUNTER);
    fs_string2file(0, TEMP_FREQ_COUNTER, "%d", freq_pos + 1);

    freq_pos = group_num - freq_pos - 1;

    lidbg("%d,start_cpu_tmp_test: %s\n", cpu_temp_time_minute, g_hw.cpu_freq_list);

    lidbg_shell_cmd("am start -n com.into.stability/com.into.stability.Run");
    ssleep(2);
    lidbg_shell_cmd("am start -n com.into.stability/.TestClassic");

    lidbg_readwrite_file(FREQ_MAX_NODE, NULL, group[freq_pos], strlen(group[freq_pos]));
    fs_file_separator(TEMP_FREQ_TEST_RESULT);

    sprintf(buff, "[%s]%d,%d", group[freq_pos], freq_pos, group_num);
    lidbg_toast_show("temp:", buff);

    while(1)
    {
        int cur_temp;
        int_time_count++;
        cur_temp = soc_temp_get(g_hw.cpu_sensor_num);
        fs_string2file(100, TEMP_FREQ_TEST_RESULT, "%d,temp=%d,time=%d,freq=%s:\n", freq_pos, cur_temp, int_time_count * 2, group[freq_pos]);
        lidbg("%d,temp=%d,time=%d,freq=%s:\n", freq_pos, cur_temp, int_time_count * 2, group[freq_pos]);
        msleep(1000);
        if(int_time_count  > cpu_temp_time_minute * 60)
            lidbg_reboot();
    }
    return 0;

err:
    is_cpu_temp_enabled = false;
    sprintf(buff, "cpu_tmp_test.stop:%d,%d", freq_pos, group_num);
    lidbg_toast_show("temp:", buff);
    lidbg_rm(TEMP_FREQ_COUNTER);
    fs_string2file(100, TEMP_FREQ_TEST_RESULT, "-------stop-------\n");
    return 0;
}




void cb_kv_cpu_temp_test(char *key, char *value)
{
    if(value && *value == '1')
    {
        lidbg_rm(TEMP_FREQ_TEST_RESULT);
        fs_file_write(TEMP_FREQ_COUNTER, true, "0", 0, strlen("0"));
        ssleep(1);
        lidbg_reboot();
    }
    else
        fs_mem_log("cb_kv_cpu_temp_test:fail,%s\n", value);
}
void temp_init(void)
{
    FS_REGISTER_KEY( "cpu_temp_test", cb_kv_cpu_temp_test);
    fs_register_filename_list(TEMP_FREQ_TEST_RESULT, true);
    FS_REGISTER_INT(cpu_temp_time_minute, "cpu_temp_time_minute", 20, NULL);
    FS_REGISTER_INT(temp_log_freq, "temp_log_freq", 50, NULL);
    FS_REGISTER_INT(cpu_temp_show, "cpu_temp_show", 0, cb_kv_show_temp);
    //FS_REGISTER_INT(antutu_test, "antutu_test", 0, NULL);
    FS_REGISTER_INT(antutu_temp_offset, "antutu_temp_offset", 10, NULL);
    FS_REGISTER_INT(normal_temp_offset, "normal_temp_offset", 20, NULL);

    if(fs_is_file_exist(TEMP_FREQ_COUNTER))
    {
        is_cpu_temp_enabled = true;
        CREATE_KTHREAD(thread_start_cpu_tmp_test, NULL);
    }

    //if(antutu_test)
    //	CREATE_KTHREAD(thread_antutu_test, NULL);

    fs_register_filename_list(TEMP_LOG_PATH, true);
    fs_regist_state("cpu_temp", &(g_var.temp));

    //FS_REGISTER_INT(fan_onoff_temp, "fan_onoff_temp", 65, NULL);
}



ssize_t  temp_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
#if 0
    int temp_val;
    temp_val = soc_temp_get(g_hw.mem_sensor_num);
    if(size > 4)
        size = 4;
    if (copy_to_user(buffer, &temp_val, size))
    {
        lidbg("copy_to_user ERR\n");
    }
    return size;
#else
    char buff[16] = {0};
    sprintf(buff, "%d %d", soc_temp_get(g_hw.mem_sensor_num), soc_temp_get(g_hw.cpu_sensor_num));
    if (copy_to_user(buffer, buff, strlen(buff)))
    {
        lidbg("copy_to_user ERR\n");
    }
    return size;

#endif
}

ssize_t  temp_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{

    return size;
}

int temp_open (struct inode *inode, struct file *filp)
{
    return 0;
}


static  struct file_operations temp_nod_fops =
{
    .owner = THIS_MODULE,
    .write = temp_write,
    .read = temp_read,
    .open =  temp_open,

};

static int temp_ops_suspend(struct device *dev)
{
    lidbg("-----------temp_suspend------------\n");
    DUMP_FUN;

    return 0;
}

static int temp_ops_resume(struct device *dev)
{
    int cur_temp, max_freq, maxcpu, mincpu, cpu_temp;
    DUMP_FUN;

    cur_temp = soc_temp_get(g_hw.mem_sensor_num);
    cpu_temp = soc_temp_get(g_hw.cpu_sensor_num);
    maxcpu = get_file_int(CPU_MAX_NODE);
    mincpu = get_file_int(CPU_MIN_NODE);
    max_freq = get_scaling_max_freq();

    lidbg("mem_temp=%d,cpu_temp=%d,freq=%d,max_freq=%d,maxcpu=%d,mincpu=%d,status=%s", cur_temp, cpu_temp, SOC_Get_CpuFreq(), max_freq, maxcpu, mincpu, get_cpu_status());

    return 0;
}
static struct dev_pm_ops temp_ops =
{
    .suspend	= temp_ops_suspend,
    .resume	= temp_ops_resume,
};


static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_temp_event,
};

static int  cpufreq_callback(struct notifier_block *nfb,
                             unsigned long event, void *data)
{
    //struct cpufreq_policy *policy = data;

    switch (event)
    {
    case CPUFREQ_NOTIFY:
        if((ctrl_max_freq != 0) /*&& (ctrl_en)*/)
        {
            //policy->max = ctrl_max_freq;
            //policy->min = 300000;

            //lidbg("%s: mitigating cpu %d to freq max: %u min: %u\n",
            //KBUILD_MODNAME, policy->cpu, policy->max, policy->min);
            break;
        }

    }
    return NOTIFY_OK;
}
static struct notifier_block cpufreq_notifier =
{
    .notifier_call = cpufreq_callback,
};

static int temp_probe(struct platform_device *pdev)
{
    int ret = 0;
    lidbg("-----------temp_probe------------\n");
    lidbg_new_cdev(&temp_nod_fops, "lidbg_temp0");
    if(g_hw.thermal_ctrl_en)
    {
        register_lidbg_notifier(&lidbg_notifier);
        ret = cpufreq_register_notifier(&cpufreq_notifier,
                                        CPUFREQ_POLICY_NOTIFIER);
        if (ret)
            lidbg("%s: cannot register cpufreq notifier\n",
                  KBUILD_MODNAME);
    }

    return 0;
}
static int temp_remove(struct platform_device *pdev)
{
    return 0;
}
static struct platform_device temp_devices =
{
    .name			= "lidbg_temp",
    .id 			= 0,
};

static struct platform_driver temp_driver =
{
    .probe = temp_probe,
    .remove = temp_remove,
    .driver = 	{
        .name = "lidbg_temp",
        .owner = THIS_MODULE,
        .pm = &temp_ops,
    },
};




static int  cpu_temp_init(void)
{
    printk(KERN_WARNING "chdrv_init\n");
    LIDBG_GET;
    CREATE_KTHREAD(thread_thermal, NULL);
    platform_device_register(&temp_devices);
    platform_driver_register(&temp_driver);
    return 0;


}

static void  cpu_temp_exit(void)
{
    printk("chdrv_exit\n");

}

module_init(cpu_temp_init);
module_exit(cpu_temp_exit);

//EXPORT_SYMBOL(thread_antutu_test);
EXPORT_SYMBOL(set_system_performance);



MODULE_AUTHOR("fly, <fly@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");
