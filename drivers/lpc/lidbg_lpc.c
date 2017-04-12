/* Copyright (c) Flyaudio
 *
 */
#include "lidbg.h"
#define DEVICE_NAME "fly_lpc"
LIDBG_DEFINE;
static bool lpc_work_en = true;
#define DATA_BUFF_LENGTH_FROM_MCU   (128)
#define BYTE u8
#define UINT u32
#define BOOL bool

#define FALSE 0
#define TRUE 1
#define LPC_SYSTEM_TYPE 0x00

#define SUSPEND_DATA_WAIT_TIME   (5*HZ)

#define HAL_NOTIFY_FIFO_SIZE (1024*32)

u8 *notify_fifo_buffer;
static struct kfifo notify_data_fifo;
spinlock_t notify_fifo_lock;
wait_queue_head_t wait_queue;
struct wake_lock lpc_wakelock;
bool is_kfifo_empty = 1;

u8 *wbuf_fifo_buffer;
static struct kfifo wbuf_data_fifo;
spinlock_t wbuf_fifo_lock;

struct work_struct wBuf_work;

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

#define  MCU_ADDR ( 0x50)

struct fly_hardware_info GlobalHardwareInfo;
struct fly_hardware_info *pGlobalHardwareInfo;


int LPCCombinDataStream(BYTE *p, UINT len)
{
    UINT i = 0;
    int ret ;
    BYTE checksum = 0;
    BYTE bufData[len + 4];
    BYTE *buf = bufData;

    if((!lpc_work_en) || (g_hw.lpc_disable))
    {
        if(len >= 3 )lidbg("ToMCU.skip:%x %x %x\n", p[0], p[1], p[2]);
        return 1;
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
    return ret;
}


static void LPCWriteDataEnqueue(BYTE *buff, short length)
{
    unsigned long irqflags;
    UINT i = 0;
    BYTE checksum = 0;
    BYTE bufData[length + 4];
    BYTE *buf = bufData;

    if((!lpc_work_en) || (g_hw.lpc_disable))
    {
        if(length >= 3 )pr_debug("ToMCU.skip:%x %x %x\n", buff[0], buff[1], buff[2]);
        return;
    }

    buf[0] = 0xFF;
    buf[1] = 0x55;
    buf[2] = length + 1;
    checksum = buf[2];
    for (i = 0; i < length; i++)
    {
        buf[3 + i] = buff[i];
        checksum += buff[i];
    }

    buf[3 + i] = checksum;

    spin_lock_irqsave(&wbuf_fifo_lock, irqflags);
    if(kfifo_is_full(&wbuf_data_fifo))
    {
        lidbg("%s:kfifo full!!!!!\n", __func__);
        spin_unlock_irqrestore(&wbuf_fifo_lock, irqflags);
        return;
    }
    length += 4;
    kfifo_in(&wbuf_data_fifo, &length ,  2);
    kfifo_in(&wbuf_data_fifo, buf,  length);
    spin_unlock_irqrestore(&wbuf_fifo_lock, irqflags);
    return;
}

static void LPCReadDataEnqueue(BYTE *buff, short length)
{
    unsigned long irqflags;
    spin_lock_irqsave(&notify_fifo_lock, irqflags);
    if(kfifo_is_full(&notify_data_fifo))
    {
        lidbg("%s:kfifo full!!!!!\n", __func__);
        spin_unlock_irqrestore(&notify_fifo_lock, irqflags);
        return;
    }
    kfifo_in(&notify_data_fifo, &length,  2);
    kfifo_in(&notify_data_fifo, buff,  length);
    spin_unlock_irqrestore(&notify_fifo_lock, irqflags);
    is_kfifo_empty = 0;
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
                    LPCReadDataEnqueue(pGlobalHardwareInfo->buffFromMCU, (short)pGlobalHardwareInfo->buffFromMCUFrameLength);
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

    SOC_I2C_Rec_Simple(LPC_I2_ID, MCU_ADDR , p, length);
    if (readFromMCUProcessor(p, length))
    {
        pr_debug("LPC Read len=%d\n", length);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

irqreturn_t MCUIIC_isr(int irq, void *dev_id)
{
    if(!lpc_work_en)
        return IRQ_HANDLED;

    //SOC_IO_ISR_Disable(MCU_IIC_REQ_GPIO);

    wake_lock_timeout(&lpc_wakelock, SUSPEND_DATA_WAIT_TIME);
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
}

irqreturn_t MCUDQBuf_isr(int irq, void *dev_id)
{
    if(!lpc_work_en)
        return IRQ_HANDLED;
    pr_debug("%s: isr\n", __func__);
    if(!work_pending(&wBuf_work))
        schedule_work(&wBuf_work);
    return IRQ_HANDLED;
}

static void workFlyMCUDQBuf(struct work_struct *work)
{
    short length_buf = 0;
    int ret;
    unsigned long irqflags;

    /*Send data when wait_gpio pull up*/
    while ((SOC_IO_Input(MCU_READ_BUSY_GPIO, MCU_READ_BUSY_GPIO, 0) == 1) && (lpc_work_en == 1))
    {
        spin_lock_irqsave(&wbuf_fifo_lock, irqflags);
        if(kfifo_is_empty(&wbuf_data_fifo))//no data, skip
        {
            spin_unlock_irqrestore(&wbuf_fifo_lock, irqflags);
            return;
        }
        ret = kfifo_out(&wbuf_data_fifo, &length_buf, 2);
        if(length_buf > 0)
        {
            u8 data_buf[length_buf];
            ret = kfifo_out(&wbuf_data_fifo, data_buf, length_buf);
            spin_unlock_irqrestore(&wbuf_fifo_lock, irqflags);

            pr_debug("%s: length:%d,0x%x, 0x%x,0x%x,0x%x,0x%x\n", __func__, length_buf, data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4]);

#ifdef SEND_DATA_WITH_UART
            ret = SOC_Uart_Send(data_buf);
#else
            ret = SOC_I2C_Send(LPC_I2_ID, MCU_ADDR, data_buf, length_buf);
#endif
        }
        else
            spin_unlock_irqrestore(&wbuf_fifo_lock, irqflags);
    }

    lidbg("workFlyMCUDQBuf wait for LPC ready!\n");
    return;
}


void mcuFirstInit(void)
{
    DUMP_FUN;
    pGlobalHardwareInfo = &GlobalHardwareInfo;
    INIT_WORK(&pGlobalHardwareInfo->FlyIICInfo.iic_work, workFlyMCUIIC);
    SOC_IO_ISR_Add(MCU_IIC_REQ_GPIO, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, MCUIIC_isr, pGlobalHardwareInfo);
    schedule_work(&pGlobalHardwareInfo->FlyIICInfo.iic_work);

    INIT_WORK(&wBuf_work, workFlyMCUDQBuf);
    SOC_IO_ISR_Add(MCU_READ_BUSY_GPIO,  IRQF_TRIGGER_RISING | IRQF_ONESHOT, MCUDQBuf_isr, NULL);
    //if(!work_pending(&wBuf_work))
    //	schedule_work(&wBuf_work);
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
    return 0;
}

int lpc_close(struct inode *inode, struct file *filp)
{
    DUMP_FUN;
    return 0;
}

ssize_t  lpc_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    short length_buf = 0;
    int ret;
    unsigned long irqflags;

    spin_lock_irqsave(&notify_fifo_lock, irqflags);
    is_kfifo_empty = kfifo_is_empty(&notify_data_fifo);
    spin_unlock_irqrestore(&notify_fifo_lock, irqflags);

    if(is_kfifo_empty)
    {
        if(wait_event_interruptible(wait_queue, !is_kfifo_empty))
            return -ERESTARTSYS;
    }

    spin_lock_irqsave(&notify_fifo_lock, irqflags);
    ret = kfifo_out(&notify_data_fifo, &length_buf, 2);
    if(length_buf > 0)
    {
        u8 data_buf[length_buf];
        ret = kfifo_out(&notify_data_fifo, data_buf, length_buf);
        spin_unlock_irqrestore(&notify_fifo_lock, irqflags);

        pr_debug("%s:lpc_read, length:%d \n", __func__, length_buf);

        if(copy_to_user(buffer, data_buf, length_buf))
        {
            lidbg("%s:copy_to_user ERR\n", __func__);
        }
    }
    else
        spin_unlock_irqrestore(&notify_fifo_lock, irqflags);

    return length_buf;
}
static ssize_t lpc_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    char mem[size];
    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
#if 0
    LPCCombinDataStream(mem, size);//Pack and send
#else
    LPCWriteDataEnqueue(mem, size);//Pack and enqueue
#endif
    // if(size >= 3 ) pr_debug("write: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", mem[0], mem[1], mem[2], mem[3], mem[4], mem[5]);
    pr_debug("size:%d,write: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", size, mem[0], mem[1], mem[2], mem[3], mem[4], mem[5]);
    if(!work_pending(&wBuf_work))
        schedule_work(&wBuf_work);
    return size;
}
static unsigned int lpc_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    unsigned long irqflags;
    pr_debug("lpc_poll+\n");

    poll_wait(filp, &wait_queue, wait);
    spin_lock_irqsave(&notify_fifo_lock, irqflags);
    if(!kfifo_is_empty(&notify_data_fifo))
    {
        mask |= POLLIN | POLLRDNORM;
        pr_debug("lpc  poll have data!!!\n");
    }
    spin_unlock_irqrestore(&notify_fifo_lock, irqflags);
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

static int  lpc_probe(struct platform_device *pdev)
{

    DUMP_FUN;
    if(g_hw.lpc_disable)
    {
        return 0;
    }
    notify_fifo_buffer = (u8 *)kmalloc(HAL_NOTIFY_FIFO_SIZE , GFP_KERNEL);
    kfifo_init(&notify_data_fifo, notify_fifo_buffer, HAL_NOTIFY_FIFO_SIZE);
    spin_lock_init(&notify_fifo_lock);

    wbuf_fifo_buffer = (u8 *)kmalloc(HAL_NOTIFY_FIFO_SIZE , GFP_KERNEL);
    kfifo_init(&wbuf_data_fifo, wbuf_fifo_buffer, HAL_NOTIFY_FIFO_SIZE);
    spin_lock_init(&wbuf_fifo_lock);

    init_waitqueue_head(&wait_queue);
    wake_lock_init(&lpc_wakelock, WAKE_LOCK_SUSPEND, "lpc_wakelock");

    mcuFirstInit();

    lidbg_new_cdev(&lpc_fops, "fly_lpc0");
    return 0;
}


static int  lpc_remove(struct platform_device *pdev)
{
    return 0;
}

#ifdef CONFIG_PM
static int lpc_suspend(struct device *dev)
{
    DUMP_FUN;
    SOC_IO_ISR_Disable(MCU_READ_BUSY_GPIO);
    return 0;
}

static int lpc_resume(struct device *dev)
{
    DUMP_FUN;
    SOC_IO_ISR_Enable(MCU_READ_BUSY_GPIO);

    return 0;
}

static struct dev_pm_ops lpc_pm_ops =
{
    .suspend	= lpc_suspend,
    .resume	= lpc_resume,
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

static void set_func_tbl(void)
{
    ((struct lidbg_interface *)plidbg_dev)->soc_func_tbl.pfnSOC_LPC_Send = LPCCombinDataStream;
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

