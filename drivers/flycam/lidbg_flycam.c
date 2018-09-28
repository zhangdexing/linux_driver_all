
#include "lidbg.h"
#include "lidbg_flycam_par.h"
LIDBG_DEFINE;

#define TAG	"Tflycam:"

static void work_format_done(struct work_struct *work);
static int checkSDCardStatus(char *path);

#define NOTIFIER_MAJOR_GSENSOR_STATUS_CHANGE	(130)
#define NOTIFIER_MINOR_EXCEED_THRESHOLD 		(10)

/*camStatus mask*/
#define FLY_CAM_ISVALID	0x01
#define FLY_CAM_ISSONIX	0x02
//#define FLY_CAM_ISDVRUSED		0x04
//#define FLY_CAM_ISONLINEUSED		0x08

#define GSENSOR_SENSITIVITY_PROP_NAME	"persist.gsensor.sensLevel"

struct fly_UsbCamInfo
{
	unsigned char camStatus;/*Camera status(DVR&RearView)*/
	unsigned char read_status;/*Camera status for HAL to read(notify poll)*/
	unsigned char onlineNotify_status;
	wait_queue_head_t camStatus_wait_queue;/*notify wait queue*/
	wait_queue_head_t onlineNotify_wait_queue;/*notify wait queue*/
	wait_queue_head_t newdvr_wait_queue;/*notify wait queue*/
	wait_queue_head_t DVR_ready_wait_queue;
	wait_queue_head_t Rear_ready_wait_queue;
	struct semaphore sem;
	struct semaphore notify_sem;
};

struct fly_UsbCamInfo *pfly_UsbCamInfo;
static DECLARE_DELAYED_WORK(work_t_format_done, work_format_done);
static DECLARE_COMPLETION (timer_stop_rec_wait);
static DECLARE_COMPLETION (Rear_fw_get_wait);
static DECLARE_COMPLETION (DVR_fw_get_wait);
static DECLARE_COMPLETION (Rear_res_get_wait);
static DECLARE_COMPLETION (DVR_res_get_wait);
static DECLARE_COMPLETION (accon_start_rec_wait);
static DECLARE_COMPLETION (set_par_wait);
static DECLARE_COMPLETION (ui_start_rec_wait);
static DECLARE_COMPLETION (start_dvr_rec_wait);
static DECLARE_COMPLETION (start_rear_rec_wait);
//static DECLARE_COMPLETION (auto_detect_wait);

/*Camera DVR & Online recording parameters*/
static int f_rec_time = 300,f_rec_totalsize = 8192;
static int f_online_bitrate = 500000,f_online_time = 60,f_online_filenum = 1,f_online_totalsize = 500;
char f_rec_res[100] = "1280x720",f_rec_path[100] = EMMC_MOUNT_POINT1"/camera_rec/";
char f_online_res[100] = "480x272",f_online_path[100] = EMMC_MOUNT_POINT0"/preview_cache/";

char r_rec_res[100] = "1280x720",r_rec_path[100] = EMMC_MOUNT_POINT1"/camera_rec/";

char em_path[100] = EMMC_MOUNT_POINT1"/camera_rec/BlackBox/";
static int top_em_time = 5,bottom_em_time = 10;

char capture_path[100] = EMMC_MOUNT_POINT0"/preview_cache/";

static struct timer_list suspend_stoprec_timer;
static struct timer_list em_start_dvr_rec_timer;
static struct timer_list em_start_rear_rec_timer;
static struct timer_list stop_thinkware_em_timer;

#define SUSPEND_STOPREC_ONLINE_TIME   (jiffies + 180*HZ)  /* 3min stop Rec after online*/
#define SUSPEND_STOPREC_ACCOFF_TIME   (jiffies + 180*HZ)  /* 3min stop Rec after accoff,fix online then accoff*/
#define SET_PAR_WAIT_TIME   (jiffies + 2*HZ) 
#define UI_REC_WAIT_TIME   (jiffies + 2*HZ) 
#define STOP_THINKNAVI_EM_WAIT_TIME   (jiffies + 30*HZ) 

/*bool var*/
static char isDVRRec,isRearRec,isOnlineNotifyReady,isDualCam;
static char isUpdating,isKSuspend,isDVRReady,isRearReady;
static char isEMDVRStartRec,isEMRearStartRec;
static char isNewDvrNotifyReady;


#define HAL_BUF_SIZE (512)
u8 *camStatus_data_for_hal;

#define FIFO_SIZE (512)
u8 *camStatus_fifo_buffer;
static struct kfifo camStatus_data_fifo;

u8 notify_data_buf;

#define NOTIFY_FIFO_SIZE (10)
u8 *notify_fifo_buffer;
static struct kfifo notify_data_fifo;

u8 camera_DVR_fw_version[20] = {0};	
u8 camera_rear_fw_version[20] = {0};	

u8 camera_rear_res[100] = {0};
u8 camera_DVR_res[100] = {0};

char tm_cmd[100] = {0};

static int isEmRecPermitted = 1, isVRLocked = 0;
static int delDays = 6, CVBSMode = 0, sensitivity_level = 1;

bool isOnlineRunning = false;

#if 0
//ioctl
#define READ_CAM_PROP(magic , nr) _IOR(magic, nr ,int) 
#define WRITE_CAM_PROP(magic , nr) _IOW(magic, nr ,int)
#endif

struct status_info s_info;

static void notify_online(int arg)
{
	lidbg(TAG"%s:====[%d]====\n",__func__,arg);
	pfly_UsbCamInfo->onlineNotify_status = arg;
	isOnlineNotifyReady = 1;
	wake_up_interruptible(&pfly_UsbCamInfo->onlineNotify_wait_queue);
	return;
}

static void notify_newDVR(unsigned char msg)
{
	unsigned char tmpval;
	lidbg(TAG"%s:====[%d]====\n",__func__,msg);
	//s_info = *info;
	//pfly_UsbCamInfo->newdvr_status = msg;
	tmpval = msg;
	isNewDvrNotifyReady = 1;

	down(&pfly_UsbCamInfo->notify_sem);
	
	if(kfifo_is_full(&notify_data_fifo))
    {
        u8 temp_reset_data;
        int tempbyte;
        //kfifo_reset(&knob_data_fifo);
        tempbyte = kfifo_out(&notify_data_fifo, &temp_reset_data, 1);
        lidbg(TAG"%s: kfifo clear!!!!!\n",__func__);
    }
	
	kfifo_in(&notify_data_fifo, &tmpval, 1);
	up(&pfly_UsbCamInfo->notify_sem);
	
	wake_up_interruptible(&pfly_UsbCamInfo->newdvr_wait_queue);
	return;
}

/******************************************************************************
 * Function: status_fifo_in
 * Description: Put status in fifo(for HAL),
 * 				For poll/read (except RET_DEFALUT,RET_START,RET_STOP , handles on our own).
 * Input parameters:
 *   status             - camera status
 * Return values:
 *      none
 * Notes: none
 *****************************************************************************/
static void status_fifo_in(unsigned char status)
{
	pfly_UsbCamInfo->read_status = status;
	if(status == RET_DEFALUT || status== RET_DVR_START || status == RET_DVR_STOP
		|| status== RET_REAR_START || status == RET_REAR_STOP)
	{
		lidbg(TAG"%s:====receive msg => %d====\n",__func__,status);
		wake_up_interruptible(&pfly_UsbCamInfo->camStatus_wait_queue);
		return;
	}
    
	lidbg(TAG"%s:====fifo in => 0x%x====\n",__func__,status);
	down(&pfly_UsbCamInfo->sem);
	//if(kfifo_is_full(&camStatus_data_fifo));
	kfifo_in(&camStatus_data_fifo, &pfly_UsbCamInfo->read_status, 1);
	up(&pfly_UsbCamInfo->sem);
	wake_up_interruptible(&pfly_UsbCamInfo->camStatus_wait_queue);
	return;
}

/******************************************************************************
 * Function: lidbg_flycam_event
 * Description: ACCON/ACCOFF event callback function
 * Input parameters:
 *   this             - notifier_block itself
 *   event          - event that triggered
 *   ptr              - private data
 * Return values:
 *      
 * Notes: none
 *****************************************************************************/
static int lidbg_flycam_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
    DUMP_FUN;
	lidbg(TAG"flycam event: %ld\n", event);
    switch (event)
    {
	    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
			lidbg(TAG"flycam event:resume %ld\n", event);
			s_info.isACCOFF = false;
			notify_newDVR(MSG_ACCON_NOTIFY);
			break;
	    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
			lidbg(TAG"flycam event:suspend %ld\n", event);
			s_info.isACCOFF = true;
			notify_newDVR(MSG_ACCOFF_NOTIFY);
			notify_online(RET_EM_ISREC_OFF);
			break;
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_UP):
			lidbg_shell_cmd("setprop lidbg.uvccam.isDelDaysFile 1");
			break;
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
			lidbg(TAG"flycam event:device_down %ld\n", event);
			break;
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_GSENSOR_STATUS_CHANGE, NOTIFIER_MINOR_EXCEED_THRESHOLD):
			lidbg(TAG"flycam event:emergency recording %ld\n", event);
			notify_newDVR(MSG_GSENSOR_NOTIFY);
			break;
	    default:
	        break;
    }

    return NOTIFY_DONE;
}

static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_flycam_event,
};


/******************************************************************************
 * Function: usb_nb_cam_func
 * Description: USB event callback function
 * Input parameters:
 *   this             - notifier_block itself
 *   event          - event that triggered
 *   ptr              - private data
 * Return values:
 *      
 * Notes: none
 *****************************************************************************/
static int usb_nb_cam_func(struct notifier_block *nb, unsigned long action, void *data)
{
	//struct usb_device *dev = data;
	switch (action)
	{
	case USB_DEVICE_ADD:
	case USB_DEVICE_REMOVE:
		notify_newDVR(MSG_USB_NOTIFY);
	    break;
	}
	return NOTIFY_OK;
}

static struct notifier_block usb_nb_cam =
{
    .notifier_call = usb_nb_cam_func,
};


static void work_format_done(struct work_struct *work)
{
	lidbg_shell_cmd("echo appcmd *158#097 > /dev/lidbg_drivers_dbg0");
	return;
}


/******************************************************************************
 * Function: checkSDCardStatus
 * Description: Check SDCard status & create file path.
 * Input parameters:
 *   	path	-	Record video files save path.
 * Return values:
 *		0	-	File path it's exist.
 *   	1	-	EMMC it's not exist.(unusual case)
 *   	2	-	SDCARD it's not exist.Change to EMMC default path.
 *   	3	-	Storage device is OK but the path you specify it's not exist,so create one.
 * Notes:  1.Check storage whether it is valid;
 *				2.Create file path if it is not exist.
 *****************************************************************************/
static int checkSDCardStatus(char *path)
{
	char temp_cmd[256];	
	int ret = 0;
	struct file *storage_path = NULL, *file_path = NULL;
	if(!strncmp(path, EMMC_MOUNT_POINT0, strlen(EMMC_MOUNT_POINT0)))
	{
		storage_path = filp_open(EMMC_MOUNT_POINT0, O_RDONLY | O_DIRECTORY, 0);
		file_path = filp_open(path, O_RDONLY | O_DIRECTORY, 0);
		if(IS_ERR(storage_path))
		{
			lidbg(TAG"%s:EMMC ERR!!%ld\n",__func__,PTR_ERR(storage_path));
			ret = 1;
		}
		else if(IS_ERR(file_path))
		{
			lidbg(TAG"%s: New Rec Dir => %s\n",__func__,path);
			sprintf(temp_cmd, "mkdir -p %s", path);
			lidbg_shell_cmd(temp_cmd);
			sprintf(temp_cmd, "mkdir -p %s/BlackBox/.tmp", path);
			lidbg_shell_cmd(temp_cmd);
			ret = 3;
		}
		else lidbg(TAG"%s: Check Rec Dir OK => %s\n",__func__,path);
	}
	else if(!strncmp(path, EMMC_MOUNT_POINT1, strlen(EMMC_MOUNT_POINT1)))
	{
		storage_path = filp_open(EMMC_MOUNT_POINT1, O_RDONLY | O_DIRECTORY, 0);
		file_path = filp_open(path, O_RDONLY | O_DIRECTORY, 0);
		if(IS_ERR(storage_path))
		{
#if 0
			lidbg(TAG"%s:SDCARD1 ERR!!Reset to %s/camera_rec/\n",__func__, EMMC_MOUNT_POINT0);
			strcpy(path, EMMC_MOUNT_POINT0"/camera_rec/");
#else
			lidbg(TAG"%s:SDCARD1 ERR!!\n",__func__);
			//strcpy(path, EMMC_MOUNT_POINT1"/camera_rec/");
#endif
			ret = 2;
		}
		else if(IS_ERR(file_path))
		{
			lidbg(TAG"%s: New Rec Dir => %s\n",__func__,path);
			sprintf(temp_cmd, "mkdir -p %s", path);
			lidbg_shell_cmd(temp_cmd);
			sprintf(temp_cmd, "mkdir -p %s/BlackBox/.tmp", path);
			lidbg_shell_cmd(temp_cmd);
			ret = 3;
		}
		else lidbg(TAG"%s: Check Rec Dir OK => %s\n",__func__,path);
	}
	if(!IS_ERR(storage_path)) filp_close(storage_path, 0);
	if(!IS_ERR(file_path)) filp_close(file_path, 0);
	return ret;
}


/******************************************************************************
 * Function: flycam_ioctl
 * Description: Flycam ioctl function.
 * Input parameters:
 *   	filp	-	file struct
 *   	cmd	-	Please refer to  lidbg_flycam_par.h magic code.
 *   	arg	-	arguments
 * Return values:
 *		Please refer to  lidbg_flycam_par.h -> cam_ioctl_ret_t.
 * Notes: 1.Check camera whether it is valid and sonix.
 				2.Get through the judgment and save args & start/stop recording.
 *****************************************************************************/
static long flycam_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	char ret = 0;
	unsigned char ret_st = 0;
	char temp_cmd[256];
	struct file *file_path;
	//lidbg(TAG"=====camStatus => %d======\n",pfly_UsbCamInfo->camStatus);
#if 0	
	if(_IOC_TYPE(cmd) == FLYCAM_FRONT_REC_IOC_MAGIC)//front cam recording mode
	{
		if((_IOC_NR(cmd) == NR_START_REC) || (_IOC_NR(cmd) == NR_STOP_REC) )
		{
			/*check camera status before doing ioctl*/
			if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
			{
				lidbg(TAG"%s:DVR not found,ioctl fail!\n",__func__);
				return RET_NOTVALID;
			}
			else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
			{
				lidbg(TAG"%s:Is not SonixCam ,ioctl fail!\n",__func__);
				return RET_NOTSONIX;
			}
			else if((pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRAfterFix)
			{
				lidbg(TAG"%s:Fix proc running!But ignore !\n",__func__);
				//return RET_NOTVALID;
				isDVRAfterFix = 1;//force to 1 tmp
			}
		}
		
		switch(_IOC_NR(cmd))
		{
			case NR_BITRATE:
				lidbg(TAG"%s:DVR NR_REC_BITRATE = [%ld]\n",__func__,arg);
				f_rec_bitrate = arg;
		        break;
		    case NR_RESOLUTION:
				lidbg(TAG"%s:DVR NR_REC_RESOLUTION  = [%s]\n",__func__,(char*)arg);
				strcpy(f_rec_res,(char*)arg);
		        break;
			case NR_PATH:
				lidbg(TAG"%s:DVR NR_REC_PATH  = [%s]\n",__func__,(char*)arg);
#if 0				
				ret_st = checkSDCardStatus((char*)arg);
				if(ret_st != 1) 
					strcpy(f_rec_path,(char*)arg);
				else
					lidbg(TAG"%s: f_rec_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) ret = RET_FAIL;
#endif				
		        break;
			case NR_TIME:
				lidbg(TAG"%s:DVR NR_REC_TIME = [%ld]\n",__func__,arg);
				f_rec_time = arg;
		        break;
			case NR_FILENUM:
				lidbg(TAG"%s:DVR NR_REC_FILENUM = [%ld]\n",__func__,arg);
				f_rec_filenum = arg;
		        break;
			case NR_TOTALSIZE:
				lidbg(TAG"%s:DVR NR_REC_TOTALSIZE = [%ld]\n",__func__,arg);
				f_rec_totalsize= arg;
		        break;
			case NR_ISDUALCAM:
				lidbg(TAG"%s:DVR NR_ISDUALCAM\n",__func__);
				isDualCam = arg;
		        break;
			case NR_ISCOLDBOOTREC:
				lidbg(TAG"%s:DVR NR_ISCOLDBOOTREC\n",__func__);
				isColdBootRec= arg;
		        break;
			case NR_ISEMPERMITTED:
				lidbg(TAG"%s:DVR NR_ISEMPERMITTED\n",__func__);
				isEmRecPermitted= arg;
		        break;
			case NR_ISVIDEOLOOP:
				lidbg(TAG"%s:DVR NR_ISVIDEOLOOP %ld\n",__func__,arg);
				isDVRVideoLoop= arg;
				break;
			case NR_DELDAYS:
				lidbg(TAG"%s:DVR NR_DELDAYS %ld\n",__func__,arg);
				delDays= arg;
				break;
			case NR_START_REC:
		        lidbg(TAG"%s:DVR NR_START_REC\n",__func__);
				setDVRProp(DVR_ID);
				ret_st = checkSDCardStatus(f_rec_path);
				if(ret_st == 2 || ret_st == 1) return RET_FAIL;
				
				if(isDVRRec)
				{
					lidbg(TAG"%s:====DVR start cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
				else if(isOnlineRec) 
				{
					lidbg(TAG"%s:====DVR restart rec====\n",__func__);
					isDVRRec = 1;
					isOnlineRec = 0;
					if(stop_rec(DVR_ID,1)) goto dvrfailproc;
					if(start_rec(DVR_ID,1)) goto dvrfailproc;
					notify_online(RET_ONLINE_INTERRUPTED);
				}
				else 
				{
					lidbg(TAG"%s:====DVR start rec====\n",__func__);
					isDVRRec = 1;
					if(start_rec(DVR_ID,1)) goto dvrfailproc;
				}
		        break;
			case NR_STOP_REC:
		        lidbg(TAG"%s:DVR NR_STOP_REC\n",__func__);
				if(isDVRRec)
				{
					lidbg(TAG"%s:====DVR stop rec====\n",__func__);
					if(stop_rec(DVR_ID,1)) goto dvrfailproc;
					isDVRRec = 0;
				}
				else if(isOnlineRec) 
				{
					lidbg(TAG"%s:====DVR stop cmd neglected====\n",__func__);
					ret = RET_IGNORE;
				}
				else
				{
					lidbg(TAG"%s:====DVR stop cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
		        break;
			case NR_SET_PAR:
				lidbg(TAG"%s:DVR NR_SET_PAR\n",__func__);
				if(isDVRRec)
				{
					if(stop_rec(DVR_ID,1)) goto dvrfailproc;
					setDVRProp(DVR_ID);
					//msleep(500);
					if(start_rec(DVR_ID,1)) goto dvrfailproc;
				}
				if(isRearRec)
				{
					if(stop_rec(REARVIEW_ID,1)) goto rearfailproc;
					setDVRProp(REARVIEW_ID);
					//msleep(500);
					if(start_rec(REARVIEW_ID,1)) goto rearfailproc;
				}
		        break;
			case NR_GET_RES:
				lidbg(TAG"%s:DVR NR_GET_RES\n",__func__);
				//invoke_AP_ID_Mode(DVR_GET_RES_ID_MODE);
				if(!wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ)) ret = RET_FAIL;
				strcpy((char*)arg,camera_DVR_res);
				lidbg(TAG"%s:DVR NR_GET_RES => %s\n",__func__,(char*)arg);
		        break;
			case NR_SATURATION:
				lidbg(TAG"%s:DVR NR_SATURATION\n",__func__);
				lidbg(TAG"saturationVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set saturation=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_TONE:
				lidbg(TAG"%s:DVR NR_TONE\n",__func__);
				lidbg(TAG"hueVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set hue=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_BRIGHT:
				lidbg(TAG"%s:DVR NR_BRIGHT\n",__func__);
				lidbg(TAG"brightVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set bright=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_CONTRAST:
				lidbg(TAG"%s:DVR NR_CONTRAST\n",__func__);
				lidbg(TAG"contrastVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set contrast=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
		    default:
		        return -ENOTTY;
		}
	}
#endif	
	if(_IOC_TYPE(cmd) == FLYCAM_FRONT_ONLINE_IOC_MAGIC)//front cam online mode
	{
		char temp_cmd[256];
		if((s_info.isACCOFF == true) && ((_IOC_NR(cmd) == NR_START_REC) ||(_IOC_NR(cmd) == NR_CAPTURE)))
		{
			lidbg(TAG"%s:====Online VR: udisk_request===\n",__func__);
			lidbg_shell_cmd("echo 'udisk_request' > /dev/flydev0");
			lidbg(TAG"%s:====Online VR: ACCOFF CHECK====\n",__func__);
			if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->DVR_ready_wait_queue, (s_info.isFrontCamReady == true), 10*HZ))
			{
				lidbg(TAG"%s:====Online VR: udisk_unrequest==suspend online timeout==\n",__func__);
				lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
				return RET_NOTVALID;
			}
		}
#if 0		
		if(!isSuspend)
		{
			/*check camera status before doing ioctl*/
			if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
			{
				lidbg(TAG"%s:DVR[online] not found,ioctl fail!\n",__func__);
				return RET_NOTVALID;
			}
			if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
			{
				lidbg(TAG"%s:is not SonixCam ,ioctl fail!\n",__func__);
				return RET_NOTSONIX;
			}
		}
#endif		
		switch(_IOC_NR(cmd))
		{
			case NR_BITRATE:
				lidbg(TAG"%s:Online NR_REC_BITRATE = [%ld]\n",__func__,arg);
				f_online_bitrate = arg;
		        break;
		    case NR_RESOLUTION:
				lidbg(TAG"%s:Online NR_REC_RESOLUTION  = [%s]\n",__func__,(char*)arg);
				strcpy(f_online_res,(char*)arg);
		        break;
			case NR_PATH:
				lidbg(TAG"%s:Online NR_REC_PATH  = [%s]\n",__func__,(char*)arg);
				/*
				ret_st = checkSDCardStatus((char*)arg);
				if(ret_st != 1) 
					strcpy(f_online_path,(char*)arg);
				else
					lidbg(TAG"%s: f_online_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) ret = RET_FAIL;
				*/
				strcpy(f_online_path,(char*)arg);
		        break;
			case NR_TIME:
				lidbg(TAG"%s:Online NR_REC_TIME = [%ld]\n",__func__,arg);
				f_online_time = arg;
		        break;
			case NR_FILENUM:
				lidbg(TAG"%s:Online NR_REC_FILENUM = [%ld]\n",__func__,arg);
				f_online_filenum = arg;
		        break;
			case NR_TOTALSIZE:
				lidbg(TAG"%s:Online NR_REC_TOTALSIZE = [%ld]\n",__func__,arg);
				f_online_totalsize= arg;
		        break;
			case NR_START_REC:
		        lidbg(TAG"%s:Online NR_START_REC\n",__func__);
				if(s_info.isACCOFF == true) 
				{
					if(isOnlineRunning == false)
					{
						mod_timer(&suspend_stoprec_timer,SUSPEND_STOPREC_ONLINE_TIME);
						notify_newDVR(MSG_START_ONLINE_VR_NOTIFY);
						isOnlineRunning = true;
					}
					else return RET_REPEATREQ;
				}
				else return RET_IGNORE;
		        break;
			case NR_STOP_REC:
		        lidbg(TAG"%s:Online NR_STOP_REC\n",__func__);
				if(s_info.isACCOFF == true) 
				{		
					if(isOnlineRunning == true)
					{
						notify_newDVR(MSG_STOP_ONLINE_VR_NOTIFY);
						del_timer(&suspend_stoprec_timer);
						lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
						isOnlineRunning = false;
					}
					else return RET_REPEATREQ;
				}
				else return RET_IGNORE;
		        break;
			case NR_CAPTURE:
				lidbg(TAG"%s:Online NR_CAPTURE\n",__func__);
				lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video0 -b 1 -c -f mjpg -S");
				break;
			case NR_CAPTURE_PATH:
				lidbg(TAG"%s:Online NR_CAPTURE_PATH  = [%s]\n",__func__,(char*)arg);
				sprintf(temp_cmd, "mkdir -p %s", (char*)arg);
				lidbg_shell_cmd(temp_cmd);

				file_path = filp_open((char*)arg, O_RDONLY | O_DIRECTORY, 0);
				if(IS_ERR(file_path))
				{
					lidbg(TAG"%s:CAPTURE_PATH ERR!!\n",__func__);
					lidbg_shell_cmd("setprop persist.uvccam.capturepath "EMMC_MOUNT_POINT0"/preview_cache/");
				}
				else
				{
					lidbg(TAG"%s:CAPTURE_PATH OK!!\n",__func__);
					strcpy(capture_path,(char*)arg);
					sprintf(temp_cmd, "setprop persist.uvccam.capturepath %s", capture_path);
					lidbg_shell_cmd(temp_cmd);
				}
				if(!IS_ERR(file_path)) filp_close(file_path, 0);
		        break;
		    default:
		        return -ENOTTY;
		}
	}
	else if(_IOC_TYPE(cmd) == FLYCAM_REAR_ONLINE_IOC_MAGIC)//rear cam online mode
	{
		switch(_IOC_NR(cmd))
		{
			case NR_CAPTURE:
				lidbg(TAG"%s:Online NR_CAPTURE\n",__func__);
				lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video0 -b 0  -c -f mjpg -S");
				break;
			default:
		        return -ENOTTY;
		}
	}
#if 0
	if(_IOC_TYPE(cmd) == FLYCAM_REAR_REC_IOC_MAGIC)//front cam recording mode
	{
		if((_IOC_NR(cmd) == NR_START_REC) || (_IOC_NR(cmd) == NR_STOP_REC) )
		{
			/*check camera status before doing ioctl*/
			if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
			{
				lidbg(TAG"%s:Rear not found,ioctl fail!\n",__func__);
				return RET_NOTVALID;
			}
			else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
			{
				lidbg(TAG"%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
				return RET_NOTSONIX;
			}
			else if(((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX) && !isRearViewAfterFix)
			{
				lidbg(TAG"%s:Rear Fix proc running!But ignore !\n",__func__);
				//return RET_NOTVALID;
				isRearViewAfterFix = 1;//force to 1 tmp
			}
		}
		
		switch(_IOC_NR(cmd))
		{
			case NR_BITRATE:
				lidbg(TAG"%s:Rear NR_REC_BITRATE = [%ld]\n",__func__,arg);
				r_rec_bitrate = arg;
		        break;
		    case NR_RESOLUTION:
				lidbg(TAG"%s:Rear NR_REC_RESOLUTION  = [%s]\n",__func__,(char*)arg);
				strcpy(r_rec_res,(char*)arg);
		        break;
			case NR_PATH:
				lidbg(TAG"%s:Rear NR_REC_PATH  = [%s]\n",__func__,(char*)arg);
#if 0				
				ret_st = checkSDCardStatus((char*)arg);
				if(ret_st != 1) 
					strcpy(r_rec_path,(char*)arg);
				else
					lidbg(TAG"%s: r_rec_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) ret = RET_FAIL;
#endif				
		        break;
			case NR_TIME:
				lidbg(TAG"%s:Rear NR_REC_TIME = [%ld]\n",__func__,arg);
				r_rec_time = arg;
		        break;
			case NR_FILENUM:
				lidbg(TAG"%s:Rear NR_REC_FILENUM = [%ld]\n",__func__,arg);
				r_rec_filenum = arg;
		        break;
			case NR_TOTALSIZE:
				lidbg(TAG"%s:Rear NR_REC_TOTALSIZE = [%ld]\n",__func__,arg);
				r_rec_totalsize= arg;
		        break;
			case NR_ISVIDEOLOOP:
				lidbg(TAG"%s:Rear NR_ISVIDEOLOOP %ld\n",__func__,arg);
				isRearVideoLoop= arg;
				break;
			case NR_CVBSMODE:
				lidbg(TAG"%s:REAR NR_CVBSMODE %ld\n",__func__,arg);
				CVBSMode = arg;
				break;
			case NR_START_REC:
		        lidbg(TAG"%s:Rear NR_START_REC\n",__func__);
				setDVRProp(REARVIEW_ID);
				ret_st = checkSDCardStatus(f_rec_path);
				if(ret_st == 2 || ret_st == 1) return RET_FAIL;
				if(isRearRec)
				{
					lidbg(TAG"%s:====Rear start cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
				else 
				{
					lidbg(TAG"%s:====Rear start rec====\n",__func__);
					isRearRec = 1;
					if(start_rec(REARVIEW_ID,1)) goto rearfailproc;
				}
		        break;
			case NR_STOP_REC:
		        lidbg(TAG"%s:Rear NR_STOP_REC\n",__func__);
				if(isRearRec)
				{
					lidbg(TAG"%s:====Rear stop rec====\n",__func__);
					if(stop_rec(REARVIEW_ID,1)) goto rearfailproc;
					isRearRec = 0;
				}
				else
				{
					lidbg(TAG"%s:====Rear stop cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
		        break;
			case NR_SET_PAR:
				lidbg(TAG"%s:Rear NR_SET_PAR\n",__func__);
				if(isRearRec)
				{
					if(stop_rec(REARVIEW_ID,1)) goto rearfailproc;
					setDVRProp(REARVIEW_ID);
					if(start_rec(REARVIEW_ID,1)) goto rearfailproc;
				}
		        break;
			case NR_GET_RES:
				lidbg(TAG"%s:Rear NR_GET_RES\n",__func__);
				//invoke_AP_ID_Mode(REAR_GET_RES_ID_MODE);
				if(!wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ)) ret = RET_FAIL;
				strcpy((char*)arg,camera_rear_res);
				lidbg(TAG"%s:Rear NR_GET_RES => %s\n",__func__,(char*)arg);
		        break;
			case NR_SATURATION:
				lidbg(TAG"%s:Rear NR_SATURATION\n",__func__);
				lidbg(TAG"saturationVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set saturation=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_TONE:
				lidbg(TAG"%s:Rear NR_TONE\n",__func__);
				lidbg(TAG"hueVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set hue=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_BRIGHT:
				lidbg(TAG"%s:Rear NR_BRIGHT\n",__func__);
				lidbg(TAG"brightVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set bright=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_CONTRAST:
				lidbg(TAG"%s:Rear NR_CONTRAST\n",__func__);
				lidbg(TAG"contrastVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set contrast=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
		    default:
		        return -ENOTTY;
		}
	}
#endif	
	if(_IOC_TYPE(cmd) == FLYCAM_STATUS_IOC_MAGIC)//Rec TestAp status
	{
		switch(_IOC_NR(cmd))
		{
			case NR_STATUS:
		        lidbg(TAG"%s:NR_STATUS\n",__func__);
				/*Check Update proc*/
				if(arg == RET_DVR_UD_SUCCESS || arg == RET_DVR_UD_FAIL ||arg == RET_DVR_FW_ACCESS_FAIL ||
					arg == RET_REAR_UD_SUCCESS ||arg == RET_REAR_UD_FAIL ||arg == RET_REAR_FW_ACCESS_FAIL )
					isUpdating = 0;
				status_fifo_in(arg);
		        break;
			case NR_ACCON_CAM_READY:/*R/W:	R[0-OK;1-Timout]	W[0-Rear;1-DVR] -> Wait for 10s*/
		        lidbg(TAG"%s:NR_ACCON_CAM_READY  E cam_id=> %ld\n",__func__,arg);
				if(arg == DVR_ID)
				{
					lidbg(TAG"DVR_ready_wait waiting\n");
					if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
					{
						//if(!wait_for_completion_timeout(&DVR_ready_wait , 10*HZ)) ret = 1;
						if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->DVR_ready_wait_queue, (isDVRReady == 1), 10*HZ))
							ret = 1;
					}
					else lidbg(TAG"DVR Camera already ready.\n");
				}
				else if(arg == REARVIEW_ID)
				{
					lidbg(TAG"Rear_ready_wait waiting\n");
					if(!((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX))
					{
						if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->Rear_ready_wait_queue, (isRearReady == 1), 10*HZ))
							ret = 1;
					}
					else lidbg(TAG"Rear Camera already ready.\n");
				}
				 lidbg(TAG"%s:NR_ACCON_CAM_READY  X \n",__func__);
		        break;
			case NR_DVR_FW_VERSION:
		        lidbg(TAG"%s:NR_DVR_FW_VERSION\n",__func__);
				strcpy(camera_DVR_fw_version,(char*)arg);
				sprintf(temp_cmd, "am broadcast -a com.lidbg.flybootserver.action --es toast DVR_%s&",camera_DVR_fw_version );
				lidbg_shell_cmd(temp_cmd);
				complete(&DVR_fw_get_wait);/*HAL get version*/
		        break;
			case NR_REAR_FW_VERSION:
		        lidbg(TAG"%s:NR_REAR_FW_VERSION\n",__func__);
				strcpy(camera_rear_fw_version,(char*)arg);
				sprintf(temp_cmd, "am broadcast -a com.lidbg.flybootserver.action --es toast REAR_%s&",camera_rear_fw_version );
				lidbg_shell_cmd(temp_cmd);
				complete(&Rear_fw_get_wait);/*HAL get version*/
		        break;
			case NR_DVR_RES:
		        lidbg(TAG"%s:NR_DVR_RES\n",__func__);
				strcpy(camera_DVR_res,(char*)arg);
				complete(&DVR_res_get_wait);/*HAL get version*/
		        break;
			case NR_REAR_RES:
		        lidbg(TAG"%s:NR_REAR_RES\n",__func__);
				strcpy(camera_rear_res,(char*)arg);
				complete(&Rear_res_get_wait);/*HAL get version*/
		        break;
			case NR_ONLINE_NOTIFY:
				//wait_event_interruptible(pfly_UsbCamInfo->onlineNotify_wait_queue, isOnlineNotifyReady == 1);
				 if(wait_event_interruptible(pfly_UsbCamInfo->onlineNotify_wait_queue, isOnlineNotifyReady == 1))
	        		return -ERESTARTSYS;
				lidbg(TAG"%s:NR_ONLINE_NOTIFY ,isOnlineNotifyReady: %d\n",__func__,isOnlineNotifyReady);
				isOnlineNotifyReady = 0;
				return pfly_UsbCamInfo->onlineNotify_status;
		        break;
			case NR_ONLINE_INVOKE_NOTIFY:
		        lidbg(TAG"%s:NR_ONLINE_INVOKE_NOTIFY arg = %ld\n",__func__,arg);
				if(arg == RET_EM_ISREC_OFF) del_timer(&stop_thinkware_em_timer);
				notify_online(arg);
		        break;
			case NR_NEW_DVR_NOTIFY:
				if(kfifo_is_empty(&notify_data_fifo))
			    {
			        if(wait_event_interruptible(pfly_UsbCamInfo->newdvr_wait_queue, !kfifo_is_empty(&notify_data_fifo)))
			            return -ERESTARTSYS;
			    }
				down(&pfly_UsbCamInfo->notify_sem);
				ret = kfifo_out(&notify_data_fifo, &notify_data_buf, 1);
				up(&pfly_UsbCamInfo->notify_sem);
				/*
				 if(wait_event_interruptible(pfly_UsbCamInfo->newdvr_wait_queue, isNewDvrNotifyReady == 1))
	        		return -ERESTARTSYS;
	        	*/
				lidbg(TAG"%s:NR_NEW_DVR_NOTIFY ,%d, isNewDvrNotifyReady: %d\n",__func__,notify_data_buf, isNewDvrNotifyReady);

				if(copy_to_user((char*)arg, (char*)&s_info, sizeof(struct status_info)))
			     {
			         lidbg(TAG"%s:copy_to_user ERR\n",__func__);
			     }
				
				isNewDvrNotifyReady = 0;
				return notify_data_buf;
		        break;
			case NR_NEW_DVR_IO:
				lidbg(TAG"%s:NR_NEW_DVR_IO  \n",__func__ );
				s_info.emergencySaveDays = ((struct status_info*)arg)->emergencySaveDays;
				s_info.emergencySwitch= ((struct status_info*)arg)->emergencySwitch;
				s_info.recordMode= ((struct status_info*)arg)->recordMode;
				s_info.recordSwitch= ((struct status_info*)arg)->recordSwitch;
				s_info.singleFileRecordTime= ((struct status_info*)arg)->singleFileRecordTime;
				s_info.sensitivityLevel= ((struct status_info*)arg)->sensitivityLevel;
				lidbg(TAG"%s:NR_NEW_DVR_IO %d  \n",__func__, s_info.recordSwitch);
				return 0;
		        break;		
			case NR_NEW_DVR_ASYN_NOTIFY:
				lidbg(TAG"%s:NR_NEW_DVR_ASYN_NOTIFY  \n",__func__ );
				if(arg == RET_DVR_SONIX)
				{
					s_info.isFrontCamReady = true;
					g_var.dvr_cam_ready = 1;
					wake_up_interruptible(&pfly_UsbCamInfo->DVR_ready_wait_queue);
				}
				else if(arg == RET_DVR_DISCONNECT)
				{
					s_info.isFrontCamReady = false;
					g_var.dvr_cam_ready = 0;
				}

				if(arg == RET_REAR_SONIX)
				{
					s_info.isRearCamReady = true;
					g_var.rear_cam_ready = 1;
					wake_up_interruptible(&pfly_UsbCamInfo->Rear_ready_wait_queue);
				}
				else if(arg == RET_REAR_DISCONNECT)
				{
					s_info.isRearCamReady = false;
					g_var.rear_cam_ready = 0;
				}

				status_fifo_in(arg);	
				return 0;
		        break;
			case NR_ENABLE_CAM_POWER:
				//lidbg(TAG"%s:NR_ENABLE_CAM_POWER  \n",__func__ );
				//USB_FRONT_WORK_ENABLE;
				//USB_BACK_WORK_ENABLE;
				return 0;
		        break;
			case NR_DISABLE_CAM_POWER:
				lidbg(TAG"%s:NR_DISABLE_CAM_POWER  \n",__func__ );
				USB_POWER_FRONT_DISABLE;
				USB_POWER_BACK_DISABLE;
				return 0;
		        break;
			case NR_ENABLE_FRONT_CAM_POWER:
				lidbg(TAG"%s:NR_ENABLE_FRONT_CAM_POWER  \n",__func__ );
				USB_FRONT_WORK_ENABLE;
				return 0;
		        break;
			case NR_DISABLE_FRONT_CAM_POWER:
				lidbg(TAG"%s:NR_DISABLE_FRONT_CAM_POWER  \n",__func__ );
				USB_POWER_FRONT_DISABLE;
				return 0;
		        break;
			case NR_ENABLE_REAR_CAM_POWER:
				lidbg(TAG"%s:NR_ENABLE_REAR_CAM_POWER  \n",__func__ );
				USB_BACK_WORK_ENABLE;
				return 0;
		        break;
			case NR_DISABLE_REAR_CAM_POWER:
				lidbg(TAG"%s:NR_DISABLE_REAR_CAM_POWER  \n",__func__ );
				USB_POWER_BACK_DISABLE;
				return 0;
		        break;
			case NR_CONN_SDCARD:
				lidbg(TAG"%s:NR_CONN_SDCARD  \n",__func__ );
				s_info.isSDCardReady = true;
				return 0;
		        break;
			case NR_DISCONN_SDCARD:
				lidbg(TAG"%s:NR_DISCONN_SDCARD  \n",__func__ );
				s_info.isSDCardReady = false;
				return 0;
		        break;
			case NR_VOLD_DISCONN_SDCARD:
				lidbg(TAG"%s:NR_VOLD_DISCONN_SDCARD  \n",__func__ );		
				notify_newDVR(MSG_VOLD_SD_REMOVE);
				return 0;
		        break;
			default:
		        return -ENOTTY;
		}
	}
	else if(_IOC_TYPE(cmd) == FLYCAM_FW_IOC_MAGIC)//front cam online mode
	{
		switch(_IOC_NR(cmd))
		{
			case NR_VERSION:
		        lidbg(TAG"%s:NR_VERSION cam_id = [%d]\n",__func__,((char*)arg)[0]);
				strcpy((char*)arg + 1,"123456");
				if(((char*)arg)[0] == DVR_ID)
					strcpy((char*)arg + 1,camera_DVR_fw_version);
				else if(((char*)arg)[0]== REARVIEW_ID)
					strcpy((char*)arg + 1,camera_rear_fw_version);
				else strcpy((char*)arg + 1,camera_DVR_fw_version);

				if(!strncmp((char*)arg + 1, "NONE", 4))
					ret = RET_FAIL;

				lidbg(TAG"%s:NR_VERSION FwVer:[%s]\n",__func__,(char*)arg + 1);

#if 0
				if(arg == 0)
				{
					isRearCheck = 0;
					lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -2&");
				}
				else if(arg == 1)
				{
					isDVRCheck = 0;
					lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -3&");
				}
				else 
				{
					isDVRCheck = 0;
					lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -3&");
				}
				if(!wait_for_completion_timeout(&fw_get_wait , 3*HZ)) ret = 1;
				strcpy((char*)arg,camera_fw_version);
				msleep(1500);
				isRearCheck = 1;
				isDVRCheck = 1;
#endif
		        break;
			case NR_UPDATE:
		        lidbg(TAG"%s:NR_UPDATE\n",__func__);
				if(isUpdating) 
				{
					lidbg(TAG"%s:Camera it's updating!Rejective.\n",__func__);
					return RET_IGNORE;
				}
				isUpdating = 1;
				if(arg == 0)
					lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -0&");
				else if(arg == 1)
					lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -1&");
				else ret = RET_FAIL;
		        break;
			default:
		        return -ENOTTY;
		}
	}
	else if(_IOC_TYPE(cmd) == FLYCAM_REC_MAGIC)//front cam online mode
	{
		//int rc = -1;
		unsigned char dvrRespond[100] = {0};
		unsigned char rearRespond[100] = {0};
		unsigned char returnRespond[200] = {0};
		unsigned char initMsg[400] = {0};
		char tmp_path[200] = {0};
		int length = 0;
		//struct mounted_volume *sdcard1 = NULL;
		
		dvrRespond[0] = ((char*)arg)[0];
		rearRespond[0] = ((char*)arg)[0];
		dvrRespond[1] = DVR_ID;
		rearRespond[1] = REARVIEW_ID;		
		switch(_IOC_NR(cmd))
		{
			case NR_CMD:
		        lidbg(TAG"%s:NR_CMD__cmd=>[0x%x]__camID=>[0x%x]\n",__func__,((char*)arg)[0],((char*)arg)[1]);
				//msleep(100);
				/*check dvr camera status before doing ioctl*/
#if 0				
				if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
				{
					lidbg(TAG"%s:DVR not found,ioctl fail!\n",__func__);
					//dvrRespond[2] = RET_NOTVALID;
				}
				else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
				{
					lidbg(TAG"%s:Is not SonixCam ,ioctl fail!\n",__func__);
					//dvrRespond[2] = RET_NOTSONIX;
				}


				/*check rear camera status before doing ioctl*/
				if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
				{
					lidbg(TAG"%s:Rear not found,ioctl fail!\n",__func__);
					//rearRespond[2] = RET_NOTVALID;
				}
				else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
				{
					lidbg(TAG"%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
					//rearRespond[2] = RET_NOTSONIX;
				}
#endif
				if(s_info.isFrontCamReady == false)
				{
					lidbg(TAG"%s:DVR not found,ioctl fail!\n",__func__);
					dvrRespond[2] = RET_NOTVALID;
				}

				if(s_info.isRearCamReady == false)
				{
					lidbg(TAG"%s:Rear not found,ioctl fail!\n",__func__);
					rearRespond[2] = RET_NOTVALID;
				}

				switch(((char*)arg)[0])
				{
					case CMD_RECORD:
						if( ((char*)arg)[1] == 1)
							s_info.recordSwitch = K_RECORD_START;
						else s_info.recordSwitch = K_RECORD_STOP;
						
						if(s_info.recordSwitch == K_RECORD_START)
						{
							if(s_info.recordMode == K_RECORD_DUAL_MODE)
							{
								if(s_info.isFrontCamReady == true)	dvrRespond[3] = 1;
								else	dvrRespond[3] = 0;
								if(s_info.isRearCamReady == true)	rearRespond[3] = 1;
								else	rearRespond[3] = 0;							
							}
							else
							{
								if(s_info.isFrontCamReady == true)	dvrRespond[3] = 1;
								else	dvrRespond[3] = 0;								
								rearRespond[3] = 0;
							}
						}
						else if(s_info.recordSwitch == K_RECORD_STOP)
						{
							dvrRespond[3] = 0;
							rearRespond[3] = 0;
						}

						notify_newDVR(MSG_RECORD_SWITCH);
						
						memcpy(returnRespond + length,dvrRespond,4);
						length += 4;
						memcpy(returnRespond + length,rearRespond,4);
						length += 4;
						if(copy_to_user((char*)arg,returnRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_CAPTURE:
						lidbg(TAG"%s:====CMD_CAPTURE====\n",__func__);
						length += 2;
						break;

					case CMD_SET_RESOLUTION:
						lidbg(TAG"%s:CMD_SET_RESOLUTION  = [%s]\n",__func__,(char*)arg + 1);
						strcpy(f_rec_res,(char*)arg + 1);
						length += 100;
						memcpy(dvrRespond + 3,(char*)arg + 1,length);
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_TIME_SEC:
						lidbg(TAG"%s:CMD_TIME_SEC = [%d]\n",__func__,(((char*)arg)[1] << 8) + ((char*)arg)[2]);
						f_rec_time = (((char*)arg)[1] << 8) + ((char*)arg)[2];
						
						s_info.singleFileRecordTime = f_rec_time/60;
						notify_newDVR(MSG_SINGLE_FILE_RECORD_TIME);
						
						memcpy(dvrRespond + 3,(char*)arg + 1,2);
						length += 5;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_FW_VER:
						lidbg(TAG"%s:CMD_FW_VER dvr_Fw=> %s\n",__func__,camera_DVR_fw_version);
						strcpy(dvrRespond + 3,camera_DVR_fw_version);
						length += 100;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_TOTALSIZE:
						lidbg(TAG"%s:CMD_TOTALSIZE\n",__func__);
						f_rec_totalsize= ((((char*)arg)[1] << 24) + (((char*)arg)[2] << 16) + (((char*)arg)[3] << 8) + ((char*)arg)[4]);
						lidbg(TAG"%s:CMD_TOTALSIZE => %dMB\n",__func__,f_rec_totalsize);
						memcpy(dvrRespond + 3,(char*)arg + 1,4);
						length += 7;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_PATH:
						lidbg(TAG"%s:CMD_PATH\n",__func__);
						ret_st = checkSDCardStatus(EMMC_MOUNT_POINT1"/camera_rec/");
						if(ret_st != 1) 
							strcpy(f_rec_path,EMMC_MOUNT_POINT1"/camera_rec/");
						else
							lidbg(TAG"%s: f_rec_path access wrong! %d", __func__ ,EFAULT);//not happend
						if(ret_st > 0) dvrRespond[2] = RET_FAIL;

						/*EM path*/
						sprintf(tmp_path, "%s/BlackBox/", f_rec_path);
						sprintf(temp_cmd, "mkdir -p %s", tmp_path);
						lidbg_shell_cmd(temp_cmd);
						sprintf(temp_cmd, "mkdir -p %s/.tmp", tmp_path);
						lidbg_shell_cmd(temp_cmd);

						file_path = filp_open(tmp_path, O_RDONLY | O_DIRECTORY, 0);
						if(IS_ERR(file_path))
						{
							lidbg(TAG"%s:EM_PATH ERR!!\n",__func__);
							lidbg_shell_cmd("setprop persist.uvccam.empath "EMMC_MOUNT_POINT1"/camera_rec/BlackBox/");
						}
						else
						{
							lidbg(TAG"%s:EMPATH OK!!\n",__func__);
							strcpy(em_path,tmp_path);
							sprintf(temp_cmd, "setprop persist.uvccam.empath %s", em_path);
							lidbg_shell_cmd(temp_cmd);
						}
						if(!IS_ERR(file_path)) filp_close(file_path, 0);
						
						strcpy(dvrRespond + 3,f_rec_path);
						length += 100;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_SET_PAR:
						lidbg(TAG"%s:CMD_SET_PAR\n",__func__);						
						initMsg[length] = 0xB0;
						length++;
						dvrRespond[0] = CMD_RECORD;
						rearRespond[0] = CMD_RECORD;
						dvrRespond[3] = s_info.recordSwitch;
						rearRespond[3] = 0;
						
						memcpy(initMsg + length,dvrRespond,4);
						length += 4;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,4);
						length += 4;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						f_rec_time = s_info.singleFileRecordTime * 60;

						dvrRespond[0] = CMD_TIME_SEC;
						rearRespond[0] = CMD_TIME_SEC;
						dvrRespond[3] = f_rec_time >> 8;
						dvrRespond[4] = f_rec_time;
						rearRespond[3] = f_rec_time >> 8;
						rearRespond[4] = f_rec_time;
						
						memcpy(initMsg + length,dvrRespond,5);
						length += 5;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,5);
						length += 5;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						dvrRespond[0] = CMD_TOTALSIZE;
						rearRespond[0] = CMD_TOTALSIZE;
						dvrRespond[3] = f_rec_totalsize >> 24;
						dvrRespond[4] = f_rec_totalsize >> 16;
						dvrRespond[5] = f_rec_totalsize >> 8;
						dvrRespond[6] = f_rec_totalsize;
						rearRespond[3] = f_rec_totalsize >> 24;
						rearRespond[4] = f_rec_totalsize >> 16;
						rearRespond[5] = f_rec_totalsize >> 8;
						rearRespond[6] = f_rec_totalsize;
						
						memcpy(initMsg + length,dvrRespond,7);
						length += 7;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,7);
						length += 7;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						dvrRespond[0] = CMD_PATH;
						rearRespond[0] = CMD_PATH;
						checkSDCardStatus(f_rec_path);
						memcpy(dvrRespond + 3,f_rec_path,60);
						memcpy(rearRespond + 3,f_rec_path,60);
						
						memcpy(initMsg + length,dvrRespond,63);
						length += 63;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,63);
						length += 63;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_DUAL_CAM;
						length++;
						initMsg[length] = s_info.recordMode;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						lidbg(TAG"%s:CMD_SET_PAR ;1:0x%x,2:0x%x,3:0x%x,4:0x%x,5:0x%x,6:0x%x,7:0x%x,\n",__func__,initMsg[0],initMsg[1],initMsg[2],initMsg[3],initMsg[4],initMsg[5],initMsg[6]);
						lidbg(TAG"%s:length = %d\n",__func__,length);
						if(copy_to_user((char*)arg,initMsg,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						//mod_timer(&set_par_timer,SET_PAR_WAIT_TIME);
						break;

					case CMD_GET_RES:
						lidbg(TAG"%s:CMD_GET_RES\n",__func__);
						//invoke_AP_ID_Mode(DVR_GET_RES_ID_MODE);
						if(!wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ)) dvrRespond[2] = RET_FAIL;
						strcpy(dvrRespond + 3,camera_DVR_res);
						lidbg(TAG"%s:DVR NR_GET_RES => %s\n",__func__,dvrRespond + 3);
						
						length += 100;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_SET_EFFECT:
						lidbg(TAG"%s:CMD_SET_EFFECT\n",__func__);
						length += 2;
						break;

					case CMD_DUAL_CAM:
						lidbg(TAG"%s:CMD_DUAL_CAM\n",__func__);
						if(((char*)arg)[1] == 1) isDualCam = 1;
						else isDualCam = 0;

						s_info.recordMode= isDualCam;
						notify_newDVR(MSG_RECORD_MODE);

						length += 2;
						break;
					case CMD_FORMAT_SDCARD:
						lidbg(TAG"%s:CMD_FORMAT_SDCARD\n",__func__);

						notify_newDVR(MSG_START_FORMAT_NOTIFY);

						if(((char*)arg)[1] == 1) 
						{
							lidbg(TAG"%s:Begin format sdcard!\n",__func__);
							//lidbg_shell_cmd("echo appcmd *158#097 > /dev/lidbg_drivers_dbg0");
							schedule_delayed_work(&work_t_format_done, 3*HZ);
							dvrRespond[1] = 1;
						}
						else
						{
							lidbg(TAG"%s:Stop format sdcard!\n",__func__);
							dvrRespond[1] = 0;
						}
						
						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_EM_EVENT_SWITCH:
						lidbg(TAG"%s:CMD_EM_EVENT_SWITCH\n",__func__);
						if(((char*)arg)[1] == 0) 
						{
							lidbg(TAG"%s:Emergency event permitted!\n",__func__);
							isEmRecPermitted = 1;
							dvrRespond[1] = !isEmRecPermitted;
						}
						else
						{
							lidbg(TAG"%s:Emergency event rejected!\n",__func__);
							isEmRecPermitted = 0;
							dvrRespond[1] = !isEmRecPermitted;
						}

						s_info.emergencySwitch= isEmRecPermitted;
						notify_newDVR(MSG_EM_SWITCH);

						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_EM_SAVE_DAYS:
						lidbg(TAG"%s:CMD_EM_SAVE_DAYS [%d]\n",__func__,((char*)arg)[1]);
						if(((char*)arg)[1] > 0)  delDays= ((char*)arg)[1];

						s_info.emergencySaveDays= delDays;
						notify_newDVR(MSG_EM_SAVE_DAYS);
						
						dvrRespond[1] = delDays;
						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_CVBS_MODE:
						lidbg(TAG"%s:CMD_CVBS_MODE [%d]\n",__func__,((char*)arg)[1]);

						if(((char*)arg)[1] == 1) 
						{
							lidbg(TAG"%s:CVBS Mode!\n",__func__);
							CVBSMode = 1;
						}
						else
						{
							lidbg(TAG"%s:USB Cam Mode!\n",__func__);
							CVBSMode = 0;
						}
						
						sprintf(temp_cmd, "setprop persist.uvccam.CVBSMode %d",CVBSMode);
						lidbg_shell_cmd(temp_cmd);
						
						dvrRespond[1] = CVBSMode;
						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_GSENSOR_SENSITIVITY:
						lidbg(TAG"%s:CMD_GSENSOR_SENSITIVITY [%d]\n",__func__,((char*)arg)[1]);
						
						if(((char*)arg)[1] == 0) 
						{
							sensitivity_level = 0;
							s_info.sensitivityLevel = 0;
						}
						else if(((char*)arg)[1] == 1) 
						{
							sensitivity_level = 1;
							s_info.sensitivityLevel = 1;
						}
						else if(((char*)arg)[1] == 2) 
						{
							sensitivity_level = 2;
							s_info.sensitivityLevel = 2;
						}
						else lidbg(TAG"%s:INVALID SENSITIVITY!! [%d]\n",__func__,((char*)arg)[1]);

						notify_newDVR(MSG_GSENSOR_SENSITIVITY);
						
						dvrRespond[1] = sensitivity_level;
						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_VR_LOCK:
						lidbg(TAG"%s:CMD_VR_LOCK [%d]\n",__func__,((char*)arg)[1]);
						
						if(((char*)arg)[1] == 0) 
						{
							isVRLocked = 0;
							s_info.isVRLocked = false;
						}
						else if(((char*)arg)[1] == 1) 
						{
							isVRLocked = 1;
							s_info.isVRLocked = true;
						}
						else lidbg(TAG"%s:INVALID CMD_VR_LOCK!! [%d]\n",__func__,((char*)arg)[1]);
						notify_newDVR(MSG_VR_LOCK);
						break;
					case CMD_AUTO_DETECT:
						lidbg(TAG"%s:CMD_AUTO_DETECT\n",__func__);

						ssleep(4);

						if(s_info.isFrontCamReady == false)
						{
							lidbg(TAG"%s:DVR not found,ioctl fail!\n",__func__);
							dvrRespond[2] = RET_NOTVALID;
						}
						else dvrRespond[2] = RET_SUCCESS;

						if(s_info.isRearCamReady == false)
						{
							lidbg(TAG"%s:Rear not found,ioctl fail!\n",__func__);
							rearRespond[2] = RET_NOTVALID;
						}
						else rearRespond[2] = RET_SUCCESS;
						
						initMsg[length] = 0xB0;
						length++;
						dvrRespond[0] = CMD_RECORD;
						rearRespond[0] = CMD_RECORD;
						//if(s_info.isSDCardReady == true)
						dvrRespond[3] = s_info.recordSwitch;
						//else dvrRespond[3] = 0;
						rearRespond[3] = 0;
						
						memcpy(initMsg + length,dvrRespond,4);
						length += 4;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,4);
						length += 4;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						dvrRespond[0] = CMD_SET_RESOLUTION;
						rearRespond[0] = CMD_SET_RESOLUTION;
						memcpy(dvrRespond + 3,f_rec_res,10);
						memcpy(rearRespond + 3,f_rec_res,10);
						
						memcpy(initMsg + length,dvrRespond,13);
						length += 13;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,13);
						length += 13;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						f_rec_time = s_info.singleFileRecordTime * 60;

						dvrRespond[0] = CMD_TIME_SEC;
						rearRespond[0] = CMD_TIME_SEC;
						dvrRespond[3] = f_rec_time >> 8;
						dvrRespond[4] = f_rec_time;
						rearRespond[3] = f_rec_time >> 8;
						rearRespond[4] = f_rec_time;
						
						memcpy(initMsg + length,dvrRespond,5);
						length += 5;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,5);
						length += 5;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						dvrRespond[0] = CMD_FW_VER;
						rearRespond[0] = CMD_FW_VER;
						memcpy(dvrRespond + 3,camera_DVR_fw_version,10);
						memcpy(rearRespond + 3,camera_DVR_fw_version,10);
						
						memcpy(initMsg + length,dvrRespond,13);
						length += 13;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,13);
						length += 13;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						dvrRespond[0] = CMD_TOTALSIZE;
						rearRespond[0] = CMD_TOTALSIZE;
						dvrRespond[3] = f_rec_totalsize >> 24;
						dvrRespond[4] = f_rec_totalsize >> 16;
						dvrRespond[5] = f_rec_totalsize >> 8;
						dvrRespond[6] = f_rec_totalsize;
						rearRespond[3] = f_rec_totalsize >> 24;
						rearRespond[4] = f_rec_totalsize >> 16;
						rearRespond[5] = f_rec_totalsize >> 8;
						rearRespond[6] = f_rec_totalsize;
						
						memcpy(initMsg + length,dvrRespond,7);
						length += 7;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,7);
						length += 7;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						dvrRespond[0] = CMD_PATH;
						rearRespond[0] = CMD_PATH;
						checkSDCardStatus(f_rec_path);
						memcpy(dvrRespond + 3,f_rec_path,60);
						memcpy(rearRespond + 3,f_rec_path,60);
						
						memcpy(initMsg + length,dvrRespond,63);
						length += 63;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						memcpy(initMsg + length,rearRespond,63);
						length += 63;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_DUAL_CAM;
						length++;
						initMsg[length] = s_info.recordMode;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_EM_EVENT_SWITCH;
						length++;
						initMsg[length] = !s_info.emergencySwitch;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_EM_SAVE_DAYS;
						length++;
						initMsg[length] = s_info.emergencySaveDays;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_CVBS_MODE;
						length++;
						initMsg[length] = CVBSMode;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_GSENSOR_SENSITIVITY;
						length++;
						initMsg[length] = s_info.sensitivityLevel;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
#if 0
						invoke_AP_ID_Mode(DVR_GET_RES_ID_MODE);
						wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ);
						memcpy(dvrRespond + 3,camera_DVR_res,70);
						length += 73;
						memcpy(initMsg + length,dvrRespond,73);
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
#endif
						lidbg(TAG"%s:CMD_AUTO_DETECT ;1:0x%x,2:0x%x,3:0x%x,4:0x%x,5:0x%x,6:0x%x,7:0x%x,\n",__func__,initMsg[0],initMsg[1],initMsg[2],initMsg[3],initMsg[4],initMsg[5],initMsg[6]);
						lidbg(TAG"%s:length = %d\n",__func__,length);
						if(copy_to_user((char*)arg,initMsg,length))
						{
							lidbg(TAG"%s:copy_to_user ERR\n",__func__);
						}
						break;
				}
				return length;
		        break;
			default:
		        return -ENOTTY;
		}
	}
	else if(_IOC_TYPE(cmd) == FLYCAM_EM_MAGIC)//front cam online mode
	{
		unsigned char dvrRespond[100] = {0};
		unsigned char rearRespond[100] = {0};
		unsigned char returnRespond[200] = {0};
		int length = 0;
		//dvrRespond[0] = ((char*)arg)[0];
		//rearRespond[0] = ((char*)arg)[0];
		dvrRespond[0] = DVR_ID;
		rearRespond[0] = REARVIEW_ID;		

		//lidbg(TAG"%s:FLYCAM_EM_MAGIC\n",__func__);
		/*check dvr camera status before doing ioctl*/
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
		{
			//lidbg(TAG"%s:DVR not found,ioctl fail!\n",__func__);
			dvrRespond[1] = RET_NOTVALID;
		}
		else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
		{
			lidbg(TAG"%s:Is not SonixCam ,ioctl fail!\n",__func__);
			dvrRespond[1] = RET_NOTSONIX;
		}

		/*check rear camera status before doing ioctl*/
		if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
		{
			//lidbg(TAG"%s:Rear not found,ioctl fail!\n",__func__);
			rearRespond[1] = RET_NOTVALID;
		}
		else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
		{
			lidbg(TAG"%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
			rearRespond[1] = RET_NOTSONIX;
		}
		
#if 1
		switch(_IOC_NR(cmd))
		{
			case NR_EM_PATH:
				lidbg(TAG"%s:NR_EM_PATH  = [%s]\n",__func__,(char*)arg);
#if 0
				ret_st = checkSDCardStatus((char*)arg);
				if((ret_st != 1) || (ret_st != 2)) 
				{
					strcpy(em_path,(char*)arg);
					sprintf(temp_cmd, "setprop persist.uvccam.empath %s", em_path);
					lidbg_shell_cmd(temp_cmd);
				}
				else
					lidbg(TAG"%s: em_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) dvrRespond[1] = RET_FAIL;
#endif
				sprintf(temp_cmd, "mkdir -p %s", (char*)arg);
				lidbg_shell_cmd(temp_cmd);
#if 0				
				sprintf(temp_cmd, "mkdir -p %s/.tmp", (char*)arg);
				lidbg_shell_cmd(temp_cmd);
#endif				

				file_path = filp_open((char*)arg, O_RDONLY | O_DIRECTORY, 0);
				if(IS_ERR(file_path))
				{
					lidbg(TAG"%s:EM_PATH ERR!!\n",__func__);
					lidbg_shell_cmd("setprop persist.uvccam.empath "EMMC_MOUNT_POINT1"/camera_rec/BlackBox/");
					strcpy(dvrRespond + 2,EMMC_MOUNT_POINT1"/camera_rec/BlackBox/");
				}
				else
				{
					lidbg(TAG"%s:EMPATH OK!!\n",__func__);
					strcpy(em_path,(char*)arg);
					sprintf(temp_cmd, "setprop persist.uvccam.empath %s", em_path);
					lidbg_shell_cmd(temp_cmd);
					strcpy(dvrRespond + 2,em_path);
				}
				if(!IS_ERR(file_path)) filp_close(file_path, 0);
				
				length += 100;
				if(copy_to_user((char*)arg,dvrRespond,length))
				{
					lidbg(TAG"%s:copy_to_user ERR\n",__func__);
				}
		        break;
#if 0
			case NR_EM_START:
		        lidbg(TAG"%s:NR_EM_START\n",__func__);
				isUIStartRec = ((char*)arg)[0];
				if(((pfly_UsbCamInfo->camStatus)  & FLY_CAM_ISSONIX) 
				 && ((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
				{
					isDualCam = 1;
				}
				else isDualCam = 0;
				
				if(isDualCam)
				{
					if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID) dvrRespond[2] = 1;
					else dvrRespond[2] = 0;
					if((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID) rearRespond[2] = 1;
					else rearRespond[2] = 0;
				}
				else
				{
					if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID) dvrRespond[2] = 1;
					else dvrRespond[2] = 0;
					rearRespond[2] = 0;
				}
				//dvrRespond[2] = isDVRRec;
				//rearRespond[2] = isRearRec;
				memcpy(returnRespond + length,dvrRespond,3);
				length += 3;
				memcpy(returnRespond + length,rearRespond,3);
				length += 3;
				if(copy_to_user((char*)arg,returnRespond,length))
				{
					lidbg(TAG"%s:copy_to_user ERR\n",__func__);
				}
				mod_timer(&ui_start_rec_timer,UI_REC_WAIT_TIME);
		        break;
#endif
			case NR_EM_START:
				if(DVR_ID == ((char*)arg)[0])
				{
			        lidbg(TAG"%s:NR_EM_DVR_START\n",__func__);
					isEMDVRStartRec = ((char*)arg)[1];
					if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID) dvrRespond[2] = 1;
						else dvrRespond[2] = 0;
					memcpy(returnRespond + length,dvrRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg(TAG"%s:copy_to_user ERR\n",__func__);
					}
					mod_timer(&em_start_dvr_rec_timer,UI_REC_WAIT_TIME);
				}
				else if(REARVIEW_ID == ((char*)arg)[0])
				{
			        lidbg(TAG"%s:NR_EM_REAR_START\n",__func__);
					isEMRearStartRec = ((char*)arg)[1];
					if((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID) rearRespond[2] = 1;
						else rearRespond[2] = 0;
					memcpy(returnRespond + length,rearRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg(TAG"%s:copy_to_user ERR\n",__func__);
					}
					mod_timer(&em_start_rear_rec_timer,UI_REC_WAIT_TIME);
				}
				break;
			
			case NR_EM_STATUS:
		        //lidbg(TAG"%s:NR_EM_STATUS\n",__func__);
				//ret = isDVRRec;
				dvrRespond[2] = isDVRRec;
				rearRespond[2] = isRearRec;
				memcpy(returnRespond + length,dvrRespond,3);
				length += 3;
				memcpy(returnRespond + length,rearRespond,3);
				length += 3;
				if(copy_to_user((char*)arg,returnRespond,length))
				{
					lidbg(TAG"%s:copy_to_user ERR\n",__func__);
				}
		        break;
			
			case NR_EM_TIME:
				if(1 == ((char*)arg)[0])
				{
					lidbg(TAG"%s:NR_EM_TOP_TIME = [%d]\n",__func__,((char*)arg)[1]);
					if(((char*)arg)[1] <= 20)
						top_em_time = ((char*)arg)[1];
					else top_em_time = 10;
					sprintf(temp_cmd, "setprop persist.uvccam.top.emtime %d", top_em_time);
					lidbg_shell_cmd(temp_cmd);
					dvrRespond[2] = top_em_time;
					rearRespond[2] = top_em_time;
					memcpy(returnRespond + length,dvrRespond,3);
					length += 3;
					memcpy(returnRespond + length,rearRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg(TAG"%s:copy_to_user ERR\n",__func__);
					}
				}
				else if(0 == ((char*)arg)[0])
				{
					lidbg(TAG"%s:NR_EM_BOTTOM_TIME = [%d]\n",__func__,((char*)arg)[1]);
					if(((char*)arg)[1] <= 20)
						bottom_em_time = ((char*)arg)[1];
					else bottom_em_time = 10;
					sprintf(temp_cmd, "setprop persist.uvccam.bottom.emtime %d", bottom_em_time);
					lidbg_shell_cmd(temp_cmd);
					dvrRespond[2] = bottom_em_time;
					rearRespond[2] = bottom_em_time;
					memcpy(returnRespond + length,dvrRespond,3);
					length += 3;
					memcpy(returnRespond + length,rearRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg(TAG"%s:copy_to_user ERR\n",__func__);
					}
				}
		        break;
#if 0
			case NR_EM_TIME:
				lidbg(TAG"%s:NR_EM_TIME = [%d]\n",__func__,((char*)arg)[0]);
				em_time = ((char*)arg)[0];
				dvrRespond[2] = em_time;
				rearRespond[2] = em_time;
				memcpy(returnRespond + length,dvrRespond,3);
				length += 3;
				memcpy(returnRespond + length,rearRespond,3);
				length += 3;
				if(copy_to_user((char*)arg,returnRespond,length))
				{
					lidbg(TAG"%s:copy_to_user ERR\n",__func__);
				}
		        break;
#endif
			case NR_EM_MANUAL:
				lidbg(TAG"%s:NR_EM_MANUAL\n",__func__);
				if(isEmRecPermitted)
				{
					if(isDVRRec) 
					{
						lidbg_shell_cmd("setprop lidbg.uvccam.dvr.blackbox 1");
						notify_online(RET_EM_ISREC_ON);
						mod_timer(&stop_thinkware_em_timer,STOP_THINKNAVI_EM_WAIT_TIME);
					}
					if(isRearRec) 
					{
						lidbg_shell_cmd("setprop lidbg.uvccam.rear.blackbox 1");
						notify_online(RET_EM_ISREC_ON);
						mod_timer(&stop_thinkware_em_timer,STOP_THINKNAVI_EM_WAIT_TIME);
					}
				}
		        break;
			case NR_CAM_STATUS:
				lidbg(TAG"%s:NR_CAM_STATUS\n",__func__);
				if(DVR_ID == ((char*)arg)[0])
				{
					if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID) dvrRespond[2] = RET_SUCCESS;
					else dvrRespond[2] = RET_NOTVALID;
					memcpy(returnRespond + length,dvrRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg(TAG"%s:copy_to_user ERR\n",__func__);
					}
				}
				else if(REARVIEW_ID == ((char*)arg)[0])
				{
					if((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID) rearRespond[2] = RET_SUCCESS;
						else rearRespond[2] = RET_NOTVALID;
					memcpy(returnRespond + length,rearRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg(TAG"%s:copy_to_user ERR\n",__func__);
					}
				}
		        break;
			default:
		        return -ENOTTY;
		}
		#endif
	}
	//else return -EINVAL;
    return ret;
}

/******************************************************************************
 * Function: flycam_poll
 * Description: Flycam poll function.
 * Input parameters:
 *   	filp	-	file struct
 *   	wait	-	poll table struct.
 * Return values:
 *		
 * Notes: Wait for the status ready and wake up wait_queue.
 *****************************************************************************/
static unsigned int  flycam_poll(struct file *filp, struct poll_table_struct *wait)
{
    //struct fly_KeyEncoderInfo *pflycam_Info= filp->private_data;
    unsigned int mask = 0;
    poll_wait(filp, &pfly_UsbCamInfo->camStatus_wait_queue, wait);
    if(!kfifo_is_empty(&camStatus_data_fifo))
    {
        mask |= POLLIN | POLLRDNORM;
    }
    return mask;
}

/******************************************************************************
 * Function: flycam_read
 * Description: Flycam read function.
 * Input parameters:
 *   	filp	-	file struct
 *   	buffer	-	user space buf.
 *   	size	-	buf size.
 *   	offset	-	buf offset.
 * Return values:
 *		ssize_t	-	size
 * Notes: Read camera status.(asynchronous)
 *****************************************************************************/
ssize_t  flycam_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
	unsigned char ret = 0;
#if 0
	if(!isBackChange)
	{
	    if(wait_event_interruptible(wait_queue, isBackChange))
	        return -ERESTARTSYS;
	}
	isBack = SOC_IO_Input(BACK_DET, BACK_DET, GPIO_CFG_PULL_UP);
	
	if (copy_to_user(buffer, &isBack,  1))
	{
		lidbg(TAG"copy_to_user ERR\n");
	}
	isBackChange = 0;
	return size;
#endif
	if(kfifo_is_empty(&camStatus_data_fifo))
    {
        if(wait_event_interruptible(pfly_UsbCamInfo->camStatus_wait_queue, !kfifo_is_empty(&camStatus_data_fifo)))
            return -ERESTARTSYS;
    }
	down(&pfly_UsbCamInfo->sem);
	ret = kfifo_out(&camStatus_data_fifo, camStatus_data_for_hal, 1);
	up(&pfly_UsbCamInfo->sem);
	lidbg(TAG"%s:====HAL read_status => 0x%x=====\n",__func__,camStatus_data_for_hal[0]);
	if(copy_to_user(buffer, camStatus_data_for_hal, 1))
    {
    	lidbg(TAG"%s:====copy_to_user fail=> 0x%x=====\n",__func__,camStatus_data_for_hal[0]);
        return -1;
    }
	return 1;
}

int flycam_open (struct inode *inode, struct file *filp)
{
	//filp->private_data = pflycam_Info;
    return 0;
}

/******************************************************************************
 * Function: flycam_write
 * Description: Flycam write function.
 * Input parameters:
 *   	filp	-	file struct
 *   	buffer	-	user space buf.
 *   	size	-	buf size.
 *   	ppos	-	buf ppos.
 * Return values:
 *		ssize_t	-	size
 * Notes: Old command interface -> Only for online at present(DVR use ioctl)
 *****************************************************************************/
ssize_t flycam_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	char *cmd[30] = {NULL};//cmds array
	char *keyval[2] = {NULL};//key-vals
	char cmd_num  = 0;//cmd amount
    char cmd_buf[256];
	int i;
	char ret = 0;
    memset(cmd_buf, '\0', 256);
    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg(TAG"copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
		cmd_buf[size - 1] = '\0';
    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;
	lidbg(TAG"-----cmd_buf------------------[%s]---\n", cmd_buf);
	lidbg(TAG"-----cmd_num------------[%d]---\n", cmd_num);

	/*Do not check camera status in ACCOFF*/
#if 0	
	if(!isSuspend)
	{
		/*check camera status before doing ioctl*/
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
		{
			lidbg(TAG"%s:DVR[online] not found,ioctl fail!\n",__func__);
			return size;
		}
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRAfterFix)
		{
			lidbg(TAG"%s:is not SonixCam ,ioctl fail!\n",__func__);
			return size;
		}
	}
#endif	
	
	for(i = 0;i < cmd_num; i++)
	{
		lidbg_token_string(cmd[i], "=", keyval) ;
		if(!strcmp(keyval[0], "capture") )
		{
			if(!strncmp(keyval[1], "1", 1))//start
			{
				lidbg(TAG"-------uvccam capture-----");
				if(g_var.is_fly) lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -c -f mjpg -S &");
				else lidbg_shell_cmd("./system/lib/modules/out/lidbg_testuvccam /dev/video1 -c -f mjpg -S &");
			}
		}
		else if(!strcmp(keyval[0], "gain") )
		{
			int gainVal;
			char temp_cmd[256];
			gainVal = simple_strtoul(keyval[1], 0, 0);
			if(gainVal > 100)
			{
		        lidbg(TAG"gain args error![0-100]");
		        return size;
		    }
			lidbg(TAG"gainVal = %d",gainVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set gain=%d ", gainVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "sharp") )
		{
			int sharpVal;
			char temp_cmd[256];
			sharpVal = simple_strtoul(keyval[1], 0, 0);
			if(sharpVal > 6)
			{
		        lidbg(TAG"sharp args error![0-6]");
		        return size;
		    }
			lidbg(TAG"sharpVal = %d",sharpVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set sharp=%d ", sharpVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "gamma") )
		{
			int gammaVal;
			char temp_cmd[256];
			gammaVal = simple_strtoul(keyval[1], 0, 0);
			if(gammaVal > 500)
			{
		        lidbg(TAG"gamma args error![0-500]");
		        return size;
		    }
			lidbg(TAG"gammaVal = %d",gammaVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set gamma=%d ", gammaVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "bright") )
		{
			int brightVal;
			char temp_cmd[256];
			brightVal = simple_strtoul(keyval[1], 0, 0);
			if(brightVal > 128)
			{
		        lidbg(TAG"bright args error![0-128]");
		        return size;
		    }
			lidbg(TAG"brightVal = %d",brightVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set bright=%d ", brightVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "vmirror") )
		{
			int vmirrorVal;
			char temp_cmd[256];
			vmirrorVal = simple_strtoul(keyval[1], 0, 0);
			if(vmirrorVal > 1)
			{
		        lidbg(TAG"vmirror args error![0|1]");
		        return size;
		    }
			lidbg(TAG"vmirrorVal = %d",vmirrorVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set vmirror=%d ", vmirrorVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "autogain") )
		{
			int autogainVal;
			char temp_cmd[256];
			autogainVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"autogainVal = %d",autogainVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set autogain=%d ", autogainVal);
			//lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "exposure") )
		{
			int exposureVal;
			char temp_cmd[256];
			exposureVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"exposureVal = %d",exposureVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set exposure=%d ", exposureVal);
			//lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "contrast") )
		{
			int contrastVal;
			char temp_cmd[256];
			contrastVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"autogainVal = %d",contrastVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set contrast=%d ", contrastVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "saturation") )
		{
			int saturationVal;
			char temp_cmd[256];
			saturationVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"exposureVal = %d",saturationVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set saturation=%d ", saturationVal);
			lidbg_shell_cmd(temp_cmd);
		}
		else if(!strcmp(keyval[0], "res") )
		{
#if 0
			if(!strncmp(keyval[1], "1080", 4))
			{
				strcpy(f_online_path,"1920x1080");
			}
			else if(!strncmp(keyval[1], "720", 3))
			{
				strcpy(f_online_path,"1280x720");
			}
			else if(!strncmp(keyval[1], "640x360", 7))
			{
				strcpy(f_online_path,"640x360");
			}
			else
			{
				lidbg(TAG"-------res wrong arg:%s-----",keyval[1]);
			}
#endif
			if(!strncmp(keyval[1], "640x360", 7))
			{
				strcpy(f_online_res,keyval[1]);
			}
			else
			{
				lidbg(TAG"-------res wrong arg:%s-----",keyval[1]);
			}
		}
		else if(!strcmp(keyval[0], "rectime") )
		{
			int rectimeVal;
			rectimeVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"rectimeVal = %d",rectimeVal);
			f_online_time = rectimeVal;
		}
		else if(!strcmp(keyval[0], "recbitrate") )
		{
			int recbitrateVal;
			recbitrateVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"recbitrateVal = %d",recbitrateVal);
			f_online_bitrate = recbitrateVal;
		}
		#if 1
		else if(!strcmp(keyval[0], "recnum") )
		{
			int recnumVal;
			recnumVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"recnumVal = %d",recnumVal);
			f_online_filenum = recnumVal;
		}
		#endif
		else if(!strcmp(keyval[0], "recpath") )
		{
			char ret_st;
			lidbg(TAG"recpathVal = %s",keyval[1]);
			ret_st = checkSDCardStatus(keyval[1]);
			if(ret_st != 1) 
				strcpy(f_online_path,keyval[1]);
			else
				lidbg(TAG"%s: f_online_path access wrong! %d", __func__ ,EFAULT);//not happend
			if(ret_st > 0) ret = RET_FAIL;
		}
		else if(!strcmp(keyval[0], "recfilesize") )
		{
			int recfilesizeVal;
			recfilesizeVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"recfilesizeVal = %d",recfilesizeVal);
			f_online_totalsize = recfilesizeVal;
		}
		else if(!strcmp(keyval[0], "formatcomplete") )
		{
			int formatVal;
			formatVal = simple_strtoul(keyval[1], 0, 0);
			lidbg(TAG"formatcomplete = %d\n",formatVal);
			ssleep(5);
			notify_newDVR(MSG_STOP_FORMAT_NOTIFY);
			ssleep(2);
			if(formatVal == 1) status_fifo_in(RET_FORMAT_SUCCESS);
			else status_fifo_in(RET_FORMAT_FAIL);
		}
	}
  
    return size;
}

static  struct file_operations flycam_nod_fops =
{
    .owner = THIS_MODULE,
    .write = flycam_write,
    .open = flycam_open,
    .read = flycam_read,
    .unlocked_ioctl = flycam_ioctl,
    .poll = flycam_poll,
};

static int flycam_ops_suspend(struct device *dev)
{
    lidbg(TAG"-----------flycam_ops_suspend------------\n");
    DUMP_FUN;
	isKSuspend = 1;
    return 0;
}

static int flycam_ops_resume(struct device *dev)
{

    lidbg(TAG"-----------flycam_ops_resume------------\n");
    DUMP_FUN;
	//lidbg(TAG"g_var.acc_flag => %d",g_var.acc_flag);
#if 0
	if(g_var.acc_flag == FLY_ACC_ON)
	{
		isFirstresume = 1;
	}
#endif
    return 0;
}

static struct dev_pm_ops flycam_ops =
{
    .suspend	= flycam_ops_suspend,
    .resume	= flycam_ops_resume,
};

static int flycam_probe(struct platform_device *pdev)
{
    return 0;
}

static int flycam_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_device flycam_devices =
{
    .name			= "lidbg_flycam",
    .id 			= 0,
};

static struct platform_driver flycam_driver =
{
    .probe = flycam_probe,
    .remove = flycam_remove,
    .driver = 	{
        .name = "lidbg_flycam",
        .owner = THIS_MODULE,
        .pm = &flycam_ops,
    },
};

static void suspend_stoprec_isr(unsigned long data)
{
	if(s_info.isACCOFF == true)
	{
		lidbg(TAG"%s:------------Force unrequest!------------\n",__func__);
		lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
		isOnlineRunning = false;
	}
	return;
}

int thread_flycam_init(void *data)
{
#ifndef	FLY_USB_CAMERA_SUPPORT
	lidbg(TAG"%s:FLY_USB_CAMERA_SUPPORT not define,exit\n",__func__);
  	return 0;
#endif
	lidbg(TAG"%s:------------start------------\n",__func__);

	/*init pfly_UsbCamInfo*/
	pfly_UsbCamInfo = (struct fly_UsbCamInfo *)kmalloc( sizeof(struct fly_UsbCamInfo), GFP_KERNEL);
    if (pfly_UsbCamInfo == NULL)
    {
        lidbg(TAG"[cam]:kmalloc err\n");
        return -ENOMEM;
    }
	
	camStatus_fifo_buffer = (u8 *)kmalloc(FIFO_SIZE , GFP_KERNEL);
    camStatus_data_for_hal = (u8 *)kmalloc(HAL_BUF_SIZE , GFP_KERNEL);
	sema_init(&pfly_UsbCamInfo->sem, 1);
    kfifo_init(&camStatus_data_fifo, camStatus_fifo_buffer, FIFO_SIZE);

	notify_fifo_buffer = (u8 *)kmalloc(NOTIFY_FIFO_SIZE , GFP_KERNEL);
	sema_init(&pfly_UsbCamInfo->notify_sem, 1);
    kfifo_init(&notify_data_fifo, notify_fifo_buffer, NOTIFY_FIFO_SIZE);

	//init_waitqueue_head(&wait_queue);
	init_waitqueue_head(&pfly_UsbCamInfo->camStatus_wait_queue);/*camera status wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->DVR_ready_wait_queue);/*DVR wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->Rear_ready_wait_queue);/*Rear wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->onlineNotify_wait_queue);/*onlineNotify wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->newdvr_wait_queue);/*onlineNotify wait queue*/

	init_completion(&DVR_res_get_wait);
	init_completion(&Rear_res_get_wait);
	init_completion(&timer_stop_rec_wait);
	init_completion(&Rear_fw_get_wait);
	init_completion(&DVR_fw_get_wait);
	init_completion(&accon_start_rec_wait);
	//init_completion(&auto_detect_wait);

	//getVal();

	init_timer(&suspend_stoprec_timer);
	suspend_stoprec_timer.function = suspend_stoprec_isr;
	suspend_stoprec_timer.data = 0;
	suspend_stoprec_timer.expires = 0;

	if(g_var.recovery_mode == 0)/*do not process when in recovery mode*/
	{
		register_lidbg_notifier(&lidbg_notifier);/*ACCON/OFF notifier*/
		lidbg_shell_cmd("/flysystem/bin/lidbg_flydvr&");	
	}
	usb_register_notify(&usb_nb_cam);/*USB notifier:must after isDVRFirstInit&isRearViewFirstInit*/
	lidbg_new_cdev(&flycam_nod_fops, "lidbg_flycam0");
	lidbg_shell_cmd("chmod 777 /dev/lidbg_flycam0");
    return 0;
}

static __init int lidbg_flycam_init(void)
{
    DUMP_BUILD_TIME;

#ifndef FLY_USB_CAMERA_SUPPORT
	lidbg(TAG"%s return\n",__func__);
       return 0;
#endif
    LIDBG_GET;
    CREATE_KTHREAD(thread_flycam_init, NULL);
	platform_device_register(&flycam_devices);
    platform_driver_register(&flycam_driver);
    return 0;

}
static void __exit lidbg_flycam_deinit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");

module_init(lidbg_flycam_init);
module_exit(lidbg_flycam_deinit);


