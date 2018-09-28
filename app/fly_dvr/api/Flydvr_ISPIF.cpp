#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lidbg_servicer.h"
#include "Flydvr_Common.h"
#include "Flydvr_ISPIF.h"
#include "Flydvr_Media.h"
#include "Sonix_ISPIF.h"

//#include "dvr_common.h"

VR_STATE front_state, rear_state;

void Flydvr_ISP_IF_LIB_Init()
{
	Sonix_ISP_IF_LIB_Init();
	Flydvr_ISP_IF_LIB_CloseOnlineFileAccess();
}

ISP_IF_VERSION Flydvr_ISP_IF_LIB_GetFrontLibVer()
{
	return Sonix_ISP_IF_LIB_GetFrontLibVer();
}

ISP_IF_VERSION Flydvr_ISP_IF_LIB_GetRearLibVer()
{
	return Sonix_ISP_IF_LIB_GetRearLibVer();
}

ISP_IF_VERSION Flydvr_ISP_IF_LIB_GetSensorName()
{
	return Sonix_ISP_IF_LIB_GetSensorName();
}

FLY_BOOL Flydvr_ISP_IF_LIB_CheckFrontCamExist()
{
	return Sonix_ISP_IF_LIB_CheckFrontCamExist();
}

FLY_BOOL Flydvr_ISP_IF_LIB_CheckRearCamExist()
{
	return Sonix_ISP_IF_LIB_CheckRearCamExist();
}

VR_STATE Flydvr_ISP_IF_LIB_GetFrontCamVRState()
{
	//if(Flydvr_IsFrontCameraConnect() == FLY_FALSE)
	//	return VR_STOP;
	return front_state;
}

VR_STATE Flydvr_ISP_IF_LIB_GetRearCamVRState()
{
	//if(Flydvr_IsRearCameraConnect()  == FLY_FALSE)
	//	return VR_STOP;
	return rear_state;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StartLPDaemon()
{
	if(Sonix_ISP_IF_LIB_StartLPDaemon() == 0)
		return FLY_TRUE;
	return FLY_FALSE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StartFrontVR()
{
	if(Sonix_ISP_IF_LIB_StartFrontVR() == 0)
	{
		front_state = VR_GENERAL;
		return FLY_TRUE;
	}
	return FLY_FALSE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StopFrontVR()
{
	if(Sonix_ISP_IF_LIB_StopFrontVR() == 0)
	{
		front_state = VR_STOP;
		return FLY_TRUE;
	}
	return FLY_FALSE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_OpenOnlineFileAccess()
{
	property_set("fly.uvccam.curprevnum", "0");//Online prop
	return FLY_TRUE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_CloseOnlineFileAccess()
{
	property_set("fly.uvccam.curprevnum", "-1");//Online prop
	return FLY_TRUE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_RemoveOnlineFile()
{
	remove(MMC0_ONLINE_VR_PATH"/tmp0.h264");
	return FLY_TRUE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StartFrontOnlineVR()
{
	Flydvr_ISP_IF_LIB_CloseOnlineFileAccess();
	Flydvr_ISP_IF_LIB_RemoveOnlineFile();
	if(Sonix_ISP_IF_LIB_StartFrontOnlineVR() == 0)
	{
		front_state = VR_ONLINE;
		return FLY_TRUE;
	}
	return FLY_FALSE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StopFrontOnlineVR()
{
	if(Sonix_ISP_IF_LIB_StopFrontOnlineVR() == 0)
	{
		front_state = VR_STOP;
		Flydvr_ISP_IF_LIB_RemoveOnlineFile();
		Flydvr_ISP_IF_LIB_CloseOnlineFileAccess();
		return FLY_TRUE;
	}
	Flydvr_ISP_IF_LIB_RemoveOnlineFile();
	Flydvr_ISP_IF_LIB_CloseOnlineFileAccess();
	return FLY_FALSE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StartRearVR()
{
	if(Sonix_ISP_IF_LIB_StartRearVR() == 0)
	{
		rear_state = VR_GENERAL;
		return FLY_TRUE;
	}
	return FLY_FALSE;
}

FLY_BOOL Flydvr_ISP_IF_LIB_StopRearVR()
{
	if(Sonix_ISP_IF_LIB_StopRearVR() == 0)
	{
		rear_state = VR_STOP;
		return FLY_TRUE;
	}
	return FLY_FALSE;
}


