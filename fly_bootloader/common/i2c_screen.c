
#include "soc.h"

/***********************************************
*  flyaudio  i2c   to constrol the screen
*
*********************************************/
#define UINT32       unsigned long
#define UINT16       unsigned short
#define UINT8         unsigned char
#define INT32          long
#define INT16          short
#define INT8            char

UINT8	 g_ReceiveBuf[26];
UINT8	PORT_STATE = 0;
//#define TRUE		1
//#define FALSE		0

#define SDA_GPIO   2
#define SCL_GPIO   3
#define	I2C_SCL_OUT		do{gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);}while(0)
#define	I2C_SCL_IN		do{gpio_set_direction(SCL_GPIO, GPIO_INPUT);}while(0)
#define	I2C_SDA_OUT		do{gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);}while(0)
#define	I2C_SDA_IN		do{gpio_set_direction(SDA_GPIO, GPIO_INPUT);}while(0)

#define	I2C_SCL_ONE		gpio_set_val(SCL_GPIO,1)
#define	I2C_SCL_ZERO	gpio_set_val(SCL_GPIO,0)
#define	I2C_SDA_ONE		gpio_set_val(SDA_GPIO,1)
#define	I2C_SDA_ZERO	gpio_set_val(SDA_GPIO,0)

#define	dd()				my_delay() //GPIO\u64cd\u4f5c\u5ef6\u65f6
//#define	dd()				mdelay(1) //GPIO\u64cd\u4f5c\u5ef6\u65f6

#define	IO_GPIO_READ_INPUT	gpio_get_val(SDA_GPIO)

bool WaitAck(void);


void my_delay()
{
    /*
    	volatile int i=0,j=0;
    	for(i=0;i<time;i++)
    		for(j=0;j<50;j++)
    			;
    */
    //volatile int i = 150;
    volatile int i = 50;
    while(i--);
}


void start_I2C()
{


    I2C_SDA_OUT;
    I2C_SCL_OUT;
    dd();
    I2C_SDA_ONE;
    I2C_SCL_ONE;
    dd();
    I2C_SDA_ZERO;
    dd();
    I2C_SCL_ZERO;
    dd();


}

void stop_I2C()
{

    I2C_SDA_ZERO;
    dd();
    I2C_SCL_ONE;
    dd();
    I2C_SDA_ONE;
    dd();
    I2C_SDA_IN;
    I2C_SCL_IN;
    //dd();


}


void I2CWriteData(UINT8 value)
{

    UINT8 i = 8;
    while(i--)
    {
        I2C_SCL_ZERO;
        dd();
        if(value & 0x80)
            I2C_SDA_ONE;
        else
            I2C_SDA_ZERO;
        value <<= 1;
        dd();
        I2C_SCL_ONE;
        dd();
    }
    I2C_SCL_ZERO;

}

UINT8 I2CReadData()
{


    UINT32 i;
    UINT8 value = 0;
    UINT8 data = 0;

    I2C_SDA_ONE;
    //dd();
    I2C_SCL_ZERO;
    //dd();
    I2C_SDA_IN;
    dd();

    for(i = 0; i < 8; i++)
    {
        I2C_SCL_ONE;
        dd();
        value <<= 1;
        data = IO_GPIO_READ_INPUT;
        //dprintf(INFO,"IO_GPIO_READ_INPUT= %d\n",IO_GPIO_READ_INPUT);
        if(data) value |= 1;
        I2C_SCL_ZERO;
        dd();
    }
    I2C_SDA_OUT;
    //RETAILMSG(1, (TEXT("--I2CReadData()  value:%d \r\n"),value));

    return value;
}

void   Write_I2C_Byte(UINT8 addr, UINT8 index, UINT8 n)    //n<=26
{
    UINT8 m;
    start_I2C();
    I2CWriteData(addr);
    I2CWriteData(index);
    start_I2C();
    I2CWriteData(addr | 0x01);
    for(m = 0; m < n - 1; m++)
    {
        g_ReceiveBuf[m] = SendData();
        Send_Ack();
    }
    g_ReceiveBuf[m] = I2CReadData();         //\u6700\u540e\u4e00\u4f4d\u4e0d\u53d1\u9001\u5e94\u7b54\u4fe1\u53f7
    stop_I2C();

    //RETAILMSG(1, (TEXT("--ReadI2C_NByte()\r\n")));
}

void  Send_Ack(void)
{
    //RETAILMSG(TOUCH_DBG, (TEXT("++Send_Ack()\r\n")));
    I2C_SDA_OUT;
    // dd();
    I2C_SDA_ZERO;
    I2C_SCL_ONE;
    dd();   // __delay_cycles(10);

    I2C_SCL_ZERO;
    I2C_SDA_ONE;
    // dd();
    I2C_SDA_IN;
    //RETAILMSG(TOUCH_DBG, (TEXT("--Send_Ack()\r\n")));

}

void  Stop_Ack(void)
{
    //RETAILMSG(TOUCH_DBG, (TEXT("++Send_Ack()\r\n")));
    I2C_SDA_OUT;
    // dd();
    I2C_SDA_ONE;
    I2C_SCL_ONE;
    dd();   // __delay_cycles(10);

    I2C_SCL_ZERO;
    I2C_SDA_ONE;
    // dd();
    I2C_SDA_IN;
    //RETAILMSG(TOUCH_DBG, (TEXT("--Send_Ack()\r\n")));

}



void SendData(unsigned char *ch, unsigned char len)
{

    int i = 0;
    for(i = 0; i < len; i++)
    {
        start_I2C();
        I2CWriteData(0xa0);
        dd();
        WaitAck();
        I2CWriteData(ch[i]);
        dd();
        WaitAck();
        /*	if(!)
        		{
        		stop_I2C();
        		debug_messages("I2C_WaitAck!(1)error");
        		}
        	Write_to_I2C(ch[i]);
        	if(!WaitAck())
        		{
        		stop_I2C();
        		debug_messages("I2C_WaitAck!(2)error");

        		}
        		*/
        stop_I2C();
    }
}



void SendDataLpcMeg(unsigned char *ch, unsigned char len)
{

    int i = 0;
    for(i = 0; i < len; i++)
    {
        start_I2C();
        I2CWriteData(0xa0);
        dd();
        WaitAck();
        I2CWriteData(ch[i]);
        dd();
        WaitAck();
        /*	if(!)
        		{
        		stop_I2C();
        		debug_messages("I2C_WaitAck!(1)error");
        		}
        	Write_to_I2C(ch[i]);
        	if(!WaitAck())
        		{
        		stop_I2C();
        		debug_messages("I2C_WaitAck!(2)error");

        		}
        		*/
        stop_I2C();
    }
}



unsigned char RecviceData()
{
    char I2CDATA;
    I2CDATA = I2CReadData( );
    //dprintf(INFO,"I2CDATA :0x%x\n",I2CDATA);

    return I2CDATA;
}


void LkRecviceData(unsigned char *ch, unsigned char len)
{

    int i = 0;
    unsigned char temp[1000] = {0};

    start_I2C();
    I2CWriteData(0xa1);
    dd();
    WaitAck();

    for(i = 0; i < len; i++)
    {
        //I2CWriteData(ch[i]);
        temp[i] = RecviceData();
        //dprintf(INFO,"temp[%d] ------------->>0x%x  \n",i,temp[i]);
        dd();
        //WaitAck();
        Send_Ack();
        dd();
    }
    Stop_Ack();
    stop_I2C();
    ch = temp;
}

UINT8 Receive_Buffer[256];
//**************************************
//return 1: bootloader 2:updata 3:system
//**************************************
char Get_STR_Status(void)
{
    UINT8 *PReceive_Buffer = (UINT8 *) Receive_Buffer;
    UINT8 send_order[8] = {0xff, 0x55, 0x4, 0x37, 0x00, 0x00};
    UINT8 Check_Sum = 0;
    //UINT32 Count = 0;
    char ret_data = 0;
    int i;
    for(i = 2; i < 6; i++)
        Check_Sum += send_order[i];
    send_order[6] = Check_Sum;

Sendagain:
    i = 0;
    SendDataLpcMeg(send_order, 7);
    debug_messages("this is Sendagain");
Tryagain:
    debug_messages("this is Tryagain");
    while(!RecviceData())
    {

        //Delay_ms(1000);
        debug_messages("RecviceData() == 0");
        mdelay(1000);
        SendData(send_order, 7);
    }
    //\u534f\u8bae\u5904\u7406
    if(*(PReceive_Buffer + 3) != 0x37)
    {
        i++;
        if(i > 10)
        {
            debug_messages("PReceive_Buffer+ 3 this is i>10");
            goto Sendagain;

        }
        else
        {
            debug_messages("PReceive_Buffer+ 3 this is i<10");
            goto Tryagain;

        }
    }
    if(*(PReceive_Buffer + 4) != 0)
    {
        i++;
        if(i > 10)
        {
            debug_messages("PReceive_Buffer+ 4 this is i>10");
            goto Sendagain;
        }
        else
        {
            debug_messages("PReceive_Buffer+ 4 this is i<10");
            goto Tryagain;
        }

    }

    switch(*(PReceive_Buffer + 5))
    {
    case 1:
        debug_messages("this is case 1");
        break;
    case 2:
        debug_messages("this is case 2");
        break;
    case 3:
        debug_messages("this is case 3");
        break;
    default :
        debug_messages("this is case default");
        ret_data = 0;
        i++;
        if(i > 10)
            i++;
        if(i > 10)
        {
            debug_messages("PReceive_Buffer+ 5 this is i>10");
            goto Sendagain;
        }
        else
        {
            debug_messages("PReceive_Buffer+ 5 this is i<10");
            goto Tryagain;
        }
        break;
    }
    debug_messages("this is case return ret_data");
    return ret_data ;

}

bool WaitAck(void)   //\u8fd4\u56de\u4e3a:=1\u6709ACK,=0\u65e0ACK
{
    I2C_SCL_ZERO;
    dd();
    I2C_SDA_ONE;
    dd();
    I2C_SCL_ONE;
    dd();
    if(IO_GPIO_READ_INPUT)
    {
        I2C_SCL_ZERO;
        return FALSE;
    }
    I2C_SCL_ZERO;
    return TRUE;
}

UINT8 Get_Char_From_STR()
{
    UINT8  ch;
    PORT_STATE = 0;	//\u521d\u59cb\u5316\u72b6\u6001\u503c
    ch = RecviceData();
    //dd();
    //Send_Ack();
    //dprintf(INFO,"FUNC:Get_Char_From_STR ch = 0x%x\n",ch);
    return ch;
}
char Get_Str_From_STR(void)
{

    UINT8 data, str_len = 0, pos = 0 ;
    UINT8 *PRead_Buffer = (UINT8 *)Receive_Buffer;
    UINT8 Check_sum = 0;
    UINT8 overflow = 0 ;
    int loopnum = 0;

    while(loopnum < 200)
    {
        data = Get_Char_From_STR();

        PRead_Buffer[pos] = data;
        //RETAILMSG(1, (TEXT(" pos %x  %x "),pos,PRead_Buffer[pos]));
        pos++;
        loopnum++;

        if(PORT_STATE == 0xff)
            return 0;

        if(pos == 1)//\u7b2c\u4e00\u4f4d\u4e3a
        {
            if(PRead_Buffer[0] != 0xFF)
            {
                pos = 0;
                continue;
            }
        }
        else if(pos == 2)//\u7b2c\u4e8c\u4f4d\u4e3a0x55
        {
            if(PRead_Buffer[1] != 0x55)
            {
                pos = 0;
                continue;
            }
        }
        else if(pos == 3)//\u957f\u5ea6
        {
            str_len = PRead_Buffer[2];
            Check_sum += str_len;
            //RETAILMSG(1, (TEXT(" str_len%x CHECKSUM %x \n"),str_len,Check_sum));
        }
        else if(pos >= 3 + str_len) //\u6570\u636e\u8bfb\u53d6\u5b8c\u6210
        {

            //RETAILMSG(1, (TEXT(" DATA %x CHECKSUM %x \n"),data,Check_sum));
            if(Check_sum == data)
            {
                str_len = 0;
                pos = 0;
                return 1;
            }
            else
            {
                return 0;
            }

        }
        else
        {
            Check_sum += PRead_Buffer[pos - 1];
        }

    }
    return 0;
}

unsigned char Get_Ver_From_STR(unsigned char *pbuf)
{

    UINT8 data, str_len = 0, pos = 0 ;
    //UINT8 *PRead_Buffer = (UINT8*)Receive_Buffer;
    UINT8 Check_sum = 0;
    UINT8 overflow = 0 ;
    int i;

    unsigned char PRead_Buffer[1000] = {0};

    start_I2C();
    I2CWriteData(0xa1);
    dd();
    WaitAck();


    while(i < 500)
    {
        data = Get_Char_From_STR();
        // dprintf(INFO,"data[%d]----------->>0x%x  \n",pos,data);
        dd();
        Send_Ack();
        dd();
        PRead_Buffer[pos] = data;
        dprintf(INFO, "PRead_Buffer[%d]----------->>0x%x  \n", pos, PRead_Buffer[pos]);
        pos++;


        if(PORT_STATE == 0xff)
            return 0;

        if(pos == 1)//\u7b2c\u4e00\u4f4d\u4e3a
        {
            if(PRead_Buffer[0] != 0xFF)
            {
                pos = 0;
                continue;
            }
        }
        else if(pos == 2)//\u7b2c\u4e8c\u4f4d\u4e3a0x55
        {
            if(PRead_Buffer[1] != 0x55)
            {
                pos = 0;
                i++;
                continue;
            }
        }
        else if(pos == 3)//\u957f\u5ea6
        {
            str_len = PRead_Buffer[2];
            Check_sum += str_len;
        }
        else if(pos == 4)//\u957f\u5ea6
        {
            if(PRead_Buffer[3] != 0x37)
            {
                pos = 0;
                //continue;
                PRead_Buffer[0] = 0xcc;
                break;
            }
        }
        else if(pos >= 3 + str_len) //\u6570\u636e\u8bfb\u53d6\u5b8c\u6210
        {
            if(Check_sum == data)
            {
                str_len = 0;
                pos = 0;
                break;
            }
            else
            {
                break;
            }

        }
        else
        {
            Check_sum += PRead_Buffer[pos - 1];
        }
    }

    Stop_Ack();
    stop_I2C();
    for(i = 0; i < (PRead_Buffer[2] + 3); i++)
    {
        pbuf[i] = PRead_Buffer[i];
        dprintf(INFO, "pbuf[%d] = 0x%x\n", i, pbuf[i]);
    }
    dprintf(INFO, "========end the loop=======\n");
    return 0;
}


void displayclose( )
{
    UINT8 Check_Sum = 0;
    UINT8 back_light[9] = {0xff, 0x55, 0x6, 0x02, 0x0d, 0x00, 0x13, 0x88}; //delayed  5s
    int i;
    gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);
    gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);

    for(i = 2; i < 9; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[8] = Check_Sum;
    SendData(back_light, 9);
}

/*********************************************************/
void resetSystemSeccess(void)
{
    UINT8 Check_Sum = 0;
    UINT8 buf[7] = {0xff, 0x55, 0x4, 0x00, 0x06, 0x01, 0x00};
    int i;
    gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);
    gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);


    for(i = 2; i < 6; i++)
    {
        Check_Sum += buf[i];
    }
    buf[6] = Check_Sum;
    SendData(buf, 7);
}
/*********************************************************/
void send_hw_info(char hw_info)
{
    UINT8 Check_Sum = 0;
    UINT8 hwInfo[7] = {0xff, 0x55, 0x4, 0x00, 0x25, hw_info};
    int i;

    dprintf(CRITICAL, "send hardware info {0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x}\n", hwInfo[0], hwInfo[1], hwInfo[2], hwInfo[3], hwInfo[4], hwInfo[5]);
    gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);
    gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);


    for(i = 2; i < 6; i++)
    {
        Check_Sum += hwInfo[i];
    }
    hwInfo[6] = Check_Sum;
    SendData(hwInfo, 7);
}

void backlight_enable()
{
    UINT8 Check_Sum = 0;
    UINT8 back_light[7] = {0xff, 0x55, 0x4, 0x02, 0x0d, 0x1};
    int i, j;

    dprintf(CRITICAL, "Open backlight !\n");
    gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);
    gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);


    for(i = 2; i < 6; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[6] = Check_Sum;
    for(j = 0; j < 3; j++)
    {
        SendData(back_light, 7);
        mdelay(10);
    }
}


void SendReadLpcVersionsMeg()
{
    UINT8 Check_Sum = 0;
    UINT8 back_light[7] = {0xff, 0x55, 0x4, 0x37, 0x00, 0x00};
    int i;
    gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);
    gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);


    for(i = 2; i < 6; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[6] = Check_Sum;
    SendData(back_light, 7);
}


UINT8 *CheckLpcVersions(/*(UINT8 *pVer8*/)
{
    int i = 0;
    //gpio_tlmm_config(GPIO_CFG(  SDA_GPIO, 0, 1, 0, 0), 0);
    //gpio_tlmm_config(GPIO_CFG(SCL_GPIO, 0, 1, 0, 0), 0);
    UINT8 temp[50] = {0};
    temp[0] = 0xcc;

    while((temp[0] == 0xcc) && ( i < 100) )
    {
        SendReadLpcVersionsMeg();
        //Get_Str_From_STR();
        Get_Ver_From_STR(temp);
        i++;
        mdelay(1);
    }

    return temp;

}



void close_lpc_watchdog( )
{
    UINT8 Check_Sum = 0;
    UINT8 back_light[7] = {0xff, 0x55, 0x4, 0x00, 0x97, 0x01};
    int i;
    gpio_set_direction(SDA_GPIO, GPIO_OUTPUT);
    gpio_set_direction(SCL_GPIO, GPIO_OUTPUT);


    for(i = 2; i < 6; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[6] = Check_Sum;
    SendData(back_light, 7);
}



