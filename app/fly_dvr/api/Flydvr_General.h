#ifndef _FLYDVR_GENERAL_H_
#define _FLYDVR_GENERAL_H_

#define DRIVER_NODE   "/dev/lidbg_flycam0"
#define LOG_PATH			"/dev/log/DVRERR.txt"

#ifdef VERBOSE_DEBUG
#define vdbg lidbg
#else
#define vdbg(fmt, args...) \
	do { } while (0)
#endif /* DEBUG */


#define wdbg(msg...) do{\
	FILE *log_fp = NULL;\
	time_t time_p;\
	struct tm *tm_p; \
	time(&time_p);\
	tm_p = localtime(&time_p);\
	if(log_fp <= 0)\
	{\
		log_fp = fopen(LOG_PATH, "a+");\
		chmod(LOG_PATH,0777);\
	}\
	fprintf(log_fp,"%d-%02d-%02d__%02d.%02d.%02d: ",(1900+tm_p->tm_year), (1+tm_p->tm_mon), tm_p->tm_mday,tm_p->tm_hour , tm_p->tm_min,tm_p->tm_sec);\
	fprintf(log_fp,msg);\
	if(log_fp > 0) fclose(log_fp);\
}while(0)

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