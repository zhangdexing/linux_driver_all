/*
 * fm1388.c  --  FM1388 driver
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#include "i2c-fm1388.h"
#include "lidbg.h"
LIDBG_DEFINE;

#define TAG "dfm1388_i2c:"

#define FM1388_I2C_ADDR 0x2c
#define FM1388_RESET_PIN 64

#define FM1388_IOC_MAGIC 'k'
#define FM1388_IOC_MAXNR  7
#define FM1388_CM8K_MODE               _IOW(FM1388_IOC_MAGIC,0,int)
#define FM1388_CM16K_MODE              _IOW(FM1388_IOC_MAGIC,1,int)
#define FM1388_VR_MODE                 _IOW(FM1388_IOC_MAGIC,2,int)
#define FM1388_SIRI_MODE               _IOW(FM1388_IOC_MAGIC,3,int)
#define FM1388_FACETIME_MODE           _IOW(FM1388_IOC_MAGIC,4,int)
#define FM1388_MEDIAPLAY48K_MODE       _IOW(FM1388_IOC_MAGIC,5,int)
#define FM1388_MEDIAPLAY44K1_MODE      _IOW(FM1388_IOC_MAGIC,8,int)
#define FM1388_BLUETOOTH_MODE          _IOW(FM1388_IOC_MAGIC,9,int)
#define FM1388_BARGEIN_MODE            _IOW(FM1388_IOC_MAGIC,10,int)
#define FM1388_GET_MODE                _IOR(FM1388_IOC_MAGIC,6,int)
#define FM1388_GET_STATUS              _IOR(FM1388_IOC_MAGIC,7,int)

#define BT_MODE "bt"
#define BARGEIN_MODE "dueros"
#define BYPASS_MODE "bypass"

#define HARDWARE_RESET
#define SOFTWARD_RESET

enum FM1388_BOOT_STATE
{
    FM1388_COLD_BOOT,
    FM1388_HOT_BOOT
};
int fm1388_boot_status;
int fm1388_config_status = false;
//
// Following address may be changed by different firmware release
//
#if defined(FRAME_CNT)
#undef FRAME_CNT
#define FRAME_CNT 0x5ffdffcc
#endif
#if defined(CRC_STATUS)
#undef CRC_STATUS
#define CRC_STATUS 0x5ffdffe8
#endif
#define DSP_PARAMETER_READY 0x5ffdffea

char filepath[] = "/flysystem/lib/out/fm1388/";

char filepath_name[255];

#define VERSION "0.0.1"

#define FM1388_SPI
//#define FM1388_SPI_ENABLE

//#define MCLK 24576000
//#define LRCK 48000

#define CTRL_GPIOS

static struct platform_device *fm1388_pdev;
static bool fm1388_is_dsp_on = false;	// is dsp power on? dsp is off in init time
//static unsigned int fm1388_dsp_mode = FM_SMVD_CFG_BARGE_IN;
static int fm1388_dsp_mode = -1;	// DSP working mode, user define mode in .cfg file
static int current_mode = 0;	// DSP working mode, user define mode in .cfg file
static struct mutex fm1388_index_lock, fm1388_dsp_lock, fm1388_mode_change_lock;
static struct mutex fm1388_init_lock;
static struct delayed_work dsp_start_vr;

//#define FM1388_IRQ
//static struct work_struct fm1388_irq_work;
//static struct workqueue_struct *fm1388_irq_wq;
u32 fm1388_irq;
static int is_host_slept = 0;
static bool isNotInspectFramecnt = false;
static bool isLock = false;
static short resumeAfterReinitCount = 0;

//
// show DSP frame counter and CRC, for debugging
//
#define SHOW_FRAMECNT
static struct work_struct fm1388_framecnt_work;
static struct workqueue_struct *fm1388_framecnt_wq;

#define SHOW_DL_TIME
#ifdef SHOW_DL_TIME
struct timex  txc;
struct rtc_time tm;
#endif

#define OPEN_NO_ERROR 0
#define OPEN_ERROR -1

struct fm1388_reg_list
{
    u8 layer;
    u8 reg;
    u16 val;
};

// Wayne 9/21/2015 for DSP parameters
struct fm1388_dsp_addr_list
{
    u32 addr;
    u16 val;
};

typedef struct dev_cmd_t
{
    unsigned int reg_addr;
    unsigned int val;
} dev_cmd;

typedef struct cfg_mode_cmd_t
{
    unsigned int mode;
    char path_setting_file_name[50];
    char dsp_setting_file_name[50];
    char comment[100];
} cfg_mode_cmd;

int load_fm1388_mode_cfg(char *file_src, unsigned int choosed_mode);
int load_fm1388_vec(char *file_src);
char *combine_path_name(char *s, char *append);

static bool isFrameCountRise(void);

#ifdef CTRL_GPIOS
struct pinctrl *pinctrliis0=NULL;
struct pinctrl_state *pins_default=NULL, *pins_sleep=NULL;
#endif

static int fm1388_i2c_write(unsigned int reg, unsigned int value)
{
    u8 data[3];
    int ret;

    data[0] = (reg & 0x000000ff);
    data[1] = (value & 0x0000ff00) >> 8;
    data[2] = (value & 0x000000ff);
    ret = SOC_I2C_Send(FM1388_I2C_BUS, FM1388_I2C_ADDR, data, 3);
    return ret;
}

static int fm1388_i2c_read(unsigned int r, unsigned int *v)
{
    u8 data[2];
    int ret;
    char sub_addr;
    sub_addr = (r & 0x000000ff);
    ret = SOC_I2C_Rec(FM1388_I2C_BUS, FM1388_I2C_ADDR, sub_addr, data, 2);
    /* Read data */
    *v = (data[0] << 8) | data[1];
    return ret;
}

static int fm1388_dsp_mode_i2c_write_addr(unsigned int addr, unsigned int value, unsigned int opcode)
{
    int ret;

    if (!fm1388_is_dsp_on)
        return EIO;

    mutex_lock(&fm1388_dsp_lock);

    //lidbg(TAG"%s: addr = %08x, value = %04x, opcode = %d\n", __func__, addr, value, opcode);
    ret = fm1388_i2c_write(FM1388_DSP_I2C_ADDR_LSB, addr & 0xffff);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set addr lsb value: %d\n", ret);
        goto err;
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_ADDR_MSB, addr >> 16);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set addr msb value: %d\n", ret);
        goto err;
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_DATA_LSB, value & 0xffff);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set data lsb value: %d\n", ret);
        goto err;
    }

    //low and high to the same for 16-bit write by Henry for debug
    if (opcode == FM1388_I2C_CMD_16_WRITE)
    {
        ret = fm1388_i2c_write(FM1388_DSP_I2C_DATA_MSB, value & 0xffff);
        if (ret < 0)
        {
            lidbg(TAG"Failed to set data msb value: %d\n", ret);
            goto err;
        }
    }
    else
    {
        ret = fm1388_i2c_write(FM1388_DSP_I2C_DATA_MSB, value >> 16);
        if (ret < 0)
        {
            lidbg(TAG"Failed to set data msb value: %d\n", ret);
            goto err;
        }
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_OP_CODE, opcode);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set op code value: %d\n", ret);
        goto err;
    }
    //lidbg(TAG"%s: write over \n", __func__);
err:
    mutex_unlock(&fm1388_dsp_lock);

    return ret;
}
#if 0
// address read, 4 bytes
static int fm1388_dsp_mode_i2c_read_addr(unsigned int addr, unsigned int *value)
{
    int ret;
    unsigned int msb, lsb;

    if (!fm1388_is_dsp_on)
        return EIO;

    mutex_lock(&fm1388_dsp_lock);

    ret = fm1388_i2c_write(FM1388_DSP_I2C_ADDR_MSB, addr >> 16);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set addr msb value: %d\n", ret);
        goto err;
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_ADDR_LSB, addr & 0xffff);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set addr lsb value: %d\n", ret);
        goto err;
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_OP_CODE, FM1388_I2C_CMD_32_READ);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set op code value: %d\n", ret);
        goto err;
    }

    fm1388_i2c_read(FM1388_DSP_I2C_DATA_MSB, &msb);
    fm1388_i2c_read(FM1388_DSP_I2C_DATA_LSB, &lsb);
    *value = (msb << 16) | lsb;
    lidbg(TAG"%s: addr = %04x, value = %04x\n", __func__, addr, *value);

err:
    mutex_unlock(&fm1388_dsp_lock);

    return ret;
}
#endif
// address read, 2 bytes
static int fm1388_dsp_mode_i2c_read_addr_2(unsigned int addr, unsigned int *value)
{
    int ret;
    //	unsigned int msb, lsb;

    if (!fm1388_is_dsp_on)
        return EIO;

    mutex_lock(&fm1388_dsp_lock);

    ret = fm1388_i2c_write(FM1388_DSP_I2C_ADDR_MSB, addr >> 16);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set addr msb value: %d\n", ret);
        goto err;
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_ADDR_LSB, addr & 0xffff);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set addr lsb value: %d\n", ret);
        goto err;
    }

    ret = fm1388_i2c_write(FM1388_DSP_I2C_OP_CODE, FM1388_I2C_CMD_32_READ);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set op code value: %d\n", ret);
        goto err;
    }

    if ((addr & 0x3) == 0)
    {
        fm1388_i2c_read(FM1388_DSP_I2C_DATA_LSB, value);
    }
    else
    {
        fm1388_i2c_read(FM1388_DSP_I2C_DATA_MSB, value);
    }
    //lidbg(TAG"%s: addr = %x, value = %x, lsb = %x\n", __func__, addr, *value);

err:
    mutex_unlock(&fm1388_dsp_lock);

    return ret;
}
#if 0
static int fm1388_dsp_mode_update_bits_addr(unsigned int addr, unsigned int mask, unsigned int value)
{
    bool change;
    unsigned int old, new;
    int ret;

    ret = fm1388_dsp_mode_i2c_read_addr(addr, &old);
    if (ret < 0)
        return ret;

    new = (old & ~mask) | (value & mask);
    change = old != new;
    if (change)
        ret = fm1388_dsp_mode_i2c_write_addr(addr, new, FM1388_I2C_CMD_32_WRITE);

    if (ret < 0)
        return ret;

    //lidbg(TAG"%s: addr = %04x, value = %04x, mask = %d\n", __func__, addr, value, mask);
    return change;
}
#endif
// register write
static int fm1388_dsp_mode_i2c_write(unsigned int reg, unsigned int value)
{
    //lidbg(TAG"%s: reg = %04x, value = %04x\n", __func__, reg, value);
    return fm1388_dsp_mode_i2c_write_addr(0x18020000 + reg * 2, value, FM1388_I2C_CMD_16_WRITE);
}

// register read
static int fm1388_dsp_mode_i2c_read(unsigned int reg, unsigned int *value)
{
    int ret = fm1388_dsp_mode_i2c_read_addr_2(0x18020000 + reg * 2, value);

    //lidbg(TAG"%s: reg = %04x, value = %04x\n", __func__, reg, *value);

    return ret;
}

// register write
static int fm1388_write(unsigned int reg,
                        unsigned int value)
{
    //lidbg(TAG"%s %02x = %04x\n", __FUNCTION__, reg, value);

    return fm1388_is_dsp_on ? fm1388_dsp_mode_i2c_write(reg, value) :
           fm1388_i2c_write(reg, value);
}

// register read
static int fm1388_read(unsigned int reg,
                       unsigned int *value)
{
    int ret;

    ret = fm1388_is_dsp_on ? fm1388_dsp_mode_i2c_read(reg, value) :
          fm1388_i2c_read(reg, value);

    //lidbg(TAG"%s %02x = %04x\n", __FUNCTION__, reg, *value);

    return ret;
}
#if 0
static int fm1388_update_bits(unsigned int reg, unsigned int mask, unsigned int value)
{
    bool change;
    unsigned int old, new;
    int ret;

    ret = fm1388_read(reg, &old);
    if (ret < 0)
        return ret;

    new = (old & ~mask) | (value & mask);
    change = old != new;

    //lidbg(TAG"%s: reg = %04x, mask = %04x, value = %04x, old = %04x, new =%04x, change = %d\n", __func__, reg, mask, value, old, new, change);

    if (change)
        ret = fm1388_write(reg, new);

    if (ret < 0)
        return ret;

    return change;
}
#endif
static int fm1388_index_write(unsigned int reg, unsigned int value)
{
    int ret;

    mutex_lock(&fm1388_index_lock);

    lidbg(TAG"%s: reg = %04x, value = %04x\n", __func__, reg, value);
    ret = fm1388_write(0x6a, reg);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set private addr: %d\n", ret);
        goto err;
    }
    ret = fm1388_write(0x6c, value);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set private value: %d\n", ret);
        goto err;
    }

    mutex_unlock(&fm1388_index_lock);

    return 0;

err:
    mutex_unlock(&fm1388_index_lock);

    return ret;
}

static unsigned int fm1388_index_read(unsigned int reg)
{
    int ret;

    mutex_lock(&fm1388_index_lock);

    ret = fm1388_write(0x6a, reg);
    if (ret < 0)
    {
        lidbg(TAG"Failed to set private addr: %d\n", ret);
        mutex_unlock(&fm1388_index_lock);
        return ret;
    }

    fm1388_read(0x6c, &ret);

    mutex_unlock(&fm1388_index_lock);
    lidbg(TAG"%s: reg = %04x, value = %04x\n", __func__, reg, ret);

    return ret;
}
#if 0
static void fm1388_set_dsp_on(bool on)
{
    if (on)
    {
        fm1388_update_bits(0x65, 0x2, 0x2);
        fm1388_is_dsp_on = true;
    }
    else
    {
        fm1388_update_bits(0x65, 0x2, 0x0);
        fm1388_is_dsp_on = false;
    }
}

static int fm1388_run_list(struct fm1388_reg_list *list, size_t list_size)
{
    return 0;
}
#endif
#if 0
static int fm1388_run_dsp_addr_list(struct  fm1388_dsp_addr_list *list, size_t list_size)
{
    int i;

    //lidbg(TAG"%s\n", __func__);
    for (i = 0; i < list_size; i++)
    {
        fm1388_dsp_mode_i2c_write_addr((unsigned int)list[i].addr, list[i].val, FM1388_I2C_CMD_16_WRITE);
    }
    return 0;
}
#endif
static void fm1388_software_reset(void)
{
#ifdef SOFTWARD_RESET
    fm1388_write(0x00, 0x10ec);
    msleep(100);
#endif
}

static void fm1388_hardware_reset(void)
{

#ifdef HARDWARE_RESET
    SOC_IO_Output(0, FM1388_RESET_PIN, 0);
    msleep(100);
    SOC_IO_Output(0, FM1388_RESET_PIN, 1);
    msleep(100);
#endif
}

#if 0
static void fm1388_set_default_mode(unsigned int mode)
{
    fm1388_dsp_mode = mode;

    lidbg(TAG"%s: default mode = %d\n", __func__, fm1388_dsp_mode);
    if(load_fm1388_mode_cfg(combine_path_name(filepath_name, "FM1388_mode.cfg"), fm1388_dsp_mode) == OPEN_ERROR)
    {
        lidbg(TAG"%s file open error!\n", combine_path_name(filepath_name, "FM1388_mode.cfg"));
    }
}
#endif

static void fm1388_dsp_mode_change(unsigned int mode)
{
    unsigned int addr, val, times = 10;

    fm1388_dsp_mode = mode;
    
    //	lidbg(TAG"%s: fm1388_dsp_mode = %d\n", __func__, fm1388_dsp_mode);
    if(load_fm1388_vec(combine_path_name(filepath_name, "FM1388_sleep.vec")) == OPEN_NO_ERROR)
    {
#if 1
        // wait for DSP default setting ready
        while(1)
        {
            addr = DSP_PARAMETER_READY;
            fm1388_dsp_mode_i2c_read_addr_2(addr, &val);

            lidbg(TAG"*** reg_addr=0x%08x, val=0x%04x\n", DSP_PARAMETER_READY, val);
            if((val & 0x100) == 0x100) 	// ready: bit 8 = 1
            {
                break;
            }
            else
            {
                times--;
                if(times == 0)
                {
                    lidbg(TAG"timeout: wait for DSP default setting ready\n");
                    break;
                }
            }
            msleep(10);
        }
#endif

        if(load_fm1388_mode_cfg(combine_path_name(filepath_name, "FM1388_mode.cfg"), fm1388_dsp_mode) == OPEN_NO_ERROR)
        {
            load_fm1388_vec(combine_path_name(filepath_name, "FM1388_wakeup.vec"));
	    current_mode = mode;
        }
        else
        {
            lidbg(TAG"%s file open error!\n", combine_path_name(filepath_name, "FM1388_mode.cfg"));
        }
    }
}

//Added by Henry
/************************************************************************
addr:	4 bytes address, DSP long address
data16:	the target value with 16bit
************************************************************************/
bool DSP_Write16(u32 addr, u16 data16)
{
    u16 addr_msb, addr_lsb;

    addr_msb = (u16)(addr >> 16);
    addr_lsb = (u16)addr;

    //lidbg(TAG"%s: addr = %04x, data = %02x\n", __func__, addr, data16);
    fm1388_i2c_write( 0x01, addr_lsb);
    fm1388_i2c_write( 0x02, addr_msb);
    fm1388_i2c_write( 0x03, data16);
    fm1388_i2c_write( 0x04, data16);
    fm1388_i2c_write( 0x00, 0x0001);

    return true;
}

/************************************************************************
addr:	4 bytes address, DSP long address
data32:	the target value with 32bit
************************************************************************/
bool DSP_Write32(u32 addr, u32 data32)
{
    u16 addr_msb, addr_lsb;

    //lidbg(TAG"%s: addr = %04x, data32 = %04x\n", __func__, addr, data32);
    addr_msb = (u16)(addr >> 16);
    addr_lsb = (u16)addr;

    fm1388_i2c_write( 0x01, addr_lsb);
    fm1388_i2c_write( 0x02, addr_msb);
    fm1388_i2c_write( 0x03, (u16)data32);
    fm1388_i2c_write( 0x04, (u16)(data32 >> 16));
    fm1388_i2c_write( 0x00, 0x0003);

    return true;
}

/************************************************************************
pfEEProm[4]:	pointers of the files
fEEPromLen[4]:	length of the files
************************************************************************/
void FM1388_Burst_Write(u8 *pfEEProm[4], u32 fEEPromLen[4])
{
    u32 i, j, utemp32;
    u32 len, addr_lng;
    u16 utemp16;
    u32 addr_dsp[4] = {0x50000000, 0x5ffc0000, 0x5ffe0000, 0x60000000};
    u32 *pEEPromBuf;


    for (i = 0; i < 4; i++)
    {
#ifdef SHOW_DL_TIME
        do_gettimeofday(&(txc.time));
        rtc_time_to_tm(txc.time.tv_sec, &tm);
        lidbg(TAG"%s: (%d) start time: %d-%d-%d %d:%d:%d \n", __func__, i, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
        // Skip Empty File
        pEEPromBuf = (u32 *)pfEEProm[i];	// pfEEProm[i] is pointer of the i_th file
        if (pEEPromBuf == NULL)
        {
            break;
        }

        // Download integer of 4bytes first
        len = fEEPromLen[i] / 4;				// fEEPromLen[i] is length of the i_th file
        for (j = 0; j < len; j++)
        {
            addr_lng = addr_dsp[i] + j * 4;
            DSP_Write32(addr_lng, *(pEEPromBuf + j));
        }

        // Download the other sub of 4bytes
        utemp16 = fEEPromLen[i] - (len * 4);
        if (utemp16 > 0)
        {
            utemp32 = 0xFFFFFFFF;
            utemp32 = utemp32 >> ((4 - utemp16) * 8);
            utemp32 = *(pEEPromBuf + j) & utemp32;
            addr_lng = addr_dsp[i] + j * 4;
            DSP_Write32(addr_lng, utemp32);
        }
#ifdef SHOW_DL_TIME
        do_gettimeofday(&(txc.time));
        rtc_time_to_tm(txc.time.tv_sec, &tm);
        lidbg(TAG"%s: (%d) end   time: %d-%d-%d %d:%d:%d \n", __func__, i, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
    }


    return;
}
//End

static void fm1388_dsp_load_fw(void)
{

    const struct firmware *fw = NULL;
    //	const struct firmware *fw1 = NULL;
    //	const struct firmware *fw2 = NULL;
    //	const struct firmware *fw3 = NULL;

#ifndef FM1388_SPI
    u8 *pfEEProm[4];
    u32 pfEEPromLen[4];

    //fm1388_set_dsp_on(fm1388_i2c, true);

    lidbg(TAG"%s: with I2C\n", __func__);
    request_firmware(&fw, "FM1388_50000000.dat", &fm1388_pdev->dev);
    request_firmware(&fw1, "FM1388_5FFC0000.dat", &fm1388_pdev->dev);
    request_firmware(&fw2, "FM1388_5FFE0000.dat", &fm1388_pdev->dev);
    request_firmware(&fw3, "FM1388_60000000.dat", &fm1388_pdev->dev);
    pfEEProm[0] = fw->data;
    pfEEPromLen[0] = fw->size;
    pfEEProm[1] = fw1->data;
    pfEEPromLen[1] = fw1->size;
    pfEEProm[2] = fw2->data;
    pfEEPromLen[2] = fw2->size;
    pfEEProm[3] = fw3->data;
    pfEEPromLen[3] = fw3->size;
    FM1388_Burst_Write(pfEEProm, pfEEPromLen);
    release_firmware(fw);
    fw = NULL;
    release_firmware(fw1);
    fw1 = NULL;
    release_firmware(fw2);
    fw2 = NULL;
    release_firmware(fw3);
    fw3 = NULL;
#else
    //lidbg(TAG"%s: with SPI\n", __func__);
#if 0
#ifdef SHOW_DL_TIME
    do_gettimeofday(&(txc.time));
    rtc_time_to_tm(txc.time.tv_sec, &tm);
    lidbg(TAG"%s: start time: %d-%d-%d %d:%d:%d \n", __func__, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
#endif
    request_firmware(&fw, "FM1388_50000000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_50000000.dat.\n", __func__);
        fm1388_spi_burst_write(0x50000000, fw->data,
                               ((fw->size / 8) + 1) * 8);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_50000000.dat over.\n", __func__);
    request_firmware(&fw, "FM1388_5FFC0000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_5FFC0000.dat.\n", __func__);

        fm1388_spi_burst_write(0x5ffc0000, fw->data,
                               ((fw->size / 8) + 1) * 8);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_5FFC0000.dat over.\n", __func__);
    request_firmware(&fw, "FM1388_5FFE0000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_5FFE0000.dat.\n", __func__);

        fm1388_spi_burst_write(0x5ffe0000, fw->data,
                               ((fw->size / 8) + 1) * 8);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_5FFE0000.dat over.\n", __func__);
    request_firmware(&fw, "FM1388_60000000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_60000000.dat.\n", __func__);

        fm1388_spi_burst_write(0x60000000, fw->data,
                               ((fw->size / 8) + 1) * 8);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_60000000.dat over.\n", __func__);
#if 0
#ifdef SHOW_DL_TIME
    do_gettimeofday(&(txc.time));
    rtc_time_to_tm(txc.time.tv_sec, &tm);
    lidbg(TAG"%s:end time: %d-%d-%d %d:%d:%d \n", __func__, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
#endif
#endif

}

mm_segment_t oldfs;
struct file *openFile(char *path, int flag, int mode)
{
    struct file *fp;

    fp = filp_open(path, flag, 0);

    if (IS_ERR(fp))
        return NULL;
    else
        return fp;
}

int readFile(struct file *fp, char *buf, int readlen)
{
    if (fp->f_op && fp->f_op->read)
        return fp->f_op->read(fp, buf, readlen, &fp->f_pos);
    else
        return -1;
}

int closeFile(struct file *fp)
{
    filp_close(fp, NULL);
    return 0;
}

void initKernelEnv(void)
{
    oldfs = get_fs();
    set_fs(KERNEL_DS);
}

int isspace(int x)
{
    if(x == ' ' || x == '\t' || x == '\n' || x == '\f' || x == '\b' || x == '\r')
        return 1;
    else
        return 0;
}

void del_space(char *src)
{
    char *cp1, *cp2, *cp3;

    // skip leading spaces
    cp1 = src;
    while(*cp1)
    {
        if (!isspace(*cp1)) break;
        cp1++;
    }

    // skip tailing spaces
    cp2 = cp1;
    while(*cp2)  	// point to string end
    {
        cp2++;
    }
    while (cp2 > cp1)
    {
        cp2--;
        if (!isspace(*cp2)) break;
    }

    // copy string
    cp3 = src;
    while(cp1 <= cp2)
    {
        *cp3++ = *cp1++;
    }
    *cp3 = '\0';
}

// parse 4 fields: "mode, path_vec, dsp_vec, mode_description"; delimiter is comma
int  parser_mode(char *src_argv, cfg_mode_cmd *data)
{
    char *pch;
    char delim[] = ",";	// comma
    int argc = 0;

    while((pch = strsep(&src_argv, delim)) != NULL)
    {
        //lidbg(TAG"token=<%s>\n", pch);
        if (*pch == 0) continue;
        if(argc == 0)
        {
            del_space(pch);
            data->mode = simple_strtoul(pch, NULL, 16);
        }
        else if(argc == 1)
        {
            del_space(pch);
            strcpy(data->path_setting_file_name, pch);
        }
        else if(argc == 2)
        {
            del_space(pch);
            strcpy(data->dsp_setting_file_name, pch);
        }
        else if(argc == 3)
        {
            del_space(pch);
            strcpy(data->comment, pch);
        }
        argc++;
    }

    return argc;
}

// parse 2 fields: "address data"; delimiter is space and tab
int  parser_reg_mem(char *src_argv, dev_cmd *data)
{
    char *pch;
    char delim[] = " \t";	// space and tab
    int argc = 0;

    while ((pch = strsep(&src_argv, delim)) != NULL)
    {
        //lidbg(TAG"token=<%s>\n", pch);
        if (*pch == 0) continue;
        if (argc == 0)
        {
            data->reg_addr = simple_strtoul(pch, NULL, 16);
        }
        else if (argc == 1)
        {
            data->val = simple_strtoul(pch, NULL, 16);
        }
        else
        {
            break;
        }
        argc++;
    }

    return argc;
}


#define EOF -1

int fgetc(struct file *fp)
{
    int cnt;
    unsigned char c;

    cnt = readFile(fp, &c, 1);

    if (cnt <= 0)
    {
        //      lidbg(TAG"0x%x, %d: EOF? ",c,c);
        //      lidbg(TAG"Read file fail");
        return EOF;
    }
    else
    {
        //      printk(KERN_ERR"%c:",c);
        return c;
    }
}

char *fgets(char *dst, int max, struct file  *fp, int *word_count)
{
    int c = 0;
    char *p;

    /* get max bytes or upto a newline */
    *word_count = 0;
    for (p = dst, max--; max > 0; max--)
    {
        if ((c = fgetc (fp)) == EOF)
            break;
        *p++ = c;
        (*word_count)++;
        if (c == '\n')
            break;
    }
    // add end char '\0' in the string
    *p = 0;
#if 1
    //	if (p == dst || c == EOF)
    if (p == dst)
        return NULL;
#else
    if (p == dst)
        return NULL;
    if (p != dst && c == EOF)
        return EOF;
#endif
    //	return (p);
    return (dst);
}

char *combine_path_name(char *s, char *append)
{

    char *save = s;
    strcpy(save, filepath);
    for (; *s; ++s);
    while ((*s++ = *append++));
    return(save);
}


// load codec init VEC, access HW register thru short address
int load_fm1388_init_vec(char *file_src)
{
    struct file *fp;
    int word_count = 0;
    char s[255];	//assume each line of the opened file is below 255 characters
    dev_cmd payload;

    //	lidbg(TAG"%s: file %s\n", __func__, file_src);

    initKernelEnv();
    fp = openFile(file_src, O_RDONLY, 0);

    if (fp == NULL)
    {
        lidbg(TAG"File %s could not be opened\n", file_src);
        set_fs(oldfs);
        return OPEN_ERROR;
    }
    else
    {
        //       lidbg(TAG"File %s opened!...\n", file_src);"
        while (fgets(s, 255, fp, &word_count) != NULL)
        {
            if(s[0] == '#' || s[0] == '/' || s[0] == 0xD || s[0] == 0x0)
            {
                continue;
            }
            else
            {
                //parse addr, value,
                if (parser_reg_mem(s, &payload) >= 2)
                {
                    //				lidbg(TAG"payload.reg_addr=0x%08x, payload.val=0x%08x\n", (unsigned int)payload.reg_addr, (unsigned int)payload.val);
                    //write to device
                    fm1388_write((unsigned int)payload.reg_addr, (unsigned int)payload.val);
                    msleep(2);
                }
            }
        }

        /* Close stream; skip error-checking for brevity of example */
        closeFile(fp);
        set_fs(oldfs);

        return OPEN_NO_ERROR;
    }
}

// load VEC file, access HW register thru long address
int load_fm1388_vec(char *file_src)
{
    struct file *fp;
    int word_count = 0;
    char s[255];	//assume each line of the opened file is below 255 characters
    dev_cmd payload;

    //	lidbg(TAG"%s: file %s\n", __func__, file_src);

    initKernelEnv();
    fp = openFile(file_src, O_RDONLY, 0);

    if (fp == NULL)
    {
        lidbg(TAG"File %s could not be opened\n", file_src);
        set_fs(oldfs);
        return OPEN_ERROR;
    }
    else
    {
        lidbg(TAG"File %s opened!...\n", file_src);
        while (fgets(s, 255, fp, &word_count) != NULL)
        {
            if(s[0] == '#' || s[0] == '/' || s[0] == 0xD || s[0] == 0x0)
            {
                continue;
            }
            else
            {
                //parse addr, value,
                if (parser_reg_mem(s, &payload) >= 2)
                {
                    //lidbg(TAG"payload.reg_addr=0x%08x, payload.val=0x%08x\n", (unsigned int)payload.reg_addr, (unsigned int)payload.val);
                    //write to device
                    fm1388_dsp_mode_i2c_write_addr((unsigned int)payload.reg_addr, payload.val, FM1388_I2C_CMD_16_WRITE);
                }
            }
        }

        /* Close stream; skip error-checking for brevity of example */
        closeFile(fp);
        set_fs(oldfs);

        return OPEN_NO_ERROR;
    }
}

// load mode configuration file
int load_fm1388_mode_cfg(char *file_src, unsigned int choosed_mode)
{
    struct file *fp;
    int word_count = 0;
    cfg_mode_cmd cfg_mode;
    char s[255];	//assume each line of the opened file is below 255 characters

    //	lidbg(TAG"%s: file %s\n", __func__, file_src);

    initKernelEnv();
    fp = openFile(file_src, O_RDONLY, 0);

    if (fp == NULL)
    {
        lidbg(TAG"File %s could not be opened\n", file_src);
        set_fs(oldfs);
        return OPEN_ERROR;
    }
    else
    {
        //       lidbg(TAG"File %s opened!...\n", file_src);
        while (fgets(s, 255, fp, &word_count) != NULL)
        {
            if(s[0] == '#' || s[0] == '/' || s[0] == 0xD || s[0] == 0x0)
            {
                continue;
            }
            else
            {
                //parse mode, path vec, dsp vec, comment
                if (parser_mode(s, &cfg_mode) >= 3)
                {
                    if(choosed_mode == cfg_mode.mode)
                    {
                        //lidbg(TAG"mode=%d, path=%s, dsp_setting=%s, comment=%s\n", cfg_mode.mode, cfg_mode.path_setting_file_name, cfg_mode.dsp_setting_file_name, cfg_mode.comment);
                        break;
                    }
                }
            }
        }

        /* Close stream; skip error-checking for brevity of example */
        closeFile(fp);
        set_fs(oldfs);

        if(choosed_mode == cfg_mode.mode)
        {
            lidbg(TAG"Set to Mode %d: %s\n", choosed_mode, cfg_mode.comment);
            load_fm1388_vec(combine_path_name(filepath_name, cfg_mode.path_setting_file_name));	// load path VEC
            load_fm1388_vec(combine_path_name(filepath_name, cfg_mode.dsp_setting_file_name));	// load DSP parameter VEC
        }
        else
        {
            lidbg(TAG"Cannot find Mode %d in cfg file\n", choosed_mode);
            return OPEN_ERROR;
        }
        return OPEN_NO_ERROR;
    }
}

static int fm1388_fw_loaded(void *data)
{
    unsigned int val;

    if(isLock)
        	return -1;

    mutex_lock(&fm1388_init_lock);
    isLock = true;
	
	while(1)
	{
	    fm1388_hardware_reset();
	    fm1388_software_reset();
	    fm1388_dsp_mode = -1;
	    fm1388_is_dsp_on = false;
	    fm1388_config_status = false;
#ifdef SHOW_DL_TIME
	    do_gettimeofday(&(txc.time));
	    rtc_time_to_tm(txc.time.tv_sec, &tm);
	    lidbg(TAG"%s#########:: %d-%d-%d %d:%d:%d \n", __func__, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif

	    if(fm1388_boot_status == FM1388_HOT_BOOT)
	    {
	        fm1388_spi_device_reload();
	    }
	    load_fm1388_init_vec(combine_path_name(filepath_name, "FM1388_init.vec"));
	    fm1388_is_dsp_on = true;	// set falg due to the last command of init VEC file will power on DSP

	    msleep(10);	// wait HWfm1388_spi_device_reload ready to load firmware
	    fm1388_dsp_load_fw();
	    msleep(10);

	    // TODO:
	    //   example to set default mode to mode 0
	    //   user may change preferred default mode here
	    load_fm1388_vec(combine_path_name(filepath_name, "FM1388_run.vec"));
	    msleep(10);
	    fm1388_dsp_mode_change(current_mode);	// set default mode, parse from .cfg
#ifdef SHOW_DL_TIME
	    do_gettimeofday(&(txc.time));
	    rtc_time_to_tm(txc.time.tv_sec, &tm);
	    lidbg(TAG"%s#########:: %d-%d-%d %d:%d:%d \n", __func__, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
	    fm1388_dsp_mode_i2c_read_addr_2(0x180200CA, &val);	// check register 0x65 to make sure DSP is running
	    lidbg(TAG"addr=0x180200CA, val=0x%x (value must be 0x7fe)\n", val);
	    fm1388_config_status = true;
	    spi_test();
		resumeAfterReinitCount++;

		if(isFrameCountRise())
			break;
	}
	
    mutex_unlock(&fm1388_init_lock);
    isLock = false;
    return 0;
}


static bool fm1388_readable_register(unsigned int reg)
{
    //lidbg(TAG"%s\n", __func__);
    switch (reg)
    {
    case 0x00 ... 0x04:
    case 0x07 ... 0x09:
    case 0x13:
    case 0x15 ... 0x2d:
    case 0x2f ... 0x39:
    case 0x3b ... 0x44:
    case 0x47 ... 0x4e:
    case 0x50 ... 0x51:
    case 0x56 ... 0x5f:
    case 0x61 ... 0x68:
    case 0x6a:
    case 0x6c:
    case 0X6f ... 0x75:
    case 0x7a ... 0x7d:
    case 0x80 ... 0x81:
    case 0x83 ... 0x9a:
    case 0x9c ... 0xa0:
    case 0xa3 ... 0xa9:
    case 0xae ... 0xb3:
    case 0xb5 ... 0xb6:
    case 0xb8:
    case 0xbd ... 0xc2:
    case 0xc5 ... 0xce:
    case 0xd0:
    case 0xd2 ... 0xe1:
    case 0xe3 ... 0xf6:
    case 0xfa ... 0xff:
        return true;
    default:
        return false;
    }
}

static ssize_t fm1388_reg_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    int count = 0;
    unsigned int i, value;
    int ret;

    lidbg(TAG"%s: fm1388_reg_show\n", __func__);
    for (i = 0x0; i <= 0xff; i++)
    {
        if (fm1388_readable_register(i))
        {
            ret = fm1388_read(i, &value);
            if (ret < 0)
                count += sprintf(buf + count, "%02x: XXXX\n",
                                 i);
            else
                count += sprintf(buf + count, "%02x: %04x\n", i,
                                 value);

            if (count >= PAGE_SIZE - 1)
                break;
        }
    }

    return count;
}

static ssize_t fm1388_reg_store(struct device *dev,
                                struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int val = 0, addr = 0;
    int i;

    lidbg(TAG"%s: fm1388_reg_store\n", __func__);
    for (i = 0; i < count; i++)
    {
        if (*(buf + i) <= '9' && *(buf + i) >= '0')
            addr = (addr << 4) | (*(buf + i) - '0');
        else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
            addr = (addr << 4) | ((*(buf + i) - 'a') + 0xa);
        else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
            addr = (addr << 4) | ((*(buf + i) - 'A') + 0xa);
        else
            break;
    }

    for (i = i + 1 ; i < count; i++)
    {
        if (*(buf + i) <= '9' && *(buf + i) >= '0')
            val = (val << 4) | (*(buf + i) - '0');
        else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
            val = (val << 4) | ((*(buf + i) - 'a') + 0xa);
        else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
            val = (val << 4) | ((*(buf + i) - 'A') + 0xa);
        else
            break;
    }

    if (addr > 0xff)
        return count;

    if (i == count)
    {
        pr_info("%s: 0x%02x = 0x%04x\n", __func__, addr, val);
        fm1388_read(addr, &val);
    }
    else
    {
        fm1388_write(addr, val);
    }
    return count;
}
static DEVICE_ATTR(fm1388_reg, 0555, fm1388_reg_show, fm1388_reg_store);

static ssize_t fm1388_index_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
    unsigned int val;
    int cnt = 0, i;

    lidbg(TAG"%s: fm1388_index_show\n", __func__);
    for (i = 0; i < 0xff; i++)
    {
        if (cnt + 10 >= PAGE_SIZE)
            break;
        val = fm1388_index_read(i);
        if (!val)
            continue;
        cnt += snprintf(buf + cnt, 10,
                        "%02x: %04x\n", i, val);
    }

    if (cnt >= PAGE_SIZE)
        cnt = PAGE_SIZE - 1;

    return cnt;
}

static ssize_t fm1388_index_store(struct device *dev,
                                  struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int val = 0, addr = 0;
    int i;

    lidbg(TAG"%s: fm1388_index_store\n", __func__);
    for (i = 0; i < count; i++)
    {
        if (*(buf + i) <= '9' && *(buf + i) >= '0')
            addr = (addr << 4) | (*(buf + i) - '0');
        else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
            addr = (addr << 4) | ((*(buf + i) - 'a') + 0xa);
        else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
            addr = (addr << 4) | ((*(buf + i) - 'A') + 0xa);
        else
            break;
    }

    for (i = i + 1; i < count; i++)
    {
        if (*(buf + i) <= '9' && *(buf + i) >= '0')
            val = (val << 4) | (*(buf + i) - '0');
        else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
            val = (val << 4) | ((*(buf + i) - 'a') + 0xa);
        else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
            val = (val << 4) | ((*(buf + i) - 'A') + 0xa);
        else
            break;

    }

    if (addr > 0xff || val > 0xffff || val < 0)
        return count;

    if (i == count)
        pr_info("0x%02x = 0x%04x\n", addr,
                fm1388_index_read(addr));
    else
        fm1388_index_write(addr, val);

    return count;
}
static DEVICE_ATTR(index_reg, 0555, fm1388_index_show, fm1388_index_store);

static ssize_t fm1388_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 1;
}

static ssize_t fm1388_addr_store(struct device *dev,
                                 struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int val = 0, addr = 0;
    int i;

    lidbg(TAG"%s: fm1388_addr_store\n", __func__);
    for (i = 0; i < count; i++)
    {
        if (*(buf + i) <= '9' && *(buf + i) >= '0')
            addr = (addr << 4) | (*(buf + i) - '0');
        else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
            addr = (addr << 4) | ((*(buf + i) - 'a') + 0xa);
        else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
            addr = (addr << 4) | ((*(buf + i) - 'A') + 0xa);
        else
            break;
    }

    for (i = i + 1 ; i < count; i++)
    {
        if (*(buf + i) <= '9' && *(buf + i) >= '0')
            val = (val << 4) | (*(buf + i) - '0');
        else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
            val = (val << 4) | ((*(buf + i) - 'a') + 0xa);
        else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
            val = (val << 4) | ((*(buf + i) - 'A') + 0xa);
        else
            break;
    }


    if (fm1388_is_dsp_on)
    {
        if (i == count)
        {
            /*
            			if ((addr & 0xffff0000) == 0x18020000) {
            				fm1388_spi_read(addr, &val, 2);
            				pr_info("0x%08x = 0x%04x\n", addr, val);
            			} else {
            				fm1388_spi_read(addr, &val, 4);
            				pr_info("0x%08x = 0x%08x\n", addr, val);
            			}
            */
            fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
        }
        else
        {
            /*
            			if ((addr & 0xffff0000) == 0x18020000)
            				fm1388_spi_write(addr, val, 2);
            			else
            				fm1388_spi_write(addr, val, 4);
            */
            fm1388_dsp_mode_i2c_write_addr(addr, val, FM1388_I2C_CMD_16_WRITE);
        }
    }

    return count;
}
static DEVICE_ATTR(fm1388_addr, 0555, fm1388_addr_show, fm1388_addr_store);

static ssize_t fm1388_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned int addr, val;
    addr = FRAME_CNT;
    fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
    lidbg(TAG"%s: FRAME COUNTER 0x%x = 0x%x\n", __func__, addr, val);
    addr = CRC_STATUS;
    fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
    if (val == 0x8888)
    {
        lidbg(TAG"%s: CRC_STAUS 0x%x = 0x%x, CRC OK!\n", __func__, addr, val);
    }
    else
    {
        lidbg(TAG"%s: CRC_STAUS 0x%x = 0x%x, CRC FAIL!\n", __func__, addr, val);
    }
    return sprintf(buf, "%x\n", val);
}

static ssize_t fm1388_reinit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    CREATE_KTHREAD(fm1388_fw_loaded, NULL);
    return sprintf(buf, "fm1388_reinit\n");
}

static ssize_t fm1388_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned int addr, val;
    unsigned int sendconfig_count = 0;
    unsigned int successful_count = 0;
    unsigned int i;
    for(i = 0; i < 1; i++)
    {
        fm1388_fw_loaded(NULL);
        sendconfig_count++;
        addr = FRAME_CNT;
        fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
        lidbg(TAG"%s: FRAME COUNTER 0x%x = 0x%x\n", __func__, addr, val);
        addr = CRC_STATUS;
        fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
        if (val == 0x8888)
        {
            lidbg(TAG"%s: CRC_STAUS 0x%x = 0x%x, CRC OK!\n", __func__, addr, val);
            successful_count++;
        }
        else
        {
            lidbg(TAG"%s: CRC_STAUS 0x%x = 0x%x, CRC FAIL!\n", __func__, addr, val);
        }
        lidbg("sendconfig_count %d successful_count %d\n", sendconfig_count, successful_count);
    }
    return sprintf(buf, "sendconfig_count %d successful_count %d\n", sendconfig_count, successful_count);
}

static DEVICE_ATTR(fm1388_status, 0555, fm1388_status_show, NULL);
static DEVICE_ATTR(fm1388_reinit, 0555, fm1388_reinit_show, NULL);
static DEVICE_ATTR(fm1388_test,   0555, fm1388_test_show, NULL);

static ssize_t fm1388_device_read(struct file *file, char __user *buffer,
                                  size_t length, loff_t *offset)
{
    char str[5];
    size_t ret;
    char *local_buffer;
    dev_cmd_mode_gs *get_mode_ret_data;
    dev_cmd_reg_rw  *get_reg_ret_data;
    dev_cmd_long    *get_addr_ret_data;

    /*
    	if (*offset > 0)
    		return 0;
    */

    local_buffer = (char *)kmalloc(length * sizeof(char), GFP_KERNEL);
    if (!local_buffer)
    {
        lidbg(TAG"%s: local_buffer allocation failure.\n", __func__);
        goto out;
    }
    if(copy_from_user(local_buffer, buffer, length))
    {
        printk("copy_from_user ERR\n");
    }
    //lidbg(TAG"local_buffer = %d:, length = %d\n", local_buffer[0], length);

    switch(local_buffer[0])
    {
    case FM_SMVD_REG_READ:
        get_reg_ret_data = (dev_cmd_reg_rw *)local_buffer;
        fm1388_read(get_reg_ret_data->reg_addr, &get_reg_ret_data->reg_val);
        ret = sizeof(dev_cmd_reg_rw);
        //lidbg(TAG"get_reg_ret_data->reg_addr = %d:, get_reg_ret_data->reg_val = 0x%4x, ret=%d\n", get_reg_ret_data->reg_addr, get_reg_ret_data->reg_val, ret);
        break;
    case FM_SMVD_DSP_ADDR_READ:
        get_addr_ret_data = (dev_cmd_long *)local_buffer;
        fm1388_dsp_mode_i2c_read_addr_2(get_addr_ret_data->addr, &get_addr_ret_data->val);
        ret = sizeof(dev_cmd_long);
        //lidbg(TAG"get_addr_ret_data->addr = %d:, get_addr_ret_data->val = 0x%4x, ret=%d\n", get_addr_ret_data->addr, get_addr_ret_data->val, ret);
        break;
    case FM_SMVD_MODE_GET:
        //        lidbg(TAG"local_buffer = %d:, length = %d, ret = %d\n", local_buffer[0], length, ret);
        get_mode_ret_data = (dev_cmd_mode_gs *)local_buffer;
        get_mode_ret_data->dsp_mode = (char)fm1388_dsp_mode;
        ret = sizeof(dev_cmd_mode_gs);
        //        lidbg(TAG"local_buffer = %d:, length = %d, ret = %d\n", local_buffer[0], length, ret);
        //        lidbg(TAG"local_buffer = %d:, length = %d, return_data->dsp_mode = %d\n", local_buffer[0], length, return_data->dsp_mode);
        break;
    default:
        ret = sprintf(str, "0");
        break;
    }

    //	ret = sprintf(str, "0");

    if (copy_to_user(buffer, local_buffer, ret))
    {
        kfree(local_buffer);
        return -EFAULT;
    }

    //	*offset += ret;
out:
    if (local_buffer) kfree(local_buffer);
    return 0;
}

static ssize_t fm1388_device_mode_write(struct file *file,
                                        const char __user *buffer, size_t length, loff_t *offset)
{
    char *mode = NULL;

    lidbg(TAG"%s: entering...\n", __func__);

    if(length <= 0)
	    return 0;

    if(is_host_slept == 1)
	    return -1;
	

    mode = (char *)kmalloc(length, GFP_KERNEL);
    if (!mode)
    {
        lidbg(TAG"%s: mode allocation failure.\n", __func__);
        goto switch_mode_out;
    }

    memset(mode, 0, length);
    if(copy_from_user(mode, buffer, length))
    {
        printk("copy_from_user ERR\n");
        goto switch_mode_out;
    }

    mutex_lock(&fm1388_mode_change_lock);

    if(!strncmp(mode, BT_MODE, strlen(BT_MODE)))
    {
        if(fm1388_dsp_mode != 1)
	{
		isLock = true;
		fm1388_dsp_mode_change(1);
		isLock = false;
	}
		
        lidbg(TAG"%s: switch mode to bluetooth.\n", __func__);
    }
    else if(!strncmp(mode, BARGEIN_MODE, strlen(BARGEIN_MODE)))
    {
        if(fm1388_dsp_mode != 0)
	{
		isLock = true;
		fm1388_dsp_mode_change(0);
		isLock = false;
	}
        lidbg(TAG"%s: switch mode to bargein.\n", __func__);
    }
	else if(!strncmp(mode, BYPASS_MODE, strlen(BYPASS_MODE)))
	{
		fm1388_dsp_mode_change(2);
        lidbg(TAG"%s: switch mode to bypass.\n", __func__);
	}
	else if(!strncmp(mode, "notinspectframecnt", strlen("notinspectframecnt")))
	{
		isNotInspectFramecnt = true;
        lidbg(TAG"%s: isNotInspectFramecnt = true.\n", __func__);
	}
	else if(!strncmp(mode, "inspectframecnt", strlen("inspectframecnt")))
	{
		isNotInspectFramecnt = false;
        lidbg(TAG"%s: isNotInspectFramecnt = false.\n", __func__);
	}
    else
        lidbg(TAG"%s: no this mode:%s.\n", __func__, mode);

    mutex_unlock(&fm1388_mode_change_lock);

switch_mode_out:
    if (mode)
        kfree(mode);
    return length;
}

static ssize_t  fm1388_status_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    char status_buf[6];

	while(1)
	{
		if(isLock && (resumeAfterReinitCount<3))
		{
			msleep(100);
			continue;
		}
		
	    if(!isFrameCountRise())
	    {
			lidbg(TAG"%s: CRC ERROR!\n", __func__);
			strcpy(status_buf, "error");
			
			if(resumeAfterReinitCount >= 3)
				break;
	    }
	    else
	    {
	        //lidbg(TAG"%s: CRC OK!\n", __func__);
	        strcpy(status_buf, "ok");
			break;
	    }
		resumeAfterReinitCount = 0;
	}

    if(copy_to_user(buffer, status_buf, strlen(status_buf)))
    {
        lidbg(TAG"%s:copy_to_user ERR\n", __func__);
        return -1;
    }
	
	lidbg(TAG"%s: CRC status %s!\n", __func__, status_buf);

    return strlen(status_buf);
	
}

static ssize_t fm1388_device_write(struct file *file,
                                   const char __user *buffer, size_t length, loff_t *offset)
{
    dev_cmd_long *local_dev_cmd;
    unsigned int cmd_name, cmd_addr, cmd_val;
    int dsp_mode;

    lidbg(TAG"%s: entering...\n", __func__);
    local_dev_cmd = (dev_cmd_long *)kmalloc(sizeof(dev_cmd_long), GFP_KERNEL);
    if (!local_dev_cmd)
    {
        lidbg(TAG"%s: local_dev_cmd allocation failure.\n", __func__);
        goto out;
    }
    if(copy_from_user(local_dev_cmd, buffer, length))
    {
        printk("copy_from_user ERR\n");
    }

    lidbg(TAG"local_dev_cmd->cmd_name = %d, length = %d\n", local_dev_cmd->cmd_name, (int)length);
    cmd_name = local_dev_cmd->cmd_name;

    switch(cmd_name)
    {
        //The short commands
    case FM_SMVD_REG_READ:		//Command #0
        break;
    case FM_SMVD_REG_WRITE:		//Command #1
        cmd_addr = local_dev_cmd->addr;
        cmd_val = local_dev_cmd->val;
        //lidbg(TAG"cmd_addr = 0x%02x, cmd_val = 0x%04x\n", cmd_addr, cmd_val);
        fm1388_dsp_mode_i2c_write(cmd_addr, cmd_val);
        break;
    case FM_SMVD_DSP_ADDR_READ:	//Command #2
        break;
    case FM_SMVD_DSP_ADDR_WRITE:	//Command #3
        cmd_addr = local_dev_cmd->addr;
        cmd_val = local_dev_cmd->val;
        //lidbg(TAG"cmd_addr = 0x%08x:, cmd_val = 0x%04x\n", cmd_addr, cmd_val);
        fm1388_dsp_mode_i2c_write_addr(cmd_addr, cmd_val, FM1388_I2C_CMD_16_WRITE);
        break;
    case FM_SMVD_MODE_SET:		//Command #4
        lidbg(TAG"%s: FM_SMVD_MODE_SET dsp_mode = %d\n", __func__, local_dev_cmd->addr);
        dsp_mode = local_dev_cmd->addr;
        fm1388_dsp_mode_change(dsp_mode);
        break;
    case FM_SMVD_MODE_GET:		//Command #5
        //The long commands
    case FM_SMVD_DSP_BWRITE:		//Command #6
        break;
    case FM_SMVD_VECTOR_GET:		//Command #7
        break;
    case FM_SMVD_REG_DUMP:			//Command #8
        break;
    default:
        break;
    }
out:
    if (local_dev_cmd) kfree(local_dev_cmd);
    return length;
}

static void dsp_start_vr_work(struct work_struct *work)
{
    lidbg(TAG"%s: entering.\n", __func__);
    //Todo: something for VR mode
}

#ifdef FM1388_IRQ
static void fm1388_irq_handling_work(struct work_struct *work)
{
    int reg_val;

    lidbg(TAG"%s: going to clear the interrupt bit.\n", __func__);
    //Todo: clear the interrupt bit.

    //Todo: notify the application about the interrupt.
    //fm1388_host_irqstatus = 1;
}

static u32 fm1388_irq_handler(void *para)
{
    unsigned long status;

    lidbg(TAG"%s: irq handler entering...\n", __func__);

    if (is_host_slept == 0)
    {
        lidbg(TAG"%s: going to execute the irq_handling_work.\n", __func__);
        queue_work(fm1388_irq_wq, &fm1388_irq_work);
    }
    else if (is_host_slept == 1)
    {
        lidbg(TAG"%s: set fm1388 to the VR mode.\n", __func__);
        //Todo: start scheduled work for the VR mode.
        //schedule_delayed_work(&dsp_start_bypass, msecs_to_jiffies(50));

        is_host_slept = 0;
    }

    return IRQ_HANDLED;
}
#endif

// for debugging
#ifdef SHOW_FRAMECNT
static void fm1388_framecnt_handling_work(struct work_struct *work)
{
    unsigned int addr, val;
    addr = CRC_STATUS;

    while (1)
    {
        msleep(10000);

        if((g_var.acc_flag == FLY_ACC_OFF)||(isNotInspectFramecnt) ||(is_host_slept == 1) || (isLock))
            continue;


        fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
        if ((val != 0x8888) || (!isFrameCountRise()))
        {
            lidbgerr(TAG"%s: CRC_STAUS 0x%x = 0x%x, 1388err CRC FAIL!!!!!!!!!!!!!!!!!!!!!!!!!!!!\\n", __func__, addr, val);
            fm1388_fw_loaded(NULL);
        }
    }
}
#endif

static long fm1388_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    if(_IOC_TYPE(cmd) != FM1388_IOC_MAGIC)
        return -ENOTTY;
    if(_IOC_NR(cmd) > FM1388_IOC_MAXNR)
        return -ENOTTY;
    mutex_lock(&fm1388_mode_change_lock);
    switch(cmd)
    {
    case FM1388_CM8K_MODE:
        fm1388_dsp_mode_change(0);
        break;
    case FM1388_CM16K_MODE:
        fm1388_dsp_mode_change(1);
        break;
    case FM1388_VR_MODE:
        fm1388_dsp_mode_change(2);
        break;
    case FM1388_SIRI_MODE:
        fm1388_dsp_mode_change(3);
        break;
    case FM1388_FACETIME_MODE:
        fm1388_dsp_mode_change(4);
        break;
    case FM1388_MEDIAPLAY48K_MODE:
        fm1388_dsp_mode_change(5);
        break;
    case FM1388_MEDIAPLAY44K1_MODE:
        fm1388_dsp_mode_change(6);
        break;
    case FM1388_BLUETOOTH_MODE:
        fm1388_dsp_mode_change(7);
        break;
    case FM1388_BARGEIN_MODE:
        fm1388_dsp_mode_change(8);
        break;
    case FM1388_GET_MODE:
        //__put_user(fm1388_dsp_mode,(int __user *)arg);
        if(copy_to_user((int __user *)arg, &fm1388_dsp_mode, 4))
        {
            lidbg(TAG"copy_to_user ERR\n");
        }
        lidbg("get mode %d\n", fm1388_dsp_mode);
        break;
    case FM1388_GET_STATUS:
        if(copy_to_user((int __user *)arg, &fm1388_config_status, 4))
        {
            lidbg(TAG"copy_to_user ERR\n");
        }
        lidbg("fm1388 config status %d\n", fm1388_config_status);
        break;
    default:
        lidbg("unknow mode\n");
        break;
    }
    mutex_unlock(&fm1388_mode_change_lock);
    return 0;
}

struct file_operations fm1388_fops =
{
    .owner = THIS_MODULE,
    .read = fm1388_device_read,
    .write = fm1388_device_write,
    .unlocked_ioctl = fm1388_device_ioctl,
};

struct file_operations fm1388_mode_fops =
{
    .owner = THIS_MODULE,
    .write = fm1388_device_mode_write,
    .read = fm1388_status_read,
};

static struct miscdevice fm1388_dev =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "fm1388",
    .fops = &fm1388_fops
};

static bool isFrameCountRise(void)
{
	    unsigned int addr, countVal=0, oldCountVal=0;		
		addr = FRAME_CNT;

	    fm1388_dsp_mode_i2c_read_addr_2(addr, &oldCountVal);
	    msleep(20);
	    fm1388_dsp_mode_i2c_read_addr_2(addr, &countVal);

		if(countVal != oldCountVal)
			return true;
		else
			return false;
}




#ifdef SUSPEND_ONLINE
static int lidbg_fm1388_event(struct notifier_block *this,
                              unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_UP)://NOTIFIER_MINOR_ACC_ON
    lidbg("lidbg_fm1388_event.FLY_DEVICE_UP\n");
#ifdef CTRL_GPIOS
        if(pinctrliis0 && pins_default)
            pinctrl_select_state(pinctrliis0,pins_default);
#endif
        if(is_host_slept == 1)
        {
            CREATE_KTHREAD(fm1388_fw_loaded,NULL);
            is_host_slept = 0;
            resumeAfterReinitCount = 0;
        }		
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
        break;
		
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
#ifdef CTRL_GPIOS
		if(pinctrliis0 && pins_default)
			pinctrl_select_state(pinctrliis0,pins_sleep);
#endif
        break;	
    default:
        break;
    }

    return NOTIFY_DONE;
}

static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_fm1388_event,
};
#endif



#ifdef CONFIG_PM
static int fm1388_i2c_suspend(struct device *dev)
{
    //Todo: something before driver's suspend.
    is_host_slept = 1;
    lidbg(TAG"%s: is_host_slept.%d\n", __func__, is_host_slept);

    return 0;
}

static int fm1388_i2c_resume(struct device *dev)
{
    //lidbg(TAG"%s: entering\n", __func__);
    //Todo: something after driver's resume

#ifdef SUSPEND_ONLINE
    lidbg(TAG"%s: return 0;\n", __func__);
    return 0;
#endif

    is_host_slept = 0;
    fm1388_boot_status = FM1388_HOT_BOOT;
    CREATE_KTHREAD(fm1388_fw_loaded, NULL);
    return 0;
}
#endif

static const struct dev_pm_ops fm1388_i2c_ops =
{
    .suspend = fm1388_i2c_suspend,
    .resume  = fm1388_i2c_resume,
};

static int fm1388_probe(struct platform_device *pdev)
{
    int ret;
    DUMP_FUN;
    LIDBG_GET;
    lidbg(TAG"%s: FM1388 Driver Version %s\n", __func__, VERSION);
    fm1388_pdev = pdev;
    mutex_init(&fm1388_index_lock);
    mutex_init(&fm1388_dsp_lock);
    mutex_init(&fm1388_mode_change_lock);
    mutex_init(&fm1388_init_lock);
    lidbg(TAG"%s: device_create_file - dev_attr_fm1388_reg.\n", __func__);
    ret = device_create_file(&pdev->dev, &dev_attr_fm1388_reg);
    if (ret != 0)
    {
        lidbg(TAG"Failed to create fm1388_reg sysfs files: %d\n", ret);
        return ret;
    }

    lidbg(TAG"%s: device_create_file - dev_attr_index_reg.\n", __func__);
    ret = device_create_file(&pdev->dev, &dev_attr_index_reg);
    if (ret != 0)
    {
        lidbg(TAG"Failed to create index_reg sysfs files: %d\n", ret);
        return ret;
    }

    lidbg(TAG"%s: device_create_file - dev_attr_fm1388_addr.\n", __func__);
    ret = device_create_file(&pdev->dev, &dev_attr_fm1388_addr);
    if (ret != 0)
    {
        lidbg(TAG"Failed to create fm1388_addr sysfs files: %d\n", ret);
        return ret;
    }

    ret = device_create_file(&pdev->dev, &dev_attr_fm1388_status);
    if (ret != 0)
    {
        lidbg(TAG"Failed to create fm1388_status sysfs files: %d\n", ret);
        return ret;
    }

    ret = device_create_file(&pdev->dev, &dev_attr_fm1388_reinit);
    if (ret != 0)
    {
        lidbg(TAG"Failed to create fm1388_reinit sysfs files: %d\n", ret);
        return ret;
    }
    ret = device_create_file(&pdev->dev, &dev_attr_fm1388_test);
    if (ret != 0)
    {
        lidbg(TAG"Failed to create fm1388_test sysfs files: %d\n", ret);
        return ret;
    }

#ifdef SOC_mt3360
    ret = GPIO_MultiFun_Set(FM1388_RESET_PIN, PINMUX_LEVEL_GPIO_END_FLAG);
    lidbg("GPIO_MultiFun_Set %d\n", ret);
#endif
    gpio_request(FM1388_RESET_PIN, "fm1388_reset");
#if 0
    request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
                            "fm1388_fw", &i2c->dev, GFP_KERNEL, i2c,
                            fm1388_fw_loaded);
#endif
    fm1388_boot_status = FM1388_COLD_BOOT;
    CREATE_KTHREAD(fm1388_fw_loaded, NULL);
    lidbg(TAG"%s: misc_register.\n", __func__);
    ret = misc_register(&fm1388_dev);
    if (ret)
        dev_err(&pdev->dev, "Couldn't register control device\n");

    lidbg_new_cdev(&fm1388_mode_fops, "fm1388_switch_mode");
	lidbg_shell_cmd("chmod 777 /dev/fm1388_switch_mode");

    INIT_DELAYED_WORK(&dsp_start_vr, dsp_start_vr_work);


#ifdef SUSPEND_ONLINE
    lidbg(TAG"%s: register_lidbg_notifier\n", __func__);
    register_lidbg_notifier(&lidbg_notifier);
#endif


#ifdef FM1388_IRQ
    INIT_WORK(&fm1388_irq_work, fm1388_irq_handling_work);
    fm1388_irq_wq = create_singlethread_workqueue("fm1388_irq_wq");

    fm1388_idx = (int *)kmalloc(sizeof(int), GFP_KERNEL);
    *fm1388_idx = 1;

    fm1388_irq = sw_gpio_irq_request(GPIOH(16), TRIG_EDGE_POSITIVE, (peint_handle)fm1388_irq_handler, (int *)fm1388_idx);

    if (fm1388_irq == 0)
    {
        lidbg(TAG"%s: sw_gpio_irq_request failed\n", __func__);
    }
#endif

#ifdef SHOW_FRAMECNT
    INIT_WORK(&fm1388_framecnt_work, fm1388_framecnt_handling_work);
    fm1388_framecnt_wq = create_singlethread_workqueue("fm1388_framecnt_wq");
    //msleep(180000);
    queue_work(fm1388_framecnt_wq, &fm1388_framecnt_work);
#endif
    return 0;
}

#ifdef CTRL_GPIOS
static int fm1388_pinctrl_probe(struct platform_device *pdev)
{
    int ret;

    lidbg(TAG"%s: entry.\n", __func__);

    if(pdev == NULL)
    {
	lidbg(TAG"%s: pdev==NULL.\n", __func__);
	return -1;
    }
	
    pinctrliis0 = devm_pinctrl_get(&pdev->dev);
    if(IS_ERR(pinctrliis0))
    {
	    ret = PTR_ERR(pinctrliis0);
	    lidbg(TAG"%s: pinctrliis0 get failed,ret=%d.\n", __func__,ret);
    }

    pins_default = pinctrl_lookup_state(pinctrliis0,"gpios_def_cfg");
    if(IS_ERR(pins_default))
    {
	    ret = PTR_ERR(pins_default);
	    lidbg(TAG"%s: pins_default get failed,ret=%d.\n", __func__,ret);
    }

    pins_sleep= pinctrl_lookup_state(pinctrliis0,"gpios_sleep_cfg");
    if(IS_ERR(pins_sleep))
    {
	    ret = PTR_ERR(pins_sleep);
	    lidbg(TAG"%s: pins_sleep get failed,ret=%d.\n", __func__,ret);
    }

	lidbg(TAG"%s: exit.\n", __func__);

    return 0;
}



static const struct of_device_id fm1388_of_match[] = {
	   {.compatible = "fm,fm1388pinsctrl",},
	   {},
};


 static struct platform_driver fm1388_pinctrl = {
	    .driver = {
		.name = "fm1388pinsctrl",
		.of_match_table = fm1388_of_match,
	   	},
		.probe = fm1388_pinctrl_probe,
 };
#endif

static int fm1388_remove(struct platform_device *pdev)
{
    return 0;
}


static struct platform_device fm1388_devices =
{
    .name			= "fm1388",
    .id 			= 0,
};

static struct platform_driver fm1388_driver =
{
    .probe  = fm1388_probe,
    .remove = fm1388_remove,
    .driver = {
        .name = "fm1388",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &fm1388_i2c_ops,
#endif
    },
};

static int fm1388_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;
#ifdef CTRL_GPIOS
    platform_driver_register(&fm1388_pinctrl);
#endif
    platform_device_register(&fm1388_devices);
    platform_driver_register(&fm1388_driver);
    return 0;
}

static void __exit fm1388_exit(void)
{
    platform_device_unregister(&fm1388_devices);
    platform_driver_unregister(&fm1388_driver);
}

module_init(fm1388_init);
module_exit(fm1388_exit);


MODULE_DESCRIPTION("FM1388 I2C Driver");
MODULE_AUTHOR(" sample code <dannylan@fortemedia.com>");
MODULE_LICENSE("GPL v2");
