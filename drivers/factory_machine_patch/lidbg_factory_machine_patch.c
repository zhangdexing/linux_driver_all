
#include "lidbg.h"
LIDBG_DEFINE;

#define TAG "faclog:"
static int is_wlan_open = 0;
static int wlan_ctrl_en = 1;

static int lidbg_factory_event(struct notifier_block *this,unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_OFF):
        lidbg(TAG"wifi state");
        lidbg_shell_cmd("echo getprop wlan.driver.status $(getprop wlan.driver.status) > /dev/lidbg_factory_patch0");
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
        if((is_wlan_open)&&(wlan_ctrl_en))
        {
            lidbg_shell_cmd("svc wifi disable &");
            lidbg(TAG"wifi disable\n");
        }
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_ANDROID_DOWN):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_GOTO_SLEEP):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_KERNEL_DOWN):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_KERNEL_UP):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_ANDROID_UP):
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_UP):
        if((is_wlan_open)&&(wlan_ctrl_en))
        {
            lidbg_shell_cmd("svc wifi enable &");
            lidbg(TAG"wifi enable\n");
        }
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_SCREEN_ON):
        break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK):
        break;
    default:
        break;
    }
    return NOTIFY_DONE;
}

static struct notifier_block lidbg_factory_notifier =
{
    .notifier_call = lidbg_factory_event,
};

void parse_factory_patch_cmd(char *pt)
{
    int argc = 0;
    int i = 0;

    char *argv[32] = {NULL};
    argc = lidbg_token_string(pt, " ", argv);

    lidbg(TAG"cmd:");
    while(i < argc)
    {
        printk(KERN_CRIT"[%d/%d=%s]", i, argc, argv[i]);
        i++;
    }
    lidbg("\n");

    //start parse cmd
    if (!strcmp(argv[0], "getprop"))
    {
        int para_count = argc - 1;
        if(para_count < 1 )
        {
            lidbg(TAG"getprop:err parameta\n");
            return;
        }
        if (!strcmp(argv[1], "wlan.driver.status") )
        {
            if (para_count >= 2 && !strcmp(argv[2], "ok" ))
                is_wlan_open = 1;
            else
                is_wlan_open = 0;
            lidbg(TAG"getprop:[%s=%d]\n", argv[1], is_wlan_open);
        }
    }
    else if  (!strcmp(argv[0], "no_wlan_ctrl"))
    {
	lidbg("no_wlan_ctrl\n");
	wlan_ctrl_en = 0;
    }

}

int factory_patch_open (struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t factory_patch_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg(TAG"copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    parse_factory_patch_cmd(cmd_buf);
    return size;
}

static  struct file_operations factory_patch_nod_fops =
{
    .owner = THIS_MODULE,
    .write = factory_patch_write,
    .open = factory_patch_open,
};
int thread_factory_patch_init(void *data)
{
    register_lidbg_notifier(&lidbg_factory_notifier);
    lidbg_new_cdev(&factory_patch_nod_fops, "lidbg_factory_patch0");
    return 0;
}

int lidbg_factory_patch_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;

    CREATE_KTHREAD(thread_factory_patch_init, NULL);

    return 0;

}
void lidbg_factory_patch_deinit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");

module_init(lidbg_factory_patch_init);
module_exit(lidbg_factory_patch_deinit);


