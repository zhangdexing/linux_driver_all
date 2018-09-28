#ifndef _BURNERAP_LIB_H
#define _BURNERAP_LIB_H

#include "..//common//usb.h"
#include "..//common//my_type.h"

#define MODE_EXTENSION_UNIT			0x00
#define MODE_VENDOR_COMMAND			0x01

// manufacturer ID
#define SF_TYPE_UNKNOWN				0x00
#define SF_TYPE_MXIC				0x01
#define SF_TYPE_ST					0x02
#define SF_TYPE_SST					0x03
#define SF_TYPE_ATMEL_AT25F			0x04
#define SF_TYPE_ATMEL_AT25FS		0x05
#define SF_TYPE_ATMEL_AT45DB		0x06
#define SF_TYPE_WINBOND				0x07
#define SF_TYPE_PMC					0x08
#define SF_TYPE_AMIC				0x0A	// shawn 2009/08/14 add
#define SF_TYPE_EON					0x0B	// shawn 2009/08/14 add
#define SF_TYPE_MXIC_LIKE			0xFF

#define ProbeMem_SET		14
#define ProbeMem_GET		15

#define LEN_ROM_VER		8
#define LEN_CODE_VER	32
#define LEN_VENDOR_VER	16
#define LEN_FW_VER	10

#define ROM_VER_COUNT	11

// shawn 2011/07/13 +++++
#define MANUFACTURER_ID_MXIC	0xc2
#define MEM_TYPE_32				0x22
// shawn 2011/07/13 -----

typedef enum 
{
	ROM_UNKNOWN = 0x00,
	ROM_220RO,			// 220RO (21x, 21xA, 21xB)
	ROM_225RO,			// 225RO
	ROM_250RO_v1,		// 250RO v1
	ROM_250RO_v2,		// 250RO v2
	ROM_230RO_v1,		// 230RO v1
	ROM_230RO_v2,		// 230RO v2
	ROM_230RO_v3,		// 230RO v3 
	ROM_231RO_v1, 		// 231RO v1
	ROM_215RO,			// 215RO
	ROM_236RO,			// 236RO
	ROM_236RO_v2,		// 236RO v2
} ENUM_ROM_VER;

typedef enum
{
	ROM_ID_UNKNOWN = -1,
	// 220 Series
	ST50220B_001 = 0,
	ST50225A_001,
	ST50225E_002,
	ST50225E_1001,
	// 230 Series
	ST50230A,
	ST50230C_0002,
	ST50230C_0003,
	// 250 Series
	ST50250A,
	ST50250B,
	// 215 Series
	STT5215,
	ST50215A_0001,
	// 236 Series
	ST50236A_0001,
	ST50236B_0002,
	// 231 Series
	ST50231A_0001,
	ST50231D_1001,	// shawn 2009/08/14 add 233
	// 256 Series
	ST50256B_0001,	// shawn 2009/08/14 modify
	// 232 Series
	SD50232A_2005,	// shawn 2009/11/02 add 232
	// 276 Series
	ST50275A_3005,	// shawn 2009/11/02 add 275
	ST50276A_4006,	// shawn 2010/07/12 add 276
	ST50290A_6007,	// shawn 2010/12/13 for 290
	ST5L232A_5007,	// shawn 2011/07/25 for 262
	ST5L286A_4119,	// shawn 2011/07/25 for 286
	ST5L288A_4221,	// shawn 2011/07/25 for 288
	ST5L292,        // carol 2013/10/09 add for 292
	ST5L271,	// carol 2013/10/30 add for 271
	ST5L281,	// carol 2013/10/30 add for 281
	ST50290_V2,      // carol 2013/12/16 add for 290_V2
	ST5L283
} ENUM_ROM_ID;

typedef struct 
{
	ENUM_ROM_ID		nID;
	char			szInId[16];
	char			szOutId[16];
	unsigned short	nVerAddr;
	char			szRomVer[8];		// ROM Code Version String
	unsigned short	nSFTypeAddr;
	unsigned long	nBypass;
	unsigned short	nSFOffset1;
	unsigned short	nSFOffset2;
	char			szSFVer[16];		// SF Version String
	bool			bIsSWErase;			// shawn 2009/08/14 for 233
	bool			bIsDisSWWriteCmd;	// shawn 2009/08/14 for 233
	bool			bIsGeneric;			// shawn 2010/07/12 add
} ROM_DEF_S;

bool 	init_xfer(usb_dev_handle *udev, int mode);
bool 	uninit_xfer(void);

bool 	get_code_version(char szFlashCodeVer[LEN_CODE_VER]);
bool 	get_vendor_version(char szFlashVendorVer[LEN_VENDOR_VER]);
bool 	get_rom_version(char szRomVer[LEN_ROM_VER]);
bool 	get_fw_version(char szFwVer[LEN_FW_VER]);
bool	get_rom_id(ENUM_ROM_ID &nID);
int		rom_str_to_id(char *RomString);
bool 	get_bypass_length(unsigned long &ulBypass);
bool 	get_flash_type(BYTE &type);
bool 	is_new_sensor_table(void);
bool	pull_cpu_rate(bool bControl);
bool	asic_read(unsigned short addr, BYTE *pValue);
bool 	asic_write(unsigned short addr, BYTE value);
bool	set_flash_type(int iFlashType);
bool	rom_read(unsigned int address, unsigned char *pData, unsigned long len);
bool 	sf_erase(BYTE SFType);
bool	sf_def_erase(void);
bool	sf_read(unsigned int address, unsigned char *pData, unsigned long len);
bool	sf_write(unsigned int address, unsigned char *pData, unsigned long len);
void	sf_wait_ready(void);
void	sf_cmdread_status(void);
bool 	set_write_protect(bool bControl, unsigned short &WPAddr, BYTE &bWriteProtect);
bool	probe_mem(BYTE *pMemType);
bool	i2c_read(BYTE slaveID, BYTE len, BYTE addr, BYTE data[5]);
bool 	i2c_write(BYTE slaveID, BYTE len, BYTE addr, BYTE data[4]);
bool	meminfo_read(BYTE *pValue);
bool	meminfo_write(BYTE value);

#endif // _BURNERAP_LIB_H
