#ifndef _FLYDVR_USB_H_
#define _FLYDVR_USB_H_

#include "Flydvr_Common.h"
#include "Flydvr_General.h"

void Flydvr_SetFrontCameraConnect(FLY_BOOL isConnect);
void Flydvr_SetRearCameraConnect(FLY_BOOL isConnect);
FLY_BOOL Flydvr_IsFrontCameraConnect(void);
FLY_BOOL Flydvr_IsRearCameraConnect(void);

#endif		//_FLYDVR_USB_H_