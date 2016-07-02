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
}cam_ctrl_t;

typedef enum {
  NR_EM_PATH,
  NR_EM_START,
  NR_EM_STATUS,
  NR_EM_TIME,
  NR_EM_MANUAL,
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
  CMD_FORMAT_SDCARD = 0x63,
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

//for hub
#define	UDISK_NODE		"1-1.1"
#define	FRONT_NODE		"1-1.2"	
#define	BACK_NODE		"1-1.3"

#endif
