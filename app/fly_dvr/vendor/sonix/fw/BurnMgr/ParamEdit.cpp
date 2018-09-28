#include <string.h>
#include "ParamEdit.h"
#include "..//common//debug.h"
#include "..//common//usb.h"
#include "..//BurnerApLib//BurnerApLib.h"
#include "..//common//misc.h"

#define	OFFSET_VIDEO_INFO			0x200
#define 	OFFSET_VIDEO_PARAM_1		0x223
#define 	OFFSET_VIDEO_PARAM_2		0x2CA
#define 	OFFSET_VIDEO_PARAM_3		0x400
#define 	OFFSET_STRING_1			0x40
#define 	OFFSET_STRING_2  			0x80
#define 	OFFSET_STRING_3  			0xc0
#define 	OFFSET_STRING_4  			0x100
#define 	OFFSET_STRING_5  			0x140


CParamEdit::CParamEdit()
{
	m_nParamLen	= 0x0800;
	m_pParam 	= (BYTE *)malloc(m_nParamLen);	
}

CParamEdit::~CParamEdit()
{
	free(m_pParam);
	m_pParam = NULL;
	m_nParamLen	= 0;
}

bool CParamEdit::Set_To_Param(BYTE *pInBuf)
{
	if (pInBuf == NULL)
		return false;
		
	memcpy(m_pParam, pInBuf, m_nParamLen);
	return true;
}

bool CParamEdit::Get_From_Param(BYTE *pOutBuf)
{
	if (pOutBuf == NULL)
		return false;
		
	memcpy(pOutBuf, m_pParam, m_nParamLen);
	return true;
}

bool CParamEdit::is_old_version(void)
{
	BYTE	RomValue[8];
	
	if (m_pParam[0] == 0xff && m_pParam[1] == 0xff)
		sf_read(0x9FF8, RomValue, sizeof(RomValue));

	if ((m_pParam[0] == 0x32 && m_pParam[1] == 0x32) || (_memicmp(RomValue, "22", 2) == 0))
		return true;
		
	return false;
}

// convert normal string to firmware string 
bool CParamEdit::str_normal_to_fw(const BYTE *inbuf, int str_len, BYTE *outbuf)
{
	int	i, j;

	TRACE("str_normal_to_fw : str_len = %d\n", str_len);
	for (i=0; i<str_len; i++)
		TRACE("str_normal_to_fw : inbuf[%d] = 0x%.2x\n", i, inbuf[i]);

	if (str_len == 0)
		return true;
	
	outbuf[0]	= str_len * 2 + 2;
	outbuf[1]	= 0x03;
	for (i=2, j=0; j<str_len; i+=2, j++)
	{
		outbuf[i] 	= inbuf[j];
		outbuf[i+1]	= 0x00;
	}
	return true;
}

// convert firmware string to normal string 
bool CParamEdit::str_fw_to_normal(const BYTE *inbuf, BYTE *outbuf, int *pStrLen)
{
	int	i, j;
	// james 2007/09/19 for string error handling
	// james 2007/11/07 to avoid string length==0x40 (mask rom issue)
	if (inbuf[0] < 0x02 || inbuf[0] > 0x3E /*0x40*/ || inbuf[1] != 0x03)
		return false;
	for (i=2, j=0; i<inbuf[0]; i+=2, j++)
		outbuf[j] = inbuf[i];
	*pStrLen = j;
	return true;
}
	
bool CParamEdit::Get_DevCap(S_DEV_CAP *pDevCap)
{
	bool	bRet;
	
	bRet = get_devcap(pDevCap);
	return bRet;
}

bool CParamEdit::get_devcap(S_DEV_CAP *pDevCap)
{
	BYTE	buf[200];
	int	i;
	
	for (i=0; i<53; i++)
		buf[i] = m_pParam[i];
		
	pDevCap->bType[2]				= buf[0];
	pDevCap->bType[1]				= buf[1];
	pDevCap->bType[0]				= buf[2];
	pDevCap->bSize					= buf[3];
	pDevCap->bcdVersion			= buf[4];
	pDevCap->bAudioMode			= buf[5];
	pDevCap->wVendorID			= ((buf[6]  << 8) + buf[7]);
	pDevCap->wProductID			= ((buf[8]  << 8) + buf[9]);
	pDevCap->bcdDevice			= ((buf[10] << 8) + buf[11]);
	pDevCap->bMaxPower			= buf[12];
	pDevCap->bKeyUsage			= buf[13];
	pDevCap->bLEDUsage			= buf[14];
	pDevCap->bWriteProtect		= buf[52];
	pDevCap->wObjFocalLenMin	= ((buf[15] << 8) + buf[16]);
	pDevCap->wObjFocalLenMax	= ((buf[17] << 8) + buf[18]);
	pDevCap->wOcularFocalLen	= ((buf[19] << 8) + buf[20]);
	pDevCap->bmCTControls[2]		= buf[21];
	pDevCap->bmCTControls[1]		= buf[22];
	pDevCap->bmCTControls[0]		= buf[23];
	pDevCap->wMaxMultiplier		= ((buf[24] << 8) + buf[25]);
	pDevCap->bmPUControls[2]		= buf[26];
	pDevCap->bmPUControls[1]		= buf[27];
	pDevCap->bmPUControls[0]		= buf[28];
	pDevCap->bVideoMode			= buf[29];
	pDevCap->bYUY2DefIdx			= buf[30];
	pDevCap->bMJPGDefIdx			= buf[31];
	pDevCap->bU1YUY2DefIdx		= buf[32];
	pDevCap->bU1MJPGDefIdx		= buf[33];	
	if (is_old_version())
	{
		pDevCap->bOldVersion = true;
		pDevCap->wString1Offset	= ((buf[34] << 8) + buf[35]);
		pDevCap->wString2Offset	= ((buf[36] << 8) + buf[37]);
		pDevCap->wString3Offset	= ((buf[38] << 8) + buf[39]);
		pDevCap->wString4Offset	= ((buf[40] << 8) + buf[41]);
		pDevCap->wVCParamOffset	= ((buf[42] << 8) + buf[43]);
	}
	else
	{
		pDevCap->bOldVersion = false;
		pDevCap->bUSBCtrl			= buf[34];
		pDevCap->bStringDisable	= buf[35];
		pDevCap->wRecVol[3]		= ((buf[36] << 8) + buf[37]);
		pDevCap->wRecVol[2]		= ((buf[38] << 8) + buf[39]);
		pDevCap->wRecVol[1]		= ((buf[40] << 8) + buf[41]);
		pDevCap->wRecVol[0]		= ((buf[42] << 8) + buf[43]);
		pDevCap->wPlayVol[3]		= ((buf[44] << 8) + buf[45]);
		pDevCap->wPlayVol[2]		= ((buf[46] << 8) + buf[47]);
		pDevCap->wPlayVol[1]		= ((buf[48] << 8) + buf[49]);
		pDevCap->wPlayVol[0]		= ((buf[50] << 8) + buf[51]);
	}

	return true;
}

bool CParamEdit::Get_Str_Setting(S_STRING_SETTING *pSS)
{
	bool	bRet;
	
	bRet = get_str_setting(pSS);
	return bRet;
}

bool CParamEdit::get_str_setting(S_STRING_SETTING *pSS)
{
	// string 1
	str_fw_to_normal(m_pParam + OFFSET_STRING_1, pSS->szString1, &(pSS->nStrLen1));
	// string 2
	str_fw_to_normal(m_pParam + OFFSET_STRING_2, pSS->szString2, &(pSS->nStrLen2));
	// string 3
	str_fw_to_normal(m_pParam + OFFSET_STRING_3, pSS->szString3, &(pSS->nStrLen3));
	// string 4
	str_fw_to_normal(m_pParam + OFFSET_STRING_4, pSS->szString4, &(pSS->nStrLen4));
	// string 5
	// james 2007/03/01
	pSS->bOldVersion = is_old_version();
	if (!pSS->bOldVersion)
		str_fw_to_normal(m_pParam + OFFSET_STRING_5, pSS->szString5, &(pSS->nStrLen5));
	
	return true;
}

bool CParamEdit::Get_VideoInfo(S_VIDEO_INFO *pVI)
{
	bool	bRet;
	
	bRet = get_video_info(pVI);
	return bRet;
}

bool CParamEdit::get_video_info(S_VIDEO_INFO *pVI)
{
	BYTE	buf[200];
	int	i;
	
	for (i=0; i<35; i++)
		buf[i] = m_pParam[i + OFFSET_VIDEO_INFO];
		
	pVI->bDevicePowerMode	= buf[0];
	pVI->bRequestErrorCode	= buf[1];
	pVI->bScanningMode		= buf[2];
	pVI->bAutoExpoMode		= buf[3];
	pVI->bAutoExpoPriority	= buf[4];
	pVI->bExpoTimeAbsolute	= buf[5];
	pVI->bExpoTimeRelative	= buf[6];
	pVI->bFocusAbsolute		= buf[7];
	pVI->bFocusRelative		= buf[8];
	pVI->bFocusAuto			= buf[9];
	pVI->bIrisAbsolute		= buf[10];
	pVI->bIrisRelative		= buf[11];
	pVI->bZoomAbsolute		= buf[12];
	pVI->bZoomRelative		= buf[13];
	pVI->bPanTiltAbsolute	= buf[14];
	pVI->bPanTiltRelative	= buf[15];
	pVI->bRollAbsolute		= buf[16];
	pVI->bRollRelative		= buf[17];
	pVI->bPrivacy				= buf[18];
	pVI->bBacklightComp		= buf[19];
	pVI->bBrightness			= buf[20];
	pVI->bContrast				= buf[21];
	pVI->bGain					= buf[22];
	pVI->bPowerLineFreq		= buf[23];
	pVI->bHue					= buf[24];
	pVI->bHueAuto				= buf[25];
	pVI->bSaturation			= buf[26];
	pVI->bSharpness			= buf[27];
	pVI->bGamma					= buf[28];
	pVI->bWBTemperature		= buf[29];
	pVI->bWBTemperatureAuto	= buf[30];
	pVI->bWBComponent			= buf[31];
	pVI->bWBComponentAuto	= buf[32];
	pVI->bDigitalMultiplier	= buf[33];
	pVI->bDigitalMultiplierLimit	= buf[34];
	
	return true;
}

bool CParamEdit::Get_VideoParam1(S_VIDEO_PARAM_1 *pVP1)
{
	bool	bRet;
	
	bRet = get_video_param1(pVP1);
	return bRet;
}

bool CParamEdit::get_video_param1(S_VIDEO_PARAM_1 *pVP1)
{
	BYTE	buf[200];
	int	i;
	
	for (i=0; i<167; i++)
		buf[i] = m_pParam[i + OFFSET_VIDEO_PARAM_1];
	
	pVP1->bDevicePowerMode	= buf[0];
	pVP1->bRequestErrorCode	= buf[1];
	pVP1->bScanningMode		= buf[2];
	pVP1->bAutoExpoMode[2]	= buf[3];
	pVP1->bAutoExpoMode[1]	= buf[4];
	pVP1->bAutoExpoMode[0]	= buf[5];
	pVP1->bAutoExpoPriority	= buf[6];
	pVP1->dwExpTimeAbsolute[4]	= ((buf[ 7] << 24) + (buf[ 8] << 16) + (buf[ 9] <<  8) + (buf[10]));
	pVP1->dwExpTimeAbsolute[3]	= ((buf[11] << 24) + (buf[12] << 16) + (buf[13] <<  8) + (buf[14]));
	pVP1->dwExpTimeAbsolute[2]	= ((buf[15] << 24) + (buf[16] << 16) + (buf[17] <<  8) + (buf[18]));
	pVP1->dwExpTimeAbsolute[1]	= ((buf[19] << 24) + (buf[20] << 16) + (buf[21] <<  8) + (buf[22]));
	pVP1->dwExpTimeAbsolute[0]	= ((buf[23] << 24) + (buf[24] << 16) + (buf[25] <<  8) + (buf[26]));
	pVP1->bExpTimeRelative	= buf[27];
	pVP1->wFocusAbsolute[4]	= ((buf[28] << 8) + buf[29]);
	pVP1->wFocusAbsolute[3]	= ((buf[30] << 8) + buf[31]);
	pVP1->wFocusAbsolute[2]	= ((buf[32] << 8) + buf[33]);
	pVP1->wFocusAbsolute[1]	= ((buf[34] << 8) + buf[35]);
	pVP1->wFocusAbsolute[0]	= ((buf[36] << 8) + buf[37]);
	pVP1->bFocusRelative[9]	= buf[38];
	pVP1->bFocusRelative[8]	= buf[39];
	pVP1->bFocusRelative[7]	= buf[40];
	pVP1->bFocusRelative[6]	= buf[41];
	pVP1->bFocusRelative[5]	= buf[42];
	pVP1->bFocusRelative[4]	= buf[43];
	pVP1->bFocusRelative[3]	= buf[44];
	pVP1->bFocusRelative[2]	= buf[45];
	pVP1->bFocusRelative[1]	= buf[46];
	pVP1->bFocusRelative[0]	= buf[47];
	pVP1->bFocusAuto[1]		= buf[48];
	pVP1->bFocusAuto[0]		= buf[49];
	pVP1->wIrisAbsolute[4]	= ((buf[50] << 8) + buf[51]);
	pVP1->wIrisAbsolute[3]	= ((buf[52] << 8) + buf[53]);
	pVP1->wIrisAbsolute[2]	= ((buf[54] << 8) + buf[55]);
	pVP1->wIrisAbsolute[1]	= ((buf[56] << 8) + buf[57]);
	pVP1->wIrisAbsolute[0]	= ((buf[58] << 8) + buf[59]);
	pVP1->bIrisRelative		= buf[60];
	pVP1->wObjFocalLength[4] = ((buf[61] << 8) + buf[62]);
	pVP1->wObjFocalLength[3] = ((buf[63] << 8) + buf[64]);
	pVP1->wObjFocalLength[2] = ((buf[65] << 8) + buf[66]);
	pVP1->wObjFocalLength[1] = ((buf[67] << 8) + buf[68]);
	pVP1->wObjFocalLength[0] = ((buf[69] << 8) + buf[70]);
	pVP1->bZoomRelative[14]	= buf[71];
	pVP1->bZoomRelative[13]	= buf[72];
	pVP1->bZoomRelative[12]	= buf[73];
	pVP1->bZoomRelative[11]	= buf[74];
	pVP1->bZoomRelative[10]	= buf[75];
	pVP1->bZoomRelative[9]	= buf[76];
	pVP1->bZoomRelative[8]	= buf[77];
	pVP1->bZoomRelative[7]	= buf[78];
	pVP1->bZoomRelative[6]	= buf[79];
	pVP1->bZoomRelative[5]	= buf[80];
	pVP1->bZoomRelative[4]	= buf[81];
	pVP1->bZoomRelative[3]	= buf[82];
	pVP1->bZoomRelative[2]	= buf[83];
	pVP1->bZoomRelative[1]	= buf[84];
	pVP1->bZoomRelative[0]	= buf[85];	
	pVP1->dwPanTiltAbsolute[9]	= ((buf[86] << 24) + (buf[87] << 16) + (buf[88] << 8) + (buf[89]));
	pVP1->dwPanTiltAbsolute[8]	= ((buf[90] << 24) + (buf[91] << 16) + (buf[92] << 8) + (buf[93]));
	pVP1->dwPanTiltAbsolute[7]	= ((buf[94] << 24) + (buf[95] << 16) + (buf[96] << 8) + (buf[97]));
	pVP1->dwPanTiltAbsolute[6]	= ((buf[98] << 24) + (buf[99] << 16) + (buf[100] << 8) + (buf[101]));
	pVP1->dwPanTiltAbsolute[5]	= ((buf[102] << 24) + (buf[103] << 16) + (buf[104] << 8) + (buf[105]));									       
	pVP1->dwPanTiltAbsolute[4]	= ((buf[106] << 24) + (buf[107] << 16) + (buf[108] << 8) + (buf[109]));
	pVP1->dwPanTiltAbsolute[3]	= ((buf[110] << 24) + (buf[111] << 16) + (buf[112] << 8) + (buf[113]));
	pVP1->dwPanTiltAbsolute[2]	= ((buf[114] << 24) + (buf[115] << 16) + (buf[116] << 8) + (buf[117]));
	pVP1->dwPanTiltAbsolute[1]	= ((buf[118] << 24) + (buf[119] << 16) + (buf[120] << 8) + (buf[121]));									      									       
	pVP1->dwPanTiltAbsolute[0]	= ((buf[122] << 24) + (buf[123] << 16) + (buf[124] << 8) + (buf[125]));
	pVP1->bPanTiltRelative[19]	= buf[126];
	pVP1->bPanTiltRelative[18]	= buf[127];
	pVP1->bPanTiltRelative[17]	= buf[128];
	pVP1->bPanTiltRelative[16]	= buf[129];
	pVP1->bPanTiltRelative[15]	= buf[130];
	pVP1->bPanTiltRelative[14]	= buf[131];
	pVP1->bPanTiltRelative[13]	= buf[132];
	pVP1->bPanTiltRelative[12]	= buf[133];
	pVP1->bPanTiltRelative[11]	= buf[134];
	pVP1->bPanTiltRelative[10]	= buf[135];
	pVP1->bPanTiltRelative[9]	= buf[136];
	pVP1->bPanTiltRelative[8]	= buf[137];
	pVP1->bPanTiltRelative[7]	= buf[138];
	pVP1->bPanTiltRelative[6]	= buf[139];
	pVP1->bPanTiltRelative[5]	= buf[140];
	pVP1->bPanTiltRelative[4]	= buf[141];
	pVP1->bPanTiltRelative[3]	= buf[142];
	pVP1->bPanTiltRelative[2]	= buf[143];
	pVP1->bPanTiltRelative[1]	= buf[144];
	pVP1->bPanTiltRelative[0]	= buf[145];
	pVP1->wRollAbsolute[4]		= ((buf[146] << 8) + buf[147]);
	pVP1->wRollAbsolute[3]		= ((buf[148] << 8) + buf[149]);
	pVP1->wRollAbsolute[2]		= ((buf[150] << 8) + buf[151]);
	pVP1->wRollAbsolute[1]		= ((buf[152] << 8) + buf[153]);
	pVP1->wRollAbsolute[0]		= ((buf[154] << 8) + buf[155]);
	pVP1->bRollRelative[9]		= buf[156];
	pVP1->bRollRelative[8]		= buf[157];
	pVP1->bRollRelative[7]		= buf[158];
	pVP1->bRollRelative[6]		= buf[159];
	pVP1->bRollRelative[5]		= buf[160];
	pVP1->bRollRelative[4]		= buf[161];
	pVP1->bRollRelative[3]		= buf[162];
	pVP1->bRollRelative[2]		= buf[163];
	pVP1->bRollRelative[1]		= buf[164];
	pVP1->bRollRelative[0]		= buf[165];
	pVP1->bPrivacy					= buf[166];
									       
	return true;
}

bool CParamEdit::Get_VideoParam2(S_VIDEO_PARAM_2 *pVP2)
{
	bool	bRet;
	
	bRet = get_video_param2(pVP2);
	return bRet;
}

bool CParamEdit::get_video_param2(S_VIDEO_PARAM_2 *pVP2)
{
	BYTE	buf[200];
	int	i;
	
	/*
		OFFSET_VIDEO_PARAM_2 - 21 = 0x2b5. 
		The result is the same.
		I had asked James and get the reply "he had forgot why use 0x2b5
		and begin with the 21 byte".
	*/
	for (i=0; i<159; i++)
		buf[i] = m_pParam[i + (OFFSET_VIDEO_PARAM_2 - 21)];
	
	pVP2->wBacklightCompensation[4] = ((buf[21] << 8) + buf[22]);
	pVP2->wBacklightCompensation[3] = ((buf[23] << 8) + buf[24]);
	pVP2->wBacklightCompensation[2] = ((buf[25] << 8) + buf[26]);
	pVP2->wBacklightCompensation[1] = ((buf[27] << 8) + buf[28]);
	pVP2->wBacklightCompensation[0] = ((buf[29] << 8) + buf[30]);
	pVP2->wBrightness[4] 		= ((buf[31] << 8) + buf[32]);
	pVP2->wBrightness[3] 		= ((buf[33] << 8) + buf[34]);
	pVP2->wBrightness[2] 		= ((buf[35] << 8) + buf[36]);
	pVP2->wBrightness[1] 		= ((buf[37] << 8) + buf[38]);
	pVP2->wBrightness[0] 		= ((buf[39] << 8) + buf[40]);
	pVP2->wContrast[4] 		= ((buf[41] << 8) + buf[42]);
	pVP2->wContrast[3] 		= ((buf[43] << 8) + buf[44]);
	pVP2->wContrast[2] 		= ((buf[45] << 8) + buf[46]);
	pVP2->wContrast[2] 		= ((buf[47] << 8) + buf[48]);
	pVP2->wContrast[0] 		= ((buf[49] << 8) + buf[50]);
	pVP2->wGain[4]				= ((buf[51] << 8) + buf[52]);
	pVP2->wGain[3]				= ((buf[53] << 8) + buf[54]);
	pVP2->wGain[2]				= ((buf[55] << 8) + buf[56]);
	pVP2->wGain[1]				= ((buf[57] << 8) + buf[58]);
	pVP2->wGain[0]				= ((buf[59] << 8) + buf[60]);
	pVP2->bPowerLineFreq[1] 	= buf[61];
	pVP2->bPowerLineFreq[0] 	= buf[62];
	pVP2->wHue[4]				= ((buf[63] << 8) + buf[64]);
	pVP2->wHue[3]				= ((buf[65] << 8) + buf[66]);
	pVP2->wHue[2]				= ((buf[67] << 8) + buf[68]);
	pVP2->wHue[1]				= ((buf[69] << 8) + buf[70]);
	pVP2->wHue[0]				= ((buf[71] << 8) + buf[72]);
	pVP2->bHueAuto[1]			= buf[73];
	pVP2->bHueAuto[0]			= buf[74];
	pVP2->wSaturation[4]		= ((buf[75] << 8) + buf[76]);
	pVP2->wSaturation[3]		= ((buf[77] << 8) + buf[78]);
	pVP2->wSaturation[2]		= ((buf[79] << 8) + buf[80]);
	pVP2->wSaturation[1]		= ((buf[81] << 8) + buf[82]);
	pVP2->wSaturation[0]		= ((buf[83] << 8) + buf[84]);
	pVP2->wSharpness[4]		= ((buf[85] << 8) + buf[86]);
	pVP2->wSharpness[3]		= ((buf[87] << 8) + buf[88]);
	pVP2->wSharpness[2]		= ((buf[89] << 8) + buf[90]);
	pVP2->wSharpness[1]		= ((buf[91] << 8) + buf[92]);
	pVP2->wSharpness[0]		= ((buf[93] << 8) + buf[94]);
	pVP2->wGamma[4]				= ((buf[95] << 8) + buf[96]);
	pVP2->wGamma[3]				= ((buf[97] << 8) + buf[98]);
	pVP2->wGamma[2]				= ((buf[99] << 8) + buf[100]);
	pVP2->wGamma[1]				= ((buf[101] << 8) + buf[102]);
	pVP2->wGamma[0]				= ((buf[103] << 8) + buf[104]);
	pVP2->wWhiteBalanceTemperature[4]	= ((buf[105] << 8) + buf[106]);
	pVP2->wWhiteBalanceTemperature[3]	= ((buf[107] << 8) + buf[108]);
	pVP2->wWhiteBalanceTemperature[2]	= ((buf[109] << 8) + buf[110]);
	pVP2->wWhiteBalanceTemperature[1]	= ((buf[111] << 8) + buf[112]);
	pVP2->wWhiteBalanceTemperature[0]	= ((buf[113] << 8) + buf[114]);
	pVP2->bWhiteBalanceTemperatureAuto[1] = buf[115];
	pVP2->bWhiteBalanceTemperatureAuto[0] = buf[116];
	pVP2->wWhiteBalanceComponent[9]	= ((buf[117] << 8) + buf[118]);
	pVP2->wWhiteBalanceComponent[8]	= ((buf[119] << 8) + buf[120]);
	pVP2->wWhiteBalanceComponent[7]	= ((buf[121] << 8) + buf[122]);
	pVP2->wWhiteBalanceComponent[6]	= ((buf[123] << 8) + buf[124]);
	pVP2->wWhiteBalanceComponent[5]	= ((buf[125] << 8) + buf[126]);
	pVP2->wWhiteBalanceComponent[4]	= ((buf[127] << 8) + buf[128]);
	pVP2->wWhiteBalanceComponent[3]	= ((buf[129] << 8) + buf[130]);
	pVP2->wWhiteBalanceComponent[2]	= ((buf[131] << 8) + buf[132]);
	pVP2->wWhiteBalanceComponent[1]	= ((buf[133] << 8) + buf[134]);
	pVP2->wWhiteBalanceComponent[0]	= ((buf[135] << 8) + buf[136]);
	pVP2->bWhiteBalanceComponentAuto[1] = buf[137];
	pVP2->bWhiteBalanceComponentAuto[0] = buf[138];
	pVP2->wMultiplierStep[4]		= ((buf[139] << 8) + buf[140]);
	pVP2->wMultiplierStep[3]		= ((buf[141] << 8) + buf[142]);
	pVP2->wMultiplierStep[2]		= ((buf[143] << 8) + buf[144]);
	pVP2->wMultiplierStep[1]		= ((buf[145] << 8) + buf[146]);
	pVP2->wMultiplierStep[0]		= ((buf[147] << 8) + buf[148]);	
	pVP2->wMultiplierLimit[4]	= ((buf[149] << 8) + buf[150]);	
	pVP2->wMultiplierLimit[3]	= ((buf[151] << 8) + buf[152]);	
	pVP2->wMultiplierLimit[2]	= ((buf[153] << 8) + buf[154]);	
	pVP2->wMultiplierLimit[1]	= ((buf[155] << 8) + buf[156]);	
	pVP2->wMultiplierLimit[0]	= ((buf[157] << 8) + buf[158]);	
	return true;	
}

bool CParamEdit::Get_VideoParam3(S_VIDEO_PARAM_3 *pVP3)
{
	bool	bRet;
	
	bRet = get_video_param3(pVP3);
	return bRet;
}

bool CParamEdit::get_video_param3(S_VIDEO_PARAM_3 *pVP3)
{
	BYTE	buf[200];
	
	memcpy(buf, m_pParam + OFFSET_VIDEO_PARAM_3, 25);
	pVP3->bAuxLedDis				= buf[0];
	pVP3->bAuxLedDef				= buf[1];
	pVP3->bAuxLedBright[3]		= buf[2];
	pVP3->bAuxLedBright[2]		= buf[3];
	pVP3->bAuxLedBright[1]		= buf[4];
	pVP3->bAuxLedBright[0]		= buf[5];
	pVP3->bPrivacyDis				= buf[6];
	pVP3->bPrivacyDef				= buf[7];
	pVP3->bWBComponentDis		= buf[8];
	pVP3->wWBBlueDef				= ((buf[9] << 8) + buf[10]);
	pVP3->wWBRedDef				= ((buf[11] << 8) + buf[12]);	
	pVP3->bWBBlueRes				= buf[13];
	pVP3->bWBRedRes				= buf[14];
	pVP3->wWBBlueMax				= ((buf[15] << 8) + buf[16]);
	pVP3->wWBRedMax				= ((buf[17] << 8) + buf[18]);
	pVP3->wWBBlueMin				= ((buf[19] << 8) + buf[20]);
	pVP3->wWBRedMin				= ((buf[21] << 8) + buf[22]);
	pVP3->bWBComponentAutoDis 	= buf[23];
	pVP3->bWBComponentAutoDef 	= buf[24];
	return true;
}

bool CParamEdit::Set_DevCap(const S_DEV_CAP &DevCap)
{
	bool 	bRet; 
	
	bRet = set_devcap(DevCap);
	return bRet;
}

bool CParamEdit::set_devcap(const S_DEV_CAP &DevCap)
{
	m_pParam[0] 		= DevCap.bType[2];
	m_pParam[1] 		= DevCap.bType[1];
	m_pParam[2] 		= DevCap.bType[0];
	m_pParam[3]		= DevCap.bSize;
	m_pParam[4]		= DevCap.bcdVersion;
	m_pParam[5]		= DevCap.bAudioMode;
	m_pParam[6]		= (BYTE)(DevCap.wVendorID >> 8);
	m_pParam[7]		= (BYTE)(DevCap.wVendorID & 0xff);
	m_pParam[8]		= (BYTE)(DevCap.wProductID >> 8);
	m_pParam[9]		= (BYTE)(DevCap.wProductID & 0xff);
	m_pParam[10]		= (BYTE)(DevCap.bcdDevice >> 8);
	m_pParam[11]		= (BYTE)(DevCap.bcdDevice & 0xff);
	m_pParam[12]		= DevCap.bMaxPower;		
	m_pParam[13]		= DevCap.bKeyUsage;
	m_pParam[14]		= DevCap.bLEDUsage;
	m_pParam[15]		= (BYTE)(DevCap.wObjFocalLenMin >> 8);
	m_pParam[16]		= (BYTE)(DevCap.wObjFocalLenMin & 0xff);
	m_pParam[17]		= (BYTE)(DevCap.wObjFocalLenMax >> 8);
	m_pParam[18]		= (BYTE)(DevCap.wObjFocalLenMax & 0xff);
	m_pParam[19]		= (BYTE)(DevCap.wOcularFocalLen >> 8);
	m_pParam[20]		= (BYTE)(DevCap.wOcularFocalLen & 0xff);
	m_pParam[21]		= DevCap.bmCTControls[2];
	m_pParam[22]		= DevCap.bmCTControls[1];
	m_pParam[23]		= DevCap.bmCTControls[0];
	m_pParam[24]		= (BYTE)(DevCap.wMaxMultiplier >> 8);
	m_pParam[25]		= (BYTE)(DevCap.wMaxMultiplier & 0xff);
	m_pParam[26]		= DevCap.bmPUControls[2];
	m_pParam[27]		= DevCap.bmPUControls[1];
	m_pParam[28]		= DevCap.bmPUControls[0];
	m_pParam[29]		= DevCap.bVideoMode;
	m_pParam[30]		= DevCap.bYUY2DefIdx;
	m_pParam[31]		= DevCap.bMJPGDefIdx;
	m_pParam[32]		= DevCap.bU1YUY2DefIdx;
	m_pParam[33]		= DevCap.bU1MJPGDefIdx;
	if (m_pParam[0] == 0x32 && m_pParam[0] == 0x32)
	{
		m_pParam[34]	 	= (BYTE)(DevCap.wString1Offset >> 8);
		m_pParam[35]		= (BYTE)(DevCap.wString1Offset & 0xff);
		m_pParam[36]	 	= (BYTE)(DevCap.wString2Offset >> 8);
		m_pParam[37]		= (BYTE)(DevCap.wString2Offset & 0xff);
		m_pParam[38]	 	= (BYTE)(DevCap.wString3Offset >> 8);
		m_pParam[39]		= (BYTE)(DevCap.wString3Offset & 0xff);
		m_pParam[40]	 	= (BYTE)(DevCap.wString4Offset >> 8);
		m_pParam[41]		= (BYTE)(DevCap.wString4Offset & 0xff);
		m_pParam[42]	 	= (BYTE)(DevCap.wVCParamOffset >> 8);
		m_pParam[43]		= (BYTE)(DevCap.wVCParamOffset & 0xff);
	}
	else
	{
		m_pParam[34]		= DevCap.bUSBCtrl;
		m_pParam[35]		= DevCap.bStringDisable;
		m_pParam[36]	 	= (BYTE)(DevCap.wRecVol[3] >> 8);
		m_pParam[37]		= (BYTE)(DevCap.wRecVol[3] & 0xff);
		m_pParam[38]	 	= (BYTE)(DevCap.wRecVol[2] >> 8);
		m_pParam[39]		= (BYTE)(DevCap.wRecVol[2] & 0xff);
		m_pParam[40]	 	= (BYTE)(DevCap.wRecVol[1] >> 8);
		m_pParam[41]		= (BYTE)(DevCap.wRecVol[1] & 0xff);
		m_pParam[42]	 	= (BYTE)(DevCap.wRecVol[0] >> 8);
		m_pParam[43]		= (BYTE)(DevCap.wRecVol[0] & 0xff);
		m_pParam[44]	 	= (BYTE)(DevCap.wPlayVol[3] >> 8);
		m_pParam[45]		= (BYTE)(DevCap.wPlayVol[3] & 0xff);
		m_pParam[46]	 	= (BYTE)(DevCap.wPlayVol[2] >> 8);
		m_pParam[47]		= (BYTE)(DevCap.wPlayVol[2] & 0xff);
		m_pParam[48]	 	= (BYTE)(DevCap.wPlayVol[1] >> 8);
		m_pParam[49]		= (BYTE)(DevCap.wPlayVol[1] & 0xff);
		m_pParam[50]	 	= (BYTE)(DevCap.wPlayVol[0] >> 8);
		m_pParam[51]		= (BYTE)(DevCap.wPlayVol[0] & 0xff);
	}
	return true;
}

bool CParamEdit::Set_Str_Setting(const S_STRING_SETTING &SS)
{
	bool	bRet;
	
	bRet = set_str_setting(SS);
	return bRet;	
}

bool CParamEdit::set_str_setting(const S_STRING_SETTING &SS)
{
	// string 1
	str_normal_to_fw(SS.szString1, SS.nStrLen1, m_pParam + OFFSET_STRING_1);
	// string 2
	str_normal_to_fw(SS.szString2, SS.nStrLen2, m_pParam + OFFSET_STRING_2);
	// string 3
	str_normal_to_fw(SS.szString3, SS.nStrLen3, m_pParam + OFFSET_STRING_3);
	// string 4
	str_normal_to_fw(SS.szString4, SS.nStrLen4, m_pParam + OFFSET_STRING_4);
	// string 5
	if (!SS.bOldVersion)
		str_normal_to_fw(SS.szString5, SS.nStrLen5, m_pParam + OFFSET_STRING_5);
	
	return true;
}

bool CParamEdit::Set_VideoInfo(const S_VIDEO_INFO &VI)
{
	bool	bRet;
	
	bRet = set_videoinfo(VI);
	return bRet;	
}

bool CParamEdit::set_videoinfo(const S_VIDEO_INFO &VI)
{
	int	offset = OFFSET_VIDEO_INFO;
	
	m_pParam[offset + 0]	= VI.bDevicePowerMode;
	m_pParam[offset + 1]	= VI.bRequestErrorCode;
	m_pParam[offset + 2]	= VI.bScanningMode;
	m_pParam[offset + 3]	= VI.bAutoExpoMode;
	m_pParam[offset + 4]	= VI.bAutoExpoPriority;
	m_pParam[offset + 5]	= VI.bExpoTimeAbsolute;
	m_pParam[offset + 6]	= VI.bExpoTimeRelative;
	m_pParam[offset + 7]	= VI.bFocusAbsolute;
	m_pParam[offset + 8]	= VI.bFocusRelative;
	m_pParam[offset + 9]	= VI.bFocusAuto;
	m_pParam[offset + 10]	= VI.bIrisAbsolute;
	m_pParam[offset + 11]	= VI.bIrisRelative;
	m_pParam[offset + 12]	= VI.bZoomAbsolute;
	m_pParam[offset + 13]	= VI.bZoomRelative;
	m_pParam[offset + 14]	= VI.bPanTiltAbsolute;
	m_pParam[offset + 15]	= VI.bPanTiltRelative;
	m_pParam[offset + 16]	= VI.bRollAbsolute;
	m_pParam[offset + 17]	= VI.bRollRelative;
	m_pParam[offset + 18]	= VI.bPrivacy;
	m_pParam[offset + 19]	= VI.bBacklightComp;
	m_pParam[offset + 20]	= VI.bBrightness;
	m_pParam[offset + 21]	= VI.bContrast;
	m_pParam[offset + 22]	= VI.bGain;
	m_pParam[offset + 23]	= VI.bPowerLineFreq;
	m_pParam[offset + 24]	= VI.bHue;
	m_pParam[offset + 25]	= VI.bHueAuto;
	m_pParam[offset + 26]	= VI.bSaturation;
	m_pParam[offset + 27]	= VI.bSharpness;
	m_pParam[offset + 28]	= VI.bGamma;
	m_pParam[offset + 29]	= VI.bWBTemperature;
	m_pParam[offset + 30]	= VI.bWBTemperatureAuto;
	m_pParam[offset + 31]	= VI.bWBComponent;
	m_pParam[offset + 32]	= VI.bWBComponentAuto;
	m_pParam[offset + 33]	= VI.bDigitalMultiplier;
	m_pParam[offset + 34]	= VI.bDigitalMultiplierLimit;
	
	return true;
}

bool CParamEdit::Set_VideoParam1(const S_VIDEO_PARAM_1 &VP1)
{
	bool 	bRet; 
	
	bRet = set_videoparam1(VP1);
	return bRet;
}

bool CParamEdit::set_videoparam1(const S_VIDEO_PARAM_1 &VP1)
{
	int	offset = OFFSET_VIDEO_PARAM_1;
	
	m_pParam[offset + 0]	= VP1.bDevicePowerMode;
	m_pParam[offset + 1]	= VP1.bRequestErrorCode;
	m_pParam[offset + 2]	= VP1.bScanningMode;
	m_pParam[offset + 3]	= VP1.bAutoExpoMode[2];
	m_pParam[offset + 4]	= VP1.bAutoExpoMode[1];
	m_pParam[offset + 5]	= VP1.bAutoExpoMode[0];
	m_pParam[offset + 6]	= VP1.bAutoExpoPriority;
	m_pParam[offset + 7]	= VP1.dwExpTimeAbsolute[4] >> 24;
	m_pParam[offset + 8]	= VP1.dwExpTimeAbsolute[4] >> 16;
	m_pParam[offset + 9]	= VP1.dwExpTimeAbsolute[4] >>  8;
	m_pParam[offset + 10]	= VP1.dwExpTimeAbsolute[4] & 0xff;
	m_pParam[offset + 11]	= VP1.dwExpTimeAbsolute[3] >> 24;
	m_pParam[offset + 12]	= VP1.dwExpTimeAbsolute[3] >> 16;
	m_pParam[offset + 13]	= VP1.dwExpTimeAbsolute[3] >>  8;
	m_pParam[offset + 14]	= VP1.dwExpTimeAbsolute[3] & 0xff;
	m_pParam[offset + 15]	= VP1.dwExpTimeAbsolute[2] >> 24;
	m_pParam[offset + 16]	= VP1.dwExpTimeAbsolute[2] >> 16;
	m_pParam[offset + 17]	= VP1.dwExpTimeAbsolute[2] >>  8;
	m_pParam[offset + 18]	= VP1.dwExpTimeAbsolute[2] & 0xff;
	m_pParam[offset + 19]	= VP1.dwExpTimeAbsolute[1] >> 24;
	m_pParam[offset + 20]	= VP1.dwExpTimeAbsolute[1] >> 16;
	m_pParam[offset + 21]	= VP1.dwExpTimeAbsolute[1] >>  8;
	m_pParam[offset + 22]	= VP1.dwExpTimeAbsolute[1] & 0xff;
	m_pParam[offset + 23]	= VP1.dwExpTimeAbsolute[0] >> 24;
	m_pParam[offset + 24]	= VP1.dwExpTimeAbsolute[0] >> 16;
	m_pParam[offset + 25]	= VP1.dwExpTimeAbsolute[0] >>  8;
	m_pParam[offset + 26]	= VP1.dwExpTimeAbsolute[0] & 0xff;
	m_pParam[offset + 27]	= VP1.bExpTimeRelative;
	m_pParam[offset + 28]	= VP1.wFocusAbsolute[4] >> 8;
	m_pParam[offset + 29]	= VP1.wFocusAbsolute[4] & 0xff;
	m_pParam[offset + 20]	= VP1.wFocusAbsolute[3] >> 8;
	m_pParam[offset + 31]	= VP1.wFocusAbsolute[3] & 0xff;
	m_pParam[offset + 32]	= VP1.wFocusAbsolute[2] >> 8;
	m_pParam[offset + 33]	= VP1.wFocusAbsolute[2] & 0xff;
	m_pParam[offset + 34]	= VP1.wFocusAbsolute[1] >> 8;
	m_pParam[offset + 35]	= VP1.wFocusAbsolute[1] & 0xff;
	m_pParam[offset + 36]	= VP1.wFocusAbsolute[0] >> 8;
	m_pParam[offset + 37]	= VP1.wFocusAbsolute[0] & 0xff;
	m_pParam[offset + 38]	= VP1.bFocusRelative[9];
	m_pParam[offset + 39]	= VP1.bFocusRelative[8];
	m_pParam[offset + 40]	= VP1.bFocusRelative[7];
	m_pParam[offset + 41]	= VP1.bFocusRelative[6];
	m_pParam[offset + 42]	= VP1.bFocusRelative[5];
	m_pParam[offset + 43]	= VP1.bFocusRelative[4];
	m_pParam[offset + 44]	= VP1.bFocusRelative[3];
	m_pParam[offset + 45]	= VP1.bFocusRelative[2];
	m_pParam[offset + 46]	= VP1.bFocusRelative[1];
	m_pParam[offset + 47]	= VP1.bFocusRelative[0];
	m_pParam[offset + 48]	= VP1.bFocusAuto[1];
	m_pParam[offset + 49]	= VP1.bFocusAuto[0];
	m_pParam[offset + 50]	= VP1.wIrisAbsolute[4] >> 8;
	m_pParam[offset + 51]	= VP1.wIrisAbsolute[4] & 0xff;
	m_pParam[offset + 52]	= VP1.wIrisAbsolute[3] >> 8;
	m_pParam[offset + 53]	= VP1.wIrisAbsolute[3] & 0xff;
	m_pParam[offset + 54]	= VP1.wIrisAbsolute[2] >> 8;
	m_pParam[offset + 55]	= VP1.wIrisAbsolute[2] & 0xff;
	m_pParam[offset + 56]	= VP1.wIrisAbsolute[1] >> 8;
	m_pParam[offset + 57]	= VP1.wIrisAbsolute[1] & 0xff;
	m_pParam[offset + 58]	= VP1.wIrisAbsolute[0] >> 8;
	m_pParam[offset + 59]	= VP1.wIrisAbsolute[0] & 0xff;
	m_pParam[offset + 60]	= VP1.bIrisRelative;
	m_pParam[offset + 61]	= VP1.wObjFocalLength[4] >> 8;
	m_pParam[offset + 62]	= VP1.wObjFocalLength[4] & 0xff;
	m_pParam[offset + 63]	= VP1.wObjFocalLength[3] >> 8;
	m_pParam[offset + 64]	= VP1.wObjFocalLength[3] & 0xff;
	m_pParam[offset + 65]	= VP1.wObjFocalLength[2] >> 8;
	m_pParam[offset + 66]	= VP1.wObjFocalLength[2] & 0xff;
	m_pParam[offset + 67]	= VP1.wObjFocalLength[1] >> 8;
	m_pParam[offset + 68]	= VP1.wObjFocalLength[1] & 0xff;
	m_pParam[offset + 69]	= VP1.wObjFocalLength[0] >> 8;
	m_pParam[offset + 70]	= VP1.wObjFocalLength[0] & 0xff;
	m_pParam[offset + 71]	= VP1.bZoomRelative[14];
	m_pParam[offset + 72]	= VP1.bZoomRelative[13];
	m_pParam[offset + 73]	= VP1.bZoomRelative[12];
	m_pParam[offset + 74]	= VP1.bZoomRelative[11];
	m_pParam[offset + 75]	= VP1.bZoomRelative[10];
	m_pParam[offset + 76]	= VP1.bZoomRelative[9];
	m_pParam[offset + 77]	= VP1.bZoomRelative[8];
	m_pParam[offset + 78]	= VP1.bZoomRelative[7];
	m_pParam[offset + 79]	= VP1.bZoomRelative[6];
	m_pParam[offset + 80]	= VP1.bZoomRelative[5];
	m_pParam[offset + 81]	= VP1.bZoomRelative[4];
	m_pParam[offset + 82]	= VP1.bZoomRelative[3];
	m_pParam[offset + 83]	= VP1.bZoomRelative[2];
	m_pParam[offset + 84]	= VP1.bZoomRelative[1];
	m_pParam[offset + 85]	= VP1.bZoomRelative[0];
	m_pParam[offset + 86]	= VP1.dwPanTiltAbsolute[9] >> 24;
	m_pParam[offset + 87]	= VP1.dwPanTiltAbsolute[9] >> 16;
	m_pParam[offset + 88]	= VP1.dwPanTiltAbsolute[9] >>  8;
	m_pParam[offset + 89]	= VP1.dwPanTiltAbsolute[9] & 0xff;
	m_pParam[offset + 90]	= VP1.dwPanTiltAbsolute[8] >> 24;
	m_pParam[offset + 91]	= VP1.dwPanTiltAbsolute[8] >> 16;
	m_pParam[offset + 92]	= VP1.dwPanTiltAbsolute[8] >>  8;
	m_pParam[offset + 93]	= VP1.dwPanTiltAbsolute[8] & 0xff;
	m_pParam[offset + 94]	= VP1.dwPanTiltAbsolute[7] >> 24;
	m_pParam[offset + 95]	= VP1.dwPanTiltAbsolute[7] >> 16;
	m_pParam[offset + 96]	= VP1.dwPanTiltAbsolute[7] >>  8;
	m_pParam[offset + 97]	= VP1.dwPanTiltAbsolute[7] & 0xff;
	m_pParam[offset + 98]	= VP1.dwPanTiltAbsolute[6] >> 24;
	m_pParam[offset + 99]	= VP1.dwPanTiltAbsolute[6] >> 16;
	m_pParam[offset + 100] = VP1.dwPanTiltAbsolute[6] >>  8;
	m_pParam[offset + 101] = VP1.dwPanTiltAbsolute[6] & 0xff;
	m_pParam[offset + 102] = VP1.dwPanTiltAbsolute[5] >> 24;
	m_pParam[offset + 103] = VP1.dwPanTiltAbsolute[5] >> 16;
	m_pParam[offset + 104] = VP1.dwPanTiltAbsolute[5] >>  8;
	m_pParam[offset + 105] = VP1.dwPanTiltAbsolute[5] & 0xff;
	m_pParam[offset + 106] = VP1.dwPanTiltAbsolute[4] >> 24;
	m_pParam[offset + 107] = VP1.dwPanTiltAbsolute[4] >> 16;
	m_pParam[offset + 108] = VP1.dwPanTiltAbsolute[4] >>  8;
	m_pParam[offset + 109] = VP1.dwPanTiltAbsolute[4] & 0xff;
	m_pParam[offset + 110] = VP1.dwPanTiltAbsolute[3] >> 24;
	m_pParam[offset + 111] = VP1.dwPanTiltAbsolute[3] >> 16;
	m_pParam[offset + 112] = VP1.dwPanTiltAbsolute[3] >>  8;
	m_pParam[offset + 113] = VP1.dwPanTiltAbsolute[3] & 0xff;
	m_pParam[offset + 114] = VP1.dwPanTiltAbsolute[2] >> 24;
	m_pParam[offset + 115] = VP1.dwPanTiltAbsolute[2] >> 16;
	m_pParam[offset + 116] = VP1.dwPanTiltAbsolute[2] >>  8;
	m_pParam[offset + 117] = VP1.dwPanTiltAbsolute[2] & 0xff;
	m_pParam[offset + 118] = VP1.dwPanTiltAbsolute[1] >> 24;
	m_pParam[offset + 119] = VP1.dwPanTiltAbsolute[1] >> 16;
	m_pParam[offset + 120] = VP1.dwPanTiltAbsolute[1] >>  8;
	m_pParam[offset + 121] = VP1.dwPanTiltAbsolute[1] & 0xff;
	m_pParam[offset + 122] = VP1.dwPanTiltAbsolute[0] >> 24;
	m_pParam[offset + 123] = VP1.dwPanTiltAbsolute[0] >> 16;
	m_pParam[offset + 124] = VP1.dwPanTiltAbsolute[0] >>  8;
	m_pParam[offset + 125] = VP1.dwPanTiltAbsolute[0] & 0xff;
	m_pParam[offset + 126] = VP1.bPanTiltRelative[19];
	m_pParam[offset + 127] = VP1.bPanTiltRelative[18];
	m_pParam[offset + 128] = VP1.bPanTiltRelative[17];
	m_pParam[offset + 129] = VP1.bPanTiltRelative[16];
	m_pParam[offset + 130] = VP1.bPanTiltRelative[15];
	m_pParam[offset + 131] = VP1.bPanTiltRelative[14];
	m_pParam[offset + 132] = VP1.bPanTiltRelative[13];
	m_pParam[offset + 133] = VP1.bPanTiltRelative[12];
	m_pParam[offset + 134] = VP1.bPanTiltRelative[11];
	m_pParam[offset + 135] = VP1.bPanTiltRelative[10];
	m_pParam[offset + 136] = VP1.bPanTiltRelative[9];
	m_pParam[offset + 137] = VP1.bPanTiltRelative[8];
	m_pParam[offset + 138] = VP1.bPanTiltRelative[7];
	m_pParam[offset + 139] = VP1.bPanTiltRelative[6];
	m_pParam[offset + 140] = VP1.bPanTiltRelative[5];
	m_pParam[offset + 141] = VP1.bPanTiltRelative[4];
	m_pParam[offset + 142] = VP1.bPanTiltRelative[3];
	m_pParam[offset + 143] = VP1.bPanTiltRelative[2];
	m_pParam[offset + 144] = VP1.bPanTiltRelative[1];
	m_pParam[offset + 145] = VP1.bPanTiltRelative[0];
	m_pParam[offset + 146] = VP1.wRollAbsolute[4] >> 8;
	m_pParam[offset + 147] = VP1.wRollAbsolute[4] & 0xff;
	m_pParam[offset + 148] = VP1.wRollAbsolute[3] >> 8;
	m_pParam[offset + 149] = VP1.wRollAbsolute[3] & 0xff;
	m_pParam[offset + 150] = VP1.wRollAbsolute[2] >> 8;
	m_pParam[offset + 151] = VP1.wRollAbsolute[2] & 0xff;
	m_pParam[offset + 152] = VP1.wRollAbsolute[1] >> 8;
	m_pParam[offset + 153] = VP1.wRollAbsolute[1] & 0xff;
	m_pParam[offset + 154] = VP1.wRollAbsolute[0] >> 8;
	m_pParam[offset + 155] = VP1.wRollAbsolute[0] & 0xff;
	m_pParam[offset + 156] = VP1.bRollRelative[9];
	m_pParam[offset + 157] = VP1.bRollRelative[8];
	m_pParam[offset + 158] = VP1.bRollRelative[7];
	m_pParam[offset + 159] = VP1.bRollRelative[6];
	m_pParam[offset + 160] = VP1.bRollRelative[5];
	m_pParam[offset + 161] = VP1.bRollRelative[4];
	m_pParam[offset + 162] = VP1.bRollRelative[3];
	m_pParam[offset + 163] = VP1.bRollRelative[2];
	m_pParam[offset + 164] = VP1.bRollRelative[1];
	m_pParam[offset + 165] = VP1.bRollRelative[0];
	m_pParam[offset + 166] = VP1.bPrivacy;
	
	return true;
}

bool CParamEdit::Set_VideoParam2(const S_VIDEO_PARAM_2 &VP2)
{
	bool	bRet;
	
	bRet = set_videoparam2(VP2);
	return bRet;
}

bool CParamEdit::set_videoparam2(const S_VIDEO_PARAM_2 &VP2)
{
	int	offset = OFFSET_VIDEO_PARAM_2;
	
	m_pParam[offset + 0] 	= VP2.wBacklightCompensation[4] >> 8;
	m_pParam[offset + 1] 	= VP2.wBacklightCompensation[4] & 0xff;
	m_pParam[offset + 2] 	= VP2.wBacklightCompensation[3] >> 8;
	m_pParam[offset + 3] 	= VP2.wBacklightCompensation[3] & 0xff;
	m_pParam[offset + 4] 	= VP2.wBacklightCompensation[2] >> 8;
	m_pParam[offset + 5] 	= VP2.wBacklightCompensation[2] & 0xff;
	m_pParam[offset + 6] 	= VP2.wBacklightCompensation[1] >> 8;
	m_pParam[offset + 7] 	= VP2.wBacklightCompensation[1] & 0xff;
	m_pParam[offset + 8] 	= VP2.wBacklightCompensation[0] >> 8;
	m_pParam[offset + 9] 	= VP2.wBacklightCompensation[0] & 0xff;
	m_pParam[offset + 10] 	= VP2.wBrightness[4] >> 8;
	m_pParam[offset + 11] 	= VP2.wBrightness[4] & 0xff;
	m_pParam[offset + 12] 	= VP2.wBrightness[3] >> 8;
	m_pParam[offset + 13] 	= VP2.wBrightness[3] & 0xff;
	m_pParam[offset + 14] 	= VP2.wBrightness[2] >> 8;
	m_pParam[offset + 15] 	= VP2.wBrightness[2] & 0xff;
	m_pParam[offset + 16]	= VP2.wBrightness[1] >> 8;
	m_pParam[offset + 17] 	= VP2.wBrightness[1] & 0xff;
	m_pParam[offset + 18] 	= VP2.wBrightness[0] >> 8;
	m_pParam[offset + 19] 	= VP2.wBrightness[0] & 0xff;
	m_pParam[offset + 20] 	= VP2.wContrast[4] >> 8;
	m_pParam[offset + 21] 	= VP2.wContrast[4] & 0xff;
	m_pParam[offset + 22] 	= VP2.wContrast[3] >> 8;
	m_pParam[offset + 23] 	= VP2.wContrast[3] & 0xff;
	m_pParam[offset + 24] 	= VP2.wContrast[2] >> 8;
	m_pParam[offset + 25] 	= VP2.wContrast[2] & 0xff;
	m_pParam[offset + 26] 	= VP2.wContrast[1] >> 8;
	m_pParam[offset + 27] 	= VP2.wContrast[1] & 0xff;
	m_pParam[offset + 28] 	= VP2.wContrast[0] >> 8;
	m_pParam[offset + 29] 	= VP2.wContrast[0] & 0xff;	
	m_pParam[offset + 30] 	= VP2.wGain[4] >> 8;
	m_pParam[offset + 31] 	= VP2.wGain[4] & 0xff;
	m_pParam[offset + 32] 	= VP2.wGain[3] >> 8;
	m_pParam[offset + 33] 	= VP2.wGain[3] & 0xff;
	m_pParam[offset + 34] 	= VP2.wGain[2] >> 8;
	m_pParam[offset + 35] 	= VP2.wGain[2] & 0xff;
	m_pParam[offset + 36] 	= VP2.wGain[1] >> 8;
	m_pParam[offset + 37] 	= VP2.wGain[1] & 0xff;
	m_pParam[offset + 38] 	= VP2.wGain[0] >> 8;
	m_pParam[offset + 39] 	= VP2.wGain[0] & 0xff;	
	m_pParam[offset + 40] 	= VP2.bPowerLineFreq[1];
	m_pParam[offset + 41] 	= VP2.bPowerLineFreq[0];
	m_pParam[offset + 42] 	= VP2.wHue[4] >> 8;
	m_pParam[offset + 43] 	= VP2.wHue[4] & 0xff;
	m_pParam[offset + 44] 	= VP2.wHue[3] >> 8;
	m_pParam[offset + 45] 	= VP2.wHue[3] & 0xff;
	m_pParam[offset + 46] 	= VP2.wHue[2] >> 8;
	m_pParam[offset + 47] 	= VP2.wHue[2] & 0xff;
	m_pParam[offset + 48] 	= VP2.wHue[1] >> 8;
	m_pParam[offset + 49] 	= VP2.wHue[1] & 0xff;
	m_pParam[offset + 50] 	= VP2.wHue[0] >> 8;
	m_pParam[offset + 51] 	= VP2.wHue[0] & 0xff;
	m_pParam[offset + 52] 	= VP2.bHueAuto[1];
	m_pParam[offset + 53] 	= VP2.bHueAuto[0];
	m_pParam[offset + 54] 	= VP2.wSaturation[4] >> 8;
	m_pParam[offset + 55] 	= VP2.wSaturation[4] & 0xff;
	m_pParam[offset + 56] 	= VP2.wSaturation[3] >> 8;
	m_pParam[offset + 57] 	= VP2.wSaturation[3] & 0xff;
	m_pParam[offset + 58] 	= VP2.wSaturation[2] >> 8;
	m_pParam[offset + 59] 	= VP2.wSaturation[2] & 0xff;
	m_pParam[offset + 60] 	= VP2.wSaturation[1] >> 8;
	m_pParam[offset + 61] 	= VP2.wSaturation[1] & 0xff;
	m_pParam[offset + 62] 	= VP2.wSaturation[0] >> 8;
	m_pParam[offset + 63] 	= VP2.wSaturation[0] & 0xff;
	m_pParam[offset + 64] 	= VP2.wSharpness[4] >> 8;
	m_pParam[offset + 65] 	= VP2.wSharpness[4] & 0xff;
	m_pParam[offset + 66] 	= VP2.wSharpness[3] >> 8;
	m_pParam[offset + 67] 	= VP2.wSharpness[3] & 0xff;
	m_pParam[offset + 68] 	= VP2.wSharpness[2] >> 8;
	m_pParam[offset + 69] 	= VP2.wSharpness[2] & 0xff;
	m_pParam[offset + 70] 	= VP2.wSharpness[1] >> 8;
	m_pParam[offset + 71] 	= VP2.wSharpness[1] & 0xff;
	m_pParam[offset + 72] 	= VP2.wSharpness[0] >> 8;
	m_pParam[offset + 73] 	= VP2.wSharpness[0] & 0xff;
	m_pParam[offset + 74] 	= VP2.wGamma[4] >> 8;
	m_pParam[offset + 75] 	= VP2.wGamma[4] & 0xff;
	m_pParam[offset + 76] 	= VP2.wGamma[3] >> 8;
	m_pParam[offset + 77] 	= VP2.wGamma[3] & 0xff;
	m_pParam[offset + 78] 	= VP2.wGamma[2] >> 8;
	m_pParam[offset + 79] 	= VP2.wGamma[2] & 0xff;
	m_pParam[offset + 80] 	= VP2.wGamma[1] >> 8;
	m_pParam[offset + 81] 	= VP2.wGamma[1] & 0xff;
	m_pParam[offset + 82] 	= VP2.wGamma[0] >> 8;
	m_pParam[offset + 83] 	= VP2.wGamma[0] & 0xff;
	m_pParam[offset + 84] 	= VP2.wWhiteBalanceTemperature[4] >> 8;
	m_pParam[offset + 85] 	= VP2.wWhiteBalanceTemperature[4] & 0xff;
	m_pParam[offset + 86] 	= VP2.wWhiteBalanceTemperature[3] >> 8;
	m_pParam[offset + 87] 	= VP2.wWhiteBalanceTemperature[3] & 0xff;
	m_pParam[offset + 88] 	= VP2.wWhiteBalanceTemperature[2] >> 8;
	m_pParam[offset + 89] 	= VP2.wWhiteBalanceTemperature[2] & 0xff;
	m_pParam[offset + 90] 	= VP2.wWhiteBalanceTemperature[1] >> 8;
	m_pParam[offset + 91] 	= VP2.wWhiteBalanceTemperature[1] & 0xff;
	m_pParam[offset + 92] 	= VP2.wWhiteBalanceTemperature[0] >> 8;
	m_pParam[offset + 93] 	= VP2.wWhiteBalanceTemperature[0] & 0xff;
	m_pParam[offset + 94] 	= VP2.bWhiteBalanceTemperatureAuto[1];
	m_pParam[offset + 95] 	= VP2.bWhiteBalanceTemperatureAuto[0];
	m_pParam[offset + 96] 	= VP2.wWhiteBalanceComponent[9] >> 8;
	m_pParam[offset + 97] 	= VP2.wWhiteBalanceComponent[9] & 0xff;
	m_pParam[offset + 98] 	= VP2.wWhiteBalanceComponent[8] >> 8;
	m_pParam[offset + 99] 	= VP2.wWhiteBalanceComponent[8] & 0xff;
	m_pParam[offset + 100] = VP2.wWhiteBalanceComponent[7] >> 8;
	m_pParam[offset + 101] = VP2.wWhiteBalanceComponent[7] & 0xff;
	m_pParam[offset + 102] = VP2.wWhiteBalanceComponent[6] >> 8;
	m_pParam[offset + 103] = VP2.wWhiteBalanceComponent[6] & 0xff;
	m_pParam[offset + 104] = VP2.wWhiteBalanceComponent[5] >> 8;
	m_pParam[offset + 105] = VP2.wWhiteBalanceComponent[5] & 0xff;
	m_pParam[offset + 106] = VP2.wWhiteBalanceComponent[4] >> 8;
	m_pParam[offset + 107] = VP2.wWhiteBalanceComponent[4] & 0xff;
	m_pParam[offset + 108] = VP2.wWhiteBalanceComponent[3] >> 8;
	m_pParam[offset + 109] = VP2.wWhiteBalanceComponent[3] & 0xff;
	m_pParam[offset + 110] = VP2.wWhiteBalanceComponent[2] >> 8;
	m_pParam[offset + 111] = VP2.wWhiteBalanceComponent[2] & 0xff;
	m_pParam[offset + 112] = VP2.wWhiteBalanceComponent[1] >> 8;
	m_pParam[offset + 113] = VP2.wWhiteBalanceComponent[1] & 0xff;
	m_pParam[offset + 114] = VP2.wWhiteBalanceComponent[0] >> 8;
	m_pParam[offset + 115] = VP2.wWhiteBalanceComponent[0] & 0xff;
	m_pParam[offset + 116] = VP2.bWhiteBalanceComponentAuto[1];
	m_pParam[offset + 117] = VP2.bWhiteBalanceComponentAuto[0];
	m_pParam[offset + 118] = VP2.wMultiplierStep[4] >> 8;
	m_pParam[offset + 119] = VP2.wMultiplierStep[4] & 0xff;
	m_pParam[offset + 120] = VP2.wMultiplierStep[3] >> 8;
	m_pParam[offset + 121] = VP2.wMultiplierStep[3] & 0xff;
	m_pParam[offset + 122] = VP2.wMultiplierStep[2] >> 8;
	m_pParam[offset + 123] = VP2.wMultiplierStep[2] & 0xff;
	m_pParam[offset + 124] = VP2.wMultiplierStep[1] >> 8;
	m_pParam[offset + 125] = VP2.wMultiplierStep[1] & 0xff;
	m_pParam[offset + 126] = VP2.wMultiplierStep[0] >> 8;
	m_pParam[offset + 127] = VP2.wMultiplierStep[0] & 0xff;
	m_pParam[offset + 128] = VP2.wMultiplierLimit[4] >> 8;
	m_pParam[offset + 129] = VP2.wMultiplierLimit[4] & 0xff;
	m_pParam[offset + 130] = VP2.wMultiplierLimit[3] >> 8;
	m_pParam[offset + 131] = VP2.wMultiplierLimit[3] & 0xff;
	m_pParam[offset + 132] = VP2.wMultiplierLimit[2] >> 8;
	m_pParam[offset + 133] = VP2.wMultiplierLimit[2] & 0xff;
	m_pParam[offset + 134] = VP2.wMultiplierLimit[1] >> 8;
	m_pParam[offset + 135] = VP2.wMultiplierLimit[1] & 0xff;
	m_pParam[offset + 136] = VP2.wMultiplierLimit[0] >> 8;
	m_pParam[offset + 137] = VP2.wMultiplierLimit[0] & 0xff;
	
	return true;
}

bool CParamEdit::Set_VideoParam3(const S_VIDEO_PARAM_3 &VP3)
{
	bool	bRet;
	
	bRet = set_videoparam3(VP3);
	return bRet;
}

bool CParamEdit::set_videoparam3(const S_VIDEO_PARAM_3 &VP3)
{
	int	offset = OFFSET_VIDEO_PARAM_3;
	
	m_pParam[offset + 0] 	= VP3.bAuxLedDis;
	m_pParam[offset + 1] 	= VP3.bAuxLedDef;
	m_pParam[offset + 2] 	= VP3.bAuxLedBright[3];
	m_pParam[offset + 3] 	= VP3.bAuxLedBright[2];
	m_pParam[offset + 4] 	= VP3.bAuxLedBright[1];
	m_pParam[offset + 5] 	= VP3.bAuxLedBright[0];
	m_pParam[offset + 6] 	= VP3.bPrivacyDis;
	m_pParam[offset + 7] 	= VP3.bPrivacyDef;
	m_pParam[offset + 8] 	= VP3.bWBComponentDis;
	m_pParam[offset + 9] 	= VP3.wWBBlueDef >> 8;
	m_pParam[offset + 10] 	= VP3.wWBBlueDef & 0xff;
	m_pParam[offset + 11] 	= VP3.wWBRedDef >> 8;
	m_pParam[offset + 12] 	= VP3.wWBRedDef & 0xff;
	m_pParam[offset + 13] 	= VP3.bWBBlueRes;
	m_pParam[offset + 14]	= VP3.bWBRedRes;
	m_pParam[offset + 15] 	= VP3.wWBBlueMax >> 8;
	m_pParam[offset + 16] 	= VP3.wWBBlueMax & 0xff;
	m_pParam[offset + 17] 	= VP3.wWBRedMax >> 8;
	m_pParam[offset + 18] 	= VP3.wWBRedMax & 0xff;
	m_pParam[offset + 19] 	= VP3.wWBBlueMin >> 8; 
	m_pParam[offset + 20] 	= VP3.wWBBlueMin & 0xff;
	m_pParam[offset + 21] 	= VP3.wWBRedMin >> 8;
	m_pParam[offset + 22] 	= VP3.wWBRedMin & 0xff;
	m_pParam[offset + 23] 	= VP3.bWBComponentAutoDis;
	m_pParam[offset + 24] 	= VP3.bWBComponentAutoDef;
	
	return true;
}
