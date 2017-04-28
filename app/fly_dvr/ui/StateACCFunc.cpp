#include "Flydvr_Common.h"
#include "Flydvr_Message.h"
#include "Flydvr_General.h"
#include "Flydvr_Menu.h"
#include "Sonix_ISPIF.h"
#include "StateACCFunc.h"
#include "MenuSetting.h"
#include "Flydvr_Media.h"

void StateACCOFFMode_FrontCamStartRecordingProc(UINT32 ulJobEvent)
{
	//Flydvr_ISP_IF_LIB_StartFrontVR();
	Flydvr_ISP_IF_LIB_StartFrontOnlineVR();
    return;
}

void StateACCOFFMode_FrontCamStopRecordingProc(UINT32 ulJobEvent)
{
	Flydvr_ISP_IF_LIB_StopFrontVR();
	Flydvr_ISP_IF_LIB_StopFrontOnlineVR();
    return;
}

void StateACCOFFMode_RearCamStartRecordingProc(UINT32 ulJobEvent)
{
	//Flydvr_ISP_IF_LIB_StartRearVR();
    return;
}

void StateACCOFFMode_RearCamStopRecordingProc(UINT32 ulJobEvent)
{
	Flydvr_ISP_IF_LIB_StopRearVR();
    return;
}

void StateACCOFFMode(UINT32 ulMsgId, UINT32 ulEvent, UINT32 ulParam)
{
	UINT32      ulJobEvent = ulEvent;
    FLY_BOOL    ret = FLY_TRUE;
    UINT32      CurSysMode;
		
	if(ulMsgId == FLYM_UI_NOTIFICATION)
	{
		switch(ulJobEvent)
	    {
	        case EVENT_NONE                           :
	       	 	break;
			case EVENT_FRONT_CAM_DETECT:
				lidbg("@@@ EVENT_FRONT_CAM_DETECT -\r\n");
				StateACCOFFMode_FrontCamStartRecordingProc(ulJobEvent);		
	        	break;

	        case EVENT_FRONT_CAM_REMOVED:
	            lidbg("@@@ EVENT_FRONT_CAM_REMOVED -\r\n");
				StateACCOFFMode_FrontCamStopRecordingProc(ulJobEvent);
	        	break;

			case EVENT_REAR_CAM_DETECT:
				lidbg("@@@ EVENT_REAR_CAM_DETECT -\r\n");
	            //StateACCOFFMode_RearCamStartRecordingProc(ulJobEvent);
	        	break;

	        case EVENT_REAR_CAM_REMOVED:
	            lidbg("@@@ EVENT_REAR_CAM_REMOVED -\r\n");
				StateACCOFFMode_RearCamStopRecordingProc(ulJobEvent);
	        	break;
			case EVENT_MMC1_DETECT:
				lidbg("@@@ EVENT_MMC1_DETECT -\r\n");
				//Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_KEY_RESUME , 0);
				if(MenuSettingConfig()->uiRecordSwitch == RECORD_START)
				{
					if(MenuSettingConfig()->uiRecordMode == RECORD_SINGLE_MODE)
					{
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_RESUME , 0);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE, 0);
					}
					else if(MenuSettingConfig()->uiRecordMode == RECORD_DUAL_MODE)
					{
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_RESUME , 0);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_RESUME , 0);
					}
				}
				else if(MenuSettingConfig()->uiRecordSwitch == RECORD_STOP)
				{
					Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
					Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
				}
				Flydvr_MkVRPath(FLYDVR_MEDIA_MMC1);//prevent bug(Force mk)
				Flydvr_DelLostDir(FLYDVR_MEDIA_MMC1);
				//PRINT_STORAGE_FILESPACE_USAGE;
	            break;

	        case EVENT_MMC1_REMOVED:
				lidbg("@@@ EVENT_MMC1_REMOVED -\r\n");
				//Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_KEY_PAUSE , 0);
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
	            break;
		}
	}
	else if(ulMsgId == FLYM_DRV)
	{
		switch(ulJobEvent)
	    {
	        case EVENT_DRV_NONE                           :
				lidbg("@@@ EVENT_DRV_NONE -\r\n");
	        	break;
#if 0						
			case EVENT_DRV_VOLD_SD_REMOVED:
				lidbg("@@@ EVENT_DRV_VOLD_SD_REMOVED -\r\n");
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
				Flydvr_SDMMC_SetMountState(SDMMC_OUT);
				Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DISCONN_SDCARD, NULL);
				Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_SD_PLUG_OUT);
	        	break;
#endif						
			default:
	        	break;
		}
	}
	return;
}

void StateACCONMode(UINT32 ulMsgId, UINT32 ulEvent, UINT32 ulParam)
{
	UINT32      ulJobEvent = ulEvent;
    FLY_BOOL    ret = FLY_TRUE;
    UINT32      CurSysMode;

	switch(ulJobEvent)
    {
        case EVENT_NONE                           :
        break;
	}
	return;
}