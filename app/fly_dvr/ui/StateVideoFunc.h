typedef enum _VideoRecordStatus{
    VIDEO_REC_START,
    VIDEO_REC_RESUME,
    VIDEO_REC_PAUSE,
    VIDEO_REC_STOP,
    VIDEO_REC_CAPTURE,
    VIDEO_REC_CARD_FULL,
    VIDEO_REC_NO_SD_CARD,
    VIDEO_REC_WRONG_MEDIA,
	VIDEO_REC_SEAMLESS_ERROR,
	VIDEO_REC_SD_CARD_ERROR
}VideoRecordStatus;

void StateVideoRecMode_FrontCamPrepareProc(UINT32 ulJobEvent);
void StateVideoRecMode(UINT32 ulMsgId, UINT32 ulEvent, UINT32 ulParam);