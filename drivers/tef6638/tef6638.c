
#include "lidbg.h"

#include "tef6638.h"

#include "TEF6638_Audio_data.h"

#include "tef6638_audio_conf.h"

LIDBG_DEFINE;

void TEF6638_Input(BYTE channel);
void TEF6638_Volume(BYTE Volume);


int tef6638_open (struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t tef6638_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char *cmd[8] = {NULL};
    int cmd_num  = 0;
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    lidbg("-----FLYSTEP------------------[%s]---\n", cmd_buf);

    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;

    if(!strcmp(cmd[0], "i2s"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Input(MediaMP3);
    }

    if(!strcmp(cmd[0], "radio"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Input(RADIO);
    }

    if(!strcmp(cmd[0], "ain0"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Input(IPOD);
    }

    if(!strcmp(cmd[0], "ain1"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Input(BT);
    }

    if(!strcmp(cmd[0], "ain2"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Input(AUX);
    }

    if(!strcmp(cmd[0], "vol50"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Volume(50);
    }

    if(!strcmp(cmd[0], "vol30"))
    {
        lidbg("case:[%s]\n", cmd[0]);
        TEF6638_Volume(30);
    }
    return size;
}

static  struct file_operations tef6638_fops =
{
    .owner = THIS_MODULE,
    .open = tef6638_open,
    .write = tef6638_write,
};



BOOL I2C_Read_TEF6638(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
    BYTE i;
    lidbg("Enter %s().\n", __func__);
    if (SOC_I2C_Rec_TEF6638(TEF6638_I2C_BUS, (TEF6638_ADDR_R >> 1), ulRegAddr, pRegValBuf, uiValBufLen) < 0)
    {

        lidbg("TEF6638 IIC read error!\n");
        return FALSE;
    }

    lidbg("FlyAduio TEF6638 IIC Read-->\n");
    for (i = 0; i < uiValBufLen + 3; i++)
    {
        lidbg("TEF6638 IIC Read:0x%x\n", pRegValBuf[i]);
    }

    return TRUE;
}



BOOL I2C_Write_TEF6638(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
    //BYTE i;
    BYTE buff[300];
    if(((ulRegAddr >> 16) & 0xff) == 0xF2)
    {
        buff[2] = ulRegAddr & 0xff;
        buff[1] = (ulRegAddr >> 8);

        buff[0] = (ulRegAddr >> 16) & 0xff;
        memcpy(&buff[3], pRegValBuf, uiValBufLen);

        /*
        lidbg("FlyAduio TEF6638 IIC Write-->\n");
        for (i = 0; i < uiValBufLen+3; i++)
        {
        	lidbg(" %2X",buff[i]);
        }
        */
        if (SOC_I2C_Send(TEF6638_I2C_BUS, (TEF6638_ADDR_W >> 1), buff, uiValBufLen + 3) < 0)
        {
            lidbg("TEF6638 IIC write error!\n");
            return FALSE;
        }
    }
    else
    {
        buff[0] = (ulRegAddr >> 16) & 0xff;
        memcpy(&buff[1], pRegValBuf, uiValBufLen);
        if (SOC_I2C_Send(TEF6638_I2C_BUS, (TEF6638_ADDR_W >> 1), buff, uiValBufLen + 1) < 0)
        {
            lidbg("TEF6638 IIC write error!\n");
            return FALSE;
        }
    }



    return TRUE;
}

static BYTE getInuptGain(BYTE input)
{
    BYTE i = 0;

    while (TEF6638_audioChannelGainTab[i].input)
    {
        if (input == TEF6638_audioChannelGainTab[i].input)
        {
            return TEF6638_audioChannelGainTab[i].gain;
        }
        i++;
    }

    return TEF6638_audioChannelGainTab[i].gain;
}


void TEF6638_Input(BYTE channel)
{
    BYTE reg[2] = {0x20, 0x00};//Audio ICC_P_INPUT=00(Radio def)
    BYTE regGain[5] = {0xF2, 0x42, 0xFB, 0x00, 0x00};//ADSP_Y_Vol_SrcScalP

    UINT regAddr;

    BYTE InputGain = getInuptGain(channel);
    //BYTE InputGain = TEF6638_audioChannelGainTab[channel]

    if (efIsExboxChannel(channel))
    {
        channel = TV;
    }

    switch(channel)
    {
    case Init:
        reg[1] = INIT_INPUT_ADDR;
        printk(KERN_CRIT "Init");
        break;
        IIS1_INPUT_CASE				//IIS1
        reg[1] = I2S1_SUBADDR;
        regGain[3] = TEF6638_InputGain_Data_PS[InputGain * 2 + 1];
        regGain[4] = TEF6638_InputGain_Data_PS[InputGain * 2 + 2];
        printk(KERN_CRIT "MediaMP3");
        break;
        AIN0_INPUT_CASE				//AIN0
        reg[1] = AIN0_SUBADDR;
        regGain[3] = TEF6638_InputGain_Data_PS[InputGain * 2 + 1];
        regGain[4] = TEF6638_InputGain_Data_PS[InputGain * 2 + 2];
        printk(KERN_CRIT "AUX");
        break;
        break;
        AIN1_INPUT_CASE                               //AIN1
        reg[1] = AIN1_SUBADDR;
        regGain[3] = TEF6638_InputGain_Data_PS[InputGain * 2 + 1];
        regGain[4] = TEF6638_InputGain_Data_PS[InputGain * 2 + 2];
        printk(KERN_CRIT "IPOD / Exbox / SYSTEM_RING");
        lidbg("Select AIN1\n");
        break;
        AIN2_3_INPUT_CASE				//AIN2&AIN3
        reg[1] = AIN2_3_SUBADDR;
        regGain[3] = TEF6638_InputGain_Data_PS[InputGain * 2 + 1];
        regGain[4] = TEF6638_InputGain_Data_PS[InputGain * 2 + 2];
        printk(KERN_CRIT "A2DP / BT");
        break;
        SPDIF0_INPUT_CASE					//SPDIF0
        reg[1] = SPDIF_SUBADDR;
        regGain[3] = TEF6638_InputGain_Data_PS[InputGain * 2 + 1];
        regGain[4] = TEF6638_InputGain_Data_PS[InputGain * 2 + 2];
        printk(KERN_CRIT "MediaCD");
        break;
        RADIO_INPUT_CASE					//Radio
        reg[1] = RADIO_SUBADDR;
        regGain[3] = TEF6638_InputGain_Data_PS[InputGain * 2 + 1];
        regGain[4] = TEF6638_InputGain_Data_PS[InputGain * 2 + 2];
        printk(KERN_CRIT "RADIO");
        break;
    default:
        printk(KERN_CRIT "xxxxxxx");
        return;
        break;
    }

    //ipcDriverStart(IPC_DRIVER_EVENT_MAIN_AUDIO_INPUT,channel,NULL,NULL);

    regAddr = reg[0] << 16;
    I2C_Write_TEF6638(regAddr, &reg[1], 1);
    regAddr = (regGain[0] << 16) + (regGain[1] << 8) + regGain[2];
    I2C_Write_TEF6638(regAddr, &regGain[3], 2);
    /*
    	if (MediaMP3 == channel || MediaSystem == channel)
    	{
    		ipcDriverStart(IPC_DRIVER_AUDIO_OUT_IIS,1,NULL,NULL);
    	}
    	else
    	{
    		ipcDriverStart(IPC_DRIVER_AUDIO_OUT_IIS,1,NULL,NULL);
    	}
    */
}



void TEF6638_Volume(BYTE Volume)
{
    BYTE reg1[5] = {0xF2, 0x43, 0x1E, 0x00, 0x00};//ADSP_Y_Vol_Main1P(Primary Maximum volume gain)
    BYTE reg2[5] = {0xF2, 0x43, 0x1F, 0x00, 0x00};//ADSP_Y_Vol_Main2P(Primary Maximum loudness boost)

    UINT regAddr;

    lidbg("TEF6638 Volume:%d\n", Volume);

    if (Volume > 54)
        Volume = 54;

    reg1[3] = TEF6638_Volume_Data[4 * Volume + 0];
    reg1[4] = TEF6638_Volume_Data[4 * Volume + 1];
    reg2[3] = TEF6638_Volume_Data[4 * Volume + 2];
    reg2[4] = TEF6638_Volume_Data[4 * Volume + 3];

    regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
    I2C_Write_TEF6638(regAddr, &reg1[3], 2);
    regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
    I2C_Write_TEF6638(regAddr, &reg2[3], 2);
}

EXPORT_SYMBOL(TEF6638_Volume);


BOOL SendToTEF6638NormalWriteData(BYTE *pData)
{
    BYTE MChipAdd;
    UINT iLength;
    UINT dataCnt = 0;
    UINT regAddr;

    lidbg("Enter %s().\n", __func__);

    while (*pData)
    {
        dataCnt++;
        MChipAdd = *pData++;
        if(MChipAdd != TEF6638_ADDR_W)
        {
            lidbg("init array error!\n");
            break;
        }
        iLength = *pData++;
        /*
        if(pData[0] == 0x00 && pData[1] == 0xFF && pData[2] == 0xFF)
        {
        	pData += iLength;
        	continue;
        }
        else
        {
        	regAddr = (pData[0] << 16) + (pData[1] << 8) + pData[2];
        	if(!I2C_Write_SAF7741(regAddr,&pData[3],iLength - 3))
        	{
        		//lidbg("SAF7741 I2C comm error!\n");
        		return FALSE;
        	}
        	pData += iLength;
        }
        */
        //X ram or Y ram (3 bytes Addr)
        if(pData[0] == 0xF2)
        {
            regAddr = (pData[0] << 16) + (pData[1] << 8) + pData[2];
            if(!I2C_Write_TEF6638(regAddr, &pData[3], iLength - 3))
            {
                //lidbg("TEF6638 I2C lowlevel comm error!\n");
                return FALSE;
            }
            pData += iLength;
        }
        else
        {
            regAddr = pData[0] << 16;
            if(!I2C_Write_TEF6638(regAddr, &pData[1], iLength - 1))
            {
                //lidbg("TEF6638 I2C highlevel comm error!\n");
                return FALSE;
            }
            pData += iLength;
        }
    }

    return TRUE;
}


BOOL TEF6638_Init(void)
{
    int i;
    //lidbg("%s:enter\n", __func__);
    lidbg("TEF6638 init start!\n");
    for(i = 0; i < 3; ++i)

    {
        if(SendToTEF6638NormalWriteData(TEF6638_Audio_Init_Data))
        {
            return TRUE;
        }
        else
        {
            lidbg("TEF6638  I2C err num:%d\n", i);
            msleep(1000);
            continue;
        }
    }

    return FALSE;
    //SendToTEF6638NormalWriteData(TEF6638_Audio_Init_Data_FM);
    //SendToTEF6638NormalWriteData(TEF6638_Radio_Init_Data);

}

void send_cmds_to_lpc(void)
{
    lidbg("TEF6638:send some cmds to LPC\n");

    LPC_CMD_4052A2_L;       //select MP3_L/MP3_R

    LPC_CMD_7388MUTE_H;     //forbid 7388 MUTE
    msleep(100);
    LPC_CMD_7388STANDBY_H;  //let 7388 work
}

static int tef6638_probe(struct platform_device *pdev)
{
    //int ret = 0;

    if((g_var.is_fly) || (g_var.recovery_mode))
    {
        lidbg("tef6638_probe do nothing\n");
        return 0;
    }

    //lidbg("%s:enter\n", __func__);

    LPC_CMD_6638RST_L;      //6638 RESET
    msleep(1000);
    LPC_CMD_6638RST_H;
    msleep(1000);

    if(TEF6638_Init())
    {
        lidbg_new_cdev(&tef6638_fops, "tef66380");//add cdev
        send_cmds_to_lpc();
        lidbg("TEF6638 init succeed!\n");
    }
    else
        lidbg("TEF6638 init failed!\n");

    TEF6638_Input(MediaMP3);
    TEF6638_Volume(20);
    return 0;

}

static int tef6638_remove(struct platform_device *pdev)
{
    //gpio_free(TEF6638_GPIO_RST);
    return 0;
}

/*
static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,
};
*/

static struct platform_device tef6638_devices =
{
    .name			= "tef6638",
    .id 			= 0,
};

static struct platform_driver tef6638_driver =
{
    .probe = tef6638_probe,
    .remove = tef6638_remove,
    .driver = {
        .name = "tef6638",
        .owner = THIS_MODULE,
    },
};

static int tef6638_module_init(void)
{
    //lidbg("%s:enter\n", __func__);
    LIDBG_GET;
    platform_device_register(&tef6638_devices);
    platform_driver_register(&tef6638_driver);
    //misc_register(&misc);
    return 0;

}

static void __exit tef6638_module_exit(void)
{
    platform_driver_unregister(&tef6638_driver);
    platform_device_unregister(&tef6638_devices);
}


module_init(tef6638_module_init);
module_exit(tef6638_module_exit);
MODULE_LICENSE("GPL");

