#include "i2c_gpio.h"
#include "soc.h"
#include "fly_private.h"
#include "fly_target.h"

static struct i2c_gpio_dev *lpc_i2c_devp = NULL;

int lpc_key_val;
struct fly_hardware_info
{

    unsigned char buffFromMCU[128];
    unsigned char buffFromMCUProcessorStatus;
    unsigned int buffFromMCUFrameLength;
    unsigned int buffFromMCUFrameLengthMax;
    unsigned char buffFromMCUCRC;
    unsigned char buffFromMCUBak[128];

};
struct fly_hardware_info *pGlobalHardwareInfo;
struct fly_hardware_info GlobalHardwareInfo;

unsigned char lpc_read(char *buf, unsigned int size)
{
    int ret;

    struct i2c_msg msg_buf[] =
    {
        {g_bootloader_hw.lpc_info.lpc_slave_add, I2C_M_RD, size, buf}
    };
    ret = bit_xfer(lpc_i2c_devp, msg_buf, 1);

    return ret;
}

unsigned char lpc_write( char *buf, unsigned int size)
{
    int ret;
    struct i2c_msg msg_buf[] = { {
            g_bootloader_hw.lpc_info.lpc_slave_add,
            I2C_M_WR, size, buf
        }
    };
#ifdef NEW_SUSPEND
	dprintf(INFO, "LK wakeup LPC, io(%d)\n", g_bootloader_hw.lk_wakeup_lpc_io);
	gpio_set_direction(g_bootloader_hw.lk_wakeup_lpc_io, GPIO_OUTPUT);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 0);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 1);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 0);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 1);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 0);
	//mdelay(500);
#endif
    ret  = bit_xfer(lpc_i2c_devp, msg_buf, 1);

    return ret;
}

static void lpc_i2c_config()
{
    int ret = 0;
    int i = 0;
    dprintf(INFO, "><><>< config lpc i2c bus ><><><\n");

    gpio_set_direction(g_bootloader_hw.lpc_info.lpc_sda, GPIO_OUTPUT);
    gpio_set_direction(g_bootloader_hw.lpc_info.lpc_scl, GPIO_OUTPUT);
    gpio_set_val(g_bootloader_hw.lpc_info.lpc_sda, 1);
    gpio_set_val(g_bootloader_hw.lpc_info.lpc_scl, 1);
    lpc_i2c_devp = malloc(sizeof(struct i2c_gpio_dev));
    if (!lpc_i2c_devp)
    {
        dprintf(INFO, "Malloc space for lpc_i2c_devp failed.\n");
        return;
    }
    lpc_i2c_devp = memset(lpc_i2c_devp, 0, sizeof(struct i2c_gpio_dev));

    lpc_i2c_devp->name = "lpc_i2c_gpio";
    lpc_i2c_devp->scl_pin = g_bootloader_hw.lpc_info.lpc_scl;
    lpc_i2c_devp->sda_pin = g_bootloader_hw.lpc_info.lpc_sda;
    lpc_i2c_devp->retries = 5;
    lpc_i2c_devp->udelay = 1;
}

void send_hw_info(char hw_info)
{
    u8 Check_Sum = 0;
    u8 hwInfo[7] = {0xff, 0x55, 0x4, 0x00, 0x25, hw_info};
    int i;

    lpc_i2c_config();
    dprintf(CRITICAL, "send hardware info {0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x}\n", hwInfo[0], hwInfo[1], hwInfo[2], hwInfo[3], hwInfo[4], hwInfo[5]);

    for(i = 2; i < 6; i++)
    {
        Check_Sum += hwInfo[i];
    }
    hwInfo[6] = Check_Sum;

    lpc_write(hwInfo, sizeof(hwInfo));
}

void backlight_enable()
{
    u8 Check_Sum = 0;
    u8 back_light[7] = {0xff, 0x55, 0x4, 0x02, 0x0d, 0x1};
    int i, j;

    dprintf(CRITICAL, "Open backlight !\n");

    for(i = 2; i < 6; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[6] = Check_Sum;

    for(j = 0; j < 3; j++)
    {
        lpc_write(back_light, sizeof(back_light));
        mdelay(10);
    }
}

void backlight_disable()
{
    u8 Check_Sum = 0;
    u8 back_light[7] = {0xff, 0x55, 0x4, 0x02, 0x0d, 0x0};
    int i, j;

    dprintf(CRITICAL, "TurnOff backlight !\n");

    for(i = 2; i < 6; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[6] = Check_Sum;

    for(j = 0; j < 3; j++)
    {
        lpc_write(back_light, sizeof(back_light));
        mdelay(10);
    }
}

void lpc_reboot()
{
    u8 Check_Sum = 0;
    u8 back_light[8] = {0xff, 0x55, 0x5, 0x00, 0x03, 0x01, 0x00};
    int i, j;

    dprintf(CRITICAL, "lpc_reboot !\n");

    for(i = 2; i < 7; i++)
    {
        Check_Sum += back_light[i];
    }

    back_light[7] = Check_Sum;

    for(j = 0; j < 3; j++)
    {
        lpc_write(back_light, sizeof(back_light));
        mdelay(10);
    }
}

static void LPCdealReadFromMCUAll(unsigned char *p, int length)
{
    int  i;
    u32 val[4] = {0};
    u32 val_temp, ad_val_left, ad_val_right;

    if(p[0] == 0x05 && p[1] == 0x05)
    {
        val[0] = p[2];
        val[1] = p[3];
        val[2] = p[4];
        val[3] = p[5];
        val_temp = (u32)val[1];
        val_temp = (val_temp << 8);
        ad_val_left = val_temp + (u32)val[0];
        val_temp = (u32)val[3];
        val_temp = (val_temp << 8);
        ad_val_right = val_temp + (u32)val[2];
        if(ad_val_left < g_bootloader_hw.adc_info[0].ad_vol || ad_val_right < g_bootloader_hw.adc_info[0].ad_vol)
        {
            lpc_key_val = 1;
        }
        else
        {
            lpc_key_val = 0;
        }

    }
}
static bool readFromMCUProcessor(char *p, int length)
{
    unsigned int i;

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
                }
                else
                {
                    dprintf(INFO, "\nRead From MCU CRC Error");
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
int lpc_adc_get(void)
{
    char buff[13] = {0};
    int val = 0, ret = 0;
    static  int countt = 0;
    pGlobalHardwareInfo = &GlobalHardwareInfo;
    if(lpc_read(buff, 12) < 0)
    {
        dprintf(INFO, "lpc_read erro%d\n");
        return 0;
    }
    readFromMCUProcessor(buff, 12);

    return lpc_key_val;


}
