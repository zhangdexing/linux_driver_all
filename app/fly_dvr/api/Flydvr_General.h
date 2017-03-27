#ifndef _FLYDVR_GENERAL_H_
#define _FLYDVR_GENERAL_H_
#include "lidbg_servicer.h"

#define DRIVER_NODE   "/dev/lidbg_flycam0"
#define LOG_PATH			"/data/lidbg/DVRDebug.txt"

#ifdef VERBOSE_DEBUG
#define vdbg lidbg
#else
#define vdbg(fmt, args...) \
	do { } while (0)
#endif /* DEBUG */


#define wdbg(msg...) general_wdbg(LOG_PATH, msg)

#define adbg(msg...) do{\
	lidbg(msg);\
	general_wdbg(LOG_PATH, msg);\
}while(0)

#define CHECK_DEBUG_FILE general_check_debug_file(LOG_PATH,1000000)

/// Video Event
typedef enum _FLY_VIDEO_EVENT {
    FLY_VIDEO_EVENT_NONE = 0,
	FLY_VIDEO_EVENT_FRONT_CAM_CONNECT,
	FLY_VIDEO_EVENT_FRONT_CAM_DISCONNECT,
	FLY_VIDEO_EVENT_REAR_CAM_CONNECT,
	FLY_VIDEO_EVENT_REAR_CAM_DISCONNECT,
	FLY_VIDEO_EVENT_GSENSOR_CRASH,
	FLY_VIDEO_EVENT_MEDIA_CONN,
	FLY_VIDEO_EVENT_MEDIA_DISCONN,
    FLY_VIDEO_EVENT_MEDIA_FULL,
    FLY_VIDEO_EVENT_MEDIA_ABNORMAL_FULL,
    FLY_VIDEO_EVENT_MEDIA_ABNORMAL_FULL_RESTORE,
    FLY_VIDEO_EVENT_FILE_FULL,
    FLY_VIDEO_EVENT_MEDIA_SLOW,
    FLY_VIDEO_EVENT_SEAMLESS,
    FLY_VIDEO_EVENT_MEDIA_ERROR,
    FLY_VIDEO_EVENT_ENCODE_START,
    FLY_VIDEO_EVENT_ENCODE_STOP,
    FLY_VIDEO_EVENT_POSTPROCESS,
    FLY_VIDEO_EVENT_BITSTREAM_DISCARD,
    FLY_VIDEO_EVENT_MEDIA_WRITE,
    FLY_VIDEO_EVENT_STREAMCB,
    FLY_VIDEO_EVENT_EMERGFILE_FULL,
    FLY_VIDEO_EVENT_RECDSTOP_CARDSLOW,
    FLY_VIDEO_EVENT_EVENT_APSTOPVIDRECD,
    FLY_VIDEO_EVENT_EVENT_PREGETTIME_CARDSLOW,
    FLY_VIDEO_EVENT_EVENT_UVCFILE_FULL,
    FLY_VIDEO_EVENT_EVENT_AUD_CAL_RESTART,
    FLY_VIDEO_EVENT_EVENT_MAX
} MMPS_3GPRECD_EVENT;

typedef struct _FLYDVR_QUEUE_MESSAGE 
{
	UINT32 ulMsgID; 
	UINT32 ulParam1;
	UINT32 ulParam2;
} FLYDVR_QUEUE_MESSAGE;

FLY_BOOL Flydvr_Initialize(void);
FLY_BOOL Flydvr_InitFlydvrMessage(void);
FLY_BOOL Flydvr_SendMessage(UINT32 ulMsgID, UINT32 ulParam1, UINT32 ulParam2);
FLY_BOOL Flydvr_GetMessage(UINT32* ulMsgID, UINT32* ulParam1, UINT32* ulParam2);
FLY_BOOL Flydvr_SendMessage_LP(UINT32 ulMsgID, UINT32 ulParam1, UINT32 ulParam2);
FLY_BOOL Flydvr_GetMessage_LP(UINT32* ulMsgID, UINT32* ulParam1, UINT32* ulParam2);
int Flydvr_SendDriverIoctl(const char *who, char magic , char nr, unsigned long  arg);
#endif		//_FLYDVR_GENERAL_H_