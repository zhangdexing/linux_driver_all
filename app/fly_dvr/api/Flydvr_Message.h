
typedef enum _FLYDVR_MESSAGE {
	FLYM_USER = 0x0,		
	FLYM_UI_NOTIFICATION,
	FLYM_MAX
}AHC_MESSAGE; 

#define EVENT_INDEX_NONE		0

typedef enum _KeyEventID {
    //Index 0
    EVENT_NONE              = EVENT_INDEX_NONE ,

    /* MMC Event.*/                              
    EVENT_MMC0_DETECT                            ,
    EVENT_MMC0_REMOVED                           ,
    EVENT_MMC1_DETECT                           ,
    EVENT_MMC1_REMOVED                          ,
    //Index 32
    EVENT_USB_DETECT                           ,
    EVENT_USB_REMOVED                          ,
    EVENT_FRONT_CAM_DETECT                           ,
	EVENT_FRONT_CAM_REMOVED                          ,
	EVENT_REAR_CAM_DETECT                           ,
	EVENT_REAR_CAM_REMOVED                          ,

    //  Capture Mode 
    EVENT_FRONT_CAM_DEQUEUE,
    EVENT_REAR_CAM_DEQUEUE,

	EVENT_FRONT_CAM_OSD_SYNC,
	EVENT_REAR_CAM_OSD_SYNC,
	EVENT_FRONT_ONLINE_CAM_OSD_SYNC,

	EVENT_FRONT_CAM_DEQUEUE_OLDFP,
	EVENT_REAR_CAM_DEQUEUE_OLDFP,

	EVENT_FRONT_CAM_FLUSH,
	EVENT_REAR_CAM_FLUSH,

	EVENT_KEY_PAUSE,
	EVENT_KEY_RESUME,

	EVENT_GSENSOR_CRASH,

	EVENT_USER_LOCK,

    //  Video Mode

	EVENT_FRONT_PAUSE,
	EVENT_FRONT_RESUME,
	EVENT_REAR_PAUSE,
	EVENT_REAR_RESUME,
	EVENT_FRONT_ONLINE_PAUSE,
	EVENT_FRONT_ONLINE_RESUME,

	EVENT_VIDEO_VR_INIT,

    /* Key Event.*/  
    EVENT_VIDEO_PREVIEW_INIT  ,
    EVENT_VIDEO_PREVIEW_MODE                   ,
    EVENT_VIDEO_KEY_CAPTURE                    ,
    EVENT_VIDEO_KEY_CAPTURE_STATUS_CLEAR       ,
    EVENT_VIDEO_KEY_RECORD                     ,
    EVENT_VIDEO_KEY_RECORD_START			   ,
    EVENT_VIDEO_KEY_RECORD_STOP                ,

	EVENT_VIDEO_KEY_EMERGENCY_SWITCH_ON                ,
	EVENT_VIDEO_KEY_EMERGENCY_SWITCH_OFF                ,

	EVENT_VIDEO_KEY_EMERGENCY_SAVEDAYS                ,

	EVENT_VIDEO_KEY_SINGLE_FILE_TIME                ,

	EVENT_VIDEO_KEY_RECORD_MODE                ,

	EVENT_VIDEO_KEY_GSENSOR_SENS                ,

	EVENT_VRCB_MEDIA_FULL                      ,

	EVENT_VRCB_MEDIA_ABNORMAL_FULL                      ,
	EVENT_VRCB_MEDIA_ABNORMAL_FULL_RESTORE                      ,

    EVENT_VRCB_RECDSTOP_CARDSLOW               ,
    EVENT_VRCB_AP_STOP_VIDEO_RECD              ,
    EVENT_VRCB_FILE_FULL                       ,
    EVENT_VRCB_MEDIA_SLOW                      ,
    EVENT_VRCB_MEDIA_ERROR                     ,    
    EVENT_VRCB_SEAM_LESS					   ,
    EVENT_VRCB_MOTION_DTC					   ,
    EVENT_VRCB_EMER_DONE					   ,
    EVENT_VR_START							   ,
    EVENT_VR_STOP							   ,
    EVENT_VR_WRITEINFO						   ,
    EVENT_VR_EMERGENT                          , 
    EVENT_VR_CONFIRM_EMERGENT                  , 
    EVENT_VR_UPDATE_EMERGENT                   , 
    EVENT_VR_CANCEL_EMERGENT                   , 
    EVENT_VR_STOP_EMERGENT                     , 

    //TBD
    EVENT_RECORD_BLINK_UI       		       ,

    EVENT_RECORD_MUTE						   ,
    EVENT_FORMAT_MEDIA						   ,	
    EVENT_CHANGE_NIGHT_MODE					   ,
    EVENT_VIDEO_PREVIEW						   ,
    EVENT_VIDEO_BROWSER						   ,
    EVENT_CAMERA_PREVIEW					   ,
    EVENT_LOCK_VR_FILE						   ,
    EVENT_SHOW_FW_VERSION					   ,
    EVENT_FORMAT_RESET_ALL					   ,
    EVENT_SHOW_GPS_INFO						   ,
    EVENT_EV_INCREASE						   ,
    EVENT_EV_DECREASE						   ,  
    EVENT_VR_OSD_SHOW                          ,
    EVENT_VR_OSD_HIDE                          ,

    //  File Access Event
    EVENT_FILE_SAVING      ,
    EVENT_FILE_DELETING                         ,
    EVENT_LOCK_FILE_G                           ,
    EVENT_LOCK_FILE_M                           ,
    EVENT_NO_FIT_STORAGE_ALLOCATE_CLEAR         ,
    EVENT_NO_FIT_STORAGE_ALLOCATE               ,


    //	Misc Event    
    EVENT_MODE_UNSUPPORTED  ,
    EVENT_MODE_LOCKED  ,  
    EVENT_LENS_WMSG    ,
    EVENT_CLEAR_WMSG   ,
    EVENT_FOCUS_PASS   ,

    //  MSDC Mode
    EVENT_MSDC_UPDATE_MESSAGE,

    //  GPS, Gsensor
    EVENT_GPSGSENSOR_UPDATE,

    //  Parking mode
    EVENT_PARKING_KEY, 

    //  LDWS mode
    EVENT_LDWS_KEY, 

    // 	Sys Calibration
    EVENT_SYS_CALIB_MODE_INIT     	,
    EVENT_SYS_CALIB_UPDATE_MESSAGE	,

    //Change Battery State
    EVENT_CHANGE_BATTERY_STATE		,

    EVENT_ADD_POI                   ,
    EVENT_DEL_POI                   ,

    // Switch Mode
    EVENT_KEY_NEXT_MODE,
    EVENT_KEY_PREV_MODE,
    EVENT_KEY_RETURN,

    // Switch WiFi Preview <-> Streaming Mode
    EVENT_SWITCH_WIFI_STREAMING_MODE,
    // Switch WiFi On/Off	
    EVENT_WIFI_SWITCH_TOGGLE,
    EVENT_WIFI_SWITCH_DISABLE,
    EVENT_WIFI_SWITCH_ENABLE,
    // Streaming
    EVENT_OPEN_H264_STREAM,
    EVENT_CLOSE_H264_STREAM,
    //
    EVENT_LASER_LED_ONOFF,
    
    EVENT_SPEEDCAM_POI_IGNORE,
    
    EVENT_LCD_STANDBY_CANCEL_HUD,
    
    EVENT_LCD_STANDBY_START_HUD,
    
    EVENT_FATIGUEALERT_START,
    EVENT_FATIGUEALERT_STOP,

    EVENT_REMIND_HEADLIGHT_START,
    EVENT_REMIND_HEADLIGHT_STOP,

	EVENT_LDWS_START,
    EVENT_LDWS_STOP,
	
    EVENT_CUS_SW1_ON,
    EVENT_CUS_SW1_OFF,
    
    EVENT_LDWS_CALIBRATION_FIRST_TIME_END,
    
    EVENT_PARKING_REC_START,
    EVENT_PARKING_REC_STOP,
    
    EVENT_CANCEL_CRUISE_SPEED_ALERT,

    //EVENT_ID_MAX = EVENT_INDEX_MAX
	EVENT_ID_MAX 
}KeyEventID;

