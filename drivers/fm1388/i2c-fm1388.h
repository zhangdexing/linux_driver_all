/*
 * drivers/i2c/busses/i2c-fm1388.h
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
#ifndef _FM1388_H_
#define _FM1388_H_

//
// Following address may be changed by different firmware release
//
#define DSP_BUFFER_ADDR0		(0x5FFDFF80)
#define DSP_BUFFER_ADDR1		(0x5FFDFF84)
#define DSP_BUFFER_ADDR2		(0x5FFDFF88)
#define DSP_BUFFER_ADDR3		(0x5FFDFF8C)
#define DSP_SPI_FRAMESIZE_ADDR	(0x5FFDFF90)
#define FRAME_CNT 				0x5FFDFFCC
#define CRC_STATUS 				0x5FFDFFE8
#define CHECKING_STATUS1		0x5FFDFF58
#define CHECKING_STATUS2		0x5FFDFF5A
#define DSP_PARAMETER_READY 	0x5FFDFFEA

#define ESUCCESS			0
#define EUNKNOWN			10000
#define EDUPOPEN			10001
#define EFAILOPEN			10002
#define ENOTOPEN			10003
#define EPARAMINVAL			10004
#define EDATAINVAL			10005
#define ESPIREAD			10006
#define ESPIWRITE			10007
#define ESPISETFAIL			10008
#define EDSPNOTWORK			10009
#define ENOMEMORY			10010
#define EMEMFAULT			10011
#define ECANNOTENRECORD		10012
#define ENOSDCARD			10013
#define EFILECANNOTWRITE	10014
#define EINPROCESSING		10015
#define EFAILTOSETMODE		10016
#define ECOMMANDINVAL		10017
#define EFILECANNOTREAD		10018
#define ECANNOTENPLAY		10019
#define ENOENOUGHDATA		10020

#define FM1388_FIFO_LEN			32768	//(1920 * 4)	//(3072)//2048 < 160*2*8=2560 < 3072
#define FM1388_BUFFER_LEN		4096
#define CMD_BUF_LEN				1024
#define DSP_SPI_REC_CH_NUM		10
#define MAX_COMMENT_LEN			100
#define MAX_PATH_LEN			128
#define CONFIG_LINE_LEN			255

#define DSP_CMD_ADDR			(0x5FFDFF7E)//(0x5FFDFFCC)//
#define DSP_STOP_CMD			(0)
//#define DSP_INIT_CMD			(0x8B2F)
//#define DSP_READY_CMD			(0x8A2F)

#define DSP_INITIALIZED			(0x07FF)
#define DSP_STATUS_ADDR			(0x180200CA)

#define PLAY_READY          	0x0001
#define PLAY_READY_RESET    	0xFFFE
#define RECORD_READY      	  	0x0002
#define RECORD_READY_RESET  	0xFFFD
#define PLAY_ERROR          	0x0004
#define PLAY_ERROR_RESET    	0xFFFB
#define RECORD_ERROR        	0x0008
#define RECORD_ERROR_RESET  	0xFFF7
#define PLAY_ENABLE         	0x4000
#define PLAY_ENABLE_RESET   	0xBFFF
#define RECORD_ENABLE       	0x8000
#define RECORD_ENABLE_RESET 	0x7FFF
#define KEY_PHRASE          	(0x25A<<4)

/* DSP Mode I2C Control*/
#define FM1388_DSP_I2C_OP_CODE	0x00
#define FM1388_DSP_I2C_ADDR_LSB	0x01
#define FM1388_DSP_I2C_ADDR_MSB	0x02
#define FM1388_DSP_I2C_DATA_LSB	0x03
#define FM1388_DSP_I2C_DATA_MSB	0x04

/*
 * Henry Zhang - define structure and enum for device_read/device_write operations
 */
#define CMD_BUF_LEN	1024
enum DEV_COMMAND {
	//The long commands
	FM_SMVD_REG_READ,		//Command #0
	FM_SMVD_REG_WRITE,		//Command #1
	FM_SMVD_DSP_ADDR_READ,	//Command #2
	FM_SMVD_DSP_ADDR_WRITE,	//Command #3
	FM_SMVD_MODE_SET,		//Command #4
	FM_SMVD_MODE_GET,		//Command #5
	//The long commands
	FM_SMVD_DSP_BWRITE,		//Command #6
	FM_SMVD_VECTOR_GET,		//Command #7
	FM_SMVD_REG_DUMP,		//Command #8
	
	FM_SMVD_DSP_ADDR_READ_SPI,
	FM_SMVD_DSP_ADDR_WRITE_SPI,

	FM_SMVD_DSP_FETCH_VDATA_START,
	FM_SMVD_DSP_FETCH_VDATA_STOP,

	FM_SMVD_DSP_PLAYBACK_START,
	FM_SMVD_DSP_PLAYBACK_STOP,
	FM_SMVD_DSP_IS_PLAYING,
};


enum {
	FM1388_I2C_CMD_16_WRITE = 1,
	FM1388_I2C_CMD_32_READ,
	FM1388_I2C_CMD_32_WRITE,
};
/*
enum {
	FM1388_MODE_BARGE_IN,
	FM1388_MODE_VR,
	FM1388_MODE_COMMUNICATION,
};
*/
enum DSP_MODE {
	FM_SMVD_DSP_BYPASS,		//the bypass mode of the dsp. in this mode the DMIC input is bypassed to codec.
	FM_SMVD_DSP_DETECTION,
	FM_SMVD_DSP_MIXTURE,
	FM_SMVD_DSP_FACTORY,
	FM_SMVD_DSP_VR,			//the voice recognition mode of the dsp.
	FM_SMVD_DSP_CM,			//the communication mode of the dsp.
	FM_SMVD_DSP_BARGE_IN,	//the barge-in mode of the dsp. in this mode FM_SMVD detects the keyword and issues interrupt to the host AP.
	FM_SMVD_GET_DSP_MODE,
	FM_SMVD_DOWNLOAD_UDT_FIRMWARE,
	FM_SMVD_DOWNLOAD_EFT_FIRMWARE,
	FM_SMVD_DOWNLOAD_WHOLE_FIRMWARE,
	FM_SMVD_SET_EFT_SVTHD,
	FM_SMVD_SET_UDT_SVTHD,
	FM_SMVD_DUMP_REGISTER,
};

typedef struct multi_ch_buf_t {
	int ch_num;
	int ch_len;
	char **ch;
} multi_ch_buf;

/*
 * The structure dev_cmd_short/long defines the command protocol between the library and the device driver.
 * In device driver, the device read and device write functions handle the structure data and parse it.
 * 
 * dev_cmd_short: the structure for device command FM_SMVD_REG_READ, FM_SMVD_REG_WRITE, FM_SMVD_DSP_READ,
 * FM_SMVD_DSP_WRITE, FM_SMVD_MODE_SET and FM_SMVD_MODE_GET, for which no extra data buffer is needed.
 */
typedef struct dev_cmd_short_t {
	u16 cmd_name;	//The commands from #0~#5
	u32 addr;			//The address of the register or dsp memory for the commands #0~#3, or the dsp mode for #4~#5. 
	u32 val;			//The operation or returned value for the commands #0~#3, or zero for the commands #4~#5.
	u8  reserved[6];
} dev_cmd_short;
/* 
 * dev_cmd_long: the structure for device command FM_SMVD_DSP_BWRITE, FM_SMVD_VECTOR_GET and FM_SMVD_REG_DUMP,
 * for which the extra data buffer is necessary for input or output data.
 */
/*
typedef struct dev_cmd_long_t {
	u16 cmd_name;	//The command from #6~#8
	u32 addr;			//The address of dsp memory for the command #6, or zero for #7~#8. 
	u32 val;			//The the valid data length.
	u8  reserved[6];
	u8  buf[CMD_BUF_LEN];	//The data buffer in fixed size, for input and output.
} dev_cmd_long;
*/

enum DSP_CFG_MODE {
	FM_SMVD_CFG_VR,
	FM_SMVD_CFG_CM,
	FM_SMVD_CFG_BARGE_IN = 3,
};

typedef struct dev_cmd_mode_gs_t {
	u16 cmd_name;
	u32 dsp_mode;
	u8  hd_reserved[10];
} dev_cmd_mode_gs;


typedef struct dev_cmd_reg_rw_t {
	u16 cmd_name;
	u32 reg_addr;
	u32 reg_val;
	u8  hd_reserved[6];
} dev_cmd_reg_rw;

typedef struct dev_cmd_start_rec_t {
	u16 cmd_name;
	u16 ch_num;
	u8  ch_idx[DSP_SPI_REC_CH_NUM];
	u8  hd_reserved[2];
} dev_cmd_start_rec;

typedef struct dev_cmd_dv_fetch_t {
	u16 cmd_name;	//The command for debug vector fetching
	u16 ch_num;
	u32 addr_input0;
	u32 addr_input1;
	u32 addr_output0;
	u32 addr_output1;
	u32 addr_dbgv;
	u32 framesize;		//The valid data length.
	u8  ch_idx[DSP_SPI_REC_CH_NUM];
	u8  reserved[CMD_BUF_LEN - 12 - DSP_SPI_REC_CH_NUM];
} dev_cmd_dv_fetch;

typedef struct dev_cmd_spi_play_t {
	u16 cmd_name;
	u8  file_path[MAX_PATH_LEN];
	u8  channel_mapping[DSP_SPI_REC_CH_NUM + 1];
	u8	need_recording;
	u16	rec_ch_num;
	u8	rec_ch_idx[DSP_SPI_REC_CH_NUM];
} dev_cmd_spi_play;

typedef struct dev_cmd_dv_playback_t {
	u16 cmd_name;	//The command for debug vector fetching
	u16 ch_num;
	u32 addr_input0;
	u32 addr_input1;
	u32 addr_output0;
	u32 addr_output1;
	u32 addr_dbgv;
	u32 framesize;		//The valid data length.
	u8  file_path[MAX_PATH_LEN];
	u8  channel_mapping[DSP_SPI_REC_CH_NUM + 1];
	u8	need_recording;
	u16	rec_ch_num;
	u8	rec_ch_idx[DSP_SPI_REC_CH_NUM];
	u8  reserved[CMD_BUF_LEN - 12 - MAX_PATH_LEN - DSP_SPI_REC_CH_NUM - 1 - 3 - DSP_SPI_REC_CH_NUM];
} dev_cmd_dv_playback;


struct fm1388_reg_list {
	u8 layer;
	u8 reg;
	u16 val;
};

// Wayne 9/21/2015 for DSP parameters
struct fm1388_dsp_addr_list {
	u32 addr;
	u16 val;
};

typedef struct dev_cmd_t {
	u32 reg_addr;
	u32 val;
} dev_cmd;

typedef struct cfg_mode_cmd_t {
	u32  mode;
    char path_setting_file_name[MAX_PATH_LEN];
    char dsp_setting_file_name[MAX_PATH_LEN];
    char comment[MAX_COMMENT_LEN];
} cfg_mode_cmd;

struct fm1388_data_t {
	struct mutex	lock;
	int				buffering;
	dev_t			record_chrdev;
	struct cdev		record_cdev;
	struct device	*record_dev;
/*Fuli 20160913 not used in current implementation
	struct kfifo	*pcm_kfifo;
	spinlock_t		pcm_lock;
*/	
	struct class	*cdev_class;
	atomic_t		audio_owner;

/*Fuli 20160913 not used in current implementation
	struct kfifo	*alsa_kfifo;
	spinlock_t		alsa_fifo_lock;
	u32				alsa_rq_flag;
	
	int				pdm_clki_gpio;
*/	
//	int				rec_cmd_flag;
//	struct mutex	rec_cmd_lock;
	struct task_struct *thread_id;
	struct task_struct *playback_thread_id;
};

extern int fm1388_spi_read(u32 addr, u32 *val, size_t len);
extern int fm1388_spi_write(u32 addr, u32 val, size_t len);
extern int fm1388_spi_burst_read(u32 addr, u8 *rxbuf, size_t len);
extern int fm1388_spi_burst_write(u32 addr, const u8 *txbuf, size_t len);
extern void fm1388_spi_device_reload(void);
extern void spi_test(void);
//fuli 20160827 added to change spi speed before burst read
extern int fm1388_spi_change_maxspeed(u32 new_speed);
//
#endif /* _FM1388_H_ */