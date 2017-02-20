#include "Flydvr_Common.h"
#include "Flydvr_General.h"
#include "Flydvr_Parameter.h"
#include "MenuSetting.h"

FLY_PARAM 				glAhcParameter;

//------------------------------------------------------------------------------
//  Function    : AIHC_PARAM_Initialize
//  Description :
//------------------------------------------------------------------------------
/**
 @brief AHC parameters initailize
 @retval TRUE or FALSE. // TRUE: Success, FALSE: Fail
*/
FLY_BOOL Flydvr_PARAM_Initialize(void) {
	
	glAhcParameter.FlipLR 				= 0x0;	//Enable left/right flipping in both preview and capture modes. If enable, the captured image will be flipped.
	glAhcParameter.FlipUD 				= 0x0;	//Enable up/down flipping in both preview and capture modes. If enable, the captured image will be flipped.
	glAhcParameter.SensorMaxVideoWidth 	= 0x0;	//Get the maximum width in sensor preview setting.
	glAhcParameter.SensorMaxVideoHeight 	= 0x0;	//Get the maximum height in sensor preview setting.
	glAhcParameter.SensorMaxStillWidth 	= 0x0;	//Get the maximum width in sensor still capture setting.
	glAhcParameter.SensorMaxStillHeight = 0x0;	//Get the maximum height in sensor still capture setting.
	glAhcParameter.NightModeAE 			= 0x0;	//Perform the night mode exposure in next captured frame.
	glAhcParameter.CaptureMemoryCheck 	= 0x0;	//Enable for checking if there has sufficient memory for next shot, if not sufficient, then the transition from capture 
           										//to view is delayed to for ensuring the capture completed.
	glAhcParameter.FreeMemoryCheck 		= 0x0;	//Get the size of available buffer. User can know how many free bytes can be allocated.
    glAhcParameter.IgnoreAhlMessage 	= 0x0;	//Set to 1 to ignore all AHL messages, instead of keeping the messages, those messages will be dropped.
    glAhcParameter.AhlMsgUnblock 		= 0x0;	//Unblock the blocked AHL messages. The input of this parameter is an AHL message ID and be blocked previously.
	glAhcParameter.AhlMsgBlock 			= 0x0;	//Block the assigned AHL message upon the input ID.
	glAhcParameter.DebugPrintEnable 	= 0x0;	//Enable/disable the debug Prints.
	memset(glAhcParameter.SDInfo, 0, sizeof(glAhcParameter.SDInfo));    //If SD card inserted and mounted, returns some information.

	glAhcParameter.OsdWidth 			= 0x0;	//Get the Osd width in pixels (current layer).
	glAhcParameter.OsdHeight 			= 0x0;	//Get the Osd height in pixels (current layer).

	//Group: File & STORAGE
	glAhcParameter.MountFlags 			= 0x0;	//Configures the flags for media mount.
																		//0- No flag
																		//1- Remove all empty DCF objects (size=0) during mount.
																		//2- Filter DCF files by date. System will only mount files from the date specified by FilterDcfFilesByDate.
																		//3- Remove all files with the same file number during mount regardless file type.
    glAhcParameter.SDCardType 			= 0x0;	//This value is grabbed and stored during the SD/MMC mount. AHL can get current card type by this parameter. 0- MMC, 1- SD																		

	glAhcParameter.MediaFatType 		= 0x0;	//Set the desired FAT type before mount or read out current mounted FAT type.  FAT12/FAT16/FAT32
	glAhcParameter.CallbackBadBlockEnable = 0x0;	//If enabled, system will process the assigned callback function to let AHL know that there are too many bad blocks on current media.

	glAhcParameter.CallbackUserDataErr 	= 0x0;	//If enabled, system will process the assigned callback function to let AHL know that system discovered an error in user data.

	glAhcParameter.CallbackSystemDataErr = 0x0;	//If enabled, system will process the assigned callback function to let AHL know that system discovered an error in system data. Such like root entry or FAT.

	glAhcParameter.CallbackAccessTimeout = 0x0;	//If enabled, system will process the assigned callback function to let AHL handle the timeout during media access.
	return FLY_TRUE;
}


//------------------------------------------------------------------------------
//  Function    : AHC_SetParam
//  Description : This function sets a value to a specific variable. The description of the specific variable is defined in the document of parameter.
//------------------------------------------------------------------------------
/**
    @brief  This function sets a value to a specific variable.
    @param[in] wParamID : ID of parameters, refer to its description.
    @param[in] wValue : Refer to document for detail usages
    @return  the status of the operation
*/
#if 0
FLY_BOOL Flydvr_SetParam(AHC_PARAM_ID wParamID, UINT32 wValue)
{
	UINT32          ret_value = FLY_TRUE;
	return ret_value;
}


//------------------------------------------------------------------------------
//  Function    : AHC_GetParam
//  Description : This function gets a value from a specific variable. The description of the specific variable is defined in the document of parameter.
//------------------------------------------------------------------------------
/**
    @brief  This function gets a value from a specific variable.
    @param[in] wParamID : ID of parameters, refer to its description.
    @param[in] *wValue : Point for getting the variable back.
    @return  the status of the operation
*/
FLY_BOOL Flydvr_GetParam(AHC_PARAM_ID wParamID, UINT32 *wValue)
{
    UINT32 value, ret =FLY_TRUE;
    
	return ret;
}
#endif

FLY_BOOL Flydvr_PARAM_Menu_Write(FLYDVR_ACCESS_PROP accessProp)
{
	CheckMenuSetting(MenuSettingConfig());
	ImportMenuInfo(MenuSettingConfig());
	
	if(accessProp==FLY_DEFAULT_USER)
	{
		return ExportMenuProp(FLYDVR_USER_PROP);
	}
	else if(accessProp==FLY_DEFAULT_FACTORY)
	{
		return ExportMenuProp(FLYDVR_FACTORY_PROP);
	}
	return FLY_FALSE;
}

FLY_BOOL Flydvr_PARAM_Menu_Read(UINT8* readback, FLYDVR_ACCESS_PROP accessProp)
{
	if(accessProp==FLY_DEFAULT_USER)
	{
		return ParseMenuSetting(FLYDVR_USER_PROP, (MenuInfo*)readback);
	}
	else if(accessProp==FLY_DEFAULT_FACTORY)
	{
		return ParseMenuSetting(FLYDVR_FACTORY_PROP, (MenuInfo*)readback);
	}
	return FLY_FALSE;
}


