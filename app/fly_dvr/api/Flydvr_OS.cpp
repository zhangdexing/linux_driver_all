#include "Flydvr_Common.h"
#include "Flydvr_OS.h"
#include "FLY_MSM_OS_Pipe.h"

FLY_BOOL Flydvr_OS_CreatePipe(FLY_OS_PIPE* osPipe, UINT32  cmdSize)
{
    return FLY_MSM_OS_CreatePipe(osPipe, cmdSize);
}


//------------------------------------------------------------------------------
//  Function    : AHC_OS_GetMessage
//  Description :
//------------------------------------------------------------------------------
/** @brief This function waits for a message to be sent to a queue

@param[in] ulMQId : The message queue ID that return by @ref AHC_OS_CreateMQueue 
@param[in] ulTimeout : is an optional timeout period (in clock ticks).  If non-zero, your task will
                            wait for a message to arrive at the queue up to the amount of time
                            specified by this argument.  If you specify 0, however, your task will wait
                            forever at the specified queue or, until a message arrives.
@retval 0xFE for bad message queue id,
		0xFF for acquire message queue internal error from OS
		0 for getting the message.
		1 for time out happens
*/

UINT8 Flydvr_OS_GetMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg, UINT32 ulTimeout)
{
    return FLY_MSM_OS_GetMessage(osPipe, msg, ulTimeout);
}


//------------------------------------------------------------------------------
//  Function    : AHC_OS_PutMessage
//  Description :
//------------------------------------------------------------------------------
/** @brief This function sends a message to a queue

@param[in] ulMQId : The message queue ID that return by @ref AHC_OS_CreateMQueue 
@param[in] msg : is a pointer to the message to send.
@retval 0xFE for bad message queue id,
		0xFF for put message queue internal error from OS
		0 for getting the message.
*/

UINT8 Flydvr_OS_PutMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg)
{
    return FLY_MSM_OS_PutMessage(osPipe, msg);
}

