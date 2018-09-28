#ifndef _BURNMGR_H
#define _BURNMGR_H

#include "..//common//usb.h"
#include "..//common//my_type.h"
#include "..//BurnerApLib//BurnerApLib.h"

// flag of source from file/flash/edit
#define SOURCE_FILE		0
#define SOURCE_FLASH	1
#define SOURCE_EDIT		2

// shawn 2009/11/02 for 232 and 275 +++++
enum{
	ROM_BUF,
	TABLE_BUF,
	PARAM_BUF,
	DES_BUF
};
// shawn 2009/11/02 for 232 and 275 -----

class CBurnMgr
{
public:
	void 	burn_init_trace(void);
	
	CBurnMgr();
	~CBurnMgr();
	
	bool	Set_XferMode(int nXferMode);
	bool 	Cam_Select(struct usb_device* pCam, int nFileNum);	// shawn 2009/11/06 modify
	void 	Cam_DeSelect(void);

	bool 	Get_CodeVersion(char szFlashCodeVer[32]);
    bool    Get_VendorVersion(char szFlashVendorVer[16]);
	bool	Get_RomVersion(char szRomVer[8]);
	bool	Get_FWVersion(char szFwVer[10]);

	char*	Get_RomFile();
	char*	Get_ParamFile();
	char*	Get_ISPFile();
	
	bool 	Set_Source_ROM(BYTE flag, char szFile[256]);
	bool 	Set_Source_Des(BYTE flag, char szFile[256]);
	bool 	Set_Source_Param(BYTE flag, char szFile[256]);
	bool 	Set_Source_ISP(BYTE flag, char szFile[256]);
	bool	Set_Source_File_From_INI(char file[256]);

	bool	Set_Target_Flash(bool bSet);
	bool	Set_Target_File(bool bSet);

	bool	Set_Save_FileName(char szFileName[256]);
	bool	Set_Save_All(bool bSet);
	bool	Set_Save_ROM(bool bSet);
	bool	Set_Save_Des(bool bSet);
	bool	Set_Save_Param(bool bSet);
	bool	Set_Save_ISP(bool bSet);

	bool	Set_Reserve_Serial(bool bSet);
	bool	Set_Reserve_Manufacturer(bool bSet);
	bool	Set_Defect_Pos(bool bSet);
	bool	Set_Custom_Space(bool bSet);

	bool	Load_Source_Data(void);

	bool	Burn_Init(void);
	bool	Burn_Erase(void);
	bool	Burn_Check(void);
	bool	Burn_Program(void);
	bool	Burn_Verify(void);
	bool	Burn_EndProc(bool bStatus);

	bool	Burn_Save_All(void);
	bool	Burn_Save_ROM(void);
	bool	Burn_Save_Des(void);
	bool	Burn_Save_Param(void);
	bool	Burn_Save_ISP(void);

	bool	Flash_Erase_MXIC(void);
	bool	Flash_Erase_SST(void);
	bool	Flash_Erase_ST(void);
	bool	Flash_Read_FW_Img(BYTE buf[64*1024]);
	bool	Flash_Erase_Verify(void);
	
	// shawn 2009/11/02 for 232 and 275 +++++
	bool DoCRC16(unsigned short *CRC16, int start, int end, int Type);
	void GenerateDesData();
	// shawn 2009/11/02 for 232 and 275 -----
	
private:	
	int						m_nXferMode;
	struct usb_device*	m_pCam;
	usb_dev_handle 		*m_udev;	
	char						m_szRomVer[LEN_ROM_VER];
	ENUM_ROM_ID				m_nRomID;
	BYTE						m_SFType;
	char						m_szFlashCodeVer[LEN_CODE_VER];
	unsigned long			m_nBypassLength;
	bool						m_bIsNewSensorTable;
	
	// burning option flag
	bool			m_bTarget_Flash;
	bool			m_bTarget_File;
	bool			m_bSave_All;		// burn to file - all
	bool			m_bSave_ROM;		// burn to file - ROM
	bool			m_bSave_Des;		// burn to file - Des
	bool			m_bSave_Param;		// burn to file - paramter
	bool			m_bSave_ISP;		// burn to file - ISP
	char			m_szTargetFile[256];	// burn to file - major file name, including path
		
	bool			m_bReserveSerial;
	bool			m_bReserveManufacturer;
	bool			m_bDefectPos;
	bool			m_bCustomSpace;
	
	bool			m_bFlag_Erase;
	bool			m_bFlag_Check;
	bool			m_bFlag_Prog;
	bool			m_bFlag_Verify;
	
	// f/w data buf
	unsigned int	m_nRomLen;
	BYTE				*m_pRom;
	char				m_szRomFile[256];
	BYTE				m_bSource_ROM;
	
	unsigned int	m_nDesLen;
	BYTE				*m_pDes;
	char				m_szDesFile[256];
	BYTE				m_bSource_Des;
	
	unsigned int	m_nParamLen;
 	BYTE				*m_pParam;
 	char				m_szParamFile[256];
 	BYTE				m_bSource_Param;
 	unsigned int	m_DrvParamAddr;	// driver parameter address
												// if 213B => 0xE000
 												// other   => 0x8400 	
 									
 	// Init Sensor Param
 	unsigned int	m_nISPLen;
	BYTE				*m_pISP;
	char				m_szISPFile[256];
	BYTE				m_bSource_ISP;			
	
	unsigned int	m_nFWDevLen;
	BYTE				*m_pFWDev;
	
	BYTE				*m_pDefectPos;
	unsigned int	m_nDefectPosLen;
	
	BYTE				*m_pCustomSpace;
	unsigned int	m_nCustomSpaceLen;
	
	// shawn 2009/11/02 for 232 and 275 +++++
	unsigned int	m_nCRC16Len;
 	BYTE					*m_pCRC16;
	// shawn 2009/11/02 for 232 and 275 -----
	
	int m_nInitSensorParamAddr;	// shawn 2009/11/04 for 233
	
	bool m_bSetTo64k;						// shawn 2009/11/06 add
	bool m_bSetTo128k; // carol
	
	// for set_write_protect use	
	BYTE				m_bWriteProtect;
	unsigned short	m_WPAddr;	
	
	bool	read_flash_rom(BYTE *pBuf, int nLen);
	bool	read_flash_des(BYTE *pBuf, int nLen);
	bool	read_flash_param(BYTE *pBuf, unsigned int DrvParamAddr);
	bool	read_flash_ISP(BYTE *pBuf, int nLen);
	bool	raise_cpu_rate(void);
	bool	down_cpu_rate(void);
	bool	param_str_err_handling(BYTE *pParam, const char szRomVer[LEN_ROM_VER]);
};

#endif // _BURNMGR_H
