
#include "../../core/inc/lidbg.h"
#include <linux/interrupt.h>
#include <linux/workqueue.h>
 #include <stddef.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
//cdev用到的头文件
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>




#define DEVICE_COUNT 1
#define DEVICE_NAME "IpcModule"

#define TAG     "s32k_ipc"
#define s32k_spi_debug(tag, format, args...) printk(KERN_CRIT"%s: "format, tag, ##args)

#define SOC_SRQ_GPIO   7       /*GPIO07 I2C_C2*/
#define MCU_SRQ_GPIO   6      /*GPIO06  I2C_C*/

#define MCU_BUSID     0

#define FIFO_SIZE           (1024*32)

#define FRAMELEN	1024
#define DATALEN 	1018


#define READ_FIFO_LOCK  spin_lock_irqsave(&read_fifo_lock,readFlag)
#define READ_FIFO_UNLOCK  spin_unlock_irqrestore(&read_fifo_lock,readFlag)

#define WRITE_FIFO_LOCK  spin_lock_irqsave(&write_fifo_lock,writeFlag)
#define WRITE_FIFO_UNLOCK  spin_unlock_irqrestore(&write_fifo_lock,writeFlag)


enum IPC_MACHIN_STATE{SEND_STATE, ACK_STATE, NACK_STATE, RESEND_STATE, WAIT};
typedef enum{
    FRAME_DATA=0,
    FRAME_ACK,
    FRAME_NACK,
    FRAME_DUMMY,
    FRAME_CORRUPT,
}FRAME_TYPE_ENUM;

static struct class *ipc_class;
  


wait_queue_head_t wait_queue;
struct work_struct dataTransWork;
struct completion ipcStateMachineCom;

struct completion userDataProcCom;

struct completion userDataFifoProcCom;


static struct kfifo read_data_fifo;
static struct kfifo write_data_fifo;
spinlock_t read_fifo_lock;
spinlock_t write_fifo_lock; 

spinlock_t flaglock;

spinlock_t userDataProcLock;
unsigned long userDataProcFlag;
    
unsigned long readFlag;
unsigned long writeFlag;

char userData[1024];
int userDataLen = 0;

char TxDataFrame[1024];
int TxDataFrameLen = 0;

char LastDataFrame[1024];
int LastDataFrameLen = 0;

char mcuDataFrame[1024];
int mcuDataFrameLen = 0;

int socSRQEnable = 1;

unsigned short SOC_frame_num = 0;
unsigned short MCU_frame_num = 0;
static DEFINE_MUTEX(userDataProcMutex);


#define DEBUG_MESSAGE_MAX_LEN 256
#define vsnprintfBuffLen	(1024*8)

#define ACK_FRAME_LEN 	5
#define NACK_FRAME_LEN 	5
#define DUMMY_FRAME_LEN 5
const unsigned char ack_frame[5] = {0x0, 0x5, 0xA7, 0x3A, 0xF8};
const unsigned char nack_frame[5] = {0x0, 0x5, 0xA9, 0xDB, 0x36};
const unsigned char dummy_frame[5] = {0x0, 0x5, 0xA5, 0x1A, 0xBA};

int StateMachine = SEND_STATE;
unsigned char frame_type;
unsigned char currtent_state;

char bufToStringBuff[DEBUG_MESSAGE_MAX_LEN];
char buffToStringPrint[DEBUG_MESSAGE_MAX_LEN*2];
char vsnprintfBuff[vsnprintfBuffLen];
char *pVsnprintfBuff = vsnprintfBuff;




void debugTagSet(const char *tagName)
{
	if (NULL != tagName)
	{
        snprintf(vsnprintfBuff, vsnprintfBuffLen, "[%s] ", tagName);
        pVsnprintfBuff = &vsnprintfBuff[strlen(vsnprintfBuff)];
	}
}


char *bufToHex(char *buf, int len)
{
	UINT i;
	UINT j;
	char str[4];

	for (i=0,j=0; i<len && j < DEBUG_MESSAGE_MAX_LEN - 4; i++,j=j+3)
	{
		snprintf(str, sizeof(str), " %02X", buf[i]);
		memcpy(&bufToStringBuff[j], str, 3);
	}
	bufToStringBuff[j] = '\n';
	bufToStringBuff[j+1] = '\0';

	return bufToStringBuff;
}

char *bufToDec(int iData)
{
	snprintf(bufToStringBuff, sizeof(bufToStringBuff), " %d", (int)iData);
	return bufToStringBuff;
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

void debugBuf(char *fmt, char *buf, int len)
{
	while (len
		&& (strlen(fmt)+1+len*3 > DEBUG_MESSAGE_MAX_LEN*2))
	{
		len--;
	}

	if (strlen(fmt)+1+len*3 > DEBUG_MESSAGE_MAX_LEN*2)
	{
		debugPrintf("debugBuf OverFlow!\n");
		return;
	}
	strcpy(buffToStringPrint,fmt);
	strcat(buffToStringPrint,bufToHex(buf,len));
	debugPrintf("%s", buffToStringPrint);
}

int init_thread_kfifo(struct kfifo *pkfifo, int size)
{
	int ret;
	ret = kfifo_alloc(pkfifo, size, GFP_KERNEL);
	if (ret < 0)
	{
		debugPrintf( "kfifo_alloc failed !\n");
		return ret;
	}
	return ret;
}

int put_buf_fifo(struct kfifo *pkfifo , const char *buf, int bufLen, spinlock_t *fifo_lock)
{
	int len, ret;
	int data_len;
	if (bufLen+sizeof(bufLen) > FIFO_SIZE)
	{
		return 0;
	}
	len = kfifo_avail(pkfifo);
	while(len < bufLen + sizeof(bufLen))
	{
		debugPrintf( "fifo did't have enough space\n");
		ret = kfifo_out_spinlocked(pkfifo, &data_len, sizeof(data_len), fifo_lock);
		debugPrintf( "kfifo length is : %d \n", data_len);
		if(ret < 0)
		{
			debugPrintf( "fail to output data \n");
		}else
		{
			char out_buf[data_len];
			ret = kfifo_out_spinlocked(pkfifo, out_buf, data_len, fifo_lock);
			if(ret < 0)
			{
				debugPrintf( "fail to output data \n");
			}else
			{
				debugPrintf( "give up one data\n ");
			}
		}
		len = kfifo_avail(pkfifo);
		debugPrintf( "kfifo vaild len : %d \n", len);
	}
	kfifo_in_spinlocked(pkfifo, &bufLen, sizeof(bufLen), fifo_lock);
	kfifo_in_spinlocked(pkfifo, buf, bufLen, fifo_lock);
	return bufLen;
}

int get_buf_fifo(struct kfifo *pkfifo, char *buf, spinlock_t *fifo_lock)
{
	int ret;
	int len;
	if (kfifo_is_empty(pkfifo))
	{
		return 0;
	}
	if(kfifo_len(pkfifo) <= sizeof(len))
	{
		debugPrintf( "get_buf_fifo err : %d\n", kfifo_len(pkfifo));
		return 0;
	}
	ret = kfifo_out_spinlocked(pkfifo, &len, sizeof(len), fifo_lock);
	{
		ret = kfifo_out_spinlocked(pkfifo, buf, len, fifo_lock);
		//lidbg( "read fifo length is %d ret : %d\n", len, ret);
	}
	return len;
}


static void Enable(int *flag)
{
    unsigned long irqflags; 
    spin_lock_irqsave(&flaglock, irqflags);
    *flag = 1;
    spin_unlock_irqrestore(&flaglock, irqflags);
}

static void Disable(int *flag)
{
    unsigned long irqflags; 
    spin_lock_irqsave(&flaglock, irqflags);
    *flag = 0;
    spin_unlock_irqrestore(&flaglock, irqflags);
}


static void socReqTrans(void)
{
    if (1 == socSRQEnable)
    {
        debugPrintf("socReq start\n");
        SOC_IO_Output(SOC_SRQ_GPIO, SOC_SRQ_GPIO, 1);
        mdelay(1);
        SOC_IO_Output(SOC_SRQ_GPIO, SOC_SRQ_GPIO, 0);
        mdelay(1);
        SOC_IO_Output(SOC_SRQ_GPIO, SOC_SRQ_GPIO, 1);
        debugPrintf("socReq end\n");

        wait_for_completion(&ipcStateMachineCom);
    }
}

static int data_transmission(const char *sendData, int sendDataLen, char *recData, int *recDataLen)
{
    socReqTrans();
    debugPrintf("ready to send %d\n", sendDataLen);
    debugBuf(__FUNCTION__, sendData, sendDataLen);
    spi_api_do_write_then_read(MCU_BUSID, sendData, 256, recData, 256);
    *recDataLen = 5;
//    if (*recDataLen > sendDataLen)
//    {
//        spi_api_do_write_then_read(MCU_BUSID, &sendData[sendDataLen], 0, &recData[sendDataLen], *recDataLen-sendDataLen);
//    }

    Enable(&socSRQEnable);
    debugBuf(__FUNCTION__, recData, 10);

    return *recDataLen;
	
}


//static void data_transmission(void)
//{
//    spi_api_do_write_then_read(MCU_BUSID, userData, 2, mcuDataFrame, 2);
//    mcuDataFrameLen = (mcuDataFrame[0]<<8) + mcuDataFrame[1];
//    spi_api_do_write_then_read(MCU_BUSID, &userData[2], userDataLen-2, &mcuDataFrame[2], mcuDataFrameLen);
//
//    debugBuf(__FUNCTION__, mcuDataFrame, mcuDataFrameLen);
//    complete(&ipcStateMachineCom);
//
//}

static void ReadDataEnqueueFromS32K(unsigned char *buff, unsigned short length)
{
	unsigned short msg_len;

    while(length > 0)
    {
        msg_len = (((unsigned short)buff[0])<<8) + ((unsigned short)buff[1]<<0);
      
        put_buf_fifo(&read_data_fifo, &msg_len, 2, &read_fifo_lock);
        put_buf_fifo(&read_data_fifo, buff + 2, msg_len, &read_fifo_lock);

        if( length >= (msg_len + 2) )
        {
            length = length - (msg_len + 2);
            buff = buff + (msg_len + 2);
        }
        else
        {
            break;
        }
    }

    return;
}

const unsigned short crc_ta_8[256]={ /* CRC ×Ö½ÚÓàÊ½±í */
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

static unsigned short IPC_CRC(unsigned char *puchMsg, unsigned int usDataLen)  
{
    unsigned short crc = 0;
    unsigned short high;

    while(usDataLen-- != 0)
    {
        high = (unsigned short)(crc>>8);
        crc <<= 8;
        crc ^= crc_ta_8[high^*puchMsg];
        puchMsg++;
    }

    return crc;
}

static unsigned char GetFrameType(unsigned char *recMcuDataFrame)
{
    unsigned short checksum;
    unsigned short framlen = (((unsigned short)recMcuDataFrame[0])<<8) + (unsigned short)recMcuDataFrame[1];
    //s32k_spi_debug(TAG, "%s!\n", __func__);
    if(framlen < 5 && framlen > 1024)
    {
		s32k_spi_debug(TAG, "FRAME_CORRUPT 0 !\n");
        return FRAME_CORRUPT;
    }
    if(framlen == 5)
    {
        if(recMcuDataFrame[2] == 0xA7 && recMcuDataFrame[3] == 0x3A && recMcuDataFrame[4] == 0xF8)
        {
			//s32k_spi_debug(TAG, "FRAME_ACK!\n");
            return FRAME_ACK;
        }
        else if(recMcuDataFrame[2] == 0xA9 && recMcuDataFrame[3] == 0xDB && recMcuDataFrame[4] == 0x36)
        {
			s32k_spi_debug(TAG, "FRAME_NACK!\n");
            return FRAME_NACK;
        }
        else if(recMcuDataFrame[2] == 0xA5 && recMcuDataFrame[3] == 0x1A && recMcuDataFrame[4] == 0xBA)
        {
			//s32k_spi_debug(TAG, "FRAME_DUMMY!\n");
            return FRAME_DUMMY;
        }
        else 
        {
			s32k_spi_debug(TAG, "FRAME_CORRUPT 1 !\n");
            return FRAME_CORRUPT;
        }
    }
    else // framlen > 5
    {
        checksum = (((unsigned short)recMcuDataFrame[framlen-2])<<8) + (unsigned short)recMcuDataFrame[framlen-1];
      
        if(IPC_CRC(recMcuDataFrame, framlen - 2) != checksum)  
        {
			s32k_spi_debug(TAG, "FRAME_CORRUPT crc errror !\n");
            return FRAME_CORRUPT;
        }
        else   
        {
            		MCU_frame_num = (((unsigned short)recMcuDataFrame[2])<<8) + (unsigned short)recMcuDataFrame[3];
			ReadDataEnqueueFromS32K(recMcuDataFrame+4,framlen-6);
			wake_up_interruptible(&wait_queue); //唤醒等待队列	
			//s32k_spi_debug(TAG, "FRAME_DATA !\n");
            return FRAME_DATA;
        }
    }
}
void mcuDataFrameProc(unsigned char *recMcuDataFrame)
{
    frame_type = GetFrameType(recMcuDataFrame);

    switch(currtent_state)
    {
        
        case SEND_STATE:
            if(frame_type == FRAME_DATA || frame_type == FRAME_DUMMY)
            {
                currtent_state = ACK_STATE;
            }
            else //FRAME_ACK  FRAME_NACK  FRAME_CORRUPT
            {
                currtent_state = NACK_STATE;
            }

            break;
        case ACK_STATE:
            if(frame_type == FRAME_ACK)
            {
                currtent_state = SEND_STATE;
            }
            else //  FRAME_NACK  FRAME_CORRUPT  FRAME_DUMMY  FRAME_DATA
            {
                currtent_state = RESEND_STATE;
            }
            break;
        case NACK_STATE:
            if(frame_type == FRAME_ACK)
            {
                currtent_state = SEND_STATE;
            }
            else //if(frame_type == FRAME_NACK)
            {
                currtent_state = RESEND_STATE;
            }
            //else //    FRAME_CORRUPT  FRAME_DUMMY  FRAME_DATA
            //{
            //    currtent_state = NACK_STATE;
            //}
            break;
        case RESEND_STATE:
            if(frame_type == FRAME_DATA || frame_type == FRAME_DUMMY)
            {
                currtent_state = ACK_STATE;
            }
            else //  FRAME_NACK  FRAME_CORRUPT    FRAME_ACK
            {
                currtent_state = NACK_STATE;
            }
            break;
        default:
            
            break;

    }
	return;
}


static int ipcStateMachineThread(void *arg)
{
	while (1)
	{
		//wait_for_completion_timeout的延时单位是系统所用的时间片jiffies
		//wait_for_completion_timeout(&pHardwareInfo->FlyIICInfo.comMcuIICThread, (HZ / 10)); //100ms
		wait_for_completion(&ipcStateMachineCom);
		mutex_lock(&userDataProcMutex);

        switch (currtent_state)
        {
            case SEND_STATE:
                if(TxDataFrameLen == DUMMY_FRAME_LEN)
	                data_transmission(dummy_frame, DUMMY_FRAME_LEN, mcuDataFrame, &mcuDataFrameLen);
	            else
					data_transmission(TxDataFrame, TxDataFrameLen, mcuDataFrame, &mcuDataFrameLen);
                break;
            case ACK_STATE:
                data_transmission(ack_frame, ACK_FRAME_LEN, mcuDataFrame, &mcuDataFrameLen);
                break;
            case NACK_STATE:
                data_transmission(nack_frame, NACK_FRAME_LEN, mcuDataFrame, &mcuDataFrameLen);
                break;
            case RESEND_STATE:
                data_transmission(LastDataFrame, LastDataFrameLen, mcuDataFrame, &mcuDataFrameLen);
                break;
        }

        mcuDataFrameProc(mcuDataFrame);

        if(currtent_state == SEND_STATE)
        {
        	complete(&userDataFifoProcCom);
        }
  		if(currtent_state == RESEND_STATE)
  		{
  			Enable(socSRQEnable);
  			complete(&ipcStateMachineCom);
  		}
  		mutex_unlock(&userDataProcMutex);  
	}

	do_exit(0);
	return 0;
}

static int userDataProcThread(void *arg)
{
    char tmpData[256] = {0};
    char tmpOneFrame[256] = {0};
    char tmpLastFrame[256] = {0};
    unsigned short  tmpDataLen = 0;
    unsigned short  tmpLastFrameLen = 0;
    unsigned short  tmpSumDataLen = 0;
    unsigned short  checksum = 0;
    
    while(1)
    {
        wait_for_completion(&userDataProcCom);
        do
        {
            mutex_lock(&userDataProcMutex);
            
            memset(TxDataFrame, 0, sizeof(TxDataFrame));

            if(tmpLastFrame)
            {
            	memcpy(TxDataFrame+4, tmpLastFrame, tmpLastFrameLen);
            	memset(tmpLastFrame, 0, sizeof(tmpLastFrame));
            	tmpSumDataLen += tmpLastFrameLen;
            }

            tmpDataLen = get_buf_fifo(&write_data_fifo, tmpData, &write_fifo_lock);

		if(tmpDataLen <= 0)
		{
			if(tmpSumDataLen)
			    {
				//struct data frame
				tmpSumDataLen += 6;
				TxDataFrameLen = tmpSumDataLen;

				TxDataFrame[0] = tmpSumDataLen >> 8;
				TxDataFrame[1] = tmpSumDataLen & 0x00ff;
				TxDataFrame[2] = SOC_frame_num >> 8;
				TxDataFrame[3] = SOC_frame_num & 0x00ff;
				
				checksum = IPC_CRC(TxDataFrame, tmpSumDataLen-2);
				TxDataFrame[tmpSumDataLen-2] = (checksum & 0xFF00)>>8;
				TxDataFrame[tmpSumDataLen-1] = (checksum & 0x00FF)>>0; 

				memset(LastDataFrame, 0, sizeof(LastDataFrame));
				memcpy(LastDataFrame, TxDataFrame, TxDataFrameLen);
				LastDataFrameLen = tmpSumDataLen;

				SOC_frame_num++;
				if(SOC_frame_num == 0xFF00)
					SOC_frame_num = 0x0000;
				 
				tmpSumDataLen = 0;

				mutex_unlock(&userDataProcMutex);
				complete(&ipcStateMachineCom);
				wait_for_completion(&userDataFifoProcCom);
				break;
			    }
			else
			    {
				memcpy(TxDataFrame, dummy_frame, sizeof(dummy_frame));
				TxDataFrameLen = sizeof(dummy_frame);
				mutex_unlock(&userDataProcMutex);
				break;
			    }
		}
            else
		{
			while(tmpDataLen)
			{
				if(tmpSumDataLen+tmpDataLen > DATALEN)
				{
					memcpy(tmpLastFrame+2, tmpData, tmpDataLen);
					tmpLastFrame[0] = tmpDataLen >> 8;
					tmpLastFrame[1] = tmpDataLen & 0x00ff;
					tmpLastFrameLen = tmpDataLen+2;
					break;
				}
			
				memcpy(tmpOneFrame+2, tmpData, tmpDataLen);
				tmpOneFrame[0] = tmpDataLen >> 8;
				tmpOneFrame[1] = tmpDataLen & 0x00ff;
				memcpy(TxDataFrame+tmpSumDataLen+4, tmpOneFrame, tmpDataLen+2);
				tmpSumDataLen += tmpDataLen+2;
				
				memset(tmpData, 0, sizeof(tmpData));
				memset(tmpOneFrame, 0, sizeof(tmpOneFrame));
				tmpDataLen = get_buf_fifo(&write_data_fifo, tmpData, &write_fifo_lock);
		        }
				      
			//struct data frame
			tmpSumDataLen += 6;
			TxDataFrameLen = tmpSumDataLen;

			TxDataFrame[0] = tmpSumDataLen >> 8;
			TxDataFrame[1] = tmpSumDataLen & 0x00ff;
			TxDataFrame[2] = SOC_frame_num >> 8;
			TxDataFrame[3] = SOC_frame_num & 0x00ff;
			
			checksum = IPC_CRC(TxDataFrame, tmpSumDataLen-2);
    			TxDataFrame[tmpSumDataLen-2] = (checksum & 0xFF00)>>8;
    			TxDataFrame[tmpSumDataLen-1] = (checksum & 0x00FF)>>0; 

    			memset(LastDataFrame, 0, sizeof(LastDataFrame));
    			memcpy(LastDataFrame, TxDataFrame, TxDataFrameLen);
    			LastDataFrameLen = tmpSumDataLen;

			SOC_frame_num++;
			if(SOC_frame_num == 0xFF00)
			{
				SOC_frame_num = 0x0000;
			}
			tmpSumDataLen = 0;

			mutex_unlock(&userDataProcMutex);
			complete(&ipcStateMachineCom);
			wait_for_completion(&userDataFifoProcCom);
		 } 
        }while(tmpDataLen > 0);
    }
    do_exit(0);
    return 0;
}




irqreturn_t MCUSRQ_isr(int irq, void *dev_id)
{
	debugPrintf("%s\n", __FUNCTION__);

    //schedule_work(&dataTransWork);
    
    Disable(&socSRQEnable);

    complete(&ipcStateMachineCom);
    
    return IRQ_HANDLED;
}

void spi_ipc_init(void)
{
  	struct task_struct *ipcStateMachineTask;
  	struct task_struct *userDataProcTask;
    
	spi_api_do_set(0,0,8,8);

	init_waitqueue_head(&wait_queue);  //等待队列初始化

    spin_lock_init(&flaglock);

    spin_lock_init(&userDataProcLock);

    init_thread_kfifo(&read_data_fifo, FIFO_SIZE);
    spin_lock_init(&read_fifo_lock);
     
    init_thread_kfifo(&write_data_fifo, FIFO_SIZE);
    spin_lock_init(&write_fifo_lock);

    init_completion(&userDataFifoProcCom);
        
	//INIT_WORK(&dataTransWork,data_transmission);
	SOC_IO_ISR_Add(MCU_SRQ_GPIO, IRQF_TRIGGER_FALLING, MCUSRQ_isr, NULL);
	SOC_IO_Input(MCU_SRQ_GPIO, MCU_SRQ_GPIO, 0);

    init_completion(&userDataProcCom);
    userDataProcTask = kthread_run(userDataProcThread, NULL, "userDataProcThread");
    if (IS_ERR(userDataProcTask)){
        debugPrintf("kernel thread creat userDataProcThread error!\n");
    }

    init_completion(&ipcStateMachineCom);
    ipcStateMachineTask = kthread_run(ipcStateMachineThread, NULL, "ipcStateMachineThread");
    if (IS_ERR(ipcStateMachineTask)){
        debugPrintf("kernel thread creat ipcStateMachineThread error!\n");
    }

}


static unsigned int ipc_poll(struct file *filp, struct poll_table_struct *wait)
{
//	debugPrintf("%s\n", __FUNCTION__);
    unsigned long irqflags; 

 	unsigned int mask = 0;

	poll_wait(filp, &wait_queue, wait);//等待 直到被唤醒

	if(!kfifo_is_empty(&read_data_fifo))
    {   
		mask |= POLLIN | POLLRDNORM;
    }
    
	return mask;

}

static int ipc_open(struct inode *inode, struct file *filp)
{  

	debugPrintf("%s\n", __FUNCTION__);

	return 0;  
}  

 ssize_t ipc_read(struct file *pfile, char *user_buf, size_t len, loff_t *off)
{
	char buf[1024];
    len = get_buf_fifo(&read_data_fifo, buf, &read_fifo_lock);    
    debugBuf(__FUNCTION__, buf, 5);
	if(!copy_to_user(user_buf, buf, len))
	{
		return len;
	}
	return 0;
}


 ssize_t ipc_write(struct file *pfile, const char *user_buf, size_t len, loff_t *off)
{
	int ret = 0;
	char buf[1024];
	if(!copy_from_user(buf, user_buf, len))
	{
	
        debugBuf(__FUNCTION__, buf, len);
        put_buf_fifo(&write_data_fifo, buf, len, &write_fifo_lock);
        complete(&userDataProcCom);
	}
	return len;
}

static struct file_operations ipc_flops = 
{
    .owner  =   THIS_MODULE,
    .open   =   ipc_open,
    .read   =   ipc_read,     
    .write  =   ipc_write,
    .poll   =   ipc_poll,
};

//1 定义cdev结构体变量和dev_nr主从设备号变量
static struct cdev ipc_cdev;
static dev_t dev_nr;



static int __init ipc_init(void)
{
    debugTagSet(TAG);

	int res;
	struct device *ipc_device;
	//1 动态申请主从设备号
	res = alloc_chrdev_region(&dev_nr, 0, 1, "ipc_chrdev");
	if(res){
    	debugPrintf("==>alloc chrdev region failed!\n");
        goto chrdev_err;
	} 
	//3 初始化cdev数据
	cdev_init(&ipc_cdev, &ipc_flops);
	//4 添加cdev变量到内核，完成驱动注册
	res = cdev_add(&ipc_cdev, dev_nr, DEVICE_COUNT);
	if(res){
		debugPrintf("==>cdev add failed!\n");
        goto cdev_err;
	}

	//创建设备类
	ipc_class = class_create(THIS_MODULE,"ipc_class");
	if(IS_ERR(ipc_class)){
	 	 res =  PTR_ERR(ipc_class);
    goto class_err;
	}

	//创建设备节点
	ipc_device = device_create(ipc_class,NULL, dev_nr, NULL,"s32k_ipc");
	if(IS_ERR(ipc_device)){
   	   	res = PTR_ERR(ipc_device);
       	goto device_err;
    }

    spi_ipc_init();
    
	debugPrintf(DEVICE_NAME " initialized.\n");
	
	return 0;

device_err:
	device_destroy(ipc_class, dev_nr);
	class_destroy(ipc_class);

class_err:
	cdev_del(&ipc_cdev); 

cdev_err:
	unregister_chrdev_region(dev_nr, DEVICE_COUNT);

chrdev_err:
	//申请主设备号失败

	return res;
    
}

static void __exit ipc_exit(void)
{
 	debugPrintf("==>demo_exit\n");
    
	//5 删除添加的cdev结构体，并释放申请的主从设备号
	cdev_del(&ipc_cdev);    
	unregister_chrdev_region(dev_nr, DEVICE_COUNT);

	device_destroy(ipc_class, dev_nr);
	class_destroy(ipc_class);
}

module_init(ipc_init);
module_exit(ipc_exit);
MODULE_AUTHOR("DONG");
MODULE_LICENSE("GPL");
