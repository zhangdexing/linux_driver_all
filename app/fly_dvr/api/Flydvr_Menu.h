#ifndef _FLYDVR_MENU_H_
#define _FLYDVR_MENU_H_
#include "Flydvr_Common.h"

#define KEYPAD_NONE                      	0
#define KEY_PRESS_START                     1
#define KEY_PRESS_UP                 		KEY_PRESS_START
#define KEY_PRESS_DOWN                 		2
#define KEY_PRESS_LEFT                 		3
#define KEY_PRESS_RIGHT                 	4
#define KEY_PRESS_OK                 		5
#define KEY_PRESS_REC                 		6
#define KEY_PRESS_MENU                 		7
#define KEY_PRESS_PLAY                 		8
#define KEY_PRESS_MODE                 		9
#define KEY_PRESS_POWER               		10
#define KEY_PRESS_TELE                 		11
#define KEY_PRESS_WIDE                 		12
#define KEY_PRESS_SOS                 		13
#define KEY_PRESS_MUTE                 		14
#define KEY_PRESS_CAPTURE                 	15
#define KEY_PRESS_FUNC1                 	16
#define KEY_PRESS_FUNC2                 	17
#define KEY_PRESS_FUNC3                 	18
#define KEY_PRESS_END                       19      // Keep this at last of KEY_PRESS_xxxxx
 
#define VRCB_RECDSTOP_CARDSLOW		        38
#define VRCB_AP_STOP_VIDEO_RECD             39
#define VRCB_MEDIA_FULL					 	40
#define VRCB_FILE_FULL					 	41
#define VRCB_MEDIA_SLOW					 	42
#define VRCB_MEDIA_ERROR		    	 	43
#define VRCB_SEAM_LESS						44
#define VRCB_MOTION_DTC						45
#define VRCB_VR_START						46
#define VRCB_VR_STOP						47
#define VRCB_VR_POSTPROCESS					48
#define VRCB_VR_EMER_DONE               	49
#define BATTERY_DETECTION				 	50

typedef enum _UI_STATE_ID {  
    UI_POWERON_STATE,	
    UI_UDISK_FWUPDATE_STATE,
	UI_ACCOFF_STATE,
    UI_ACCON_STATE,
    UI_DEVICEOFF_STATE,
    UI_DEVICEON_STATE,
    UI_VIDEO_STATE,
    UI_SD_FORMAT_STATE,
    UI_STATE_UNSUPPORTED
}UI_STATE_ID;

typedef struct 
{
    UI_STATE_ID        CurrentState;
    UI_STATE_ID        LastState;
    FLY_BOOL           bModeLocked;///< true means the state of state machine is locked and we can not change the state by
}UI_STATE_INFO;

FLY_BOOL StateSwitchMode(UI_STATE_ID mState);
static void uiStateInitialize(void);
void uiStateMachine( UINT32 ulMsgId, UINT32 ulEvent, UINT32 ulParam);
void uiCheckDefaultMenuExist(void);
void uiSaveCurrentConfig(void);

#endif