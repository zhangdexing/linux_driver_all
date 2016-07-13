
#include "lidbg.h"

struct input_dev *input = NULL;

//typedef unsigned char  KEYCODE_T;
//key 580   APP_SWITCH
static int lidbg_keycode[] =
{
    KEY_MENU,   KEY_HOME,  KEY_BACK,
    KEY_DOWN,   KEY_UP,  KEY_RIGHT, KEY_LEFT,
    KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_PAUSE, KEY_MUTE,
    KEY_POWER, KEY_SLEEP, KEY_WAKEUP,
    KEY_ENTER, KEY_END, KEY_HOMEPAGE,580
};

void lidbg_key_report(u32 key_value, u32 type)
{
    lidbg("key - key_value:%d\n", key_value);
    /*
    	if(key_value == KEY_HOME)
    	{
    		lidbg_shell_cmd("am start -a android.intent.action.MAIN -c android.intent.category.HOME");
    		return ;
    	}
    */
#ifdef SOC_msm8x25
#else
    if(key_value == KEY_HOME)
        key_value = KEY_HOMEPAGE;
#endif
    if(type == KEY_PRESSED)
        lidbg("key - press\n");
    else if(type == KEY_RELEASED)
        lidbg("key - release\n");
    else
        lidbg("key - press&release\n");

    if((type == KEY_PRESSED) || (type == KEY_RELEASED))
    {
        input_report_key(input, key_value, type);
        input_sync(input);
    }
    else
    {
        input_report_key(input, key_value, KEY_PRESSED);
        //input_sync(input);
        input_report_key(input, key_value, KEY_RELEASED);
        input_sync(input);
    }
}
ssize_t  key_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{

    return size;
}

ssize_t  key_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    u8 key_buf[4] = {0};
    u32 key_val, key_type, val_temp;
    memset(key_buf, '\0', 4);
    if(size > 4)
        size = 4;
    if(copy_from_user(key_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    val_temp = (u32)key_buf[1];
    val_temp = (val_temp << 8);
    key_val = val_temp + (u32)key_buf[0];
    val_temp = (u32)key_buf[3];
    val_temp = (val_temp << 8);
    key_type = val_temp + (u32)key_buf[2];
    lidbg_key_report(key_val, key_type);
    return size;
}

int key_open (struct inode *inode, struct file *filp)
{
    return 0;
}


static  struct file_operations key_nod_fops =
{
    .owner = THIS_MODULE,
    .write = key_write,
    .read = key_read,
    .open =  key_open,

};

static int key_ops_suspend(struct device *dev)
{
    lidbg("-----------key_suspend------------\n");
    DUMP_FUN;

    return 0;
}

static int key_ops_resume(struct device *dev)
{

    lidbg("-----------key_resume------------\n");
    DUMP_FUN;

    return 0;
}
static struct dev_pm_ops key_ops =
{
    .suspend	= key_ops_suspend,
    .resume	= key_ops_resume,
};
static int key_probe(struct platform_device *pdev)
{
    lidbg("-----------key_probe------------\n");
    lidbg_new_cdev(&key_nod_fops, "lidbg_key");

    return 0;
}
static int key_remove(struct platform_device *pdev)
{
    return 0;
}
static struct platform_device key_devices =
{
    .name			= "lidbg_key",
    .id 			= 0,
};

static struct platform_driver key_driver =
{
    .probe = key_probe,
    .remove = key_remove,
    .driver = 	{
        .name = "lidbg_key",
        .owner = THIS_MODULE,
        .pm = &key_ops,
    },
};

int lidbg_key_init(void)
{
    int error;
    int i;

    input = input_allocate_device();
    if (!input)
    {
        lidbg("input_allocate_device err!\n");
        goto fail;
    }
    input->name = "lidbg_key";
    //input->phys = "lidbg_key";
    //input->dev.parent = &pdev->dev;

    input->id.bustype = BUS_HOST;
    //input->id.vendor = 0x0001;
    //input->id.product = 0x0001;
    //input->id.version = 0x0100;


    __set_bit(EV_KEY, input->evbit);
    //??????.SKUD.futengfei
#if 0//(defined(BOARD_V1) || defined(BOARD_V2))
    for (i = 1; i < KEY_MAX; i++)
    {
        input_set_capability(input, EV_KEY, i);
    }
#else
    for (i = 0; i < ARRAY_SIZE(lidbg_keycode); i++)
    {
        input_set_capability(input, EV_KEY, lidbg_keycode[i] & KEY_MAX);
    }
#endif

    //clear_bit(KEY_RESERVED, zlgkpd->input->keybit);

    error = input_register_device(input);
    if (error)
    {
        lidbg("input_register_device err!\n");

        goto fail;
    }
    platform_device_register(&key_devices);
    platform_driver_register(&key_driver);

    LIDBG_MODULE_LOG;


    return 0;
fail:
    input_free_device(input);
    return 0;

}


void lidbg_key_deinit(void)
{
    input_unregister_device(input);
}


void lidbg_key_main(int argc, char **argv)
{
    u32 key_value = 0;
    u32 type;

    if(argc < 2)
    {
        lidbg("Usage:\n");
        lidbg("key type\n");
        return;

    }
    type = simple_strtoul(argv[1], 0, 0);

    if(!strcmp(argv[0], "home"))
    {
        key_value = KEY_HOME ;
    }
    else if(!strcmp(argv[0], "back"))
    {
        key_value = KEY_BACK ;
    }

    else if(!strcmp(argv[0], "up"))
    {
        key_value = KEY_UP ;

    }
    else if(!strcmp(argv[0], "down"))
    {
        key_value = KEY_DOWN ;

    }
    else if(!strcmp(argv[0], "menu"))
    {
        key_value = KEY_MENU ;

    }
    else if(!strcmp(argv[0], "right"))
    {
        key_value = KEY_RIGHT ;

    }
    else if(!strcmp(argv[0], "left"))
    {
        key_value = KEY_LEFT ;

    }
    else if(!strcmp(argv[0], "voldown"))
    {
        key_value = KEY_VOLUMEDOWN ;

    }
    else if(!strcmp(argv[0], "volup"))
    {
        key_value = KEY_VOLUMEUP ;

    }

    else if(!strcmp(argv[0], "pause"))
    {
        key_value = KEY_PAUSE ;

    }
    else if(!strcmp(argv[0], "mute"))
    {
        key_value = KEY_MUTE ;

    }

    else if(!strcmp(argv[0], "sleep"))
    {
        key_value = KEY_SLEEP ;

    }

    else if(!strcmp(argv[0], "power"))
    {
        key_value = KEY_POWER ;

    }
    else if(!strcmp(argv[0], "enter"))
    {
        key_value = KEY_ENTER;

    }
    else if(!strcmp(argv[0], "end"))
    {
        key_value = KEY_END;

    }
    else
    {
        key_value = simple_strtoul(argv[0], 0, 0);
    }

    lidbg_key_report(key_value, type);

}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");

EXPORT_SYMBOL(lidbg_key_main);
EXPORT_SYMBOL(lidbg_key_report);
module_init(lidbg_key_init);
module_exit(lidbg_key_deinit);
