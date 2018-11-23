#include "lidbg.h"

LIDBG_DEFINE;

#define TAG	 "xxxx:"
#define debug(fmt, args...)  do { printk( KERN_CRIT "[debug]   [%s] " fmt,get_current_time(),##args);}while(0)
#define MSG_LEN 100

static spinlock_t spinlock;


int xxxx_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t  xxxx_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	char local_buf[MSG_LEN];

	if (copy_to_user(buffer, local_buf,  MSG_LEN))
	{
		debug(TAG"copy_to_user ERR\n");
		return -EFAULT;
	}
	
	return MSG_LEN;
}

static int test(void *data)
{
	int ret = -1;
	int bus_id = 3;
	int bits_per_word = 8;
	int max_speed_hz = 1*1024*1024;
	int num = 10;
	int i;

	char psend_data[100] = "1234567890";
	char precv_data[100] = {0};

	while(1)
	{
		ret = spi_api_do_set( bus_id, mode, bits_per_word, max_speed_hz);
		printk(KERN_CRIT"spi_api_do_set:ret=%d\n");

		spi_api_do_write_then_read(bus_id, &psend_data[0], num, &precv_data[0], num);

		printk(KERN_CRIT"spi_api_do_write_then_read:ret=%d,send:");
		for(i=0;i<num;i++)
			printk(KERN_CRIT"0x%x ",psend_data[i]);
		printk(KERN_CRIT",read:");
		for(i=0;i<num;i++)
			printk(KERN_CRIT"0x%x ",precv_data[i]);
		printk(KERN_CRIT"\n\n\n\n\n");

		msleep(1000);
	}
}

ssize_t xxxx_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	char local_buf[MSG_LEN];
	int ret;

	char *cmd[32] = {NULL};
	int cmd_num  = 0;
	char cmd_buf[512];
	memset(cmd_buf, '\0', 512);

	if(copy_from_user(local_buf, buf, size))
	{
		debug("copy_from_user ERR\n");
		return -EFAULT;
	}

	if(cmd_buf[size - 1] == '\n')
		cmd_buf[size - 1] = '\0';

	cmd_num = lidbg_token_string(local_buf, " ", cmd) ;

	if(!strcmp("write",cmd[0]))
	{
		CREATE_KTHREAD(test, NULL);	
	};	

	return size;
}



static  struct file_operations xxxx_fops =
{
	.owner = THIS_MODULE,
	.open = xxxx_open,
	.read = xxxx_read,
	.write = xxxx_write,
};


static int lidbg_xxxx_probe(struct platform_device *pdev)
{
	spin_lock_init(&spinlock);
	lidbg_new_cdev(&xxxx_fops, "xxxx_test");
	lidbg_shell_cmd("chmod 777 /dev/xxxx_test");
	CREATE_KTHREAD(test, NULL);
	return 0;
}

#ifdef MY_PM
static int detect_carback_pm_suspend(struct device *dev)
{
	return 0;
}

static int detect_carback_pm_resume(struct device *dev)
{
	return 0;
}

static struct dev_pm_ops lidbg_xxxx_ops =
{
	.suspend	= xxxx_pm_suspend,
	.resume	= xxxx_pm_resume,
};
#endif

static struct platform_device lidbg_xxxx_device =
{
	.name               = "lidbg_xxxx",
	.id                 = -1,
};

static struct platform_driver lidbg_xxxx_driver =
{
	.probe		= lidbg_xxxx_probe,
	.driver     = {
		.name = "lidbg_xxxx",
		.owner = THIS_MODULE,
#ifdef MY_PM
		.pm = &lidbg_xxxx_ops,
#endif
	},
};

static int __init lidbg_xxxx_init(void)
{
	platform_device_register(&lidbg_xxxx_device);
	platform_driver_register(&lidbg_xxxx_driver);
	return 0;
}

static void __exit lidbg_xxxx_exit(void)
{}



module_init(lidbg_xxxx_init);
module_exit(lidbg_xxxx_exit);


MODULE_DESCRIPTION("lidbg.xxxx");
MODULE_LICENSE("GPL");
