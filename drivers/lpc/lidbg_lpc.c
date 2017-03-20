/* Copyright (c) 2012, swlee
 *
 */
#include "lidbg.h"
#define DEVICE_NAME "fly_lpc"
LIDBG_DEFINE;
static bool lpc_work_en = true;
static struct kfifo lpc_data_fifo;
static struct kfifo lpc_ad_fifo;
#define DATA_BUFF_LENGTH_FROM_MCU   (128)
#define FIFO_SIZE (1024*4)
u8 *fifo_buffer;
#define AD_FIFO_SIZE (1024)
u32 *ad_fifo_buff;
#define BYTE u8
#define UINT u32
#define UINT32 u32
#define BOOL bool
#define ULONG u32

#define FALSE 0
#define TRUE 1
#define HAL_BUF_SIZE (1024*4)
u8 *lpc_data_for_hal;
#define LPC_SYSTEM_TYPE 0x00


#define HAL_NOTIFY_FIFO_SIZE (1024)

u8 *notify_fifo_buffer;
static struct kfifo notify_data_fifo;
struct semaphore notify_sem;
wait_queue_head_t wait_queue;

#define LPC_DATA_SIZE_MAX 256

static int lpc_resume(struct device *dev);
static int lpc_suspend(struct device *dev);

static struct wake_lock lpc_wakelock;

struct lpc_device
{
    char *name;
    unsigned int counter;
    wait_queue_head_t queue;
    struct semaphore sem;
    struct cdev cdev;
};
typedef struct _FLY_IIC_INFO
{
    struct work_struct iic_work;
} FLY_IIC_INFO, *P_FLY_IIC_INFO;



struct fly_hardware_info
{

    FLY_IIC_INFO FlyIICInfo;

    BYTE buffFromMCU[DATA_BUFF_LENGTH_FROM_MCU];
    BYTE buffFromMCUProcessorStatus;
    UINT buffFromMCUFrameLength;
    UINT buffFromMCUFrameLengthMax;
    BYTE buffFromMCUCRC;
    BYTE buffFromMCUBak[DATA_BUFF_LENGTH_FROM_MCU];

};
struct lpc_device *dev;
//#define  MCU_ADDR_W  0xA0
//#define  MCU_ADDR_R  0xA1
//7 bit i2c addr
#define  MCU_ADDR ( 0x50)
 
int lpc_ping_test = 0;
int lpc_ctrl_by_app = 0;
#define LPC_DEBUG_LOG

#ifdef CONFIG_HAS_EARLYSUSPEND
static void lpc_early_suspend(struct early_suspend *handler);
static void lpc_late_resume(struct early_suspend *handler);
struct early_suspend early_suspend;
#endif

struct fly_hardware_info GlobalHardwareInfo;
struct fly_hardware_info *pGlobalHardwareInfo;

int thread_lpc(void *data)
{
    LPC_PRINT(true, -1, "thread_lpc start");

    while(1)
    {
        if((lpc_work_en) && (lpc_ctrl_by_app == 0) && (g_var.acc_flag == FLY_ACC_ON))
            LPC_CMD_NO_RESET;
        msleep(1000*10);
    }
    return 0;
}

u32 ping_data = 0;
bool ping_repay = 0;
#define LPC_LOG_PATH LIDBG_LOG_DIR"lpc_log.txt"

int thread_lpc_ping_test(void *data)
{
    u32 ping_wait_cnt = 0;

    u32 err_cnt = 0;
    msleep(1000 * 20);
    while(1)
    {
        LPC_CMD_PING_TEST(ping_data & 0xff);

        while(ping_repay == 0)
        {
            msleep(50);
            ping_wait_cnt ++;
            if(ping_wait_cnt > 20 * 5)
            {
                lidbg_fs_log(LPC_LOG_PATH, "lpc_ping_test err %d\n", err_cnt);
                err_cnt++;
                break;
            }

        }
        ping_data++;
        ping_wait_cnt = 0;
        ping_repay = 0;
        //msleep(1000);

    }
    return 0;
}


int LPCCombinDataStream(BYTE *p, UINT len)
{
    UINT i = 0;
    int ret ;
    BYTE checksum = 0;
    BYTE bufData[16];
    BYTE *buf;
    bool bMalloc = FALSE;

    if((!lpc_work_en) || (g_hw.lpc_disable))
    {
        lidbg("ToMCU.skip:%x %x %x\n", p[0], p[1], p[2]);
        return 1;
    }

    if (3 + len + 1 > 16)
    {
        buf = (BYTE *)kmalloc(sizeof(BYTE) * (4 + len), GFP_KERNEL);
        if (buf == NULL)
        {
            LIDBG_ERR("kmalloc.\n");
        }
        bMalloc = TRUE;
    }
    else
    {
        buf = bufData;
    }

    buf[0] = 0xFF;
    buf[1] = 0x55;
    buf[2] = len + 1;
    checksum = buf[2];
    for (i = 0; i < len; i++)
    {
        buf[3 + i] = p[i];
        checksum += p[i];
    }

    buf[3 + i] = checksum;

#ifdef SEND_DATA_WITH_UART
    ret = SOC_Uart_Send(buf);
#else
    ret = SOC_I2C_Send(LPC_I2_ID, MCU_ADDR, buf, 3 + i + 1);
#endif

#if 1 //def LPC_DEBUG_LOG
    lidbg("ToMCU.%d:%x %x %x\n", ret, p[0], p[1], p[2]);
#endif

    if (bMalloc)
    {
        kfree(buf);
        buf = NULL;
    }
    return ret;
}


static void LPCdealReadFromMCUAll(BYTE *p, UINT length)
{
#if 1
    {
       
        u8 val[4] = {0};
		
#ifdef LPC_DEBUG_LOG
 	u32 i;
        pr_debug("From LPC:");//mode ,command,para
        for(i = 0; i < length; i++)
        {
            pr_debug("%x ", p[i]);

        }
        pr_debug("\n");
#endif

        if(p[0] == 0x05 && p[1] == 0x05)
        {
            val[0] = p[2];
            val[1] = p[3];
            val[2] = p[4];
            val[3] = p[5];
            down(&dev->sem);
            if(kfifo_is_full(&lpc_ad_fifo))
            {
                kfifo_reset(&lpc_ad_fifo);
                lidbgerr("kfifo_reset!!!!!\n");
            }
            kfifo_in(&lpc_ad_fifo, val, 4);
            up(&dev->sem);
        }


    }

    switch (p[0])
    {
    case LPC_SYSTEM_TYPE:
        if((p[1] == 0x44) && (p[2] == (ping_data & 0xff))
                &&	 (p[3] == (ping_data & 0xff))
                &&	(p[4] == (ping_data & 0xff))
                &&	(p[5] == (ping_data & 0xff))
                &&	(p[6] == (ping_data & 0xff))
          )
            ping_repay = 1;
        break;
    case 0x96:
        switch (p[2])
        {
        case 0x7f:
#ifdef LPC_DEBUG_LOG
            pr_debug("LPC ping return!\n");
#endif
            break;
        }
    default:
        break;
    }
#endif
}

static void LPCDataEnqueue(BYTE *buff, UINT length)
{
	lidbg("buffFromMCU OK!%d\n",buff[0]);
#if 0	
	switch (buff[0])//mode
    {
    	case LPCCOMM_SYSTEM_DATA_HEAD:
			down(&system_notify_sem);
			if(kfifo_is_full(&system_notify_data_fifo))
			{
				u8 temp_reset_data;
				int tempbyte;
				//kfifo_reset(&knob_data_fifo);
				tempbyte = kfifo_out(&system_notify_data_fifo, &temp_reset_data, 1);
				lidbg("%s:system  kfifo clear!!!!!\n",__func__);
			}
			kfifo_in(&system_notify_data_fifo, &(length),  1);
			kfifo_in(&system_notify_data_fifo, buff,  length);
			up(&system_notify_sem);

			wake_up_interruptible(&system_wait_queue);
			break;
		case LPCCOMM_AUDIO_DATA_HEAD:
			down(&audio_notify_sem);
			if(kfifo_is_full(&audio_notify_data_fifo))
			{
				u8 temp_reset_data;
				int tempbyte;
				//kfifo_reset(&knob_data_fifo);
				tempbyte = kfifo_out(&audio_notify_data_fifo, &temp_reset_data, 1);
				lidbg("%s:audio kfifo clear!!!!!\n",__func__);
			}
			kfifo_in(&audio_notify_data_fifo, &(length),  1);
			kfifo_in(&audio_notify_data_fifo, buff,  length);
			up(&audio_notify_sem);

			wake_up_interruptible(&audio_wait_queue);
			break;	
		case LPCCOMM_KEY_DATA_HEAD:
			down(&key_notify_sem);
			if(kfifo_is_full(&key_notify_data_fifo))
			{
				u8 temp_reset_data;
				int tempbyte;
				//kfifo_reset(&knob_data_fifo);
				tempbyte = kfifo_out(&key_notify_data_fifo, &temp_reset_data, 1);
				lidbg("%s:key kfifo clear!!!!!\n",__func__);
			}
			kfifo_in(&key_notify_data_fifo, &(length),  1);
			kfifo_in(&key_notify_data_fifo, buff,  length);
			up(&key_notify_sem);

			wake_up_interruptible(&key_wait_queue);
		break;

		case LPCCOMM_RADIO_DATA_HEAD:
			down(&radio_notify_sem);
			if(kfifo_is_full(&radio_notify_data_fifo))
			{
				u8 temp_reset_data;
				int tempbyte;
				//kfifo_reset(&knob_data_fifo);
				tempbyte = kfifo_out(&radio_notify_data_fifo, &temp_reset_data, 1);
				lidbg("%s:radio kfifo clear!!!!!\n",__func__);
			}
			kfifo_in(&radio_notify_data_fifo, &(length),  1);
			kfifo_in(&radio_notify_data_fifo, buff,  length);
			up(&radio_notify_sem);

			wake_up_interruptible(&radio_wait_queue);
		break;
	}
#endif
	down(&notify_sem);
	if(kfifo_is_full(&notify_data_fifo))
	{
		u8 temp_reset_data;
		int tempbyte;
		//kfifo_reset(&knob_data_fifo);
		tempbyte = kfifo_out(&notify_data_fifo, &temp_reset_data, 1);
		lidbg("%s:kfifo clear!!!!!\n",__func__);
	}
	kfifo_in(&notify_data_fifo, &(length),  1);
	kfifo_in(&notify_data_fifo, buff,  length);
	up(&notify_sem);

	wake_up_interruptible(&wait_queue);
	return;
}

static BOOL readFromMCUProcessor(BYTE *p, UINT length)
{
    UINT i;

    for (i = 0; i < length; i++)
    {
        switch (pGlobalHardwareInfo->buffFromMCUProcessorStatus)
        {
        case 0:
            if (0xFF == p[i])
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 1;
            }
            break;
        case 1:
            if (0xFF == p[i])
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 1;
            }
            else if (0x55 == p[i])
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 2;
            }
            else
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
            }
            break;
        case 2:
            pGlobalHardwareInfo->buffFromMCUProcessorStatus = 3;
            pGlobalHardwareInfo->buffFromMCUFrameLength = 0;
            pGlobalHardwareInfo->buffFromMCUFrameLengthMax = p[i];
            pGlobalHardwareInfo->buffFromMCUCRC = p[i];
            break;
        case 3:
            if (pGlobalHardwareInfo->buffFromMCUFrameLength < (pGlobalHardwareInfo->buffFromMCUFrameLengthMax - 1))
            {
                pGlobalHardwareInfo->buffFromMCU[pGlobalHardwareInfo->buffFromMCUFrameLength] = p[i];
                pGlobalHardwareInfo->buffFromMCUCRC += p[i];
                pGlobalHardwareInfo->buffFromMCUFrameLength++;
            }
            else
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
                if (pGlobalHardwareInfo->buffFromMCUCRC == p[i])
                {
                    LPCdealReadFromMCUAll(pGlobalHardwareInfo->buffFromMCU, pGlobalHardwareInfo->buffFromMCUFrameLengthMax - 1);
                    LPCDataEnqueue(pGlobalHardwareInfo->buffFromMCU, pGlobalHardwareInfo->buffFromMCUFrameLength);
                }
                else
                {
                    lidbg("\nRead From MCU CRC Error");
                }
            }
            break;
        default:
            pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
            break;
        }
    }

    if (pGlobalHardwareInfo->buffFromMCUProcessorStatus > 1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL actualReadFromMCU(BYTE *p, UINT length)
{

    if(!lpc_work_en)
        return FALSE;
    if(g_var.recovery_mode)
    {
        BYTE buff[DATA_BUFF_LENGTH_FROM_MCU];
        BYTE iReadLen = DATA_BUFF_LENGTH_FROM_MCU;
        int read_cnt, fifo_len;
        read_cnt = SOC_I2C_Rec_Simple(LPC_I2_ID, MCU_ADDR, buff, iReadLen);
        pr_debug("-------recovery_irq--%d,%d-----\n",read_cnt,(FIFO_SIZE - kfifo_len(&lpc_data_fifo )));
        down(&dev->sem);
        if(kfifo_is_full(&lpc_data_fifo)||(kfifo_len(&lpc_data_fifo) + DATA_BUFF_LENGTH_FROM_MCU > FIFO_SIZE))
        {
            kfifo_reset(&lpc_data_fifo);
            lidbgerr("kfifo_reset!!!!!\n");
        }
	 
        kfifo_in(&lpc_data_fifo, buff, iReadLen);

        fifo_len = kfifo_len(&lpc_data_fifo);
        pr_debug("-------fifo_len=%d-------\n",fifo_len);
        up(&dev->sem);
        if(fifo_len > 0)
        {
            wake_up_interruptible(&dev->queue);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        SOC_I2C_Rec_Simple(LPC_I2_ID, MCU_ADDR , p, length);
        if (readFromMCUProcessor(p, length))
        {

#ifdef LPC_DEBUG_LOG
            pr_debug("LPC Read len=%d\n",length);
#endif
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

}

//MCU��IIC������
irqreturn_t MCUIIC_isr(int irq, void *dev_id)
{
    if(!lpc_work_en)
        return IRQ_HANDLED;
    //SOC_IO_ISR_Disable(MCU_IIC_REQ_GPIO);
    wake_lock(&lpc_wakelock);
    schedule_work(&pGlobalHardwareInfo->FlyIICInfo.iic_work);
    return IRQ_HANDLED;
}

static void workFlyMCUIIC(struct work_struct *work)
{
    BYTE buff[16];
    BYTE iReadLen = 12;

    while ((SOC_IO_Input(MCU_IIC_REQ_GPIO, MCU_IIC_REQ_GPIO, 0) == 0) && (lpc_work_en == 1))
    {
        actualReadFromMCU(buff, iReadLen);
        iReadLen = 16;
    }
    //SOC_IO_ISR_Enable(MCU_IIC_REQ_GPIO);
    wake_unlock(&lpc_wakelock);
}


void mcuFirstInit(void)
{
    pGlobalHardwareInfo = &GlobalHardwareInfo;
    INIT_WORK(&pGlobalHardwareInfo->FlyIICInfo.iic_work, workFlyMCUIIC);

    //let i2c_c high
    while (SOC_IO_Input(0, MCU_IIC_REQ_GPIO, GPIO_CFG_PULL_UP) == 0)
    {
        u8 buff[32];
        static int count = 0;
        count++;
        WHILE_ENTER;
        actualReadFromMCU(buff, 32);
        if(count > 100)
        {
            lidbg("exit mcuFirstInit!\n");
            break;
        }
        msleep(100);
    }
    SOC_IO_ISR_Add(MCU_IIC_REQ_GPIO, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, MCUIIC_isr, pGlobalHardwareInfo);
    if(!g_var.recovery_mode && !g_var.is_fly)
    {
        CREATE_KTHREAD(thread_lpc, NULL);
    }


    FS_REGISTER_INT(lpc_ping_test, "lpc_ping_test", 0, NULL);

    if(lpc_ping_test)
        CREATE_KTHREAD(thread_lpc_ping_test, NULL);

}


void LPCSuspend(void)
{

    //SOC_IO_ISR_Disable(MCU_IIC_REQ_GPIO);
}

void LPCResume(void)
{
    BYTE buff[16];
    BYTE iReadLen = 12;

    //SOC_IO_ISR_Enable(MCU_IIC_REQ_GPIO);
    wake_lock(&lpc_wakelock);
    //clear lpc i2c buffer
    while (SOC_IO_Input(MCU_IIC_REQ_GPIO, MCU_IIC_REQ_GPIO, 0) == 0)
    {
        WHILE_ENTER;
        actualReadFromMCU(buff, iReadLen);
        iReadLen = 16;
    }
     wake_unlock(&lpc_wakelock);

}

void lpc_linux_sync(bool print, int mint, char *extra_info)
{
    static char buff[64] = {0x00, 0xfd};
    int mtime = 0;
    memset(&buff[2], '\0', sizeof(buff) - 2);
    mtime = ktime_to_ms(ktime_get_boottime());
    snprintf(&buff[2], sizeof(buff) - 3, "%s:%d %d.%d", extra_info, mint, mtime / 1000, mtime % 1000);

    SOC_LPC_Send(buff, strlen(buff + 2) + 2);
    if(print)
        lidbg("[%s]\n", buff + 2);
}


int lpc_open(struct inode *inode, struct file *filp)
{
    DUMP_FUN;
    filp->private_data = dev;
    return 0;
}

int lpc_close(struct inode *inode, struct file *filp)
{
    DUMP_FUN;
    return 0;
}

ssize_t  lpc_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    //struct lpc_device *dev = filp->private_data;
    //int bytes;
    //int read_len;//, fifo_len;
	unsigned char length_buf;
	int ret;
	u8 data_buf[LPC_DATA_SIZE_MAX];

	if(kfifo_is_empty(&notify_data_fifo))
    {
        if(wait_event_interruptible(wait_queue, !kfifo_is_empty(&notify_data_fifo)))
            return -ERESTARTSYS;
    }
/*
	fifo_len = kfifo_len(&notify_data_fifo);

	if(size < fifo_len)
	{
		lidbg("%s:buf too small![%d] \n",__func__,size);
		return -1;
	}
    else
		read_len = fifo_len;
*/
	
	down(&notify_sem);
	ret = kfifo_out(&notify_data_fifo, &length_buf, 1);
	ret = kfifo_out(&notify_data_fifo, data_buf, length_buf);
	up(&notify_sem);

	lidbg("%s:lpc_read, length:%d \n",__func__,length_buf);

	if(copy_to_user(buffer, data_buf, length_buf))
     {
         lidbg("%s:copy_to_user ERR\n",__func__);
     }

    return length_buf;
}
static ssize_t lpc_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    char *mem = kzalloc(size, GFP_KERNEL);
    if (!mem)
    {
        LIDBG_ERR("kzalloc \n");
        return false;
    }
		
	if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
	
	LPCCombinDataStream(mem, size);//Pack and send
	pr_debug("write: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",mem[0],mem[1],mem[2],mem[3],mem[4],mem[5]);

    kfree(mem);
    return size;
}
static unsigned int lpc_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    //struct lpc_device *dev = filp->private_data;
    pr_debug("lpc_poll+\n");

    poll_wait(filp, &wait_queue, wait);
    down(&notify_sem);
    if(!kfifo_is_empty(&notify_data_fifo))
    {
        mask |= POLLIN | POLLRDNORM;
        pr_debug("lpc  poll have data!!!\n");
    }
    up(&notify_sem);
    pr_debug("lpc_poll-\n");

    return mask;

}


static struct file_operations lpc_fops =
{
    .owner = THIS_MODULE,
    .open = lpc_open,
    .read = lpc_read,
    .write = lpc_write,
    .poll = lpc_poll,
    .release = lpc_close,
};

#ifdef SUSPEND_ONLINE
static int lidbg_lpc_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
      DUMP_FUN;

	lidbg("lpc event: %ld\n", event);

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
	lidbg("lpc event:resume %ld\n", event);
              lpc_work_en = true;
		break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
	lidbg("lpc event:suspend %ld\n", event);
		//lpc_work_en = false;
		break;
    default:
        break;
    }

    return NOTIFY_DONE;
}

static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_lpc_event,
};
#endif

#define FLY_HAL_FILE "/flysystem/lib/modules/FlyHardware.ko"

static int lpc_ping_en = 0;
static int  lpc_probe(struct platform_device *pdev)
{

    DUMP_FUN;
    FS_REGISTER_INT(lpc_ping_en, "lpc_ping_en", 0, NULL);
	
    wake_lock_init(&lpc_wakelock, WAKE_LOCK_SUSPEND, "lpc_wakelock");

    lpc_data_for_hal = (u8 *)kmalloc(HAL_BUF_SIZE, GFP_KERNEL);
    fifo_buffer = (u8 *)kmalloc(FIFO_SIZE, GFP_KERNEL);
    ad_fifo_buff = (u32 *)kmalloc(AD_FIFO_SIZE, GFP_KERNEL);
    if((lpc_data_for_hal == NULL) || (fifo_buffer == NULL) || (ad_fifo_buff == NULL))
    {
        lidbg("knob_probe kmalloc err\n");
        return 0;
    }

	notify_fifo_buffer = (u8 *)kmalloc(HAL_NOTIFY_FIFO_SIZE , GFP_KERNEL);
	sema_init(&notify_sem, 1);
    kfifo_init(&notify_data_fifo, notify_fifo_buffer, HAL_NOTIFY_FIFO_SIZE);

	init_waitqueue_head(&wait_queue);
#ifdef SOC_mt3360
    return 0;
#endif

#ifdef SUSPEND_ONLINE
	register_lidbg_notifier(&lidbg_notifier);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    early_suspend.suspend = lpc_early_suspend;
    early_suspend.resume = lpc_late_resume;
    register_early_suspend(&early_suspend);
#endif
    dev = (struct lpc_device *)kmalloc( sizeof(struct lpc_device), GFP_KERNEL);
    sema_init(&dev->sem, 1);
    init_waitqueue_head(&dev->queue);
    kfifo_init(&lpc_data_fifo, fifo_buffer, FIFO_SIZE);
    kfifo_init(&lpc_ad_fifo, ad_fifo_buff, AD_FIFO_SIZE);
    lidbg_new_cdev(&lpc_fops, "lidbg_lpc_comm0");

#if 0
    if((!g_var.recovery_mode && g_var.is_fly) || g_hw.lpc_disable) //origin system and fly mode when lpc_ping_en enable,lpc driver will go on;
    {
        lidbg("lpc_init do nothing.disable,[%d,%d,%d,%d]\n", g_var.is_fly, lpc_ping_en, g_var.recovery_mode, fs_is_file_exist(FLY_HAL_FILE));
        return 0;
    }
#endif 

#if defined(PLATFORM_msm8226) || defined(PLATFORM_ID_4) || defined(PLATFORM_ID_7)
	  if(g_var.recovery_mode)
	 		return 0;
#endif
    mcuFirstInit();

    return 0;
}


static int  lpc_remove(struct platform_device *pdev)
{
    return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void lpc_early_suspend(struct early_suspend *handler) {}
static void lpc_late_resume(struct early_suspend *handler) {}
#endif


#ifdef CONFIG_PM
static int lpc_suspend(struct device *dev)
{
    DUMP_FUN;
    //lpc_work_en = false;
    //LPCSuspend();
    return 0;
}

static int lpc_resume(struct device *dev)
{
    DUMP_FUN;
#ifdef SUSPEND_ONLINE
#else
    lpc_work_en = true;
#endif
  if((!g_var.is_fly) && (g_var.recovery_mode == 0) )
     LPCResume();
    return 0;
}

static struct dev_pm_ops lpc_pm_ops =
{
    .suspend	= lpc_suspend,
    .resume		= lpc_resume,
};
#endif


static struct platform_device lidbg_lpc =
{
    .name               = "lidbg_lpc",
    .id                 = -1,
};

static struct platform_driver lpc_driver =
{
    .probe		= lpc_probe,
    .remove     = lpc_remove,
    .driver         = {
        .name = "lidbg_lpc",
        .owner = THIS_MODULE,

#ifdef CONFIG_PM
        .pm = &lpc_pm_ops,
#endif
    },
};

bool iLPC_ADC_Get (u32 channel , u32 *value)
{
    int bytes;
    u32 val_temp = 0;
    u32 ad_val = 0;
    u8 lpc_ad_buff[2] = {0};
    if(kfifo_len(&lpc_ad_fifo))
    {
        down(&dev->sem);
        bytes = kfifo_out(&lpc_ad_fifo, &lpc_ad_buff, 2);
        up(&dev->sem);
        val_temp = (u32)lpc_ad_buff[1];
        val_temp = (val_temp << 8);
        ad_val = val_temp + (u32)lpc_ad_buff[0];
        *value = ad_val;
    }
    else
    {
        return 0;
    }
    return 1;
}
static void set_func_tbl(void)
{
    plidbg_dev->soc_func_tbl.pfnLPC_ADC_Get = iLPC_ADC_Get;
#ifdef SOC_msm8x25
    ((struct lidbg_hal *)plidbg_dev)->soc_func_tbl.pfnSOC_LPC_Send = LPCCombinDataStream;
#else
    ((struct lidbg_interface *)plidbg_dev)->soc_func_tbl.pfnSOC_LPC_Send = LPCCombinDataStream;
#endif

}

static int __init lpc_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;
    set_func_tbl();
    platform_device_register(&lidbg_lpc);
    platform_driver_register(&lpc_driver);
    return 0;
}

static void __exit lpc_exit(void)
{
    platform_driver_unregister(&lpc_driver);
}


module_init(lpc_init);
module_exit(lpc_exit);


MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("lidbg lpc driver");

EXPORT_SYMBOL(lpc_linux_sync);

