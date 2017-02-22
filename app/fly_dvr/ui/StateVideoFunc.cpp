#include "Flydvr_Common.h"
#include "Flydvr_Message.h"
#include "Flydvr_General.h"
#include "Flydvr_Menu.h"
#include "StateVideoFunc.h"
#include "Sonix_ISPIF.h"
#include "Flydvr_Media.h"
#include "Flydvr_USB.h"
#include "MenuSetting.h"

#define STORAGE_SIZE_NEED 	 1024;//MB

static FLY_BOOL             bVideoRecording         = FLY_FALSE;

FLY_BOOL VideoFunc_RecordStatus(void)
{
	return bVideoRecording;
}

VideoRecordStatus VideoFunc_Record(void)
{
	UINT32 freeSpace;
	if(SDMMC_OUT == Flydvr_SDMMC_GetMountState())
    {
        lidbg("%s:No Card: Mount Fail !!!!!!\r\n",__func__);
		Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
		Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE, 0);
        return VIDEO_REC_NO_SD_CARD;
    }

	//Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_KEY_RESUME, 0);
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

	return VIDEO_REC_RESUME;
}

void StateVideoRecMode_FrontCamStartRecordingProc(UINT32 ulJobEvent)
{
	Flydvr_ISP_IF_LIB_StartFrontVR();
    return;
}

void StateVideoRecMode_FrontCamStopRecordingProc(UINT32 ulJobEvent)
{
	Flydvr_ISP_IF_LIB_StopFrontVR();
    return;
}

void StateVideoRecMode_RearCamStartRecordingProc(UINT32 ulJobEvent)
{
	Flydvr_ISP_IF_LIB_StartRearVR();
    return;
}

void StateVideoRecMode_RearCamStopRecordingProc(UINT32 ulJobEvent)
{
	Flydvr_ISP_IF_LIB_StopRearVR();
    return;
}

/*VR Write Button*/
void StateVideoRecMode_ResumeProc(UINT32 ulJobEvent)
{
	VideoFunc_Record();
    return;
}

void StateVideoRecMode_PauseProc(UINT32 ulJobEvent)
{
	//Flydvr_ISP_IF_LIB_StopFrontVR();
	//Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_KEY_PAUSE , 0);
	Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
	Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
    return;
}





void StateVideoRecMode(UINT32 ulEvent, UINT32 ulParam)
{
    UINT32      ulJobEvent = ulEvent;
    FLY_BOOL    ret = FLY_TRUE;
    UINT32      CurSysMode;

	switch(ulJobEvent)
    {
        case EVENT_NONE                           :
        break;

		case EVENT_VIDEO_VR_INIT:
			lidbg("@@@ EVENT_VIDEO_VR_INIT -\r\n");
			Flydvr_ISP_IF_LIB_StartLPDaemon();

			if(Flydvr_SDMMC_GetMountState() == SDMMC_OUT)
			{
				lidbg("EVENT_VIDEO_VR_INIT:No Card: Mount Fail !!!!!!\r\n");
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
			}
			else
			{
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
			}

			Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_SINGLE_FILE_TIME , MenuSettingConfig()->uiSingleFileRecordTime);//mins


			usleep(10*1000);

			/*Before driver, in case driver not notify*/
			if(Flydvr_ISP_IF_LIB_CheckFrontCamExist() == FLY_TRUE)
			{
				Flydvr_SetFrontCameraConnect(FLY_TRUE);
				Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_DETECT, 0);
			}
			if(Flydvr_ISP_IF_LIB_CheckRearCamExist() == FLY_TRUE)
			{
				Flydvr_SetRearCameraConnect(FLY_TRUE);
				Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_DETECT, 0);
			}
			
			break;

		case EVENT_VIDEO_KEY_RECORD_START               :
            lidbg("@@@ EVENT_VIDEO_KEY_RECORD_START -\r\n");
			MenuSettingConfig()->uiRecordSwitch = RECORD_START;
			uiSaveCurrentConfig();
            StateVideoRecMode_ResumeProc(ulJobEvent);
        	break;

		case EVENT_VIDEO_KEY_RECORD_STOP               :
            lidbg("@@@ EVENT_VIDEO_KEY_RECORD_STOP -\r\n");
			MenuSettingConfig()->uiRecordSwitch = RECORD_STOP;
			uiSaveCurrentConfig();
            StateVideoRecMode_PauseProc(ulJobEvent);
        	break;

		case EVENT_VIDEO_KEY_RECORD_MODE:
            lidbg("@@@ EVENT_VIDEO_KEY_RECORD_MODE [%d]-\r\n",ulParam);
			MenuSettingConfig()->uiRecordMode= ulParam;
			uiSaveCurrentConfig();
			if(Flydvr_SDMMC_GetMountState() == SDMMC_OUT)
			{
				lidbg("EVENT_VIDEO_KEY_RECORD_MODE:No Card: Mount Fail !!!!!!\r\n");
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
			}
			else
			{
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
			}
        	break;

		case EVENT_VIDEO_KEY_SINGLE_FILE_TIME:
            lidbg("@@@ EVENT_VIDEO_KEY_SINGLE_FILE_TIME [%d]-\r\n",ulParam);
			MenuSettingConfig()->uiSingleFileRecordTime= ulParam;
			uiSaveCurrentConfig();
			Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_SINGLE_FILE_TIME , MenuSettingConfig()->uiSingleFileRecordTime);//mins
        	break;

		case EVENT_VIDEO_KEY_EMERGENCY_SAVEDAYS:
            lidbg("@@@ EVENT_VIDEO_KEY_EMERGENCY_SAVEDAYS -\r\n");
			MenuSettingConfig()->uiEmergencySaveDays= ulParam;
			uiSaveCurrentConfig();
        	break;

		case EVENT_VIDEO_KEY_EMERGENCY_SWITCH_ON:
            lidbg("@@@ EVENT_VIDEO_KEY_EMERGENCY_SWITCH_ON -\r\n");
			MenuSettingConfig()->uiEmergencySwitch= EMERGENCY_START;
			uiSaveCurrentConfig();
        	break;

		case EVENT_VIDEO_KEY_EMERGENCY_SWITCH_OFF:
            lidbg("@@@ EVENT_VIDEO_KEY_EMERGENCY_SWITCH_OFF -\r\n");
			MenuSettingConfig()->uiEmergencySwitch= EMERGENCY_STOP;
			uiSaveCurrentConfig();
        	break;

		case EVENT_FRONT_CAM_DETECT:
			lidbg("@@@ EVENT_FRONT_CAM_DETECT -\r\n");
			StateVideoRecMode_FrontCamStartRecordingProc(ulJobEvent);		
        	break;

        case EVENT_FRONT_CAM_REMOVED:
            lidbg("@@@ EVENT_FRONT_CAM_REMOVED -\r\n");
			StateVideoRecMode_FrontCamStopRecordingProc(ulJobEvent);
        	break;

		case EVENT_REAR_CAM_DETECT:
			lidbg("@@@ EVENT_REAR_CAM_DETECT -\r\n");
            StateVideoRecMode_RearCamStartRecordingProc(ulJobEvent);
        	break;

        case EVENT_REAR_CAM_REMOVED:
            lidbg("@@@ EVENT_REAR_CAM_REMOVED -\r\n");
			StateVideoRecMode_RearCamStopRecordingProc(ulJobEvent);
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
			Flydvr_DelLostDir(FLYDVR_MEDIA_MMC1);
            break;

        case EVENT_MMC1_REMOVED:
			lidbg("@@@ EVENT_MMC1_REMOVED -\r\n");
			//Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_KEY_PAUSE , 0);
			Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
			Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
            break;

		case EVENT_VRCB_MEDIA_FULL:
			lidbg("@@@ EVENT_VRCB_MEDIA_FULL -\r\n");
			{
				UINT8 byMediaID = FLYDVR_MEDIA_MMC1;
				INT8 minVRFileName[255];
				INT8 totalPathVRFileName[255];
				UINT32 filecnt, freeSpace;
				do
				{
					Flydvr_GetVRFileInfo(byMediaID, minVRFileName, &filecnt);
					lidbg("%s: ======[remove:%s]======\n", __func__,minVRFileName);	
					if(filecnt > 0)
					{
						sprintf(totalPathVRFileName, "%s/%s", MMC1_VR_PATH, minVRFileName);
						remove(totalPathVRFileName);
					}
					else 
					{
						lidbg("%s: ======No File left to delete!======\n", __func__);	
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_PAUSE , 0);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_PAUSE , 0);
						break;
					}
					Flydvr_GetPathFreeSpace(byMediaID, &freeSpace);	
				}while(freeSpace < MMC1_REVERSE_SIZE);
				
				lidbg("%s: ======[VRFile:total %d files; mini:%s]======\n", __func__,filecnt,minVRFileName);
				//Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_VRCB_MEDIA_FULL , 0);
			}
			break;
		case EVENT_GSENSOR_CRASH:
			if(MenuSettingConfig()->uiEmergencySwitch == EMERGENCY_START)
			{
				lidbg("@@@ EVENT_GSENSOR_CRASH -\r\n");
				Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_GSENSOR_CRASH , 0);
			}
			else
			{
				lidbg("@@@ EVENT_GSENSOR_CRASH REJECT!-\r\n");
			}
			break;
        default:
        break;
	}
	return;
}

