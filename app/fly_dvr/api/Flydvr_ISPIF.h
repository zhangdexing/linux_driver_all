#ifndef _FLYDVR_ISPIF_H_
#define _FLYDVR_ISPIF_H_

#include "Flydvr_Common.h"

typedef	struct {
	UINT8			ID[16];
	UINT8			MajorVer;
	UINT8			MinorVer;
	UINT8			InnerVer0;
	UINT8			InnerVer1;
	UINT16			Year;
	UINT8			Month;
	UINT8			Day;
	UINT8			Vid;
	UINT8			Pid;
	char				szMfg[64];
	char				VerMark[11];
} ISP_IF_VERSION;

typedef enum{
    VR_STOP = 0,
    VR_ONLINE,
    VR_GENERAL
}VR_STATE;



void Flydvr_ISP_IF_LIB_Init();
ISP_IF_VERSION Flydvr_ISP_IF_LIB_GetFrontLibVer();
ISP_IF_VERSION Flydvr_ISP_IF_LIB_GetRearLibVer();
ISP_IF_VERSION Flydvr_ISP_IF_LIB_GetSensorName();
FLY_BOOL Flydvr_ISP_IF_LIB_CheckFrontCamExist();
FLY_BOOL Flydvr_ISP_IF_LIB_CheckRearCamExist();
VR_STATE Flydvr_ISP_IF_LIB_GetFrontCamVRState();
VR_STATE Flydvr_ISP_IF_LIB_GetRearCamVRState();
FLY_BOOL Flydvr_ISP_IF_LIB_StartLPDaemon();
FLY_BOOL Flydvr_ISP_IF_LIB_StartFrontVR();
FLY_BOOL Flydvr_ISP_IF_LIB_StopFrontVR();
FLY_BOOL Flydvr_ISP_IF_LIB_StartFrontOnlineVR();
FLY_BOOL Flydvr_ISP_IF_LIB_StopFrontOnlineVR();
FLY_BOOL Flydvr_ISP_IF_LIB_StartRearVR();
FLY_BOOL Flydvr_ISP_IF_LIB_StopRearVR();
FLY_BOOL Flydvr_ISP_IF_LIB_OpenOnlineFileAccess();
FLY_BOOL Flydvr_ISP_IF_LIB_CloseOnlineFileAccess();

#endif		//_FLYDVR_ISPIF_H_