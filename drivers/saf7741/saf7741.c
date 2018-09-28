
#include "lidbg.h"

#include "saf7741.h"

#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
#include "SAF7741_125_Audio_data.h"
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
#include "SAF7741_140_Audio_data.h"
#endif

#include "saf7741_audio_conf_125.h"

LIDBG_DEFINE;

BOOL I2C_Read_SAF7741(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
    BYTE i;
    lidbg("Enter %s().\n", __func__);
    if (SOC_I2C_Rec_SAF7741(SAF7741_I2C_BUS, (SAF7741_ADDR_R >> 1), ulRegAddr, pRegValBuf, uiValBufLen) < 0)
    {

        lidbg("SAF7741 IIC read error!\n");
        return FALSE;
    }

    lidbg("FlyAduio SAF7741 IIC Read-->\n");
    for (i = 0; i < uiValBufLen + 3; i++)
    {
        lidbg("SAF7741 IIC Read:0x%x\n", pRegValBuf[i]);
    }

    return TRUE;
}



BOOL I2C_Write_SAF7741(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
    //BYTE i;
    BYTE buff[300];

    buff[2] = ulRegAddr & 0xff;
    buff[1] = (ulRegAddr >> 8);

    buff[0] = (ulRegAddr >> 16) & 0xff;
    memcpy(&buff[3], pRegValBuf, uiValBufLen);

    //lidbg("FlyAduio SAF7741 IIC Write-->\n");
    /*	for (i = 0; i < uiValBufLen+3; i++)
    	{
    		//lidbg(" %2X",buff[i]);
    	}
    */
    if (SOC_I2C_Send(SAF7741_I2C_BUS, (SAF7741_ADDR_W >> 1), buff, uiValBufLen + 3) < 0)
    {
        lidbg("SAF7741 IIC write error!\n");
        return FALSE;
    }

    return TRUE;
}

static BYTE getInuptGain(BYTE input)
{
    BYTE i = 0;

    while (SAF7741_audioChannelGainTab[i].input)
    {
        if (input == SAF7741_audioChannelGainTab[i].input)
        {
            return SAF7741_audioChannelGainTab[i].gain;
        }
        i++;
    }

    return SAF7741_audioChannelGainTab[i].gain;
}

void SAF7741_EnableTape(void)
{
#if SAF7741_SILICON_SEL == SAF7741_SILICON_140
    UINT regAddr;

    BYTE reg[6] = {0x0D, 0x00, 0x6A, 0x00, 0x00, 0x8A}; //XA_EasyP_Index

    regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
    I2C_Write_SAF7741(regAddr, &reg[3], 3);
#endif
}

void SAF7741_Input(BYTE channel)
{
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg[6] = {0x0D, 0x00, 0x6F, 0x00, 0x00, 0x00};//XA_EasyP_Index
    BYTE regGain[5] = {0x0D, 0x10, 0x2D, 0x00, 0x00};//ADSP_Y_Vol_SrcScalP
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    BYTE reg[6] = {0x0D, 0x00, 0x6A, 0x00, 0x00, 0x00};//XA_EasyP_Index
    BYTE regGain[5] = {0x0D, 0x10, 0x45, 0x00, 0x00};//ADSP_Y_Vol_SrcScalP
#endif
    UINT regAddr;

    BYTE InputGain = getInuptGain(channel);
    //BYTE InputGain = SAF7741_audioChannelGainTab[channel]

    if (efIsExboxChannel(channel))
    {
        channel = TV;
    }

    switch(channel)
    {
    case Init:
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x25;	//I2S 1  空出来没用的
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x15;	//Radio2  空出来没用的
#endif
        printk(KERN_CRIT "Init");
        break;
    case MediaMP3:									//IIS 4
    case MediaSystem:
    case SYSTEM_RING:
    case BT_RING:
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x1F;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x25;
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "MediaMP3");
        break;
    case AUX :                                       //AIN0
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x19;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x19;
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "AUX");
        break;
        break;
    case IPOD :                                     //AIN1
    case TV:
    case VAP:
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x1D;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x1D;
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "IPOD / Exbox / SYSTEM_RING");
        lidbg("Select AIN1\n");
        break;
    case A2DP:
    case BT:
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x1D;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x1D;
        SAF7741_EnableTape();
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "A2DP / BT");
        break;
    case EXT_TEL:									//AIN4
    case GR_AUDIO:
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x31;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x31;
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "EXT_TEL / GPS");
        break;
    case MediaCD:									//SPDIF1
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x2B;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x2B;
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "MediaCD");
        break;
    case RADIO:										//Radio1
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x13;
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
        reg[3] = 0x00;
        reg[4] = 0x00;
        reg[5] = 0x13;
#endif
        regGain[3] = SAF7741_InputGain_Data_PS[InputGain * 2 + 0];
        regGain[4] = SAF7741_InputGain_Data_PS[InputGain * 2 + 1];
        printk(KERN_CRIT "RADIO");
        break;
    default:
        printk(KERN_CRIT "xxxxxxx");
        return;
        break;
    }

    //ipcDriverStart(IPC_DRIVER_EVENT_MAIN_AUDIO_INPUT,channel,NULL,NULL);

    regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
    I2C_Write_SAF7741(regAddr, &reg[3], 3);
    regAddr = (regGain[0] << 16) + (regGain[1] << 8) + regGain[2];
    I2C_Write_SAF7741(regAddr, &regGain[3], 2);
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

void SAF7741_Balance_P(BYTE ibalance)
{
    //printk("\nFlyAudio SAF7741:Set Balance---->%d",ibalance);
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg1[5] = {0x0D, 0x10, 0x25, 0x00, 0x00};//ADSP_Y_Vol_BalPL
    BYTE reg2[5] = {0x0D, 0x10, 0x26, 0x00, 0x00};//ADSP_Y_Vol_BalPR
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    BYTE reg1[5] = {0x0D, 0x10, 0x41, 0x00, 0x00};//ADSP_Y_Vol_BalPL
    BYTE reg2[5] = {0x0D, 0x10, 0x42, 0x00, 0x00};//ADSP_Y_Vol_BalPR
#endif
    UINT regAddr;

    if(ibalance > (BALANCE_LEVEL_COUNT - 1)) ibalance = BALANCE_LEVEL_COUNT - 1;

    reg1[3] = SAF7741_Balance_Fader_Data_P[4 * ibalance + 0];
    reg1[4] = SAF7741_Balance_Fader_Data_P[4 * ibalance + 1];
    reg2[3] = SAF7741_Balance_Fader_Data_P[4 * ibalance + 2];
    reg2[4] = SAF7741_Balance_Fader_Data_P[4 * ibalance + 3];

    regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
    I2C_Write_SAF7741(regAddr, &reg1[3], 2);
    regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
    I2C_Write_SAF7741(regAddr, &reg2[3], 2);
}

void SAF7741_Fader_P(BYTE ifader)
{
    //printk("\nFlyAudio SAF7741:Set Fader---->%d",ifader);
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg1[5] = {0x0D, 0x10, 0x23, 0x00, 0x00};//YA_Vol_FadF
    BYTE reg2[5] = {0x0D, 0x10, 0x24, 0x00, 0x00};//YA_Vol_FadR
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    BYTE reg1[5] = {0x0D, 0x10, 0x3F, 0x00, 0x00};//YA_Vol_FadF
    BYTE reg2[5] = {0x0D, 0x10, 0x40, 0x00, 0x00};//YA_Vol_FadR
#endif
    UINT regAddr;

    if (ifader > (FADER_LEVEL_COUNT - 1)) ifader = FADER_LEVEL_COUNT - 1;

    reg1[3] = SAF7741_Balance_Fader_Data_P[4 * ifader + 0];
    reg1[4] = SAF7741_Balance_Fader_Data_P[4 * ifader + 1];
    reg2[3] = SAF7741_Balance_Fader_Data_P[4 * ifader + 2];
    reg2[4] = SAF7741_Balance_Fader_Data_P[4 * ifader + 3];

    regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
    I2C_Write_SAF7741(regAddr, &reg1[3], 2);
    regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
    I2C_Write_SAF7741(regAddr, &reg2[3], 2);
}

void SendToSAF7741UsingPortByLength(BYTE *p, BYTE level)
{
    UINT len;
    BYTE *p1, *p2, *p3;
    UINT regAddr;

    p2 = p;
    p3 = p2 + (1 + 1 + 3 + 3);
    len = p3[7];
    p1 = p3 + (1 + 1 + 3 + 3) + level * (5 + len * 2);

    regAddr = (p1[2] << 16) + (p1[3] << 8) + p1[4];
    I2C_Write_SAF7741(regAddr, &p1[5], p1[1] - 3);

    regAddr = (p2[2] << 16) + (p2[3] << 8) + p2[4];
    I2C_Write_SAF7741(regAddr, &p2[5], p2[1] - 3);

    regAddr = (p3[2] << 16) + (p3[3] << 8) + p3[4];
    I2C_Write_SAF7741(regAddr, &p3[5], p3[1] - 3);
}

void SAF7741_DesScalBMT_P(void)
{
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg[5] = {0x0D, 0x10, 0x77, 0x00, 0x00};//ADSP_Y_Vol_DesScalBMTP
    UINT regAddr;

    UINT /*Temp1=0,*/Temp2 = 6;
    //Temp1 = LARGER(pFlyAudioInfo->sFlyAudioInfo.curBassLevel,pFlyAudioInfo->sFlyAudioInfo.curMidLevel);
    //Temp2 = LARGER(Temp1,pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel);

    reg[3] = SAF7741_DesScalBMT_Data[Temp2 * 2 + 0];
    reg[4] = SAF7741_DesScalBMT_Data[Temp2 * 2 + 1];

    //DBG0("\nFlyAudio SAF7741:Updata DesScalBMT---->%d",Temp2);

    regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
    I2C_Write_SAF7741(regAddr, &reg[3], 2);
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    return;
#endif
}

void SAF7741_Bass(BYTE BassFreq, BYTE BassLevel)
{
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg[5] = {0x0D, 0x14, 0x25, 0x00, 0x00};//ADSP_Y_BMT_GbasP
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    BYTE reg[5] = {0x0D, 0x10, 0xB1, 0x00, 0x00};//ADSP_Y_BMT_GbasP
#endif
    UINT regAddr;

    if (BassFreq > (BASS_FREQ_COUNT - 1)) BassFreq = (BASS_FREQ_COUNT - 1);
    if (BassLevel > (BASS_LEVEL_COUNT - 1)) BassLevel = BASS_LEVEL_COUNT - 1;

    reg[3] = SAF7741_Bass_Data_P[BassLevel * 2 + 0]; //60Hz,80Hz,100Hz数据一样
    reg[4] = SAF7741_Bass_Data_P[BassLevel * 2 + 1];

    //printk("\nFlyAudio SAF7741:Set Bass Freq-->%d,Level-->%d",BassFreq,BassLevel);

    SendToSAF7741UsingPortByLength((BYTE *)BassFreqsel, BassFreq);

    regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
    I2C_Write_SAF7741(regAddr, &reg[3], 2);

    SAF7741_DesScalBMT_P();
}

void SAF7741_Mid(BYTE MidFreq, BYTE MidLevel)
{
    //printk("\nFlyAudio SAF7741:Set Mid Freq-->%d,Level-->%d",MidFreq,MidLevel);

    if (MidFreq > (MID_FREQ_COUNT - 1)) MidFreq = (MID_FREQ_COUNT - 1);
    if (MidLevel > (MID_LEVEL_COUNT - 1)) MidLevel = MID_LEVEL_COUNT - 1;

    if (MidFreq == M_FREQ_500)
    {
        SendToSAF7741UsingPortByLength((BYTE *)SAF7741_Mid_500_Data_P, MidLevel);
    }
    else if (MidFreq == M_FREQ_1000)
    {
        SendToSAF7741UsingPortByLength((BYTE *)SAF7741_Mid_1000_Data_P, MidLevel);
    }
    else if (MidFreq == M_FREQ_1500)
    {
        SendToSAF7741UsingPortByLength((BYTE *)SAF7741_Mid_1500_Data_P, MidLevel);
    }

    SAF7741_DesScalBMT_P();
}

void SAF7741_Treble(BYTE TrebFreq, BYTE TrebLevel)
{
    //printk("\nFlyAudio SAF7741:Set Treble Freq-->%d,Level-->%d",TrebFreq,TrebLevel);

    if (TrebFreq > (TREB_FREQ_COUNT - 1)) TrebFreq = TREB_FREQ_COUNT - 1;
    if (TrebLevel > (TREB_LEVEL_COUNT - 1)) TrebLevel = TREB_LEVEL_COUNT - 1;

    if (TrebFreq == T_FREQ_10K)
    {
        SendToSAF7741UsingPortByLength((BYTE *)SAF7741_Treble_10k_Data_P, TrebLevel);
    }
    else if (TrebFreq == T_FREQ_12K)
    {
        SendToSAF7741UsingPortByLength((BYTE *)SAF7741_Treble_12k_Data_P, TrebLevel);
    }

    SAF7741_DesScalBMT_P();
}

void SAF7741_Mute_P(BOOL para)
{
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg[5] = {0x0D, 0x10, 0x6D, 0x00, 0x00};//ADSP_Y_Mute_P
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    BYTE reg[5] = {0x0D, 0x10, 0x72, 0x00, 0x00};//ADSP_Y_Mute_P
#endif
    UINT regAddr;

    if(para)
    {
        reg[3] = 0x00;
        reg[4] = 0x00;
        lidbg("FlyAudio SAF7741:Mute P---->ON\n");
    }
    else
    {
        reg[3] = 0x08;
        reg[4] = 0x00;
        lidbg("FlyAudio SAF7741:Mute P---->OFF\n");
    }

    regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
    I2C_Write_SAF7741(regAddr, &reg[3], 2);
}

void SAF7741_Mute(BOOL para)
{
    printk("\nFlyAudio SAF7741_Mute-->%d", para);
    if(para)
    {
        SAF7741_Mute_P(TRUE);
    }
    else
    {
        SAF7741_Mute_P(FALSE);
    }
}

void SAF7741_Volume(BYTE Volume)
{
#if SAF7741_SILICON_SEL == SAF7741_SILICON_125
    BYTE reg1[5] = {0x0D, 0x10, 0x50, 0x00, 0x00};//ADSP_Y_Vol_Main1P
    BYTE reg2[5] = {0x0D, 0x10, 0x51, 0x00, 0x00};//ADSP_Y_Vol_Main2P
#elif SAF7741_SILICON_SEL == SAF7741_SILICON_140
    BYTE reg1[5] = {0x0D, 0x10, 0x6B, 0x00, 0x00};//ADSP_Y_Vol_Main1P
    BYTE reg2[5] = {0x0D, 0x10, 0x6C, 0x00, 0x00};//ADSP_Y_Vol_Main2P
#endif
    UINT regAddr;

    lidbg("SAF7741 Volume:%d\n", Volume);
    /*
    	if (GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax > 0)
    	{
    		Volume = Volume * 60 / GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax;
    	}


    	if(EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput)
    	{
    		Volume = Volume + 6;
    		if (Volume > (sizeof(SAF7741_Volume_Data)/4)) Volume = (sizeof(SAF7741_Volume_Data)/4) - 1;
    	}
    */
    if (Volume > 60)
        Volume = 60;

    reg1[3] = SAF7741_Volume_Data[4 * Volume + 0];
    reg1[4] = SAF7741_Volume_Data[4 * Volume + 1];
    reg2[3] = SAF7741_Volume_Data[4 * Volume + 2];
    reg2[4] = SAF7741_Volume_Data[4 * Volume + 3];

    regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
    I2C_Write_SAF7741(regAddr, &reg1[3], 2);
    regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
    I2C_Write_SAF7741(regAddr, &reg2[3], 2);
}

EXPORT_SYMBOL(SAF7741_Volume);

/*
void SAF7741_ReadGraphicalSpectrumAnalyzer(void)
{
	BYTE Value[3];

	I2C_Read_SAF7741(ADSP_X_Gsa_Bar321, Value, 3);
}
*/

BOOL SendToSAF7741NormalWriteData(BYTE *pData)
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
        if(MChipAdd != SAF7741_ADDR_W)break;
        iLength = *pData++;
        if(pData[0] == 0x00 && pData[1] == 0xFF && pData[2] == 0xFF)
        {
            pData += iLength;
            continue;
        }
        else
        {
            regAddr = (pData[0] << 16) + (pData[1] << 8) + pData[2];
            if(!I2C_Write_SAF7741(regAddr, &pData[3], iLength - 3))
            {
                //lidbg("SAF7741 I2C comm error!\n");
                return FALSE;
            }
            pData += iLength;
        }
    }

    return TRUE;
}


BOOL SAF7741_Init(void)
{
    int i;
    //lidbg("%s:enter\n", __func__);
    lidbg("SAF7741 init start!\n");
    for(i = 0; i < 3; ++i)

    {
        if(SendToSAF7741NormalWriteData(SAF7741_Audio_Init_Data))
        {
            return TRUE;
        }
        else
        {
            lidbg("SAF7741 I2C err num:%d\n", i);
            msleep(1000);
            continue;
        }
    }

    return FALSE;
    //SendToSAF7741NormalWriteData(SAF7741_Audio_Init_Data_FM);
    //SendToSAF7741NormalWriteData(SAF7741_Radio_Init_Data);

}

void send_cmds_to_lpc(void)
{
    lidbg("SAF7741:send some cmds to LPC\n");

    LPC_CMD_4052A2_L;       //select MP3_L/MP3_R

    LPC_CMD_7388MUTE_H;     //forbid 7388 MUTE
    msleep(100);
    LPC_CMD_7388STANDBY_H;  //let 7388 work

    LPC_CMD_7741RST_L;      //7741 RESET
    msleep(1000);
    LPC_CMD_7741RST_H;
    msleep(1000);
}

static int saf7741_probe(struct platform_device *pdev)
{
    //int ret = 0;

    if((g_var.is_fly) || (g_var.recovery_mode))
    {
        lidbg("saf7741_probe do nothing\n");
        return 0;
    }

    //lidbg("%s:enter\n", __func__);
    /*
    	ret = saf7741_io_config(SAF7741_GPIO_RST, "SAF7741_reset");
    	if(ret)
    	{
    		//gpio_free(SAF7741_GPIO_RST);
    		return ret;
    	}

    	SAF7741_Reset();    //At LPC
    */

    send_cmds_to_lpc();

    if(SAF7741_Init())
        lidbg("SAF7741 init succeed!\n");
    else
        lidbg("SAF7741 init failed!\n");

    /*	SAF7741_Input(IPOD);  //IPOD  TV  AUX
    	SAF7741_Balance_P(10);
    	SAF7741_Fader_P(10);
    	SAF7741_Bass(1, 5);
    	SAF7741_Mid(1, 5);
    	SAF7741_Treble(1, 5);
    	SAF7741_Mute(FALSE);  // TRUE
    	SAF7741_Volume(10);
    */
    SAF7741_Volume(20);
    //SAF7741_ReadGraphicalSpectrumAnalyzer();
    return 0;

}

static int saf7741_remove(struct platform_device *pdev)
{
    //gpio_free(SAF7741_GPIO_RST);
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

static struct platform_device saf7741_devices =
{
    .name			= "saf7741",
    .id 			= 0,
};

static struct platform_driver saf7741_driver =
{
    .probe = saf7741_probe,
    .remove = saf7741_remove,
    .driver = {
        .name = "saf7741",
        .owner = THIS_MODULE,
    },
};

static int saf7741_module_init(void)
{
    //lidbg("%s:enter\n", __func__);
    LIDBG_GET;
    platform_device_register(&saf7741_devices);
    platform_driver_register(&saf7741_driver);
    //misc_register(&misc);
    return 0;

}

static void __exit saf7741_module_exit(void)
{
    platform_driver_unregister(&saf7741_driver);
    platform_device_unregister(&saf7741_devices);
}


module_init(saf7741_module_init);
module_exit(saf7741_module_exit);
MODULE_LICENSE("GPL");

