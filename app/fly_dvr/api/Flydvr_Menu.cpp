#include "Flydvr_Menu.h"
#include "Flydvr_Message.h"
#include "Flydvr_General.h"
#include "Flydvr_Menu.h"
#include "Flydvr_Parameter.h"
#include "Flydvr_Media.h"
#include "Flydvr_USB.h"
#include "StateVideoFunc.h"
#include "StateACCFunc.h"
#include "MenuSetting.h"
#include "Sonix_ISPIF.h"

/*===========================================================================
 * Global variable
 *===========================================================================*/ 
 
static FLY_BOOL			FlydvrMenuInit    = FLY_FALSE;
static UI_STATE_INFO	uiSysState;

void RecordSettingInit( void )
{
    return;
}


FLY_BOOL StateSwitchMode(UI_STATE_ID mState)
{
    FLY_BOOL	ahc_ret = FLY_TRUE;

    if(FLY_TRUE == uiSysState.bModeLocked)
    {
        lidbg("--E-- Switch Mode Locked\n");
        return FLY_FALSE;
    }


    switch( mState )
    {

		case UI_POWERON_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
				lidbg("UI_POWERON_STATE\n");
				wdbg("UI_POWERON_STATE\n");
			}
        break;

		case UI_ACCOFF_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
				lidbg("UI_ACCOFF_STATE\n");
				wdbg("UI_ACCOFF_STATE\n");
				Flydvr_ISP_IF_LIB_StopFrontVR();
				Flydvr_ISP_IF_LIB_StopRearVR();
				Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DISABLE_CAM_POWER, NULL);
				CHECK_DEBUG_FILE;
			}
        break;

		case UI_ACCON_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
				lidbg("UI_ACCON_STATE\n");
				wdbg("UI_ACCON_STATE\n");
				/*Close online*/
				if(Flydvr_ISP_IF_LIB_GetFrontCamVRState() == VR_ONLINE)
				{
					lidbg("%s: ======ACCON front force close online!======\n", __func__);
					wdbg("ACCON front force close online!\n");
					Flydvr_ISP_IF_LIB_StopFrontOnlineVR();
				}

				/*Before enable power*/
				/*Reinit: Check if power already hold, restore VR*/
				if(Flydvr_IsFrontCameraConnect() == FLY_TRUE 
					&& Flydvr_ISP_IF_LIB_GetFrontCamVRState() == VR_STOP) //In case accon and no power state switch
				{
					lidbg("%s: ======ACCON front restore!======\n", __func__);
					wdbg("ACCON front restore!\n");
					Flydvr_ISP_IF_LIB_StartFrontVR();
				}
				
				if(Flydvr_IsRearCameraConnect() == FLY_TRUE
					&& Flydvr_ISP_IF_LIB_GetRearCamVRState() == VR_STOP) //In case accon and no power state switch
				{
					lidbg("%s: ======ACCON rear restore!======\n", __func__);
					wdbg("ACCON rear restore!\n");
					Flydvr_ISP_IF_LIB_StartRearVR();
				}

				/*Enable power*/
				Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_ENABLE_CAM_POWER, NULL);

				/*Reinit: SD State*/
				if(FLY_FALSE == Flydvr_SDMMC_GetMountState())
				{
					wdbg("ACCON SD NOT FOUND!\n");
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DISCONN_SDCARD, NULL);
				}
				else
				{
					wdbg("ACCON SD DETECT!\n");
					Flydvr_SDMMC_SetMountState(SDMMC_IN);
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_CONN_SDCARD, NULL);
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_SD_PLUG_IN);
					Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_MMC1_DETECT, 0);
					Flydvr_DelDaysFile(FLYDVR_MEDIA_MMC1, MenuSettingConfig()->uiEmergencySaveDays);
					Flydvr_DelLostDir(FLYDVR_MEDIA_MMC1);
				}
			}
        break;

		case UI_DEVICEOFF_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
			}
        break;

		case UI_DEVICEON_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
			}
        break;

		case UI_VIDEO_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
				lidbg("UI_VIDEO_STATE\n");
				wdbg("UI_VIDEO_STATE\n");
				if(uiSysState.LastState == UI_SD_FORMAT_STATE)
				{
					if(Flydvr_CheckVRPath(FLYDVR_MEDIA_MMC1)== FLY_FALSE)
					{
						lidbg("%s: ======VR Path Not Found!Make New One!======\n", __func__);
						Flydvr_MkVRPath(FLYDVR_MEDIA_MMC1);
					}
					lidbg("%s: ======Format => Video!======\n", __func__);
					if(Flydvr_CheckVRPath(FLYDVR_MEDIA_MMC1)== FLY_TRUE)
					{
						if(Flydvr_IsFrontCameraConnect() == FLY_TRUE)
							Flydvr_ISP_IF_LIB_StartFrontVR();
						if(Flydvr_IsRearCameraConnect() == FLY_TRUE)
							Flydvr_ISP_IF_LIB_StartRearVR();
					}
				}
			}
        break;

		case UI_SD_FORMAT_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
				lidbg("UI_SD_FORMAT_STATE\n");
				wdbg("UI_SD_FORMAT_STATE\n");
				if(Flydvr_IsFrontCameraConnect() == FLY_TRUE)
					Flydvr_ISP_IF_LIB_StopFrontVR();
				if(Flydvr_IsRearCameraConnect() == FLY_TRUE)
					Flydvr_ISP_IF_LIB_StopRearVR();
			}
        break;

		case UI_UDISK_FWUPDATE_STATE:
			if( uiSysState.CurrentState != mState )
			{   
				uiSysState.LastState = uiSysState.CurrentState;
			}
        break;
    }

	uiSysState.CurrentState = mState;
    
    return ahc_ret;
}

static void uiStateInitialize(void)
{
    FLY_BOOL 	runOp  = FLY_FALSE;
    
    //SystemSettingInit();
    RecordSettingInit();

    uiSysState.CurrentState = UI_STATE_UNSUPPORTED;

	
	StateSwitchMode(UI_POWERON_STATE);

	/*Test code*/
	StateSwitchMode(UI_VIDEO_STATE);//
	//Flydvr_SendMessage(0, EVENT_FRONT_CAM_DETECT, 0);//

	//#ifdef CFG_BOOT_INIT_LAST_STATE_AS_CUR //may be defined in config_xxx.h
	//if(uiGetLastState() == UI_STATE_UNSUPPORTED)
	//	uiSysState.LastState = uiGetCurrentState(); 
 	//#endif
	
	return;
}


void uiStateMachine( UINT32 ulMsgId, UINT32 ulEvent, UINT32 ulParam)
{

    if(FLY_FALSE == FlydvrMenuInit)
    {
        uiStateInitialize();      
        FlydvrMenuInit = FLY_TRUE;
    }
        

    switch( uiSysState.CurrentState )
    {
		case UI_POWERON_STATE:
			lidbg("=======UI_POWERON_STATE=======\n");
        break;

		case UI_ACCOFF_STATE:
			lidbg("=======UI_ACCOFF_STATE=======\n");
			StateACCOFFMode(ulMsgId, ulEvent,ulParam);
        break;

		case UI_ACCON_STATE:
			lidbg("=======UI_ACCON_STATE=======\n");
			StateACCONMode(ulMsgId, ulEvent,ulParam);
        break;

		case UI_DEVICEOFF_STATE:
			lidbg("=======UI_DEVICEOFF_STATE=======\n");
        break;

		case UI_DEVICEON_STATE:
			lidbg("=======UI_DEVICEON_STATE=======\n");
        break;

		case UI_VIDEO_STATE:
			lidbg("=======UI_VIDEO_STATE=======\n");
			//ulEvent = KeyParser_VideoRecEvent(ulMsgId, ulEvent, ulParam);
			StateVideoRecMode(ulMsgId, ulEvent,ulParam);
        break;

		case UI_SD_FORMAT_STATE:
			lidbg("=======UI_SD_FORMAT_STATE=======\n");
        break;

		case UI_UDISK_FWUPDATE_STATE:
			lidbg("=======UI_UDISK_FWUPDATE_STATE=======\n");
        break;
    }
	return;
}

void uiCheckDefaultMenuExist(void)
{
	MenuInfo *useMenu;
	MenuInfo *defMenu;
	FLY_BOOL ret;

	lidbg("### %s -\r\n", __func__);

	useMenu = MenuSettingConfig();
	ret = Flydvr_PARAM_Menu_Read((UINT8*)useMenu, FLY_DEFAULT_USER);
	ListAllMenuSetting(useMenu);
	return;
}

void uiSaveCurrentConfig(void)
{
	MenuInfo *useMenu;
	FLY_BOOL ret;

	lidbg("### %s -\r\n", __func__);

	useMenu = MenuSettingConfig();
	ret = Flydvr_PARAM_Menu_Write(FLY_DEFAULT_USER);
	ListAllMenuSetting(useMenu);
	return;
}

