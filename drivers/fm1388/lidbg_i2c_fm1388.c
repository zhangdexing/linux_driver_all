/*
 * drivers/i2c/busses/i2c-fm1388.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
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
#include <linux/kfifo.h>
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
#include <linux/kthread.h>
#include "fm_wav.h"
#include <linux/sched.h>
#include <linux/sched/rt.h>


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


enum FM1388_CAR_TYPE
{
	RUIJIE=1,
	MENGDIOU
};
static int fm1388_is_bypass = 0;

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

char default_filepath[255] = "/flysystem/lib/out/fm1388/";

char filepath_name[255];

#define CDEV_COUNT 1
#define FM1388_CDEV_NAME 		"fm1388_smp"

#define VERSION "1.0.2"

#define FM1388_SPI
//#define FM1388_SPI_ENABLE

#define VOICE_DATA_FILE_NAME 	"voice.dat"
#define DEFAULT_SDCARD_PATH		"/sdcard/"

#define MAX_KFIFO_BUFFER_SIZE	320000 //40960

#define USER_DEFINED_PATH_FILE	"user_defined_path.cfg"
#define USER_VEC_PATH			"VEC_PATH="
#define USER_KERNEL_SDCARD_PATH	"K_SD_PATH="
#define USER_USER_SDCARD_PATH	"U_SD_PATH="
#define USER_OUTPUT_LOG			"OUTPUT_LOG="
#define USER_KERNEL_LOG_PREFIX	"klog-"
#define USER_LOG_FOLDER			"FMLog"
bool	b_output_log				= false;
char 	SDCARD_PATH[MAX_PATH_LEN];
char 	filepath[CONFIG_LINE_LEN];
char 	filepath_save[CONFIG_LINE_LEN];


#ifdef CUBIE_TRUCK
#define SPI_BURST_READ_SPEED 	12000000
#else
#define SPI_BURST_READ_SPEED 	20000000
#endif

struct fm1388_data_t*		fm1388_data;


#define CTRL_GPIOS

static struct platform_device *fm1388_pdev;
static bool fm1388_is_dsp_on = false;	// is dsp power on? dsp is off in init time
//static unsigned int fm1388_dsp_mode = FM_SMVD_CFG_BARGE_IN;
static int fm1388_dsp_mode = -1;	// DSP working mode, user define mode in .cfg file
static bool fm1388_dsp_working 	= false; //set it to true after check firmware frame count is varied
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

dev_cmd_dv_fetch 			fetch_param;
mm_segment_t 				oldfs;

dev_cmd_dv_playback			playback_param;
static u8 voice_buffer_play[FM1388_BUFFER_LEN] 	= {0};
static u32 play_status 							= 0;
static u32 play_status_counter 					= 0;


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
struct timex    txc2;
#endif

#define OPEN_NO_ERROR 0
#define OPEN_ERROR -1

void postwork_set_mode(void);
int load_fm1388_mode_cfg(char *file_src, unsigned int choosed_mode);
int load_fm1388_vec(char *file_src);
char *combine_path_name(char *s, char *append);
static void set_vec_file_path(void);


struct file *openFile(char *path, int flag, int mode);
#define FRAME_NUM		2		//the frame number, each time get from fifo and write to file
#define MAX_TEST_COUNT 	128
#define MAX_READY_CHECK	20480
//#define OUTPUT_READY_STATE_DATA
#define OUTPUT_ESLAPSE_DATA
u32			record_error_counter 				= 0L;
u32  		record_total_counter 				= 0L;
u32			first_record_error_frame_counter 	= 0L;
u32			last_record_error_frame_counter 	= 0L;
u32			playback_error_counter 				= 0L;
u32  		playback_total_counter 				= 0L;
u32			first_playback_error_frame_counter 	= 0L;
u32			last_playback_error_frame_counter 	= 0L;
static u8	record_spi_buffer[DSP_OUTPUT_BUFFER_SIZE] 	= {0};
static u8	temp_buffer[DSP_OUTPUT_BUFFER_SIZE * FRAME_NUM]	= {0};
static u8	source_file_channel_number			= 0;
static u32	source_file_total_size				= 0L;
static u8 	temp_buffer1[FM1388_BUFFER_LEN] 	= {0};
static u8 	temp_buffer2[FM1388_BUFFER_LEN] 	= {0};

#ifdef OUTPUT_READY_STATE_DATA
typedef struct ready_check_t {
	struct timespec time;
	u8				local_index;
	u8				dsp_index;
	u16				state;
	u16				ready_state;
	bool			hit;
	u32				err_counter;
	u32				counter;
} ready_check;
u16 record_ready_check_counter = 0;
ready_check st_record_ready_check[MAX_READY_CHECK];
u16 playback_ready_check_counter = 0;
ready_check st_playback_ready_check[MAX_READY_CHECK];
#endif

#ifdef OUTPUT_ESLAPSE_DATA
typedef struct eslapse_check_t {
	struct timespec time;
	u32				eslapse;
	u32				counter;
} eslapse_check;
u16 record_eslapse_check_counter = 0;
eslapse_check st_record_eslapse_check[MAX_TEST_COUNT];
u16 playback_eslapse_check_counter = 0;
eslapse_check st_playback_eslapse_check[MAX_TEST_COUNT];
#endif

typedef struct error_check_t {
	struct timespec time;
	u32				counter;
	u16				state;
	u8				index;
	u8				dsp_index;
	u32				error_counter;
} error_check;
u16 record_error_check_counter = 0;
error_check st_record_error_check[MAX_TEST_COUNT];
u16 playback_error_check_counter = 0;
error_check st_playback_error_check[MAX_TEST_COUNT];





//routines for output debug to file, 
//we save log file under sdcard, it can not output log to file from driver starting,
//sdcard fs is not ready at that time.
/* Can not create or check folder existence on sdcard in kernel mode in simple way.
int check_create_log_folder(void) {
	int  ret = ESUCCESS;
	char str_log_folder_path[MAX_PATH_LEN];
	mm_segment_t	org_fs;
	struct file*	filp  = NULL;  
	
	if(b_output_log == false) return ret;
	
	snprintf(str_log_folder_path, MAX_PATH_LEN, "%s%s/", SDCARD_PATH, USER_LOG_FOLDER);

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	
	filp = filp_open(str_log_folder_path, O_CREAT | O_RDWR | O_DIRECTORY, 0666);  
	if (IS_ERR(filp)) {  
		pr_err("Cannot open kernel log folder(%s).\n", str_log_folder_path);  
		set_fs(org_fs);
		//b_output_log = false;
		return -EFAILOPEN;
	} 
	else {
		filp_close(filp, NULL);
	}

	set_fs(org_fs);
	return ret;
}
*/


int generate_log_file_name(char* file_name) {
	struct timex txc;
	struct rtc_time tm;
	char str_today[32];
	
	if(file_name == NULL) return -1;
	
	if(b_output_log == false) return ESUCCESS;
	
	//get today's date
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec, &tm);
	snprintf(str_today, 32, "%04d%02d%02d", (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday);	

	//combine log file name
	snprintf(file_name, MAX_PATH_LEN, "%s%s/%s%s.txt", 
			SDCARD_PATH, USER_LOG_FOLDER, USER_KERNEL_LOG_PREFIX, str_today);
			
	return ESUCCESS;
}

int get_output_log_file_path(char* str_log_file_path) {
	int  ret = ESUCCESS;

	if(str_log_file_path == NULL) return -1;
	
	/*
	ret = check_create_log_folder();
	if(ret != ESUCCESS) return ret;
	*/
	/*
	if ((k_obj = kobject_create_and_add("/sdcard/sss", NULL)) == NULL ) {
		pr_err("sss sys node create error \n");
	}	
	if ((k_obj = kobject_create_and_add("sysfs_demo", NULL)) == NULL ) {
		pr_err("sysfs_demo sys node create error \n");
	}	
	*/
	
	ret = generate_log_file_name(str_log_file_path);
	if(ret != ESUCCESS) return ret;
	
	return ESUCCESS;
}

int output_debug_log(bool output_console, const char *fmt, ...) {  
    va_list  		argp;  
    int 			ret = ESUCCESS;
    char 			str_log_file_path[MAX_PATH_LEN];
	struct timex 	txc;
	struct rtc_time tm;
 	mm_segment_t	org_fs;
	struct file*	filp  = NULL;  
	char 			strTemp[CONFIG_LINE_LEN + 1];
	
   
    if(fmt == NULL) return -EPARAMINVAL;
 
    //not allow to output log
    if(b_output_log == false) {
		if(output_console == true) {
			va_start(argp,  fmt);  
			
			vsnprintf(strTemp, CONFIG_LINE_LEN, fmt, argp);  
			pr_err("%s", strTemp);
			
			va_end(argp);  
		}
		return ESUCCESS;
	}
   
    //get log file path
    memset(str_log_file_path, 0, MAX_PATH_LEN);
	ret = get_output_log_file_path(str_log_file_path);
    if(ret != ESUCCESS) return ret;
    
	org_fs = get_fs();
	set_fs(KERNEL_DS);

    //open file
	filp = filp_open(str_log_file_path, O_CREAT | O_RDWR | O_APPEND, 0666);  
	if (IS_ERR(filp)) {  
		pr_err("Cannot open kernel log file(%s).\n", str_log_file_path);  
		set_fs(org_fs);

		if(output_console == true) {
			va_start(argp,  fmt);  
			
			vsnprintf(strTemp, CONFIG_LINE_LEN, fmt, argp);  
			pr_err("%s", strTemp);
			
			va_end(argp);  
		}
		return -EFAILOPEN;
	}  

	//get today's date time
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec, &tm);
    
    snprintf(strTemp,  CONFIG_LINE_LEN, "[%04d-%02d-%02d %02d:%02d:%02d]:  ", 
				(tm.tm_year + 1900), (1 + tm.tm_mon), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);  

	filp->f_op->write(filp, (u8*)(strTemp), sizeof(char) * strlen(strTemp), &filp->f_pos);

    va_start(argp,  fmt);  
    vsnprintf(strTemp, CONFIG_LINE_LEN, fmt, argp);  
	filp->f_op->write(filp, (u8*)(strTemp), sizeof(char) * strlen(strTemp), &filp->f_pos);

    if(output_console == true) {
		pr_err("%s", strTemp);
	}
    va_end(argp);  
   

	if(!IS_ERR(filp)) {
		filp_close(filp, NULL);
	}
	
	set_fs(org_fs);
    return ESUCCESS;
} 
//


static bool isFrameCountRise(void);

#ifdef CTRL_GPIOS
struct pinctrl *pinctrliis0=NULL;
struct pinctrl_state *pins_default=NULL, *pins_sleep=NULL;
#endif

static int fm1388_fw_loaded(void *data);


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
    SOC_IO_Output(0, FM1388_RESET_PIN, 1);
    msleep(100);
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
		postwork_set_mode();
        }
        else
        {
            lidbg(TAG"%s file open error!\n", combine_path_name(filepath_name, "FM1388_mode.cfg"));
			fm1388_dsp_mode = current_mode;
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

//set SPI status
u8 set_spi_rec_status( u16 new_state )
{
    u8  err;
    u32 state;
    
    err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    state |= new_state;
    err = fm1388_spi_write(DSP_CMD_ADDR, state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }
 
    return err;
}

//send CMD to FM1388 to enable SPI record 
u8 enable_spi_rec( void )
{
    u8  err = ESUCCESS;
    u32 state;
    
    err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    state |= RECORD_ENABLE;
    err = fm1388_spi_write(DSP_CMD_ADDR, state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }

    return err;
}

//send CMD to FM1388 to disable SPI record when record stopped
u8 disable_spi_rec( void )
{
    u8  err = ESUCCESS;
    u32 state;
    
    err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    state &= RECORD_ENABLE_RESET;
    err = fm1388_spi_write(DSP_CMD_ADDR, state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }

    return err;
}

//send CMD to FM1388 for checking SPI record is ready or not 
u8 is_spi_rec_buf_ready(u8 buf_index)
{
    u8  err = ESUCCESS;
    u32 temp_counter;
    u32 dsp_buf_index;
    u32	ready_state1, ready_state2;
    u32  ready_state_all = 0;
	u32	buffer_ready_word_addr[BUFFER_NUMBER] = {
		RECORD_SYNC_BUFFER0_ADDR,
		RECORD_SYNC_BUFFER1_ADDR,
		RECORD_SYNC_BUFFER2_ADDR,
		RECORD_SYNC_BUFFER3_ADDR };
    u8	buf_ready_pattern[BUFFER_NUMBER][BUFFER_NUMBER] = {	{0x1, 0x3, 0x7, 0xF},
															{0x2, 0x6, 0xE, 0xF},
															{0x4, 0xC, 0xD, 0xF},
															{0x8, 0x9, 0xB, 0xF}
														};
	
    err = fm1388_spi_read( RECORD_ERROR_COUNTER_ADDR, &temp_counter, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
	//if address is continuous, we can read 4 bytes once and use 2 read can get 4 buffer's ready word.
    //read 4 buffer ready word, then combine them together
    err = fm1388_spi_read( buffer_ready_word_addr[0], &ready_state1, 4);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    err = fm1388_spi_read( buffer_ready_word_addr[2], &ready_state2, 4);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }

    ready_state_all = (u32)(ready_state_all | (ready_state1 & 0x0001) | ((ready_state1 & 0x00010000) >> 15) |
						((ready_state2 & 0x0001) << 2) | ((ready_state2 & 0x00010000) >> 13));
/*    
	output_debug_log(true, "[%s]: buffer index = %#x, ready_state=%04x, ready_state1=%08x, \
		ready_state2=%08x, ready_state_all=%04x, temp_counter=%d\n", 
			__func__, buf_index, ready_state, ready_state1, ready_state2, ready_state_all, temp_counter);
*/
	if(record_error_counter != temp_counter) {
		if(record_error_check_counter == 0) first_record_error_frame_counter = record_total_counter;
		last_record_error_frame_counter = record_total_counter;
		
		record_error_counter = temp_counter;
		if(record_error_check_counter < MAX_TEST_COUNT) {
			fm1388_spi_read( DSP_RECORD_BUFFER_INDEX_ADDR, &dsp_buf_index, 2);
			
			st_record_error_check[record_error_check_counter].counter = record_total_counter;
			st_record_error_check[record_error_check_counter].index = buf_index;
			st_record_error_check[record_error_check_counter].dsp_index = dsp_buf_index;
			st_record_error_check[record_error_check_counter].state = ready_state_all;
			st_record_error_check[record_error_check_counter].error_counter = temp_counter;
			do_posix_clock_monotonic_gettime(&(st_record_error_check[record_error_check_counter].time));  
		}
		record_error_check_counter++;
	}
	
#ifdef OUTPUT_READY_STATE_DATA 
	fm1388_spi_read( DSP_RECORD_BUFFER_INDEX_ADDR, &dsp_buf_index, 2);
	if(record_ready_check_counter < MAX_READY_CHECK) {
		st_record_ready_check[record_ready_check_counter].dsp_index = dsp_buf_index;
		st_record_ready_check[record_ready_check_counter].local_index = buf_index;
		st_record_ready_check[record_ready_check_counter].state = ready_state_all;
		st_record_ready_check[record_ready_check_counter].err_counter = temp_counter;
		st_record_ready_check[record_ready_check_counter].counter = record_total_counter;
		do_posix_clock_monotonic_gettime(&(st_record_ready_check[record_ready_check_counter].time));  
		record_ready_check_counter ++;
	}
#endif
	
	//output_debug_log(true, "[%s]: buffer index = %#x, ready_state=%04x, ready_state_all=%04x\n", 
	//	__func__, buf_index, ready_state, ready_state_all);
    if(buf_ready_pattern[buf_index][3] == (ready_state_all & buf_ready_pattern[buf_index][3])) return 4;
    if(buf_ready_pattern[buf_index][2] == (ready_state_all & buf_ready_pattern[buf_index][2])) return 3;
    if(buf_ready_pattern[buf_index][1] == (ready_state_all & buf_ready_pattern[buf_index][1])) return 2;
    if(buf_ready_pattern[buf_index][0] == (ready_state_all & buf_ready_pattern[buf_index][0])) return 1;
    else return 0;
}

//get FM1388 status, reset the record ready bit, 
//then send new status to FM1388 to let it record data to another buffer. 
u8 spi_record_buf_finish(u8 buf_index)
{
    u8  err = ESUCCESS;
    u32 ready_state;
	u32	buffer_ready_word_addr[BUFFER_NUMBER] = {
		RECORD_SYNC_BUFFER0_ADDR,
		RECORD_SYNC_BUFFER1_ADDR,
		RECORD_SYNC_BUFFER2_ADDR,
		RECORD_SYNC_BUFFER3_ADDR };
    
    err = fm1388_spi_read( buffer_ready_word_addr[buf_index], &ready_state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }

    ready_state = 0;
    err = fm1388_spi_write(buffer_ready_word_addr[buf_index], ready_state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }
    return err;
}

/********************************************************************************************************
* Description :  revert endian-mode from[7..0] to[0..7] of the burst read data .
* Argument(s) :  
* 				*pdata          is point to where the source from  
*                data_length     is data length to revert in bytes, must be X 8   
* Return(s)   :  error number.           
********************************************************************************************************/
int swap_spi_data(u8* pdata, u32 data_length )
{
    u32 i, j, offset;    
    u32 block_number;
	u8  temp;
    
 	if ((pdata == NULL) || (data_length == 0)) {
		output_debug_log(true, "[%s]: please provide valid parameter. pdata=0x%#x, data_length =%#x\n", 
						__func__, *pdata, data_length);
		return -EPARAMINVAL;
	}
	
   block_number = data_length >> 3;

    for( i = 0; i < block_number; i++) {
		offset = i * 8;
        for( j = 0; j < 4; j++ ) {
			temp = pdata[offset + j];
			pdata[offset + j] = pdata[offset + 7 - j];
			pdata[offset + 7 - j] = temp;
        }
   }   

	return ESUCCESS;
}

bool valid_channel(u32 ch_num, u8* ch_idx, u8 channel_idx) {
	if((ch_num <= 0) || (ch_num > DSP_SPI_REC_CH_NUM)) return false;
	if(ch_num == DSP_SPI_REC_CH_NUM) return true;
	if(ch_idx == NULL) return false;
	if(ch_idx[channel_idx] != '1') return false;
	return true;
}

int fetch_one_frame_voice_data(u32 spi_base_addr, u8* local_buffer, char* channel_need_rec) {
    int  err 			= 0;
	u8   channel_idx 	= 0;
	u32  offset			= 0;
	u32  valid_offset	= 0;

	if((local_buffer == NULL) || (channel_need_rec == NULL)) return -EPARAMINVAL;
	
	offset = 0;
	valid_offset = 0;
	for(channel_idx = 0; channel_idx < fetch_param.ch_num; channel_idx ++) {
		offset = channel_need_rec[channel_idx] * fetch_param.framesize;
		err = fm1388_spi_burst_read(spi_base_addr + offset,  
									local_buffer + valid_offset,  
									fetch_param.framesize );
		if( err != ESUCCESS ) { 
			output_debug_log(true, "[%s]: fm1388_spi_burst_read return error = %#x\n", __func__, err);
			break;
		}
		
		valid_offset += fetch_param.framesize;
	}
	
if((record_total_counter >= 10) && (record_total_counter < 15)){
		for(offset = 0; offset < 320; offset+=16) {
			output_debug_log(true, "%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n\n", 
				local_buffer[offset+0], local_buffer[offset+1], local_buffer[offset+2], local_buffer[offset+3], local_buffer[offset+4], local_buffer[offset+5], local_buffer[offset+6], local_buffer[offset+7], 
				local_buffer[offset+8], local_buffer[offset+9], local_buffer[offset+10], local_buffer[offset+11], local_buffer[offset+12], local_buffer[offset+13], local_buffer[offset+14], local_buffer[offset+15]);				
		}
	}

	return err;
}

void init_global_variables( void ) {
	record_error_counter = 0;
	record_total_counter = 0;
	first_record_error_frame_counter = 0L;
	last_record_error_frame_counter = 0L;
	record_error_check_counter = 0;
	memset(st_record_error_check, 0, sizeof(error_check) * MAX_TEST_COUNT);
	
#ifdef OUTPUT_READY_STATE_DATA	
	record_ready_check_counter = 0;
	memset(st_record_ready_check, 0, sizeof(ready_check) * MAX_READY_CHECK);
#endif
	
#ifdef OUTPUT_ESLAPSE_DATA
	record_eslapse_check_counter = 0;
	memset(st_record_eslapse_check, 0, sizeof(eslapse_check) * MAX_TEST_COUNT);
#endif


	playback_error_counter = 0;
	playback_total_counter = 0;
	first_playback_error_frame_counter = 0L;
	last_playback_error_frame_counter = 0L;
	playback_error_check_counter = 0;
	memset(st_playback_error_check, 0, sizeof(error_check) * MAX_TEST_COUNT);
	
#ifdef OUTPUT_READY_STATE_DATA	
	playback_ready_check_counter = 0;
	memset(st_playback_ready_check, 0, sizeof(ready_check) * MAX_READY_CHECK);
#endif
	
#ifdef OUTPUT_ESLAPSE_DATA
	playback_eslapse_check_counter = 0;
	memset(st_playback_eslapse_check, 0, sizeof(eslapse_check) * MAX_TEST_COUNT);
#endif

	return;
}

void output_record_debug_info( void ) {
	int i = 0;
#ifdef OUTPUT_READY_STATE_DATA    
	u32 eslapse = 0L;
#endif
	
	if(record_error_check_counter > 0) {
		output_debug_log(true, "[%s]: recording loss frame happend around below frame counter:%d\n", 
							__func__, record_error_check_counter);
		for(i = 0; i < MAX_TEST_COUNT; i++) {
			if(i < record_error_check_counter) {
				output_debug_log(true, "[%d]:%05d.%ld:\t%ld\t%ld\t%d\t%d\t%04x\n", i, 
					st_record_error_check[i].time.tv_sec, 
					st_record_error_check[i].time.tv_nsec, 
					(i > 0) ? (st_record_error_check[i].counter + st_record_error_check[i - 1].error_counter) : st_record_error_check[i].counter,
					st_record_error_check[i].error_counter,
					st_record_error_check[i].index,
					st_record_error_check[i].dsp_index,
					st_record_error_check[i].state);
			}
		}	
	}

#ifdef OUTPUT_READY_STATE_DATA    
	output_debug_log(false, "[%s]: recording ready check result:\n", __func__);
	if(record_ready_check_counter > 0) {
		for(i = 0; i < MAX_READY_CHECK; i++) {
			if(i < record_ready_check_counter) {
				eslapse = 0;
				if(i >= 1) {
					eslapse = (unsigned long)((long)(st_record_ready_check[i].time.tv_sec - st_record_ready_check[i - 1].time.tv_sec) * 1000000000L + 
						((long)(st_record_ready_check[i].time.tv_nsec - st_record_ready_check[i - 1].time.tv_nsec)));
				}
				
				output_debug_log(false, "[%d]:%05d.%ld:\t%09ld\t%02d\t%02d\t%04x\t%09d\t%09d\n", i, 
					st_record_ready_check[i].time.tv_sec, 
					st_record_ready_check[i].time.tv_nsec, 
					eslapse, 
					st_record_ready_check[i].local_index, 
					st_record_ready_check[i].dsp_index, 
					st_record_ready_check[i].state,
					st_record_ready_check[i].err_counter,
					st_record_ready_check[i].counter);
			}
		}	
	}
#endif
	
	return;
}

int fm1388_push_data_to_fifo(u8* data_buffer, u16 data_size) {
	u32 fifo_avail = 0L;
	unsigned long timeout = jiffies + msecs_to_jiffies(2);
	u32 ret = 0L;

	if(data_buffer == NULL) return ret;
	
	spin_lock(&fm1388_data->rec_lock);
	fifo_avail = kfifo_avail(fm1388_data->rec_kfifo);
	spin_unlock(&fm1388_data->rec_lock);
	timeout = jiffies + msecs_to_jiffies(2);
	while ((fifo_avail < data_size) && time_before(jiffies, timeout)) {
		usleep_range(200, 300);		//reduce the sleep time causes time reduce from fetch end to next fetch
		spin_lock(&fm1388_data->rec_lock);
		fifo_avail = kfifo_avail(fm1388_data->rec_kfifo);
		spin_unlock(&fm1388_data->rec_lock);
	}
	
	if (fifo_avail < data_size) {
		output_debug_log(true, "[%s]: will lost one frame data for no enough space in fifo.\n", __func__);
	}
	else {
		ret = kfifo_in_spinlocked(fm1388_data->rec_kfifo, data_buffer, data_size, &fm1388_data->rec_lock);
	}

	return ret;
}

int fm1388_save_audio_data_to_file(void* data) {
	unsigned long timeout = jiffies + msecs_to_jiffies(2);
	struct file *fpdata = NULL;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	int	 data_size 		= 0;
	int  kfifo_data_len;
	u16  ret = 0;
	u32	 total_gotten = 0L;
#ifdef OUTPUT_ESLAPSE_DATA    
	struct timespec start_time, end_time;  
	u32	 eslapse		= 0L;
	u32  max_eslapse 	= 0L;
#endif

	output_debug_log(true, "[%s]: start save data thread.\n", __func__);
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	
	data_size = fetch_param.ch_num * fetch_param.framesize * FRAME_NUM;

	//snprintf(data_file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, VOICE_DATA_FILE_NAME);
	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, VOICE_DATA_FILE_NAME);
	fpdata 	= filp_open(data_file_path, O_CREAT | O_RDWR, 0666);
	if (IS_ERR(fpdata)) {
		output_debug_log(true, "[%s]: the file or path does not exist. %s\n", __func__, data_file_path);
		set_fs(oldfs);
		return -EFAILOPEN;
	}

    while (!kthread_should_stop()) {
#ifdef OUTPUT_ESLAPSE_DATA    
		do_posix_clock_monotonic_gettime(&start_time);  
#endif
		spin_lock(&fm1388_data->rec_lock);
		kfifo_data_len = kfifo_len(fm1388_data->rec_kfifo);
		spin_unlock(&fm1388_data->rec_lock);
		timeout = jiffies + msecs_to_jiffies(10);
		while ((kfifo_data_len < data_size) && time_before(jiffies, timeout)) {
			usleep_range(200, 300);
			spin_lock(&fm1388_data->rec_lock);
			kfifo_data_len = kfifo_len(fm1388_data->rec_kfifo);
			spin_unlock(&fm1388_data->rec_lock);
		}

		if(kfifo_data_len < data_size) {
			//output_debug_log(true, "[%s]: no enough data in fifo.\n", __func__);
		}
		else {
			ret = kfifo_out_spinlocked(fm1388_data->rec_kfifo, temp_buffer, data_size, &fm1388_data->rec_lock);
			total_gotten += ret;
			if(!IS_ERR(fpdata)) {
				fpdata->f_op->write(fpdata, (u8*)(temp_buffer), data_size, &fpdata->f_pos);
#ifdef OUTPUT_ESLAPSE_DATA    
				do_posix_clock_monotonic_gettime(&end_time);  
				eslapse = (unsigned long)((long)(end_time.tv_sec - start_time.tv_sec) * 1000000000L + 
						(long)(end_time.tv_nsec - start_time.tv_nsec));
								
				if(eslapse > max_eslapse) max_eslapse = eslapse;
#endif
			}
			else {
				output_debug_log(true, "[%s]: open data file failed.\n", __func__);
			}
		}

		usleep_range(300, 400);
	} 

	if(!IS_ERR(fpdata)) {
		filp_close(fpdata, NULL);
	}
	set_fs(oldfs);

	fm1388_data->save_data_thread_id = NULL;

	output_debug_log(true, "[%s]: totally got data %d.\n", __func__, total_gotten);
#ifdef OUTPUT_ESLAPSE_DATA    
	output_debug_log(true, "[%s]: max. eslapse:%d\n", __func__, max_eslapse);
#endif
	output_debug_log(true, "[%s]: exit save data thread.\n", __func__);
	return ESUCCESS;
}

int fm1388_fetch_voice_data_by_channel_4buffers(void *data) {
    int  err = 0;
	u8*  start_addr;
	u32  spi_base_addr;
	u8   channel_idx 	= 0;
	int  ready_num 		= 0;
	u16	 cur_buffer		= 0;
	u16	 i;
	char channel_need_rec[DSP_SPI_REC_CH_NUM];
	u16	 data_size 		= 0;
#ifdef OUTPUT_ESLAPSE_DATA    
	struct timespec start_time, end_time, init_time, end_time_1;  
	u32	 eslapse		= 0L;
	u32  max_eslapse 	= 0L;
#endif
	u32 total_put = 0L, ret = 0L;
	
		
	output_debug_log(true, "[%s]: start SPI recording thread.\n", __func__);

	//fuli 20160827 added to change spi speed before burst read
	err = fm1388_spi_change_maxspeed(SPI_BURST_READ_SPEED);
	if(err) {
		output_debug_log(true, "[%s]: fm1388_spi_change_maxspeed return value = %d\n", __func__, err);
		fm1388_data->thread_id = NULL;
		return err;
	}
	//	

	data_size = fetch_param.ch_num * fetch_param.framesize;
	start_addr = record_spi_buffer;
	init_global_variables();

	i = 0;
	memset(channel_need_rec, 0, sizeof(char) * DSP_SPI_REC_CH_NUM);
	for(channel_idx = 0; (channel_idx < DSP_SPI_REC_CH_NUM); channel_idx ++) {
		if(!valid_channel(fetch_param.ch_num, fetch_param.ch_idx, channel_idx)) {
			continue;
		}
		channel_need_rec[i] = channel_idx;
		i++;
	}

	err = enable_spi_rec();
	if (err) {
		err = -ECANNOTENRECORD;

		fm1388_data->thread_id = NULL;
		output_debug_log(true, "[%s]: Fail to enable SPI Recording.\n", __func__);
		return err;
	}

#ifdef OUTPUT_ESLAPSE_DATA    
	do_posix_clock_monotonic_gettime(&init_time);  
#endif
    while (!kthread_should_stop()) {
		ready_num = is_spi_rec_buf_ready(cur_buffer);
		if((ready_num < 0) || (ready_num > 4))
			output_debug_log(true, "[%s]: is_spi_rec_ready return value = %#x\n", __func__, ready_num);
		if(ready_num > 0) {
			do {
#ifdef OUTPUT_ESLAPSE_DATA    
				do_posix_clock_monotonic_gettime(&start_time);  
#endif
				spi_base_addr = fetch_param.addr_output0 + cur_buffer * DSP_OUTPUT_BUFFER_SIZE;//buffer_size;
				
				err = fetch_one_frame_voice_data(spi_base_addr, start_addr, channel_need_rec);
				if(err != ESUCCESS) break;

				err = spi_record_buf_finish(cur_buffer);
				record_total_counter++;

#ifdef OUTPUT_ESLAPSE_DATA    
								do_posix_clock_monotonic_gettime(&end_time_1);	
#endif

				ret = fm1388_push_data_to_fifo(start_addr, data_size);	
				total_put += ret;
				cur_buffer = (cur_buffer + 1) & 0x03;

#ifdef OUTPUT_ESLAPSE_DATA    
				do_posix_clock_monotonic_gettime(&end_time);  
				eslapse = (unsigned long)((long)(end_time.tv_sec - start_time.tv_sec) * 1000000000L + 
						(long)(end_time.tv_nsec - start_time.tv_nsec));
								
				if(eslapse > max_eslapse) max_eslapse = eslapse;
				if(eslapse > 20000000L) {
					if(record_eslapse_check_counter < (MAX_TEST_COUNT - 1)) {
						st_record_eslapse_check[record_eslapse_check_counter].counter = record_total_counter;
						st_record_eslapse_check[record_eslapse_check_counter].eslapse = eslapse;
						do_posix_clock_monotonic_gettime(&(st_record_eslapse_check[record_eslapse_check_counter].time));  
						record_eslapse_check_counter++;

						eslapse = (unsigned long)((long)(end_time.tv_sec - end_time_1.tv_sec) * 1000000000L + 
								(long)(end_time.tv_nsec - end_time_1.tv_nsec));
						st_record_eslapse_check[record_eslapse_check_counter].counter = record_total_counter;
						st_record_eslapse_check[record_eslapse_check_counter].eslapse = eslapse;
						do_posix_clock_monotonic_gettime(&(st_record_eslapse_check[record_eslapse_check_counter].time));  
						record_eslapse_check_counter++;
					}
				}

#endif

				ready_num --;
			} while(ready_num > 0);
			if(err != ESUCCESS) break;
		}
		else {
			//usleep_range(100, 200);
		}
    } 
    err = fm1388_spi_read( RECORD_ERROR_COUNTER_ADDR, &record_error_counter, 2);
	
	//disable record in DSP
	disable_spi_rec();
	fm1388_data->thread_id = NULL;

	output_debug_log(true, "[%s]: totally put data %d.\n", __func__, total_put);
	
	output_record_debug_info();
#ifdef OUTPUT_ESLAPSE_DATA    
	output_debug_log(true, "[%s]: record %d frames. loss %d frames, max_eslapse=%ld, \
	first_lost_happen_at=%ld, last_lost_happen_at=%ld\n", 
			__func__, record_total_counter + record_error_counter, record_error_counter, max_eslapse, 
			first_record_error_frame_counter, last_record_error_frame_counter);
#else
	output_debug_log(true, "[%s]: record %d frames. loss %d frames, first_lost_happen_at=%ld, last_lost_happen_at=%ld\n", 
			__func__, record_total_counter + record_error_counter, record_error_counter, 
			first_record_error_frame_counter, last_record_error_frame_counter);
#endif

#ifdef OUTPUT_ESLAPSE_DATA    
	eslapse = (unsigned long)((long)(end_time.tv_sec - init_time.tv_sec) * 1000000L + 
				((long)(end_time.tv_nsec - init_time.tv_nsec) / 1000));
	output_debug_log(true, "[%s]: recording total consumed time=%ld\n", __func__, eslapse);

	if(record_eslapse_check_counter > 0) {
		output_debug_log(true, "[%s]: recording eslapse_check result:%d\n", __func__, record_eslapse_check_counter);
		for(i = 0; i < MAX_TEST_COUNT; i++) {
			if(i < record_eslapse_check_counter) {
				output_debug_log(true, "[%d]:%05d.%ld:\t%ld\t%ld\n", i, 
					st_record_eslapse_check[i].time.tv_sec, 
					st_record_eslapse_check[i].time.tv_nsec, 
					st_record_eslapse_check[i].counter, 
					st_record_eslapse_check[i].eslapse);
			}
		}	
	}
#endif
	output_debug_log(true, "[%s]: end up SPI recording thread.\n", __func__);
    return err;
}

//set SPI playback status
u8 set_spi_play_status( u16 new_state )
{
    u8  err;
    u32 state;
    
    err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    state |= new_state;
    err = fm1388_spi_write(DSP_CMD_ADDR, state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }
 
    return err;
}

//send CMD to FM1388 to enable SPI playback 
u8 enable_spi_play( void )
{
    u8  err = ESUCCESS;
    u32 state;
    
    err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
	//output_debug_log(true, "[%s]: spi playback status= %04x\n", __func__, state);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    state = state | PLAY_ENABLE | RECORD_ENABLE;

	//output_debug_log(true, "[%s]: spi playback new status= %04x\n", __func__, state);
    err = fm1388_spi_write(DSP_CMD_ADDR, state, 2);
	//output_debug_log(true, "[%s]: after calling SPI, err= %x\n", __func__, err);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }

    return err;
}

//send CMD to FM1388 to disable SPI playback when playback stopped
u8 disable_spi_play( void )
{
    u8  err = ESUCCESS;
    u32 state;
    
    err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
	//output_debug_log(true, "[%s]: spi playback status= %04x\n", __func__, state);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }
    
    state &= PLAY_ENABLE_RESET;
    state &= RECORD_ENABLE_RESET; //for we set Record enable in spi_playback_data_ready(), so reset it when finished playback.
	
	output_debug_log(true, "[%s]: spi playback new status= %04x\n", __func__, state);
    err = fm1388_spi_write(DSP_CMD_ADDR, state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }

    return err;
}

//send CMD to FM1388 for checking SPI playback buffer is ready or not 
u8 is_spi_playback_buffer_ready( u8 buf_index, u8 is_recording )
{
    u8  err = ESUCCESS, i;
    u32 temp_counter;
#ifdef OUTPUT_READY_STATE_DATA 
    u32 state;
#endif
    u32 dsp_buf_index;
    u32	ready_state_array[BUFFER_NUMBER];
    u32 ready_state_all = 0;
	u32	buffer_ready_word_addr[BUFFER_NUMBER] = {
		PLAYBACK_SYNC_BUFFER0_ADDR,
		PLAYBACK_SYNC_BUFFER1_ADDR,
		PLAYBACK_SYNC_BUFFER2_ADDR,
		PLAYBACK_SYNC_BUFFER3_ADDR };

    u8	buf_ready_pattern[BUFFER_NUMBER][BUFFER_NUMBER] = {	{0xE, 0xC, 0x8, 0x0},
															{0xD, 0x9, 0x1, 0x0},
															{0xB, 0x3, 0x2, 0x0},
															{0x7, 0x6, 0x4, 0x0}
														};

	
    err = fm1388_spi_read( PLAYBACK_ERROR_COUNTER_ADDR, &temp_counter, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }

	for(i = 0; i < BUFFER_NUMBER; i++) {
		err = fm1388_spi_read( buffer_ready_word_addr[i], &(ready_state_array[i]), 2);
		if( err != 0 ){ 
			output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
			return err;
		}
	}
	ready_state_all = (u32)((ready_state_array[0] & 0x0001) | ((ready_state_array[1] & 0x0001) << 1) |
						((ready_state_array[2] & 0x0001) << 2) | ((ready_state_array[3] & 0x0001) << 3));

		
	if(playback_error_counter != temp_counter) {
		if(playback_error_check_counter == 0) first_playback_error_frame_counter = playback_total_counter;
		last_playback_error_frame_counter = playback_total_counter;
		
		playback_error_counter = temp_counter;
		if(playback_error_check_counter < MAX_TEST_COUNT) {
			fm1388_spi_read( DSP_PLAYBACK_BUFFER_INDEX_ADDR, &dsp_buf_index, 2);

			st_playback_error_check[playback_error_check_counter].counter 	= playback_total_counter;
			st_playback_error_check[playback_error_check_counter].index 	= buf_index;
			st_playback_error_check[playback_error_check_counter].dsp_index = dsp_buf_index;
			st_playback_error_check[playback_error_check_counter].state 	= ready_state_all;
			st_playback_error_check[playback_error_check_counter].error_counter = temp_counter;
			do_posix_clock_monotonic_gettime(&(st_playback_error_check[playback_error_check_counter].time));  
		}
		playback_error_check_counter++;
	}
	
#ifdef OUTPUT_READY_STATE_DATA 
	fm1388_spi_read( DSP_PLAYBACK_BUFFER_INDEX_ADDR, &dsp_buf_index, 2);
	if(playback_ready_check_counter < MAX_READY_CHECK) {
		err = fm1388_spi_read( DSP_CMD_ADDR, &state, 2);
		if( err != 0 ){ 
			output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
			return err;
		}
		
		st_playback_ready_check[playback_ready_check_counter].dsp_index 	= dsp_buf_index;
		st_playback_ready_check[playback_ready_check_counter].local_index 	= buf_index;
		st_playback_ready_check[playback_ready_check_counter].state 		= state;
		st_playback_ready_check[playback_ready_check_counter].ready_state 	= ready_state_all;
		st_playback_ready_check[playback_ready_check_counter].err_counter 	= temp_counter;
		st_playback_ready_check[playback_ready_check_counter].counter 		= playback_total_counter;
		do_posix_clock_monotonic_gettime(&(st_playback_ready_check[playback_ready_check_counter].time));  
		playback_ready_check_counter ++;
	}
#endif
	//output_debug_log(true, "[%s]: buffer index = %#x, ready_state=%04x, ready_state_all=%04x\n", 
	//	__func__, buf_index, ready_state, ready_state_all);

	if(is_recording == 1) {
		if(ready_state_array[buf_index] == 0) return 1;
		else return 0;
	}
	else {
		if(buf_ready_pattern[buf_index][3] == (ready_state_all | buf_ready_pattern[buf_index][3])) return 4;
		if(buf_ready_pattern[buf_index][2] == (ready_state_all | buf_ready_pattern[buf_index][2])) return 3;
		if(buf_ready_pattern[buf_index][1] == (ready_state_all | buf_ready_pattern[buf_index][1])) return 2;
		if(buf_ready_pattern[buf_index][0] == (ready_state_all | buf_ready_pattern[buf_index][0])) return 1;
		else return 0;
	}
}

//get FM1388 status, reset the playback ready bit, 
//then send new status to FM1388 to let it playback data. 
u8 spi_playback_data_ready( u8 buf_index )
{
    u8  err   = ESUCCESS;
    //u32 ready_state = 0;
	u32	buffer_ready_word_addr[BUFFER_NUMBER] = {
		PLAYBACK_SYNC_BUFFER0_ADDR,
		PLAYBACK_SYNC_BUFFER1_ADDR,
		PLAYBACK_SYNC_BUFFER2_ADDR,
		PLAYBACK_SYNC_BUFFER3_ADDR };
    /*
    err = fm1388_spi_read( buffer_ready_word_addr[buf_index], &ready_state, 2);
    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_read(). err = %x\n", __func__, err);
        return err;
    }

    ready_state = 1;
    */
		err = fm1388_spi_write(buffer_ready_word_addr[buf_index], 1, 2);

    if( err != 0 ){ 
		output_debug_log(true, "[%s]: error return from fm1388_spi_write(). err = %x\n", __func__, err);
        return err;
    }

    return err;
}

int get_wav_header(struct file* fpdata, wav_header* header) {
	int ret = ESUCCESS;
	
	if(fpdata == NULL) {
		output_debug_log(true, "[%s]: file pinter is invalid\n", __func__);
		return -EPARAMINVAL;
	}
	
	if(header == NULL) {
		output_debug_log(true, "[%s]: parameter--header is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	ret = fpdata->f_op->read(fpdata, (u8*)(header), sizeof(wav_header), &fpdata->f_pos);

	if ((ret <= 0) || (ret != sizeof(wav_header))) {
		output_debug_log(true, "[%s]: Error occurs when read wav file header.\n", __func__);
		return -EFILECANNOTREAD;
	}
	
	return ESUCCESS;
}

int fm1388_pull_data_from_fifo(u8* data_buffer, u16 data_size) {
	u32 kfifo_data_len = 0L;
	unsigned long timeout = jiffies + msecs_to_jiffies(2);
	u32 ret = 0L;

	if(data_buffer == NULL) return ret;
	
	spin_lock(&fm1388_data->play_lock);
	kfifo_data_len = kfifo_len(fm1388_data->play_kfifo);
	spin_unlock(&fm1388_data->play_lock);
	timeout = jiffies + msecs_to_jiffies(5);
	while ((kfifo_data_len < data_size) && time_before(jiffies, timeout)) {
		usleep_range(100, 200);
		spin_lock(&fm1388_data->play_lock);
		kfifo_data_len = kfifo_len(fm1388_data->play_kfifo);
		spin_unlock(&fm1388_data->play_lock);
	}

	if(kfifo_data_len < data_size) {
		output_debug_log(true, "[%s]: no enough data in fifo, may cause frame loss.\n", __func__);
	}
	else {
		ret = kfifo_out_spinlocked(fm1388_data->play_kfifo, data_buffer, data_size, &fm1388_data->play_lock);
	}

	return ret;
}


//collect sample data together for one channel
//then move the channel data to target buffer location
//at last, swap data per 8 bytes--it is required by SPI
u8 get_channel_data(u8* buffer, u32 frame_size, u8 channel_number, char* channel_mapping, u8* channel_data) {
	u16 *pDest, *pSource;
	u32 sr_num;
	u32 i, j;
	u8	 target_channel = 0;
		
	pSource = (u16 *)buffer;
	sr_num = frame_size >> 1;//bytes to word
	for (i = 0; i < channel_number; i++) {
		if((channel_mapping[i] != 0) && (channel_mapping[i] != '-')) {
			target_channel = channel_mapping[i] - '0';
			pDest = (u16 *)(channel_data + target_channel * frame_size);
			for (j = 0; j < sr_num; j++) {
				*pDest++ = *(pSource + i + j * (channel_number));
			}

			//convert data according to SPI requirements
			//swap_spi_data(channel_data + target_channel * frame_size, frame_size);
		}
	}

	return 0;
}

//read wav file to get voice data, data size is frame_size * channel_num
//then collect sample data together for one channel
//then move the channel data to target buffer location
//at last, swap data per 8 bytes--it is required by SPI
int get_one_data_block(int channel_num, int frame_size) {
	u32 read_len = 0;
	u32 blocksize;

	if(channel_num <= 0) {
		output_debug_log(true, "[%s]: channel_num is invalid. channel_num=%d\n", __func__, channel_num);
		return -EPARAMINVAL;
	}
	
	if(frame_size <= 0) {
		output_debug_log(true, "[%s]: frame_size is invalid. frame_size=%x\n", __func__, frame_size);
		return -EPARAMINVAL;
	}
	
	blocksize = frame_size * channel_num;
	read_len = fm1388_pull_data_from_fifo(voice_buffer_play, blocksize);
	if (read_len < blocksize) {
		output_debug_log(true, "[%s]: can not get enough data, total_len=%x, blocksize=%x\n", __func__, read_len, blocksize);
		return -ENOENOUGHDATA;
	}

	return ESUCCESS;
}

int push_one_frame_to_dsp(u32 spi_base_addr) {
    int  err 			= 0;
	u8*  start_addr		= NULL;
	u32  offset			= 0;
	u8   channel_idx 	= 0;
	u8 	 target_channel = 0;
	
	memset(voice_buffer_play, 0, FM1388_BUFFER_LEN * sizeof(u8));
	start_addr = voice_buffer_play;
	offset = 0;

	//get one block data from wav file and convert data by channel for playback
	err = get_one_data_block(source_file_channel_number, playback_param.framesize);
	if(err != ESUCCESS) {
		output_debug_log(true, "[%s]: Error occurs when read data block for playback\n", __func__);
		return err;
	}

	for(channel_idx = 0; channel_idx < source_file_channel_number; channel_idx ++) {
		if((playback_param.channel_mapping[channel_idx] == 0) || (playback_param.channel_mapping[channel_idx] == '-')) {
			continue;
		}

		target_channel = playback_param.channel_mapping[channel_idx] - '0';
		offset = playback_param.framesize * target_channel;
		err = fm1388_spi_burst_write(spi_base_addr + offset,  
									start_addr + offset,  
									playback_param.framesize );
		if( err != ESUCCESS ) { 
			output_debug_log(true, "[%s]: fm1388_spi_burst_read return error = %#x\n", __func__, err);
			break;
		}
	}

	return err;
}

void output_playback_debug_info( void ) {
	int i = 0;
#ifdef OUTPUT_READY_STATE_DATA    
	u32 eslapse = 0L;
#endif
	if(st_playback_error_check[0].counter == 4) {
		playback_error_counter -= st_playback_error_check[0].error_counter;
		output_debug_log(true, "[%s]: playback is not sync at the beginning.\n", __func__);
	}

	if(playback_error_check_counter > 0) {
		output_debug_log(true, "[%s]: playback loss frame happend around below frame counter:%d\n", 
							__func__, playback_error_check_counter);
		for(i = 0; i < MAX_TEST_COUNT; i++) {
			if(i < playback_error_check_counter) {
				output_debug_log(true, "[%d]:%05d.%ld:\t%ld\t%ld\t%d\t%d\t%04x\n", i, 
					st_playback_error_check[i].time.tv_sec, 
					st_playback_error_check[i].time.tv_nsec, 
					(i > 0) ? (st_playback_error_check[i].counter + st_playback_error_check[i - 1].error_counter) : st_playback_error_check[i].counter,
					st_playback_error_check[i].error_counter,
					st_playback_error_check[i].index,
					st_playback_error_check[i].dsp_index,
					st_playback_error_check[i].state);
			}
		}	
	}

#ifdef OUTPUT_READY_STATE_DATA    
	output_debug_log(false, "[%s]: playback ready check result:\n", __func__);
	if(playback_ready_check_counter > 0) {
		for(i = 0; i < MAX_READY_CHECK; i++) {
			if(i < playback_ready_check_counter) {
				eslapse = 0;
				if(i >= 1) {
					eslapse = (unsigned long)((long)(st_playback_ready_check[i].time.tv_sec - st_playback_ready_check[i - 1].time.tv_sec) * 1000000000L + 
						((long)(st_playback_ready_check[i].time.tv_nsec - st_playback_ready_check[i - 1].time.tv_nsec)));
				}
				
				output_debug_log(false, "[%d]:%05d.%ld:\t%09ld\t%02d\t%02d\t%04x\t%04x\t%09d\t%09d\n", i, 
					st_playback_ready_check[i].time.tv_sec, 
					st_playback_ready_check[i].time.tv_nsec, 
					eslapse, 
					st_playback_ready_check[i].local_index, 
					st_playback_ready_check[i].dsp_index, 
					st_playback_ready_check[i].state,
					st_playback_ready_check[i].ready_state,
					st_playback_ready_check[i].err_counter,
					st_playback_ready_check[i].counter);
			}
		}	
	}
#endif
}


int fm1388_read_audio_data_from_file(void* data) {
	unsigned long timeout = jiffies + msecs_to_jiffies(2);
	struct file *fpdata = NULL;
	wav_header header;
	int	 data_size 		= 0;
	int  fifo_avail		= 0;
    int  err 			= 0;
	u16  ret 			= 0;
	u32	 total_gotten 	= 0L;
	
#ifdef OUTPUT_ESLAPSE_DATA    
	struct timespec start_time, end_time;  
	u32	 eslapse		= 0L;
	u32  max_eslapse 	= 0L;
#endif

	output_debug_log(true, "[%s]: start get data thread.\n", __func__);
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	fpdata 	= filp_open(playback_param.file_path, O_RDONLY, 0);
	if (IS_ERR(fpdata)) {
		output_debug_log(true, "[%s]: the file or path does not exist. %s\n", __func__, playback_param.file_path);
		err = -EFAILOPEN;
		goto READ_ERR;
	}
	
	err = get_wav_header(fpdata, &header);
	if(err != ESUCCESS) {
		output_debug_log(true, "[%s]: fail to read wav file header from %s\n", __func__, playback_param.file_path);
		err = -EFILECANNOTREAD;
		goto READ_ERR;
	}
	
	output_debug_log(true, "[%s]: header.num_channels = %d, header.sample_rate=%d, header.data_sz:%#x\n", 
		__func__, header.num_channels, header.sample_rate, header.data_sz);
		
	data_size = header.num_channels * playback_param.framesize;
	
	//global variables
	source_file_channel_number = header.num_channels;
	source_file_total_size = header.data_sz;

	output_debug_log(true, "%s: header.num_channels=%d, playback_param.framesize=%d, channel_mapping=%s\n", 
				__func__, header.num_channels, playback_param.framesize, playback_param.channel_mapping);
	
    while (!kthread_should_stop() && ((total_gotten + data_size) <= header.data_sz)) {
#ifdef OUTPUT_ESLAPSE_DATA    
		do_posix_clock_monotonic_gettime(&start_time);  
#endif
		spin_lock(&fm1388_data->play_lock);
		fifo_avail = kfifo_avail(fm1388_data->play_kfifo);
		spin_unlock(&fm1388_data->play_lock);
		timeout = jiffies + msecs_to_jiffies(20);
		while ((fifo_avail < data_size) && time_before(jiffies, timeout)) {
			usleep_range(200, 300);		//reduce the sleep time causes time reduce from fetch end to next fetch
			spin_lock(&fm1388_data->play_lock);
			fifo_avail = kfifo_avail(fm1388_data->play_kfifo);
			spin_unlock(&fm1388_data->play_lock);
		}
		
		if (fifo_avail < data_size) {
			usleep_range(300, 400);
			//output_debug_log(true, "[%s]: will lost one frame data for no enough space in fifo.\n", __func__);
		}
		else {
			if(!IS_ERR(fpdata)) {
				fpdata->f_op->read(fpdata, (u8*)(temp_buffer1), data_size, &fpdata->f_pos);
				
				//re-arrange data by channel
				memset(temp_buffer2, 0, FM1388_BUFFER_LEN);
				get_channel_data(temp_buffer1, playback_param.framesize, header.num_channels, 
									playback_param.channel_mapping, temp_buffer2);
				
				ret = kfifo_in_spinlocked(fm1388_data->play_kfifo, temp_buffer2, data_size, &fm1388_data->play_lock);

				if(ret != data_size) {
					output_debug_log(true, "[%s]: got data is not enough, required:%d, gotten:%d.\n", __func__, data_size, ret);
				}
				
				total_gotten += ret;
				
#ifdef OUTPUT_ESLAPSE_DATA    
				do_posix_clock_monotonic_gettime(&end_time);  
				eslapse = (unsigned long)((long)(end_time.tv_sec - start_time.tv_sec) * 1000000000L + 
						(long)(end_time.tv_nsec - start_time.tv_nsec));
								
				if(eslapse > max_eslapse) max_eslapse = eslapse;
#endif
			}
			else {
				output_debug_log(true, "[%s]: open data file failed.\n", __func__);
			}
		}
		
		usleep_range(100, 200);
	} 

READ_ERR:
	if(!IS_ERR(fpdata)) {
		filp_close(fpdata, NULL);
	}
	set_fs(oldfs);
	fm1388_data->load_data_thread_id = NULL;

	output_debug_log(true, "[%s]: totally read data %d.\n", __func__, total_gotten);
#ifdef OUTPUT_ESLAPSE_DATA    
	output_debug_log(true, "[%s]: max. eslapse:%d\n", __func__, max_eslapse);
#endif
	output_debug_log(true, "[%s]: exit get data thread.\n", __func__);
	return ESUCCESS;
}

	
int fm1388_trans_voice_data_by_channel_4buffers(void *data)
{
	u32  spi_base_addr;
	u8   channel_idx 	= 0;
	int  ready_num 		= 0;
    int  err 			= 0;
    int	 i				= 0;
	u32	 total_len 		= 0L;
	u32  block_size 	= 0L;
	u16	 cur_play_buffer= 0;

#ifdef OUTPUT_ESLAPSE_DATA    
	struct timespec start_time, end_time, init_time;  
	u32	 eslapse		= 0L;
	u32  max_eslapse 	= 0L;
#endif
	
	//for recording
	u8*  rec_start_addr;
	u32  rec_spi_base_addr;
	u8   rec_ready_num	= 0;
	u16	 rec_data_size 	= 0;
	u16	 cur_rec_buffer	= 0;
	u32  total_put 		= 0L, ret = 0L;
	char channel_need_rec[DSP_SPI_REC_CH_NUM];



	rec_data_size  = playback_param.rec_ch_num * playback_param.framesize;
	rec_start_addr = record_spi_buffer;

	init_global_variables();
	
	output_debug_log(true, "[%s]: start SPI playback thread.\n", __func__);
	err = fm1388_spi_change_maxspeed(SPI_BURST_READ_SPEED);
	if(err) {
		output_debug_log(true, "[%s]: fm1388_spi_change_maxspeed return value = %d\n", __func__, err);
		goto PLAY_ERR;
	}

	i = 0;
		while((i < 5) && ((source_file_channel_number <= 0) || (source_file_total_size <= 0))) {
			msleep(100);
			i++;
		}
		if(i >= 5) {
			output_debug_log(true, "[%s]: source file data is not prepared.\n", __func__);
			goto PLAY_ERR;
		}


	if(playback_param.need_recording == 1) {
		memset(channel_need_rec, 0, sizeof(char) * DSP_SPI_REC_CH_NUM);
		for(channel_idx = 0; (channel_idx < DSP_SPI_REC_CH_NUM); channel_idx ++) {
			if(!valid_channel(playback_param.rec_ch_num, playback_param.rec_ch_idx, channel_idx)) {
				continue;
			}
			channel_need_rec[i] = channel_idx;
			i++;
		}
	}

	//output_debug_log(true, "[%s]: header.num_channels = %d, header.sample_rate=%d, header.data_sz:%#x\n", 
	//	__func__, header.num_channels, header.sample_rate, header.data_sz);
		
	//pre-load 4 buffer data, thus Host has 4 frames' time ahead DSP
	total_len = 0L;
	block_size = source_file_channel_number * playback_param.framesize;
		for(cur_play_buffer = 0; (cur_play_buffer < 4) && ((total_len + block_size) < source_file_total_size); cur_play_buffer++) {
			spi_base_addr = playback_param.addr_input0 + cur_play_buffer * DSP_OUTPUT_BUFFER_SIZE;;
			
			err = push_one_frame_to_dsp(spi_base_addr);
			if(err != ESUCCESS) {
				output_debug_log(true, "[%s]: Error occurs when read data block for playback at %d\n", __func__, total_len);

			break;
		}
		
		total_len += block_size;
	}
	
	if(err != ESUCCESS) {
		output_debug_log(true, "[%s]: fail to preload data. ret=%d\n", __func__, err);
		goto PLAY_ERR;
	}
	
	//enable spi playback
	err = enable_spi_play(); 
	if (err) {
		output_debug_log(true, "[%s]: fail to enable playback. ret=%d\n", __func__, err);
		err = -ECANNOTENPLAY;
		goto PLAY_ERR;
	}
			
	//set 4 buffer data ready
	for(cur_play_buffer = 0; cur_play_buffer < 4; cur_play_buffer++) {
		spi_playback_data_ready( cur_play_buffer );
	}

	cur_play_buffer = 0;
	cur_rec_buffer = 0;
	playback_total_counter = 4;
			
	output_debug_log(true, "[%s]: block_size:%#x will read %d times.\n", __func__, block_size, source_file_total_size / block_size);

#ifdef OUTPUT_ESLAPSE_DATA    
	do_posix_clock_monotonic_gettime(&init_time);  
#endif
	while (!kthread_should_stop() && ((total_len + block_size) <= source_file_total_size)) {	
#ifdef OUTPUT_ESLAPSE_DATA    
		do_posix_clock_monotonic_gettime(&start_time);  
#endif
		if(playback_param.need_recording == 1) {
			rec_ready_num = is_spi_rec_buf_ready(cur_rec_buffer);
			//output_debug_log(true, "[%s]: is_spi_rec_ready return value = %#x\n", __func__, rec_ready_num);
			if(rec_ready_num > 0) {
				rec_spi_base_addr = playback_param.addr_output0 + cur_rec_buffer * DSP_OUTPUT_BUFFER_SIZE;
				
				err = fetch_one_frame_voice_data(rec_spi_base_addr, rec_start_addr, channel_need_rec);
				if(err != ESUCCESS) break;

				err = spi_record_buf_finish(cur_rec_buffer);

				ret = fm1388_push_data_to_fifo(rec_start_addr, rec_data_size);	
				total_put += ret;
		

			
				record_total_counter ++;
				cur_rec_buffer = (cur_rec_buffer + 1) & 0x03;
			}
		}

		ready_num = is_spi_playback_buffer_ready(cur_play_buffer, playback_param.need_recording);
		//output_debug_log(true, "[%s]: is_spi_playback_buffer_ready return value = %#x, read:%#x\n", __func__, is_ready, total_len);
		if(ready_num > 0) {
			do {
				spi_base_addr = playback_param.addr_input0 + cur_play_buffer * DSP_OUTPUT_BUFFER_SIZE;;
				
				err = push_one_frame_to_dsp(spi_base_addr);
				if(err != ESUCCESS) {
					output_debug_log(true, "[%s]: Error occurs when read data block for playback, data read:%#x, total:%#x\n", 
						__func__, total_len, source_file_total_size);
					break;
				}

				
				err = spi_playback_data_ready( cur_play_buffer );
				
				total_len += block_size;
				playback_total_counter++;
				cur_play_buffer = (cur_play_buffer + 1) & 0x3;
				
				ready_num --;
			
			} while ((ready_num > 0) && ((total_len + block_size) <= source_file_total_size));
			
			if(err != ESUCCESS) break;
		}
		else {
			usleep_range(100, 200);
		}
#ifdef OUTPUT_ESLAPSE_DATA    
		do_posix_clock_monotonic_gettime(&end_time);  
		eslapse = (unsigned long)((long)(end_time.tv_sec - start_time.tv_sec) * 1000000000L + 
				(long)(end_time.tv_nsec - start_time.tv_nsec));
						
		if(eslapse > max_eslapse) max_eslapse = eslapse;
		if(eslapse > 20000000L) {
			if(playback_eslapse_check_counter < MAX_TEST_COUNT) {
				st_playback_eslapse_check[playback_eslapse_check_counter].counter = playback_total_counter;
				st_playback_eslapse_check[playback_eslapse_check_counter].eslapse = eslapse;
				do_posix_clock_monotonic_gettime(&(st_playback_eslapse_check[playback_eslapse_check_counter].time));  
				playback_eslapse_check_counter++;
			}
		}
#endif
	}
	
    err = fm1388_spi_read( PLAYBACK_ERROR_COUNTER_ADDR, &playback_error_counter, 2);
    err = fm1388_spi_read( RECORD_ERROR_COUNTER_ADDR, &record_error_counter, 2);
	
	//disable playback in DSP
	disable_spi_play(); //already disabled recording too

PLAY_ERR:	
	fm1388_data->playback_thread_id = NULL;
	if(err != ESUCCESS) {
		output_debug_log(true, "[%s]: playback thread exit abnormally.\n", __func__);
		return err;
	}


	output_playback_debug_info();
#ifdef OUTPUT_ESLAPSE_DATA    
	output_debug_log(true, "[%s]: playback %d frames. loss %d frames, max_eslapse=%ld, eslapse_check_counter=%ld, \
	first_lost_happen_at=%ld, last_lost_happen_at=%ld\n", 
			__func__, playback_total_counter + playback_error_counter, playback_error_counter, max_eslapse, 
			playback_eslapse_check_counter, first_playback_error_frame_counter, last_playback_error_frame_counter);
#else
	output_debug_log(true, "[%s]: playback %d frames. loss %d frames, first_lost_happen_at=%ld, last_lost_happen_at=%ld\n", 
			__func__, playback_total_counter + playback_error_counter, playback_error_counter,  
			first_playback_error_frame_counter, last_playback_error_frame_counter);
#endif

	if(playback_param.need_recording == 1) {
		output_record_debug_info();
#ifdef OUTPUT_ESLAPSE_DATA    
		output_debug_log(true, "[%s]: record %d frames. loss %d frames, max_eslapse=%ld, eslapse_check_counter=%ld, \
		first_lost_happen_at=%ld, last_lost_happen_at=%ld\n", 
				__func__, record_total_counter, record_error_counter, max_eslapse, 
				record_eslapse_check_counter, first_record_error_frame_counter, last_record_error_frame_counter);
#else
		output_debug_log(true, "[%s]: record %d frames. loss %d frames, first_lost_happen_at=%ld, last_lost_happen_at=%ld\n", 
				__func__, record_total_counter, record_error_counter, 
				first_record_error_frame_counter, last_record_error_frame_counter);
#endif
	}
	
#ifdef OUTPUT_ESLAPSE_DATA    
	eslapse = (unsigned long)((long)(end_time.tv_sec - init_time.tv_sec) * 1000000L + 
				((long)(end_time.tv_nsec - init_time.tv_nsec) / 1000));
	output_debug_log(true, "[%s]: total consumed time=%ld\n", __func__, eslapse);
	
	if(playback_eslapse_check_counter > 0) {
		output_debug_log(true, "[%s]: eslapse_check result:%d\n", __func__, playback_eslapse_check_counter);
		for(i = 0; i < MAX_TEST_COUNT; i++) {
			if(i < playback_eslapse_check_counter) {
				output_debug_log(true, "[%d]:%05d.%ld:\t%ld\t%ld\n", i, 
					st_playback_eslapse_check[i].time.tv_sec, 
					st_playback_eslapse_check[i].time.tv_nsec, 
					st_playback_eslapse_check[i].counter, 
					st_playback_eslapse_check[i].eslapse);
			}
		}	
	}
#endif

	output_debug_log(true, "[%s]: end up SPI playback thread.\n", __func__);

    return err;
}



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
                               fw->size);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_50000000.dat over.\n", __func__);
    request_firmware(&fw, "FM1388_5FFC0000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_5FFC0000.dat.\n", __func__);

        fm1388_spi_burst_write(0x5ffc0000, fw->data,
                               fw->size);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_5FFC0000.dat over.\n", __func__);
    request_firmware(&fw, "FM1388_5FFE0000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_5FFE0000.dat.\n", __func__);

        fm1388_spi_burst_write(0x5ffe0000, fw->data,
                               fw->size);

        release_firmware(fw);
        fw = NULL;
    }
    //lidbg(TAG"%s: FM1388_5FFE0000.dat over.\n", __func__);
    request_firmware(&fw, "FM1388_60000000.dat", &fm1388_pdev->dev);
    if (fw)
    {
        lidbg(TAG"%s: firmware FM1388_60000000.dat.\n", __func__);

        fm1388_spi_burst_write(0x60000000, fw->data,
                               fw->size);

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

void strip_white_space(char* source_string, int offset, char* target_string, int len) {
	int i = 0, j = 0;
	
	if((source_string == NULL) || (target_string == NULL) || (offset < 0) || (len <= 0)) return;
	
	i = offset;
	while((source_string[i] != 0) && (i < CONFIG_LINE_LEN) && (j < len)){
		if((source_string[i] != ' ') && (source_string[i] != '\t') && (source_string[i] != '\r') && (source_string[i] != '\n')) {
			target_string[j++] = source_string[i];
		}
		
		i++;
	}
	
	target_string[j] = 0;
	return;
}

// load user-defined path setting file, if file does not exist use default setting
int load_user_setting(void)
{
    struct file*	fp;
    int 			word_count = 0;
    char 			s[CONFIG_LINE_LEN];	//assume each line of the opened file is below 255 characters
	char			strTemp[MAX_PATH_LEN];

	static int is_init = false;
	if(is_init)
		return 0;
	is_init = true;
	
	//set default path to SDcard and VEC path, then parse config file to get user-defined path to replace these path
	strncpy(SDCARD_PATH, DEFAULT_SDCARD_PATH, MAX_PATH_LEN);
	//strncpy(filepath, default_filepath, CONFIG_LINE_LEN);
	b_output_log = false;
	
    combine_path_name(filepath_name, USER_DEFINED_PATH_FILE);
	pr_err("%s: load user-defined path file %s\n", __func__, filepath_name);
	
    initKernelEnv();
    fp = openFile(filepath_name, O_RDONLY, 0);

    if (fp == NULL) {
       pr_err ("%s, File %s could not be opened or does not exist, will use default setting for sdcard and vec location.\n", __func__, filepath_name);
       set_fs(oldfs);
       return -EFAILOPEN;
    }
    else{
       pr_err ("%s, File %s opened!...\n", __func__, filepath_name);
	   while (fgets(s, CONFIG_LINE_LEN, fp, &word_count) != NULL) {
			if(s[0] == '*' || s[0] == '#' || s[0] == '/' || s[0] == 0xD || s[0] == 0x0) {
				continue;
			} 
			else {
				if(strncasecmp(s, USER_VEC_PATH, strlen(USER_VEC_PATH)) == 0) {
					strip_white_space(s, strlen(USER_VEC_PATH), filepath, CONFIG_LINE_LEN);
					pr_err("%s: vec file path is %s\n", __func__, filepath);
				}
				else if(strncasecmp(s, USER_KERNEL_SDCARD_PATH, strlen(USER_KERNEL_SDCARD_PATH)) == 0) {
					strip_white_space(s, strlen(USER_KERNEL_SDCARD_PATH), SDCARD_PATH, MAX_PATH_LEN);
					pr_err("%s: sdcard path is %s\n", __func__, SDCARD_PATH);
				}
				else if(strncasecmp(s, USER_USER_SDCARD_PATH, strlen(USER_USER_SDCARD_PATH)) == 0) {
					//not need parse user space sd card path
				}
				else if(strncasecmp(s, USER_OUTPUT_LOG, strlen(USER_OUTPUT_LOG)) == 0) {
					strip_white_space(s, strlen(USER_OUTPUT_LOG), strTemp, MAX_PATH_LEN);
					b_output_log = (strTemp[0] == 'y' || strTemp[0] == 'Y') ? true : false;
					output_debug_log(false, "[%s]: output_log is %s\n", __func__, b_output_log ? "yes" : "no");
				}
				else {
					continue;
				}
			}
	   }

       closeFile(fp);
       set_fs(oldfs);
    }
    
	output_debug_log(true, "Finally used path setting is:\n");
	output_debug_log(true, "\tsdcard=%s\n", SDCARD_PATH);
	output_debug_log(true, "\tcfg_path=%s\n", filepath);
	output_debug_log(true, "\toutput_log=%d\n", b_output_log);
	
    return ESUCCESS;
}



// load codec init VEC, access HW register thru short address
int load_fm1388_init_vec(char *file_src)
{
    struct file *fp;
    int word_count = 0;
    char s[CONFIG_LINE_LEN];	//assume each line of the opened file is below 255 characters
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
        while (fgets(s, CONFIG_LINE_LEN, fp, &word_count) != NULL)
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
    char s[CONFIG_LINE_LEN];	//assume each line of the opened file is below 255 characters
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
        while (fgets(s, CONFIG_LINE_LEN, fp, &word_count) != NULL)
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
	u32 addr;
	u8  counter = 0;

    if(isLock)
        	return -1;

    mutex_lock(&fm1388_init_lock);
    isLock = true;

	load_user_setting();
	
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

		//Fuli 20160902 follow SAB's behavior to check dsp status
		counter = 0;
		do {
			addr = DSP_STATUS_ADDR;
			fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
			counter ++;
			msleep(100);
		} while((counter < 10) && (val != DSP_INITIALIZED));
		
		if(counter >= 10) {
			lidbg(TAG"%s: [0x%x] = 0x%x, DSP is not initialized successfully.\n", __func__, addr, val);
		}


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
		if(g_var.acc_flag == FLY_ACC_OFF)
			break;
		msleep(500);
	}
	
    mutex_unlock(&fm1388_init_lock);
    isLock = false;
    return 0;
}

static void fm1388_spi_lock(struct mutex *lock)
{
	mutex_lock(lock);
}

static void fm1388_spi_unlock(struct mutex *lock)
{
	mutex_unlock(lock);
}

/* Access to the audio buffer is controlled through "audio_owner". Either the
 * character device or the ALSA-capture device can be opened. */
static int fm1388_record_open(struct inode *inode, struct file *file)
{
	lidbg(TAG"%s: entering...\n", __func__);
	if(!fm1388_dsp_working) {
		lidbg(TAG"%s: Sorry, DSP does not work normally now, please try to reboot your system or contact relative person.\n", __func__);
		return -EDSPNOTWORK;
	}
	
	if (!atomic_add_unless(&fm1388_data->audio_owner, 1, 1))
		return -EBUSY;
	lidbg(TAG"%s: fm1388_data->audio_owner.counter = %d\n", __func__, fm1388_data->audio_owner.counter);

	file->private_data = fm1388_data;

	fm1388_spi_lock(&fm1388_data->lock);
	fm1388_data->buffering = 1;
	fm1388_spi_unlock(&fm1388_data->lock);

	return ESUCCESS;
}

static int fm1388_record_release(struct inode *inode, struct file *file)
{
	lidbg(TAG"%s: entering...\n", __func__);
	fm1388_spi_lock(&fm1388_data->lock);
	fm1388_data->buffering = 0;
	fm1388_spi_unlock(&fm1388_data->lock);

	lidbg(TAG"%s: decrease audio_owner.\n", __func__);
	atomic_dec(&fm1388_data->audio_owner);
	lidbg(TAG"%s: fm1388_data->audio_owner.counter = %d\n", __func__, fm1388_data->audio_owner.counter);

	return 0;
}

/* The write function is a hack to load the A-model on systems where the
 * firmware files are not accesible to the user. */
static ssize_t fm1388_record_write(struct file *file,
								  const char __user *buf,
								  size_t count_want,
								  loff_t *f_pos)
{
	lidbg(TAG"%s: entering...\n", __func__);
	if(!fm1388_dsp_working) {
		lidbg(TAG"%s: Sorry, DSP does not work normally now, please try to reboot your system or contact relative person.\n", __func__);
		return -EDSPNOTWORK;
	}
	
	return count_want;
}

static ssize_t fm1388_record_read(struct file *file,
				 char __user *buf, size_t count_want, loff_t *f_pos)
{
	return 0;
}

static const struct file_operations record_fops = {
	.owner   = THIS_MODULE,
	.open    = fm1388_record_open,
	.release = fm1388_record_release,
	.read    = fm1388_record_read,
	.write   = fm1388_record_write,
};

static int fm1388_create_cdev(struct platform_device *pdev)
{
    int 			ret = 0, err = -1;
    struct device*	dev = &pdev->dev;
    int 			cdev_major, dev_no;

	lidbg(TAG"%s: entering...\n", __func__);
	atomic_set(&fm1388_data->audio_owner, 0);
	lidbg(TAG"%s: fm1388_data->audio_owner.counter = %d\n", __func__, fm1388_data->audio_owner.counter);
	 
	ret = alloc_chrdev_region(&fm1388_data->record_chrdev, 0, 1, FM1388_CDEV_NAME);
	if (ret) {
		dev_err(dev, "%s: failed to allocate character device\n", __func__);
		return ret;
	}
	lidbg(TAG"%s: alloc_chrdev_region ok...\n", __func__);

	cdev_major = MAJOR(fm1388_data->record_chrdev);
	lidbg(TAG"%s: char dev major = %d", __func__, cdev_major);
		
	fm1388_data->cdev_class = class_create(THIS_MODULE, FM1388_CDEV_NAME);
	if (IS_ERR(fm1388_data->cdev_class)) {
		dev_err(dev, "%s: failed to create class\n", __func__);
		return err;
	}
	lidbg(TAG"%s: cdev_class create ok...\n", __func__);

	dev_no = MKDEV(cdev_major, 1);
	
	cdev_init(&fm1388_data->record_cdev, &record_fops);
	lidbg(TAG"%s: cdev_init ok...\n", __func__);

	fm1388_data->record_cdev.owner = THIS_MODULE;

	ret = cdev_add(&fm1388_data->record_cdev, dev_no, 1);
	if (ret) {
		dev_err(dev, "%s: failed to add character device\n", __func__);
		return ret;
	}
	lidbg(TAG"%s: cdev_add ok...\n", __func__);

	fm1388_data->record_dev = device_create(fm1388_data->cdev_class, NULL,
					 dev_no, NULL, "fm1388_smp%d", 1);
	if (IS_ERR(fm1388_data->record_dev)) {
		lidbg("%s: could not create device\n", __func__);
		//dev_err(&fm1388_i2c->dev, "%s: could not create device\n", __func__);
		return -ENOTOPEN;
	}
 	lidbg(TAG"%s: device_create ok...\n", __func__);
    return ret; 
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

    output_debug_log(false, "[%s]: fm1388_reg_show\n", __func__);
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

    output_debug_log(false, "[%s]: fm1388_reg_store\n", __func__);
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
        output_debug_log(true, "[%s]: 0x%02x = 0x%04x\n", __func__, addr, val);
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

    output_debug_log(false, "[%s]: fm1388_index_show\n", __func__);
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

    output_debug_log(false, "[%s]: fm1388_index_store\n", __func__);
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
        output_debug_log(true, "[%s]: 0x%02x = 0x%04x\n", addr,
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
    size_t ret;
    char *local_buffer;
    dev_cmd_mode_gs *get_mode_ret_data;
    dev_cmd_reg_rw  *get_reg_ret_data;
    dev_cmd_short    *get_addr_ret_data;

    /*
    	if (*offset > 0)
    		return 0;
    */

	if(!fm1388_dsp_working) {
		lidbg(TAG"%s: Sorry, DSP does not work normally now, please try to reboot your system or contact relative person.\n", __func__);
		output_debug_log(true, "[%s]: Sorry, DSP does not work normally now, please try to reboot your system or contact relative person.\n", __func__);
		return -EDSPNOTWORK;
	}

    local_buffer = (char *)kmalloc(length * sizeof(char), GFP_KERNEL);
    if (!local_buffer)
    {
        lidbg(TAG"%s: local_buffer allocation failure.\n", __func__);
		output_debug_log(true, "[%s]: local_buffer allocation failure.\n", __func__);
        goto out;
    }
    if(copy_from_user(local_buffer, buffer, length))
    {
        printk("copy_from_user ERR\n");
		output_debug_log(true, "[%s]: failed to copy data from user space.\n", __func__);
		goto out;
    }
    //lidbg(TAG"local_buffer = %d:, length = %d\n", local_buffer[0], length);

    switch(local_buffer[0])
    {
    case FM_SMVD_REG_READ:
        get_reg_ret_data = (dev_cmd_reg_rw *)local_buffer;
        fm1388_read(get_reg_ret_data->reg_addr, &get_reg_ret_data->reg_val);
        ret = sizeof(dev_cmd_reg_rw);
		output_debug_log(true, "[%s]: read reg(%02x), return value(%x).\n", __func__, get_reg_ret_data->reg_addr, get_reg_ret_data->reg_val);
        //lidbg(TAG"get_reg_ret_data->reg_addr = %d:, get_reg_ret_data->reg_val = 0x%4x, ret=%d\n", get_reg_ret_data->reg_addr, get_reg_ret_data->reg_val, ret);
        break;
    case FM_SMVD_DSP_ADDR_READ:
        get_addr_ret_data = (dev_cmd_short *)local_buffer;
        fm1388_dsp_mode_i2c_read_addr_2(get_addr_ret_data->addr, &get_addr_ret_data->val);
        ret = sizeof(dev_cmd_short);
		output_debug_log(true, "[%s]: read memory(%08x), return value(%x).\n", __func__, get_addr_ret_data->addr, get_addr_ret_data->val);
        //lidbg(TAG"get_addr_ret_data->addr = %d:, get_addr_ret_data->val = 0x%4x, ret=%d\n", get_addr_ret_data->addr, get_addr_ret_data->val, ret);
        break;
	case FM_SMVD_DSP_ADDR_READ_SPI:
		get_addr_ret_data = (dev_cmd_short*)local_buffer;
		/*replace parameter read/write by I2C
		if(get_addr_ret_data->addr % 4 == 0) {
			fm1388_spi_read(get_addr_ret_data->addr, &get_addr_ret_data->val, 4);
		}
		else if(get_addr_ret_data->addr % 4 == 2) {
			fm1388_spi_read(get_addr_ret_data->addr, &get_addr_ret_data->val, 2);
		}
		else {
			fm1388_spi_read(get_addr_ret_data->addr - 1, &get_addr_ret_data->val, 2);
		}
		output_debug_log(true, "[%s]: read memory(%08x), return value(%x) by SPI.\n", __func__, get_addr_ret_data->addr, get_addr_ret_data->val);
		*/
		fm1388_dsp_mode_i2c_read_addr_2(get_addr_ret_data->addr, &get_addr_ret_data->val);
		output_debug_log(true, "[%s]: read memory(%08x), return value(%x) by I2C.\n", __func__, get_addr_ret_data->addr, get_addr_ret_data->val);
		ret = sizeof(dev_cmd_short);
		break;
    case FM_SMVD_MODE_GET:
        //        lidbg(TAG"local_buffer = %d:, length = %d, ret = %d\n", local_buffer[0], length, ret);
        get_mode_ret_data = (dev_cmd_mode_gs *)local_buffer;
        get_mode_ret_data->dsp_mode = (char)fm1388_dsp_mode;
		output_debug_log(true, "[%s]: get mode return=%x.\n", __func__, get_mode_ret_data->dsp_mode);
        ret = sizeof(dev_cmd_mode_gs);
        //        lidbg(TAG"local_buffer = %d:, length = %d, ret = %d\n", local_buffer[0], length, ret);
        //        lidbg(TAG"local_buffer = %d:, length = %d, return_data->dsp_mode = %d\n", local_buffer[0], length, return_data->dsp_mode);
        break;

	case FM_SMVD_DSP_IS_PLAYING:
		get_addr_ret_data = (dev_cmd_short*)local_buffer;
		if(fm1388_data->playback_thread_id) {
			get_addr_ret_data->val = 1;
		}
		else {
			get_addr_ret_data->val = 0;
		}
	
		if(play_status != get_addr_ret_data->val) {
			play_status = get_addr_ret_data->val;
			play_status_counter = 0;
		}
		else play_status_counter++;
	
		if(play_status_counter % 10 == 0)
			output_debug_log(true, "[%s]: Check playing status. %d\n", __func__, get_addr_ret_data->val);
		ret = sizeof(dev_cmd_short);
		break;

    default:
        ret = -ECOMMANDINVAL;
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
    char mode[30];

    lidbg(TAG"%s: entering...\n", __func__);

    if(length <= 0)
	    return 0;

    if((is_host_slept == 1)||isLock)
	    return -1;
	

    memset(mode, 0, length);
    if(copy_from_user(mode, buffer, length))
    {
        printk("copy_from_user ERR\n");
        goto switch_mode_out;
    }
	
	if(fm1388_is_bypass && (!strncmp(mode, BT_MODE, strlen(BT_MODE)) || !strncmp(mode, BYPASS_MODE, strlen(BYPASS_MODE)) || !strncmp(mode, BARGEIN_MODE, strlen(BARGEIN_MODE))))
	{
		lidbg(TAG"%s: Is fixed bypass mode!!!\n", __func__);
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
	else if(!strncmp(mode, "cmd", 3))
	{
		if(!strncmp(mode+3,"mode",4))
		{
			isLock = true;
			fm1388_dsp_mode_change(mode[7]-'0');
			isLock = false;
		}
		else if(!strncmp(mode+3,"check",strlen("check")))
		{
			isNotInspectFramecnt = false;
		}
		else if(!strncmp(mode+3,"notcheck",strlen("notcheck")))
		{
			isNotInspectFramecnt = true;
		}
	}
	else if(!strncmp(mode, "setpath", 7))
	{
		char buf[10]={0};
		strcpy(filepath,filepath_save);
		if(strncmp(mode, "setpath0", 8))
		{
			if(length>=8)
			{
				sprintf(buf,"%c/",mode[7]);
				strcat(filepath,buf);
			}
		}
		lidbg_shell_cmd(format_string(false, "echo mode %d,path %s > /sdcard/fm1388.txt",current_mode,filepath));
		fm1388_fw_loaded(NULL);
	}
    else
        lidbg(TAG"%s: no this mode:%s.\n", __func__, mode);

    mutex_unlock(&fm1388_mode_change_lock);

switch_mode_out:
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
	char __user * buffer, size_t length, loff_t * offset)
{
	dev_cmd_short*		local_dev_cmd = NULL;
	dev_cmd_start_rec*	local_start_cmd = NULL;
	dev_cmd_spi_play*	local_playback_cmd = NULL;
	dev_cmd_record_result* local_record_result = NULL;
	dev_cmd_playback_result* local_playback_result = NULL;
	u32 cmd_name, cmd_addr, cmd_val;
	int dsp_mode;
	int (*thread_fn)(void *data);
	int (*playback_thread_fn)(void *data);
	int (*save_data_thread_fn)(void *data);
	int (*load_data_thread_fn)(void *data);
	int i, address;
	int ret_val = ESUCCESS;
	unsigned int fm1388_rec_data_addr[5];
	unsigned int  channel_addr[] = {
		DSP_BUFFER_ADDR0,
		DSP_BUFFER_ADDR1,
		DSP_BUFFER_ADDR2,
		DSP_BUFFER_ADDR3,
		DSP_SPI_FRAMESIZE_ADDR 
	};
	unsigned char temp_channel_index[DSP_SPI_REC_CH_NUM + 1];
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
    //int maxpri; 
	
	//pr_err("%s: entering...\n", __func__);
	if(!fm1388_dsp_working) {
		output_debug_log(true, "[%s]: Sorry, DSP does not work normally now, please try to reboot your system or contact relative person.\n", __func__);
		return -EDSPNOTWORK;
	}
	
	local_dev_cmd = (dev_cmd_short *)kmalloc(length, GFP_KERNEL);
	if (!local_dev_cmd) {
		output_debug_log(true, "[%s]: local_dev_cmd allocation failure.\n", __func__);
		return -ENOMEMORY;
	}
	
	if(copy_from_user(local_dev_cmd, buffer, length)) {
		output_debug_log(true, "[%s]: failed to copy data from user space.\n", __func__);
		if(local_dev_cmd) {
			kfree(local_dev_cmd);
		}
		return -EMEMFAULT;
	}
	
	cmd_name = local_dev_cmd->cmd_name;

	switch(cmd_name) {
		//The short commands
		case FM_SMVD_REG_WRITE:		//Command #1
			cmd_addr = local_dev_cmd->addr;
			cmd_val  = local_dev_cmd->val;
			fm1388_dsp_mode_i2c_write( cmd_addr, cmd_val);
			output_debug_log(true, "[%s]: write new value(%04x) to DSP register(%02x).\n", __func__, cmd_val, cmd_addr);
			ret_val = sizeof(dev_cmd_short);
			break;
			
		case FM_SMVD_DSP_ADDR_WRITE:	//Command #3
			cmd_addr = local_dev_cmd->addr;
			cmd_val  = local_dev_cmd->val;
			fm1388_dsp_mode_i2c_write_addr( cmd_addr, cmd_val, FM1388_I2C_CMD_16_WRITE);
			output_debug_log(true, "[%s]: write new value(%04x) to DSP memory(%08x).\n", __func__, cmd_val, cmd_addr);
			ret_val = sizeof(dev_cmd_short);
			break;
			
		case FM_SMVD_DSP_ADDR_WRITE_SPI:	
			cmd_addr = local_dev_cmd->addr;
			cmd_val  = local_dev_cmd->val;
/*replace parameter read/write by I2C
			fm1388_spi_write(cmd_addr, cmd_val, 2);
			output_debug_log(true, "[%s]: write new value(%04x) to DSP memory(%08x) by SPI.\n", __func__, cmd_val, cmd_addr);
*/
			fm1388_dsp_mode_i2c_write_addr( cmd_addr, cmd_val, FM1388_I2C_CMD_16_WRITE);
			output_debug_log(true, "[%s]: write new value(%04x) to DSP memory(%08x) by I2C.\n", __func__, cmd_val, cmd_addr);
			ret_val = sizeof(dev_cmd_short);
			break;
			
		case FM_SMVD_MODE_SET:		//Command #4
			ret_val = sizeof(dev_cmd_short);
			dsp_mode = local_dev_cmd->addr;
			output_debug_log(true, "[%s]: will set new mode(%x) to DSP.\n", __func__, dsp_mode);
			if(dsp_mode != fm1388_dsp_mode) {
				fm1388_dsp_mode_change( dsp_mode);
				if(dsp_mode == fm1388_dsp_mode) {
				}
				else {
					ret_val = -EFAILTOSETMODE;
					output_debug_log(true, "[%s]: fail to set new mode.\n", __func__);
				}
			}
			else {
				output_debug_log(true, "[%s]: the mode you want to set is the same as current mode.\n", __func__);
			}
			break;
			
		//The long commands
		case FM_SMVD_DSP_BWRITE:		//Command #6
			break;
			
		case FM_SMVD_VECTOR_GET:		//Command #7
			break;
			
		case FM_SMVD_REG_DUMP:			//Command #8
			break;
			
		case FM_SMVD_DSP_PLAYBACK_START:
			output_debug_log(true, "[%s]: will start SPI Playback.\n", __func__);

			if(fm1388_data->playback_thread_id) {
				output_debug_log(true, "[%s]: playback is in processing, please stop it first.\n", __func__);
				kthread_stop(fm1388_data->playback_thread_id);
						msleep(20);
				fm1388_data->playback_thread_id = NULL;
				//ret_val = -EINPROCESSING;
				//break;
			}
			
			if(fm1388_data->thread_id) {
				output_debug_log(true, "[%s]: recording is in processing, please stop it first.\n", __func__);
				kthread_stop(fm1388_data->thread_id);
				msleep(20);
				fm1388_data->thread_id = NULL;
			}
			
			if(fm1388_data->load_data_thread_id) {
				kthread_stop(fm1388_data->load_data_thread_id);
				msleep(20);
				fm1388_data->load_data_thread_id = NULL;
			}
			if(fm1388_data->save_data_thread_id) {
				kthread_stop(fm1388_data->save_data_thread_id);
				msleep(20);
				fm1388_data->save_data_thread_id = NULL;
			}

			play_status_counter 		= 0;
			play_status 				= 0;
			source_file_channel_number 	= 0;
			source_file_total_size		= 0L;
			
			ret_val = sizeof(dev_cmd_spi_play);

				
			for( i = 0; i < 5; i++ ) {
				/*replace parameter read/write by I2C
				fm1388_spi_read(channel_addr[i], &address, 4);
				*/
				//fm1388_dsp_mode_i2c_read_addr_2( channel_addr[i], &address);
				fm1388_spi_read(channel_addr[i], &address, 4);
				fm1388_rec_data_addr[i] = address;
				if (i == 4)
					fm1388_rec_data_addr[i] &= 0x0000FFFF;
				
				if(fm1388_rec_data_addr[i] == 0) {
					ret_val = -EDATAINVAL;
					output_debug_log(true, "[%s]: got wrong channel address for the %i th. data.\n", __func__, i);
					break;
				}
			}
			
			if(i != 5) {
				ret_val = -EDATAINVAL;
				output_debug_log(true, "[%s]: fail to start SPI playback.\n", __func__);
				break;
			}
			
			if(fm1388_rec_data_addr[4] > 0x1000) { //suppose frame size should not be too large
				output_debug_log(true, "[%s]: got wrong frame size, framesize=%x\n", __func__, fm1388_rec_data_addr[4]);
				output_debug_log(true, "[%s]: fail to start SPI playback.\n", __func__);
				ret_val = -EDATAINVAL;
				break;
			}

			playback_thread_fn 			= fm1388_trans_voice_data_by_channel_4buffers;
			load_data_thread_fn 		= fm1388_read_audio_data_from_file;
			local_playback_cmd 			= (dev_cmd_spi_play *) local_dev_cmd;
			snprintf(playback_param.file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, local_playback_cmd->file_path);
			strncpy(playback_param.channel_mapping, local_playback_cmd->channel_mapping, DSP_SPI_REC_CH_NUM + 1);
			playback_param.addr_input0 	= fm1388_rec_data_addr[0];
			playback_param.addr_input1 	= fm1388_rec_data_addr[1];
			playback_param.addr_output0 = fm1388_rec_data_addr[2];
			playback_param.addr_output1 = fm1388_rec_data_addr[3];
			playback_param.framesize 	= fm1388_rec_data_addr[4];

			playback_param.need_recording = local_playback_cmd->need_recording;
			if(playback_param.need_recording == 1) {
				playback_param.rec_ch_num 		= local_playback_cmd->rec_ch_num;
				memcpy(playback_param.rec_ch_idx, local_playback_cmd->rec_ch_idx, DSP_SPI_REC_CH_NUM * sizeof(char));
				
				//initialize recording structure
				//thread_fn 				= fm1388_fetch_voice_data_by_channel;
				fetch_param.ch_num 			= local_playback_cmd->rec_ch_num;
				memcpy(fetch_param.ch_idx, local_playback_cmd->rec_ch_idx, DSP_SPI_REC_CH_NUM * sizeof(char));
				fetch_param.addr_input0 	= fm1388_rec_data_addr[0];
				fetch_param.addr_input1 	= fm1388_rec_data_addr[1];
				fetch_param.addr_output0 	= fm1388_rec_data_addr[2];
				fetch_param.addr_output1 	= fm1388_rec_data_addr[3];
				fetch_param.framesize 		= fm1388_rec_data_addr[4];
				//fm1388_data->thread_id = (struct task_struct *)kthread_run(thread_fn, NULL, "fm1388_fetch_voice_data");

				kfifo_reset(fm1388_data->rec_kfifo);
				save_data_thread_fn	= fm1388_save_audio_data_to_file;
				fm1388_data->save_data_thread_id = (struct task_struct *)kthread_create(save_data_thread_fn, NULL, "fm1388_save_voice_data");
				sched_setscheduler(fm1388_data->save_data_thread_id, SCHED_RR, &param);
				output_debug_log(true, "[%s]: save data thread(SCHED_RR) -- prio=%d, static_prio=%d, normal_prio=%d\n", __func__, 
						fm1388_data->save_data_thread_id->prio, fm1388_data->save_data_thread_id->static_prio, fm1388_data->save_data_thread_id->normal_prio);
				wake_up_process(fm1388_data->save_data_thread_id);
			}
						kfifo_reset(fm1388_data->play_kfifo);
			fm1388_data->load_data_thread_id = (struct task_struct *)kthread_create(load_data_thread_fn, NULL, "fm1388_load_voice_data");
			sched_setscheduler(fm1388_data->load_data_thread_id, SCHED_RR, &param);
			output_debug_log(true, "[%s]: playback thread(SCHED_RR) -- prio=%d, static_prio=%d, normal_prio=%d\n", __func__, 
					fm1388_data->load_data_thread_id->prio, fm1388_data->load_data_thread_id->static_prio, fm1388_data->load_data_thread_id->normal_prio);
			wake_up_process(fm1388_data->load_data_thread_id);
			
			//give some time to let load data thread get source file information
			msleep(500);
			
//			fm1388_data->playback_thread_id = (struct task_struct *)kthread_run(playback_thread_fn, NULL, "fm1388_playback_voice_data");
			fm1388_data->playback_thread_id = (struct task_struct *)kthread_create(playback_thread_fn, NULL, "fm1388_transfer_voice_data");
			sched_setscheduler(fm1388_data->playback_thread_id, SCHED_FIFO, &param);
			output_debug_log(true, "[%s]: playback thread(SCHED_FIFO) -- prio=%d, static_prio=%d, normal_prio=%d\n", __func__, 
					fm1388_data->playback_thread_id->prio, fm1388_data->playback_thread_id->static_prio, fm1388_data->playback_thread_id->normal_prio);
			wake_up_process(fm1388_data->playback_thread_id);

			output_debug_log(true, "[%s]: start SPI Playback with:\n", __func__);
			output_debug_log(true, "[%s]: \tplayback_param.channel_mapping=%s\n", __func__, playback_param.channel_mapping);
			output_debug_log(true, "[%s]: \tplayback_param.file_path=%s\n", __func__, playback_param.file_path);
			output_debug_log(true, "[%s]: \tplayback_param.addr_input0=%x\n", __func__, playback_param.addr_input0);
			output_debug_log(true, "[%s]: \tplayback_param.addr_input1=%x\n", __func__, playback_param.addr_input1);
			output_debug_log(true, "[%s]: \tplayback_param.addr_output0=%x\n", __func__, playback_param.addr_output0);
			output_debug_log(true, "[%s]: \tplayback_param.addr_output1=%x\n", __func__, playback_param.addr_output1);
			output_debug_log(true, "[%s]: \tplayback_param.framesize=%x\n", __func__, playback_param.framesize);
			if(playback_param.need_recording == 1) {
				for(i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
					if(playback_param.rec_ch_idx[i] == 0) {
						temp_channel_index[i] = playback_param.rec_ch_idx[i] + '0';
					}
					else {
						temp_channel_index[i] = playback_param.rec_ch_idx[i];
					}
				}
				temp_channel_index[DSP_SPI_REC_CH_NUM] = 0;

				output_debug_log(true, "[%s]: Also recording at the same time with:\n", __func__);
				output_debug_log(true, "[%s]: \tfetch_param.ch_num=%d\n", __func__, fetch_param.ch_num);
				output_debug_log(true, "[%s]: \tfetch_param.ch_idx=%s\n", __func__, temp_channel_index);
			}
			
			output_debug_log(false, "[%s]: SPI Playback started.\n", __func__);
			break;

		case FM_SMVD_DSP_PLAYBACK_STOP:
			output_debug_log(true, "[%s]: will stop SPI Playback.\n", __func__);

			ret_val = sizeof(dev_cmd_playback_result);
			if(fm1388_data->playback_thread_id) {
				kthread_stop(fm1388_data->playback_thread_id);
				msleep(20);
				fm1388_data->playback_thread_id = NULL;
			}
			if(fm1388_data->load_data_thread_id) {
				kthread_stop(fm1388_data->load_data_thread_id);
				msleep(20);
				fm1388_data->load_data_thread_id = NULL;
			}

			if(fm1388_data->save_data_thread_id) {
				kthread_stop(fm1388_data->save_data_thread_id);
				msleep(20);
				fm1388_data->save_data_thread_id = NULL;
			}

			local_playback_result = (dev_cmd_playback_result *) local_dev_cmd;
			local_playback_result->total_frame_number 			= (u32)playback_total_counter;
			local_playback_result->error_frame_number			= (u32)playback_error_counter;
			local_playback_result->first_error_frame_counter	= (u32)first_playback_error_frame_counter;
			local_playback_result->last_error_frame_counter	= (u32)last_playback_error_frame_counter;
			if(copy_to_user(buffer, local_playback_result, ret_val)) //bring back playback info
				lidbg(TAG"copy_to_user(buffer, local_playback_result, ret_val) failed\n");

			output_debug_log(true, "[%s]: SPI Playback stopped.\n", __func__);
			break;

		case FM_SMVD_DSP_FETCH_VDATA_START:
			output_debug_log(true, "[%s]: will start SPI Recording.\n", __func__);
			ret_val = sizeof(dev_cmd_start_rec);
			
			if(fm1388_data->playback_thread_id) {
				output_debug_log(true, "[%s]: playback is in processing, please stop it first.\n", __func__);
				kthread_stop(fm1388_data->playback_thread_id);
				msleep(20);
				fm1388_data->playback_thread_id = NULL;
			}

			if(fm1388_data->load_data_thread_id) {
				kthread_stop(fm1388_data->load_data_thread_id);
				msleep(20);
				fm1388_data->load_data_thread_id = NULL;
			}
			if(fm1388_data->save_data_thread_id) {
				kthread_stop(fm1388_data->save_data_thread_id);
				msleep(20);
				fm1388_data->save_data_thread_id = NULL;
			}
			if(fm1388_data->thread_id) {
				output_debug_log(true, "[%s]: recording is in processing, please stop it first.\n", __func__);
				msleep(20);
				kthread_stop(fm1388_data->thread_id);
				fm1388_data->thread_id = NULL;
				//ret_val = -EINPROCESSING;
				//break;
			}
			
			for( i = 0; i < 5; i++ ) {
				/*replace parameter read/write by I2C
				fm1388_spi_read(channel_addr[i], &address, 4);
				*/
				//fm1388_dsp_mode_i2c_read_addr_2( channel_addr[i], &address);
				fm1388_spi_read(channel_addr[i], &address, 4);
				lidbg(TAG"\n");
				output_debug_log(true, "[%s]: \tfm1388_spi_read(channel_addr[i], &address, 4);addr %x,value: %x\n", __func__, address,channel_addr[i]);
				
				fm1388_rec_data_addr[i] = address;
				if (i == 4)
					fm1388_rec_data_addr[i] &= 0x0000FFFF;
				
				if(fm1388_rec_data_addr[i] == 0) {
					ret_val = -EDATAINVAL;
					output_debug_log(true, "[%s]: got wrong channel address for the %i th. data.\n", __func__, i);
					break;
				}
			}
			
			if(i != 5) {
				ret_val = -EDATAINVAL;
				output_debug_log(true, "[%s]: fail to start SPI recording.\n", __func__);
				break;
			}
			
			if(fm1388_rec_data_addr[4] > 0x1000) { //suppose frame size should not be too large
				output_debug_log(true, "[%s]: got wrong frame size, framesize=%x\n", __func__, fm1388_rec_data_addr[4]);
				output_debug_log(true, "[%s]: fail to start SPI recording.\n", __func__);
				ret_val = -EDATAINVAL;
				break;
			}

			kfifo_reset(fm1388_data->rec_kfifo);
			thread_fn 				= fm1388_fetch_voice_data_by_channel_4buffers;//fm1388_fetch_voice_data_by_channel_in_timer;//fm1388_fetch_voice_data_by_channel;
			local_start_cmd 		= (dev_cmd_start_rec *) local_dev_cmd;
			fetch_param.ch_num 		= local_start_cmd->ch_num;
			memcpy(fetch_param.ch_idx, local_start_cmd->ch_idx, DSP_SPI_REC_CH_NUM * sizeof(char));
			fetch_param.addr_input0 = fm1388_rec_data_addr[0];
			fetch_param.addr_input1 = fm1388_rec_data_addr[1];
			fetch_param.addr_output0 = fm1388_rec_data_addr[2];
			fetch_param.addr_output1 = fm1388_rec_data_addr[3];
			fetch_param.framesize 	= fm1388_rec_data_addr[4];

//			fm1388_data->thread_id = (struct task_struct *)kthread_run(thread_fn, NULL, "fm1388_fetch_voice_data");
			fm1388_data->thread_id = (struct task_struct *)kthread_create(thread_fn, NULL, "fm1388_fetch_voice_data");
			sched_setscheduler(fm1388_data->thread_id, SCHED_FIFO, &param);
			output_debug_log(true, "[%s]: recording thread(SCHED_FIFO) -- prio=%d, static_prio=%d, normal_prio=%d\n", __func__, 
					fm1388_data->thread_id->prio, fm1388_data->thread_id->static_prio, fm1388_data->thread_id->normal_prio);
			wake_up_process(fm1388_data->thread_id);

			save_data_thread_fn	= fm1388_save_audio_data_to_file;
			fm1388_data->save_data_thread_id = (struct task_struct *)kthread_create(save_data_thread_fn, NULL, "fm1388_save_voice_data");
			sched_setscheduler(fm1388_data->save_data_thread_id, SCHED_RR, &param);
			output_debug_log(true, "[%s]: save data thread(SCHED_RR) -- prio=%d, static_prio=%d, normal_prio=%d\n", __func__, 
					fm1388_data->save_data_thread_id->prio, fm1388_data->save_data_thread_id->static_prio, fm1388_data->save_data_thread_id->normal_prio);
			wake_up_process(fm1388_data->save_data_thread_id);
			
			for(i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
				if(fetch_param.ch_idx[i] == 0) {
					temp_channel_index[i] = fetch_param.ch_idx[i] + '0';
				}
				else { 	
					temp_channel_index[i] = fetch_param.ch_idx[i];
				}
			}
			temp_channel_index[DSP_SPI_REC_CH_NUM] = 0;
			
			output_debug_log(true, "[%s]: start SPI recording with:\n", __func__);
			output_debug_log(true, "[%s]: \tfetch_param.ch_num=%d\n", __func__, fetch_param.ch_num);
			output_debug_log(true, "[%s]: \tfetch_param.ch_idx=%s\n", __func__, temp_channel_index);
			output_debug_log(true, "[%s]: \tfetch_param.addr_input0=%x\n", __func__, fetch_param.addr_input0);
			output_debug_log(true, "[%s]: \tfetch_param.addr_input1=%x\n", __func__, fetch_param.addr_input1);
			output_debug_log(true, "[%s]: \tfetch_param.addr_output0=%x\n", __func__, fetch_param.addr_output0);
			output_debug_log(true, "[%s]: \tfetch_param.addr_output1=%x\n", __func__, fetch_param.addr_output1);
			output_debug_log(true, "[%s]: \tfetch_param.framesize=%x\n", __func__, fetch_param.framesize);

			output_debug_log(true, "[%s]: SPI Recording started.\n", __func__);
			break;

		case FM_SMVD_DSP_FETCH_VDATA_STOP:
			output_debug_log(true, "[%s]: will stop SPI Recording.\n", __func__);
			ret_val = sizeof(dev_cmd_short);
			
			if(fm1388_data->thread_id) {
				kthread_stop(fm1388_data->thread_id);
				msleep(20);
				fm1388_data->thread_id = NULL;
			}
			output_debug_log(true, "[%s]: sent stop recording thread message.\n", __func__);

			local_record_result = (dev_cmd_record_result *) local_dev_cmd;
			local_record_result->total_frame_number 		= (u32)(record_total_counter + record_error_counter);
			local_record_result->error_frame_number			= (u32)record_error_counter;
			local_record_result->first_error_frame_counter	= (u32)first_record_error_frame_counter;
			local_record_result->last_error_frame_counter	= (u32)last_record_error_frame_counter;
			if(copy_to_user(buffer, local_record_result, ret_val)) //bring back recording info
				lidbg("copy_to_user(buffer, local_record_result, ret_val) failed\n");
			
			if(fm1388_data->save_data_thread_id) {
				kthread_stop(fm1388_data->save_data_thread_id);
				msleep(20);
				fm1388_data->save_data_thread_id = NULL;
			}
			output_debug_log(true, "[%s]: sent stop save data thread message.\n", __func__);

			output_debug_log(true, "[%s]: SPI Recording stopped.\n", __func__);
			break;

		default:
			ret_val = -ECOMMANDINVAL;
			break;
	}

	if (local_dev_cmd) kfree(local_dev_cmd);
	return ret_val;
}

static void dsp_start_vr_work(struct work_struct *work)
{
    lidbg(TAG"%s: entering.\n", __func__);
    //Todo: something for VR mode
}



// for debugging
#ifdef SHOW_FRAMECNT
static void fm1388_framecnt_handling_work(struct work_struct *work)
{
    unsigned int addr, val;
    addr = CRC_STATUS;

    while (1)
    {
        msleep(10000);

        if((g_var.acc_flag == FLY_ACC_OFF)||(isNotInspectFramecnt) ||(is_host_slept == 1) || (isLock) )
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

void check_dsp_status(void) {
	u32 val, framecount1, framecount2;
	u32 addr;

	addr = CRC_STATUS;
	fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
	lidbg(TAG"%s: crc value = %#x @ %#x (0x8888 for crc checking success)\n", __func__, val, addr);
	msleep(100);
	
	addr = FRAME_CNT;
	fm1388_dsp_mode_i2c_read_addr_2(addr, &framecount1);
	msleep(200);
	
	//Fuli 20160830 check DSP is working or not
	addr = FRAME_CNT;
	fm1388_dsp_mode_i2c_read_addr_2(addr, &framecount2);
lidbg(TAG"%s: framecount1=%x, framecount2=%x\n", __func__, framecount1, framecount2);
	if(framecount1 != framecount2){
		lidbg(TAG"%s: Firmware is working now.\n", __func__);
		fm1388_dsp_working = true;
	}
	else {
		lidbg(TAG"%s: Firmware is not working normally.\n", __func__);
		fm1388_dsp_working = false;
	}
	/*
	//Fuli 20160830 follow SAB's behavior, check CRC
	addr = CHECKING_STATUS1;
	fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
	lidbg(TAG"%s: CRC Status. 0x%x = 0x%x\n", __func__, addr, val);

	//Fuli 20160830 follow SAB's behavior, check CRC
	addr = CHECKING_STATUS2;
	fm1388_dsp_mode_i2c_read_addr_2(addr, &val);
	lidbg(TAG"%s: CRC Status. 0x%x = 0x%x\n", __func__, addr, val);
*/
//Fuli 20160831 sab does not do this read
//	fm1388_dsp_mode_i2c_read_addr(fm1388_i2c, DSP_STATUS_ADDR, &val);	// check register 0x65 to make sure DSP is running
//
//	lidbg(TAG"%s: dsp status value = %#x @ 0x180200CA (0x7fe for running success)\n", __func__,val);

	return;
}

void postwork_set_mode(void) {
	//Fuli 20160902 wait for a while to let DSP working normally
	//msleep(200);
	msleep(10);
	//
	
    load_fm1388_vec(combine_path_name(filepath_name, "FM1388_run.vec"));
    //msleep(2000);
	msleep(200);
	check_dsp_status();
	
	//Fuli 20160902 wait a while
	//msleep(2000);
	msleep(20);
	//
}



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
            SOC_IO_Output(0, FM1388_RESET_PIN, 1);
            CREATE_KTHREAD(fm1388_fw_loaded,NULL);
            is_host_slept = 0;
            resumeAfterReinitCount = 0;
        }		
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
        break;
		
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
		is_host_slept = 1;
        SOC_IO_Output(0, FM1388_RESET_PIN, 0);
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
    lidbg(TAG"%s: return 0;\n", __func__);

    return 0;
}

static int fm1388_i2c_resume(struct device *dev)
{
    lidbg(TAG"%s: return 0;\n", __func__);

	return 0;
}
#endif

static const struct dev_pm_ops fm1388_i2c_ops =
{
    .suspend = fm1388_i2c_suspend,
    .resume  = fm1388_i2c_resume,
};

static void set_vec_file_path(void)
{
	int car_type=0;

	if(gboot_mode == MD_FLYSYSTEM)
		strcpy(filepath,"/flysystem/lib/out/fm1388/");
	else
		strcpy(filepath,"/system/lib/modules/out/fm1388/");
	
	fs_get_intvalue(g_var.pflyhal_config_list, "fm1388_is_bypass", &fm1388_is_bypass, NULL);
	lidbg(TAG"fm1388_is_bypass=%d\n", fm1388_is_bypass);
	
	if(fm1388_is_bypass)
	{
		current_mode = 5;
		strcat(filepath,"common/");
	}
	else
	{
		fs_get_intvalue(g_var.pflyhal_config_list, "fm1388_config", &car_type, NULL);
		switch(car_type)
		{
			case RUIJIE:
				strcat(filepath,"ruijie/");
				break;
			case MENGDIOU:
				strcat(filepath,"mengdiou/");
				break;
			default:
				strcat(filepath,"ruijie/");
				break;
		}
		strcpy(filepath_save,filepath);
		lidbg(TAG"car_type=%u,vec file path=%s\n", car_type, filepath);
	}
	
}

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
	fm1388_data = (struct fm1388_data_t *)kzalloc(sizeof(struct fm1388_data_t), GFP_KERNEL);
	if (fm1388_data == NULL)
		return -ENOMEM;
	lidbg(TAG"%s: fm1388_data allocated successfully\n", __func__);

	mutex_init(&fm1388_data->lock);
    lidbg(TAG"%s: device_create_file - dev_attr_fm1388_reg.\n", __func__);
	set_vec_file_path();
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

	fm1388_create_cdev(pdev);

    lidbg_new_cdev(&fm1388_mode_fops, "fm1388_switch_mode");
	lidbg_shell_cmd("chmod 777 /dev/fm1388_switch_mode");


    INIT_DELAYED_WORK(&dsp_start_vr, dsp_start_vr_work);


#ifdef SUSPEND_ONLINE
    lidbg(TAG"%s: register_lidbg_notifier\n", __func__);
    register_lidbg_notifier(&lidbg_notifier);
#endif


#ifdef SHOW_FRAMECNT
    INIT_WORK(&fm1388_framecnt_work, fm1388_framecnt_handling_work);
    fm1388_framecnt_wq = create_singlethread_workqueue("fm1388_framecnt_wq");
    //msleep(180000);
    queue_work(fm1388_framecnt_wq, &fm1388_framecnt_work);
#endif

	fm1388_data->thread_id = NULL;
	fm1388_data->playback_thread_id = NULL;
	
	fm1388_data->save_data_thread_id = NULL;
	spin_lock_init(&fm1388_data->rec_lock);
	fm1388_data->rec_kfifo = NULL;
	fm1388_data->rec_kfifo = (struct kfifo *)kmalloc(sizeof(struct kfifo), GFP_KERNEL);
	if (fm1388_data->rec_kfifo == NULL) {
		pr_err("%s: no fifo for rec_kfifo\n", __func__);
		return -ENOMEM;
	}
	pr_err("%s: rec_kfifo alloc ok...\n", __func__);

	ret = kfifo_alloc(fm1388_data->rec_kfifo, MAX_KFIFO_BUFFER_SIZE, GFP_KERNEL);
	if (ret) {
		pr_err("%s: no kfifo memory\n", __func__);
		return ret;
	}
	pr_err("%s: rec_kfifo buffer alloc ok...\n", __func__);

	fm1388_data->load_data_thread_id = NULL;
	spin_lock_init(&fm1388_data->play_lock);
	fm1388_data->play_kfifo = NULL;
	fm1388_data->play_kfifo = (struct kfifo *)kmalloc(sizeof(struct kfifo), GFP_KERNEL);
	if (fm1388_data->play_kfifo == NULL) {
		pr_err("%s: no fifo for play_kfifo\n", __func__);
		return -ENOMEM;
	}
	pr_err("%s: play_kfifo alloc ok...\n", __func__);

	ret = kfifo_alloc(fm1388_data->play_kfifo, MAX_KFIFO_BUFFER_SIZE, GFP_KERNEL);
	if (ret) {
		pr_err("%s: no play_kfifo memory\n", __func__);
		return ret;
	}
	pr_err("%s: play_kfifo buffer alloc ok...\n", __func__);

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
MODULE_AUTHOR(" sample code <henryhzhang@fortemedia.com>;<fuli@fortemedia.com>");
MODULE_LICENSE("GPL v2");
