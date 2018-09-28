#ifndef _FLYDVR_MSM_OS_PIPE_H_
#define _FLYDVR_MSM_OS_PIPE_H_

typedef struct {
    INT32   fd[2];          
    INT32	cmdLength;
} FLY_OS_PIPE;

FLY_BOOL FLY_MSM_OS_CreatePipe(FLY_OS_PIPE* osPipe, UINT32  cmdSize);
UINT8 FLY_MSM_OS_GetMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg, UINT32 ulTimeout);
UINT8 FLY_MSM_OS_PutMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg);

#endif		//_FLYDVR_MSM_OS_PIPE_H_