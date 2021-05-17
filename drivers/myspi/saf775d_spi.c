#include "../../core/inc/lidbg.h"

#define TAG	 "saf775d:"
#define MSG_LEN 100

#define DEVICE_NAME "SpiModule"
#define vsnprintfBuffLen	(1024*8)
#define DEVICE_COUNT 1

#define Boot_Phase _IO('C',0)
#define Application_Phase _IO('C',1)
#define write_then_read _IO('C',2)
#define only_write _IO('C',3)
#define only_read _IO('C',4)

char *cmd;
char vsnprintfBuff[vsnprintfBuffLen];
char *pVsnprintfBuff = vsnprintfBuff;

//1.定义一个字符设备结构体
struct chr_dev
{
 struct cdev cdev;//字符设备
 dev_t devno;
 struct class *test_class;
 struct device *test_device;
};
struct chr_dev  *chr_devp;


void debugTagSet(const char *tagName)
{
	if (NULL != tagName)
	{
        snprintf(vsnprintfBuff, vsnprintfBuffLen, "[%s] ", tagName);
        pVsnprintfBuff = &vsnprintfBuff[strlen(vsnprintfBuff)];
	}
}


void debugPrintf(char *fmt,...)
{
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(pVsnprintfBuff, vsnprintfBuffLen, fmt, ap);
	va_end(ap);
	pVsnprintfBuff[len] = 0;
 	printk("%s",vsnprintfBuff);     
}


static int my_open (struct inode *inode, struct file *filefp)
{
	debugPrintf("open the char device\n");
	return 0;//success
}
static int my_release(struct inode *inode, struct file *filefp)
{	
	debugPrintf("close the char device\n");
	return 0;
}
static ssize_t my_read (struct file *filefp, char __user *buf, size_t count, loff_t *off)
{
	int ret,i;
	unsigned char local_buf[MSG_LEN];
	local_buf[0]=count;
	ret = spi_api_do_read(2,local_buf,count);
		debugPrintf("enter read.spi_api_do_read() ret=%d\n",ret);
	
	//for(i=0;i<count;i++)
	//	debugPrintf("local_buf[%d]=%d\n",i,local_buf[i]);
		
	ret = copy_to_user(buf, local_buf,  count);
	if(ret < 0)
	{
		debugPrintf("copy_to_user ERR\n");
		return -EFAULT;
	}
		debugPrintf("enter read.ret=%d\n",ret);
	
	return 0;
}

//******************************************************************
static ssize_t my_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	
	int ret = 0;
	char local_buf[MSG_LEN];

	if(copy_from_user(local_buf, buf, size))
	{
		debugPrintf("copy_from_user ERR\n");
		return -EFAULT;
	}
	if(!strcmp("write",cmd))
	{
		int i;
		char txbuf[50];
		for(i=0;i<size;i++)
		{
			txbuf[i]=local_buf[i];
		}
		ret = spi_api_do_write(2,txbuf,txbuf[0]);	
		if(ret < 0)
			debugPrintf("spi_api_do_write() failed.\n");
			
	}
	else if(!strcmp("write_read",cmd))
	{
		int i;
		char txbuf[50];
		char rxbuf[50];
		rxbuf[0]=32;
		for(i=0;i<size&&i<50;i++)
		{
			txbuf[i]=local_buf[i];
			debugPrintf("txbuf[%d]=%02x\n",i,txbuf[i]);
		}
		spi_api_do_write_then_read(2,txbuf, size, rxbuf,32);
		
		for(i=0;i<32;i++)
		{
			debugPrintf("rxbuf[%d]=%02x\n",i,rxbuf[i]);
		}
	}
	else if(!strcmp("read",cmd))
	{	
		int i;
		char rxbuf[100];
		rxbuf[0]=32;
		ret = spi_api_do_read(2,rxbuf,32);
		for(i=0;i<32;i++)
		{
			debugPrintf("rxbuf[%d]=%02x\n",i,rxbuf[i]);
		}
	}
	/*
	else if(!strcmp("set",cmd))
	{
		
		//spidev->spi = spi;
		//spi->max_speed_hz = 1000000;
		//spi->bits_per_word = 8;
		//spi->mode = 0;
		
		//spi->mode &= ~SPI_CPHA;
		//spi->mode &= ~SPI_CPOL;
		//spi->mode &= ~SPI_CS_HIGH;
		
		
		u8 mode = 0;
		mode &= ~SPI_CPHA;
		mode &= ~SPI_CPOL;
		mode &= ~SPI_CS_HIGH;
		
		ret = spi_api_do_set(2,mode,8,1000000);

	}
	*/
	return size;
}


static long Spi_ioctl(struct file*filefp,unsigned int command,unsigned long arg)
{
	if(command == write_then_read)
	{
		debugPrintf("mode:write_then_read.\n");
		cmd="write_read";
		
	}
	else if(command == only_write)
	{
		debugPrintf("mode:only_write.\n");
		cmd="write";
		
	}
	else if(command == only_read)
	{
		debugPrintf("mode:only_read.\n");
		cmd="read";
	}
	return 0;
}

static struct file_operations spi_flops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.read = my_read,
	.write = my_write,
	.unlocked_ioctl = Spi_ioctl,
};
//1 定义cdev结构体变量和dev_nr主从设备号变量
static struct cdev spi_cdev;
static dev_t dev_nr;
static struct class *spi_class;
struct device *spi_device;

static int __init spi_init(void)
{
	int res;
   	 debugTagSet(TAG);
	spi_api_do_set(2,1,8,2000000);//
	//1 动态申请主从设备号
	res = alloc_chrdev_region(&dev_nr, 0, 1, "spi_chrdev");
	if(res){
    	debugPrintf("==>alloc chrdev region failed!\n");
        goto chrdev_err;
	} 
	//3 初始化cdev数据
	cdev_init(&spi_cdev, &spi_flops);
	//4 添加cdev变量到内核，完成驱动注册
	res = cdev_add(&spi_cdev, dev_nr, DEVICE_COUNT);
	if(res){
		debugPrintf("==>cdev add failed!\n");
        goto cdev_err;
	}

	//创建设备类
	spi_class = class_create(THIS_MODULE,"spi_class");
	if(IS_ERR(spi_class)){
	 	 res =  PTR_ERR(spi_class);
    goto class_err;
	}

	//创建设备节点
	spi_device = device_create(spi_class,NULL, dev_nr, NULL,"saf775d_spi");
	if(IS_ERR(spi_device)){
   	   	res = PTR_ERR(spi_device);
       	goto device_err;
    }


    debugPrintf("==>spi begin.1/11 16:05\n");
	debugPrintf(DEVICE_NAME " initialized.\n");
	
	return 0;

device_err:
	device_destroy(spi_class, dev_nr);
	class_destroy(spi_class);

class_err:
	cdev_del(&spi_cdev); 

cdev_err:
	unregister_chrdev_region(dev_nr, DEVICE_COUNT);

chrdev_err:
	//申请主设备号失败

	return res;
    
}

static void __exit spi_exit(void)
{
 	debugPrintf("==>demo_exit\n");
    
	//5 删除添加的cdev结构体，并释放申请的主从设备号
	cdev_del(&spi_cdev);    
	unregister_chrdev_region(dev_nr, DEVICE_COUNT);

	device_destroy(spi_class, dev_nr);
	class_destroy(spi_class);
}

module_init(spi_init);
module_exit(spi_exit);
MODULE_AUTHOR("DONG");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("saf775d spi T/R interface driver for flyaudio");

