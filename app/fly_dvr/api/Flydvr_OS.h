#ifndef _FLYDVR_OS_H_
#define _FLYDVR_OS_H_

#include "Flydvr_Common.h"
#include "Flydvr_General.h"
#include "FLY_MSM_OS_Pipe.h"


		
FLY_BOOL Flydvr_OS_CreatePipe(FLY_OS_PIPE* osPipe, UINT32  cmdSize);
UINT8 Flydvr_OS_GetMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg, UINT32 ulTimeout);
UINT8 Flydvr_OS_PutMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg);

#endif		//_FLYDVR_OS_H_