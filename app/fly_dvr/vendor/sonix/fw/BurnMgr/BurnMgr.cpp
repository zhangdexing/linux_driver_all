#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BurnMgr.h"
#include "FW_File.h"
#include "..//common//debug.h"
#include "..//common//misc.h"

#define DEF_LEN_ROM		(0x7800)	// 30*1024
#define DEF_LEN_ROM36 (0x9000)	// 36*1024				// shawn 2009/11/02 for 233
#define DEF_LEN_ROM24 (0x6000)	// 24*1024				// shawn 2009/11/02 for 232 and 275
#define DEF_LEN_ROM18 (0x4800)	// 18*1024				// shawn 2009/11/02 for 232 and 275
#define DEF_LEN_DES		(0x0800)	//  2*1024
#define DEF_LEN_DES4	(0x1000)	//  4*1024				// shawn 2009/11/02 for 232 and 275
#define DEF_LEN_PARAM	(0x0800)	//  2*1024
#define DEF_LEN_PARAM4	(0x0F00)	//  4*1024 - 256	// shawn 2009/11/02 for 232 and 275
#define DEF_LEN_ISP		(0x2000)	//  8*1024
#define DEF_LEN_ISP24	(0x6000)	//  24*1024				// shawn 2009/11/02 for 232 and 275
#define DEF_LEN_ISP30	(0x7800)	//  30*1024				// shawn 2009/11/02 for 232 and 275
#define DEF_LEN_FW_IMG	(64*1024)
#define NEW_LEN_FW_IMG  (128*1024) // carol 2013/08/29 add

#define DEF_LEN_DEFECT_POS		(0x1000)
#define DEF_LEN_CUSTOM_SPACE	(0x3800)

#define PARAM_HEADER_LEN 21	// shawn 2009/11/04 for 233

// shawn 2009/11/03 for 232 and 275 +++++
BYTE m_abyNumOfFmt[SPEEDNUM];
BYTE byActualXUNum;
GUID aGUIDXU[XUNUM];
unsigned short awXUCTRL[XUNUM];
BYTE m_abyNumOfResFmtStill[SPEEDNUM][FMTNUM];
BYTE m_abyNumOfResFmtVideo[SPEEDNUM][FMTNUM];
BYTE m_abyFmtIdx[SPEEDNUM][FMTNUM];
BYTE m_abyNumOfFRFmtVideo[SPEEDNUM][FMTNUM][RESNUM];
BYTE m_abyStillCfgIdx[SPEEDNUM][FMTNUM][RESNUM];
ConfigData aCfgData[CFGNUM];
FRData m_aFRData_FmtVideo[SPEEDNUM][FMTNUM][RESNUM][FRNUM];
// shawn 2009/11/03 for 232 and 275 -----


CBurnMgr::CBurnMgr()
{
	m_nXferMode = MODE_VENDOR_COMMAND;
	m_bTarget_Flash = false;
	m_bTarget_File = false;
	m_bSave_All = false;
	m_bSave_ROM = false;
	m_bSave_Des = false;
	m_bSave_Param = false;
	m_bSave_ISP = false;
	m_bReserveSerial = false;
	m_bReserveManufacturer = false;
	m_bDefectPos = false;
	m_bCustomSpace = false;
	m_bFlag_Erase = false;
	m_bFlag_Check = false;
	m_bFlag_Prog = false;
	m_bFlag_Verify = false;
	m_pCam = NULL;
	memset(m_szRomVer, 0x00, LEN_ROM_VER*sizeof(char));
	memset(m_szFlashCodeVer, 0x00, LEN_CODE_VER*sizeof(char));
	memset(m_szTargetFile, 0x00, 256);
	m_nRomLen = 0;
	m_pRom = NULL;
	memset(m_szRomFile, 0x00, 256);
	m_nDesLen = 0;
	m_pDes = NULL;
	memset(m_szDesFile, 0x00, 256);
	m_nParamLen = 0;
	m_pParam = NULL;
	memset(m_szParamFile, 0x00, 256);
	m_DrvParamAddr = 0x0000;
	m_nISPLen = 0;
	m_pISP = NULL;
	memset(m_szISPFile, 0x00, 256);
	m_nFWDevLen = 0;
	m_pFWDev = NULL;
	m_pDefectPos = NULL;
	m_nDefectPosLen = 0;
	m_pCustomSpace = NULL;
	m_nCustomSpaceLen = 0;
	m_WPAddr = 0;
	
	// shawn 2009/11/02 for 232 and 275 +++++
	m_nCRC16Len = 0;
 	m_pCRC16 = NULL;
 	byActualXUNum = 1;
 	
 	int i = 0;
 	int j = 0;
 	int k = 0;
 	int l = 0;
 	
 	for (i = 0; i < SPEEDNUM; i++)
	{
		m_abyNumOfFmt[i] = 0;
		
		for (j = 0; j < FMTNUM; j++)
		{
			m_abyFmtIdx[i][j] = 0;
			m_abyNumOfResFmtVideo[i][j] = 0;
			m_abyNumOfResFmtStill[i][j] = 0;
			
			for (k = 0; k < RESNUM; k++)
			{
				m_abyNumOfFRFmtVideo[i][j][k] = 0;
				m_abyStillCfgIdx[i][j][k] = 0;
				
				for (l = 0; l < FRNUM; l++)
				{
					m_aFRData_FmtVideo[i][j][k][l].wFRInt = 0;
					m_aFRData_FmtVideo[i][j][k][l].byCfgIdx = 0;
				}
			}
		}
	}
	
	for (i = 0; i < CFGNUM; i++)
		memset(aCfgData+i, 0, sizeof(ConfigData));
		
	aGUIDXU[0] = XU1GUID;
	awXUCTRL[0] = 0x0F;

	for (i = 1; i < XUNUM; i++)
	{
		memset(aGUIDXU+i, 0, sizeof(GUID));
		awXUCTRL[i] = 0;
	}
 	// shawn 2009/11/02 for 232 and 275 -----
 	
 	m_nInitSensorParamAddr = 0;	// shawn 2009/11/04 for 233
 	
 	m_bSetTo64k = false;				// shawn 2009/11/06 add
}

CBurnMgr::~CBurnMgr()
{
	
}

void CBurnMgr::burn_init_trace(void)
{
	char	str[256];
	
	TRACE("====== Burning Profile ======\n");
	TRACE("pCam              : 0x%.4x\n", m_pCam);
	TRACE("szRomVer          : %s 0x%.2x 0x%.2x 0x%.2x\n", m_szRomVer, m_szRomVer[5], m_szRomVer[6], m_szRomVer[7]);
  	sprintf(str, "nRomID      : %d", m_nRomID);
	switch (m_nRomID)
	{
	case ST50220B_001: 	strcat(str, " (ST50220B_001)\n");	break;
	case ST50225A_001: 	strcat(str, " (ST50225A_001)\n");	break;
	case ST50225E_002: 	strcat(str, " (ST50225E_002)\n");	break;
	case ST50225E_1001: 	strcat(str, " (ST50225E_1001)\n");	break;
	case ST50230A: 			strcat(str, " (ST50230A)\n");		break;
	case ST50230C_0002: 	strcat(str, " (ST50230C_0002)\n");	break;
	case ST50230C_0003: 	strcat(str, " (ST50230C_0003)\n");	break;
	case ST50250A: 			strcat(str, " (ST50250A)\n");		break;
	case ST50250B: 			strcat(str, " (ST50250B)\n");		break;
	case STT5215: 			strcat(str, " (STT5215)\n");		break;
	case ST50215A_0001: 	strcat(str, " (ST50215A_0001)\n");	break;
	case ST50236A_0001: 	strcat(str, " (ST50236A_0001)\n");	break;
	case ST50236B_0002: 	strcat(str, " (ST50236B_0002)\n");	break;
	case ST50231A_0001: 	strcat(str, " (ST50231A_0001)\n");	break;
	case ST50231D_1001: 	strcat(str, " (ST50231D_1001)\n");	break;	// shawn 2009/08/14 add 233
	case ST50256B_0001: 	strcat(str, " (ST50256B_0001)\n");	break;	// shawn 2009/08/14 modify
	case SD50232A_2005: 	strcat(str, " (SD50232A_2005)\n");	break;	// shawn 2009/11/02 add 232
	case ST50275A_3005: 	strcat(str, " (ST50275A_3005)\n");	break;	// shawn 2009/11/02 add 275
	case ST50276A_4006: 	strcat(str, " (ST50276A_4006)\n");	break;	// shawn 2010/07/13 add 276
	case ST50290A_6007: 	strcat(str, " (ST50290A_6007)\n");	break;	// shawn 2010/12/13 add 290
	case ST5L232A_5007: 	strcat(str, " (ST5L232A_5007)\n");	break;	// shawn 2011/07/25 add 262
	case ST5L286A_4119: 	strcat(str, " (ST5L286A_4119)\n");	break;	// shawn 2011/07/25 add 286
	case ST5L288A_4221: 	strcat(str, " (ST5L288A_4221)\n");	break;	// shawn 2011/07/25 add 288
	case ROM_ID_UNKNOWN:	strcat(str, " (ROM_ID_UNKNOWN)\n");	break;
	}
	TRACE(str);
	
	sprintf(str, "Serial Flash Type : (0x%.2x)", m_SFType);
	switch (m_SFType)
	{
	case SF_TYPE_UNKNOWN:		strcat(str, " (SF_TYPE_UNKNOWN)\n");	break;
	case SF_TYPE_MXIC:			strcat(str, " (SF_TYPE_MXIC)\n");	break;
	case SF_TYPE_ST:			strcat(str, " (SF_TYPE_ST)\n");		break;
	case SF_TYPE_SST:			strcat(str, " (SF_TYPE_SST)\n");	break;
	case SF_TYPE_ATMEL_AT25F:	strcat(str, " (SF_TYPE_ATMEL_AT25F)\n");	break;
	case SF_TYPE_ATMEL_AT25FS:	strcat(str, " (SF_TYPE_ATMEL_AT25FS)\n");	break;
	case SF_TYPE_ATMEL_AT45DB:	strcat(str, " (SF_TYPE_ATMEL_AT45DB)\n");	break;	
	case SF_TYPE_WINBOND:		strcat(str, " (SF_TYPE_WINBOND)\n");	break;
	case SF_TYPE_PMC:			strcat(str, " (SF_TYPE_PMC)\n");		break;
	case SF_TYPE_AMIC:			strcat(str, " (SF_TYPE_AMIC)\n");		break;	// shawn 2009/11/02 add
	case SF_TYPE_EON:			strcat(str, " (SF_TYPE_EON)\n");		break;		// shawn 2009/11/02 add
	case SF_TYPE_MXIC_LIKE:		strcat(str, " (SF_TYPE_MXIC_LIKE)\n");	break;
	default: strcat(str, " (Invalid Value - Error!)\n");	break;
	}
	TRACE(str);
	
	TRACE("szFlashCodeVer    : %s\n", m_szFlashCodeVer);
	TRACE("nBypassLength     : %u (0x%.4x)\n", (unsigned int)m_nBypassLength, (unsigned int)m_nBypassLength);
	TRACE("bIsNewSensorTable : %d\n", (int)m_bIsNewSensorTable);
	TRACE("---------- burning option flag ----------\n");
	TRACE("bReserveSerial    : %d\n", m_bReserveSerial);
	TRACE("bReserveManufacturer : %d\n", m_bReserveManufacturer);
	TRACE("bDefectPos        : %d\n", m_bDefectPos);	
	TRACE("bCustomSpace      : %d\n", m_bCustomSpace);
	TRACE("bFlag_Erase       : %d\n", m_bFlag_Erase);
	TRACE("bFlag_Check       : %d\n", m_bFlag_Check);
	TRACE("bFlag_Prog        : %d\n", m_bFlag_Prog);
	TRACE("bFlag_Verify      : %d\n", m_bFlag_Verify);
	TRACE("--------------- Target -----------------\n");
	TRACE("bTarget_Flash     : %d\n", m_bTarget_Flash);
	TRACE("bTarget_File      : %d\n", m_bTarget_File);
	TRACE("szTargetFile      : %s\n", m_szTargetFile);
	TRACE("bSave_All         : %d\n", m_bSave_All);
	TRACE("bSave_ROM         : %d\n", m_bSave_ROM);
	TRACE("bSave_Des         : %d\n", m_bSave_Des);
	TRACE("bSave_Param       : %d\n", m_bSave_Param);
	TRACE("bSave_ISP         : %d\n", m_bSave_ISP);
	TRACE("----------------- ROM ------------------\n");
	TRACE("bSource_ROM       : %d (0:File, 1:Flash)\n", m_bSource_ROM);
	TRACE("nRomLen           : %d (0x%.4x)\n", m_nRomLen, m_nRomLen);
	TRACE("szRomFile         : %s\n", m_szRomFile);
	TRACE("----------------- Des ------------------\n");
	TRACE("bSource_Des       : %d (0:File, 1:Flash)\n", m_bSource_Des);
	TRACE("nDesLen           : %d (0x%.4x)\n", m_nDesLen, m_nDesLen);
	TRACE("szDesFile         : %s\n", m_szDesFile);
	TRACE("---------------- Param -----------------\n");
	TRACE("bSource_Param     : %d (0:File, 1:Flash)\n", m_bSource_Param);
	TRACE("nParamLen         : %d (0x%.4x)\n", m_nParamLen, m_nParamLen);
	TRACE("szParamFile       : %s\n", m_szParamFile);
	TRACE("DrvParamAddr      : 0x%.4x\n", m_DrvParamAddr);	
	TRACE("----------------- ISP ------------------\n");
	TRACE("bSource_ISP       : %d (0:File, 1:Flash)\n", m_bSource_ISP);
	TRACE("nISPLen           : %d (0x%.4x)\n", m_nISPLen, m_nISPLen);
	TRACE("szISPFile         : %s\n", m_szISPFile);
	TRACE("nDefectPosLen     : %d (0x%.4x)\n", m_nDefectPosLen, m_nDefectPosLen);
	TRACE("m_nCustomSpaceLen : %d (0x%.4x)\n", m_nCustomSpaceLen, m_nCustomSpaceLen);
	TRACE("\n");
}

bool CBurnMgr::read_flash_rom(BYTE *pBuf, int nLen)
{
	unsigned short	address;
	
	if (m_bSetTo64k)	// shawn 2009/11/06 add
		address = 0x0000;
	else
	{
		if (m_nRomID == ST50231D_1001)	// shawn 2009/11/04 for 233
			address = 0x6000;
		else
			address = 0x0000;
	}
	
	if (!sf_read(address, pBuf, nLen))
	{
		DBG_Print("read_flash_rom : sf_read() - Fail!\n");
		return false;
	}
	return true;
}

bool CBurnMgr::read_flash_des(BYTE *pBuf, int nLen)
{
	unsigned short	address;
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		address = 0xD000;
	else
		address = 0x7800;
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	if (!sf_read(address, pBuf, nLen))
	{
		DBG_Print("CBurnMgr::read_flash_des : sf_read - Fail!\n");
		return false;
	}
	return true;	
}

bool CBurnMgr::read_flash_param(BYTE *pBuf, unsigned int DrvParamAddr)
{
	unsigned short	address;
	
	// get first parameter
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		address = 0xC000;
	else if (m_nRomID == ST50231D_1001)	// shawn 2009/11/04 for 233
		address = 0x0000;
	else
		address = 0x8000;
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	if (m_nRomID == ST50231D_1001)	// shawn 2009/11/04 for 233
	{
		if (!sf_read(address, pBuf, m_nInitSensorParamAddr))
		{
			DBG_Print("CBurnMgr::read_flash_param : get parameter - Fail!\n");
			return false;
		}
	}
	else
	{
		if (!sf_read(address, pBuf, 0x400))
		{
			DBG_Print("CBurnMgr::read_flash_param : get first parameter - Fail!\n");
			return false;
		}
		// get second parameter
		if (!sf_read(DrvParamAddr, pBuf+0x400, 0x400))
		{
			DBG_Print("CBurnMgr::read_flash_param : get second parameter - Fail!\n");
			return false;
		}
	}
	return true;
}

bool CBurnMgr::read_flash_ISP(BYTE *pBuf, int nLen)
{
	unsigned short	address;

	// get init sensor parameter buffer
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == SD50232A_2005)
		address = 0x4800;
	else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		address = 0x6000;
	else if (m_nRomID == ST50231D_1001)	// shawn 2009/11/04 for 233
		address = m_nInitSensorParamAddr;
	else
		address = 0x8800;
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	if (!sf_read(address, pBuf, nLen))
	{
		DBG_Print("CBurnMgr::read_flash_ISP : get init sensor parameter buffer - Fail!\n");
		return false;
	}
	return true;
}

bool CBurnMgr::raise_cpu_rate(void)
{
	TRACE("CBurnMgr::raise_cpu_rate()\n");
	
	if (m_nRomID == ST50230C_0002 || 
		m_nRomID == ST50230C_0003 ||
		m_nRomID == ST50231A_0001 || 
		m_nRomID == ST50236B_0002 ||
		m_nRomID == STT5215       || 
		m_nRomID == ST50215A_0001 ||
		m_nRomID == ST50256B_0001 ||	// shawn 2009/08/14 add 256
		m_nRomID == ST50275A_3005 ||	// shawn 2009/11/02 add 275
		m_nRomID == ST50276A_4006 ||	// shawn 2010/07/13 add 276
		m_nRomID == ST50290A_6007 ||	// shawn 2010/12/13 add 290
		m_nRomID == ST5L232A_5007 ||	// shawn 2011/07/25 add 262
		m_nRomID == ST5L286A_4119 ||	// shawn 2011/07/25 add 286
		m_nRomID == ST5L288A_4221 ||	// shawn 2011/07/25 add 288
		m_nRomID == ST5L292 ) // carol 2013/08/29 add
	{
		if (m_SFType == SF_TYPE_SST)
		{
			TRACE("CBurnMgr::raise_cpu_rate : CPU Rate = 12MHz\n");
			pull_cpu_rate(false);
		}
		else
		{
			TRACE("CBurnMgr::raise_cpu_rate : CPU Rate = 24MHz\n");
			pull_cpu_rate(true);
		}
	}
	// shawn 2009/08/14 for 233 +++++
	else if (m_nRomID == ST50231D_1001 || m_nRomID == SD50232A_2005)	// shawn 2009/11/02 add 232
	{
		TRACE("CBurnMgr::raise_cpu_rate : CPU Rate = 12MHz\n");
		pull_cpu_rate(false);
	}
	else
	{
		TRACE("CBurnMgr::raise_cpu_rate : CPU Rate = 24MHz\n");
		pull_cpu_rate(true);
	}
	// shawn 2009/08/14 for 233 -----
	
	return true;
}

bool CBurnMgr::down_cpu_rate(void)
{
	TRACE("CBurnMgr::down_cpu_rate()\n");
	
	if (!pull_cpu_rate(false))
	{
		DBG_Print("CBurnMgr::down_cpu_rate : pull_cpu_rate() - Fail!\n");
		return false;
	}
	return true;
}

bool CBurnMgr::param_str_err_handling(BYTE *pParam, const char szRomVer[LEN_ROM_VER])
{
	TRACE("CBurnMgr::param_str_err_handling()\n");
	
	unsigned int	StrOffset;
	unsigned int 	StrOffsetAddr;
	
	for (StrOffset=0x40; StrOffset <= 0x140; StrOffset += 0x40)
	{
		// james 2007/11/07 to avoid string length==0x40 (mask rom issue)
		if (pParam[StrOffset] <= 2 || 
			pParam[StrOffset] > 0x3E /*0x40 */ ||
			pParam[StrOffset+1] != 0x03)
		{
			// error string procedure
			// james 2007/10/30 220RO have only 4 strings
			if (_memicmp(szRomVer, "22", 2)==0)
			{
				if (StrOffset == 0x140)
					break;
					
				// 220RO should set string offset to 0
				StrOffsetAddr = 0x22 + (StrOffset/0x40 - 1)*2;
				pParam[StrOffsetAddr] 	= 0;
				pParam[StrOffsetAddr+1] = 0;
				TRACE("pParam[%d] = %d\n", StrOffsetAddr  , pParam[StrOffsetAddr]);
				TRACE("pParam[%d] = %d\n", StrOffsetAddr+1, pParam[StrOffsetAddr+1]);
			}

			memset(pParam+StrOffset, 0xff, 0x40);
			// james 2007/10/29 modify string error handling
			pParam[StrOffset] 	= 0x02;
			pParam[StrOffset+1]	= 0x03;
		}
		else
		{
			// correct string procedure
			TRACE("StrOffset = %x", StrOffset);
			if (_memicmp(szRomVer, "220RO", 4)==0)
			{
				StrOffsetAddr = 0x22 + (StrOffset/0x40 - 1)*2;
				pParam[StrOffsetAddr] 	= (unsigned char)(StrOffset >> 8);
				pParam[StrOffsetAddr+1] = (unsigned char)(StrOffset & 0xff);
				TRACE("pParam[%d] = %d\n", StrOffsetAddr  , pParam[StrOffsetAddr]);
				TRACE("pParam[%d] = %d\n", StrOffsetAddr+1, pParam[StrOffsetAddr+1]);
			}
		}
	}
	return true;
}

bool CBurnMgr::Flash_Erase_MXIC(void)
{
	TRACE("CBurnMgr::Flash_Erase_MXIC()\n");
	
	BYTE	bWriteProtect = 0;
	unsigned short 	WPAddr = 0xFFFF;
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == ST50231D_1001)
	{
		sf_read(0x000f, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0x000f = 0x%x\n", bWriteProtect);
	}
	else if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		sf_read(0xC034, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0xC034 = 0x%x\n", bWriteProtect);
	}
	else
	{
		sf_read(0x8034, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0x8034 = 0x%x\n", bWriteProtect);
	}
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	if (!set_write_protect(false, WPAddr, bWriteProtect))
	{
		DBG_Print("CBurnMgr::Flash_Erase_MXIC : set_write_protect() to disabled - Fail!\n");
	}
	
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
	asic_write(0x1091,0x1);
	//SF_CMDread_Status
	sf_cmdread_status();
	asic_write(0x1080,0x0);
	
	return true;	
}

bool CBurnMgr::Flash_Erase_SST(void)
{
	TRACE("CBurnMgr::Flash_Erase_SST()\n");
	
	BYTE	bWriteProtect = 0;
	unsigned short 	WPAddr = 0xFFFF;
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == ST50231D_1001)
	{
		sf_read(0x000f, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0x000f = 0x%x\n", bWriteProtect);
	}
	else if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		sf_read(0xC034, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0xC034 = 0x%x\n", bWriteProtect);
	}
	else
	{
		sf_read(0x8034, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0x8034 = 0x%x\n", bWriteProtect);
	}
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	if (!set_write_protect(false, WPAddr, bWriteProtect))
	{
		DBG_Print("CBurnMgr::Flash_Erase_SST : set_write_protect() to disabled - Fail!\n");
	}
	
	asic_write(0x1080,0x1);		// serial flash mode
	//SF_Set_WEL_Bit
	asic_write(0x1091,0x0);		// chip select
	asic_write(0x1082,0x06);	// write data
	asic_write(0x1081,0x01);	// trigger for write
	sf_wait_ready();
	asic_write(0x1091,0x1);
	//chip erase
	asic_write(0x1091,0x0);
	asic_write(0x1082,0x60);
	asic_write(0x1081,0x01);
	sf_wait_ready();
	asic_write(0x1091,0x1);
	//SF_CMDread_Status
	sf_cmdread_status();
	asic_write(0x1080,0x0);
	
	return true;
}

bool CBurnMgr::Flash_Erase_ST(void)
{
	TRACE("CBurnMgr::Flash_Erase_ST()\n");
	
	BYTE	bWriteProtect = 0;
	unsigned short 	WPAddr = 0xFFFF;
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == ST50231D_1001)
	{
		sf_read(0x000f, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0x000f = 0x%x\n", bWriteProtect);
	}
	else if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		sf_read(0xC034, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0xC034 = 0x%x\n", bWriteProtect);
	}
	else
	{
		sf_read(0x8034, &bWriteProtect, 1);
		TRACE("CBurnMgr::Flash_Erase_MXIC : Get Write Protect from 0x8034 = 0x%x\n", bWriteProtect);
	}
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	if (!set_write_protect(false, WPAddr, bWriteProtect))
	{
		DBG_Print("CBurnMgr::Flash_Erase_ST : set_write_protect() to disabled - Fail!\n");
	}

	asic_write(0x1080,0x1);
	//SF_Set_WEL_Bit
	asic_write(0x1091,0x0);
	asic_write(0x1082,0x06);
	asic_write(0x1081,0x01);
	sf_wait_ready();
	asic_write(0x1091,0x1);
	//chip erase
	asic_write(0x1091,0x0);
	asic_write(0x1082,0xc7);
	asic_write(0x1081,0x01);
	sf_wait_ready();
	asic_write(0x1091,0x1);
	//SF_CMDread_Status
	sf_cmdread_status();
	asic_write(0x1080,0x0);
	
	return true;	
}

bool CBurnMgr::Flash_Read_FW_Img(BYTE buf[DEF_LEN_FW_IMG])
{
	TRACE("CBurnMgr::Flash_Read_FW_Img()\n");
	
	if (!sf_read(0x0000, buf, DEF_LEN_FW_IMG))
	{
		DBG_Print("CBurnMgr::Flash_Read_FW_Img : sf_read() - Fail!\n");
		return false;
	}
	return true;
}

bool CBurnMgr::Set_XferMode(int nXferMode)
{
	TRACE("CBurnMgr::Set_XferMode()\n");
	
	assert(m_nXferMode == MODE_VENDOR_COMMAND ||
		   m_nXferMode == MODE_EXTENSION_UNIT);
	
	m_nXferMode = nXferMode;
	return true;	
}

bool CBurnMgr::Cam_Select(struct usb_device* pCam, int nFileNum)
{
	m_bSetTo64k = false; // carol 2013/12/16 add for os automatically assign init value
	m_bSetTo128k = false; // carol 2013/12/16 add


	if (nFileNum == 1)
		m_bSetTo64k = true;
	else if (nFileNum == 2) // carol 2013/08/29 add
		m_bSetTo128k = true;
	else // carol 2013/08/29 add
		return false;
		//m_bSetTo64k = false;  // carol 2013/08/29 mark
	
	m_pCam = pCam;
	if ((m_udev = usb_open(pCam)) == NULL)
	{
		DBG_Print("CBurnMgr::Cam_Select : usb_open() - Fail!\n");
		return false;
	}
	else // carol 2013/10/09 add
	{
		usb_detach_kernel_driver_np(m_udev, 0);
	}
	if (!init_xfer(m_udev, m_nXferMode))
	{
		DBG_Print("CBurnMgr::Cam_Select : init_xfer() Fail!\n");
		return false;
	}
	if (!get_rom_version(m_szRomVer))
	{
		DBG_Print("CBurnMgr::Cam_Select : get_rom_version() - Fail!\n");
		return false;
	}	
	get_rom_id(m_nRomID);
	
	m_bIsNewSensorTable = is_new_sensor_table();
	DBG_Print("m_bIsNewSensorTable: %d\n", m_bIsNewSensorTable);

	if (!get_flash_type(m_SFType))
	{
		DBG_Print("CBurnMgr::Cam_Select : get_flash_type() - Fail!\n");
		return false;
	}

	if (!get_code_version(m_szFlashCodeVer))
	{
		DBG_Print("CBurnMgr::Cam_Select : get_code_version() - Fail!\n");
		return false;
	}
	
	if (!get_bypass_length(m_nBypassLength))
	{
		DBG_Print("CBurnMgr::Cam_Select : get_bypass_length() - Fail!\n");
		return false;
	}
	
	// set default burning option flag
	m_bSource_ROM		= SOURCE_FLASH;
	m_bSource_Des		= SOURCE_FLASH;
	m_bSource_Param	= SOURCE_FLASH;
	m_bSource_ISP		= SOURCE_FLASH;
	
	m_bTarget_Flash	= false;
	m_bTarget_File		= false;

	m_bSave_All			= false;
	m_bSave_ROM			= false;
	m_bSave_Des			= false;
	m_bSave_Param		= false;
	m_bSave_ISP			= false;
	
	m_bFlag_Erase 		= true;
	m_bFlag_Check 		= true;
	m_bFlag_Prog 		= true;
	m_bFlag_Verify 	= true;


	if (m_bSetTo64k)
		m_nRomLen 	= DEF_LEN_FW_IMG;
	else if (m_bSetTo128k) // carol 2013/08/29 add
		m_nRomLen 	= NEW_LEN_FW_IMG;
	else
	{
		if (m_nRomID == ST50231D_1001)
			m_nRomLen 	= DEF_LEN_ROM36;
		else if (m_nRomID == SD50232A_2005)
			m_nRomLen 	= DEF_LEN_ROM18;
		else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
			m_nRomLen 	= DEF_LEN_ROM24;
		else
			m_nRomLen 	= DEF_LEN_ROM;
	}
	
	m_pRom = (BYTE *)malloc(m_nRomLen);
	memset(m_pRom, 0xff, m_nRomLen);
	m_bSource_ROM = SOURCE_FLASH;
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		m_nDesLen 	= DEF_LEN_DES4;
	else
		m_nDesLen 	= DEF_LEN_DES;
	// shawn 2009/11/02 modify for 232 and 275 -----
	
	m_pDes = (BYTE *)malloc(m_nDesLen);
	memset(m_pDes, 0xff, m_nDesLen);
	m_bSource_Des = SOURCE_FLASH;
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		m_nParamLen = DEF_LEN_PARAM4;
	else
		m_nParamLen = DEF_LEN_PARAM;
	// shawn 2009/11/02 modify for 232 and 275 -----
	
 	m_pParam = (BYTE *)malloc(m_nParamLen);
 	memset(m_pParam, 0xff, m_nParamLen);
 	m_bSource_Param = SOURCE_FLASH;
 	
 	if (m_nRomID == ST50220B_001 || 
 		m_nRomID == ST50225A_001 ||
 		m_nRomID == ST50225E_002)
 		m_DrvParamAddr = 0xE000;
 	// shawn 2009/11/02 add 232 and 275 +++++
 	else if (m_nRomID == SD50232A_2005 || ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
 		m_DrvParamAddr = 0xC400;
 	// shawn 2009/11/02 add 232 and 275 -----
 	else
 		m_DrvParamAddr = 0x8400;
 	
 	// shawn 2009/11/02 modify for 232 and 275 +++++
 	if (m_nRomID == SD50232A_2005)
 		m_nISPLen = DEF_LEN_ISP30;
 	else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
 		m_nISPLen = DEF_LEN_ISP24;
 	else
 		m_nISPLen = DEF_LEN_ISP;
 	// shawn 2009/11/02 modify for 232 and 275 -----
 	
 	m_pISP = (BYTE *)malloc(m_nISPLen);
 	memset(m_pISP, 0xff, m_nISPLen);
 	m_bSource_ISP = SOURCE_FLASH;
 	
 	m_nFWDevLen = DEF_LEN_FW_IMG;
 	m_pFWDev = (BYTE *)malloc(m_nFWDevLen);
	
	m_bDefectPos = false;
	m_nDefectPosLen = DEF_LEN_DEFECT_POS;
	m_pDefectPos = (BYTE *)malloc(m_nDefectPosLen);
	
	m_bCustomSpace = false;
	m_nCustomSpaceLen = DEF_LEN_CUSTOM_SPACE;
	m_pCustomSpace = (BYTE *)malloc(m_nCustomSpaceLen);
	
	// shawn 2009/11/02 for 232 and 275 +++++
	m_nCRC16Len = 24;
 	m_pCRC16 = (BYTE *)malloc(m_nCRC16Len);
 	memset(m_pCRC16, 0xff, m_nCRC16Len);
 	// shawn 2009/11/02 for 232 and 275 -----
	
	return true;	
}

void CBurnMgr::Cam_DeSelect(void)
{
	TRACE("CBurnMgr::Cam_DeSelect()\n");
	
	if (m_udev != NULL)
		usb_close(m_udev);
	uninit_xfer();
	free(m_pRom);	m_pRom 	= NULL;
	free(m_pDes);	m_pDes 	= NULL;
	free(m_pParam);	m_pParam = NULL;
	free(m_pISP);	m_pISP 	= NULL;
	free(m_pFWDev);	m_pFWDev = NULL;
	free(m_pCRC16); m_pCRC16 = NULL;	// shawn 2009/11/02 for 232 and 275
	return;	
}

bool CBurnMgr::Get_CodeVersion(char szFlashCodeVer[32])
{
	TRACE("CBurnMgr::Get_CodeVersion()\n");
	
	return get_code_version(szFlashCodeVer);
}

bool CBurnMgr::Get_VendorVersion(char szFlashVendorVer[16])
{
	TRACE("CBurnMgr::Get_VendorVersion()\n");
	
	return get_vendor_version(szFlashVendorVer);
}

bool CBurnMgr::Get_RomVersion(char szRomVer[8])
{
	TRACE("CBurnMgr::Get_RomVersion()\n");
	
	return get_rom_version(szRomVer);	
}

bool CBurnMgr::Get_FWVersion(char szFwVer[10])
{
	TRACE("CBurnMgr::Get_FWVersion()\n");
	
	return get_fw_version(szFwVer);	
}

char* CBurnMgr::Get_RomFile()
{
	TRACE("CBurnMgr::Get_RomFile()\n");
	
	return (char *)m_szRomFile;
}

char* CBurnMgr::Get_ParamFile()
{
	TRACE("CBurnMgr::Get_ParamFile()\n");
	
	return (char *)m_szParamFile;
}

char* CBurnMgr::Get_ISPFile()
{
	TRACE("CBurnMgr::Get_ISPFile()\n");
	
	return (char *)m_szISPFile;
}
	
// set source from .ini file.
bool CBurnMgr::Set_Source_File_From_INI(char file[256])
{
	TRACE("CBurnMgr::Set_Source_File_From_INI() - file = %s\n", file);
	
	FILE	*fp;
	char	szRomFile[256];
	char	szParamFile[256];
	char	szISPFile[256];	
	
	if ((fp = fopen(file, "r")) == NULL)
	{
		DBG_Print("CBurnMgr::CBurnMgr::Set_Source_From_INI : open .ini file Fail! - %s\n", file);
		return false;
	}
	//fscanf(fp, "%s", szRomFile);
	memcpy(szRomFile, file, 256);
	//if (!m_bSetTo64k) // carol
	if (!m_bSetTo64k && !m_bSetTo128k) // carol
	{
		//fscanf(fp, "%s", szParamFile);
		//fscanf(fp, "%s", szISPFile);
		memcpy(szParamFile, file, 256);
		memcpy(szISPFile, file, 256);
	}

	fclose(fp);
	
	Set_Source_ROM(SOURCE_FILE, szRomFile);
	
	
	//if (!m_bSetTo64k) // carol
	if (!m_bSetTo64k && !m_bSetTo128k) // carol
	{
		Set_Source_Param(SOURCE_FILE, szParamFile);
		Set_Source_ISP(SOURCE_FILE, szISPFile);	
	}
	
	return true;
}

bool CBurnMgr::Set_Source_ROM(BYTE flag, char szFile[256])
{
	TRACE("CBurnMgr::Set_Source_ROM()\n");
	
	m_bSource_ROM = flag;
	if (flag == SOURCE_FILE)
		strcpy(m_szRomFile, szFile);
	else
		memset(m_szRomFile, 0x00, 256);
	return true;	
}

bool CBurnMgr::Set_Source_Des(BYTE flag, char szFile[256])
{
	TRACE("CBurnMgr::Set_Source_Des()\n");
	
	m_bSource_Des = flag;
	if (flag == SOURCE_FILE)
		strcpy(m_szDesFile, szFile);
	else
		memset(m_szDesFile, 0x00, 256);
	return true;
}

bool CBurnMgr::Set_Source_Param(BYTE flag, char szFile[256])
{
	TRACE("CBurnMgr::Set_Source_Param()\n");
	
	m_bSource_Param = flag;
	if (flag == SOURCE_FILE)
		strcpy(m_szParamFile, szFile);
	else
		memset(m_szParamFile, 0x00, 256);
	return true;
}

bool CBurnMgr::Set_Source_ISP(BYTE flag, char szFile[256])
{
	TRACE("CBurnMgr::Set_Source_ISP()\n");
	
	m_bSource_ISP = flag;
	if (flag == SOURCE_FILE)
		strcpy(m_szISPFile, szFile);
	else
		memset(m_szISPFile, 0x00, 256);
	return true;
}

bool CBurnMgr::Set_Target_Flash(bool bSet)
{
	TRACE("CBurnMgr::Set_Target_Flash()\n");
	
	m_bTarget_Flash = bSet;
	return true;		
}

bool CBurnMgr::Set_Target_File(bool bSet)
{
	TRACE("CBurnMgr::Set_Target_File()\n");
	
	m_bTarget_File = bSet;
	return true;
}

bool CBurnMgr::Set_Save_FileName(char szFileName[256])
{
	TRACE("CBurnMgr::Set_Save_FileName()\n");
	
	strcpy(m_szTargetFile, szFileName);
	return true;	
}

bool CBurnMgr::Set_Save_All(bool bSet)
{
	TRACE("CBurnMgr::Set_Save_All()\n");
	
	m_bSave_All = bSet;
	return true;
}

bool CBurnMgr::Set_Save_ROM(bool bSet)
{
	TRACE("CBurnMgr::Set_Save_ROM()\n");
	
	m_bSave_ROM = bSet;
	return true;	
}

bool CBurnMgr::Set_Save_Des(bool bSet)
{
	TRACE("CBurnMgr::Set_Save_Des()\n");
	
	m_bSave_Des = bSet;
	return true;
}

bool CBurnMgr::Set_Save_Param(bool bSet)
{
	TRACE("CBurnMgr::Set_Save_Param()\n");
	
	m_bSave_Param = bSet;	
	return true;
}

bool CBurnMgr::Set_Save_ISP(bool bSet)
{
	TRACE("CBurnMgr::Set_Save_ISP()\n");
	
	m_bSave_ISP = bSet;
	return true;
}

bool CBurnMgr::Set_Reserve_Serial(bool bSet)
{
	TRACE("CBurnMgr::Set_Reserve_Serial()\n");
	
	m_bReserveSerial = bSet;
	return true;
}

bool CBurnMgr::Set_Reserve_Manufacturer(bool bSet)
{
	TRACE("CBurnMgr::Set_Reserve_Manufacturer()\n");
	
	m_bReserveManufacturer = bSet;
	return true;
}

bool CBurnMgr::Set_Defect_Pos(bool bSet)
{
	TRACE("CBurnMgr::Set_Defect_Pos()\n");
	
	m_bDefectPos = bSet;	
	return true;
}

bool CBurnMgr::Set_Custom_Space(bool bSet)
{
	TRACE("CBurnMgr::Set_Custom_Space()\n");
	
	m_bCustomSpace = bSet;
	return true;
}

bool CBurnMgr::Load_Source_Data(void)
{
	TRACE("CBurnMgr::Load_Source_Data()\n");
	
	//if (m_bSetTo64k) // carol 2013/08/29 mark
	if (m_bSetTo64k || m_bSetTo128k) // carol 2013/08/29 add
	{
		if (m_bSource_ROM == SOURCE_FILE)
		{
			if (!read_file_64k(m_szRomFile, m_pRom, m_nRomLen))
			{
				DBG_Print("CBurnMgr:: : read_file_64k() - Fail!\n");
				return false;
			}
		}
		TRACE("CBurnMgr::Load_Source_Data : get 64k file - OK!\n");
	}
	else
	{
		int		nOutLen;
	
		if (m_bReserveSerial)
		{
			if (!sf_read(0x80c0, m_pParam + 0xc0, 0x40))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : Reserve Serial Number - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Load_Source_Data : Reserve Serial Number - OK!\n");
		}
		
		if (m_bReserveManufacturer)
		{
			if (!sf_read(0x8080, m_pParam + 0x80, 0x40))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : Reserve Manufacturer - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Load_Source_Data : Reserve Manufacturer - OK!\n");
		}
		
		if (m_bDefectPos)
		{
			if (!sf_read(0xA800, m_pDefectPos, m_nDefectPosLen))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : Load Defect Pos - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Load_Source_Data : Load Defect Pos - OK!\n");
		}
		
		if (m_bCustomSpace)
		{
			if (!sf_read(0xB800, m_pCustomSpace, m_nCustomSpaceLen))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : Load Custom Space - Fail!\n");
				return false;
			}
			DBG_Print("CBurnMgr::Load_Source_Data : Load Custom Space - OK!\n");
		}
		
		// get ROM
		if (m_bSource_ROM == SOURCE_FILE)
		{
			if (!read_file_rom(m_szRomFile, m_nBypassLength, 
								m_pRom, m_nRomLen, &nOutLen))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : read_file_rom() - Fail!\n");
				return false;
			}
			// shawn 2010/07/13 for 276 +++++
			else
			{
				if (m_nRomID == ST50276A_4006)
				{
					unsigned char byTmp1 = 0;
					unsigned char byTmp2 = 0;
					unsigned short wAddr = 0x0158;
					
					asic_read(0x1185, &byTmp1);
					byTmp1 |= 0x70;
					asic_write(0x1185, byTmp1);
					asic_read(0x1185, &byTmp2);
	
					if ( (byTmp2&0x70) == (byTmp1&0x70) )	// 237
					{
						// Key for 237:SEPT5ADC
						if ((m_pRom[wAddr]     != 0x53) || 
							 	(m_pRom[wAddr + 1] != 0x45) || 
							 	(m_pRom[wAddr + 2] != 0x50) || 
							 	(m_pRom[wAddr + 3] != 0x54) || 
							 	(m_pRom[wAddr + 4] != 0x35) || 
							 	(m_pRom[wAddr + 5] != 0x41) || 
							 	(m_pRom[wAddr + 6] != 0x44) || 
							 	(m_pRom[wAddr + 7] != 0x43) )
						{
							DBG_Print("CBurnMgr::Load_Source_Data : rom file error!\n");
							memset(m_pRom, 0xff, m_nRomLen);
							return false;
						}
					}
					else	// 234
					{
						// Key for 234: QUARTRE1
						if ((m_pRom[wAddr]     != 0x51) || 
							 	(m_pRom[wAddr + 1] != 0x55) || 
							 	(m_pRom[wAddr + 2] != 0x41) || 
							 	(m_pRom[wAddr + 3] != 0x52) || 
							 	(m_pRom[wAddr + 4] != 0x54) || 
							 	(m_pRom[wAddr + 5] != 0x52) || 
							 	(m_pRom[wAddr + 6] != 0x45) || 
							 	(m_pRom[wAddr + 7] != 0x31) )
						{
							DBG_Print("CBurnMgr::Load_Source_Data : rom file error!\n");
							memset(m_pRom, 0xff, m_nRomLen);
							return false;
						}
					}
				}
			}
			// shawn 2010/07/13 for 276 -----
		}
		else if (m_bSource_ROM == SOURCE_FLASH)
		{
			if (!read_flash_rom(m_pRom, m_nRomLen))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : read_flash_rom() - Fail!\n");
				return false;
			}
		}
		TRACE("CBurnMgr::Load_Source_Data : get ROM - OK!\n");
		// get Descriptor
		if (m_bSource_Des == SOURCE_FILE)
		{
			if (!read_file_des(m_szDesFile, m_pDes, m_nDesLen, &nOutLen))
			{
				DBG_Print("CBurnMgr:: : read_file_des() - Fail!\n");
				return false;
			}
		}
		else if (m_bSource_Des == SOURCE_FLASH)
		{
			if (!read_flash_des(m_pDes, m_nDesLen))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : read_flash_des() - Fail!\n");
				return false;
			}
		}
		TRACE("CBurnMgr::Load_Source_Data : get Descriptor - OK!\n");
		// get Parameter
		if (m_bSource_Param == SOURCE_FILE)
		{
			if (!read_file_param(m_szParamFile, m_pParam, m_nParamLen, &nOutLen))
			{
				DBG_Print("CBurnMgr:: : read_file_param() - Fail!\n");
				return false;
			}
		}
		else if (m_bSource_Param == SOURCE_FLASH)
		{
			if (!read_flash_param(m_pParam, m_DrvParamAddr))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : read_flash_param() - Fail!\n");
				return false;
			}
		}
		TRACE("CBurnMgr::Load_Source_Data : get Parameter - OK!\n");
		// get Init Sensor Parameter
		if (m_bSource_ISP == SOURCE_FILE)
		{
			if (!read_file_ISP(m_szISPFile, m_pISP, m_nISPLen, &nOutLen, m_nRomID))	// shawn 2009/11/03 modify for 232 and 275
			{
				DBG_Print("CBurnMgr::Load_Source_Data : read_file_ISP() - Fail!\n");
				return false;
			}
		}
		else if (m_bSource_ISP == SOURCE_FLASH)
		{
			if (!read_flash_ISP(m_pISP, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Load_Source_Data : read_flash_ISP() - Fail!\n");
				return false;
			}
		}
		TRACE("CBurnMgr::Load_Source_Data : get Init Sensor Parameter - OK!\n");
	}
	// shawn 2009/08/14 for 233 -----
	
	return true;	
}

bool CBurnMgr::Burn_Save_All(void)
{
	TRACE("CBurnMgr::Burn_Save_All()\n");
	
	BYTE	buf[DEF_LEN_FW_IMG];
	char	szFile[256];
	FILE	*fp;
	
	if (!m_bTarget_File || !m_bSave_All)
	{
		DBG_Print("Burn_Save_All : !bTarget_File || !bSave_All - return!\n");
		return false;
	}

	memset(buf, 0xff, DEF_LEN_FW_IMG);
	
	// shawn 2009/11/02 modify for 232 and 275 +++++
	if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		// ROM program
		memcpy(buf + 0x0000, m_pRom, m_nRomLen);
		// InitSensorParam program
		if (m_nRomID == SD50232A_2005)
			memcpy(buf + 0x4800, m_pISP, m_nISPLen);
		else
			memcpy(buf + 0x6000, m_pISP, m_nISPLen);
		// Parameter program
		param_str_err_handling(m_pParam, m_szRomVer);
		// write first parameter
		memcpy(buf + 0xC000, m_pParam, 0x0400);
		// write second parameter
		memcpy(buf + m_DrvParamAddr, m_pParam+0x0400, 0x0400);
		// Descriptor program
		memcpy(buf + 0xD000, m_pDes, m_nDesLen);
	}
	else
	{
		// ROM program
		memcpy(buf + 0x0000, m_pRom, m_nRomLen);
		// Descriptor program
		memcpy(buf + 0x7800, m_pDes, m_nDesLen);
		// Parameter program
		param_str_err_handling(m_pParam, m_szRomVer);
		// write first parameter
		memcpy(buf + 0x8000, m_pParam, 0x0400);
		// write second parameter
		memcpy(buf + m_DrvParamAddr, m_pParam+0x0400, 0x0400);
		// InitSensorParam program
		memcpy(buf + 0x8800, m_pISP, m_nISPLen);
	}
	// shawn 2009/11/02 modify for 232 and 275 -----

	strcpy(szFile, m_szTargetFile);
	strcat(szFile, "_ALL.bin");
	fp = fopen(szFile, "wb");
	fwrite(buf, 1, DEF_LEN_FW_IMG, fp);
	fclose(fp);	
	
	return true;
}

bool CBurnMgr::Burn_Save_ROM(void)
{
	TRACE("CBurnMgr::Burn_Save_ROM()\n");
	
	char	szFile[256];
	FILE	*fp;
	
	if (!m_bTarget_File || !m_bSave_ROM)
	{
		DBG_Print("CBurnMgr::Burn_Save_ROM : !m_bTarget_File || !m_bSave_ROM - return!\n");
		return false;
	}

	strcpy(szFile, m_szTargetFile);
	strcat(szFile, "_ROM.bin");
	fp = fopen(szFile, "wb");
	fwrite(m_pRom, 1, m_nRomLen, fp);
	fclose(fp);	
	return true;
}

bool CBurnMgr::Burn_Save_Des(void)
{
	TRACE("CBurnMgr::Burn_Save_Des()\n");
	
	char	szFile[256];
	FILE	*fp;
	
	if (!m_bTarget_File || !m_bSave_Des)
	{
		DBG_Print("CBurnMgr::Burn_Save_Des : !m_bTarget_File || !m_bSave_Des - return!\n");
		return false;
	}

	strcpy(szFile, m_szTargetFile);
	strcat(szFile, "_DES.bin");
	fp = fopen(szFile, "wb");
	fwrite(m_pDes, 1, m_nDesLen, fp);
	fclose(fp);
	return true;	
}

bool CBurnMgr::Burn_Save_Param(void)
{
	TRACE("CBurnMgr::Burn_Save_Param()\n");
	
	char	szFile[256];
	FILE	*fp;
	
	if (!m_bTarget_File || !m_bSave_Param)
	{
		DBG_Print("CBurnMgr::Burn_Save_Param : !m_bTarget_File || !m_bSave_Param - return!\n");
		return false;
	}

	strcpy(szFile, m_szTargetFile);
	strcat(szFile, "_PAR.dat");
	fp = fopen(szFile, "wb");
	fwrite(m_pParam, 1, m_nParamLen, fp);
	fclose(fp);
	return true;	
}

bool CBurnMgr::Burn_Save_ISP(void)
{
	TRACE("CBurnMgr::Burn_Save_ISP()\n");
	
	char	szFile[256];
	FILE	*fp;
	unsigned long 	iStartAddr, iEndAddr;
	unsigned int	i, j;
	int 	tmp = 0, iFFCount = 0;
	char	szTmp[10];

	if (!m_bTarget_File || !m_bSave_ISP)
	{
		DBG_Print("CBurnMgr::Burn_Save_ISP : !m_bTarget_File || !m_bSave_ISP - return!\n");
		return false;
	}
	
	strcpy(szFile, m_szTargetFile);
	strcat(szFile, "_SEN.txt");
	fp = fopen(szFile, "w+");
	if (m_bIsNewSensorTable)
	{
		TRACE("m_pISP[0] = 0x%.2x\n", m_pISP[0]);
		for (i=1; i<=m_pISP[0]; i++)
		{
			tmp = 0;
			
			// shawn 2009/11/02 modify for 232 and 275 +++++
			if (m_nRomID == SD50232A_2005)
			{
				iStartAddr 	= (m_pISP[2*i-1]<<8 | m_pISP[2*i])   - 0x4800;
				iEndAddr 	= (m_pISP[2*i+1]<<8 | m_pISP[2*i+2]) - 0x4800;
			}
			else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
			{
				iStartAddr 	= (m_pISP[2*i-1]<<8 | m_pISP[2*i])   - 0x6000;
				iEndAddr 	= (m_pISP[2*i+1]<<8 | m_pISP[2*i+2]) - 0x6000;
			}
			else
			{
				iStartAddr 	= (m_pISP[2*i-1]<<8 | m_pISP[2*i])   - 0x8800;
				iEndAddr 	= (m_pISP[2*i+1]<<8 | m_pISP[2*i+2]) - 0x8800;
			}
			// shawn 2009/11/02 modify for 232 and 275 -----
			
			TRACE("iStartAddr = %d, iEndAddr = %d\n", iStartAddr, iEndAddr);
			for (j=iStartAddr; j<iEndAddr; j++)
			{
				if (tmp == 6)
				{
					sprintf(szTmp, "\n");
					fwrite(szTmp, 1, 1, fp);
					tmp = 0;
				}
				sprintf(szTmp, "0x%.2x,  ", m_pISP[j]);
				fwrite(szTmp, 1, 7, fp);
				tmp++;
			}
			sprintf(szTmp, "\n\n");
			fwrite(szTmp, 1, 2, fp);
		}
	}
	else
	{
		for (i=0; i<m_nISPLen; i++)
		{
			if (m_pISP[i] == 0xff)
				iFFCount++;
			else
				iFFCount = 0;
			if (tmp !=0 && (((i-0xc00)%0xc00) == 0))
			{
				sprintf(szTmp, "\n\n");
				fwrite(szTmp, 1, 2, fp);
				tmp = 0;
			}
			if (tmp != 0 && (((i-0xe00)%0xe00) == 0))
			{
				sprintf(szTmp, "\n\n");
				fwrite(szTmp, 1, 2, fp);
				tmp = 0;
			}
			if (iFFCount <= 4)
			{
				if (tmp != 0 && ((tmp%6) == 0))
				{
					sprintf(szTmp, "\n");
					fwrite(szTmp, 1, 1, fp);
					tmp = 0;
				}
				sprintf(szTmp, "0x%.2x,   ", m_pISP[i]);
				fwrite(szTmp, 1, 7, fp);
				tmp++;
			}
		}
	}
	fclose(fp);
	return true;	
}
	
bool CBurnMgr::Burn_Init(void)
{
	TRACE("CBurnMgr::Burn_Init()\n");	
	
	if (!raise_cpu_rate())
	{
		DBG_Print("CBurnMgr::Burn_Init : raise_cpu_rate() - Fail!\n");
	}
	// force to set flash type
	if (m_SFType == SF_TYPE_SST)
	{	
		set_flash_type(SF_TYPE_SST);
	}
	else if (m_SFType == SF_TYPE_PMC)
	{
		set_flash_type(SF_TYPE_PMC);		
	}
	else
		;

	m_WPAddr = 0xffff;
	
	// shawn 2009/08/14 for 233 +++++
	if (m_nRomID == ST50231D_1001)
	{
		// shawn 2009/11/02 modify +++++
		if (m_bSetTo64k)	// shawn 2009/11/06 modify
			m_bWriteProtect = *(m_pRom + 0x000f);
		else
			m_bWriteProtect = *(m_pParam + 0x000f);
		// shawn 2009/11/02 modify -----
	
		TRACE("CBurnMgr::Burn_Init : Get bWriteProtect value at 0x000f = 0x%.2x\n", m_bWriteProtect);
	}
	else
	{
		// shawn 2009/11/02 modify for 232 and 275 +++++
		if (m_bSetTo64k)	// shawn 2009/11/06 modify
		{
			if (m_nRomID == SD50232A_2005 || 
				m_nRomID == ST50275A_3005 || 
				m_nRomID == ST50276A_4006 || 	// shawn 2010/07/13 add 276
				m_nRomID == ST50290A_6007 )		// shawn 2010/12/13 for 290
			{
				m_bWriteProtect = *(m_pRom + 0xC034);
				TRACE("CBurnMgr::Burn_Init : Get bWriteProtect value at 0xC034 = 0x%.2x\n", m_bWriteProtect);
			}
			// shawn 2011/07/25 for 262, 286 and 288 +++++
			else if (m_nRomID == ST5L232A_5007 || 
					 m_nRomID == ST5L286A_4119 || 
					 m_nRomID == ST5L288A_4221)
			{
				unsigned short wTmpAddr = 0;
				
				wTmpAddr = *(m_pRom + 0x160 + 0x08);
				wTmpAddr *= 256;
				wTmpAddr += 0x34;
				m_bWriteProtect = *(m_pRom + wTmpAddr);
				TRACE("CBurnMgr::Burn_Init : Get bWriteProtect value at 0x%.2x = 0x%.2x\n", wTmpAddr, m_bWriteProtect);
			}
			// shawn 2011/07/25 for 262, 286 and 288 -----
			else
			{
				m_bWriteProtect = *(m_pRom + 0x8034);
				TRACE("CBurnMgr::Burn_Init : Get bWriteProtect value at 0x8034 = 0x%.2x\n", m_bWriteProtect);
			}
		}
		else if(m_bSetTo128k) // carol 2013/08/29 add
		{
			DWORD dwTmpAddr1 = 0;
			DWORD dwTmpAddr2 = 0;
			for(int i=0, j=24; i<4; i++, j=j-8)
			{
				dwTmpAddr1 = (*((m_pRom+0x16F)+i) << j);
				dwTmpAddr2 = (dwTmpAddr2 | dwTmpAddr1);
			}
			dwTmpAddr2 += 0x34;
			m_bWriteProtect = *(m_pRom+dwTmpAddr2);
		}
		else
		{
			m_bWriteProtect = *(m_pParam + 0x0034);
			TRACE("CBurnMgr::Burn_Init : Get bWriteProtect value at 0x0034 = 0x%.2x\n", m_bWriteProtect);
		}
		// shawn 2009/11/02 modify for 232 and 275 -----
	}
	// shawn 2009/08/14 for 233 -----
	
	if (!set_write_protect(false, m_WPAddr, m_bWriteProtect))
	{
		DBG_Print("CBurnMgr::Burn_Init : set_write_protect() - Fail!\n");
	}
	
	return true;
}

bool CBurnMgr::Burn_Erase(void)
{
	TRACE("CBurnMgr::Burn_Erase()\n");
	
	if (!m_bFlag_Erase)
	{
		DBG_Print("CBurnMgr::Burn_Erase : bFlag_Erase = false - return!\n");
		return true;
	}
	
	if (!sf_erase(m_SFType))
	{
		DBG_Print("CBurnMgr::Burn_Erase : sf_erase() - Fail!\n");
		return false;
	}
	return true;
}

bool CBurnMgr::Burn_Check(void)
{
	TRACE("CBurnMgr::Burn_Check()\n");
	
	if (!m_bFlag_Check)
	{
		DBG_Print("CBurnMgr::Burn_Check : bFlag_Check = false - return!\n");
		return true;
	}
	
	return Flash_Erase_Verify();
}

bool CBurnMgr::Flash_Erase_Verify(void)
{
	TRACE("CBurnMgr::Flash_Erase_Verify()\n");
	
	BYTE	buf[DEF_LEN_FW_IMG];
	int		i;
	
	if(m_bSetTo64k)
	{
		sleep(1);
		if (!sf_read(0x0000, buf, DEF_LEN_FW_IMG))
		{
			DBG_Print("CBurnMgr::Flash_Erase_Verify : sf_read() - Fail!\n");
			return false;
		}
	}

	else if(m_bSetTo128k) // carol 2013/08/29 add
	{

		if (!sf_read(0x0000, buf, DEF_LEN_FW_IMG))
		{
			DBG_Print("CBurnMgr::Flash_Erase_Verify : sf_read() - Fail!\n");
			return false;
		}
		
		for (i=0; i<DEF_LEN_FW_IMG; i++)
		{
			if (buf[i] != 0xFF)
			{	
				printf("buf[%d]: %x\n", i, buf[i]);
				return false;
			}
		}
		if (!sf_read(0x10000, buf, DEF_LEN_FW_IMG))
		{
			DBG_Print("CBurnMgr::Flash_Erase_Verify : sf_read() - Fail!\n");
			return false;
		}
		
	}
	else
		;	

	for (i=0; i<DEF_LEN_FW_IMG; i++)
	{
		if (buf[i] != 0xFF)
		{	
			return false;
		}
	}
	
	return true;	
}

bool CBurnMgr::Burn_Program(void)
{
	TRACE("CBurnMgr::Burn_Program()\n");
	
	if (m_bSetTo64k)
	{
		if (!sf_write(0, m_pRom, m_nRomLen))
		{
			DBG_Print("Burn_Program : write 64k file - Fail!\n");
			return false;
		}
		TRACE("CBurnMgr::Burn_Program : 64k file program - OK!\n");
	}
	else if(m_bSetTo128k) // carol 2013/08/29 add
	{
		if (!sf_write(0, m_pRom, DEF_LEN_FW_IMG))
		{
			DBG_Print("Burn_Program : write 128k file - Fail!\n");
			return false;
		}
		if (!sf_write(0x10000, m_pRom+0x10000, DEF_LEN_FW_IMG))
		{
			DBG_Print("Burn_Program : write 128k file - Fail!\n");
			return false;
		}
		TRACE("CBurnMgr::Burn_Program : 64k file program - OK!\n");
	}
	else
	{
		unsigned short	address;
		
		if (m_nRomID == ST50231D_1001)	// shawn 2009/11/04 for 233
		{
			// Parameter program
			int string_len[4];
			int param_string_len = PARAM_HEADER_LEN;
			
			// get length of string0
			string_len[0] = m_pParam[param_string_len];
			param_string_len += m_pParam[param_string_len];
			// get length of string1
			string_len[1] = m_pParam[param_string_len];
			param_string_len += m_pParam[param_string_len];
			// get length of string2
			string_len[2] = m_pParam[param_string_len];
			param_string_len += m_pParam[param_string_len];
			// get length of string3
			string_len[3] = m_pParam[param_string_len];
			param_string_len += m_pParam[param_string_len];
	
			// get length of param + string + s3/s4
			m_nInitSensorParamAddr = param_string_len + 7;
			
			// string error handling +++++
			int i = 0;
			int j = 0;
			bool IsStrError = false;
			
			for(unsigned int StrOffset=PARAM_HEADER_LEN ; StrOffset<=param_string_len ; i++)
			{
				if(m_pParam[StrOffset]<2 || m_pParam[StrOffset]>0x3E
					|| m_pParam[StrOffset+1]!=0x03)
				{
					// error string procedure
					m_pParam[StrOffset] = 0x02;
					m_pParam[StrOffset+1] = 0x03;
	
					// shawn 2008/09/01 add for strings error
					unsigned int tmpStrOffset = StrOffset + 2;
					for ( j = i + 1; j <= 3; j++  )
					{
						m_pParam[tmpStrOffset] = 0x02;
						m_pParam[tmpStrOffset+1] = 0x03;
						tmpStrOffset += 2;
					}
					m_pParam[tmpStrOffset] = 0x07;
					for ( j = 1; j <= 6; j++ )
						m_pParam[tmpStrOffset+j] = 0xFF;
					IsStrError = true;
					break;
				}
				else
				{
					// correct string procedure
					TRACE("StrOffset = %x", StrOffset);
				}
				if ( i <= 2 )	// shawn 2008/07/07 modify
					StrOffset += string_len[i];
				else	// for stop the for loop
					StrOffset += 300;
			}
			
			if ( IsStrError )
			{
				param_string_len = PARAM_HEADER_LEN;
				// get length of string0
				param_string_len += m_pParam[param_string_len];
				// get length of string1
				param_string_len += m_pParam[param_string_len];
				// get length of string2
				param_string_len += m_pParam[param_string_len];
				// get length of string3
				param_string_len += m_pParam[param_string_len];
				// get length of param + string +s3/s4
				m_nInitSensorParamAddr = param_string_len + 7;
				
				// shawn 2008/12/17 add for additional parameter
				m_pParam[m_nInitSensorParamAddr] = 0x02;
				m_pParam[m_nInitSensorParamAddr+1] = 0x00;
				
				// Driver parameter
				for ( i = m_nInitSensorParamAddr+2; i < (m_nInitSensorParamAddr+2)+32; i++)
					m_pParam[i] = 0xFF;
	
				// shawn 2009/04/13 add Video Control Data for 233
				for (i = (m_nInitSensorParamAddr+2)+32; i <(m_nInitSensorParamAddr+2)+32+118; i++)
					m_pParam[i] = 0xFF;
			}
			// string error handling -----
			
			address = 0x0000;
				
			if (!sf_write(address, m_pParam, m_nInitSensorParamAddr))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Param data - Fail!\n");
				return false;
			}
			
			// shawn 2008/10/31 add additional parameter
			address = 0x5100;
			
			if(!sf_write(address, m_pParam+m_nInitSensorParamAddr, m_pParam[m_nInitSensorParamAddr]))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Param data (Additional Param) - Fail!\n");
				return false;
			}
			
			// shawn 2008/11/10 add to check version
			if ( ( m_pParam[m_nInitSensorParamAddr+1] != m_pRom[0xFFF] ) &&
				 ( m_pParam[m_nInitSensorParamAddr+1] != 0 ) )	// shawn 2008/12/17 add
				DBG_Print("Firmware code and Parameters version do NOT match!\n");
	
			// shawn 2008/12/16 add driver parameter
			address = 0x5500;
			
			if(!sf_write(address, m_pParam+m_nInitSensorParamAddr+m_pParam[m_nInitSensorParamAddr], 32))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Param data (Driver Param) - Fail!\n");
				return false;
			}
	
			// shawn 2009/04/13 add Video Control Data for 233
			address = 0x1000;
			
			if(!sf_write(address, m_pParam+m_nInitSensorParamAddr+m_pParam[m_nInitSensorParamAddr]+32, 118))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Param data (Video Control Data) - Fail!\n");
				return false;
			}
			
			TRACE("CBurnMgr::Burn_Program : Parameter program - OK!\n");
			
			// InitSensorParam program
			int area_count = m_pISP[0];
			short SensorTableAddr = 0;
			short addr_differ = 0;
	
			SensorTableAddr = m_pISP[1];
			SensorTableAddr = ( SensorTableAddr<<8 ) + m_pISP[2];
						
			addr_differ = ( m_nInitSensorParamAddr + (area_count * 2) + 3 ) - SensorTableAddr;
						
			for ( i = 1; i <= ( area_count + 1 ); i++ )
			{
				SensorTableAddr = m_pISP[(2*i)-1];
				SensorTableAddr = ( SensorTableAddr<<8 ) + m_pISP[2*i];
				SensorTableAddr += addr_differ;
							
				m_pISP[(2*i)-1] = SensorTableAddr>>8;
				m_pISP[2*i] = ( SensorTableAddr<<8 )>>8;
			}					
			
			address = m_nInitSensorParamAddr;
			m_nISPLen = ( (m_pISP[((area_count*2)+1)] << 8) + m_pISP[((area_count*2)+2)] ) - 
									( (m_pISP[1] << 8) + m_pISP[2] ) + 
									( (area_count * 2) + 3 );
			
			if (!sf_write(address, m_pISP, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Init Sensor Param data - Fail!\n");
				return false;
			}
			
			TRACE("CBurnMgr::Burn_Program : InitSensorParam program - OK!\n");
			
			// ROM program
			// empty the first 16 bytes of ROM data in the head
			address = 0x6010;
			if (!sf_write(address, m_pRom+16, 48))
			{
				DBG_Print("Burn_Program : write ROM data #1 - Fail!\n");
				return false;
			}
			address = 0x6040;
			if (!sf_write(address, m_pRom+64, m_nRomLen-64))
			{
				DBG_Print("CBurnMgr::Burn_Program : write ROM data #2 - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Burn_Program : nRomLen = 0x%4x\n", m_nRomLen);
			TRACE("CBurnMgr::Burn_Program : ROM program - OK!\n");
		}
		else
		{	
			// ROM program
			// empty the first 16 bytes of ROM data in the head
			address = 0x0010;
			if (!sf_write(address, m_pRom+16, 48))
			{
				DBG_Print("Burn_Program : write ROM data #1 - Fail!\n");
				return false;
			}
			address = 0x0040;
			if (!sf_write(address, m_pRom+64, m_nRomLen-64))
			{
				DBG_Print("CBurnMgr::Burn_Program : write ROM data #2 - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Burn_Program : nRomLen = 0x%4x\n", m_nRomLen);
			TRACE("CBurnMgr::Burn_Program : ROM program - OK!\n");
			
			// Descriptor program
			// shawn 2009/11/02 modify for 232 and 275 +++++
			if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
			{
					address = 0xD000;
					GenerateDesData();
			}
			else
				address = 0x7800;
			// shawn 2009/11/02 modify for 232 and 275 -----
				
			if (!sf_write(address, m_pDes, m_nDesLen))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Des data - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Burn_Program : Descriptor program - OK!\n");
		
			// Parameter program
			// string error handling
			param_str_err_handling(m_pParam, m_szRomVer);
			// write first parameter
			// shawn 2009/11/02 modify for 232 and 275 +++++
			if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
				address = 0xC000;
			else
				address = 0x8000;
			// shawn 2009/11/02 modify for 232 and 275 -----
			
			if (!sf_write(address, m_pParam, 0x0400))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Param data #1 - Fail!\n");
				return false;
			}	
			// write second parameter
			if (!sf_write(m_DrvParamAddr, m_pParam+0x0400, 0x0400))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Param data #2 - Fail!\n");
				return false;
			}
			TRACE("CBurnMgr::Burn_Program : Parameter program - OK!\n");
			
			// InitSensorParam program
			// shawn 2009/11/02 modify for 232 and 275 +++++
			if (m_nRomID == SD50232A_2005)
				address = 0x4800;
			else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
				address = 0x6000;
			else
				address = 0x8800;
			// shawn 2009/11/02 modify for 232 and 275 -----
			
			if (!sf_write(address, m_pISP, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Burn_Program : write Init Sensor Param data - Fail!\n");
				return false;
			}
			
			TRACE("CBurnMgr::Burn_Program : InitSensorParam program - OK!\n");
		}
	}
	// shawn 2009/08/14 for 233 -----
	
	return true;
}

bool CBurnMgr::Burn_Verify(void)
{
	TRACE("CBurnMgr::Burn_Verify()\n");
	
	unsigned short 	address;
	BYTE	*buf;

	// shawn 2009/11/02 for 232 and 275 +++++	
	unsigned int CRCBufferLen = m_nCRC16Len;
	bool success = true;
	unsigned short CRCData;
	int i, j;
	// shawn 2009/11/02 for 232 and 275 -----
	
	if (m_bSetTo64k)
	{
		address = 0x0000;
		buf = (BYTE *)malloc(m_nRomLen);
		if (!read_flash_rom(buf, m_nRomLen))
		{
			DBG_Print("CBurnMgr::Burn_Verify : 64k file read_flash_rom() - Fail!\n");
			goto fail_exit;
		}
		if (!_buf_cmp(m_pRom, buf, m_nRomLen))
		{
			DBG_Print("CBurnMgr::Burn_Verify : 64k file _buf_cmp() - Fail!\n");
			goto fail_exit;
		}
		free(buf);
		TRACE("CBurnMgr::Burn_Verify : verify 64k file - OK!\n");
	}
	else if(m_bSetTo128k) // carol 2013/08/29 add
	{
		buf = (BYTE *)malloc(NEW_LEN_FW_IMG);
		if (!sf_read(0x0000, buf, DEF_LEN_FW_IMG))
		{
			DBG_Print("CBurnMgr::Flash_Erase_Verify : sf_read() - Fail!\n");
			return false;
		}
		if (!sf_read(0x10000, (unsigned char*)&buf[DEF_LEN_FW_IMG], DEF_LEN_FW_IMG))
		{
			DBG_Print("CBurnMgr::Flash_Erase_Verify : sf_read() - Fail!\n");
			return false;
		}

		if (!_buf_cmp(m_pRom, buf, m_nRomLen))
		{
			DBG_Print("CBurnMgr::Burn_Verify : 128k file _buf_cmp() - Fail!\n");
			goto fail_exit;
		}
		free(buf);
		TRACE("CBurnMgr::Burn_Verify : verify 128k file - OK!\n");
	}
	else
	{
		if (m_nRomID == ST50231D_1001)	// shawn 2009/11/04 for 233
		{
			// verify Parameter
			buf = (BYTE *)malloc(m_nInitSensorParamAddr);
			if (!read_flash_param(buf, 0))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter read_flash_param() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pParam, buf, m_nInitSensorParamAddr))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
					
			// shawn 2008/10/31 add additional parameter
			buf = (BYTE *)malloc(m_pParam[m_nInitSensorParamAddr]);
			address = 0x5100;
	
			if(!sf_read(address, buf, m_pParam[m_nInitSensorParamAddr]))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter sf_read() (Additional Param) - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pParam+m_nInitSensorParamAddr, buf, m_pParam[m_nInitSensorParamAddr]))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter _buf_cmp() (Additional Param) - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			
			// shawn 2008/12/16 add driver parameter
			buf = (BYTE *)malloc(32);
			address = 0x5500;
			
			if(!sf_read(address, buf, 32))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter sf_read() (Driver Param) - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pParam+m_nInitSensorParamAddr+m_pParam[m_nInitSensorParamAddr], buf, 32))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter _buf_cmp() (Driver Param) - Fail!\n");
				goto fail_exit;
			}
			free(buf);
	
			// shawn 2009/04/13 add Video Control Data for 233
			buf = (BYTE *)malloc(118);
			address = 0x1000;
			
			if(!sf_read(address, buf, 118))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter sf_read() (Video Control Data) - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pParam+m_nInitSensorParamAddr+m_pParam[m_nInitSensorParamAddr]+32, buf, 118))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter _buf_cmp() (Video Control Data) - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify Parameter - OK!\n");
			
			// verify InitSensorParam
			buf = (BYTE *)malloc(m_nISPLen);
			if (!read_flash_ISP(buf, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Init Sensor Parameter read_flash_ISP() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pISP, buf, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Init Sensor Parameter _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify Init Sensor Parameter - OK!\n");
			
			// verify ROM
			address = 0x6000;
			buf = (BYTE *)malloc(m_nRomLen);
			if (!read_flash_rom(buf, m_nRomLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : ROM read_flash_rom() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pRom+16, buf+16, m_nRomLen-16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : ROM _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify ROM - OK!\n");
			
			// all check ok! -> write the first 16 bytes of rom
			address = 0x6000;	
			buf = (BYTE *)malloc(16);
			if (!sf_write(address, m_pRom, 16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : write the first 16 bytes of ROM - Fail!\n");
				goto fail_exit;
			}
			// read back the 16 bytes & check again
			if (!sf_read(address, buf, 16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : read back the first 16 bytes of ROM - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pRom, buf, 16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : compare the first 16 bytes of ROM - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify the first 16 bytes of ROM - OK!\n");
		}
		else
		{
			// verify ROM
			address = 0x0000;
			buf = (BYTE *)malloc(m_nRomLen);
			if (!read_flash_rom(buf, m_nRomLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : ROM read_flash_rom() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pRom+16, buf+16, m_nRomLen-16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : ROM _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify ROM - OK!\n");
			
			// verify Descriptor
			address = 0x7800;
			buf = (BYTE *)malloc(m_nDesLen);
			if (!read_flash_des(buf, m_nDesLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Descriptor read_flash_des() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pDes, buf, m_nDesLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Descriptor _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify Descriptor - OK!\n");
			
			// verify Parameter
			// shawn 2009/11/02 modify for 232 and 275 +++++
			if (m_nRomID == SD50232A_2005 || m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
				address = 0xC000;
			else
				address = 0x8000;
			// shawn 2009/11/02 modify for 232 and 275 -----
			
			buf = (BYTE *)malloc(m_nParamLen);
			if (!read_flash_param(buf, m_DrvParamAddr))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter read_flash_param() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pParam, buf, m_nParamLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Parameter _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify Parameter - OK!\n");
			
			// verify InitSensorParam
			// shawn 2009/11/02 modify for 232 and 275 +++++
			if (m_nRomID == SD50232A_2005)
				address = 0x4800;
			else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
				address = 0x6000;
			else
				address = 0x8800;
			// shawn 2009/11/02 modify for 232 and 275 -----
			
			buf = (BYTE *)malloc(m_nISPLen);
			if (!read_flash_ISP(buf, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Init Sensor Parameter read_flash_ISP() - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pISP, buf, m_nISPLen))
			{
				DBG_Print("CBurnMgr::Burn_Verify : Init Sensor Parameter _buf_cmp() - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify Init Sensor Parameter - OK!\n");
			
			// verify Defect Pos
			if (m_bDefectPos)
			{
				buf = (BYTE *)malloc(m_nDefectPosLen);
				if (!sf_read(0xA800, buf, m_nDefectPosLen))
				{
					DBG_Print("CBurnMgr::Burn_Verify : Defect Pos sf_read() - Fail!\n");
					goto fail_exit;
				}
				if (!_buf_cmp(m_pDefectPos, buf, m_nDefectPosLen))
				{
					DBG_Print("CBurnMgr::Burn_Verify : Defect Pos _buf_cmp() - Fail!\n");
					goto fail_exit;
				}
				free(buf);
				TRACE("CBurnMgr::Burn_Verify : verify Defect Pos - OK!\n");
			}
			
			// verify Custom Space
			if (m_bCustomSpace)
			{
				buf = (BYTE *)malloc(m_nCustomSpaceLen);
				if (!sf_read(0xB800, buf, m_nCustomSpaceLen))
				{
					DBG_Print("CBurnMgr::Burn_Verify : Custom Space sf_read() - Fail!\n");
					goto fail_exit;
				}
				if (!_buf_cmp(m_pCustomSpace, buf, m_nCustomSpaceLen))
				{
					DBG_Print("CBurnMgr::Burn_Verify : Custom Space _buf_cmp() - Fail!\n");
					goto fail_exit;
				}
				free(buf);
				TRACE("CBurnMgr::Burn_Verify : verify Custom Space - OK!\n");
			}
		
			// all check ok! -> write the first 16 bytes of rom
			address = 0x0000;	
			buf = (BYTE *)malloc(16);
			if (!sf_write(address, m_pRom, 16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : write the first 16 bytes of ROM - Fail!\n");
				goto fail_exit;
			}
			// read back the 16 bytes & check again
			if (!sf_read(address, buf, 16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : read back the first 16 bytes of ROM - Fail!\n");
				goto fail_exit;
			}
			if (!_buf_cmp(m_pRom, buf, 16))
			{
				DBG_Print("CBurnMgr::Burn_Verify : compare the first 16 bytes of ROM - Fail!\n");
				goto fail_exit;
			}
			free(buf);
			TRACE("CBurnMgr::Burn_Verify : verify the first 16 bytes of ROM - OK!\n");
		}
		
		// shawn 2009/11/02 for 232 and 275 +++++
		address = 0xCF00;
		
		if (m_nRomID == SD50232A_2005)
		{
			// CRC_Init
			DoCRC16(&CRCData, 0, 4096, ROM_BUF);
			m_pCRC16[0] = CRCData >> 8;
			m_pCRC16[1] = CRCData & 0xFF;
	
			// CRC_0
			DoCRC16(&CRCData, 0, 2048, ROM_BUF);
			m_pCRC16[2] = CRCData >> 8;
			m_pCRC16[3] = CRCData & 0xFF;
					
			// CRC_1 ~ CRC_7
			for ( i = 4096, j = 4; i < 18432; i += 2048, j += 2 )
			{
				DoCRC16(&CRCData, i, (i+2048), ROM_BUF);
				m_pCRC16[j] = CRCData >> 8;
				m_pCRC16[j+1] = CRCData & 0xFF;	
			}
	
			// CRC_Tables
			DoCRC16(&CRCData, 0, DEF_LEN_ISP30, TABLE_BUF);
			m_pCRC16[18] = CRCData >> 8;
			m_pCRC16[19] = CRCData & 0xFF;
	
			// CRC_Parameter
			DoCRC16(&CRCData, 0, DEF_LEN_PARAM4, PARAM_BUF);
			m_pCRC16[20] = CRCData >> 8;
			m_pCRC16[21] = CRCData & 0xFF;
	
			// CRC_Descriptor
			DoCRC16(&CRCData, 0, DEF_LEN_DES4, DES_BUF);
			m_pCRC16[22] = CRCData >> 8;
			m_pCRC16[23] = CRCData & 0xFF;
					
			// Write to SF
			if (!sf_write(address, m_pCRC16, CRCBufferLen))
			{
				DBG_Print("Write CRC16 data Failed!");
				success = false;
			}
	
			// Verify CRC16 data
			BYTE *pBufferCRC=(BYTE *)malloc(m_nCRC16Len);
			
			if(success && !sf_read(address, pBufferCRC, m_nCRC16Len))
			{
				DBG_Print("Read CRC16 data Failed!");
				success = false;
			}
	
			if(success && !_buf_cmp(m_pCRC16, pBufferCRC, m_nCRC16Len))
			{
				DBG_Print("CRC16 Data Compare Fail!");
				success = false;
			}
					
			if(pBufferCRC != NULL)
			{
				free(pBufferCRC);
				pBufferCRC = NULL;
			}			
					
			if(!success)
				goto fail_exit;
		}
		else if (m_nRomID == ST50275A_3005 || m_nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		{
			// CRC_Init
			DoCRC16(&CRCData, 0, 16384, ROM_BUF);
			m_pCRC16[0] = CRCData >> 8;
			m_pCRC16[1] = CRCData & 0xFF;
	
			// CRC_0
			DoCRC16(&CRCData, 0, 4096, ROM_BUF);
			m_pCRC16[2] = CRCData >> 8;
			m_pCRC16[3] = CRCData & 0xFF;
					
			// CRC_1 ~ CRC_2
			for ( i = 16384, j = 4; i < 24576; i += 4096, j += 2 )
			{
				DoCRC16(&CRCData, i, (i+4096), ROM_BUF);
				m_pCRC16[j] = CRCData >> 8;
				m_pCRC16[j+1] = CRCData & 0xFF;	
			}
	
			// CRC_Tables
			DoCRC16(&CRCData, 0, DEF_LEN_ISP24, TABLE_BUF);
			m_pCRC16[18] = CRCData >> 8;
			m_pCRC16[19] = CRCData & 0xFF;
	
			// CRC_Parameter
			DoCRC16(&CRCData, 0, DEF_LEN_PARAM4, PARAM_BUF);
			m_pCRC16[20] = CRCData >> 8;
			m_pCRC16[21] = CRCData & 0xFF;
	
			// CRC_Descriptor
			DoCRC16(&CRCData, 0, DEF_LEN_DES4, DES_BUF);
			m_pCRC16[22] = CRCData >> 8;
			m_pCRC16[23] = CRCData & 0xFF;
					
			// Write to SF
			if (!sf_write(address, m_pCRC16, CRCBufferLen))
			{
				DBG_Print("Write CRC16 data Failed!");
				success = false;
			}
	
			// Verify CRC16 data
			BYTE *pBufferCRC=(BYTE *)malloc(m_nCRC16Len);
			
			if(success && !sf_read(address, pBufferCRC, m_nCRC16Len))
			{
				DBG_Print("Read CRC16 data Failed!");
				success = false;
			}
	
			if(success && !_buf_cmp(m_pCRC16, pBufferCRC, m_nCRC16Len))
			{
				DBG_Print("CRC16 Data Compare Fail!");
				success = false;
			}
					
			if(pBufferCRC != NULL)
			{
				free(pBufferCRC);
				pBufferCRC = NULL;
			}			
					
			if(!success)
				goto fail_exit;
		}
		// shawn 2009/11/02 for 232 and 275 -----
	}
	// shawn 2009/08/14 for 233 -----

	return true;
	
fail_exit:
	free(buf);
	return false;	
}

bool CBurnMgr::Burn_EndProc(bool bStatus)
{
	TRACE("CBurnMgr::Burn_EndProc()\n");
	
	// down cpu rate
	if (!down_cpu_rate())
	{
		DBG_Print("CBurnMgr::Burn_EndProc : down_cpu_rate() - Fail!\n");
	}
	
	if (!set_write_protect(true, m_WPAddr, m_bWriteProtect))
	{
		DBG_Print("CBurnMgr::Burn_EndProc : set_write_protect() - Fail!\n");
		return false;
	}
	return true;	
}

// shawn 2009/11/02 for 232 and 275
bool CBurnMgr::DoCRC16(unsigned short *CRC16, int start, int end, int Type)
{
	unsigned short tmpCRC16 = 0xFFFF;
	BYTE temp = 0;
	int i, j;
		
	for ( i = start; i < end; i++ )
	{
		// shawn 2009/05/04 add
		switch (Type)
		{
		case ROM_BUF:
			temp = m_pRom[i];
			break;
		case TABLE_BUF:
			temp = m_pISP[i];
			break;
		case PARAM_BUF:
			temp = m_pParam[i];
			break;
		case DES_BUF:
			temp = m_pDes[i];
			break;
		}
		
		for ( j = 0; j < 8; j++ )
		{
			if ( (tmpCRC16 ^ (temp>>j)) & 0x1 )
			{
				tmpCRC16 >>= 1;
				tmpCRC16 ^= 0xA001;
			}
			else
				tmpCRC16 >>= 1;
		}
	}		

	*CRC16 = tmpCRC16;
	return true;	
}

// shawn 2009/11/02 for 232 and 275
void CBurnMgr::GenerateDesData()
{
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int m = 0;
	int n = 0;
	int iTotalLen = 0;
	int iTotalAddr = 0;
	int iSpeedType = 0;
	int iTmpLen = 0;
	int iTmpAddr = 0;
	//int iTmpCfgDesLen = 0;
	//int iTmpCfgDesLenAddr = 0;
	int iTmp = 0;
	int iTmpbSourceIDAddr = 0;	// shawn 2009/09/23 for XU
	int iTmpXUNum = 0;			// shawn 2009/09/23 for XU
	BYTE bmAttributes = 0;
	BYTE MaxPower = 0;
	BYTE iFunction = 0;
	BYTE byCTControl1 = 0;
	BYTE byCTControl2 = 0;
	BYTE byCTControl3 = 0;
	BYTE byPULen = 0;
	BYTE byPUControl1 = 0;
	BYTE byPUControl2 = 0;
	BYTE byPUControl3 = 0;
	BYTE bLength = 0;
	BYTE bNumFormats = 0;
	BYTE bStillCaptureMethod = 0;
	BYTE bTriggerSupport = 0;
	BYTE bTriggerUsage = 0;
	BYTE bNumFrameDescriptors = 0;
	BYTE bDefaultFrameIndex = 0;
	BYTE bSourceID = 0;		// shawn 2009/09/23 for XU
	BYTE bUnitID = 0;		// shawn 2009/09/23 for XU
	BYTE baSourceID = 0;	// shawn 2009/09/23 for XU
	BYTE bNumControls = 0;	// shawn 2009/09/23 for XU
	unsigned short wWidth = 0;
	unsigned short wHeight = 0;
	unsigned long dwMinBitRate = 0;
	unsigned long dwMaxBitRate = 0;
	unsigned long dwMaxVideoFrameBufferSize = 0;
	unsigned long dwDefaultFrameInterval;
	BYTE bFrameIntervalType = 0;
	unsigned long dwFrameInterval = 0;
	BYTE bNumImageSizePatterns = 0;
	bool IsStillEmpty = false;

	bmAttributes = 0x80 + ((m_pParam[0x0022] & 0x02) ? 0x20 : 0x00);
	MaxPower = m_pParam[0x000c];
	iFunction = (m_pParam[0x0023] & 0x10) ? 0x00 : 0x05;
	byCTControl1 = m_pParam[0x0015];
	byCTControl2 = m_pParam[0x0016];
	byCTControl3 = m_pParam[0x0017];
	byPULen = (m_pParam[0x001d] & 0x80) ? 2 : 3;
	byPUControl1 = m_pParam[0x001A];
	byPUControl2 = m_pParam[0x001B];
	byPUControl3 = m_pParam[0x001C];
	bStillCaptureMethod = (m_pParam[0x001D] & 0x0C) >> 2;
	bTriggerSupport = (m_pParam[0x000D] & 0x04) ? 0x01 : 0x00;
	bTriggerUsage = (m_pParam[0x000D] & 0x08) ? 0x01 : 0x00;

	memset(m_pDes, 0xff, m_nDesLen);
	
	for (iSpeedType = HIGH_SPEED; iSpeedType <= FULL_SPEED; iSpeedType++)
	{
		bLength = 0x0E + m_abyNumOfFmt[iSpeedType] - 1;
		bNumFormats = m_abyNumOfFmt[iSpeedType];
		iTotalLen = 0;
		iTotalAddr = 0;
		
		// Configuration Descriptor +++++
		m_pDes[i]   = 0x09;	// bLength
		m_pDes[i+1] = 0x02;	// bDescriptorType
		m_pDes[i+4] = 0x02;	// bNumInterfaces
		m_pDes[i+5] = 0x01;	// bConfigurationValue
		m_pDes[i+6] = 0x00;	// iConfiguration
		m_pDes[i+7] = bmAttributes;	// bmAttributes
		m_pDes[i+8] = MaxPower;		// MaxPower
		
		iTotalAddr = i + 2;
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Configuration Descriptor -----

		// shawn 2009/08/21 for Dell +++++
/*#if defined(_TOKACHI)
		m_pDesBuffer[i]    = 0x28;	// bLength
		m_pDesBuffer[i+1]  = 0xFF;	// bDescriptorType
		m_pDesBuffer[i+2]  = 0x42;
		m_pDesBuffer[i+3]  = 0x49;
		m_pDesBuffer[i+4]  = 0x53;
		m_pDesBuffer[i+5]  = 0x54;
		m_pDesBuffer[i+6]  = 0x00;
		m_pDesBuffer[i+7]  = 0x01;
		m_pDesBuffer[i+8]  = 0x06;
		m_pDesBuffer[i+9]  = 0x01;
		m_pDesBuffer[i+10] = 0x10;
		m_pDesBuffer[i+11] = 0x00;
		m_pDesBuffer[i+12] = 0x00;
		m_pDesBuffer[i+13] = 0x00;
		m_pDesBuffer[i+14] = 0x00;
		m_pDesBuffer[i+15] = 0x00;
		m_pDesBuffer[i+16] = 0xD1;
		m_pDesBuffer[i+17] = 0x10;
		m_pDesBuffer[i+18] = 0xF4;
		m_pDesBuffer[i+19] = 0x01;
		m_pDesBuffer[i+20] = 0xD2;
		m_pDesBuffer[i+21] = 0x11;
		m_pDesBuffer[i+22] = 0xF4;
		m_pDesBuffer[i+23] = 0x01;
		m_pDesBuffer[i+24] = 0xD3;
		m_pDesBuffer[i+25] = 0x12;
		m_pDesBuffer[i+26] = 0xF4;
		m_pDesBuffer[i+27] = 0x01;
		m_pDesBuffer[i+28] = 0xD4;
		m_pDesBuffer[i+29] = 0x13;
		m_pDesBuffer[i+30] = 0xF4;
		m_pDesBuffer[i+31] = 0x01;
		m_pDesBuffer[i+32] = 0xD5;
		m_pDesBuffer[i+33] = 0x14;
		m_pDesBuffer[i+34] = 0xF4;
		m_pDesBuffer[i+35] = 0x01;
		m_pDesBuffer[i+36] = 0xD6;
		m_pDesBuffer[i+37] = 0x15;
		m_pDesBuffer[i+38] = 0xF4;
		m_pDesBuffer[i+39] = 0x01;

		iTotalLen += m_pDesBuffer[i];
		i += m_pDesBuffer[i];
#endif*/
		// shawn 2009/08/21 for Dell -----
		
		// IAD Descriptor +++++
		m_pDes[i]   = 0x08;	// bLength
		m_pDes[i+1] = 0x0B;	// bDescriptorType
		m_pDes[i+2] = 0x00;	// bFirstInterface
		m_pDes[i+3] = 0x02;	// bInterfaceCount
		m_pDes[i+4] = 0x0E;	// bFunctionClass
		m_pDes[i+5] = 0x03;	// bFunctionSubClass
		m_pDes[i+6] = 0x00;	// bFunctionProtocol
		m_pDes[i+7] = iFunction;	// iFunction

		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// IAD Descriptor -----

		// Interface Descriptor +++++
		m_pDes[i]   = 0x09;	// bLength
		m_pDes[i+1] = 0x04;	// bDescriptorType
		m_pDes[i+2] = 0x00;	// bInterfaceNumber
		m_pDes[i+3] = 0x00;	// bAlternateSetting
		m_pDes[i+4] = 0x01;	// bNumEndpoints
		m_pDes[i+5] = 0x0E;	// bInterfaceClass
		m_pDes[i+6] = 0x01;	// bInterfaceSubClass
		m_pDes[i+7] = 0x00;	// bInterfaceProtocol
		m_pDes[i+8] = iFunction;	// iInterface

		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Interface Descriptor -----

		// Class-Specific Video Control Interface Header Descriptor +++++
		m_pDes[i]    = 0x0D;	// bLength
		m_pDes[i+1]  = 0x24;	// bDescriptorType
		m_pDes[i+2]  = 0x01;	// bDescriptorSubtype
		m_pDes[i+3]  = 0x00;	// bcdVDC (L)
		m_pDes[i+4]  = 0x01;	// bcdVDC (H)
		m_pDes[i+7]  = 0xC0;	// dwClockFrequency (0)
		m_pDes[i+8]  = 0xE1;	// dwClockFrequency (1)
		m_pDes[i+9]  = 0xE4;	// dwClockFrequency	(2)
		m_pDes[i+10] = 0x00;	// dwClockFrequency (3)
		m_pDes[i+11] = 0x01;	// bInCollection
		m_pDes[i+12] = 0x01;	// baInterfaceNr[1]

		iTmpAddr = i + 5;
		iTmpLen += m_pDes[i];
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Class-Specific Video Control Interface Header Descriptor -----

		// Video Control Output Terminal Descriptor +++++
		m_pDes[i]   = 0x09;	// bLength
		m_pDes[i+1] = 0x24;	// bDescriptorType
		m_pDes[i+2] = 0x03;	// bDescriptorSubtype
		m_pDes[i+3] = 0x02;	// bTerminalID
		m_pDes[i+4] = 0x01;	// wTerminalType (L)
		m_pDes[i+5] = 0x01;	// wTerminalType (H)
		m_pDes[i+6] = 0x00;	// bAssocTerminal
		//m_pDesBuffer[i+7] = 0x05;	// bSourceID
		iTmpbSourceIDAddr = i + 7;	// shawn 2009/09/23 for XU
		m_pDes[i+8] = 0x00;	// iTerminal

		iTmpLen += m_pDes[i];
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Video Control Output Terminal Descriptor -----

		// Video Control Extension Unit Descriptor +++++
		// shawn 2009/09/23 for XU +++++
		bUnitID = 0x04;
		baSourceID = 0x03;

		for (j = 0; j < byActualXUNum; j++)
		{
			for (k = 0; k < CTRLNUM; k++)
			{
				if ( (awXUCTRL[j]>>k)&0x1 )
					bNumControls++;
			}
				
			// shawn 2009/10/08 modify +++++
			if (bNumControls > 8)
			{
				m_pDes[i]    = 0x1B;				// bLength
				m_pDes[i+20] = 0x10;				// bNumControls
				m_pDes[i+23] = 0x02;				// bControlSize
				m_pDes[i+24] = awXUCTRL[j]&0xff;	// bmControls (L)
				m_pDes[i+25] = awXUCTRL[j]>>8;	// bmControls (H)
				m_pDes[i+26] = 0x00;				// iExtension
			}
			else
			{
				m_pDes[i]    = 0x1A;			// bLength
				m_pDes[i+20] = 0x08;			// bNumControls
				m_pDes[i+23] = 0x01;			// bControlSize
				m_pDes[i+24] = awXUCTRL[j];	// bmControls
				m_pDes[i+25] = 0x00;			// iExtension
			}
							
			m_pDes[i+1]  = 0x24;			// bDescriptorType
			m_pDes[i+2]  = 0x06;			// bDescriptorSubtype
			m_pDes[i+3]  = bUnitID;		// bUnitID
			memcpy(m_pDes+i+4, &aGUIDXU[j], sizeof(GUID));	// guidExtensionCode
			m_pDes[i+21] = 0x01;			// bNrInPins
			m_pDes[i+22] = baSourceID;	// baSourceID[1]
			// shawn 2009/10/08 modify -----

			bSourceID = baSourceID + 1;
			bNumControls = 0;
			bUnitID++;
			baSourceID++;
			iTmpXUNum++;

			iTmpLen += m_pDes[i];
			iTotalLen += m_pDes[i];
			i += m_pDes[i];
		}

		if (!iTmpXUNum)
		{
			// Default XU1 +++++
			m_pDes[i]    = 0x1A;	// bLength
			m_pDes[i+1]  = 0x24;	// bDescriptorType
			m_pDes[i+2]  = 0x06;	// bDescriptorSubtype
			m_pDes[i+3]  = 0x04;	// bUnitID
			
			memcpy(m_pDes+i+4, &XU1GUID, sizeof(GUID));
			m_pDes[i+20] = 0x08;	// bNumControls
			m_pDes[i+21] = 0x01;	// bNrInPins
			m_pDes[i+22] = 0x03;	// baSourceID[1]
			m_pDes[i+23] = 0x01;	// bControlSize
			m_pDes[i+24] = 0x0F;	// bmControls
			m_pDes[i+25] = 0x00;	// iExtension

			iTmpLen += m_pDes[i];
			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Default XU1 -----

			// Default XU2 +++++
			/*m_pDesBuffer[i]    = 0x1A;	// bLength
			m_pDesBuffer[i+1]  = 0x24;	// bDescriptorType
			m_pDesBuffer[i+2]  = 0x06;	// bDescriptorSubtype
			m_pDesBuffer[i+3]  = 0x05;	// bUnitID
						
			memcpy(m_pDesBuffer+i+4, &XU2GUID, sizeof(GUID));							
			m_pDesBuffer[i+20] = 0x08;	// bNumControls
			m_pDesBuffer[i+21] = 0x01;	// bNrInPins
			m_pDesBuffer[i+22] = 0x04;	// baSourceID[1]
			m_pDesBuffer[i+23] = 0x01;	// bControlSize
			m_pDesBuffer[i+24] = 0xFF;	// bmControls
			m_pDesBuffer[i+25] = 0x00;	// iExtension

			bSourceID = m_pDesBuffer[i+22] + 1;
			iTmpLen += m_pDesBuffer[i];
			iTotalLen += m_pDesBuffer[i];
			i += m_pDesBuffer[i];*/
			// Default XU2 -----
		}

		m_pDes[iTmpbSourceIDAddr] = bSourceID;	// Video Control Output Terminal Descriptor => bSourceID
		// shawn 2009/09/23 for XU -----
		// Video Control Extension Unit Descriptor -----

		// Video Control Input Terminal Descriptor +++++
		m_pDes[i]    = 0x12;	// bLength
		m_pDes[i+1]  = 0x24;	// bDescriptorType
		m_pDes[i+2]  = 0x02;	// bDescriptorSubtype
		m_pDes[i+3]  = 0x01;	// bTerminalID
		m_pDes[i+4]  = 0x01;	// wTerminalType (L)
		m_pDes[i+5]  = 0x02;	// wTerminalType (H)
		m_pDes[i+6]  = 0x00;	// bAssocTerminal
		m_pDes[i+7]  = 0x00;	// iTerminal
		m_pDes[i+8]  = 0x00;	// wObjectiveFocalLengthMin (L)
		m_pDes[i+9]  = 0x00;	// wObjectiveFocalLengthMin (H)
		m_pDes[i+10] = 0x00;	// wObjectiveFocalLengthMax (L)
		m_pDes[i+11] = 0x00;	// wObjectiveFocalLengthMax (H)
		m_pDes[i+12] = 0x00;	// wOcularFocalLength (L)
		m_pDes[i+13] = 0x00;	// wOcularFocalLength (H)
		m_pDes[i+14] = 0x03;	// bControlSize
		m_pDes[i+15] = byCTControl1;	// bmControls1
		m_pDes[i+16] = byCTControl2;	// bmControls2
		m_pDes[i+17] = byCTControl3;	// bmControls3

		iTmpLen += m_pDes[i];
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Video Control Input Terminal Descriptor -----

		// Video Control Processing Unit Descriptor +++++
		if ( byPULen == 3)			// bLength
			m_pDes[i] = 0x0C;
		else
			m_pDes[i] = 0x0B;

		m_pDes[i+1]  = 0x24;	// bDescriptorType
		m_pDes[i+2]  = 0x05;	// bDescriptorSubtype
		m_pDes[i+3]  = 0x03;	// bUnitID
		m_pDes[i+4]  = 0x01;	// bSourceID
		m_pDes[i+5]  = 0x00;	// wMaxMultiplier (L)
		m_pDes[i+6]  = 0x00;	// wMaxMultiplier (H)
		m_pDes[i+7]  = 0x02;	// bControlSize
		m_pDes[i+8]  = byPUControl1;	// bmControls1
		m_pDes[i+9]  = byPUControl2;	// bmControls2

		if ( byPULen == 3)
		{
			m_pDes[i+10] = byPUControl3;	// bmControls3
			m_pDes[i+11] = 0x00;			// iProcessing
		}
		else
			m_pDes[i+10] = 0x00;			// iProcessing

		iTmpLen += m_pDes[i];
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Video Control Processing Unit Descriptor -----

		// Class-Specific Video Control Interface Header Descriptor Total Length +++++
		m_pDes[iTmpAddr] = iTmpLen & 0xFF;	// wTotalLength (L)
		m_pDes[iTmpAddr+1] = iTmpLen >> 8;	// wTotalLength (H)

		iTmpLen = 0;
		iTmpAddr = 0;
		// Class-Specific Video Control Interface Header Descriptor Total Length -----

		// Endpoint Descriptor +++++
		m_pDes[i]   = 0x07;	// bLength
		m_pDes[i+1] = 0x05;	// bDescriptorType
		m_pDes[i+2] = 0x83;	// bEndpointAddress
		m_pDes[i+3] = 0x03;	// bmAttributes
		m_pDes[i+4] = 0x10;	// wMaxPacketSize (L)
		m_pDes[i+5] = 0x00;	// wMaxPacketSize (H)
		m_pDes[i+6] = 0x06;	// bInterval

		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Endpoint Descriptor -----

		// Class-specific VC Interrupt Endpoint Descriptor +++++
		m_pDes[i]   = 0x05;	// bLength
		m_pDes[i+1] = 0x25;	// bDescriptorType
		m_pDes[i+2] = 0x03;	// bDescriptorSubtype
		m_pDes[i+3] = 0x10;	// wMaxTransferSize (L)
		m_pDes[i+4] = 0x00;	// wMaxTransferSize (H)

		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Class-specific VC Interrupt Endpoint Descriptor -----

		// Interface Descriptor +++++
		m_pDes[i]   = 0x09;	// bLength
		m_pDes[i+1] = 0x04;	// bDescriptorType
		m_pDes[i+2] = 0x01;	// bInterfaceNumber
		m_pDes[i+3] = 0x00;	// bAlternateSetting
		m_pDes[i+4] = 0x00;	// bNumEndpoints
		m_pDes[i+5] = 0x0E;	// bInterfaceClass
		m_pDes[i+6] = 0x02;	// bInterfaceSubClass
		m_pDes[i+7] = 0x00;	// bInterfaceProtocol
		m_pDes[i+8] = iFunction;	// iInterface

		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Interface Descriptor -----

		// Video Class-Specific VS Video Input Header Descriptor +++++
		m_pDes[i]    = bLength;				// bLength
		m_pDes[i+1]  = 0x24;	// bDescriptorType
		m_pDes[i+2]  = 0x01;	// bDescriptorSubtype
		m_pDes[i+3]  = bNumFormats;			// bNumFormats
		m_pDes[i+6]  = 0x81;	// bEndpointAddress
		m_pDes[i+7]  = 0x00;	// bmInfo
		m_pDes[i+8]  = 0x02;	// bTerminalLink
		
		IsStillEmpty = true;
		
		for (j = 0; j < FMTNUM; j++)
		{
			if (m_abyNumOfResFmtStill[iSpeedType][j])
			{
				IsStillEmpty = false;
				break;
			}
		}
		
		if (IsStillEmpty)
			m_pDes[i+9]  = 0;						// bStillCaptureMethod
		else
			m_pDes[i+9]  = bStillCaptureMethod;	// bStillCaptureMethod

		m_pDes[i+10] = bTriggerSupport;			// bTriggerSupport
		m_pDes[i+11] = bTriggerUsage;				// bTriggerUsage
		m_pDes[i+12] = 0x01;	// bControlSize
		m_pDes[i+13] = 0x00;	// Video Payload Format 1

		if (m_abyNumOfFmt[iSpeedType] == 2)
			m_pDes[i+14] = 0x00;	// Video Payload Format 2
		else if (m_abyNumOfFmt[iSpeedType] == 3)
		{
			m_pDes[i+14] = 0x00;	// Video Payload Format 2
			m_pDes[i+15] = 0x00;	// Video Payload Format 3
		}

		iTmpAddr = i + 4;
		iTmpLen += m_pDes[i];
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Video Class-Specific VS Video Input Header Descriptor -----

		// Format, Rseolution and Frame Rate for Video and Still +++++
		for (j = 0; j < m_abyNumOfFmt[iSpeedType]; j++)
		{
			bNumFrameDescriptors = m_abyNumOfResFmtVideo[iSpeedType][j];
			
			for (k = 0; k < FMTNUM; k++)
			{
				if (m_abyFmtIdx[iSpeedType][k] == j+1)
					break;
			}

			switch (k)
			{
			case 0:
				if (iSpeedType == HIGH_SPEED)
					bDefaultFrameIndex = m_pParam[0x001E];
				else
					bDefaultFrameIndex = m_pParam[0x0020];

				// Video Streaming Uncompressed Format Type Descriptor (YUY2) +++++
				m_pDes[i]    = 0x1B;	// bLength
				m_pDes[i+1]  = 0x24;	// bDescriptorType
				m_pDes[i+2]  = 0x04;	// bDescriptorSubtype
				m_pDes[i+3]  = j + 1;	// bFormatIndex
				m_pDes[i+4]  = bNumFrameDescriptors;	// bNumFrameDescriptors
				
				memcpy(m_pDes+i+5, &YUY2GUID, sizeof(GUID));
				m_pDes[i+21] = 0x10;	// bBitsPerPixel
				m_pDes[i+22] = bDefaultFrameIndex;	// bDefaultFrameIndex
				m_pDes[i+23] = 0x00;	// bAspectRatioX
				m_pDes[i+24] = 0x00;	// bAspectRatioY
				m_pDes[i+25] = 0x00;	// bmInterlaceFlags
				m_pDes[i+26] = 0x00;	// bCopyProtect
				// Video Streaming Uncompressed Format Type Descriptor (YUY2) -----
				break;
			case 1:
				if (iSpeedType == HIGH_SPEED)
					bDefaultFrameIndex = m_pParam[0x001F];
				else
					bDefaultFrameIndex = m_pParam[0x0021];

				// Video Streaming MJPEG Format Type Descriptor +++++
				m_pDes[i]    = 0x0B;	// bLength
				m_pDes[i+1]  = 0x24;	// bDescriptorType
				m_pDes[i+2]  = 0x06;	// bDescriptorSubtype
				m_pDes[i+3]  = j + 1;	// bFormatIndex
				m_pDes[i+4]  = bNumFrameDescriptors;	// bNumFrameDescriptors
				m_pDes[i+5]  = 0x00;	// bmFlags
				m_pDes[i+6]  = bDefaultFrameIndex;	// bDefaultFrameIndex
				m_pDes[i+7]  = 0x00;	// bAspectRatioX
				m_pDes[i+8]  = 0x00;	// bAspectRatioY
				m_pDes[i+9]  = 0x00;	// bmInterlaceFlags
				m_pDes[i+10] = 0x00;	// bCopyProtect
				// Video Streaming MJPEG Format Type Descriptor -----
				break;
			case 2:
				if (iSpeedType == HIGH_SPEED)
					bDefaultFrameIndex = m_pParam[0x001E];
				else
					bDefaultFrameIndex = m_pParam[0x0020];

				// Video Streaming Uncompressed Format Type Descriptor (RAW) +++++
				m_pDes[i]    = 0x1B;	// bLength
				m_pDes[i+1]  = 0x24;	// bDescriptorType
				m_pDes[i+2]  = 0x04;	// bDescriptorSubtype
				m_pDes[i+3]  = j+1;	// bFormatIndex
				m_pDes[i+4]  = bNumFrameDescriptors;	// bNumFrameDescriptors
				
				memcpy(m_pDes+i+5, &RAWGUID, sizeof(GUID));
				m_pDes[i+21] = 0x10;	// bBitsPerPixel
				m_pDes[i+22] = bDefaultFrameIndex;	// bDefaultFrameIndex
				m_pDes[i+23] = 0x00;	// bAspectRatioX
				m_pDes[i+24] = 0x00;	// bAspectRatioY
				m_pDes[i+25] = 0x00;	// bmInterlaceFlags
				m_pDes[i+26] = 0x00;	// bCopyProtect
				// Video Streaming Uncompressed Format Type Descriptor (RAW) -----
				break;
			}

			iTmpLen += m_pDes[i];
			iTotalLen += m_pDes[i];
			i += m_pDes[i];

			for (l = 0; l < m_abyNumOfResFmtVideo[iSpeedType][j]; l++)
			{
				bFrameIntervalType = m_abyNumOfFRFmtVideo[iSpeedType][j][l];
				bLength = 26 + (4 * bFrameIntervalType);
				wWidth = aCfgData[m_aFRData_FmtVideo[iSpeedType][j][l][0].byCfgIdx - 1].wOutWinWid;
				wHeight = aCfgData[m_aFRData_FmtVideo[iSpeedType][j][l][0].byCfgIdx - 1].wOutWinHei;
				
				if (iSpeedType == HIGH_SPEED)
					dwMaxVideoFrameBufferSize = aCfgData[m_aFRData_FmtVideo[iSpeedType][j][l][0].byCfgIdx - 1].dwFRSzHS;
				else
					dwMaxVideoFrameBufferSize = aCfgData[m_aFRData_FmtVideo[iSpeedType][j][l][0].byCfgIdx - 1].dwFRSzFS;

				iTmp = 100000000 / ((m_aFRData_FmtVideo[iSpeedType][j][l][0].wFRInt)<<8);
				dwDefaultFrameInterval = (unsigned long)((float)10000000 / ((float)iTmp / (float)10));

				iTmp = 100000000 / ((m_aFRData_FmtVideo[iSpeedType][j][l][bFrameIntervalType-1].wFRInt)<<8);
				dwMinBitRate = (unsigned long)(((float)iTmp / (float)10) * (float)dwMaxVideoFrameBufferSize * (float)8);
				
				iTmp = 100000000 / ((m_aFRData_FmtVideo[iSpeedType][j][l][0].wFRInt)<<8);
				dwMaxBitRate = (unsigned long)(((float)iTmp / (float)10) * (float)dwMaxVideoFrameBufferSize * (float)8);
				
				// Video Streaming Frame Type Descriptor +++++
				m_pDes[i]    = bLength;	// bLength
				
				switch (k)
				{
				case 0:
					// YUY2 +++++
					m_pDes[i+1]  = 0x24;	// bDescriptorType
					m_pDes[i+2]  = 0x05;	// bDescriptorSubtype
					// YUY2 -----
					break;
				case 1:
					// MJPEG +++++
					m_pDes[i+1]  = 0x24;	// bDescriptorType
					m_pDes[i+2]  = 0x07;	// bDescriptorSubtype
					// MJPEG -----
					break;
				case 2:
					// RAW +++++
					m_pDes[i+1]  = 0x24;	// bDescriptorType
					m_pDes[i+2]  = 0x05;	// bDescriptorSubtype
					// RAW -----
					break;
				}

				m_pDes[i+3]  = l+1;		// bFrameIndex
				m_pDes[i+4]  = 0x00;		// bmCapabilities
				m_pDes[i+5]  = wWidth & 0xff;		// wWidth (L)
				m_pDes[i+6]  = wWidth >> 8;		// wWidth (H)
				m_pDes[i+7]  = wHeight & 0xff;	// wHeight (L)
				m_pDes[i+8]  = wHeight >> 8;		// wHeight (H)
				m_pDes[i+9]  = dwMinBitRate & 0xff;				// dwMinBitRate (0)
				m_pDes[i+10] = dwMinBitRate >> 8;					// dwMinBitRate (1)
				m_pDes[i+11] = dwMinBitRate >> 16;				// dwMinBitRate (2)
				m_pDes[i+12] = dwMinBitRate >> 24;				// dwMinBitRate (3)
				m_pDes[i+13] = dwMaxBitRate & 0xff;;				// dwMaxBitRate (0)
				m_pDes[i+14] = dwMaxBitRate >> 8;					// dwMaxBitRate (1)
				m_pDes[i+15] = dwMaxBitRate >> 16;				// dwMaxBitRate (2)
				m_pDes[i+16] = dwMaxBitRate >> 24;				// dwMaxBitRate (3)
				m_pDes[i+17] = dwMaxVideoFrameBufferSize & 0xff;	// dwMaxVideoFrameBufferSize (0)
				m_pDes[i+18] = dwMaxVideoFrameBufferSize >> 8;	// dwMaxVideoFrameBufferSize (1)
				m_pDes[i+19] = dwMaxVideoFrameBufferSize >> 16;	// dwMaxVideoFrameBufferSize (2)
				m_pDes[i+20] = dwMaxVideoFrameBufferSize >> 24;	// dwMaxVideoFrameBufferSize (3)
				m_pDes[i+21] = dwDefaultFrameInterval & 0xff;		// dwDefaultFrameInterval (0)
				m_pDes[i+22] = dwDefaultFrameInterval >> 8;		// dwDefaultFrameInterval (1)
				m_pDes[i+23] = dwDefaultFrameInterval >> 16;		// dwDefaultFrameInterval (2)
				m_pDes[i+24] = dwDefaultFrameInterval >> 24;		// dwDefaultFrameInterval (3)
				m_pDes[i+25] = bFrameIntervalType;				// bFrameIntervalType
				
				for (m = 0, n = 26; m < bFrameIntervalType; m++, n += 4)
				{
					iTmp = 100000000 / ((m_aFRData_FmtVideo[iSpeedType][j][l][m].wFRInt)<<8);
					dwFrameInterval = (unsigned long)((float)10000000 / ((float)iTmp / (float)10));

					m_pDes[i+n]   = dwFrameInterval & 0xff;	// dwFrameInterval[m] (0)
					m_pDes[i+n+1] = dwFrameInterval >> 8;		// dwFrameInterval[m] (1)
					m_pDes[i+n+2] = dwFrameInterval >> 16;	// dwFrameInterval[m] (2)
					m_pDes[i+n+3] = dwFrameInterval >> 24;	// dwFrameInterval[m] (3)
				}

				iTmpLen += m_pDes[i];
				iTotalLen += m_pDes[i];
				i += m_pDes[i];
				// Video Streaming Frame Type Descriptor -----
			}

			// Still Image Frame Type Descriptor +++++
			bNumImageSizePatterns = m_abyNumOfResFmtStill[iSpeedType][j];
			bLength = 6 + (4 * bNumImageSizePatterns);

			if (bStillCaptureMethod == 2)	// shawn 2009/10/02 fix crash bug
			{
				if (bNumImageSizePatterns)
				{
					m_pDes[i]    = bLength;				// bLength
					m_pDes[i+1]  = 0x24;	// bDescriptorType
					m_pDes[i+2]  = 0x03;	// bDescriptorSubtype
					m_pDes[i+3]  = 0x00;	// bEndpointAddress
					m_pDes[i+4]  = bNumImageSizePatterns;	// bNumImageSizePatterns
					
					for (m = 0, n = 5; m < bNumImageSizePatterns; m++, n += 4)
					{
						wWidth = aCfgData[(m_abyStillCfgIdx[iSpeedType][j][m])-1].wOutWinWid;
						wHeight = aCfgData[(m_abyStillCfgIdx[iSpeedType][j][m])-1].wOutWinHei;

						m_pDes[i+n]   = wWidth & 0xff;	// wWidth[m] (L)
						m_pDes[i+n+1]  = wWidth >> 8;		// wWidth[m] (H)
						m_pDes[i+n+2]  = wHeight & 0xff;	// wHeight[m] (L)
						m_pDes[i+n+3]  = wHeight >> 8;	// wHeight[m] (H)
					}

					m_pDes[i+bLength-1]  = 0x00;			//bNumCompressionPattern

					iTmpLen += m_pDes[i];
					iTotalLen += m_pDes[i];
					i += m_pDes[i];
				}
			}
			// Still Image Frame Type Descriptor -----
		}
		// Format, Rseolution and Frame Rate for Video and Still -----

		// Color Matching Descriptor +++++
		m_pDes[i]   = 0x06;	// bLength
		m_pDes[i+1] = 0x24;	// bDescriptorType
		m_pDes[i+2] = 0x0D;	// bDescriptorSubtype
		m_pDes[i+3] = 0x01;	// bColorPrimaries
		m_pDes[i+4] = 0x01;	// bTransferCharacteristics
		m_pDes[i+5] = 0x04;	// bMatrixCoefficients

		iTmpLen += m_pDes[i];
		iTotalLen += m_pDes[i];
		i += m_pDes[i];
		// Color Matching Descriptor -----

		// Video Class-Specific VS Video Input Header Descriptor Total Length +++++
		m_pDes[iTmpAddr] = iTmpLen & 0xFF;	// wTotalLength (L)
		m_pDes[iTmpAddr+1] = iTmpLen >> 8;	// wTotalLength (H)

		iTmpLen = 0;
		iTmpAddr = 0;
		// Video Class-Specific VS Video Input Header Descriptor Total Length -----

		if (iSpeedType == HIGH_SPEED)
		{
			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x01;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x80;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x00;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x02;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x00;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x01;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x03;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x20;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x03;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x04;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x20;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x0B;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x05;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x20;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x13;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x06;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x00;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x14;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----
		}
		else
		{
			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x01;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x80;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x00;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x02;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x00;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x01;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x03;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x00;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x02;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x04;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x58;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x02;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x05;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0x20;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x03;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----

			// Interface Descriptor +++++
			m_pDes[i]   = 0x09;	// bLength
			m_pDes[i+1] = 0x04;	// bDescriptorType
			m_pDes[i+2] = 0x01;	// bInterfaceNumber
			m_pDes[i+3] = 0x06;	// bAlternateSetting
			m_pDes[i+4] = 0x01;	// bNumEndpoints
			m_pDes[i+5] = 0x0E;	// bInterfaceClass
			m_pDes[i+6] = 0x02;	// bInterfaceSubClass
			m_pDes[i+7] = 0x00;	// bInterfaceProtocol
			m_pDes[i+8] = 0x00;	// iInterface

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Interface Descriptor -----

			// Endpoint Descriptor +++++
			m_pDes[i]   = 0x07;	// bLength
			m_pDes[i+1] = 0x05;	// bDescriptorType
			m_pDes[i+2] = 0x81;	// bEndpointAddress
			m_pDes[i+3] = 0x05;	// bmAttributes
			m_pDes[i+4] = 0xBC;	// wMaxPacketSize (L)
			m_pDes[i+5] = 0x03;	// wMaxPacketSize (H)
			m_pDes[i+6] = 0x01;	// bInterval

			iTotalLen += m_pDes[i];
			i += m_pDes[i];
			// Endpoint Descriptor -----
		}

		// Cofiguration Descriptor Total Length +++++
		m_pDes[iTotalAddr] = iTotalLen & 0xFF;	// wTotalLength (L)
		m_pDes[iTotalAddr+1] = iTotalLen >> 8;	// wTotalLength (H)
		// Cofiguration Descriptor Total Length -----
	}
}
