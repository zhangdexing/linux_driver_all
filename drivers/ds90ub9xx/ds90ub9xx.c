#include "ds90ub9xx.h"
#define TAG "FLY: ds90ub9xx "
#define hal_comm_debug(tag, format, args...) printk(KERN_DEBUG"%s: "format, tag, ##args)
#define hal_comm_debug_err(tag, format, args...) printk(KERN_CRIT"%s: "format, tag, ##args)

static unsigned UB928_STATE = 0;
typedef struct {
	unsigned char reg;
	unsigned char data;
} ds90ub9xx_setting;

static ds90ub9xx_setting ub927_init_config[] = {
	{0x03, 0xda},
	{0x04, 0x90},
	{0x06, 0x58},
	{0x07, 0x28},
	{0x08, 0x28},
};
static ds90ub9xx_setting ub928_init_config[] = {
	{0x20, 0x90},
};

static int ub927_i2c_write(unsigned char *buf, unsigned len)
{
	int ret = -1, retries = 0;
	while(retries < 5){
		ret = SOC_I2C_Send(UB927_I2C_BUS, UB927_I2C_ADDR, buf, len);
		if(ret == 1)
			break;
		retries++;
		msleep(300);
	}
	if(retries >= 5){
		hal_comm_debug_err(TAG, "UB927 I2C communication timeout, bus:%d addr:0x%x reg:0x%x data:0x%x\n", UB927_I2C_BUS, UB927_I2C_ADDR, buf[0], buf[1]);
		return -1;
	}
	return 0;
}

static int ub928_i2c_write(unsigned char *buf, unsigned len)
{
	int ret = -1, retries = 0;
	while(retries < 5){
		ret = SOC_I2C_Send(UB928_I2C_BUS, UB928_I2C_ADDR, buf, len);
		if(ret == 1)
			break;
		retries++;
		msleep(300);
	}
	if(retries >= 5){
		hal_comm_debug_err(TAG, "UB928 I2C communication timeout, bus:%d addr:0x%x reg:0x%x data:0x%x\n", UB928_I2C_BUS, UB928_I2C_ADDR, buf[0], buf[1]);
		return -1;
	}
	return 0;
}

static int gt9xx_i2c_write(unsigned char *buf, unsigned len)
{
	int ret = -1, retries = 0;
	while(retries < 5){
		ret = SOC_I2C_Send(TS_I2C_BUS, GT9XX_I2C_ADDR, buf, len);
		if(ret == 1)
			break;
		retries++;
		msleep(300);
	}
	if(retries >= 5){
		hal_comm_debug_err(TAG, "GT9XX I2C communication timeout, bus:%d addr:0x%x reg:0x%x%x data:0x%x\n", TS_I2C_BUS, GT9XX_I2C_ADDR, buf[0], buf[1], buf[2]);
		return -1;
	}
	return 0;
}

static int gt9xx_reset_guitar(void)
{
	int ret;
	unsigned char buf[2] = {0x20, 0x10};
	u8 opr_buffer[4] = {0x80, 0x40, 0xAA, 0xAA};
	/* This reset sequence will selcet I2C slave address */
	ret = ub928_i2c_write(buf, 2);
	if(ret < 0)
		return ret;
	msleep(20);
	SOC_IO_Output(0, GTP_INT_PORT, 1);
	udelay(GT9XX_RESET_DELAY_T3_US);
	buf[1] = 0x90;
	ret = ub928_i2c_write(buf, 2);
	if(ret < 0)
		return ret;
	msleep(GT9XX_RESET_DELAY_T4);

	/* ========= gt9xx int sync ========== */
	SOC_IO_Output(0, GTP_INT_PORT, 0);
	msleep(50);
	SOC_IO_Input(0, GTP_INT_PORT, 0);
	/* =================================== */

	/*in case of recursively reset by calling gt9xx_i2c_write*/
	hal_comm_debug_err(TAG, "Init external watchdog...\n");
	ret = gt9xx_i2c_write(opr_buffer, 4);
	if(ret < 0){
		hal_comm_debug_err(TAG, "fail Init external watchdog ! \n");
		return ret;
	}

	return 0;
}

static int push_ub927_config(ds90ub9xx_setting *config, unsigned count)
{
	int ret;
	unsigned i;
	unsigned char buf[2] = {0};
	for(i = 0; i < count; i++){
		buf[0] = config[i].reg;
		buf[1] = config[i].data;
		ret = ub927_i2c_write(buf, 2);
		if(ret < 0)
			return ret;
	}
	return ret;
}

static int push_ub928_config(ds90ub9xx_setting *config, unsigned count)
{
	int ret;
	unsigned i;
	unsigned char buf[2] = {0};
	for(i = 0; i < count; i++){
		buf[0] = config[i].reg;
		buf[1] = config[i].data;
		ret = ub928_i2c_write(buf, 2);
		if(ret < 0)
			return ret;
	}
	return ret;
}

static int init_ub927(void)
{
	return push_ub927_config(ub927_init_config, sizeof(ub927_init_config) / sizeof(ds90ub9xx_setting));
}

static int init_ub928(void)
{
	return push_ub928_config(ub928_init_config, sizeof(ub928_init_config) / sizeof(ds90ub9xx_setting));
}

static void loading_gt9xx_driver(void)
{
	UB928_STATE = 1;
	return ;
}

int ds90ub9xx_open(struct inode *inode, struct file *filp)
{
	hal_comm_debug_err(TAG, "ds90ub9xx_open\n");
	return 0;
}

int ds90ub9xx_close(struct inode *inode, struct file *filp)
{
	hal_comm_debug_err(TAG, "ds90ub9xx_close \n");
	return 0;
}

static ssize_t ds90ub9xx_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	int argc = 0, i = 0, ret = -1;
	char *argv[32] = {0};
	char str[1024];
	if (copy_from_user(str, (char *)buf, size) > 0)
	{
		hal_comm_debug_err(TAG, "ds90ub9xx_write copy_from_user ERR !\n");
		return 0;
	}
	argc = lidbg_token_string(str, " ", argv);
	if(argc < 4)
	{
	    hal_comm_debug_err(TAG, "input error please input: w ub927/ub928 reg(0x20) data(0x90)\n");
	    return size;
	}
	if(!strcmp(argv[0], "w")){
		char psend_data[2];
		psend_data[0] = simple_strtoul(argv[2], 0, 0);
		psend_data[1] = simple_strtoul(argv[3], 0, 0);
		hal_comm_debug_err(TAG, "direction:%s device:%s reg:0x%x data:0x%x\n", argv[0], argv[1], psend_data[0], psend_data[1]);
		if(!strcmp(argv[1], "ub928")){
			if(UB928_STATE){
				ret = ub928_i2c_write(psend_data, 2);
				if(ret < 0)
					return ret;
				else
					hal_comm_debug_err(TAG, "ub928_i2c_write succese \n");
			}else{
				hal_comm_debug_err(TAG, "no ub928 device ! \n");
			}
		}else if(!strcmp(argv[1], "ub927")){
			ret = ub927_i2c_write(psend_data, 2);
			if(ret < 0)
				return ret;
			else
				hal_comm_debug_err(TAG, "ub927_i2c_write succese \n");
		}else
			hal_comm_debug_err(TAG, "input error please input: w ub927/ub928 reg(0x20) data(0x90)\n");
	}else if(!strcmp(argv[0], "r")){
		hal_comm_debug_err(TAG, "no read func only write func please input: w ub927/ub928 reg(0x20) data(0x90)\n");
	}else
		hal_comm_debug_err(TAG, "no actualize func only write func please input: w ub927/ub928 reg(0x20) data(0x90)\n");

	return size;
}

static ssize_t ds90ub9xx_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	hal_comm_debug_err(TAG, "ds90ub9xx_read \n");
	return size;
}

static long ds90ub9xx_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	hal_comm_debug_err(TAG, "ds90ub9xx_ioctl\n");
	return 0;
}


static struct file_operations ds90ub9xx_dev_fops =
{
	.owner = THIS_MODULE,
	.open = ds90ub9xx_open,
	.write = ds90ub9xx_write,
	.read = ds90ub9xx_read,
	.unlocked_ioctl = ds90ub9xx_ioctl,
	.release = ds90ub9xx_close,
};

static int  ds90ub9xx_probe(struct platform_device *pdev)
{
	int ret;
	hal_comm_debug_err(TAG, "FUNC : %s LINE : %d UB928_STATE:%d\n",__func__,__LINE__,UB928_STATE);
	ret = init_ub927();
	if(ret < 0){
		hal_comm_debug_err(TAG, "init ub927 fail !\n");
		return ret;
	}
	ret = init_ub928();
	if(ret < 0){
		hal_comm_debug_err(TAG, "init ub928 fail !\n");
	}else{
		ret = gt9xx_reset_guitar();
		if(ret < 0){
			hal_comm_debug_err(TAG, "gt9xx_reset_guitar fail !\n");
		}else{
			loading_gt9xx_driver();
		}
	}
	ret = lidbg_new_cdev(&ds90ub9xx_dev_fops, "ds90ub9xx");
	if(ret < 0)
	{
		hal_comm_debug_err(TAG, "Fail to creat ds90ub9xx cdev \n");
		return -1;
	}
	hal_comm_debug_err(TAG, "ds90ub9xx_probe ok UB928_STATE:%d\n", UB928_STATE);
	return 0;
}


static int  ds90ub9xx_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_device ds90ub9xx_device =
{
	.name           = "ds90ub9xx",
	.id                 = -1,
};

static struct platform_driver ds90ub9xx_driver =
{
	.probe		= ds90ub9xx_probe,
	.remove     = ds90ub9xx_remove,
	.driver         = {
	    .name = "ds90ub9xx",
	    .owner = THIS_MODULE,
	},
};



static int  ds90ub9xx_init(void)
{
	hal_comm_debug_err(TAG, "FUNC : %s LINE : %d \n",__func__,__LINE__);
	platform_device_register(&ds90ub9xx_device);
	platform_driver_register(&ds90ub9xx_driver);
	return 0;
}

static void  ds90ub9xx_exit(void)
{
	platform_driver_unregister(&ds90ub9xx_driver);
}


module_init(ds90ub9xx_init);
module_exit(ds90ub9xx_exit);


MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("ds90ub9xx driver");


