#ifndef __LIDBG_FLYCAM_PAR_
#define __LIDBG_FLYCAM_PAR_

#define HYUNDAI_MODE	0

#define REARVIEW_ID	0
#define DVR_ID				1
#define REAR_BLOCK_ID_MODE				2
#define DVR_BLOCK_ID_MODE				3
#define REAR_GET_RES_ID_MODE				4
#define DVR_GET_RES_ID_MODE				5
#define SET_REAR_OSD_ID_MODE				6
#define SET_DVR_OSD_ID_MODE				7


typedef enum {
  NR_BITRATE,
  NR_RESOLUTION,
  NR_PATH,
  NR_TIME,
  NR_FILENUM,
  NR_TOTALSIZE,
  NR_START_REC,
  NR_STOP_REC,
  NR_SET_PAR,
  NR_GET_RES,
  NR_SATURATION,
  NR_TONE,
  NR_BRIGHT,
  NR_CONTRAST,
  NR_CAPTURE,
  NR_CAPTURE_PATH,
  NR_ISDUALCAM,
  NR_ISCOLDBOOTREC,
  NR_ISEMPERMITTED,
  NR_ISVIDEOLOOP,
  NR_DELDAYS,
  NR_CVBSMODE,
}cam_ctrl_t;

typedef enum {
  NR_EM_PATH,
  NR_EM_START,
  NR_EM_STATUS,
  NR_EM_TIME,
  NR_EM_MANUAL,
  NR_CAM_STATUS,
}em_ctrl_t;

typedef enum {
  NR_STATUS,
  NR_ACCON_CAM_READY,
  NR_DVR_FW_VERSION,
  NR_REAR_FW_VERSION,
  NR_DVR_RES,
  NR_REAR_RES,
  NR_ONLINE_NOTIFY,
  NR_ONLINE_INVOKE_NOTIFY,
  NR_NEW_DVR_NOTIFY,
  NR_NEW_DVR_IO,
  NR_NEW_DVR_ASYN_NOTIFY,
  NR_ENABLE_CAM_POWER,
  NR_DISABLE_CAM_POWER,
  NR_CONN_SDCARD,
  NR_DISCONN_SDCARD,
}status_ctrl_t;

typedef enum {
  NR_VERSION,
  NR_UPDATE,
}fw_ctrl_t;

typedef enum {
  NR_CMD,
}rec_ctrl_t;

typedef enum {
  CMD_AUTO_DETECT = 0x1,
  CMD_RECORD = 0x40,
  CMD_CAPTURE,
  CMD_SET_RESOLUTION,
  CMD_TIME_SEC,
  CMD_FW_VER,
  CMD_TOTALSIZE,
  CMD_PATH,
  CMD_SET_PAR,
  CMD_GET_RES,
  CMD_SET_EFFECT = 0x60,
  CMD_DUAL_CAM,
  CMD_EM_EVENT_SWITCH,
  CMD_FORMAT_SDCARD,
  CMD_EM_SAVE_DAYS,
  CMD_CVBS_MODE,
}rec_cmd_t;

#define FLYCAM_FRONT_REC_IOC_MAGIC  'F'
#define FLYCAM_FRONT_ONLINE_IOC_MAGIC  'f'
#define FLYCAM_REAR_REC_IOC_MAGIC  'R'
#define FLYCAM_REAR_ONLINE_IOC_MAGIC  'r'
#define FLYCAM_STATUS_IOC_MAGIC  's'
#define FLYCAM_FW_IOC_MAGIC  'w'
#define FLYCAM_REC_MAGIC  'c'
#define FLYCAM_EM_MAGIC  'e'

typedef enum {
 CAM_ID_BACK,
 CAM_ID_FRONT,
}cam_id_t;

typedef enum {
  RET_SUCCESS,
  RET_NOTVALID,
  RET_NOTSONIX,
  RET_FAIL,
  RET_IGNORE,
  RET_REPEATREQ,
}cam_ioctl_ret_t;

typedef enum {
  RET_ONLINE_INSUFFICIENT_SPACE_STOP = 1,
  RET_ONLINE_DISCONNECT,
  RET_ONLINE_FOUND_SONIX,
  RET_ONLINE_FOUND_NOTSONIX,
  RET_ONLINE_INTERRUPTED,
  RET_ONLINE_FORCE_STOP,
  RET_EM_ISREC_ON,
  RET_EM_ISREC_OFF,
}onlineNotify_ret_t;

typedef enum {
  RET_DEFALUT,
  RET_DVR_START,
  RET_DVR_STOP,
  RET_DVR_EXCEED_UPPER_LIMIT,
  RET_DVR_DISCONNECT,
  RET_DVR_INSUFFICIENT_SPACE_CIRC,
  RET_DVR_INSUFFICIENT_SPACE_STOP,
  RET_DVR_INIT_INSUFFICIENT_SPACE_STOP,
  RET_DVR_SONIX,
  RET_DVR_NOT_SONIX,
  RET_DVR_UD_SUCCESS,
  RET_DVR_UD_FAIL,
  RET_DVR_FW_ACCESS_FAIL,
  RET_DVR_FORCE_STOP,
  RET_EM_SD0_INSUFFICIENT_SPACE,
  RET_EM_SD1_INSUFFICIENT_SPACE,
  RET_DVR_OSD_FAIL,
  RET_FORMAT_SUCCESS = 0x50,
  RET_FORMAT_FAIL,
  RET_REAR_START = 0x81,
  RET_REAR_STOP,
  RET_REAR_EXCEED_UPPER_LIMIT,
  RET_REAR_DISCONNECT,
  RET_REAR_INSUFFICIENT_SPACE_CIRC,
  RET_REAR_INSUFFICIENT_SPACE_STOP,
  RET_REAR_INIT_INSUFFICIENT_SPACE_STOP,
  RET_REAR_SONIX,
  RET_REAR_NOT_SONIX,
  RET_REAR_UD_SUCCESS,
  RET_REAR_UD_FAIL,
  RET_REAR_FW_ACCESS_FAIL,
  RET_REAR_FORCE_STOP,
  RET_REAR_OSD_FAIL,
}cam_read_ret_t;

typedef enum {
  MSG_USB_NOTIFY,
  MSG_EM_SAVE_DAYS,
  MSG_EM_SWITCH,
  MSG_RECORD_MODE,
  MSG_RECORD_SWITCH,
  MSG_SINGLE_FILE_RECORD_TIME,
  MSG_GSENSOR_NOTIFY,
  MSG_ACCON_NOTIFY,
  MSG_ACCOFF_NOTIFY,
  MSG_START_FORMAT_NOTIFY,
  MSG_STOP_FORMAT_NOTIFY,
  MSG_START_ONLINE_VR_NOTIFY,
  MSG_STOP_ONLINE_VR_NOTIFY,
}dvr_msg_t;

struct status_info {
	unsigned char emergencySaveDays;
	unsigned char emergencySwitch;
	unsigned char recordMode;
	unsigned char recordSwitch;
	unsigned char singleFileRecordTime;
	bool isACCOFF;
	bool isFrontCamReady;
	bool isRearCamReady;
	bool isSDCardReady;
};

typedef enum {
    K_RECORD_STOP = 0 ,
    K_RECORD_START = 1,
    K_RECORD_NUM
} K_RECORD_SWITCH_SETTING;

typedef enum {
    K_EMERGENCY_STOP = 0 ,
    K_EMERGENCY_START = 1,
    K_EMERGENCY_NUM
} K_EMERGENCY_SWITCH_SETTING;

typedef enum {
    K_EMERGENCY_SAVE_1_DAYS = 1 ,
    K_EMERGENCY_SAVE_2_DAYS = 2 ,
    K_EMERGENCY_SAVE_3_DAYS= 3 ,
    K_EMERGENCY_SAVE_4_DAYS = 4 ,
    K_EMERGENCY_SAVE_5_DAYS = 5 ,
    K_EMERGENCY_SAVE_6_DAYS = 6 ,
    K_EMERGENCY_SAVE_7_DAYS = 7 ,
    K_EMERGENCY_SAVE_8_DAYS = 8 ,
    K_EMERGENCY_SAVE_9_DAYS = 9 ,
    K_EMERGENCY_SAVE_10_DAYS = 10 ,
    K_EMERGENCY_SAVE_11_DAYS = 11 ,
    K_EMERGENCY_SAVE_12_DAYS = 12 ,
    K_EMERGENCY_SAVE_13_DAYS = 13 ,
    K_EMERGENCY_SAVE_14_DAYS= 14 ,
    K_EMERGENCY_SAVE_NUM
} K_EMERGENCY_SAVEDAYS_SETTING;

typedef enum {
    K_SINGLEFILE_RECORD_TIME_1MINS = 1 ,
    K_SINGLEFILE_RECORD_TIME_2MINS = 2 ,
    K_SINGLEFILE_RECORD_TIME_3MINS = 3 ,
    K_SINGLEFILE_RECORD_TIME_4MINS = 4 ,
    K_SINGLEFILE_RECORD_TIME_5MINS = 5 ,
    K_SINGLEFILE_RECORD_TIME_6MINS = 6 ,
    K_SINGLEFILE_RECORD_TIME_7MINS = 7 ,
    K_SINGLEFILE_RECORD_TIME_8MINS = 8 ,
    K_SINGLEFILE_RECORD_TIME_9MINS = 9 ,
    K_SINGLEFILE_RECORD_TIME_10MINS = 10 ,
    K_SINGLEFILE_RECORD_TIME_NUM
} K_RECORD_TIME_SETTING;

typedef enum {
    K_RECORD_SINGLE_MODE = 0 ,
    K_RECORD_DUAL_MODE = 1,
    K_RECORD_MODE_NUM
} K_RECORD_MODE_SETTING;

//for hub
#define	UDISK_NODE		"1-1.1"
#define	FRONT_NODE		"1-1.2"	
#define	BACK_NODE		"1-1.3"

//int getVal();

#endif
