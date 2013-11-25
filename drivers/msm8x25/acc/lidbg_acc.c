#include "lidbg.h"
#if (defined(BOARD_V1) || defined(BOARD_V2))
#include <proc_comm.h>
#else
#include <mach/proc_comm.h>
#endif
#include <mach/clk.h>
#include <mach/socinfo.h>
#include <clock.h>
#include <clock-pcom.h>

LIDBG_DEFINE;

#define RUN_ACCBOOT
#define DEVICE_NAME "lidbg_acc"
#define HAL_SO "/flysystem/lib/hw/flyfa.default.so"
#define FASTBOOT_LOG_PATH "/data/log_fb.txt"


int suspend_state = 0;   //0 :early suspend; 1: suspend 

static DECLARE_COMPLETION(acc_ready);
static DECLARE_COMPLETION(suspend_start);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
DECLARE_MUTEX(lidbg_acc_sem);
#else
DEFINE_SEMAPHORE(lidbg_acc_sem);
#endif

static struct task_struct *acc_task;
static struct task_struct *resume_task;

static spinlock_t  active_wakelock_list_lock;


#ifdef CONFIG_HAS_EARLYSUSPEND
static void acc_early_suspend(struct early_suspend *handler);
static void acc_late_resume(struct early_suspend *handler);
struct early_suspend early_suspend;
#endif

typedef struct
{
	unsigned int  acc_flag;
	u32 resume_count;
} lidbg_acc;

lidbg_acc *plidbg_acc = NULL;
static struct task_struct *suspend_task;

static int pc_clk_is_enabled(int id)
{
	if (msm_proc_comm(PCOM_CLKCTL_RPC_ENABLED, &id, NULL))
		return 0;
	else
		return id;
}


int check_all_clk_disable(void)
{
	int i=P_NR_CLKS-1;
	int ret = 0;
	DUMP_FUN;
	while(i>=0)
	{
		if (pc_clk_is_enabled(i))
		{
		 	lidbg("pc_clk_is_enabled:%3d\n", i);		 	
			ret++;
		}
		i--;
	}
	return ret;
}




int safe_clk[] = {113,105,103,102,95,51,31,20,16,15,12,10,8,4,3,1};

bool find_unsafe_clk(void)
{
	int j,i=P_NR_CLKS-1;
	int ret = 0;
	bool is_safe = 0;
	DUMP_FUN;
	while(i>=0)
	{
		if (pc_clk_is_enabled(i))
		{
			is_safe = 0;
			for(j = 0; j < sizeof(safe_clk); j++)
			{
				if(i == safe_clk[j] )
				{	
					is_safe = 1;
					break;
				}
			}	
			if(is_safe == 0)
			{
				lidbg_fs_log(FASTBOOT_LOG_PATH,"block unsafe clk:%d\n",i);
				ret = 1;
				return ret;
			}
		}
		i--;
	}
	return ret;

}

void show_wakelock(void)
{
    int index = 0;
    struct wakelock_item *pos;
    struct list_head *client_list = &lidbg_wakelock_list;

    if(list_empty(client_list))
        lidbg("<err.lidbg_show_wakelock:nobody_register>\n");
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (pos->name && pos->cunt > 0)
        {
            index++;
            lidbg("<%d.INFO%d:[%s].%d,%d>\n", pos->cunt, index, pos->name, pos->is_count_wakelock, pos->cunt_max);
        }
		if(pos->cunt != 0)
			lidbg_fs_log(FASTBOOT_LOG_PATH,"block wakelock %s\n", pos->name);
    }
}

struct list_head *active_wake_locks = NULL;
void get_wake_locks(struct list_head *p)
{
	active_wake_locks = p;
}

static void list_active_locks(void)
{
 	struct wake_lock *lock;
	int type = 0;
	unsigned long irq_flags;
	if(active_wake_locks == NULL) return;
	DUMP_FUN;
	spin_lock_irqsave(&active_wakelock_list_lock, irq_flags);
	list_for_each_entry(lock, &active_wake_locks[type], link)
	{
		lidbg("%s\n", lock->name);
	}
	spin_unlock_irqrestore(&active_wakelock_list_lock, irq_flags);
}

static void set_func_tbl(void)
{
	 plidbg_dev->soc_func_tbl.pfnSOC_Get_WakeLock = get_wake_locks;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void acc_early_suspend(struct early_suspend *handler)
{
	 lidbg("acc_early_suspend:%d\n", plidbg_acc->resume_count);

	suspend_state = 0;
	check_all_clk_disable();
	
	if(find_unsafe_clk())
	{
	}
	complete(&suspend_start);
}

static void acc_late_resume(struct early_suspend *handler)
{
    DUMP_FUN_ENTER;
	suspend_state = 1;

}
#endif


static int thread_acc_suspend(void *data)
{
	int time_count;
	
	while(1)
	{
		set_current_state(TASK_UNINTERRUPTIBLE);
		if(kthread_should_stop()) break;
		if(1)
		{
			time_count = 0;
			wait_for_completion(&suspend_start);
			while(1)
			{
				msleep(1000);
                time_count++;
						
				if(suspend_state == 0)    //if suspend state always in early suspend
				{
					 if(time_count >= 10)
					 {
						lidbgerr("thread_acc_suspend wait suspend timeout!\n");
						show_wakelock();
						list_active_locks();
						//break;
					 }
				}
				else
					break;
			}
		}
	}
	return 0;
}

ssize_t  acc_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    return size;
}

ssize_t  acc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{

		char data_rec[32];
		char *p=NULL;
		int len=count;
		
		lidbg("acc_write.\n");
		
		if (copy_from_user( data_rec, buf, count))
		{
		printk("copy_from_user ERR\n");
		}
		
		if((p = memchr(data_rec, '\n', count)))
		{
			len=p - data_rec;
			*p='\0';
		}
		else
			data_rec[len] =  '\0';

		printk("acc_nod_write:==%d====[%s]\n", len, data_rec);

		// processing data
		 if(!strcmp(data_rec, "acc_on"))
		{
			printk("******goto acc_on********\n");
			SOC_Write_Servicer(CMD_ACC_ON);
			lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT,NOTIFIER_MINOR_ACC_ON));
		}
		else if(!strcmp(data_rec, "acc_off"))
		{
			printk("******goto acc_off********\n");
			SOC_Write_Servicer(CMD_ACC_OFF);
			lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT,NOTIFIER_MINOR_ACC_OFF));
		}
		else if(!strcmp(data_rec, "power"))
		{
			printk("******goto fastboot********\n");
			SOC_Write_Servicer(CMD_FAST_POWER_OFF);
		}		
		
	return count;
}


int acc_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int acc_release(struct inode *inode, struct file *filp)
{
    return 0;
}

void cb_password_poweroff(char *password )
{
	SOC_Write_Servicer(CMD_FAST_POWER_OFF);
}

static int  acc_probe(struct platform_device *pdev)
{
	int ret;
	DUMP_FUN_ENTER;
	plidbg_acc = kmalloc(sizeof(lidbg_acc), GFP_KERNEL);
	if (!plidbg_acc)
	{
		ret = -ENODEV;
		goto fail_mem;
	}

	 plidbg_acc->resume_count = 0;
	 
	if(!fs_is_file_exist(HAL_SO))
	{
		FORCE_LOGIC_ACC;
	}
	
    spin_lock_init(&active_wakelock_list_lock);

	fs_regist_state("acc_times", (int*)&plidbg_acc->resume_count);
	te_regist_password("001200", cb_password_poweroff);

	fs_file_separator(FASTBOOT_LOG_PATH);
	
	return 0;

	fail_mem:
		return ret;
}

static struct file_operations dev_fops =
{
    .owner	=	THIS_MODULE,
    .open   =   acc_open,
    .read   =   acc_read,
    .write  =   acc_write,
    .release =  acc_release,
};

static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,

};

static int  acc_remove(struct platform_device *pdev)
{
    return 0;
}

#ifdef CONFIG_PM
static int acc_resume(struct device *dev)
{
    DUMP_FUN_ENTER;

    lidbg("fastboot_resume:%d\n", ++plidbg_acc->resume_count);
    return 0;

}

static int acc_suspend(struct device *dev)
{
    DUMP_FUN_ENTER;
    suspend_state = 1;
    return 0;

}

static struct dev_pm_ops acc_pm_ops =
{
    .suspend	= acc_suspend,
    .resume		= acc_resume,
};
#endif

static struct platform_driver acc_driver =
{
    .probe		= acc_probe,
    .remove     = acc_remove,
    .driver         = {
        .name = "lidbg_acc1",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &acc_pm_ops,
#endif
    },
};

static struct platform_device lidbg_acc_device =
{
    .name               = "lidbg_acc1",
    .id                 = -1,
};
   

static int __init acc_init(void)
{
	int ret;
	LIDBG_GET;
	set_func_tbl();
    platform_device_register(&lidbg_acc_device);

    platform_driver_register(&acc_driver);

	INIT_COMPLETION(acc_ready);
	ret = misc_register(&misc);

#ifdef CONFIG_HAS_EARLYSUSPEND
	{
		early_suspend.level =EARLY_SUSPEND_LEVEL_DISABLE_FB + 5; //the later the better
		early_suspend.suspend = acc_early_suspend;
		early_suspend.resume = acc_late_resume;
		register_early_suspend(&early_suspend);
	}
#endif

	 INIT_COMPLETION(suspend_start);
	suspend_task = kthread_create(thread_acc_suspend, NULL, "suspend_task");
	if(IS_ERR(suspend_task))
	{
		lidbg("Unable to start kernel suspend_task.\n");

	}
	else wake_up_process(suspend_task);

	lidbg_chmod("/dev/lidbg_acc");
	
	lidbg (DEVICE_NAME"acc dev_init\n");

	return ret;
}

static void __exit acc_exit(void)
{
	misc_deregister(&misc);
    	lidbg (DEVICE_NAME"acc dev_exit\n");
}

module_init(acc_init);
module_exit(acc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("lidbg_acc driver");

