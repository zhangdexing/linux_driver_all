/*
 * fm1388.h  --  FM1388 driver
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FM1388_H_
#define _FM1388_H_

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
};

/*
 * The structure dev_cmd_short/long defines the command protocol between the library and the device driver.
 * In device driver, the device read and device write functions handle the structure data and parse it.
 * 
 * dev_cmd_short: the structure for device command FM_SMVD_REG_READ, FM_SMVD_REG_WRITE, FM_SMVD_DSP_READ,
 * FM_SMVD_DSP_WRITE, FM_SMVD_MODE_SET and FM_SMVD_MODE_GET, for which no extra data buffer is needed.
 */
typedef struct dev_cmd_short_t {
	unsigned short cmd_name;	//The commands from #0~#5
	unsigned int addr;			//The address of the register or dsp memory for the commands #0~#3, or the dsp mode for #4~#5. 
	unsigned int val;			//The operation or returned value for the commands #0~#3, or zero for the commands #4~#5.
	unsigned char reserved[6];
} dev_cmd_short;
/* 
 * dev_cmd_long: the structure for device command FM_SMVD_DSP_BWRITE, FM_SMVD_VECTOR_GET and FM_SMVD_REG_DUMP,
 * for which the extra data buffer is necessary for input or output data.
 */
typedef struct dev_cmd_long_t {
	unsigned short cmd_name;	//The command from #6~#8
	unsigned int addr;			//The address of dsp memory for the command #6, or zero for #7~#8. 
	unsigned int val;			//The the valid data length.
	unsigned char reserved[6];
	unsigned char buf[CMD_BUF_LEN];	//The data buffer in fixed size, for input and output.
} dev_cmd_long;
//Henry Zhang - end of device_read/device_write operation definitions

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

enum DSP_CFG_MODE {
	FM_SMVD_CFG_VR,
	FM_SMVD_CFG_CM,
	FM_SMVD_CFG_BARGE_IN = 3,
};

typedef struct dev_cmd_mode_gs_t {
	unsigned short cmd_name;
	unsigned int dsp_mode;
	unsigned char hd_reserved[10];
} dev_cmd_mode_gs;


typedef struct dev_cmd_reg_rw_t {
	unsigned short cmd_name;
	unsigned int reg_addr;
	unsigned int reg_val;
	unsigned char hd_reserved[6];
} dev_cmd_reg_rw;

/* DSP Mode I2C Control*/
#define FM1388_DSP_I2C_OP_CODE			0x00
#define FM1388_DSP_I2C_ADDR_LSB			0x01
#define FM1388_DSP_I2C_ADDR_MSB			0x02
#define FM1388_DSP_I2C_DATA_LSB			0x03
#define FM1388_DSP_I2C_DATA_MSB			0x04

//Sep.7, 2015 - Added by Henry for FM1388
#define TYPE1_BASE_ADDR		0x5FFDFFC0
#define SLEEP_WAKEUP		(TYPE1_BASE_ADDR + 4)
#define N_BUF				(TYPE1_BASE_ADDR + 5)
#define FRAME_CNT			(TYPE1_BASE_ADDR + 6)
#define MX_PR_IDX			(TYPE1_BASE_ADDR + 7)
#define MX_PR_DAT			(TYPE1_BASE_ADDR + 8)
#define TX_RX_IDX			(TYPE1_BASE_ADDR + 9)
#define TX_RX_DAT			(TYPE1_BASE_ADDR + 10)
#define REC_ADDR			(TYPE1_BASE_ADDR + 12)
#define CRC_STATUS			(TYPE1_BASE_ADDR + 20)
#define TYPE2_BASE_ADDR		0x5FFDFF30
#define CODEC_INIT_ENA		(TYPE2_BASE_ADDR + 0)
#define TYPE2_UNKNOWN_1		(TYPE2_BASE_ADDR + 1)
#define SRCIN_AEC_NLIN		(TYPE2_BASE_ADDR + 24)
#define SRCIN_LIN			(TYPE2_BASE_ADDR + 25)
#define TYPE2_UNKNOWN_26	(TYPE2_BASE_ADDR + 26)
#define SRCIN_MIC0_3		(TYPE2_BASE_ADDR + 27)
#define SRCOB_0_7			(TYPE2_BASE_ADDR + 28)
#define TYPE2_UNKNOWN_29	(TYPE2_BASE_ADDR + 29)
#define LED_FLASH_ENA		(TYPE2_BASE_ADDR + 30)
#define INB_SE_OB_SE		(TYPE2_BASE_ADDR + 31)
#define NBYTE_PER_SMPL		(TYPE2_BASE_ADDR + 32)
#define N_BOUNDS			(TYPE2_BASE_ADDR + 33)
#define TYPE2_UNKNOWN_34	(TYPE2_BASE_ADDR + 34)
#define TYPE2_UNKNOWN_35	(TYPE2_BASE_ADDR + 35)
#define BYPASS				(TYPE2_BASE_ADDR + 36)
#define HW_BASE_ADDR		0x5FFDFE00
unsigned short HW_PAIR[70];

struct fm1388_init_reg {
	u8 reg;
	u16 val;
};

struct fm1388_init_mem32 {
	u32 addr;
	u16 val;
};

struct fm1388_hw_setting {
	u16 lsb_val;
	u16 msb_val;
};
#if 0
static struct fm1388_init_reg fm1388_i2c_init[] = {
	{0xFA, 0x0001},
	{0x63, 0xFDD5},
	{0x64, 0x0181},
	{0x67, 0x07FF},
	{0x65, 0x07FF},
};

static struct fm1388_init_mem32 fm1388_gpv_type2[] = {
	{CODEC_INIT_ENA,	0x0000},
	{TYPE2_UNKNOWN_1,	0x0000},
	{SRCIN_AEC_NLIN,	0x0001},
	{SRCIN_LIN,			0x0001},
	{TYPE2_UNKNOWN_26,	0x0000},
	{SRCIN_MIC0_3,		0x5432},
	{SRCOB_0_7,			0x1012},
	{TYPE2_UNKNOWN_29,	0x0034},
	{LED_FLASH_ENA,		0x0001},
	{INB_SE_OB_SE,		0x0606},
	{NBYTE_PER_SMPL,	0x0004},
	{N_BOUNDS,			0x000A},
	{TYPE2_UNKNOWN_34,	0x0000},
	{TYPE2_UNKNOWN_35,	0x0000},
	{BYPASS,			0x0000},
};

static struct fm1388_hw_setting fm1388_hw_setting_pair[] = {
	{0x807C, 0x18C0},
	{0x807D, 0x0000},
	{0x807D, 0x2000},
	{0x807D, 0x2002},
	{0x807D, 0x2000},
	{0x8073, 0x4444},
	{0x8061, 0x981E},
	{0x8062, 0x9C30},
	{0x8063, 0xFDD6},
	{0x8064, 0xC1B1},
	{0xC015, 0x0490},
	{0xC038, 0x0F71},
	{0xC039, 0x0F71},
	{0xC01E, 0x0000},
	{0xC03D, 0x364E},
	{0xC017, 0x0C40},
	{0xC013, 0x0732},
	{0xC03A, 0x3002},
	{0x8050, 0xD545},
	{0x8003, 0x0000},
	{0x8026, 0x4480},
	{0x8025, 0x4580},
	{0x8027, 0x9040},
	{0x80A3, 0x3330},
	{0x803B, 0x1F00},
	{0x8070, 0x8008},
	{0x803C, 0x0650},
	{0x801B, 0x0055},
	{0x802B, 0x8A8A},
	{0x8015, 0x0001},
	{0x8001, 0x5800},
};


static struct fm1388_init_mem32 fm1388_dsp_run = {
	0x180200CA, 0x07FE
};
#endif

//extern int __init fm1388_spi_init(void);
//extern struct spi_driver fm1388_spi_driver;
extern void spi_test(void);
extern void fm1388_spi_device_reload(void);
extern int fm1388_spi_read(unsigned int addr, unsigned int *val, size_t len);
extern int fm1388_spi_write(unsigned int addr, unsigned int val, size_t len);
extern int fm1388_spi_burst_read(unsigned int addr, u8 *rxbuf, size_t len);
extern int fm1388_spi_burst_write(u32 addr, const u8 *txbuf, size_t len);
//end of Henry's comments
#endif /* _FM1388_H_ */
