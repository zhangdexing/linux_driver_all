#ifndef _PARAMEDIT_H
#define _PARAMEDIT_H

#include "..//common//my_type.h"

#define	MAX_STR_LEN		128

typedef struct DEV_CAP
{
	BYTE		bType[3];
	BYTE		bSize;
	BYTE		bcdVersion;
	BYTE 		bAudioMode;
	WORD		wVendorID;
	WORD		wProductID;
	WORD		bcdDevice;
	BYTE		bMaxPower;
	BYTE		bKeyUsage;
	BYTE		bLEDUsage;
	BYTE		bWriteProtect;
	WORD		wObjFocalLenMin;
	WORD		wObjFocalLenMax;
	WORD		wOcularFocalLen;
	BYTE		bmCTControls[3];
	WORD		wMaxMultiplier;
	BYTE		bmPUControls[3];
	BYTE		bVideoMode;
	BYTE		bYUY2DefIdx;
	BYTE		bMJPGDefIdx;
	BYTE		bU1YUY2DefIdx;
	BYTE		bU1MJPGDefIdx;
	WORD		wString1Offset;
	WORD		wString2Offset;
	WORD		wString3Offset;
	WORD		wString4Offset;
	WORD		wVCParamOffset;
	BYTE		bUSBCtrl;
	BYTE		bStringDisable;
	WORD		wRecVol[4];
	WORD		wPlayVol[4];	
	bool		bOldVersion;
} S_DEV_CAP;

typedef struct DEV_CAP_2
{
	BYTE		bType[3];
	WORD		wVendorID;
	WORD		wProductID;
	WORD		bcdDevice;
	BYTE		bMaxPower;
	BYTE		bS3S4Backup;
	BYTE		bVdoImgDrop;
	BYTE		bKeyUsage;
	BYTE		bLEDUsage;
	BYTE		bMirrFlipUsage;
	BYTE		bWPUsage;
	BYTE		bmCTControls;
	BYTE		bmPUControls[2];
	BYTE		bS3S4Size;
	BYTE		szString0[MAX_STR_LEN];
	int		nStrLen0;
	BYTE		szString1[MAX_STR_LEN];
	int		nStrLen1;
	BYTE		szString2[MAX_STR_LEN];
	int		nStrLen2;
	BYTE		szString3[MAX_STR_LEN];
	int		nStrLen3;
} S_DEV_CAP_2;

/*
	"abc" as an example, 
	[0] = strlen("abc") * 2 + 2; // = 8
	[1] = 0x03; // type
	[2] = 'a';
	[3] = 0x00;
	[4] = 'b';
	[5] = 0x00;
	[6] = 'c';
	[7] = 0x00;
*/
typedef struct STRING_SETTING
{
	BYTE		szString1[MAX_STR_LEN];
	int		nStrLen1;					// NOT including 0x00 bytes & 2 header bytes.
	BYTE		szString2[MAX_STR_LEN];
	int		nStrLen2;
	BYTE		szString3[MAX_STR_LEN];
	int		nStrLen3;
	BYTE		szString4[MAX_STR_LEN];
	int		nStrLen4;
	BYTE		szString5[MAX_STR_LEN];
	int		nStrLen5;
	bool		bOldVersion;
} S_STRING_SETTING;

typedef struct VIDEO_INFO
{
	BYTE		bDevicePowerMode;
	BYTE		bRequestErrorCode;
	BYTE		bScanningMode;
	BYTE		bAutoExpoMode;
	BYTE		bAutoExpoPriority;
	BYTE		bExpoTimeAbsolute;
	BYTE		bExpoTimeRelative;
	BYTE		bFocusAbsolute;
	BYTE		bFocusRelative;
	BYTE		bFocusAuto;
	BYTE		bIrisAbsolute;
	BYTE		bIrisRelative;
	BYTE		bZoomAbsolute;
	BYTE		bZoomRelative;
	BYTE		bPanTiltAbsolute;
	BYTE		bPanTiltRelative;
	BYTE		bRollAbsolute;
	BYTE		bRollRelative;
	BYTE		bPrivacy;
	BYTE		bBacklightComp;
	BYTE		bBrightness;
	BYTE		bContrast;
	BYTE		bGain;
	BYTE		bPowerLineFreq;
	BYTE		bHue;
	BYTE		bHueAuto;
	BYTE		bSaturation;
	BYTE		bSharpness;
	BYTE		bGamma;
	BYTE		bWBTemperature;
	BYTE		bWBTemperatureAuto;
	BYTE		bWBComponent;
	BYTE		bWBComponentAuto;
	BYTE		bDigitalMultiplier;
	BYTE		bDigitalMultiplierLimit;	
} S_VIDEO_INFO;

typedef struct VIDEO_PARAM_1
{
	BYTE		bDevicePowerMode;
	BYTE		bRequestErrorCode;
	BYTE		bScanningMode;
	BYTE		bAutoExpoMode[3];
	BYTE		bAutoExpoPriority;
	DWORD		dwExpTimeAbsolute[5];
	BYTE		bExpTimeRelative;
	WORD		wFocusAbsolute[5];
	BYTE		bFocusRelative[10];
	BYTE		bFocusAuto[2];
	WORD		wIrisAbsolute[5];
	BYTE		bIrisRelative;
	WORD		wObjFocalLength[5];
	BYTE		bZoomRelative[15];
	DWORD		dwPanTiltAbsolute[10];
	BYTE		bPanTiltRelative[20];
	WORD		wRollAbsolute[5];
	BYTE		bRollRelative[10];
	BYTE		bPrivacy;
} S_VIDEO_PARAM_1;

typedef struct VIDEO_PARAM_2
{
	WORD		wBacklightCompensation[5];
	WORD		wBrightness[5];
	WORD		wContrast[5];
	WORD		wGain[5];
	BYTE		bPowerLineFreq[2];
	WORD		wHue[5];
	BYTE		bHueAuto[2];
	WORD		wSaturation[5];
	WORD		wSharpness[5];
	WORD		wGamma[5];
	WORD		wWhiteBalanceTemperature[5];
	BYTE		bWhiteBalanceTemperatureAuto[2];
	WORD		wWhiteBalanceComponent[10];
	BYTE		bWhiteBalanceComponentAuto[2];
	WORD		wMultiplierStep[5];
	WORD		wMultiplierLimit[5];
} S_VIDEO_PARAM_2;

typedef struct VIDEO_PARAM_3
{
	BYTE		bAuxLedDis;
	BYTE		bAuxLedDef;
	BYTE		bAuxLedBright[4];
	BYTE		bPrivacyDis;
	BYTE		bPrivacyDef;
	BYTE		bWBComponentDis;
	WORD		wWBBlueDef;
	WORD		wWBRedDef;
	BYTE		bWBBlueRes;
	BYTE		bWBRedRes;
	WORD		wWBBlueMax;
	WORD		wWBRedMax;
	WORD		wWBBlueMin;	
	WORD		wWBRedMin;
	BYTE		bWBComponentAutoDis;
	BYTE		bWBComponentAutoDef;			
} S_VIDEO_PARAM_3;

class CParamEdit
{
public: 
	CParamEdit();
	~CParamEdit();
	
	// copy the content of pParam to m_pParam
	bool	Set_To_Param(BYTE *pInBuf);
	// copy the content of m_pParam to pParam
	bool	Get_From_Param(BYTE *pOutBuf);
	
	bool	Get_DevCap(S_DEV_CAP *pDevCap);
	bool	Get_Str_Setting(S_STRING_SETTING *pSS);
	bool	Get_VideoInfo(S_VIDEO_INFO *pVI);
	bool	Get_VideoParam1(S_VIDEO_PARAM_1 *pVP1);
	bool	Get_VideoParam2(S_VIDEO_PARAM_2 *pVP2);
	bool	Get_VideoParam3(S_VIDEO_PARAM_3 *pVP3);
	
	bool	Set_DevCap(const S_DEV_CAP &DevCap);
	bool	Set_Str_Setting(const S_STRING_SETTING &SS);
	bool	Set_VideoInfo(const S_VIDEO_INFO &VI);
	bool	Set_VideoParam1(const S_VIDEO_PARAM_1 &VP1);
	bool	Set_VideoParam2(const S_VIDEO_PARAM_2 &VP2);
	bool	Set_VideoParam3(const S_VIDEO_PARAM_3 &VP3);
	
private: 
	BYTE				*m_pParam;
	unsigned int	m_nParamLen;
	
	bool	is_old_version(void);
	// convert normal string to firmware string 
	bool	str_normal_to_fw(const BYTE *inbuf, int str_len, BYTE *outbuf);
	// convert firmware string to normal string 
	bool	str_fw_to_normal(const BYTE *inbuf, BYTE *outbuf, int *pStrLen);
	
	bool	get_devcap(S_DEV_CAP *pDevCap);
	bool	get_str_setting(S_STRING_SETTING *pSS);
	bool	get_video_info(S_VIDEO_INFO *pVI);
	bool	get_video_param1(S_VIDEO_PARAM_1 *pVP1);
	bool	get_video_param2(S_VIDEO_PARAM_2 *pVP2);
	bool	get_video_param3(S_VIDEO_PARAM_3 *pVP3);
	
	//bool	get_devcap_232A(S_DEV_CAP *pDevCap);	
	//bool	get_str_setting_232A(S_STRING_SETTING *pSS);
	//bool	get_video_info_232A(S_VIDEO_INFO *pVI);
	//bool	get_video_param1_232A(S_VIDEO_PARAM_1 *pVP1);
	//bool	get_video_param2_232A(S_VIDEO_PARAM_2 *pVP2);
	//bool	get_video_param3_232A(S_VIDEO_PARAM_3 *pVP3);
	
	bool	set_devcap(const S_DEV_CAP &DevCap);
	bool	set_str_setting(const S_STRING_SETTING &SS);
	bool	set_videoinfo(const S_VIDEO_INFO &VI);
	bool	set_videoparam1(const S_VIDEO_PARAM_1 &VP1);
	bool	set_videoparam2(const S_VIDEO_PARAM_2 &VP2);
	bool	set_videoparam3(const S_VIDEO_PARAM_3 &VP3);
	
	//bool	set_devcap_232A(const S_DEV_CAP &DevCap);
	//bool	set_str_setting_232A(const S_STRING_SETTING &SS);
	//bool	set_videoinfo_232A(const S_VIDEO_INFO &VI);
	//bool	set_videoparam1_232A(const S_VIDEO_PARAM_1 &VP1);
	//bool	set_videoparam2_232A(const S_VIDEO_PARAM_2 &VP2);
	//bool	set_videoparam3_232A(const S_VIDEO_PARAM_3 &VP3);
};


#endif // _PARAMEDIT_H
