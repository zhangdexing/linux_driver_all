#include "Flydvr_Common.h"
#include "Flydvr_OS.h"

FLY_BOOL FLY_MSM_OS_CreatePipe(FLY_OS_PIPE* osPipe, UINT32  cmdSize)
{
	if(pipe(osPipe->fd)==-1)
	{
		lidbg("Create pipe error.\n");
		return FLY_FALSE;
	}
	osPipe->cmdLength = cmdSize;
	return FLY_TRUE;
}

UINT8 FLY_MSM_OS_GetMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg, UINT32 ulTimeout)
{
	UINT8 ret;
	INT32 len;        /*length of the word read from the pipe*/
	fd_set read_fdst;
	struct timeval tv;
	INT32 nfds = 0;

	FD_ZERO(&read_fdst);
	FD_SET(osPipe->fd[0], &read_fdst);
	tv.tv_sec = ulTimeout/1000;
	tv.tv_usec = (ulTimeout*1000) - (ulTimeout/1000*1000);
	nfds = select(osPipe->fd[0]+1, &read_fdst,NULL, NULL, &tv);
	switch (nfds)
	{
		case -1:
			lidbg("Pipe: error\n");
			ret = 0xFF;
			break;
		case 0:
			//lidbg("Pipe: time out\n");
			ret = 0x01;
			break;
		default:
			len=read(osPipe->fd[0],(char*)msg,sizeof(FLYDVR_QUEUE_MESSAGE));	
			ret = 0;
			break;
	}
	//lidbg("Pipe1: %s, len = %d\n",buf,len);
	return ret;
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

UINT8 FLY_MSM_OS_PutMessage(FLY_OS_PIPE* osPipe, FLYDVR_QUEUE_MESSAGE* msg)
{
    write(osPipe->fd[1],(char*)msg,sizeof(FLYDVR_QUEUE_MESSAGE));
	return 0;
}