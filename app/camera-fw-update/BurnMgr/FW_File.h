#ifndef _FW_FILE_H
#define _FW_FILE_H

int	 get_file_len(const char *szFile);
bool read_file_rom(const char *szFile, unsigned long ulBypass, 
						BYTE *pBuf, int nBufLen, int *pnOutLen);

bool read_file_des(const char *szFile, BYTE *pBuf, int nBufLen, int *pnOutLen);

bool read_file_param(const char *szFile, BYTE *pBuf, int nBufLen, int *pnOutLen);

bool read_file_ISP(const char *szFile, BYTE *pBuf, int nBufLen, int *pnOutLen, int nRomID);	// shawn 2009/11/03 modify for 232 and 275

// shawn 2009/08/14 for 233
bool read_file_64k(const char *szFile, BYTE *pBuf, int nBufLen);

// shawn 2009/11/02 for 232 and 275 +++++
#define HEADERVER		0x0100	// shawn 2009/06/23 for 275
#define TABLEFIXLEN	38
#define SPEEDNUM		2
#define FMTNUM			3
#define RESNUM			256
#define FRNUM				256
#define FMTINFONUM	12
#define CFGNUM			256
#define ASICSETNUM	7
#define ASICNUM			100
#define STRVDO232		37
#define STOPVDO232	22
#define STRVDO275		50
#define STOPVDO275	24
#define STRVDO276OLD 52		// shawn 2010/09/20 add
#define STRVDO276	 22		// shawn 2010/09/13 modify
#define STOPVDO276	25
#define SENPOWERLEN	1024
#define HEADTAILLEN	8192
#define XUNUM				10		// shawn 2009/09/22 for XU
#define CTRLNUM			10		// shawn 2009/09/22 for XU
#define INITNUM			256
#define INITLEN			8192
#define FRLEN				1024
#define CFGLENV1		41
#define CFGLENV2		64		// shawn 2009/06/23 for 275
#define CFGRESLEN		22		// shawn 2009/06/23 for 275
#define IQNUM				256		// shawn 2009/06/25 for 275
#define IQLEN				8192	// shawn 2009/06/25 for 275
#define TESTNUM			256		// shawn 2010/05/31 add
#define TESTLEN			8192	// shawn 2010/05/31 add

typedef struct
{
	unsigned short wFRInt;
	BYTE byCfgIdx;
} FRData;

typedef struct
{
	BYTE byFmtIdx;
	BYTE byResIdx;
	unsigned short wFRInt;

	unsigned short wOutWinWid;
	unsigned short wOutWinHei;
	unsigned short wOutWinHStr;
	unsigned short wOutWinVStr;

	unsigned short wInWinWid;
	unsigned short wInWinHei;
	unsigned short wInWinHStr;
	unsigned short wInWinVStr;

	BYTE byInitTableIdx;
	BYTE byFRTableIdx;
	BYTE bySCL;
	unsigned long dwPCK;
	unsigned short wPCKPerLine;
	unsigned short wPayLoadSzHS;
	unsigned short wPayLoadSzFS;
	unsigned long dwFRSzHS;
	unsigned long dwFRSzFS;

	// shawn 2009/06/23 for 275 +++++
	BYTE byIQTableIdx;
	BYTE abyReserved[CFGRESLEN];
	// shawn 2009/06/23 for 275 -----
} ConfigData;

typedef struct {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;

enum{
	HIGH_SPEED,
	FULL_SPEED
};

static const GUID XU1GUID = 
{0x28F03370, 0x6311, 0x4A2E, {0xBA, 0x2C, 0x68, 0x90, 0xEB, 0x33, 0x40, 0x16}};

static const GUID XU2GUID = 
{0x2812AE3F, 0xBCD7, 0x4E11, {0xA3, 0x57, 0x6F, 0x1E, 0xDE, 0xF7, 0xD6, 0x1D}};

static const GUID YUY2GUID = 
{0x32595559, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

static const GUID RAWGUID = 
{0x959F33AC, 0x1DBE, 0x4849, {0x8F, 0xB8, 0x58, 0x3F, 0x78, 0x1A, 0x30, 0xA8}};

bool LoadDataAction(int m_iRomVer, 
										BYTE *pReadTmp, 
										BYTE *pReadTmp2, 
										bool bIsFromUnproccessedFile, 
										bool bIsCheckProc,
										BYTE m_abyNumOfFmt[SPEEDNUM],
										BYTE *byActualXUNum,
										GUID aGUIDXU[XUNUM],
										unsigned short awXUCTRL[XUNUM],
										BYTE m_abyNumOfResFmtStill[SPEEDNUM][FMTNUM],
										BYTE m_abyNumOfResFmtVideo[SPEEDNUM][FMTNUM],
										BYTE m_abyFmtIdx[SPEEDNUM][FMTNUM],
										BYTE m_abyNumOfFRFmtVideo[SPEEDNUM][FMTNUM][RESNUM],
										BYTE m_abyStillCfgIdx[SPEEDNUM][FMTNUM][RESNUM],
										ConfigData aCfgData[CFGNUM],
										FRData m_aFRData_FmtVideo[SPEEDNUM][FMTNUM][RESNUM][FRNUM]);
										
bool GetSectionPos(BYTE *pReadTmp, unsigned long dwStartPos, unsigned long &dwSectionPos, bool bDontParse);	// shawn 2010/09/13 modify
// shawn 2009/11/02 for 232 and 275 -----

#endif
