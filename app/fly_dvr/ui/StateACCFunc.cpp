#include "Flydvr_Common.h"
#include "Flydvr_Message.h"
#include "Flydvr_General.h"
#include "Flydvr_Menu.h"
#include "Sonix_ISPIF.h"
#include "StateACCFunc.h"

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

void StateACCOFFMode(UINT32 ulEvent, UINT32 ulParam)
{
	UINT32      ulJobEvent = ulEvent;
    FLY_BOOL    ret = FLY_TRUE;
    UINT32      CurSysMode;

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
	}
	return;
}

void StateACCONMode(UINT32 ulEvent, UINT32 ulParam)
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