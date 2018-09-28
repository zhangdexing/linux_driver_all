// BurnerApLib.cpp : Defines the entry point for the DLL application.
//
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "BurnerApLib.h"
#include "..//common//misc.h"
#include "..//common//debug.h"

// #define DEBUG_ROM_DEF
#define SET_WP_NEW_VERSION

#define	SF_LEN			(64*1024)

const int				timeout = 3000;
static usb_dev_handle 	*s_udev = NULL;
static int				s_mode;
static ENUM_ROM_ID		s_rom_id;

BYTE SF_Write_Enable;
BYTE SF_Write_Command;

// the order of rom id sequence must be the SAME as the declaration of ENUM_ROM_ID in BurnerApLib.h
#define 	ROM_DEF_COUNT	28 // carol 2013/12/16 modify
ROM_DEF_S	ROM_Def[ROM_DEF_COUNT] = 
{
// shawn 2009/08/14 +++++
// 220 Series
{ ST50220B_001,  "ST50220B-001",  "SN9C21x" , 0x9FF8, {0x32,0x32,0x30,0x52,0x30,0x01,0x00,0x00}, 0x03C5, 40*1024, 0x0000, 0x0008, "", false, false, false},
{ ST50225A_001,  "ST50225A-001",  "SN9C21xA", 0x9FF8, {0x32,0x32,0x30,0x52,0x30,0x02,0x00,0x00}, 0x03C5, 40*1024, 0x0000, 0x0008, "", false, false, false},
{ ST50225E_002,  "ST50225E-002",  "SN9C21xB", 0x9FF8, {0x32,0x32,0x30,0x52,0x30,0x03,0x00,0x00}, 0x03C5, 40*1024, 0x0000, 0x0008, "", false, false, false},
{ ST50225E_1001, "ST50225E-1001", "SN9C213D", 0x9FF8, {0x32,0x32,0x35,0x52,0x30,0x03,0x00,0x00}, 0x031D, 40*1024, 0x0000, 0x01E8, "SN9C21X", false, false, false},
// 230 Series
{ ST50230A,      "ST50230A",      "SN9C23xA", 0xD2B2, {0x32,0x33,0x30,0x52,0x30,0x01,0x00,0x00}, 0x0530, 56*1024, 0x0000, 0x01E8, "SN9C230", false, false, false},
{ ST50230C_0002, "ST50230C-0002", "",         0xDFF8, {0x32,0x33,0x30,0x52,0x30,0x02,0x00,0x00}, 0x05E0, 56*1024, 0x0000, 0x01E8, "SN9C230", true, false, false},
{ ST50230C_0003, "ST50230C-0003", "",         0xDFF8, {0x32,0x33,0x30,0x52,0x30,0x03,0x00,0x03}, 0x05F0, 56*1024, 0x0000, 0x01E8, "SN9C231", true, false, false},
// 250 Series
{ ST50250A,      "ST50250A",      "",         0x9FF8, {0x32,0x35,0x30,0x52,0x30,0x01,0x00,0x00}, 0x0530, 40*1024, 0x0000, 0x01E8, "SN9C250", false, false, false},
{ ST50250B,      "ST50250B",      "ST5025xB", 0x9FF8, {0x32,0x35,0x30,0x52,0x30,0x02,0x00,0x00}, 0x05F0, 40*1024, 0x0000, 0x01E8, "SN9C250", false, false, false},
// 215 Series
{ STT5215,       "STT5215",       "",         0xBFF8, {0x32,0x31,0x35,0x52,0x30,0x01,0x00,0x00}, 0x05F0, 48*1024, 0x0000, 0x01E8, "SN9C215", false, false, false},
{ ST50215A_0001, "ST50215A-0001", "",         0xBFF8, {0x32,0x31,0x35,0x52,0x30,0x01,0x00,0x01}, 0x05F0, 48*1024, 0x0000, 0x01E8, "SN9C215", false, false, false},
// 236 Series
{ ST50236A_0001, "ST50236A-0001", "SN9C236",  0xBFF8, {0x32,0x33,0x36,0x52,0x30,0x01,0x00,0x00}, 0x0690, 48*1024, 0x0000, 0x01E8, "SN9C236", false, false, false},
{ ST50236B_0002, "ST50236B-0002", "SN9C236",  0xBFF8, {0x32,0x33,0x36,0x52,0x30,0x02,0x00,0x02}, 0x05F0, 48*1024, 0x0000, 0x01E8, "SN9C236", true, false, false},
// 231 Series
{ ST50231A_0001, "ST50231A-0001", "",         0xDFF8, {0x32,0x33,0x31,0x52,0x30,0x01,0x00,0x01}, 0x05F0, 56*1024, 0x0000, 0x01E8, "SN9C231", true, false, false},
// shawn 2009/08/14 add 2331
{ ST50231D_1001, "ST50231D-1001", "SN9C233",  0xBFF8, {0x32,0x33,0x31,0x52,0x30,0x02,0x10,0x01}, 0x05F0, 48*1024, 0x6000, 0x61E8, "SN9C233", true, true, false},
// 256 Series	// shawn 2009/08/14 modify
{ ST50256B_0001, "ST50256B-0001", "SN9C236D", 0xBFF8, {0x32,0x35,0x36,0x52,0x30,0x01,0x00,0x01}, 0x05F0, 48*1024, 0x0000, 0x01E8, "SN9C236D", true, true, false},
// shawn 2009/08/14 -----
{ SD50232A_2005, "SD50232B-2005", "SN9C232", 0xC7F8, {0x32,0x33,0x32,0x52,0x30,0x01,0x20,0x05}, 0x05F0, 56*1024, 0x0000, 0x0148, "SN9C232", true, true, true},		// shawn 2009/11/02 add 232
{ ST50275A_3005, "ST50275A-3005", "SN9C234", 0xBFF8, {0x32,0x37,0x35,0x52,0x30,0x01,0x30,0x05}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C234", true, true, true},		// shawn 2009/11/02 add 275
{ ST50276A_4006, "ST50276A-4006", "SN9C237", 0xBFF8, {0x32,0x37,0x36,0x52,0x30,0x01,0x40,0x06}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C237", true, true, true},		// shawn 2010/07/12 add 276
{ ST50290A_6007, "ST50290A-6007", "SN9C291", 0xBFF8, {0x32,0x39,0x30,0x52,0x30,0x01,0x60,0x07}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C291", true, true, true},		// shawn 2010/12/13 add 290
{ ST5L232A_5007, "ST5L232A-5007", "SN9C262", 0xC7F8, {0x32,0x33,0x32,0x52,0x30,0x02,0x50,0x07}, 0x05F0, 56*1024, 0x0000, 0x0148, "SN9C262", true, true, true},		// shawn 2011/07/25 add 262
{ ST5L286A_4119, "ST5L286A-4119", "SN9C234C", 0xBFF8, {0x32,0x38,0x36,0x52,0x30,0x01,0x41,0x19}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C234C", true, true, true},	// shawn 2011/07/25 add 286
{ ST5L288A_4221, "ST5L288A-4221", "SN9C234C/D", 0xBFF8, {0x32,0x38,0x38,0x52,0x30,0x01,0x42,0x21}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C234C/D", true, true, true},	// shawn 2011/07/25 add 288
{ ST5L292, "ST5L292", "ST5L292", 0x9FF8, {0x32,0x39,0x32,0x52,0x30,0x01,0x61,0x08}, 0x05F0, 48*1024, 0x0000, 0xA148, "SN9C292", true, true, true}, // carol 2013/08/29 add
{ ST5L271, "ST5L271", "ST5L271", 0xBFF8, {0x32,0x37,0x31,0x52,0x30,0x01,0x43,0x22}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C271", true, true,
true}, // carol 2013/10/30 add for 271
{ ST5L281, "ST5L281", "ST5L281", 0xBFF8, {0x32,0x38,0x31,0x52,0x30,0x01,0x45,0x23}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C281", true, true,
true}, // carol 2013/10/30 add for 281
{ ST50290_V2, "ST50290_V2", "SN9C291_V2", 0xBFF8, {0x32,0x39,0x30,0x52,0x30,0x02,0x61,0x20}, 0x05F0, 48*1024, 0x0000, 0x0148, "SN9C291_V2", true, true, true},
{ST5L283,       "ST5L283",       "ST5L283", 0xAFF8, {0x32,0x38,0x33,0x52,0x30,0x01,0x46,0x24},      0x05F0, 48*1024, 0x0000, 0xB148, "SN9C283", true, true, true}

};

static void debug_rom_def(ROM_DEF_S &rom);

static bool ext_asic_read(unsigned short addr, BYTE *pValue);
static bool ext_asic_write(unsigned short addr, BYTE value);
static bool	ext_rom_read(unsigned int address, unsigned char *pData, unsigned long len);
static bool ext_sf_def_erase(void);
static bool	ext_sf_read(unsigned int address, unsigned char *pData, unsigned long len);
static bool	ext_sf_write(unsigned int address, unsigned char *pData, unsigned long len);
static bool ext_probe_mem(BYTE *pMemType);
static bool	ext_i2c_read(BYTE slaveID, BYTE len, BYTE addr, BYTE *pValue);
static bool ext_i2c_write(BYTE slaveID, BYTE len, BYTE addr, BYTE *pValue);
static bool	ext_meminfo_read(BYTE *pValue);
static bool	ext_meminfo_write(BYTE value);


static bool vnd_asic_read(unsigned short addr, BYTE *pValue);
static bool vnd_asic_write(unsigned short addr, BYTE value);
static bool	vnd_rom_read(unsigned int address, unsigned char *pData, unsigned long len);
static bool vnd_sf_def_erase(void);
static bool	vnd_sf_read(unsigned int address, unsigned char *pData, unsigned long len);
static bool	vnd_sf_write(unsigned int address, unsigned char *pData, unsigned long len);
static bool vnd_probe_mem(BYTE *pMemType);
static bool	vnd_meminfo_read(BYTE *pValue);
static bool	vnd_meminfo_write(BYTE value);


#ifdef DEBUG_ROM_DEF
static void debug_rom_def(ROM_DEF_S &rom)
{
	DBG_Print("nID               : %d\n", rom.nID);
	DBG_Print("szInId            : %s\n", rom.szInId);
	DBG_Print("szOutId           : %s\n", rom.szOutId);
	DBG_Print("nVerAddr          : 0x%.4x\n", rom.nVerAddr);
	DBG_Print("szRomVer          : %s - 0x%.2x 0x%.2x 0x%.2x\n", rom.szRomVer, rom.szRomVer[5], rom.szRomVer[6], rom.szRomVer[7]);
	DBG_Print("nSFTypeAddr       : 0x%.4x\n", rom.nSFTypeAddr);
	DBG_Print("nBypass           : %u (0x%x)\n", (unsigned int)rom.nBypass, (unsigned int)rom.nBypass);
	DBG_Print("nSFOffset1        : 0x%.4x\n", rom.nSFOffset1);
	DBG_Print("nSFOffset2        : 0x%.4x\n", rom.nSFOffset2);
	DBG_Print("szSFVer           : %s\n", rom.szSFVer);
	DBG_Print("bIsSWErase        : %d\n", rom.bIsSWErase);			// shawn 2009/08/14 for 233
	DBG_Print("bIsDisSWWriteCmd  : %d\n", rom.bIsDisSWWriteCmd);	// shawn 2009/08/14 for 233
}
#else
static void debug_rom_def(ROM_DEF_S &rom) {}
#endif

// transfer initialization
bool 	init_xfer(usb_dev_handle *udev, int mode)
{
	assert(udev != NULL);
	s_udev = udev;
	
	if (mode != MODE_EXTENSION_UNIT && mode != MODE_VENDOR_COMMAND)
	{
		DBG_Print("init_xfer : mode = %d - Error!\n", mode);
		return false;
	}
	s_mode = mode;
	
	get_rom_id(s_rom_id);
	
	return true;
}

bool 	uninit_xfer(void)
{
	s_udev = NULL;
	return true;
}

int		rom_str_to_id(char *RomString)
{
	int		i;

	for (i=0; i<ROM_DEF_COUNT; i++)
	{
		if (_memicmp(RomString, ROM_Def[i].szRomVer, 4) == 0 && RomString[5] == ROM_Def[i].szRomVer[5])	// shawn 2010/11/08 modify
		{
			break;
		}
	}
	return (ROM_Def[i].nID);
}

bool ext_i2c_read(BYTE slaveID, BYTE len, BYTE addr, BYTE data[5])
{
	int	ret;
	BYTE	buf[8];
	
	TRACE("ext_i2c_read : slaveID = 0x%.2x, len = %d, addr = 0x%.4x\n", slaveID, len, addr);
	// Dummy write for setting address
	memset(buf, 0x00, 8);
	buf[0] = slaveID;
	buf[1] = len;
	buf[2] = addr;
	buf[7] = 0xff;
	ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0200, 0x0400, (char *)buf, 8, timeout);
	if (ret < 0)
	{
		DBG_Print("ext_i2c_read : dummy write, ret = 0x%x - Fail!\n", ret);
		return false;
	}
	// Read I2C
	buf[7] = 0x00;
	ret = usb_control_msg(s_udev, 0xA1, 0x81, 0x0200, 0x0400, (char *)buf, 8, timeout);
	if (ret < 0)
	{
		DBG_Print("ext_i2c_read : read i2c, ret = %d - Fail!\n", ret);
		return false;
	}

	for (int i=0; i<=7; i++)
		TRACE("ext_i2c_read : buf[%d] = 0x%.2x\n", i, buf[i]);

	memcpy(data, buf+2, 5);		
	return true;	
}

bool ext_i2c_write(BYTE slaveID, BYTE len, BYTE addr, BYTE data[4])
{
	int	ret;
	BYTE	buf[8];
	
	TRACE("ext_i2c_write() : slaveID = 0x%.2x, len = 0x%.2x, addr = 0x%.4x\n", slaveID, len, addr);
	memset(buf, 0x00, 8);
	buf[0] = slaveID;
	buf[1] = len;
	buf[2] = addr;
	buf[3] = data[0];
	buf[4] = data[1];
	buf[5] = data[2];
	buf[6] = data[3];
	buf[7] = 0x00;
	ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0200, 0x0400, (char *)buf, 8, timeout);
	if (ret < 0)
	{
		DBG_Print("ext_i2c_write : ret = 0x%x - Fail!\n", ret);
		return false;
	}
	return true;	
}

bool 	asic_read(unsigned short addr, BYTE *pValue)
{
	if (s_udev == NULL)
	{
		DBG_Print("asic_read : s_udev = NULL - return!\n");
		return false;
	}
		
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_asic_read(addr, pValue);
	return vnd_asic_read(addr, pValue);
}

bool 	asic_write(unsigned short addr, BYTE value)
{
	if (s_udev == NULL)
	{
		DBG_Print("asic_write : s_udev = NULL - return!\n");
		return false;
	}
	
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_asic_write(addr, value);
	return vnd_asic_write(addr, value);
}

bool	rom_read(unsigned int address, unsigned char *pData, unsigned long len)
{
	if (s_udev == NULL)
	{
		DBG_Print("rom_read : s_udev = NULL - return!\n");
		return false;
	}

	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_rom_read(address, pData, len);
	return vnd_rom_read(address, pData, len);
}

bool 	sf_def_erase(void)
{
	if (s_udev == NULL)
	{
		DBG_Print("sf_def_erase : s_udev = NULL - return!\n");
		return false;
	}

	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_sf_def_erase();
	return vnd_sf_def_erase();	
}

bool	sf_read(unsigned int address, unsigned char *pData, unsigned long len)
{
	if (s_udev == NULL)
	{
		DBG_Print("sf_read : s_udev = NULL - return!\n");
		return false;
	}
	
	if(address >= 0x10000) // carol 2013/08/29 add
		s_mode = MODE_EXTENSION_UNIT;
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_sf_read(address, pData, len);
	return vnd_sf_read(address, pData, len);	
}

bool	sf_write(unsigned int address, unsigned char *pData, unsigned long len)
{
	if (s_udev == NULL)
	{
		DBG_Print("sf_write : s_udev = NULL - return!\n");
		return false;
	}
	
	if(address >= 0x10000)
		s_mode = MODE_EXTENSION_UNIT; // carol 2013/08/29 add
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_sf_write(address, pData, len);
	return vnd_sf_write(address, pData, len);
}

static bool 	ext_asic_read(unsigned short addr, BYTE *pValue)
{
	int		ret;
	BYTE	data[4];
	
	data[0] = (BYTE) addr;	
	data[1] = (BYTE) (addr >> 8);
	data[2] = 0x00;
	data[3] = 0xff;
	ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0100, 0x0400, (char *)data, 4, timeout);
	if (ret < 0)
	{
		DBG_Print("ext_asic_read : dummy write - Fail!, ret = %d\n", ret);
		return false;
	}

	ret = usb_control_msg(s_udev, 0xA1, 0x81, 0x0100, 0x0400, (char *)data, 4, timeout);
	if (ret < 0)
	{
		DBG_Print("ext_asic_read : read - Fail!, ret = %d\n", ret);
		return false;
	}

	// DBG_Print("0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x\n", data[0], data[1], data[2], data[3]);
	*pValue = data[2];
	return true;	
}

static bool 	vnd_asic_read(unsigned short addr, BYTE *pValue)
{
	int		ret;
	BYTE	data;
	
	ret = usb_control_msg(s_udev, 0xc1, 0x08, addr, 0x0c45, (char *)&data, 1, timeout);	// shawn 2009/08/14 for 233
	if (ret < 0)
	{
		DBG_Print("vnd_asic_read : ret = %d - Fail!\n", ret);
		return false;
	}

	// TRACE("vnd_asic_read : addr = 0x%.4x, data = 0x%.2x\n", addr, data);
	*pValue = data;
	return true;	
}

static bool 	ext_asic_write(unsigned short addr, BYTE value)
{
	int	ret;
	BYTE	data[8];
	
	// TRACE("ext_asic_write() : address = 0x%.4x, value = 0x%.2x\n", addr, value);
	data[0] = (BYTE) addr;	
	data[1] = (BYTE) (addr >> 8);
	data[2] = value;
	data[3] = 0x00;
	ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0100, 0x0300/*0x0400*/, (char *)data, 4, timeout); // carol
	if (ret < 0)
	{
		DBG_Print("ext_asic_write : ret = 0x%x - Fail!\n", ret);
		return false;
	}
	return true;	
}

static bool 	vnd_asic_write(unsigned short addr, BYTE value)
{
	int	ret;

	ret = usb_control_msg(s_udev, 0x41, 0x08, addr, 0x0c45, (char *)&value, 1, timeout);
	if (ret < 0)
	{
		DBG_Print("vnd_asic_write : ret = %d - Fail!\n", ret);
		return false;
	}
	return true;	
}

static bool 	ext_sf_def_erase(void)
{
	int		ret;
	BYTE	data[11];
	
	// TRACE("ext_sf_def_erase()\n");
	memset(data, 0x00, 11);
	data[2] = 0xc8;
	ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0300, 0x0400, (char *)data, 11, timeout);
	if (ret < 0)
	{
		DBG_Print("ext_sf_def_erase : ret = %d - Fail!\n", ret);
		return false;
	}
	return true;
}

static bool 	vnd_sf_def_erase(void)
{
	int		ret;	
	
	ret = usb_control_msg(s_udev, 0x41, 0x05, 0x1688, 0x0c45, (char *)NULL, 0, timeout);
	if (ret < 0)
	{
		DBG_Print("vnd_sf_def_erase : ret = %d - Fail!\n", ret);
		return false;
	}
	return true;
}

static bool	ext_rom_read(unsigned int address, unsigned char *pData, unsigned long len)
{
	unsigned long	offset;
	BYTE	n;
	BYTE	data[11];
	int		ret;
	
	offset = 0; n = 0;
	while (len > 0)
	{
		if (len <= 8)
		{
			n = len;
			len = 0;
		}
		else
		{
			n = 8;
			len -= 8;
		}

		memset(data, 0x00, 11);
		data[0] = (BYTE) (address);
		data[1] = (BYTE) (address >> 8);
		data[2] = 8;
		ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0400, 0x0300/*0x0400*/, (char *)data, 11, timeout); // carol 2013/08/29 modify
		if (ret < 0)
		{
			DBG_Print("ext_rom_read : dummy write - Fail!, ret = %d\n", ret);
			s_mode = MODE_VENDOR_COMMAND; // carol 2013/08/29 add
			return false;
		}
		data[2] = n;
		ret = usb_control_msg(s_udev, 0xA1, 0x81, 0x0400, 0x0300/*0x0400*/, (char *)data, 11, timeout);  // carol 2013/08/29 modify
		if (ret < 0)
		{
			DBG_Print("ext_rom_read : read - Fail!, ret = %d\n", ret);
			s_mode = MODE_VENDOR_COMMAND; // carol 2013/08/29 add
			return false;
		}
		memcpy(pData+offset, data+3, n);
		offset += n;
	}
	s_mode = MODE_VENDOR_COMMAND; // carol 2013/08/29 add
	
	return true;
}

static bool	vnd_rom_read(unsigned int address, unsigned char *pData, unsigned long len)
{
	unsigned long	offset, nread;
	int		ret;
	
	offset = 0; nread = 0;
	while (len > 0)
	{
		if (len <= 64)
		{
			nread = len;
			len = 0;
		}
		else
		{
			nread = 64;
			len -= 64;
		}
		ret = usb_control_msg(s_udev, 0xc1, 0x04, address+offset, 0x0c45, (char *)(pData+offset), nread, timeout);
		if (ret < 0)
		{
			DBG_Print("vnd_rom_read : ret = %d - Fail!\n", ret);
			return false;
		}
		offset += nread;
	}
	
	return true;
}

static bool	ext_sf_write(unsigned int address, unsigned char *pData, unsigned long len)
{
	unsigned long	offset;
	BYTE	n;
	BYTE	data[11];
	int		ret;
	
	offset = 0; n = 0;
	while (len > 0)
	{
		if (len <= 8)
		{
			n = len;
			len = 0;
		}
		else
		{
			n = 8;
			len -= 8;
		}
		memset(data, 0x00, 11);
		data[0] = (BYTE) (address + offset);	
		data[1] = (BYTE) ((address + offset) >> 8);
		if(address<0x10000) // carol
			data[2] = n;
		else
			data[2] = 0x18;
		
		memcpy(data+3, pData+offset, n);			
		ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0300, /*0x0400*/0x0300, (char *)data, 11, timeout);
		if (ret < 0)
		{
			DBG_Print("ext_sf_write : ret = %d - Fail!\n", ret);
			s_mode = MODE_VENDOR_COMMAND; // carol temp add
			return false;
		}
		offset += n;
		s_mode = MODE_VENDOR_COMMAND; // carol temp add
	}
	
	return true;
}

static bool	vnd_sf_write(unsigned int address, unsigned char *pData, unsigned long len)
{
	unsigned long	offset, n, eachlen;
	int		ret;
	
	offset = 0; n = 0; eachlen = 0;
	
	unsigned char byManufacturerID = 0;
	unsigned char byMemType = 0;
		
	asic_write(0x1080, 0x1);
	asic_write(0x1091, 0x0);
	asic_write(0x1082, 0x9F);
	asic_write(0x1081, 0x01);
	sf_wait_ready();

	asic_write(0x1083, 0x0);
	asic_write(0x1081, 0x02);
	sf_wait_ready();

	asic_read(0x1083, &byManufacturerID);
	TRACE("ManufactureID = %x\n", byManufacturerID);
		
	asic_write(0x1083, 0x0);
	asic_write(0x1081, 0x02);
	sf_wait_ready();
	asic_read(0x1083, &byMemType);
	TRACE("Mem Type = %x\n", byMemType);
		
	asic_write(0x1080, 0x0);

	if ( (byManufacturerID == MANUFACTURER_ID_MXIC) && (byMemType == MEM_TYPE_32) )
		eachlen = 32;
	else
		eachlen = 64;
		
	if (address % eachlen)
	{
		unsigned char byTmp = 0;
		
		byTmp = eachlen - (address % eachlen);

		if (byTmp >= len)
		{
			byTmp = (unsigned char)len;
			len = 0;
		}
		else
		{
			len -= byTmp;
		}
		
		ret = usb_control_msg(s_udev, 0x41, 0x03, address, 0x0c45, (char *)(pData), (unsigned long)byTmp, timeout);
		
		if (ret < 0)
		{
			DBG_Print("vnd_sf_write : ret = %d - Fail!\n", ret);
			return false;
		}
		
		offset = byTmp;
	}
	
	while (len > 0)
	{
		if (len <= eachlen)
		{
			n = len;
			len = 0;
		}
		else
		{
			n = eachlen;
			len -= eachlen;
		}
	
		ret = usb_control_msg(s_udev, 0x41, 0x03, address+offset, 0x0c45, (char *)(pData+offset), n, timeout);
		if (ret < 0)
		{
			DBG_Print("vnd_sf_write : ret = %d - Fail!\n", ret);
			return false;
		}
		offset += n;
	}
	
	return true;
}
static bool	ext_meminfo_read(BYTE *pValue)
{
	return true;
}

static bool	ext_meminfo_write(BYTE value)
{
	return true;
}

static bool	vnd_meminfo_read(BYTE *pValue)
{
	int		ret;
	BYTE	data;
	
	ret = usb_control_msg(s_udev, 0xc1, 0x07, 0x0000, 0x0c45, (char *)&data, 1, timeout);	// shawn 2009/08/14 for 233
	if (ret < 0)
	{
		DBG_Print("vnd_meminfo_read : ret = %d - Fail!\n", ret);
		return false;
	}

	// TRACE("vnd_meminfo_read : data = 0x%.2x\n", data);
	*pValue = data;
	return true;	
}

static bool	vnd_meminfo_write(BYTE value)
{
	int	ret;

	// TRACE("vnd_meminfo_write : value = 0x%.2x\n", value);
	ret = usb_control_msg(s_udev, 0x41, 0x07, 0X0000, 0x0c45, (char *)&value, 1, timeout);	// shawn 2009/08/14 for 233
	if (ret < 0)
	{
		DBG_Print("vnd_meminfo_write : ret = %d - Fail!\n", ret);
		return false;
	}
	return true;
}

static bool ext_probe_mem(BYTE *pMemType)
{
	return true;
}

static bool vnd_probe_mem(BYTE *pMemType)
{
	BYTE	temp;

	temp = 0x00;
	if (!meminfo_write(temp)) 
	{
		DBG_Print("vnd_probe_mem : Fail! - meminfo_write()\n");
		return false;
	}

	if (!meminfo_read(pMemType)) 
	{
		DBG_Print("vnd_probe_mem : Fail! - meminfo_read()\n");
		return false;
	}

	return true;
}

static bool	ext_sf_read(unsigned int address, unsigned char *pData, unsigned long len)
{
	unsigned long	offset;
	BYTE	n;
	BYTE	data[11];	
	int		ret;
	
	offset = 0; n = 0;
	while (len > 0)
	{
		if (len <= 8)
		{
			n = len;
			len = 0;
		}
		else
		{
			n = 8;
			len -= 8;
		}
		memset(data, 0x00, 11);
		// dummy write
		data[0] = (BYTE) (address + offset);	
		data[1] = (BYTE) ((address + offset) >> 8);
		if(address < 0x10000) // carol
			data[2] = 0x88;
		else
			data[2] = 0x98;
		ret = usb_control_msg(s_udev, 0x21, 0x01, 0x0300, 0x0300/*0x0400*/, (char *)data, 11, timeout);
		if (ret < 0)
		{
			DBG_Print("ext_sf_read : dummy write - Fail!, ret = %d\n", ret);
			s_mode = MODE_VENDOR_COMMAND; // carol temp add
			return false;
		}
		data[2] = n;
		ret = usb_control_msg(s_udev, 0xA1, 0x81, 0x0300, 0x0300/*0x0400*/, (char *)data, 11, timeout); 
		if (ret < 0)
		{
			DBG_Print("ext_sf_read : read - Fail!, ret = %d\n", ret);
			s_mode = MODE_VENDOR_COMMAND; // carol temp add
			return false;
		}
		memcpy(pData+offset, data+3, n);
		offset += n;
	}
	
	s_mode = MODE_VENDOR_COMMAND; // carol temp add

	return true;
}

static bool	vnd_sf_read(unsigned int address, unsigned char *pData, unsigned long len)
{
	unsigned long	offset, n;
	int		ret;
	
	offset = 0; n = 0;
	while (len > 0)
	{
		if (len <= 64)
		{
			n = len;
			len = 0;
		}
		else
		{
			n = 64;
			len -= 64;
		}
		ret = usb_control_msg(s_udev, 0xc1, 0x03, address+offset, 0x0c45, (char *)(pData+offset), n, timeout);
	
		if (ret < 0)
		{
			DBG_Print("vnd_sf_read : read - Fail!, ret = %d\n", ret);
			return false;
		}

		offset += n;
	}
	
	return true;
}

#ifdef SET_WP_NEW_VERSION			
bool 	set_write_protect(bool bControl, unsigned short &WPAddr, BYTE &bWriteProtect)
{
	// shawn 2008/08/07 add
	BYTE 	iFlashType;
	BYTE 	VALUE;	// shawn 2008/10/16 add
	
	get_flash_type(iFlashType);
		
	// shawn 2009/08/14 for 233 +++++
	ENUM_ROM_ID	id;
	
	if (s_udev == NULL)
	{
		DBG_Print("set_write_protect : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (!get_rom_id(id))
	{
		DBG_Print("set_write_protect : get_rom_id() - Fail!");
		return false;
	}
	// shawn 2009/08/14 for 233 -----
	
	if (bControl)	// enable write protect
	{
		
		if (ROM_Def[id].bIsDisSWWriteCmd)
		{
			asic_write(0x05F3, SF_Write_Enable);
			asic_write(0x05F5, SF_Write_Command);
			
			// shawn 2008/08/08 add for writing Key to disable SF write
			//                  ( if key1 != ~ key2, disable write )
			asic_write(0x05F8, 0x34);
			asic_write(0x05F9, 0x34);
			asic_write(0x05FA, 0x34);
			asic_write(0x05FB, 0x34);
		}
		
		BYTE bSizeTmp, bSF_BPTmp;

		if (ROM_Def[id].bIsGeneric)
		{
			sf_read(0xc003, &bSizeTmp, 1);
			sf_read(0xc034, &bSF_BPTmp, 1);
		}
		else
		{
			sf_read(0x8003, &bSizeTmp, 1);
			sf_read(0x8034, &bSF_BPTmp, 1);
		}
		
		bSF_BPTmp = bSF_BPTmp>>7;
		
		if ( ((bSizeTmp >= 0x35) && (bSF_BPTmp)) || id == ST50231D_1001 )
		{
			// james 2008/07/10  Enable write protect @ flash status register (bp0, bp1)
			asic_write(0x1080, 0x1);
			asic_write(0x1091, 0x0);

			asic_write(0x1082, 0x06);
			asic_write(0x1081, 0x1);
			sf_wait_ready();
			asic_write(0x1091, 0x1);
			sf_cmdread_status();

			// shawn 2008/12/08 add
			if (iFlashType == SF_TYPE_SST)
			{
				asic_write(0x1091, 0x0);
				asic_write(0x1082, 0x50);	// Enable-Write-Status-Register(EWSR)
				asic_write(0x1081, 0x1);			
				sf_wait_ready();
				asic_write(0x1091, 0x1);
			}

			asic_write(0x1091, 0x0);
			asic_write(0x1082, 0x1);
			asic_write(0x1081, 0x1);
			sf_wait_ready();
			asic_write(0x1082, 0x8c);	// shawn 2010/02/08 modify
			asic_write(0x1081, 0x1);
			sf_wait_ready();
			asic_write(0x1091, 0x1);
			sf_cmdread_status();

			asic_write(0x1080, 0x0);
		}
		// shawn 2010/07/12 modify -----
		
		if (WPAddr != 0xFFFF)
			asic_write(WPAddr, ~bWriteProtect);	// shawn 2010/07/12 modify for enable HW WP

		// carol 2013/08/29 add for back to HW default +++++
		asic_write(0x1007, 0);
		asic_write(0x1006, 0);
	
		BYTE byTmp = 0;	
		asic_read(0x1073, &byTmp);
		byTmp &= 0xFE;
		asic_write(0x1073, byTmp);
		// carol 2013/08/29 add for back to HW default -----

	}
	else			// disable write protect
	{
		// shawn 2009/08/14 for 233 +++++
		/*if (id == ST50231D_1001)
		{
			if ( !probe_mem(&VALUE) )
			{
				DBG_Print("set_write_protect : probe_mem() for 233\n");
				return false;
			}
			if ( VALUE == 2 )	// EE
			{
				BYTE EEPROM_Write_En;
				
				asic_read(0x1000, &EEPROM_Write_En);
				EEPROM_Write_En |= 0x40;	// set bit 6 to 1
				asic_write(0x1000, EEPROM_Write_En);
				usleep(10000);
				return true;
			}
		}*/
		
		if (ROM_Def[id].bIsDisSWWriteCmd)
		{
			asic_read(0x05F3, &SF_Write_Enable);
			asic_write(0x05F3, 0x06);			// SF write enable	

			// shawn 2008/08/08 add for writing Key to enable SF write
			//                  ( if key1 = ~ key2, enable write )
			asic_write(0x05F8, 0x12);
			asic_write(0x05F9, 0x12);
			asic_write(0x05FA, 0xED);
			asic_write(0x05FB, 0xED);

			asic_read(0x05F5, &SF_Write_Command);
			if ( iFlashType == SF_TYPE_SST )
				asic_write(0x05F5, 0xAF);		//	SF write command for SST
			else
				asic_write(0x05F5, 0x02);		//	SF write command for others
		}
		// shawn 2009/08/14 for 233 -----

		BYTE bWriteProtectGPIO=0;
		WPAddr=0xFFFF;

		bWriteProtectGPIO = (bWriteProtect>>4)&0x7;	// shawn 2008/10/15 modify to fix influence of bSF_BP
		bWriteProtect = bWriteProtect&0x0F;

		if (bWriteProtect!=0) // this firmware supports Write Protect
		{
			if (bWriteProtect == 1) // High active protect
			{
				DBG_Print("set_write_protect : WP High active...\n");
				if (bWriteProtectGPIO<8)
				{
					bWriteProtect = 1<<bWriteProtectGPIO;
					asic_write(0x1007, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					bWriteProtect = ~bWriteProtect;
					asic_write(0x1006, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					WPAddr = 0x1006;
				}
				else
				{
					bWriteProtect = 1<<(bWriteProtectGPIO-8);
					asic_write(0x100A, bWriteProtect);
					bWriteProtect = ~bWriteProtect;
					asic_write(0x1009, bWriteProtect);
					WPAddr = 0x1009;
				}
			}
			else	// Low active protect
			{
				DBG_Print("set_write_protect : WP low active...\n");
				if (bWriteProtectGPIO<8)
				{
					bWriteProtect = 1<<bWriteProtectGPIO;
					DBG_Print("set_write_protect : GPIO = %x\n", bWriteProtect);
					asic_write(0x1007, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					asic_write(0x1006, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					WPAddr = 0x1006;
				}
				else
				{
					bWriteProtect = 1<<(bWriteProtectGPIO-8);
					asic_write(0x100A, bWriteProtect);
					asic_write(0x1009, bWriteProtect);
					WPAddr = 0x1009;
				}
			}
		}
			
		// james 2007/08/16  disable write protect @ flash status register (bp0, bp1)
		asic_write(0x1080,0x1);
		asic_write(0x1091,0x0);

		asic_write(0x1082,0x06);
		asic_write(0x1081,0x1);
		sf_wait_ready();
		asic_write(0x1091,0x1);
		sf_cmdread_status();

		// shawn 2008/12/08 add
		if (iFlashType == SF_TYPE_SST)
		{
			asic_write(0x1091,0x0);
			asic_write(0x1082,0x50);	// Enable-Write-Status-Register(EWSR)
			asic_write(0x1081,0x1);			
			sf_wait_ready();
			asic_write(0x1091,0x1);
		}

		asic_write(0x1091,0x0);
		asic_write(0x1082,0x1);
		asic_write(0x1081,0x1);
		sf_wait_ready();
		asic_write(0x1082,0x0);
		asic_write(0x1081,0x1);
		sf_wait_ready();
		asic_write(0x1091,0x1);
		sf_cmdread_status();

		asic_write(0x1080,0x0);
	}

	return true;
}

#else
bool 	set_write_protect(bool bControl, unsigned short &WPAddr, BYTE &bWriteProtect)
{
	// TRACE("set_write_protect() : bControl = %d, WPAddr = 0x%.4x, bWriteProtect = 0x%.2x\n",	bControl, WPAddr, bWriteProtect);	
	if (s_udev == NULL)
	{
		DBG_Print("set_write_protect : s_udev = NULL - return!\n");
		return false;
	}
	
	if (bControl)	// enable write protect
	{
		// 加新 code
		 
		if (WPAddr != 0xFFFF)
			asic_write(WPAddr, bWriteProtect);
	}
	else			// disable write protect
	{
		BYTE bWriteProtectGPIO = 0;
		//bWriteProtect=0;
		WPAddr = 0xFFFF;
		
		// 加新 code
		 
		// Open write protect address 0x8034
		//ReqToDriver(SF_READ, 0x8034, &bWriteProtect, 1); // james 2007/10/11 get 0x8034 from outside
		bWriteProtectGPIO = bWriteProtect >> 4;
		bWriteProtect = bWriteProtect & 0x0F;

		if (bWriteProtect != 0) // this firmware supports Write Protect
		{
			if (bWriteProtect == 1) // High active protect
			{
				DBG_Print("set_write_protect : WP High active...\n");
				if (bWriteProtectGPIO < 8)
				{
					bWriteProtect = 1 << bWriteProtectGPIO;
					asic_write(0x1007, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					bWriteProtect = ~bWriteProtect;
					asic_write(0x1006, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					WPAddr = 0x1006;
				}
				else
				{
					bWriteProtect = 1 << (bWriteProtectGPIO-8);
					asic_write(0x100A, bWriteProtect);
					bWriteProtect = ~bWriteProtect;
					asic_write(0x1009, bWriteProtect);
					WPAddr = 0x1009;
				}
			}
			else	// Low active protect
			{
				DBG_Print("set_write_protect : WP low active...\n");
				DBG_Print("set_write_protect : bWriteProtectGPIO = %d\n", bWriteProtectGPIO);
				if (bWriteProtectGPIO < 8)
				{
					bWriteProtect = 1 << bWriteProtectGPIO;
					DBG_Print("set_write_protect : GPIO = %x", bWriteProtect);
					asic_write(0x1007, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					asic_write(0x1006, 0xFF/*bWriteProtect*/); // carol 2013/08/29 modify
					WPAddr = 0x1006;
				}
				else
				{
					bWriteProtect = 1<<(bWriteProtectGPIO-8);
					asic_write(0x100A, bWriteProtect);
					asic_write(0x1009, bWriteProtect);
					WPAddr = 0x1009;
				}
			}
		}
		// james 2007/08/16  disable write protect @ flash status register (bp0, bp1)
		asic_write(0x1080,0x01);
		asic_write(0x1091,0x00);

		asic_write(0x1082,0x06);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		asic_write(0x1091,0x01);
		sf_cmdread_status();

		asic_write(0x1091,0x00);
		asic_write(0x1082,0x01);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		asic_write(0x1082,0x00);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		asic_write(0x1091,0x01);
		sf_cmdread_status();

		asic_write(0x1080,0x00);
	}

	return true;
}
#endif


bool 	sf_erase(BYTE SFType)
{
	bool bRet = false;
	ENUM_ROM_ID	id;
	
	if (s_udev == NULL)
	{
		DBG_Print("sf_erase : s_udev = NULL - return!\n");
		return false;
	}
	
	if (!get_rom_id(id))
	{
		DBG_Print("sf_erase : get_rom_id() - Fail!");
		return false;
	}

	switch (SFType)
	{
	case SF_TYPE_WINBOND:
	case SF_TYPE_PMC:
	case SF_TYPE_ST:
	case SF_TYPE_AMIC:
	case SF_TYPE_EON:
		DBG_Print(("SW erase(1)!"));
		asic_write(0x1080,0x1);
		//SF_Set_WEL_Bit
		asic_write(0x1091,0x0);
		asic_write(0x1082,0x06);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		asic_write(0x1091,0x1);
		//chip erase
		asic_write(0x1091,0x0);
		asic_write(0x1082,0xc7);	// for PMC chip erase
		//SetRegData(0x1082,0xc7);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		// shawn 2010/11/12 modify for fixing "No such device" issue +++++
		bRet = asic_write(0x1091,0x1);
        //yiling 2014/05/06 add sf_cmdread_status() to wait erase progress finish
		//SF_CMDread_Status		
		sf_cmdread_status();
		bRet = asic_write(0x1080,0x0);

		// shawn 2010/11/12 modify for fixing "No such device" issue -----
		break;
	case SF_TYPE_SST:
		if (ROM_Def[id].bIsSWErase)
		{
			DBG_Print(("SW erase(2)!"));
			asic_write(0x1080,0x1);	// serial flash mode
			//SF_Set_WEL_Bit
			asic_write(0x1091,0x0);	// chip select
			asic_write(0x1082,0x06);	// write data
			asic_write(0x1081,0x01);	// trigger for write
			sf_wait_ready();
			asic_write(0x1091,0x1);
			//chip erase
			asic_write(0x1091,0x0);
			asic_write(0x1082,0x60);
			asic_write(0x1081,0x01);
			sf_wait_ready();
			// shawn 2010/11/12 modify for fixing "No such device" issue +++++
			bRet = asic_write(0x1091,0x1);
            //yiling 2014/05/06 add sf_cmdread_status() to wait erase progress finish
			//SF_CMDread_Status		
			sf_cmdread_status();
			bRet = asic_write(0x1080,0x0);
			// shawn 2010/11/12 modify for fixing "No such device" issue -----
		}
		else
		{
			TRACE("request FW erase!");
			bRet = sf_def_erase();
		}
		break;
	case SF_TYPE_UNKNOWN:
	case SF_TYPE_MXIC:	
	case SF_TYPE_ATMEL_AT25F:
	case SF_TYPE_ATMEL_AT25FS:
	case SF_TYPE_ATMEL_AT45DB:
	case SF_TYPE_MXIC_LIKE:
	default:
		if (ROM_Def[id].bIsSWErase)
		{
			DBG_Print(("SW erase(3)!"));
			asic_write(0x1080,0x1);
			//SF_Set_WEL_Bit
			asic_write(0x1091,0x0);
			asic_write(0x1082,0x06);
			asic_write(0x1081,0x01);
			sf_wait_ready();
			asic_write(0x1091,0x1);
			//chip erase
			asic_write(0x1091,0x0);
			asic_write(0x1082,0x60);
			//SetRegData(0x1082,0xc7);
			asic_write(0x1081,0x01);
			sf_wait_ready();
			
			// shawn 2010/11/12 modify for fixing "No such device" issue +++++
			bRet = asic_write(0x1091,0x1);
            //yiling 2014/05/06 add sf_cmdread_status() to wait erase progress finish
			//SF_CMDread_Status		
			sf_cmdread_status();
			bRet = asic_write(0x1080,0x0);
			// shawn 2010/11/12 modify for fixing "No such device" issue -----
		}
		else
		{
			DBG_Print(("request FW erase!"));
			bRet = sf_def_erase();
		}
		break;
	}

	// carol 2013/08/29 add +++++ // carol 2013/10/09 mark +++++
	/*if( (usb_reset(s_udev)) == 0)
		DBG_Print("reset success\n");
	else
		DBG_Print("reset fail\n");
	*/
	// carol 2013/08/29 add -----  // carol 2013/10/09 mark -----
	return bRet;
}

bool	pull_cpu_rate(bool bControl)
{
	BYTE	uiCPU_RATE;
	
	if (s_udev == NULL)
	{
		DBG_Print("pull_cpu_rate : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (bControl)
	{
		asic_read(0x1002, &uiCPU_RATE);
		uiCPU_RATE &= 0xFC;
		asic_write(0x1002, uiCPU_RATE);
	}
	else
	{
		asic_read(0x1002, &uiCPU_RATE);
		uiCPU_RATE |= 0x01;
		asic_write(0x1002, uiCPU_RATE);
	}

	return true;
}

void	sf_cmdread_status(void)
{
	int	i;
	unsigned char data[1];

	if (s_udev == NULL)
	{
		DBG_Print("sf_cmdread_status : s_udev = NULL - return!\n");
		return;
	}	
	
	for (i=0; i<10000; i++)
	{
		asic_write(0x1091, 0x00);
		asic_write(0x1082, 0x05);
		asic_write(0x1081, 0x01);
		sf_wait_ready();
		
		asic_write(0x1083, 0x00);
		asic_write(0x1081, 0x02);
		sf_wait_ready();
		
		asic_read(0x1083, &data[0]);
		if ((data[0]&0x01)!=0x01)
		{
			asic_write(0x1091, 0x01);
			break;
		}
		sleep(1);
	}	
}

void	sf_wait_ready(void)
{
	int	i;
	unsigned char data;

	if (s_udev == NULL)
	{
		DBG_Print("sf_wait_ready : s_udev = NULL - return!\n");
		return;
	}	
		
	for (i=0; i<1000; i++)
	{
		asic_read(0x1084, &data);
		if (data == 1)
		{
			break;
		}
		sleep(1);
	}	
}

bool 	set_flash_type(int iFlashType)
{
	if (s_udev == NULL)
	{
		DBG_Print("set_flash_type : s_udev = NULL - return!\n");
		return false;
	}	
	
	switch (iFlashType)
	{
	case SF_TYPE_SST:
		DBG_Print("SetFlashType : case SF_TYPE_SST");
		// initialize registers for SST serial flash
		asic_write(0x03C5, SF_TYPE_SST); // SF_Type = SST
		asic_write(0x1080, 0x00); 		// EXTRA_SEL = 0, PAGE_SIZE = 0
		asic_write(0x1090, 0x02); 		// SPEED = 2
		asic_write(0x108C, 0x00); 		// DUMMY_CLK = 0
		asic_write(0x108D, 0x07); 		// CS_HI_TIME = 0
		asic_write(0x108E, 0x07); 		// CS_SET_TIME = 0
		asic_write(0x108F, 0x07); 		// CS_HOLD_TIME = 0

		asic_write(0x1080,0x1);			// serial flash mode
		//SF_Set_EWSR_Bit
		asic_write(0x1091,0x0);			// chip select
		asic_write(0x1082,0x50);		// write data
		asic_write(0x1081,0x01);		// trigger for write
		sf_wait_ready();
		asic_write(0x1091,0x1);
		//chip erase
		asic_write(0x1091,0x0);
		asic_write(0x1082,0x01);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		asic_write(0x1082,0x00);
		asic_write(0x1081,0x01);
		sf_wait_ready();
		asic_write(0x1091,0x1);
		//SF_CMDread_Status
		sf_cmdread_status();
		asic_write(0x1080,0x0);

		asic_write(0x1087, 0x05); 		// ISP_STATUS_CMD = 0x05
		asic_write(0x1088, 0x03); 		// ISP_READ_CMD = 0x03
		asic_write(0x1092, 0x01); 		// STATUS_RDY = 1
		asic_write(0x1093, 0x00); 		// RDY_BIT = 0
		break;
	case SF_TYPE_PMC:
		DBG_Print("SetFlashType : case SF_TYPE_PMC");
		// initialize registers for SST serial flash
		asic_write(0x03C5, SF_TYPE_PMC); // SF_Type = SST
		asic_write(0x1080, 0x00); 		// EXTRA_SEL = 0, PAGE_SIZE = 0
		asic_write(0x1090, 0x02); 		// SPEED = 2
		asic_write(0x108C, 0x00); 		// DUMMY_CLK = 0
		asic_write(0x108D, 0x07); 		// CS_HI_TIME = 0
		asic_write(0x108E, 0x07); 		// CS_SET_TIME = 0
		asic_write(0x108F, 0x07); 		// CS_HOLD_TIME = 0

		asic_write(0x1087, 0x05); 		// ISP_STATUS_CMD = 0x05
		asic_write(0x1088, 0x03); 		// ISP_READ_CMD = 0x03
		asic_write(0x1092, 0x01); 		// STATUS_RDY = 1
		asic_write(0x1093, 0x00); 		// RDY_BIT = 0
		break;
	default:
		DBG_Print("SetFlashType : case default");
		break;
	}
	return true;
}

bool	get_rom_id(ENUM_ROM_ID &nID)
{
	char 	RomVer[16];
	int		i;
	bool	bRet = false;

	if (s_udev == NULL)
	{
		DBG_Print("get_rom_id : s_udev = NULL - return!\n");
		return false;
	}	

	if (!get_rom_version(RomVer))
	{
		DBG_Print("get_rom_id : get_rom_version() - Fail!\n");
		return false;
	}
	
	for (i=0; i<ROM_DEF_COUNT; i++)
	{
		if (_memicmp(RomVer, ROM_Def[i].szRomVer, 4) == 0 && RomVer[5] == ROM_Def[i].szRomVer[5])	// shawn 2010/11/08 modify
		{
			nID = ROM_Def[i].nID;
			bRet = true;
			break;
		}
	}
	
	return bRet;
}

bool 	get_code_version(char szFlashCodeVer[LEN_CODE_VER])
{
	ENUM_ROM_ID	id;

	if (s_udev == NULL)
	{
		DBG_Print("get_code_version : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (!get_rom_id(id))
	{
		DBG_Print("get_code_version : get_rom_id() - Fail!");
		return false;
	}

	if (id == ST50220B_001 || id == ST50225A_001 || id == ST50225E_002)
	{
		sf_read(0x0000, (unsigned char *)szFlashCodeVer, 16);
	}
	else
	{
		sf_read(ROM_Def[id].nSFOffset1, (unsigned char *)(szFlashCodeVer),    8);
		sf_read(ROM_Def[id].nSFOffset2, (unsigned char *)(szFlashCodeVer+7), 16);
	}

	return true;
}

bool 	get_vendor_version(char szFlashVendorVer[LEN_VENDOR_VER])
{
	ENUM_ROM_ID	id;

	if (s_udev == NULL)
	{
		DBG_Print("get_vendor_version : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (!get_rom_id(id))
	{
		DBG_Print("get_vendor_version : get_rom_id() - Fail!");
		return false;
	}
    if(id==ST5L292){
        sf_read(0x154, (unsigned char *)szFlashVendorVer, 12);
    }
	return true;
}

bool 	get_fw_version(char szFwVer[LEN_FW_VER])
{
	ENUM_ROM_ID	id;

	if (s_udev == NULL)
	{
		DBG_Print("get_fw_version : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (!get_rom_id(id))
	{
		DBG_Print("get_fw_version : get_rom_id() - Fail!");
		return false;
	}
    sf_read(0x201C, (unsigned char *)szFwVer, 10);
	return true;
}

bool 	get_rom_version(char szRomVer[LEN_ROM_VER])
{
	bool	bRet = false;
	int		i;
	
	if (s_udev == NULL)
	{
		DBG_Print("get_rom_version : s_udev = NULL - return!\n");
		return false;
	}	
	
	for (i=0; i<ROM_DEF_COUNT; i++)
	{
		rom_read(ROM_Def[i].nVerAddr, (unsigned char *)szRomVer, LEN_ROM_VER);
		if (_memicmp(szRomVer, ROM_Def[i].szRomVer, 4) == 0 && szRomVer[5] == ROM_Def[i].szRomVer[5])	// shawn 2010/11/08 modify
		{
			debug_rom_def(ROM_Def[i]);
			bRet = true;
			break;
		}
	}
	
	return bRet;
}

bool get_bypass_length(unsigned long &ulBypass)
{
	ENUM_ROM_ID	id;

	if (s_udev == NULL)
	{
		DBG_Print("get_bypass_length : s_udev = NULL - return!\n");
		return false;
	}	
	
	ulBypass = 0;
	if (!get_rom_id(id))
	{
		DBG_Print("get_bypass_length : get_rom_id() - Fail!");
		return false;
	}

	ulBypass = ROM_Def[id].nBypass;
	
	return true;
}

bool is_new_sensor_table(void)
{
	bool	bRet = false;
	ENUM_ROM_ID	id;
	
	if (s_udev == NULL)
	{
		DBG_Print("is_new_sensor_table : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (!get_rom_id(id))
	{
		DBG_Print("is_new_sensor_table : get_rom_id() - Fail!");
		return false;
	}
	
	switch (id)
	{
	case ST50225E_1001: 
	case ST50230A:	
	case ST50230C_0002:	
	case ST50230C_0003:	
	case ST50250A:	
	case ST50250B:	
	case STT5215:
	case ST50215A_0001:
	case ST50236A_0001:		
	case ST50236B_0002:
	case ST50231A_0001:
	case ST50231D_1001:	// shawn 2009/08/14 add 233
	case ST50256B_0001:	// shawn 2009/08/14 add 256
	case SD50232A_2005:	// shawn 2009/11/02 add 232
	case ST50275A_3005:	// shawn 2009/11/02 add 275
	case ST50276A_4006:	// shawn 2010/07/13 add 276
	case ST50290A_6007:	// shawn 2010/12/13 add 290
	case ST5L232A_5007:	// shawn 2011/07/25 add 262
	case ST5L286A_4119:	// shawn 2011/07/25 add 286
	case ST5L288A_4221:	// shawn 2011/07/25 add 288
	case ST5L292: // carol 2013/08/29 add
		bRet = true;
		break;
	default:	
		bRet = false;
		break;
	}
	
	return bRet;
}

bool 	get_flash_type(BYTE &type)
{
	BYTE	byte = SF_TYPE_UNKNOWN;
	ENUM_ROM_ID	id;
	
	if (s_udev == NULL)
	{
		DBG_Print("get_flash_type : s_udev = NULL - return!\n");
		return false;
	}	
	
	if (!get_rom_id(id))
	{
		DBG_Print("get_flash_type : get_rom_id() - Fail!");
		return false;
	}

	if (!asic_read(ROM_Def[id].nSFTypeAddr, &byte))
	{
		DBG_Print("get_flash_type : asic_read() Fail!\n");
		return false;
	}
	type = byte;
	
	return true;
}

bool	probe_mem(BYTE *pMemType)
{
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_probe_mem(pMemType);
	return vnd_probe_mem(pMemType);
}

bool	i2c_read(BYTE slaveID, BYTE len, BYTE addr, BYTE data[5])
{
	// assert(s_mode == MODE_EXTENSION_UNIT);	
	return ext_i2c_read(slaveID, len, addr, data);
}

bool 	i2c_write(BYTE slaveID, BYTE len, BYTE addr, BYTE data[4])
{
	// assert(s_mode == MODE_EXTENSION_UNIT);	
	return ext_i2c_write(slaveID, len, addr, data);
}

bool	meminfo_read(BYTE *pValue)
{
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_meminfo_read(pValue);
	return vnd_meminfo_read(pValue);
}

bool	meminfo_write(BYTE value)
{
	if (s_mode == MODE_EXTENSION_UNIT)
		return ext_meminfo_write(value);
	return vnd_meminfo_write(value);
}

