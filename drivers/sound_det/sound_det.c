
#include "lidbg.h"

LIDBG_DEFINE;
#define TAG "sound:"
struct misc_device
{
    char *name;
    unsigned int counter;
    wait_queue_head_t queue;
    struct semaphore sem;

};
struct misc_device *dev;
static int sound_detect_dbg = 0;
int snd_status = -1;
int snd_dbg = 0;
volatile int flag = 0;
struct completion snd_status_sem;
#ifdef SOUND_DET_TEST
void SAF7741_Volume(BYTE Volume);
#endif

int sound_detect_event(int state)
{
    snd_status = state;
    if(state == SND_START || state == SND_NAVI_START)
        lidbg(TAG"music_start,%d\n", state);
    else
        lidbg(TAG"music_stop,%d\n", state);
#ifdef SOUND_DET_TEST
    SAF7741_Volume((state == SND_START || state == SND_NAVI_START) ? 0 : 20);
#endif
    complete(&snd_status_sem);
    return 1;
}

void cb_sound_detect(char *key, char *value )
{
    lidbg_setprop("persist.lidbg.sound.dbg", value);
    lidbg(TAG"persist.lidbg.sound.dbg=%s\n", value);
}
int sound_detect_init(void)
{
    init_completion(&snd_status_sem);
    FS_REGISTER_INT(sound_detect_dbg, "sound_detect_dbg", 0, cb_sound_detect);
    lidbg(TAG"sound_detect_init\n");
    return 1;
}

int  iGPS_sound_status(void)
{
    wait_for_completion(&snd_status_sem);
    if(snd_dbg)
        lidbg(TAG"hal get snd_status:%d\n", snd_status);
    return snd_status;
}


static void parse_cmd(char *pt)
{
    int argc = 0;
    char *argv[32] = {NULL};

    argc = lidbg_token_string(pt, " ", argv);

    if (!strcmp(argv[0], "sound"))
    {
        int value;
        value = simple_strtoul(argv[1], 0, 0);
        sound_detect_event(value);
    }
    else if (!strcmp(argv[0], "phoneCallState"))
    {
        int value;
        value = simple_strtoul(argv[1], 0, 0);
        if(value >= 1 && value <= 3)
            g_var.is_phone_in_call_state = 1;
        else
            g_var.is_phone_in_call_state = 0;
        lidbg("[is_phone_in_call_state:%d,value:%d]\n", g_var.is_phone_in_call_state, value);
    }
    else if (!strcmp(argv[0], "dbg"))
    {
        snd_dbg = !snd_dbg;
        lidbg("[snd_dbg:%d]\n", snd_dbg);
    }
    else
        lidbg("error:%s\n", argv[0]);

}

int dev_open(struct inode *inode, struct file *filp)
{
    filp->private_data = dev;
    return 0;
}
int dev_close(struct inode *inode, struct file *filp)
{
    return 0;
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
    flag = 1;
    wake_up_interruptible(&dev->queue);
    return size;//warn:don't forget it;
}
ssize_t  dev_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{

    flag = 0;
    if(copy_to_user(buffer, &snd_status, 4))
    {
        lidbg("copy_from_user ERR\n");
    }
    return size;
}

static unsigned int dev_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct misc_device *dev = filp->private_data;
    poll_wait(filp, &dev->queue, wait);
    if(flag)
    {
        mask |= POLLIN | POLLRDNORM;
        pr_debug("plc poll have data!!!\n");
    }
    return mask;

}
static struct file_operations dev_fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .write = dev_write,
    .read = dev_read,
    .poll = dev_poll,
    .release = dev_close,
};

static int soc_dev_probe(struct platform_device *pdev)
{

    sound_detect_init();
    dev = (struct misc_device *)kmalloc( sizeof(struct misc_device), GFP_KERNEL );
    init_waitqueue_head(&dev->queue);
    lidbg_new_cdev(&dev_fops, "fly_sound");

    return 0;

}
static int  soc_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
    lidbg("soc_dev_suspend\n");
    if(!g_var.is_fly) {}

    return 0;

}
static int soc_dev_remove(struct platform_device *pdev)
{
    lidbg("soc_dev_remove\n");
    if(!g_var.is_fly) {}

    return 0;

}
static int soc_dev_resume(struct platform_device *pdev)
{
    lidbg("soc_dev_resume\n");
    if(!g_var.is_fly) {}

    return 0;
}

struct platform_device soc_devices =
{
    .name			= "sound_det",
    .id 			= 0,
};
static struct platform_driver soc_devices_driver =
{
    .probe = soc_dev_probe,
    .remove = soc_dev_remove,
    .suspend = soc_dev_suspend,
    .resume = soc_dev_resume,
    .driver = {
        .name = "sound_det",
        .owner = THIS_MODULE,
    },
};
static void set_func_tbl(void)
{
    plidbg_dev->soc_func_tbl.pfnGPS_sound_status = iGPS_sound_status;
}



int dev_init(void)
{
    lidbg("=======sound_det_init123========\n");
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

MODULE_AUTHOR("fly, <fly@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");

module_init(dev_init);
module_exit(dev_exit);

