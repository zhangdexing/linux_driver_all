
#include "lidbg_target.h"
#define  RECOVERY_MODE_DIR "/sbin/recovery"


bool is_recovery_mode = 0;
bool ctrl_en = 1;

struct thermal_ctrl
{
    int temp_low;
    int temp_high;
    u32 limit_freq;
    char *limit_freq_string;
    int max_cpu;
};

#ifdef PLATFORM_MSM8226
struct thermal_ctrl cpu_thermal[] =
{
    //{1,  60,  1401600,"1401600"},
    { -500, 50,  1344000, "1344000",-1},
    {51, 55,  1190400, "1190400",-1},
    {56, 60,  998400, "998400",-1},
    {61, 70,  600000, "600000",-1},
    {71, 500, 300000, "300000",-1},
    {0, 0, 0, "0"}, //end flag

};

#elif defined(PLATFORM_MSM8974)

struct thermal_ctrl cpu_thermal[] =
{
    { -500,  50,  2265600, "2265600",-1},
    {61, 65,  1958400, "1958400",-1},
    {66, 70,  1728000, "1728000",-1},
    {71, 75,  1497600, "1497600",-1},
    {76, 80,  1267200, "1267200",-1},
    {81, 85,  1190400, "1190400",-1},
    {86, 90,  960000, "960000",-1},
    {91, 95,  729600, "729600",-1},
    {96, 500, 300000, "300000",-1},
    {0, 0, 0, "0"} //end flag
};

#elif defined(PLATFORM_MSM8909)
struct thermal_ctrl cpu_thermal[] =
{
    {-500,  85,  1267200, "1267200",4},
    {86,    89,  1094400, "1094400",4},
    {90,    92, 800000,  "800000",4},
    {93,   500, 533333,  "533333",2},     

    {0,     0, 0, "0"} //end flag
};

#endif


#define FREQ_MAX_NODE "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define CPU_MAX_NODE    "/sys/devices/system/cpu/cpu0/core_ctl/max_cpus"

int ctrl_max_freq = 0;

u32 get_file_int(char *file)
{
    static char max_freq[32];
    static u32 tmp;
    lidbg_readwrite_file(file, max_freq, NULL, 32);
    tmp = simple_strtoul(max_freq, 0, 0);
    //lidbg("scaling_max_freq=%d,%s\n", tmp,max_freq);
    return tmp;
}

#define CPU_STATUS_NODE "/sys/devices/system/cpu/online"
char* get_cpu_status(void)
{
    static char cpu_status[32];
    lidbg_readwrite_file(CPU_STATUS_NODE, cpu_status, NULL, 32);
    return cpu_status;
}


void freq_limit_exit(void)
{
    int max_freq;
    lidbg("freq_limit_exit\n");
    ctrl_max_freq = cpu_thermal[0].limit_freq;
    cpufreq_update_policy(0);
    lidbg_readwrite_file(CPU_MAX_NODE, NULL, cpu_thermal[0].limit_freq_string, strlen(cpu_thermal[0].limit_freq_string));
    msleep(100);
    max_freq = get_file_int(FREQ_MAX_NODE);
    lidbg("max_freq=%d,limit_freq=%d\n", max_freq, cpu_thermal[0].limit_freq);
    ctrl_en = 0;

}



static int thread_freq_limit(void *data)
{
    int count = 0;
    int i = 0;

    struct tsens_device tsens_dev;
    //cpu0 temp_sensor_id
#if defined(PLATFORM_MSM8226) || defined(PLATFORM_MSM8974)
    tsens_dev.sensor_num = 5;
#elif defined(PLATFORM_MSM8909)
    tsens_dev.sensor_num = 3;
#endif
    lidbg("create kthread \n");

    while(ctrl_en)
    {
        int tmp,i;
        long temp;
        u32 max_freq = 0;
	for(i = 0; i < 4; i++)
	{
	    tmp = cpufreq_get(i); //cpufreq.c
	    if(tmp > 0)
	        break;
	}

        max_freq = get_file_int(FREQ_MAX_NODE);

        tsens_get_temp(&tsens_dev, &temp);//cpu0 temp
        lidbg("cpufreq=%d,maxfreq=%d,cpu0_temp = %ld,status=%s", tmp, max_freq, temp, get_cpu_status());
        for(i = 0; i < SIZE_OF_ARRAY(cpu_thermal); i++)
        {
            if((cpu_thermal[i].temp_low == 0) || (cpu_thermal[i].temp_high == 0))
                break;

            if((temp >= cpu_thermal[i].temp_low ) && (temp <= cpu_thermal[i].temp_high )
                    && (max_freq != cpu_thermal[i].limit_freq))
            {

                lidbg_readwrite_file(FREQ_MAX_NODE, NULL, cpu_thermal[i].limit_freq_string, strlen(cpu_thermal[i].limit_freq_string));
		if(cpu_thermal[i].max_cpu>0)
		{
		char max_cpu[10];
		sprintf(max_cpu,"%d",cpu_thermal[i].max_cpu);
		lidbg_readwrite_file(CPU_MAX_NODE, NULL, max_cpu, strlen(max_cpu));
		}
                lidbg("kernel:set max freq to: %d,temp:%ld,max_cpu:%d\n", cpu_thermal[i].limit_freq, temp,get_file_int(CPU_MAX_NODE));

                ctrl_max_freq = cpu_thermal[i].limit_freq;
                cpufreq_update_policy(0);
                break;
            }
        }

        count++;

        if(count >= 20 * 4)
            is_recovery_mode = is_file_exist(RECOVERY_MODE_DIR);

        if((count >= 90 * 4) && (temp < 85))
        {
            if(is_recovery_mode == 0)
                is_recovery_mode = is_file_exist(RECOVERY_MODE_DIR);
            if(is_recovery_mode == 1)
            {
                msleep(1000);
                continue;
            }

	    freq_limit_exit();

            return 1;
        }
        msleep(250);

    }
    lidbg("thread_freq_limit stoped\n");
    return 1;
}


static int  cpufreq_callback(struct notifier_block *nfb,
                             unsigned long event, void *data)
{
    struct cpufreq_policy *policy = data;

    switch (event)
    {
    case CPUFREQ_NOTIFY:
        if((ctrl_max_freq != 0) /*&& (ctrl_en)*/)
        {
            policy->max = ctrl_max_freq;
            policy->min = 300000;

            if(is_recovery_mode == 1)
                policy->min = ctrl_max_freq;

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


#define PROC_READ_CHECK {static int len_check = 0;if ((len_check++)%2) return 0;}
static int  freq_ctrl_stop_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{

    PROC_READ_CHECK;
    lidbg("freq_ctrl_stop_proc\n");
    ctrl_en = 0;
    return 1;
}
static const struct file_operations freq_ctrl_fops =
{
    .read  = freq_ctrl_stop_proc,
};



void freq_ctrl_start(void)
{
    struct task_struct *task;

    int ret = 0;
    is_recovery_mode = 0;
    proc_create("freq_ctrl_stop", 0, NULL, &freq_ctrl_fops);
    ret = cpufreq_register_notifier(&cpufreq_notifier,
                                    CPUFREQ_POLICY_NOTIFIER);
    if (ret)
        pr_err("%s: cannot register cpufreq notifier\n",
               KBUILD_MODNAME);

    task = kthread_create(thread_freq_limit, NULL, "thread_freq_limit");
    if(IS_ERR(task))
    {
        lidbg("Unable to start thread.\n");
    }
    else wake_up_process(task);


}


