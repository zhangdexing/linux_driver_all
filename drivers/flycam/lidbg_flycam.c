
#include "lidbg.h"
#include "lidbg_flycam_par.h"
LIDBG_DEFINE;

static void work_DVR_fixScreenBlurred(struct work_struct *work);
static void work_RearView_fixScreenBlurred(struct work_struct *work);
static void work_format_done(struct work_struct *work);
static int start_rec(char cam_id,char isPowerCtl);
static int stop_rec(char cam_id,char isPowerCtl);
static int dvr_start_recording(void);
static int dvr_stop_recording(void);
static int rear_start_recording(void);
static int rear_stop_recording(void);
static void setDVRProp(int camID);
static int checkSDCardStatus(char *path);
static int thread_accon_start_rec(void *data);
static void dvr_fail_proc(void);
static void rear_fail_proc(void);

#define NOTIFIER_MAJOR_GSENSOR_STATUS_CHANGE	(130)
#define NOTIFIER_MINOR_EXCEED_THRESHOLD 		(10)

/*camStatus mask*/
#define FLY_CAM_ISVALID	0x01
#define FLY_CAM_ISSONIX	0x02
//#define FLY_CAM_ISDVRUSED		0x04
//#define FLY_CAM_ISONLINEUSED		0x08

struct fly_UsbCamInfo
{
	unsigned char camStatus;/*Camera status(DVR&RearView)*/
	unsigned char read_status;/*Camera status for HAL to read(notify poll)*/
	unsigned char onlineNotify_status;
	wait_queue_head_t camStatus_wait_queue;/*notify wait queue*/
	wait_queue_head_t onlineNotify_wait_queue;/*notify wait queue*/
	wait_queue_head_t DVR_ready_wait_queue;
	wait_queue_head_t Rear_ready_wait_queue;
	struct semaphore sem;
};

struct fly_UsbCamInfo *pfly_UsbCamInfo;
static DECLARE_DELAYED_WORK(work_t_DVR_fixScreenBlurred, work_DVR_fixScreenBlurred);
static DECLARE_DELAYED_WORK(work_t_RearView_fixScreenBlurred, work_RearView_fixScreenBlurred);
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
static int f_rec_bitrate = 8000000,f_rec_time = 300,f_rec_filenum = 5,f_rec_totalsize = 8192;
static int f_online_bitrate = 500000,f_online_time = 60,f_online_filenum = 1,f_online_totalsize = 500;
char f_rec_res[100] = "1280x720",f_rec_path[100] = EMMC_MOUNT_POINT1"/camera_rec/";
char f_online_res[100] = "480x272",f_online_path[100] = EMMC_MOUNT_POINT0"/preview_cache/";

static int r_rec_bitrate = 8000000,r_rec_time = 300,r_rec_filenum = 5,r_rec_totalsize = 4096;
char r_rec_res[100] = "1280x720",r_rec_path[100] = EMMC_MOUNT_POINT1"/camera_rec/";

char em_path[100] = EMMC_MOUNT_POINT1"/camera_rec/BlackRec/";
static int top_em_time = 5,bottom_em_time = 10;

char capture_path[100] = EMMC_MOUNT_POINT0"/preview_cache/";

static struct timer_list suspend_stoprec_timer;
static struct timer_list set_par_timer;
static struct timer_list ui_start_rec_timer;
static struct timer_list em_start_dvr_rec_timer;
static struct timer_list em_start_rear_rec_timer;

#define SUSPEND_STOPREC_ONLINE_TIME   (jiffies + 180*HZ)  /* 3min stop Rec after online*/
#define SUSPEND_STOPREC_ACCOFF_TIME   (jiffies + 180*HZ)  /* 3min stop Rec after accoff,fix online then accoff*/
#define SET_PAR_WAIT_TIME   (jiffies + 2*HZ) 
#define UI_REC_WAIT_TIME   (jiffies + 2*HZ) 

/*bool var*/
static char isDVRRec,isOnlineRec,isRearRec,isDVRFirstInit,isRearViewFirstInit,isRearCheck = 1,isDVRCheck = 1,isOnlineNotifyReady,isDualCam,isColdBootRec,isDVRACCRec,isRearACCRec;
static char isSuspend,isDVRAfterFix,isRearViewAfterFix,isDVRFirstResume,isRearFirstResume,isUpdating,isKSuspend,isDVRReady,isRearReady,isDVRACCResume,isRearACCResume;
static char isUIStartRec,isDVRPlugRec,isRearPlugRec,isEMDVRStartRec,isEMRearStartRec,isBeforeFormatDVRRec,isBeforeFormatRearRec;

//struct work_struct work_t_fixScreenBlurred;

#define HAL_BUF_SIZE (512)
u8 *camStatus_data_for_hal;

#define FIFO_SIZE (512)
u8 *camStatus_fifo_buffer;
static struct kfifo camStatus_data_fifo;

u8 camera_DVR_fw_version[20] = {0};	
u8 camera_rear_fw_version[20] = {0};	

u8 camera_rear_res[100] = {0};
u8 camera_DVR_res[100] = {0};

char tm_cmd[100] = {0};


static int dvr_osd_fail_times,rear_osd_fail_times;
static char isDVROSDFail,isRearOSDFail;

static int isDVRVideoLoop = 1,isRearVideoLoop = 1,isPrevYUV = 0,isEmRecPermitted = 1,isThinkNavi;


#if 0
//ioctl
#define READ_CAM_PROP(magic , nr) _IOR(magic, nr ,int) 
#define WRITE_CAM_PROP(magic , nr) _IOW(magic, nr ,int)
#endif

static void notify_online(int arg)
{
	pfly_UsbCamInfo->onlineNotify_status = arg;
	isOnlineNotifyReady = 1;
	wake_up_interruptible(&pfly_UsbCamInfo->onlineNotify_wait_queue);
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
		lidbg("%s:====receive msg => %d====\n",__func__,status);
		wake_up_interruptible(&pfly_UsbCamInfo->camStatus_wait_queue);
		return;
	}
	else if(status == RET_DVR_OSD_FAIL)
	{
		lidbg("%s:===DVR_OSD_FAIL==%d===\n",__func__,g_var.android_boot_completed);
		if(1==g_var.android_boot_completed)  
		{
			isDVROSDFail = 1;
			dvr_osd_fail_times++;
		}
		if(isDVROSDFail && (dvr_osd_fail_times < 5))// 1 per 2 real times
		{
			if(!isSuspend) lidbg_shell_cmd("echo 'usb_reboot' > /dev/flydev0");
			isDVROSDFail = 0;
		}
		return;
	}
	else if(status == RET_REAR_OSD_FAIL)
	{
		lidbg("%s:===REAR_OSD_FAIL==%d===\n",__func__,g_var.android_boot_completed);
		if(1==g_var.android_boot_completed) 	
		{
			isRearOSDFail = 1;
			rear_osd_fail_times++;
		}
		if(isRearOSDFail && (rear_osd_fail_times < 5))// 1 per 2 real times
		{
			if(!isSuspend) lidbg_shell_cmd("echo 'usb_reboot' > /dev/flydev0");
			isRearOSDFail = 0;
		}
		return;
	}
	else if(status == RET_DVR_DISCONNECT || status == RET_DVR_INIT_INSUFFICIENT_SPACE_STOP)
	{
		isDVRRec = 0;
		isOnlineRec = 0;
	}
	else if(status == RET_REAR_DISCONNECT || status == RET_REAR_INIT_INSUFFICIENT_SPACE_STOP)
	{
		isRearRec = 0;
		isRearVideoLoop = 0;
	}
	lidbg("%s:====fifo in => %d====\n",__func__,status);
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
	lidbg("flycam event: %ld\n", event);
    switch (event)
    {
	    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
			lidbg("flycam event:resume %ld\n", event);
			isSuspend = 0;
			isDVRACCResume = 1;
			isRearACCResume = 1;
			dvr_osd_fail_times = 0;
			rear_osd_fail_times = 0;
			del_timer(&suspend_stoprec_timer);
			if(isKSuspend)
			{
				isDVRFirstResume = 1;
				isRearFirstResume = 1;
				if(!isRearRec)				
					schedule_delayed_work(&work_t_RearView_fixScreenBlurred, 0);/*Rec Block mode(First ACCON)*/
				else
				{
					status_fifo_in(RET_REAR_SONIX);//camera already working
				}
				
				if(!isOnlineRec && !isDVRRec) 
					schedule_delayed_work(&work_t_DVR_fixScreenBlurred, 3*HZ);/*Rec Block mode(First ACCON)*/
				else 
				{
					/*Online Rec when ACCON,stop online & start dvr*/
					if(isOnlineRec && isDVRACCRec)
					{
						lidbg("%s:==Online Rec when ACCON==\n",__func__);
						complete(&accon_start_rec_wait);
					}
					//else complete(&auto_detect_wait);
					status_fifo_in(RET_DVR_SONIX);//camera already working
					//notify_online(RET_ONLINE_FOUND_SONIX);
					isDVRFirstResume = 0;
					isDVRACCResume = 0;
					isDVRAfterFix = 1;
				}
				isKSuspend = 0;
			}
			else 
			{
				if(isOnlineRec && isDVRACCRec)
				{
					lidbg("%s:==Online Rec when ACCON==\n",__func__);
					complete(&accon_start_rec_wait);
					status_fifo_in(RET_DVR_SONIX);
				}
				//else complete(&auto_detect_wait);
			}
			break;
	    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
			lidbg("flycam event:suspend %ld\n", event);
			isSuspend = 1;
			mod_timer(&suspend_stoprec_timer,SUSPEND_STOPREC_ACCOFF_TIME);		
			break;
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, FLY_DEVICE_DOWN):
			lidbg("flycam event:device_down %ld\n", event);
			isDVRFirstResume = 0;
			isRearFirstResume = 0;
			isDVRACCResume = 0;
			isRearACCResume = 0;

			/*Auto stop*/
			isDVRACCRec = isDVRRec;
			isRearACCRec = isRearRec;
			if(isDVRRec) dvr_stop_recording();
			if(isRearRec) rear_stop_recording();

			break;
		case NOTIFIER_VALUE(NOTIFIER_MAJOR_GSENSOR_STATUS_CHANGE, NOTIFIER_MINOR_EXCEED_THRESHOLD):
			lidbg("flycam event:emergency recording %ld\n", event);
			if(isEmRecPermitted)
			{
				if(isDVRRec) lidbg_shell_cmd("setprop lidbg.uvccam.dvr.blackbox 1");
				if(isRearRec) lidbg_shell_cmd("setprop lidbg.uvccam.rear.blackbox 1");
			}
			notify_online(RET_EM_ISREC_ON);
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
 * Function: suspend_stoprec_timer_isr
 * Description: Suspend timeout stop Rec ISR function
 * Input parameters:
 *   data             - private data
 * Return values:
 *      none
 * Notes: none
 *****************************************************************************/
static void suspend_stoprec_timer_isr(unsigned long data)
{
	//if(isDVRRec ||  isOnlineRec || isRearRec)
	//{
	    lidbg("-------[TIMER]uvccam stop_recording -----\n");
		complete(&timer_stop_rec_wait);
	//}
	return;
}

static void set_par_timer_isr(unsigned long data)
{
	complete(&set_par_wait);
	return;
}

static void ui_start_rec_timer_isr(unsigned long data)
{
	complete(&ui_start_rec_wait);
	return;
}

static void start_dvr_rec_timer_isr(unsigned long data)
{
	complete(&start_dvr_rec_wait);
}

static void start_rear_rec_timer_isr(unsigned long data)
{
	complete(&start_rear_rec_wait);
}


/*can not block in ISR function*/
static int thread_stop_rec_func(void *data)
{
	while(1)
	{
		wait_for_completion(&timer_stop_rec_wait);
		/*notify DVR&Online*/
		if(isDVRRec) 
		{	
			if(stop_rec(DVR_ID,1))lidbg("%s:====return fail====\n",__func__);
			status_fifo_in(RET_DVR_FORCE_STOP);
		}
		if(isOnlineRec) 
		{
			if(stop_rec(DVR_ID,1))lidbg("%s:====return fail====\n",__func__);
			notify_online(RET_ONLINE_FORCE_STOP);
		}
		if(isRearRec) 
		{
			if(stop_rec(REARVIEW_ID,1))lidbg("%s:====return fail====\n",__func__);
			notify_online(RET_REAR_FORCE_STOP);
		}
		lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
		isDVRRec = 0;
		isDVRVideoLoop = 0;
		isOnlineRec = 0;
		isRearRec = 0;
	}
	return 0;
}

/*can not block in ISR function*/
static int thread_set_par_func(void *data)
{
	char temp_cmd[256];	
	while(1)
	{
		wait_for_completion(&set_par_wait);
		setDVRProp(DVR_ID);
		setDVRProp(REARVIEW_ID);
		if(isDVRRec)
		{
			lidbg("%s:restart DVR recording\n",__func__);
			stop_rec(DVR_ID,1);
			//msleep(500);
			start_rec(DVR_ID,1);
		}
		if(isRearRec)
		{
			if(isDualCam)
			{
				lidbg("%s:Dual => restart Rear recording\n",__func__);
				lidbg("%s:CMD_SET_PAR\n",__func__);
				stop_rec(REARVIEW_ID,1);
				msleep(800);
				start_rec(REARVIEW_ID,1);
			}
			else
			{
				lidbg("%s:Singal => stop Rear recording\n",__func__);
				//rear_stop_recording();
			}
		}
		else if(isDualCam && ((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX) && isDVRRec)
		{
			lidbg("%s:Dual => start Rear recording\n",__func__);
			//rear_start_recording();
			stop_rec(REARVIEW_ID,1);
			msleep(800);
			start_rec(REARVIEW_ID,1);
		}

		if(isDualCam)
		{
			if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID)	isDVRVideoLoop = 1;
			else	isDVRVideoLoop = 0;
			if((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID)	isRearVideoLoop = 1;
			else	isRearVideoLoop = 0;
		}
		else
		{
			if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID)	isDVRVideoLoop = 1;
			else	isDVRVideoLoop = 0;
			isRearVideoLoop = 0;
		}
		sprintf(temp_cmd, "setprop persist.uvccam.isDVRVideoLoop %d", isDVRVideoLoop);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop persist.uvccam.isRearVideoLoop %d", isRearVideoLoop);
		lidbg_shell_cmd(temp_cmd);
	}
	return 0;
}

static int thread_start_dvr_rec_func(void *data)
{
	int rc = -1;
	while(1)
	{
		wait_for_completion(&start_dvr_rec_wait);
		if(((pfly_UsbCamInfo->camStatus)  & FLY_CAM_ISSONIX))
		{
			if(isEMDVRStartRec== 1)
			{
				/*DVR start recording*/
				rc = dvr_start_recording();
				if(rc == -1)
					dvr_fail_proc();
			}
			else if(isEMDVRStartRec == 0)
			{
				/*DVR stop recording*/
				rc = dvr_stop_recording();
				if(rc == -1)
					dvr_fail_proc();
			}
		}
	}
	return 0;
}

static int thread_start_rear_rec_func(void *data)
{
	int rc = -1;
	while(1)
	{
		wait_for_completion(&start_rear_rec_wait);
		if(((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
		{
			if(isEMRearStartRec== 1)
			{
				/*Rear start recording*/
				rc = rear_start_recording();
				if(rc == -1)
					rear_fail_proc();
			}
			//else if(((char*)arg)[1] == 0)//stop
			else if(isEMRearStartRec == 0)
			{
				/*Rear stop recording*/
				rc = rear_stop_recording();
				if(rc == -1)
					rear_fail_proc();
			}
		}
	}
	return 0;
}

/*can not block in ISR function*/
static int thread_ui_startrec_func(void *data)
{
	int rc = -1;
	while(1)
	{
		wait_for_completion(&ui_start_rec_wait);
		if(isDualCam)//(char*)arg)[1] == DVR_ID
		{
			lidbg("%s:CMD_RECORD => DualCam\n",__func__);
			if(((pfly_UsbCamInfo->camStatus)  & FLY_CAM_ISSONIX) 
				 && ((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
			{
				//if(((char*)arg)[1] == 1)//start
				if(isUIStartRec== 1)
				{
					/*DVR start recording*/
					rc = dvr_start_recording();
					if(rc == -1)
					{
						dvr_fail_proc();
						//dvrRespond[2] = RET_FAIL;
					}
					//else dvrRespond[2] = rc;

					/*Rear start recording*/
					rc = rear_start_recording();
					if(rc == -1)
					{
						rear_fail_proc();
						//rearRespond[2] = RET_FAIL;
					}
					//else rearRespond[2] = rc;
				}
				//else if(((char*)arg)[1] == 0)//stop
				else if(isUIStartRec == 0)
				{
					/*DVR stop recording*/
					rc = dvr_stop_recording();
					if(rc == -1)
					{
						dvr_fail_proc();
						//dvrRespond[2] = RET_FAIL;
					}
					//else dvrRespond[2] = rc;

					/*Rear stop recording*/
					rc = rear_stop_recording();
					if(rc == -1)
					{
						rear_fail_proc();
						//rearRespond[2] = RET_FAIL;
					}
					//else rearRespond[2] = rc;
				}
			}
			else lidbg("%s:DualCam start Fail\n",__func__);
		}
		else
		{
			lidbg("%s:CMD_RECORD => SingleCam\n",__func__);
			if((pfly_UsbCamInfo->camStatus)  & FLY_CAM_ISSONIX)
			{
				//if(((char*)arg)[1] == 1)
				if(isUIStartRec == 1)
				{
					/*DVR start recording*/
					rc = dvr_start_recording();
					if(rc == -1)
					{
						dvr_fail_proc();
						//dvrRespond[2] = RET_FAIL;
					}
					//else dvrRespond[2] = rc;
				}
				//else if(((char*)arg)[1] == 0)
				else if(isUIStartRec == 0)
				{
					/*DVR stop recording*/
					rc = dvr_stop_recording();
					if(rc == -1)
					{
						dvr_fail_proc();
						//dvrRespond[2] = RET_FAIL;
					}
					//else dvrRespond[2] = rc;
				}
			}
			else lidbg("%s:SingleCam start Fail\n",__func__);
		}
		
		sprintf(tm_cmd, "setprop fly.uvccam.coldboot.isRec %d", isDVRRec);
		lidbg_shell_cmd(tm_cmd);
	}
	return 0;
}

static int thread_accon_start_rec(void *data)
{
	while(1)
	{
		wait_for_completion(&accon_start_rec_wait);
		if(!isDVRRec &&  isDVRACCRec)
		{
			lidbg("%s:==ACCON AUTO start DVR==\n",__func__);
			dvr_start_recording();
		}
		//complete(&auto_detect_wait);
		/*
		if( isDualCam && !isRearRec &&  isRearACCRec)
		{
			lidbg("%s:==ACCON AUTO start Rear==\n",__func__);
			rear_start_recording();
		}
		*/
	}
	return 0;
}

struct name_list
{
    char name[33];
    struct list_head list;
};

static int readdir_build_namelist(void *arg, const char *name, int namlen,	loff_t offset, u64 ino, unsigned int d_type)
{
    if(!(name[0] == '.' || (name[0] == '.' && name[1] == '.'))) // ignore "." and ".."
    {
        struct list_head *names = arg;
        struct name_list *entry;
        entry = kzalloc(sizeof(struct name_list), GFP_KERNEL);
        if (entry == NULL)
            return -ENOMEM;
        memcpy(entry->name, name, namlen);
        entry->name[namlen] = '\0';
        list_add(&entry->list, names);
    }
    return 0;
}

/******************************************************************************
 * Function: lidbg_checkCam
 * Description: Check Camera status
 * Input parameters:
 *   data             - private data
 * Return values:
 *      Camera status mask value.(FLY_CAM_ISVALID & FLY_CAM_ISSONIX.)
 * Notes: none
 *****************************************************************************/
static int lidbg_checkCam(void)
{
    LIST_HEAD(names);
	int back_charcnt = 0,front_charcnt = 0;
	char back_hub_path[256],front_hub_path[256];
    struct file *dir_file, *f_dir, *b_dir;
    int status;
	char insure_is_dir[50] = "/sys/bus/usb/drivers/usb/";
	char f_videocnt = 0,b_videocnt = 0;
	unsigned char retcode = 0;
#if 0
    if(!insure_is_dir )
   	{
   		lidbg("dir err!");
		return -1;
	}
#endif
    //lidbg("insure_is_dir => %s",insure_is_dir);
	//obtain hub node path
    dir_file = filp_open(insure_is_dir, O_RDONLY | O_DIRECTORY, 0);
    if (IS_ERR(dir_file))
    {
        LIDBG_ERR("open%s,%ld\n", insure_is_dir, PTR_ERR(dir_file));
		filp_close(dir_file, 0);
        return -1;
    }
    else
    {
        struct name_list *entry;
        int count = 0;
        //LIDBG_SUC("open:<%s,%s>\n", insure_is_dir, dir_file->f_path.dentry->d_name.name);
        status = vfs_readdir(dir_file, readdir_build_namelist, &names);
        if (dir_file)
            fput(dir_file);
        while (!list_empty(&names))
        {
            entry = list_entry(names.next, struct name_list, list);
            if (!status && entry)
            {
                count++;
				//lidbg("======5===%s==1==",entry->name);
				if((!strncmp(entry->name,  BACK_NODE , 5)) && (strlen(entry->name) >= back_charcnt) )
				{
					back_charcnt = strlen(entry->name);
					sprintf(back_hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", entry->name,entry->name);//back cam
				}
				else if((!strncmp(entry->name,  FRONT_NODE , 5)) && (strlen(entry->name) >= front_charcnt))
				{
					front_charcnt = strlen(entry->name);
					sprintf(front_hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", entry->name,entry->name);//front cam
				} 
                list_del(&entry->list);
                kfree(entry);
            }
            entry = NULL;
        }
				
		//check front Cam status:isPlugIn? isSonix?
		if(front_charcnt == 0)
		{
			//lidbg("%s: can not found front cam plug in! ", __func__ );	
		}
		else
		{
			if(IS_ERR(f_dir = filp_open(front_hub_path, O_RDONLY | O_DIRECTORY, 0)))
			{
				pr_debug("%s: front_hub_path access wrong!\n ", __func__ );
			}
			else
			{
				status = vfs_readdir(f_dir, readdir_build_namelist, &names);
				while (!list_empty(&names))
      		    {
					entry = list_entry(names.next, struct name_list, list);
					if(!strncmp(entry->name,  "video" , 5))
					{
						f_videocnt++;
						//lidbg("f_video found");
					}
					list_del(&entry->list);
               		kfree(entry);
				}
				filp_close(f_dir, 0);
			}
		}
		//check back Cam status:isPlugIn? isSonix?
		if(back_charcnt == 0)
		{
			pr_debug("%s: can not found back cam plug in!\n ", __func__ );	
		}
		else
		{
			if(IS_ERR(b_dir = filp_open(back_hub_path, O_RDONLY | O_DIRECTORY, 0)))
			{
				pr_debug("%s: back_hub_path access wrong! \n", __func__ );
			}
			else
			{
				status = vfs_readdir(b_dir, readdir_build_namelist, &names);
				while (!list_empty(&names))
      		    {
					entry = list_entry(names.next, struct name_list, list);
					if(!strncmp(entry->name,  "video" , 5))
					{
						b_videocnt++;
						//lidbg("b_video found");
					}
					list_del(&entry->list);
               		kfree(entry);
				}
				filp_close(b_dir, 0);
			}
		}
		if(f_videocnt == 2)
		{
			lidbg("%s: front isSonixCam\n", __func__ );
			retcode |= FLY_CAM_ISSONIX;
			retcode |= FLY_CAM_ISVALID;
		}
		else if(f_videocnt == 1)  
		{
			lidbg("%s: front is not SonixCam\n", __func__ );
			retcode &= ~FLY_CAM_ISSONIX;
			retcode |= FLY_CAM_ISVALID;
		}
		else
		{
			lidbg("%s: front not found\n", __func__ );
			retcode &= ~FLY_CAM_ISSONIX;
			retcode &= ~FLY_CAM_ISVALID;
		}
		if(b_videocnt == 2)
		{
			lidbg("%s: back isSonixCam\n", __func__ );
			retcode |= (FLY_CAM_ISSONIX << 4);
			retcode |= (FLY_CAM_ISVALID << 4);
		}
		else if(b_videocnt == 1)
		{
			lidbg("%s: back is not SonixCam\n", __func__ );
			retcode &= (~(FLY_CAM_ISSONIX << 4));
			retcode |= (FLY_CAM_ISVALID << 4);
		}
		else
		{
			lidbg("%s: back not found\n", __func__ );
			retcode &= (~(FLY_CAM_ISSONIX << 4));
			retcode &= (~(FLY_CAM_ISVALID << 4));
		}
        return retcode;
    }
}

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
	unsigned char oldCamStatus = 0;
	//struct usb_device *dev = data;
	switch (action)
	{
	case USB_DEVICE_ADD:
	case USB_DEVICE_REMOVE:
		oldCamStatus = pfly_UsbCamInfo->camStatus;
		pfly_UsbCamInfo->camStatus = lidbg_checkCam();
		lidbg("=====Camera retcode => 0x%x======\n",pfly_UsbCamInfo->camStatus);
		
		/*
			usb camera first plug in/out:
			Sonix:fix ScreenBlurred issue & start exposure adaptation & notify RET_SONIX;
			Not Sonix:notify RET_NOT_SONIX.
			Plug out:RET_DISCONNECT
		*/
		if(g_var.recovery_mode == 0)
		{
			/*RearView*/
			if(isRearCheck)
			{
				/*usb camera plug out */
				if(((oldCamStatus>>4) & FLY_CAM_ISVALID) && !((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISVALID))
				{
					isRearPlugRec = isRearRec;
					lidbg_shell_cmd("setprop lidbg.uvccam.rear.osdset 0&");
					lidbg_shell_cmd("setprop lidbg.uvccam.rear.status 0&");
					status_fifo_in(RET_REAR_DISCONNECT);
					isRearRec = 0;
					isRearViewAfterFix = 0;
					isRearReady = 0;
				}
				/*usb camera plug in */
				if(!((oldCamStatus>>4) & FLY_CAM_ISVALID) && ((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX) && !isRearViewFirstInit)
				{
					isRearReady = 1;
					lidbg_shell_cmd("setprop lidbg.uvccam.rear.osdset 1&");
					lidbg_shell_cmd("setprop lidbg.uvccam.rear.status 1&");
					wake_up_interruptible(&pfly_UsbCamInfo->Rear_ready_wait_queue);
					if(!isSuspend)
					{
						if(!isRearFirstResume ) schedule_delayed_work(&work_t_RearView_fixScreenBlurred, 0);
#if 0
						else
						{
							status_fifo_in(RET_REAR_SONIX);//firstResume:directly pop sonix status
						}
#endif
					}
				}
				else if(!((oldCamStatus>>4) & FLY_CAM_ISVALID) && !((pfly_UsbCamInfo->camStatus>>4)  & FLY_CAM_ISSONIX) &&((pfly_UsbCamInfo->camStatus>>4)  & FLY_CAM_ISVALID) )
				{
					status_fifo_in(RET_REAR_NOT_SONIX);
				}
			}	
			/*DVR*/
			if(isDVRCheck)
			{
				/*usb camera plug out :notify RET_DISCONNECT*/
				if((oldCamStatus & FLY_CAM_ISVALID) && !(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
				{
					isDVRPlugRec = isDVRRec;
					status_fifo_in(RET_DVR_DISCONNECT);
					notify_online(RET_ONLINE_DISCONNECT);
					lidbg_shell_cmd("setprop lidbg.uvccam.dvr.osdset 0&");
					lidbg_shell_cmd("setprop lidbg.uvccam.dvr.status 0&");
					isDVRRec = 0;
					isOnlineRec= 0;
					isDVRAfterFix = 0;
					isDVRReady = 0;
				}
				/*usb camera plug in :notify RET_SONIX | RET_NOT_SONIX*/
				if(!(oldCamStatus & FLY_CAM_ISVALID) && (pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRFirstInit)
				{
					//if(isDVRFirstInit) schedule_delayed_work(&work_t_DVR_fixScreenBlurred, 10);
					//else schedule_delayed_work(&work_t_DVR_fixScreenBlurred, 0);
#if 0
					if(isDVRFirstResume) complete(&DVR_ready_wait);
	  			    else schedule_delayed_work(&work_t_DVR_fixScreenBlurred, 0);
#endif
					//complete(&DVR_ready_wait);
					isDVRReady = 1;
					wake_up_interruptible(&pfly_UsbCamInfo->DVR_ready_wait_queue);
					lidbg_shell_cmd("setprop lidbg.uvccam.dvr.osdset 1&");
					lidbg_shell_cmd("setprop lidbg.uvccam.dvr.status 1&");
					if(!isSuspend)
					{
						//lidbg("=====CHECK::isDVRFirstResume = %d =====\n",isDVRFirstResume);
						if(!isDVRFirstResume) schedule_delayed_work(&work_t_DVR_fixScreenBlurred, 0);
#if 0
						else
						{
							status_fifo_in(RET_DVR_SONIX);//firstResume:directly pop sonix status
							notify_online(RET_ONLINE_FOUND_SONIX);
						}
#endif
					}
				}
				else if(!(oldCamStatus & FLY_CAM_ISVALID) && !(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) &&(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID) )
				{
					status_fifo_in(RET_DVR_NOT_SONIX);
					notify_online(RET_ONLINE_FOUND_NOTSONIX);
				}
			}
		}
		
	    break;
	}
	return NOTIFY_OK;
}

static struct notifier_block usb_nb_cam =
{
    .notifier_call = usb_nb_cam_func,
};

static int invoke_AP_ID_Mode(char cam_id_mode)
{
	char temp_cmd[256];	
	sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video2 -b %d -c -f H264 -r &", cam_id_mode);
	lidbg_shell_cmd(temp_cmd);
	return 0;
}


/******************************************************************************
 * Function: start_rec
 * Description: start sonix AP recording
 * Input parameters:
 *   	cam_id	-	camera ID (0 : RearView , 1: DVR)
 * Return values:
 *		0	-	start recording success
 *   	1	-	await sonix AP return timeout
 * Notes: none
 *****************************************************************************/
static int start_rec(char cam_id,char isPowerCtl)
{
	lidbg("%s:====E====\n",__func__);
	pfly_UsbCamInfo->read_status = RET_DEFALUT;
	if(isSuspend) mod_timer(&suspend_stoprec_timer,SUSPEND_STOPREC_ONLINE_TIME);
	if(isPowerCtl)
	{
		lidbg("%s:====udisk_request====\n",__func__);
		lidbg_shell_cmd("echo 'udisk_request' > /dev/flydev0");//don't control HUB power in rearview
	}
    if((cam_id == DVR_ID) || (cam_id == DVR_BLOCK_ID_MODE))
		lidbg_shell_cmd("setprop lidbg.uvccam.dvr.recording 1");
	else if((cam_id == REARVIEW_ID)  || (cam_id == REAR_BLOCK_ID_MODE))
		lidbg_shell_cmd("setprop lidbg.uvccam.rearview.recording 1");
	
 	invoke_AP_ID_Mode(cam_id);
	
	if((cam_id == DVR_ID) || (cam_id == DVR_BLOCK_ID_MODE))
	{
		if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->camStatus_wait_queue, (pfly_UsbCamInfo->read_status == RET_DVR_START), 6*HZ))
		{
			lidbg("%s:====read_status wait timeout => %d====\n",__func__,pfly_UsbCamInfo->read_status);
			return 1; 
		}
	}
	else if((cam_id == REARVIEW_ID)  || (cam_id == REAR_BLOCK_ID_MODE))
	{
		if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->camStatus_wait_queue, (pfly_UsbCamInfo->read_status == RET_REAR_START), 6*HZ))
		{
			lidbg("%s:====read_status wait timeout => %d====\n",__func__,pfly_UsbCamInfo->read_status);
			return 1; 
		}
	}
	lidbg("%s:====X====\n",__func__);
	return 0;
}

/******************************************************************************
 * Function: stop_rec
 * Description: stop sonix AP recording
 * Input parameters:
 *  	cam_id	-	camera ID (0 : RearView , 1: DVR)
 * Return values:
 *		0	-	stop recording success
 *   	1	-	await sonix AP return timeout
 * Notes: none
 *****************************************************************************/
static int stop_rec(char cam_id,char isPowerCtl)
{
	char ret = 0;
	lidbg("%s:====E====\n",__func__);
	pfly_UsbCamInfo->read_status = RET_DEFALUT;
	if(isSuspend) del_timer(&suspend_stoprec_timer);
	if((cam_id == DVR_ID) || (cam_id == DVR_BLOCK_ID_MODE))
		lidbg_shell_cmd("setprop lidbg.uvccam.dvr.recording 0");
	else if((cam_id == REARVIEW_ID)  || (cam_id == REAR_BLOCK_ID_MODE))
		lidbg_shell_cmd("setprop lidbg.uvccam.rearview.recording 0");
	//msleep(500);
	
	if((cam_id == DVR_ID) || (cam_id == DVR_BLOCK_ID_MODE))
	{
		if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->camStatus_wait_queue, (pfly_UsbCamInfo->read_status == RET_DVR_STOP), 3*HZ))
		{
			lidbg("%s:====read_status wait timeout => %d====\n",__func__,pfly_UsbCamInfo->read_status);
			ret = 1; 
		}
	}
	else if((cam_id == REARVIEW_ID)  || (cam_id == REAR_BLOCK_ID_MODE))
	{
		if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->camStatus_wait_queue, (pfly_UsbCamInfo->read_status == RET_REAR_STOP), 3*HZ))
		{
			lidbg("%s:====read_status wait timeout => %d====\n",__func__,pfly_UsbCamInfo->read_status);
			ret = 1; 
		}
	}
	msleep(500);
	if(isPowerCtl) 
	{	
		lidbg("%s:====udisk_unrequest====\n",__func__);
		lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");//don't control HUB power in rearview
	}
	lidbg("%s:====X====\n",__func__);
	return ret;
}

/******************************************************************************
 * Function: fixScreenBlurred
 * Description: Fix ScreenBlurred issue & Start exposure adaptation when camera first plug in.
 * Input parameters:
 *   	cam_id	-	camera ID (0 : RearView , 1: DVR)
 *		isOnline	-	if it is Online mode,there will be different sleep time & power control.
 * Return values:
 *		none
 * Notes: none
 *****************************************************************************/
static void fixScreenBlurred(char cam_id , char isOnline)
{
	if((cam_id == DVR_ID) || (cam_id == DVR_BLOCK_ID_MODE))
	{
		//if(!isDVRFirstResume)
		if(isDVRFirstInit)
		{
			isDVRCheck = 0;
			lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -3&");
			if(!wait_for_completion_timeout(&DVR_fw_get_wait , 3*HZ)) lidbg("%s:====DVR_fw_get_wait return fail====\n",__func__);
		}
		
		lidbg_shell_cmd("setprop fly.uvccam.dvr.res 640x360");
		lidbg_shell_cmd("setprop fly.uvccam.dvr.recpath "EMMC_MOUNT_POINT0"/camera_rec/");
		if(start_rec(cam_id,1))lidbg("%s:====return fail====\n",__func__);
		if(isOnline) 
		{
			msleep(3000);
			if(stop_rec(cam_id,0))lidbg("%s:====return fail====\n",__func__);
		}
		else 
		{
			msleep(3500);
			if(stop_rec(cam_id,1))lidbg("%s:====return fail====\n",__func__);
		}
		setDVRProp(cam_id);
		msleep(100);

		lidbg_shell_cmd("rm -f "EMMC_MOUNT_POINT0"/camera_rec/tmp*.h264&");
		if(!isDVRFirstResume) isDVRCheck = 1;
	}
	else if((cam_id == REARVIEW_ID) ||  (cam_id == REAR_BLOCK_ID_MODE))
	{
		if(!isRearFirstResume)
		{
			isRearCheck = 0;
			//lidbg_shell_cmd("/flysystem/lib/out/fw_update -2 -2&");
			//if(!wait_for_completion_timeout(&Rear_fw_get_wait , 3*HZ)) lidbg("%s:====Rear_fw_get_wait return fail====\n",__func__);
		}
		
		lidbg_shell_cmd("setprop fly.uvccam.rearview.res 640x360");
		lidbg_shell_cmd("setprop fly.uvccam.rearview.recpath "EMMC_MOUNT_POINT0"/");//must different path
		if(start_rec(cam_id,1))lidbg("%s:====return fail====\n",__func__);
		if(isOnline) 
		{
			msleep(2000);
			if(stop_rec(cam_id,0))lidbg("%s:====return fail====\n",__func__);
		}
		else 
		{
			msleep(5000);
			if(stop_rec(cam_id,1))lidbg("%s:====return fail====\n",__func__);
		}
		setDVRProp(cam_id);
		msleep(100);

		lidbg_shell_cmd("rm -f "EMMC_MOUNT_POINT0"/tmp*.h264&");
		if(!isRearFirstResume) isRearCheck = 1;
	}
	return;
}

/******************************************************************************
 * Function: work_DVR_fixScreenBlurred
 * Description: DVR Fix ScreenBlurred issue & Start exposure adaptation when camera first plug in.
 * Input parameters:
 *   	work	-	kernel work struct
 * Return values:
 *		none
 * Notes: none
 *****************************************************************************/
static void work_DVR_fixScreenBlurred(struct work_struct *work)
{
	char ret = -1;
	lidbg("%s:====E====\n",__func__);
	//if(isSuspend) mod_timer(&suspend_stoprec_timer,SUSPEND_STOPREC_ONLINE_TIME);
	if(isDVRFirstInit)
	{
		lidbg_shell_cmd("echo 1 > /dev/mc3xxx_enable0");
		pfly_UsbCamInfo->camStatus = lidbg_checkCam();
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
		{
			lidbg("%s:====None camera found!====\n",__func__);
			lidbg_shell_cmd("setprop lidbg.uvccam.dvr.status 0&");
			isDVRFirstInit = 0;/*no matter what,let the next work continue*/
			isDVRAfterFix = 1;
			return;
		}
		lidbg_shell_cmd("setprop lidbg.uvccam.dvr.osdset 1&");
		lidbg_shell_cmd("setprop lidbg.uvccam.dvr.status 1&");
	}
	/*Rec Block mode(First ACCON) : REAR_BLOCK_ID_MODE & DVR_BLOCK_ID_MODE*/
	if(isDVRACCRec && isDVRACCResume) 
	{
		lidbg("%s:====Don't need to fix Blurred!====\n",__func__);
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
		{
			if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->DVR_ready_wait_queue, (isDVRReady == 1), 10*HZ))
			{
				lidbg("%s:====ACCON None camera found!====\n",__func__);
				isDVRFirstResume = 0;
				isDVRACCResume = 0;
				isDVRAfterFix = 1;
				//complete(&auto_detect_wait);
				return;
			}
		}
	}
	else if(isDVRFirstResume)
	{
		fixScreenBlurred(DVR_BLOCK_ID_MODE,0);
		/*For Block mode*/
		pfly_UsbCamInfo->camStatus = lidbg_checkCam();
		if(!((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISSONIX))
		{
			lidbg("%s:====FirstResume None camera found!====\n",__func__);
			isDVRFirstResume = 0;
			isDVRACCResume = 0;
			isDVRAfterFix = 1;
			//complete(&auto_detect_wait);
			return;
		}
	}
	else 
		fixScreenBlurred(DVR_ID,0);

	/*Auto start*/
	if((isDVRFirstInit && isColdBootRec) || (isDVRACCResume && isDVRACCRec))
	{
		lidbg("%s:==FirstInit==\n",__func__);
		/*Wait for SDCard ready*/
		checkSDCardStatus(f_rec_path);
		while((ret =checkSDCardStatus(f_rec_path)) == 3)
	    {
	       if(1==g_var.android_boot_completed) break;
	       ssleep(1);
	       lidbg("%s:====Wait for SDCard ready!====\n",__func__);
	    }
		if(ret != 3)
		{
			if(!(isDVRFirstInit && isDualCam && !((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX)))
			{
				if(!isDVRRec)
				{
					lidbg("%s:==AUTO start==\n",__func__);
					dvr_start_recording();
				}
			}
		}
	}
	else if(isDVRPlugRec && !isOnlineRec)
	{
		dvr_start_recording();
	}

	//complete(&auto_detect_wait);
	isDVRFirstInit = 0;
	isDVRAfterFix = 1;
	isDVRFirstResume = 0;
	isDVRACCResume = 0;
	status_fifo_in(RET_DVR_SONIX);
	notify_online(RET_ONLINE_FOUND_SONIX);
	
	lidbg("%s:====X====\n",__func__);
	return;
}

/******************************************************************************
 * Function: work_RearView_fixScreenBlurred
 * Description: DVR Fix ScreenBlurred issue & Start exposure adaptation when camera first plug in.
 * Input parameters:
 *   	work	-	kernel work struct
 * Return values:
 *		none
 * Notes: none
 *****************************************************************************/
static void work_RearView_fixScreenBlurred(struct work_struct *work)
{
	char ret = -1;
	char temp_cmd[256];	
	lidbg("%s:====E====\n",__func__);
	if(isRearViewFirstInit)
	{
		pfly_UsbCamInfo->camStatus = lidbg_checkCam();
		if(!((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX))
		{
			lidbg("%s:====None camera found!====\n",__func__);
			lidbg_shell_cmd("setprop lidbg.uvccam.rear.status 0&");
			isRearViewFirstInit = 0;/*no matter what,let the next work continue*/
			isRearViewAfterFix = 1;
			return;
		}
		lidbg_shell_cmd("setprop lidbg.uvccam.rear.osdset 1&");
		lidbg_shell_cmd("setprop lidbg.uvccam.rear.status 1&");
	}
	/*Rec Block mode(First ACCON) : REAR_BLOCK_ID_MODE & DVR_BLOCK_ID_MODE*/
	if(isRearACCRec && isRearACCResume) 
	{
		lidbg("%s:====Don't need to fix Blurred!====\n",__func__);
		if(!((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX))
		{
			if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->Rear_ready_wait_queue, (isRearReady == 1), 10*HZ))
			{
				lidbg("%s:====ACCON None camera found!====\n",__func__);
				isRearFirstResume = 0;
				isRearACCResume = 0;
				isRearViewAfterFix = 1;
				return;
			}
		}
	}
	else if(isRearFirstResume)
	{
		fixScreenBlurred(REAR_BLOCK_ID_MODE,0);
		/*For Block mode*/
		pfly_UsbCamInfo->camStatus = lidbg_checkCam();
		if(!((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX))
		{
			lidbg("%s:====FirstResume None camera found!====\n",__func__);
			isRearFirstResume = 0;
			isRearACCResume = 0;
			isRearViewAfterFix = 1;
			return;
		}
	}
	else 
		fixScreenBlurred(REARVIEW_ID,0);

	/*Auto start*/
	if((isRearViewFirstInit && isColdBootRec) || (isRearACCResume && isRearACCRec))
	{
		lidbg("%s:==FirstInit==\n",__func__);
		/*Wait for SDCard ready*/
		checkSDCardStatus(f_rec_path);
		while((ret =checkSDCardStatus(f_rec_path)) == 3)
	    {
	       if(1==g_var.android_boot_completed) break;
	       ssleep(1);
	       lidbg("%s:====Wait for SDCard ready!====\n",__func__);
	    }
		if(ret != 3)
		{
			rear_start_recording();
			/*for HYUNDAI*/
			if(isThinkNavi)
			{
				if( !isRearRec)
				{
					lidbg("%s:==AUTO start==\n",__func__);
					//rear_start_recording();
					isRearVideoLoop = 0;
					sprintf(temp_cmd, "setprop persist.uvccam.isRearVideoLoop %d", isRearVideoLoop);
					lidbg_shell_cmd(temp_cmd);
				}
			}
			else
			{
				if( isDualCam && !isRearRec)
				{
					lidbg("%s:==AUTO start==\n",__func__);
					//rear_start_recording();
					isRearVideoLoop = 1;
					sprintf(temp_cmd, "setprop persist.uvccam.isRearVideoLoop %d", isRearVideoLoop);
					lidbg_shell_cmd(temp_cmd);
				}
			}
		}
	}
	else if(isRearPlugRec && !isOnlineRec)
	{
		rear_start_recording();
		if(isDualCam)
		{
			isRearVideoLoop = 1;
			sprintf(temp_cmd, "setprop persist.uvccam.isRearVideoLoop %d", isRearVideoLoop);
			lidbg_shell_cmd(temp_cmd);
		}
	}
	
	isRearViewFirstInit = 0;
	isRearViewAfterFix = 1;
	isRearFirstResume = 0;
	isRearACCResume = 0;
	status_fifo_in(RET_REAR_SONIX);	
	
	lidbg("%s:====X====\n",__func__);
	return;
}
#if 0
static void work_startRec(struct work_struct *work)
{
	lidbg("%s:====E====\n",__func__);
	if(isSuspend) mod_timer(&suspend_stoprec_timer,SUSPEND_STOPREC_ONLINE_TIME);
	if(start_rec(DVR_ID))lidbg("%s:====return fail====\n",__func__);
	lidbg("%s:====X====\n",__func__);
	return;
}

static void work_stopRec(struct work_struct *work)
{
	lidbg("%s:====E====\n",__func__);
	if(isSuspend) del_timer(&suspend_stoprec_timer);
	if(stop_rec())lidbg("%s:====return fail====\n",__func__);
	lidbg("%s:====X====\n",__func__);
	return;
}
#endif


static void work_format_done(struct work_struct *work)
{
	lidbg_shell_cmd("echo appcmd *158#097 > /dev/lidbg_drivers_dbg0");
	return;
}

/******************************************************************************
 * Function: setDVRProp
 * Description: Set DVR property for recording.
 * Input parameters:
 *   	camID	-	camera ID (0 : RearView , 1: DVR)
 * Return values:
 *		none
 * Notes: none
 *****************************************************************************/
static void setDVRProp(int camID)
{
	char temp_cmd[256];	
	if(camID == DVR_ID)
	{
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recbitrate %d", f_rec_bitrate);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.res %s", f_rec_res);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recpath %s", f_rec_path);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.rectime %d", f_rec_time);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recnum %d", f_rec_filenum);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recfilesize %d", f_rec_totalsize);
		lidbg_shell_cmd(temp_cmd);
	}
	else if(camID == REARVIEW_ID)
	{
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recbitrate %d", f_rec_bitrate);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.res %s", f_rec_res);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recpath %s", f_rec_path);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.rectime %d", f_rec_time);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recnum %d", f_rec_filenum);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recfilesize %d", f_rec_totalsize);
		lidbg_shell_cmd(temp_cmd);
	}
	sprintf(temp_cmd, "setprop fly.uvccam.isDualCam %d", isDualCam);
	lidbg_shell_cmd(temp_cmd);
}

/******************************************************************************
 * Function: setOnlineProp
 * Description: Set Online property for recording.
 * Input parameters:
 *   	camID	-	camera ID (0 : RearView , 1: DVR)
 * Return values:
 *		none
 * Notes: none
 *****************************************************************************/
static void setOnlineProp(int camID)
{
	char temp_cmd[256];	
	if(camID == DVR_ID)
	{
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recbitrate %d", f_online_bitrate);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.res %s", f_online_res);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recpath %s", f_online_path);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.rectime %d", f_online_time);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recnum %d", f_online_filenum);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.dvr.recfilesize %d", f_online_totalsize);
		lidbg_shell_cmd(temp_cmd);
	}
	else if(camID == REARVIEW_ID)
	{
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recbitrate %d", f_online_bitrate);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.res %s", f_online_res);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recpath %s", f_online_path);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.rectime %d", f_online_time);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recnum %d", f_online_filenum);
		lidbg_shell_cmd(temp_cmd);
		sprintf(temp_cmd, "setprop fly.uvccam.rearview.recfilesize %d", f_online_totalsize);
		lidbg_shell_cmd(temp_cmd);
	}
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
	struct file *storage_path, *file_path;
	if(!strncmp(path, EMMC_MOUNT_POINT0, strlen(EMMC_MOUNT_POINT0)))
	{
		storage_path = filp_open(EMMC_MOUNT_POINT0, O_RDONLY | O_DIRECTORY, 0);
		file_path = filp_open(path, O_RDONLY | O_DIRECTORY, 0);
		if(IS_ERR(storage_path))
		{
			lidbg("%s:EMMC ERR!!%ld\n",__func__,PTR_ERR(storage_path));
			ret = 1;
		}
		else if(IS_ERR(file_path))
		{
			lidbg("%s: New Rec Dir => %s\n",__func__,path);
			sprintf(temp_cmd, "mkdir -p %s", path);
			lidbg_shell_cmd(temp_cmd);
			sprintf(temp_cmd, "mkdir -p %s/BlackBox/.tmp", path);
			lidbg_shell_cmd(temp_cmd);
			ret = 3;
		}
		else lidbg("%s: Check Rec Dir OK => %s\n",__func__,path);
	}
	else if(!strncmp(path, EMMC_MOUNT_POINT1, strlen(EMMC_MOUNT_POINT1)))
	{
		storage_path = filp_open(EMMC_MOUNT_POINT1, O_RDONLY | O_DIRECTORY, 0);
		file_path = filp_open(path, O_RDONLY | O_DIRECTORY, 0);
		if(IS_ERR(storage_path))
		{
#if 0
			lidbg("%s:SDCARD1 ERR!!Reset to %s/camera_rec/\n",__func__, EMMC_MOUNT_POINT0);
			strcpy(path, EMMC_MOUNT_POINT0"/camera_rec/");
#else
			lidbg("%s:SDCARD1 ERR!!\n",__func__);
			strcpy(path, EMMC_MOUNT_POINT1"/camera_rec/");
#endif
			ret = 2;
		}
		else if(IS_ERR(file_path))
		{
			lidbg("%s: New Rec Dir => %s\n",__func__,path);
			sprintf(temp_cmd, "mkdir -p %s", path);
			lidbg_shell_cmd(temp_cmd);
			sprintf(temp_cmd, "mkdir -p %s/BlackBox/.tmp", path);
			lidbg_shell_cmd(temp_cmd);
			ret = 3;
		}
		else lidbg("%s: Check Rec Dir OK => %s\n",__func__,path);
	}
	if(!IS_ERR(storage_path)) filp_close(storage_path, 0);
	if(!IS_ERR(file_path)) filp_close(file_path, 0);
	return ret;
}

static int dvr_start_recording(void)
{
	int ret_st = -1;
	lidbg("%s:DVR CMD_START_REC\n",__func__);
	setDVRProp(DVR_ID);
	ret_st = checkSDCardStatus(f_rec_path);
	if(ret_st == 2 || ret_st == 1) return RET_FAIL;
	if(isDVRRec)
	{
		lidbg("%s:====DVR start cmd repeatedly====\n",__func__);
		return RET_REPEATREQ;
	}
	else if(isOnlineRec) 
	{
		lidbg("%s:====DVR restart rec====\n",__func__);
		isDVRRec = 1;
		isOnlineRec = 0;
		if(stop_rec(DVR_ID,1)) return -1;
		//msleep(700);
		if(start_rec(DVR_ID,1)) return -1;
		notify_online(RET_ONLINE_INTERRUPTED);
	}
	else 
	{
		lidbg("%s:====DVR start rec====\n",__func__);
		isDVRRec = 1;
		if(start_rec(DVR_ID,1)) return -1;
	}
	return RET_SUCCESS;
}

static int dvr_stop_recording(void)
{
	lidbg("%s:DVR CMD_STOP_REC\n",__func__);
	if(isDVRRec)
	{
		lidbg("%s:====DVR stop rec====\n",__func__);
		if(stop_rec(DVR_ID,1)) return -1;
		isDVRRec = 0;
	}
	else if(isOnlineRec) 
	{
		lidbg("%s:====DVR stop cmd neglected====\n",__func__);
		return RET_IGNORE;
	}
	else
	{
		lidbg("%s:====DVR stop cmd repeatedly====\n",__func__);
		return RET_REPEATREQ;
	}
	return RET_SUCCESS;
}

static int rear_start_recording(void)
{
	int ret_st = -1;
	lidbg("%s:Rear CMD_START_REC\n",__func__);
	if(isPrevYUV > 0)
	{
		lidbg("%s:Not enough bandwidth for rear recording when preview use YUV!\n",__func__);
		return RET_SUCCESS;
	}
	setDVRProp(REARVIEW_ID);
	ret_st = checkSDCardStatus(f_rec_path);
	if(ret_st == 2 || ret_st == 1) return RET_FAIL;
	if(isRearRec)
	{
		lidbg("%s:====Rear start cmd repeatedly====\n",__func__);
		return RET_REPEATREQ;
	}
	else 
	{
		lidbg("%s:====Rear start rec====\n",__func__);
		isRearRec = 1;
		if(start_rec(REARVIEW_ID,1)) return -1;
	}
	return RET_SUCCESS;
}

static int rear_stop_recording(void)
{
	lidbg("%s:Rear CMD_STOP_REC\n",__func__);
	if(isRearRec)
	{
		lidbg("%s:====Rear stop rec====\n",__func__);
		if(stop_rec(REARVIEW_ID,1)) return -1;
		isRearRec = 0;
	}
	else
	{
		lidbg("%s:====Rear stop cmd repeatedly====\n",__func__);
		return RET_REPEATREQ;
	}
	return RET_SUCCESS;
}

static void dvr_fail_proc(void)
{
	lidbg("%s:===E===\n",__func__);
	stop_rec(DVR_ID,1);
	isDVRRec = 0;
	isOnlineRec = 0;
	return;
}

static void rear_fail_proc(void)
{
	lidbg("%s:===E===\n",__func__);
	stop_rec(REARVIEW_ID,1);
	isRearRec = 0;
	return;
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
	//lidbg("=====camStatus => %d======\n",pfly_UsbCamInfo->camStatus);
	if(_IOC_TYPE(cmd) == FLYCAM_FRONT_REC_IOC_MAGIC)//front cam recording mode
	{
		if((_IOC_NR(cmd) == NR_START_REC) || (_IOC_NR(cmd) == NR_STOP_REC) )
		{
			/*check camera status before doing ioctl*/
			if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
			{
				lidbg("%s:DVR not found,ioctl fail!\n",__func__);
				return RET_NOTVALID;
			}
			else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
			{
				lidbg("%s:Is not SonixCam ,ioctl fail!\n",__func__);
				return RET_NOTSONIX;
			}
			else if((pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRAfterFix)
			{
				lidbg("%s:Fix proc running!But ignore !\n",__func__);
				//return RET_NOTVALID;
				isDVRAfterFix = 1;//force to 1 tmp
			}
		}
		
		switch(_IOC_NR(cmd))
		{
			case NR_BITRATE:
				lidbg("%s:DVR NR_REC_BITRATE = [%ld]\n",__func__,arg);
				f_rec_bitrate = arg;
		        break;
		    case NR_RESOLUTION:
				lidbg("%s:DVR NR_REC_RESOLUTION  = [%s]\n",__func__,(char*)arg);
				strcpy(f_rec_res,(char*)arg);
		        break;
			case NR_PATH:
				lidbg("%s:DVR NR_REC_PATH  = [%s]\n",__func__,(char*)arg);
				ret_st = checkSDCardStatus((char*)arg);
				if(ret_st != 1) 
					strcpy(f_rec_path,(char*)arg);
				else
					lidbg("%s: f_rec_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) ret = RET_FAIL;
		        break;
			case NR_TIME:
				lidbg("%s:DVR NR_REC_TIME = [%ld]\n",__func__,arg);
				f_rec_time = arg;
		        break;
			case NR_FILENUM:
				lidbg("%s:DVR NR_REC_FILENUM = [%ld]\n",__func__,arg);
				f_rec_filenum = arg;
		        break;
			case NR_TOTALSIZE:
				lidbg("%s:DVR NR_REC_TOTALSIZE = [%ld]\n",__func__,arg);
				f_rec_totalsize= arg;
		        break;
			case NR_ISDUALCAM:
				lidbg("%s:DVR NR_ISDUALCAM\n",__func__);
				isDualCam = arg;
		        break;
			case NR_ISCOLDBOOTREC:
				lidbg("%s:DVR NR_ISCOLDBOOTREC\n",__func__);
				isColdBootRec= arg;
		        break;
			case NR_ISEMPERMITTED:
				lidbg("%s:DVR NR_ISEMPERMITTED\n",__func__);
				isEmRecPermitted= arg;
		        break;
			case NR_ISVIDEOLOOP:
				lidbg("%s:DVR NR_ISVIDEOLOOP %ld\n",__func__,arg);
				isDVRVideoLoop= arg;
				break;
			case NR_START_REC:
		        lidbg("%s:DVR NR_START_REC\n",__func__);
				setDVRProp(DVR_ID);
				ret_st = checkSDCardStatus(f_rec_path);
				if(ret_st == 2 || ret_st == 1) return RET_FAIL;
				
				if(isDVRRec)
				{
					lidbg("%s:====DVR start cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
				else if(isOnlineRec) 
				{
					lidbg("%s:====DVR restart rec====\n",__func__);
					isDVRRec = 1;
					isOnlineRec = 0;
					if(stop_rec(DVR_ID,1)) goto dvrfailproc;
					if(start_rec(DVR_ID,1)) goto dvrfailproc;
					notify_online(RET_ONLINE_INTERRUPTED);
				}
				else 
				{
					lidbg("%s:====DVR start rec====\n",__func__);
					isDVRRec = 1;
					if(start_rec(DVR_ID,1)) goto dvrfailproc;
				}
		        break;
			case NR_STOP_REC:
		        lidbg("%s:DVR NR_STOP_REC\n",__func__);
				if(isDVRRec)
				{
					lidbg("%s:====DVR stop rec====\n",__func__);
					if(stop_rec(DVR_ID,1)) goto dvrfailproc;
					isDVRRec = 0;
				}
				else if(isOnlineRec) 
				{
					lidbg("%s:====DVR stop cmd neglected====\n",__func__);
					ret = RET_IGNORE;
				}
				else
				{
					lidbg("%s:====DVR stop cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
		        break;
			case NR_SET_PAR:
				lidbg("%s:DVR NR_SET_PAR\n",__func__);
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
				lidbg("%s:DVR NR_GET_RES\n",__func__);
				invoke_AP_ID_Mode(DVR_GET_RES_ID_MODE);
				if(!wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ)) ret = RET_FAIL;
				strcpy((char*)arg,camera_DVR_res);
				lidbg("%s:DVR NR_GET_RES => %s\n",__func__,(char*)arg);
		        break;
			case NR_SATURATION:
				lidbg("%s:DVR NR_SATURATION\n",__func__);
				lidbg("saturationVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set saturation=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_TONE:
				lidbg("%s:DVR NR_TONE\n",__func__);
				lidbg("hueVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set hue=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_BRIGHT:
				lidbg("%s:DVR NR_BRIGHT\n",__func__);
				lidbg("brightVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set bright=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_CONTRAST:
				lidbg("%s:DVR NR_CONTRAST\n",__func__);
				lidbg("contrastVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --ef-set contrast=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
		    default:
		        return -ENOTTY;
		}
	}
	else if(_IOC_TYPE(cmd) == FLYCAM_FRONT_ONLINE_IOC_MAGIC)//front cam online mode
	{
		char temp_cmd[256];
		if(isSuspend && ((_IOC_NR(cmd) == NR_START_REC) ||(_IOC_NR(cmd) == NR_CAPTURE)))
		{
			lidbg("%s:====udisk_request==suspend online==\n",__func__);
			lidbg_shell_cmd("echo 'udisk_request' > /dev/flydev0");
			lidbg("%s:====ACCOFF CHECK====\n",__func__);
			if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->DVR_ready_wait_queue, (isDVRReady == 1), 10*HZ))
			{
				lidbg("%s:====udisk_unrequest==suspend online timeout==\n",__func__);
				if(!isDVRRec) lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
				return RET_NOTVALID;
			}
		}
		if(!isSuspend)
		{
			/*check camera status before doing ioctl*/
			if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
			{
				lidbg("%s:DVR[online] not found,ioctl fail!\n",__func__);
				return RET_NOTVALID;
			}
			if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
			{
				lidbg("%s:is not SonixCam ,ioctl fail!\n",__func__);
				return RET_NOTSONIX;
			}
		}
		switch(_IOC_NR(cmd))
		{
			case NR_BITRATE:
				lidbg("%s:Online NR_REC_BITRATE = [%ld]\n",__func__,arg);
				f_online_bitrate = arg;
		        break;
		    case NR_RESOLUTION:
				lidbg("%s:Online NR_REC_RESOLUTION  = [%s]\n",__func__,(char*)arg);
				strcpy(f_online_res,(char*)arg);
		        break;
			case NR_PATH:
				lidbg("%s:Online NR_REC_PATH  = [%s]\n",__func__,(char*)arg);
				/*
				ret_st = checkSDCardStatus((char*)arg);
				if(ret_st != 1) 
					strcpy(f_online_path,(char*)arg);
				else
					lidbg("%s: f_online_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) ret = RET_FAIL;
				*/
				strcpy(f_online_path,(char*)arg);
		        break;
			case NR_TIME:
				lidbg("%s:Online NR_REC_TIME = [%ld]\n",__func__,arg);
				f_online_time = arg;
		        break;
			case NR_FILENUM:
				lidbg("%s:Online NR_REC_FILENUM = [%ld]\n",__func__,arg);
				f_online_filenum = arg;
		        break;
			case NR_TOTALSIZE:
				lidbg("%s:Online NR_REC_TOTALSIZE = [%ld]\n",__func__,arg);
				f_online_totalsize= arg;
		        break;
			case NR_START_REC:
		        lidbg("%s:Online NR_START_REC\n",__func__);
				//checkSDCardStatus(f_online_path);
				if(isDVRRec)
				{
					lidbg("%s:====Online start cmd neglected====\n",__func__);
					ret = RET_IGNORE;
				}
				else if(isOnlineRec) 
				{
					lidbg("%s:====Online start cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
				else
				{
					lidbg("%s:====Online start rec====\n",__func__);
					isOnlineRec = 1;
					if(isSuspend) fixScreenBlurred(DVR_ID,1);
					setOnlineProp(DVR_ID);
					if(start_rec(DVR_ID,1)) goto dvrfailproc;
					msleep(10);
					setDVRProp(DVR_ID);
				}
		        break;
			case NR_STOP_REC:
		        lidbg("%s:Online NR_STOP_REC\n",__func__);
				if(isDVRRec)
				{
					lidbg("%s:====Online stop cmd neglected====\n",__func__);
#if 1
					if(isSuspend) 
					{
						lidbg("%s:====udisk_unrequest===neglected=\n",__func__);
						if(!isDVRRec) lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
					}
#endif
					ret = RET_IGNORE;
				}
				else if(isOnlineRec) 
				{
					lidbg("%s:====Online stop rec====\n",__func__);
					if(stop_rec(DVR_ID,1)) goto dvrfailproc;
					isOnlineRec = 0;
				}
				else
				{
					lidbg("%s:====Online stop cmd repeatedly====\n",__func__);
#if 1
					if(isSuspend)
					{
						lidbg("%s:====udisk_unrequest===repeatedly=\n",__func__);
						if(!isDVRRec) lidbg_shell_cmd("echo 'udisk_unrequest' > /dev/flydev0");
					}
#endif
					ret = RET_REPEATREQ;
				}
		        break;
			case NR_CAPTURE:
				lidbg("%s:Online NR_CAPTURE\n",__func__);
				lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video0 -b 1 -c -f mjpg -S");
				break;
			case NR_CAPTURE_PATH:
				lidbg("%s:Online NR_CAPTURE_PATH  = [%s]\n",__func__,(char*)arg);
				sprintf(temp_cmd, "mkdir -p %s", (char*)arg);
				lidbg_shell_cmd(temp_cmd);

				file_path = filp_open((char*)arg, O_RDONLY | O_DIRECTORY, 0);
				if(IS_ERR(file_path))
				{
					lidbg("%s:CAPTURE_PATH ERR!!\n",__func__);
					lidbg_shell_cmd("setprop persist.uvccam.capturepath "EMMC_MOUNT_POINT0"/preview_cache/");
				}
				else
				{
					lidbg("%s:CAPTURE_PATH OK!!\n",__func__);
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
				lidbg("%s:Online NR_CAPTURE\n",__func__);
				lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video0 -b 0  -c -f mjpg -S");
				break;
			default:
		        return -ENOTTY;
		}
	}

	if(_IOC_TYPE(cmd) == FLYCAM_REAR_REC_IOC_MAGIC)//front cam recording mode
	{
		if((_IOC_NR(cmd) == NR_START_REC) || (_IOC_NR(cmd) == NR_STOP_REC) )
		{
			/*check camera status before doing ioctl*/
			if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
			{
				lidbg("%s:Rear not found,ioctl fail!\n",__func__);
				return RET_NOTVALID;
			}
			else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
			{
				lidbg("%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
				return RET_NOTSONIX;
			}
			else if(((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX) && !isRearViewAfterFix)
			{
				lidbg("%s:Rear Fix proc running!But ignore !\n",__func__);
				//return RET_NOTVALID;
				isRearViewAfterFix = 1;//force to 1 tmp
			}
		}
		
		switch(_IOC_NR(cmd))
		{
			case NR_BITRATE:
				lidbg("%s:Rear NR_REC_BITRATE = [%ld]\n",__func__,arg);
				r_rec_bitrate = arg;
		        break;
		    case NR_RESOLUTION:
				lidbg("%s:Rear NR_REC_RESOLUTION  = [%s]\n",__func__,(char*)arg);
				strcpy(r_rec_res,(char*)arg);
		        break;
			case NR_PATH:
				lidbg("%s:Rear NR_REC_PATH  = [%s]\n",__func__,(char*)arg);
				ret_st = checkSDCardStatus((char*)arg);
				if(ret_st != 1) 
					strcpy(r_rec_path,(char*)arg);
				else
					lidbg("%s: r_rec_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) ret = RET_FAIL;
		        break;
			case NR_TIME:
				lidbg("%s:Rear NR_REC_TIME = [%ld]\n",__func__,arg);
				r_rec_time = arg;
		        break;
			case NR_FILENUM:
				lidbg("%s:Rear NR_REC_FILENUM = [%ld]\n",__func__,arg);
				r_rec_filenum = arg;
		        break;
			case NR_TOTALSIZE:
				lidbg("%s:Rear NR_REC_TOTALSIZE = [%ld]\n",__func__,arg);
				r_rec_totalsize= arg;
		        break;
			case NR_ISVIDEOLOOP:
				lidbg("%s:Rear NR_ISVIDEOLOOP %ld\n",__func__,arg);
				isRearVideoLoop= arg;
				break;
			case NR_START_REC:
		        lidbg("%s:Rear NR_START_REC\n",__func__);
				setDVRProp(REARVIEW_ID);
				ret_st = checkSDCardStatus(f_rec_path);
				if(ret_st == 2 || ret_st == 1) return RET_FAIL;
				if(isRearRec)
				{
					lidbg("%s:====Rear start cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
				else 
				{
					lidbg("%s:====Rear start rec====\n",__func__);
					isRearRec = 1;
					if(start_rec(REARVIEW_ID,1)) goto rearfailproc;
				}
		        break;
			case NR_STOP_REC:
		        lidbg("%s:Rear NR_STOP_REC\n",__func__);
				if(isRearRec)
				{
					lidbg("%s:====Rear stop rec====\n",__func__);
					if(stop_rec(REARVIEW_ID,1)) goto rearfailproc;
					isRearRec = 0;
				}
				else
				{
					lidbg("%s:====Rear stop cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
		        break;
			case NR_SET_PAR:
				lidbg("%s:Rear NR_SET_PAR\n",__func__);
				if(isRearRec)
				{
					if(stop_rec(REARVIEW_ID,1)) goto rearfailproc;
					setDVRProp(REARVIEW_ID);
					if(start_rec(REARVIEW_ID,1)) goto rearfailproc;
				}
		        break;
			case NR_GET_RES:
				lidbg("%s:Rear NR_GET_RES\n",__func__);
				invoke_AP_ID_Mode(REAR_GET_RES_ID_MODE);
				if(!wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ)) ret = RET_FAIL;
				strcpy((char*)arg,camera_rear_res);
				lidbg("%s:Rear NR_GET_RES => %s\n",__func__,(char*)arg);
		        break;
			case NR_SATURATION:
				lidbg("%s:Rear NR_SATURATION\n",__func__);
				lidbg("saturationVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set saturation=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_TONE:
				lidbg("%s:Rear NR_TONE\n",__func__);
				lidbg("hueVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set hue=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_BRIGHT:
				lidbg("%s:Rear NR_BRIGHT\n",__func__);
				lidbg("brightVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set bright=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
			case NR_CONTRAST:
				lidbg("%s:Rear NR_CONTRAST\n",__func__);
				lidbg("contrastVal = %ld\n",arg);
				sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --ef-set contrast=%ld ", arg);
				lidbg_shell_cmd(temp_cmd);
		        break;
		    default:
		        return -ENOTTY;
		}
	}
	
	else if(_IOC_TYPE(cmd) == FLYCAM_STATUS_IOC_MAGIC)//Rec TestAp status
	{
		switch(_IOC_NR(cmd))
		{
			case NR_STATUS:
		        lidbg("%s:NR_STATUS\n",__func__);
				/*Check Update proc*/
				if(arg == RET_DVR_UD_SUCCESS || arg == RET_DVR_UD_FAIL ||arg == RET_DVR_FW_ACCESS_FAIL ||
					arg == RET_REAR_UD_SUCCESS ||arg == RET_REAR_UD_FAIL ||arg == RET_REAR_FW_ACCESS_FAIL )
					isUpdating = 0;
				status_fifo_in(arg);
		        break;
			case NR_ACCON_CAM_READY:/*R/W:	R[0-OK;1-Timout]	W[0-Rear;1-DVR] -> Wait for 10s*/
		        lidbg("%s:NR_ACCON_CAM_READY  E cam_id=> %ld\n",__func__,arg);
				if(arg == DVR_ID)
				{
					lidbg("DVR_ready_wait waiting\n");
					if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
					{
						//if(!wait_for_completion_timeout(&DVR_ready_wait , 10*HZ)) ret = 1;
						if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->DVR_ready_wait_queue, (isDVRReady == 1), 10*HZ))
							ret = 1;
					}
					else lidbg("DVR Camera already ready.\n");
				}
				else if(arg == REARVIEW_ID)
				{
					lidbg("Rear_ready_wait waiting\n");
					if(!((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX))
					{
						if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->Rear_ready_wait_queue, (isRearReady == 1), 10*HZ))
							ret = 1;
					}
					else lidbg("Rear Camera already ready.\n");
				}
				 lidbg("%s:NR_ACCON_CAM_READY  X \n",__func__);
		        break;
			case NR_DVR_FW_VERSION:
		        lidbg("%s:NR_DVR_FW_VERSION\n",__func__);
				strcpy(camera_DVR_fw_version,(char*)arg);
				sprintf(temp_cmd, "am broadcast -a com.lidbg.flybootserver.action --es toast DVR_%s&",camera_DVR_fw_version );
				lidbg_shell_cmd(temp_cmd);
				complete(&DVR_fw_get_wait);/*HAL get version*/
		        break;
			case NR_REAR_FW_VERSION:
		        lidbg("%s:NR_REAR_FW_VERSION\n",__func__);
				strcpy(camera_rear_fw_version,(char*)arg);
				sprintf(temp_cmd, "am broadcast -a com.lidbg.flybootserver.action --es toast REAR_%s&",camera_rear_fw_version );
				lidbg_shell_cmd(temp_cmd);
				complete(&Rear_fw_get_wait);/*HAL get version*/
		        break;
			case NR_DVR_RES:
		        lidbg("%s:NR_DVR_RES\n",__func__);
				strcpy(camera_DVR_res,(char*)arg);
				complete(&DVR_res_get_wait);/*HAL get version*/
		        break;
			case NR_REAR_RES:
		        lidbg("%s:NR_REAR_RES\n",__func__);
				strcpy(camera_rear_res,(char*)arg);
				complete(&Rear_res_get_wait);/*HAL get version*/
		        break;
			case NR_ONLINE_NOTIFY:
		        lidbg("%s:NR_ONLINE_NOTIFY\n",__func__);
				wait_event_interruptible(pfly_UsbCamInfo->onlineNotify_wait_queue, isOnlineNotifyReady == 1);
          		//arg = pfly_UsbCamInfo->onlineNotify_status;
				//lidbg("%s:NR_ONLINE_NOTIFY=ss==2=%ld\n",__func__,arg);
				isOnlineNotifyReady = 0;
				return pfly_UsbCamInfo->onlineNotify_status;
		        break;
			case NR_ONLINE_INVOKE_NOTIFY:
		        lidbg("%s:NR_ONLINE_INVOKE_NOTIFY arg = %ld\n",__func__,arg);
				notify_online(arg);
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
		        lidbg("%s:NR_VERSION cam_id = [%d]\n",__func__,((char*)arg)[0]);
				strcpy((char*)arg + 1,"123456");
				if(((char*)arg)[0] == DVR_ID)
					strcpy((char*)arg + 1,camera_DVR_fw_version);
				else if(((char*)arg)[0]== REARVIEW_ID)
					strcpy((char*)arg + 1,camera_rear_fw_version);
				else strcpy((char*)arg + 1,camera_DVR_fw_version);

				if(!strncmp((char*)arg + 1, "NONE", 4))
					ret = RET_FAIL;

				lidbg("%s:NR_VERSION FwVer:[%s]\n",__func__,(char*)arg + 1);

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
		        lidbg("%s:NR_UPDATE\n",__func__);
				if(isUpdating) 
				{
					lidbg("%s:Camera it's updating!Rejective.\n",__func__);
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
		int length = 0;
		//struct mounted_volume *sdcard1 = NULL;
		
		dvrRespond[0] = ((char*)arg)[0];
		rearRespond[0] = ((char*)arg)[0];
		dvrRespond[1] = DVR_ID;
		rearRespond[1] = REARVIEW_ID;		
		switch(_IOC_NR(cmd))
		{
			case NR_CMD:
		        lidbg("%s:NR_CMD__cmd=>[0x%x]__camID=>[0x%x]\n",__func__,((char*)arg)[0],((char*)arg)[1]);
				/*check dvr camera status before doing ioctl*/
				if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
				{
					lidbg("%s:DVR not found,ioctl fail!\n",__func__);
					dvrRespond[2] = RET_NOTVALID;
				}
				else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
				{
					lidbg("%s:Is not SonixCam ,ioctl fail!\n",__func__);
					dvrRespond[2] = RET_NOTSONIX;
				}
				else if((pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRAfterFix)
				{
					lidbg("%s:Fix proc running!But ignore !\n",__func__);
					//return RET_NOTVALID;
					isDVRAfterFix = 1;//force to 1 tmp
				}

				/*check rear camera status before doing ioctl*/
				if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
				{
					lidbg("%s:Rear not found,ioctl fail!\n",__func__);
					rearRespond[2] = RET_NOTVALID;
				}
				else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
				{
					lidbg("%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
					rearRespond[2] = RET_NOTSONIX;
				}
				else if(((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX) && !isRearViewAfterFix)
				{
					lidbg("%s:Rear Fix proc running!But ignore !\n",__func__);
					//return RET_NOTVALID;
					isRearViewAfterFix = 1;//force to 1 tmp
				}
				switch(((char*)arg)[0])
				{
					case CMD_RECORD:
						isUIStartRec = ((char*)arg)[1];
						if(isUIStartRec == 1)
						{
							if(isDualCam)
							{
								if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID)	isDVRVideoLoop = 1;
								else	isDVRVideoLoop = 0;
								if((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID)	isRearVideoLoop = 1;
								else	isRearVideoLoop = 0;
								dvrRespond[3] = isDVRVideoLoop;
								rearRespond[3] = isRearVideoLoop;
							}
							else
							{
								if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID)	isDVRVideoLoop = 1;
								else	isDVRVideoLoop = 0;
								isRearVideoLoop = 0;
								dvrRespond[3] = isDVRVideoLoop;
								rearRespond[3] = isRearVideoLoop;
							}
						}
						else if(isUIStartRec == 0)
						{
							isDVRVideoLoop = 0;
							isRearVideoLoop = 0;
							dvrRespond[3] = isDVRVideoLoop;
							rearRespond[3] = isRearVideoLoop;
						}
						//dvrRespond[3] = isDVRRec;
						//rearRespond[3] = isRearRec;
						memcpy(returnRespond + length,dvrRespond,4);
						length += 4;
						memcpy(returnRespond + length,rearRespond,4);
						length += 4;
						if(copy_to_user((char*)arg,returnRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						sprintf(temp_cmd, "setprop persist.uvccam.isDVRVideoLoop %d", isDVRVideoLoop);
						lidbg_shell_cmd(temp_cmd);
						sprintf(temp_cmd, "setprop persist.uvccam.isRearVideoLoop %d", isRearVideoLoop);
						lidbg_shell_cmd(temp_cmd);
						//mod_timer(&ui_start_rec_timer,UI_REC_WAIT_TIME);
						break;

					case CMD_CAPTURE:
						lidbg("%s:====CMD_CAPTURE====\n",__func__);
						length += 2;
						break;

					case CMD_SET_RESOLUTION:
						lidbg("%s:CMD_SET_RESOLUTION  = [%s]\n",__func__,(char*)arg + 1);
						strcpy(f_rec_res,(char*)arg + 1);
						length += 100;
						memcpy(dvrRespond + 3,(char*)arg + 1,length);
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_TIME_SEC:
						lidbg("%s:CMD_TIME_SEC = [%d]\n",__func__,(((char*)arg)[1] << 8) + ((char*)arg)[2]);
						f_rec_time = (((char*)arg)[1] << 8) + ((char*)arg)[2];
						memcpy(dvrRespond + 3,(char*)arg + 1,2);
						length += 5;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_FW_VER:
						lidbg("%s:CMD_FW_VER dvr_Fw=> %s\n",__func__,camera_DVR_fw_version);
						strcpy(dvrRespond + 3,camera_DVR_fw_version);
						length += 100;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_TOTALSIZE:
						lidbg("%s:CMD_TOTALSIZE\n",__func__);
						f_rec_totalsize= ((((char*)arg)[1] << 24) + (((char*)arg)[2] << 16) + (((char*)arg)[3] << 8) + ((char*)arg)[4]);
						lidbg("%s:CMD_TOTALSIZE => %dMB\n",__func__,f_rec_totalsize);
						memcpy(dvrRespond + 3,(char*)arg + 1,4);
						length += 7;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_PATH:
						lidbg("%s:CMD_PATH\n",__func__);
						ret_st = checkSDCardStatus((char*)arg + 1);
						if(ret_st != 1) 
							strcpy(f_rec_path,(char*)arg + 1);
						else
							lidbg("%s: f_rec_path access wrong! %d", __func__ ,EFAULT);//not happend
						if(ret_st > 0) dvrRespond[2] = RET_FAIL;
						strcpy(dvrRespond + 3,f_rec_path);
						length += 100;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_SET_PAR:
						lidbg("%s:CMD_SET_PAR\n",__func__);						
						initMsg[length] = 0xB0;
						length++;
						dvrRespond[0] = CMD_RECORD;
						rearRespond[0] = CMD_RECORD;
						dvrRespond[3] = isDVRVideoLoop;
						rearRespond[3] = isRearVideoLoop;
						
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
						initMsg[length] = isDualCam;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;
						
						lidbg("%s:CMD_SET_PAR ;1:0x%x,2:0x%x,3:0x%x,4:0x%x,5:0x%x,6:0x%x,7:0x%x,\n",__func__,initMsg[0],initMsg[1],initMsg[2],initMsg[3],initMsg[4],initMsg[5],initMsg[6]);
						lidbg("%s:length = %d\n",__func__,length);
						if(copy_to_user((char*)arg,initMsg,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						mod_timer(&set_par_timer,SET_PAR_WAIT_TIME);
						break;

					case CMD_GET_RES:
						lidbg("%s:CMD_GET_RES\n",__func__);
						invoke_AP_ID_Mode(DVR_GET_RES_ID_MODE);
						if(!wait_for_completion_timeout(&DVR_res_get_wait , 3*HZ)) dvrRespond[2] = RET_FAIL;
						strcpy(dvrRespond + 3,camera_DVR_res);
						lidbg("%s:DVR NR_GET_RES => %s\n",__func__,dvrRespond + 3);
						
						length += 100;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;

					case CMD_SET_EFFECT:
						lidbg("%s:CMD_SET_EFFECT\n",__func__);
						length += 2;
						break;

					case CMD_DUAL_CAM:
						lidbg("%s:CMD_DUAL_CAM\n",__func__);
						if(((char*)arg)[1] == 1) isDualCam = 1;
						else isDualCam = 0;
						length += 2;
						break;
					case CMD_FORMAT_SDCARD:
						lidbg("%s:CMD_FORMAT_SDCARD\n",__func__);
#if 0						
						sdcard1 = find_mounted_volume_by_mount_point("/mnt/media_rw/sdcard1") ;
   						if(sdcard1 == NULL)
						{
							lidbg("%s:sdcard not exist!\n",__func__);
							dvrRespond[1] = 0;
						}
#endif							
						//else
						//{
							sprintf(temp_cmd, "setprop lidbg.uvccam.isFormat %d", 1);
							lidbg_shell_cmd(temp_cmd);
							if(!strncmp(f_rec_path, EMMC_MOUNT_POINT1, strlen(EMMC_MOUNT_POINT1)))
							{
								/*
								if(isDVRRec)
								{
									isBeforeFormatDVRRec = 1;
									dvr_stop_recording();
								}
								if(isRearRec)
								{
									isBeforeFormatRearRec = 1;
									rear_stop_recording();
								}
								*/
								isBeforeFormatDVRRec = isDVRRec;
								isUIStartRec = 0;
								complete(&ui_start_rec_wait);
							}
							if(((char*)arg)[1] == 1) 
							{
								lidbg("%s:Begin format sdcard!\n",__func__);
								//lidbg_shell_cmd("echo appcmd *158#097 > /dev/lidbg_drivers_dbg0");
								schedule_delayed_work(&work_t_format_done, 3*HZ);
								dvrRespond[1] = 1;
							}
							else
							{
								lidbg("%s:Stop format sdcard!\n",__func__);
								dvrRespond[1] = 0;
							}
						//}
						
						//schedule_delayed_work(&work_t_format_done, 5*HZ);
						
						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_EM_EVENT_SWITCH:
						lidbg("%s:CMD_EM_EVENT_SWITCH\n",__func__);
						if(((char*)arg)[1] == 0) 
						{
							lidbg("%s:Emergency event permitted!\n",__func__);
							isEmRecPermitted = 1;
							dvrRespond[1] = !isEmRecPermitted;
						}
						else
						{
							lidbg("%s:Emergency event rejected!\n",__func__);
							isEmRecPermitted = 0;
							dvrRespond[1] = !isEmRecPermitted;
						}
						sprintf(temp_cmd, "setprop persist.uvccam.isEmNotPermit %d", !isEmRecPermitted);
						lidbg_shell_cmd(temp_cmd);
						length += 2;
						if(copy_to_user((char*)arg,dvrRespond,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
						}
						break;
					case CMD_AUTO_DETECT:
						lidbg("%s:CMD_AUTO_DETECT\n",__func__);
						//if(isDVRFirstResume)	msleep(1500);
						//if(isDVRACCResume) ssleep(10);
						//else msleep(3000);
						//wait_for_completion_timeout(&auto_detect_wait , 12*HZ);
						//if(isDVRFirstResume) ssleep(8);
						ssleep(4);

						/*check dvr camera status before doing ioctl*/
						if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
						{
							lidbg("%s:DVR not found,ioctl fail!\n",__func__);
							dvrRespond[2] = RET_NOTVALID;
						}
						else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
						{
							lidbg("%s:Is not SonixCam ,ioctl fail!\n",__func__);
							dvrRespond[2] = RET_NOTSONIX;
						}
						else dvrRespond[2] = RET_SUCCESS;

						/*check rear camera status before doing ioctl*/
						if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
						{
							lidbg("%s:Rear not found,ioctl fail!\n",__func__);
							rearRespond[2] = RET_NOTVALID;
						}
						else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
						{
							lidbg("%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
							rearRespond[2] = RET_NOTSONIX;
						}
						else rearRespond[2] = RET_SUCCESS;
						
						initMsg[length] = 0xB0;
						length++;
						dvrRespond[0] = CMD_RECORD;
						rearRespond[0] = CMD_RECORD;
						dvrRespond[3] = isDVRVideoLoop;
						rearRespond[3] = isRearVideoLoop;
						
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
						initMsg[length] = isDualCam;
						length++;
						/*------msgTAIL------*/
						initMsg[length] = ';';
						length++;

						initMsg[length] = CMD_EM_EVENT_SWITCH;
						length++;
						initMsg[length] = !isEmRecPermitted;
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
						lidbg("%s:CMD_AUTO_DETECT ;1:0x%x,2:0x%x,3:0x%x,4:0x%x,5:0x%x,6:0x%x,7:0x%x,\n",__func__,initMsg[0],initMsg[1],initMsg[2],initMsg[3],initMsg[4],initMsg[5],initMsg[6]);
						lidbg("%s:length = %d\n",__func__,length);
						if(copy_to_user((char*)arg,initMsg,length))
						{
							lidbg("%s:copy_to_user ERR\n",__func__);
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

		//lidbg("%s:FLYCAM_EM_MAGIC\n",__func__);
		/*check dvr camera status before doing ioctl*/
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
		{
			lidbg("%s:DVR not found,ioctl fail!\n",__func__);
			dvrRespond[1] = RET_NOTVALID;
		}
		else if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX))
		{
			lidbg("%s:Is not SonixCam ,ioctl fail!\n",__func__);
			dvrRespond[1] = RET_NOTSONIX;
		}
		else if((pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRAfterFix)
		{
			lidbg("%s:Fix proc running!But ignore !\n",__func__);
			//return RET_NOTVALID;
			isDVRAfterFix = 1;//force to 1 tmp
		}

		/*check rear camera status before doing ioctl*/
		if(!((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID))
		{
			lidbg("%s:Rear not found,ioctl fail!\n",__func__);
			rearRespond[1] = RET_NOTVALID;
		}
		else if(!((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX))
		{
			lidbg("%s:Rear Is not SonixCam ,ioctl fail!\n",__func__);
			rearRespond[1] = RET_NOTSONIX;
		}
		else if(((pfly_UsbCamInfo->camStatus >> 4)  & FLY_CAM_ISSONIX) && !isRearViewAfterFix)
		{
			lidbg("%s:Rear Fix proc running!But ignore !\n",__func__);
			//return RET_NOTVALID;
			isRearViewAfterFix = 1;//force to 1 tmp
		}
		
#if 1
		switch(_IOC_NR(cmd))
		{
			case NR_EM_PATH:
				lidbg("%s:NR_EM_PATH  = [%s]\n",__func__,(char*)arg);
#if 0
				ret_st = checkSDCardStatus((char*)arg);
				if((ret_st != 1) || (ret_st != 2)) 
				{
					strcpy(em_path,(char*)arg);
					sprintf(temp_cmd, "setprop persist.uvccam.empath %s", em_path);
					lidbg_shell_cmd(temp_cmd);
				}
				else
					lidbg("%s: em_path access wrong! %d", __func__ ,EFAULT);//not happend
				if(ret_st > 0) dvrRespond[1] = RET_FAIL;
#endif
				sprintf(temp_cmd, "mkdir -p %s", (char*)arg);
				lidbg_shell_cmd(temp_cmd);
				sprintf(temp_cmd, "mkdir -p %s/.tmp", (char*)arg);
				lidbg_shell_cmd(temp_cmd);

				file_path = filp_open((char*)arg, O_RDONLY | O_DIRECTORY, 0);
				if(IS_ERR(file_path))
				{
					lidbg("%s:EM_PATH ERR!!\n",__func__);
					lidbg_shell_cmd("setprop persist.uvccam.empath "EMMC_MOUNT_POINT1"/camera_rec/BlackRec/");
					strcpy(dvrRespond + 2,EMMC_MOUNT_POINT1"/camera_rec/BlackRec/");
				}
				else
				{
					lidbg("%s:EMPATH OK!!\n",__func__);
					strcpy(em_path,(char*)arg);
					sprintf(temp_cmd, "setprop persist.uvccam.empath %s", em_path);
					lidbg_shell_cmd(temp_cmd);
					strcpy(dvrRespond + 2,em_path);
				}
				if(!IS_ERR(file_path)) filp_close(file_path, 0);
				
				length += 100;
				if(copy_to_user((char*)arg,dvrRespond,length))
				{
					lidbg("%s:copy_to_user ERR\n",__func__);
				}
		        break;
#if 0
			case NR_EM_START:
		        lidbg("%s:NR_EM_START\n",__func__);
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
					lidbg("%s:copy_to_user ERR\n",__func__);
				}
				mod_timer(&ui_start_rec_timer,UI_REC_WAIT_TIME);
		        break;
#endif
			case NR_EM_START:
				if(DVR_ID == ((char*)arg)[0])
				{
			        lidbg("%s:NR_EM_DVR_START\n",__func__);
					isEMDVRStartRec = ((char*)arg)[1];
					if((pfly_UsbCamInfo->camStatus) & FLY_CAM_ISVALID) dvrRespond[2] = 1;
						else dvrRespond[2] = 0;
					memcpy(returnRespond + length,dvrRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg("%s:copy_to_user ERR\n",__func__);
					}
					mod_timer(&em_start_dvr_rec_timer,UI_REC_WAIT_TIME);
				}
				else if(REARVIEW_ID == ((char*)arg)[0])
				{
			        lidbg("%s:NR_EM_REAR_START\n",__func__);
					isEMRearStartRec = ((char*)arg)[1];
					if((pfly_UsbCamInfo->camStatus >> 4) & FLY_CAM_ISVALID) rearRespond[2] = 1;
						else rearRespond[2] = 0;
					memcpy(returnRespond + length,rearRespond,3);
					length += 3;
					if(copy_to_user((char*)arg,returnRespond,length))
					{
						lidbg("%s:copy_to_user ERR\n",__func__);
					}
					mod_timer(&em_start_rear_rec_timer,UI_REC_WAIT_TIME);
				}
				break;
			
			case NR_EM_STATUS:
		        lidbg("%s:NR_EM_STATUS\n",__func__);
				//ret = isDVRRec;
				dvrRespond[2] = isDVRRec;
				rearRespond[2] = isRearRec;
				memcpy(returnRespond + length,dvrRespond,3);
				length += 3;
				memcpy(returnRespond + length,rearRespond,3);
				length += 3;
				if(copy_to_user((char*)arg,returnRespond,length))
				{
					lidbg("%s:copy_to_user ERR\n",__func__);
				}
		        break;
			
			case NR_EM_TIME:
				if(1 == ((char*)arg)[0])
				{
					lidbg("%s:NR_EM_TOP_TIME = [%d]\n",__func__,((char*)arg)[1]);
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
						lidbg("%s:copy_to_user ERR\n",__func__);
					}
				}
				else if(0 == ((char*)arg)[0])
				{
					lidbg("%s:NR_EM_BOTTOM_TIME = [%d]\n",__func__,((char*)arg)[1]);
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
						lidbg("%s:copy_to_user ERR\n",__func__);
					}
				}
		        break;
#if 0
			case NR_EM_TIME:
				lidbg("%s:NR_EM_TIME = [%d]\n",__func__,((char*)arg)[0]);
				em_time = ((char*)arg)[0];
				dvrRespond[2] = em_time;
				rearRespond[2] = em_time;
				memcpy(returnRespond + length,dvrRespond,3);
				length += 3;
				memcpy(returnRespond + length,rearRespond,3);
				length += 3;
				if(copy_to_user((char*)arg,returnRespond,length))
				{
					lidbg("%s:copy_to_user ERR\n",__func__);
				}
		        break;
#endif
			case NR_EM_MANUAL:
				lidbg("%s:NR_EM_MANUAL\n",__func__);
				if(isEmRecPermitted)
				{
					if(isDVRRec) lidbg_shell_cmd("setprop lidbg.uvccam.dvr.blackbox 1");
					if(isRearRec) lidbg_shell_cmd("setprop lidbg.uvccam.rear.blackbox 1");
				}
				notify_online(RET_EM_ISREC_ON);
		        break;
			default:
		        return -ENOTTY;
		}
		#endif
	}
	//else return -EINVAL;
    return ret;

dvrfailproc:
	lidbg("%s:======DVR failproc=======\n",__func__);
	stop_rec(DVR_ID,1);
	isDVRRec = 0;
	isOnlineRec = 0;
	return RET_FAIL;

rearfailproc:
	lidbg("%s:======Rear failproc=======\n",__func__);
	stop_rec(REARVIEW_ID,1);
	isRearRec = 0;
	return RET_FAIL;
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
		lidbg("copy_to_user ERR\n");
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
	lidbg("%s:====HAL read_status => %d=====\n",__func__,camStatus_data_for_hal[0]);
	if(copy_to_user(buffer, camStatus_data_for_hal, 1))
    {
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
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
		cmd_buf[size - 1] = '\0';
    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;
	lidbg("-----cmd_buf------------------[%s]---\n", cmd_buf);
	lidbg("-----cmd_num------------[%d]---\n", cmd_num);

	/*Do not check camera status in ACCOFF*/
	if(!isSuspend)
	{
		/*check camera status before doing ioctl*/
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))
		{
			lidbg("%s:DVR[online] not found,ioctl fail!\n",__func__);
			return size;
		}
		if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX) && !isDVRAfterFix)
		{
			lidbg("%s:is not SonixCam ,ioctl fail!\n",__func__);
			return size;
		}
	}
	
	for(i = 0;i < cmd_num; i++)
	{
		lidbg_token_string(cmd[i], "=", keyval) ;
		if(!strcmp(keyval[0], "record") )
		{
			if(!strncmp(keyval[1], "1", 1))//start
			{
			    lidbg("%s:==write==Online NR_START_REC\n",__func__);
				lidbg_shell_cmd("echo 'udisk_request' > /dev/flydev0");
				//setOnlineProp(DVR_ID);
				checkSDCardStatus(f_online_path);
				if(!(pfly_UsbCamInfo->camStatus & FLY_CAM_ISVALID))/*Camera not valid*/
				{
					if( isSuspend)
					{
						lidbg("%s:==write==ACCOFF wait Camera Begin!====\n",__func__);
						if(!wait_event_interruptible_timeout(pfly_UsbCamInfo->DVR_ready_wait_queue, (isDVRReady == 1), 10*HZ))
							goto failproc;
					}
					else 
					{
						lidbg("%s:==write==ACCON can not found Camera!Force exit!====\n",__func__);/*ACCON don't wait*/
						goto failproc;
					}
				}
				if(isDVRRec)
				{
					lidbg("%s:==write==Online start cmd neglected====\n",__func__);
					ret = RET_IGNORE;
				}
				else if(isOnlineRec) 
				{
					lidbg("%s:==write==Online start cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
				else
				{
					lidbg("%s:==write==Online start rec====\n",__func__);
					isOnlineRec = 1;
					if(isSuspend) fixScreenBlurred(DVR_ID,1);
					setOnlineProp(DVR_ID);
					if(start_rec(DVR_ID,1)) goto failproc;
				}
			}
			else if(!strncmp(keyval[1], "0", 1))//stop
			{
				lidbg("%s:==write==Online NR_STOP_REC\n",__func__);
				if(isDVRRec)
				{
					lidbg("%s:==write==Online stop cmd neglected====\n",__func__);
					ret = RET_IGNORE;
				}
				else if(isOnlineRec) 
				{
					lidbg("%s:==write==Online stop rec====\n",__func__);
					if(stop_rec(DVR_ID,1)) goto failproc;
					isOnlineRec = 0;
				}
				else
				{
					lidbg("%s:==write==Online stop cmd repeatedly====\n",__func__);
					ret = RET_REPEATREQ;
				}
			}
			else
			{
				lidbg("-------uvccam recording wrong arg:%s-----",keyval[1]);
			}
		}
		else if(!strcmp(keyval[0], "capture") )
		{
			if(!strncmp(keyval[1], "1", 1))//start
			{
				lidbg("-------uvccam capture-----");
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
		        lidbg("gain args error![0-100]");
		        return size;
		    }
			lidbg("gainVal = %d",gainVal);
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
		        lidbg("sharp args error![0-6]");
		        return size;
		    }
			lidbg("sharpVal = %d",sharpVal);
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
		        lidbg("gamma args error![0-500]");
		        return size;
		    }
			lidbg("gammaVal = %d",gammaVal);
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
		        lidbg("bright args error![0-128]");
		        return size;
		    }
			lidbg("brightVal = %d",brightVal);
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
		        lidbg("vmirror args error![0|1]");
		        return size;
		    }
			lidbg("vmirrorVal = %d",vmirrorVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set vmirror=%d ", vmirrorVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "autogain") )
		{
			int autogainVal;
			char temp_cmd[256];
			autogainVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("autogainVal = %d",autogainVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set autogain=%d ", autogainVal);
			//lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "exposure") )
		{
			int exposureVal;
			char temp_cmd[256];
			exposureVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("exposureVal = %d",exposureVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set exposure=%d ", exposureVal);
			//lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "contrast") )
		{
			int contrastVal;
			char temp_cmd[256];
			contrastVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("autogainVal = %d",contrastVal);
			sprintf(temp_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set contrast=%d ", contrastVal);
			lidbg_shell_cmd(temp_cmd);
		}

		else if(!strcmp(keyval[0], "saturation") )
		{
			int saturationVal;
			char temp_cmd[256];
			saturationVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("exposureVal = %d",saturationVal);
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
				lidbg("-------res wrong arg:%s-----",keyval[1]);
			}
#endif
			if(!strncmp(keyval[1], "640x360", 7))
			{
				strcpy(f_online_res,keyval[1]);
			}
			else
			{
				lidbg("-------res wrong arg:%s-----",keyval[1]);
			}
		}
		else if(!strcmp(keyval[0], "rectime") )
		{
			int rectimeVal;
			rectimeVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("rectimeVal = %d",rectimeVal);
			f_online_time = rectimeVal;
		}
		else if(!strcmp(keyval[0], "recbitrate") )
		{
			int recbitrateVal;
			recbitrateVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("recbitrateVal = %d",recbitrateVal);
			f_online_bitrate = recbitrateVal;
		}
		#if 1
		else if(!strcmp(keyval[0], "recnum") )
		{
			int recnumVal;
			recnumVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("recnumVal = %d",recnumVal);
			f_online_filenum = recnumVal;
		}
		#endif
		else if(!strcmp(keyval[0], "recpath") )
		{
			char ret_st;
			lidbg("recpathVal = %s",keyval[1]);
			ret_st = checkSDCardStatus(keyval[1]);
			if(ret_st != 1) 
				strcpy(f_online_path,keyval[1]);
			else
				lidbg("%s: f_online_path access wrong! %d", __func__ ,EFAULT);//not happend
			if(ret_st > 0) ret = RET_FAIL;
		}
		else if(!strcmp(keyval[0], "recfilesize") )
		{
			int recfilesizeVal;
			recfilesizeVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("recfilesizeVal = %d",recfilesizeVal);
			f_online_totalsize = recfilesizeVal;
		}
		else if(!strcmp(keyval[0], "formatcomplete") )
		{
			char temp_cmd[256];
			int formatVal;
			formatVal = simple_strtoul(keyval[1], 0, 0);
			lidbg("formatcomplete = %d\n",formatVal);
			if(formatVal == 1) status_fifo_in(RET_FORMAT_SUCCESS);
			else status_fifo_in(RET_FORMAT_FAIL);
			ssleep(7);
			/*
			if(!isDVRRec && !isOnlineRec&&  isBeforeFormatDVRRec)
				dvr_start_recording();
			if(!isRearRec &&  isBeforeFormatRearRec)
				rear_start_recording();
			*/
			if(isBeforeFormatDVRRec)
			{
				isUIStartRec = 1;
				mod_timer(&ui_start_rec_timer,UI_REC_WAIT_TIME);
			}
			isBeforeFormatDVRRec = 0;
			isBeforeFormatRearRec = 0;
			sprintf(temp_cmd, "setprop lidbg.uvccam.isFormat %d", 0);
			lidbg_shell_cmd(temp_cmd);
		}
#if 0
		else if(!strcmp(keyval[0], "test") )
		{
			isBackChange = 1;
			wake_up_interruptible(&wait_queue);
		}
#endif
	}
  
    return size;
failproc:
	lidbg("=====Flycam Write operation error!======\n");
	stop_rec(DVR_ID,1);
	isDVRRec = 0;
	isOnlineRec = 0;
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
    lidbg("-----------flycam_ops_suspend------------\n");
    DUMP_FUN;
	isKSuspend = 1;
    return 0;
}

static int flycam_ops_resume(struct device *dev)
{

    lidbg("-----------flycam_ops_resume------------\n");
    DUMP_FUN;
	//lidbg("g_var.acc_flag => %d",g_var.acc_flag);
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

int thread_flycam_init(void *data)
{
	char temp_cmd[256];
#ifndef	FLY_USB_CAMERA_SUPPORT
	lidbg("%s:FLY_USB_CAMERA_SUPPORT not define,exit\n",__func__);
  	return 0;
#endif
	lidbg("%s:------------start------------\n",__func__);

	/*init pfly_UsbCamInfo*/
	pfly_UsbCamInfo = (struct fly_UsbCamInfo *)kmalloc( sizeof(struct fly_UsbCamInfo), GFP_KERNEL);
    if (pfly_UsbCamInfo == NULL)
    {
        lidbg("[cam]:kmalloc err\n");
        return -ENOMEM;
    }
	
	camStatus_fifo_buffer = (u8 *)kmalloc(FIFO_SIZE , GFP_KERNEL);
    camStatus_data_for_hal = (u8 *)kmalloc(HAL_BUF_SIZE , GFP_KERNEL);
	sema_init(&pfly_UsbCamInfo->sem, 1);
    kfifo_init(&camStatus_data_fifo, camStatus_fifo_buffer, FIFO_SIZE);

	//init_waitqueue_head(&wait_queue);
	init_waitqueue_head(&pfly_UsbCamInfo->camStatus_wait_queue);/*camera status wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->DVR_ready_wait_queue);/*DVR wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->Rear_ready_wait_queue);/*Rear wait queue*/
	init_waitqueue_head(&pfly_UsbCamInfo->onlineNotify_wait_queue);/*onlineNotify wait queue*/

	init_completion(&DVR_res_get_wait);
	init_completion(&Rear_res_get_wait);
	init_completion(&timer_stop_rec_wait);
	init_completion(&Rear_fw_get_wait);
	init_completion(&DVR_fw_get_wait);
	init_completion(&accon_start_rec_wait);
	//init_completion(&auto_detect_wait);

	if(g_var.recovery_mode == 0)/*do not process when in recovery mode*/
	{
		CREATE_KTHREAD(thread_stop_rec_func, NULL);
		CREATE_KTHREAD(thread_accon_start_rec, NULL);
		CREATE_KTHREAD(thread_set_par_func, NULL);
		CREATE_KTHREAD(thread_ui_startrec_func, NULL);
		CREATE_KTHREAD(thread_start_dvr_rec_func, NULL);
		CREATE_KTHREAD(thread_start_rear_rec_func, NULL);
		register_lidbg_notifier(&lidbg_notifier);/*ACCON/OFF notifier*/

		lidbg_shell_cmd("setprop fly.uvccam.coldboot.isRec 1");

		if(fs_find_string(g_var.pflyhal_config_list, "DisableVideoLoop") > 0)
		{
			isDVRVideoLoop = 0;
			isRearVideoLoop = 0;
			isEmRecPermitted = 1;
			isThinkNavi = 1;
			sprintf(temp_cmd, "setprop persist.uvccam.isDVRVideoLoop %d", isDVRVideoLoop);
			lidbg_shell_cmd(temp_cmd);
			sprintf(temp_cmd, "setprop persist.uvccam.isRearVideoLoop %d", isRearVideoLoop);
			lidbg_shell_cmd(temp_cmd);
		}
		lidbg("%s:====isDVRVideoLoop:%d====\n",__func__,isDVRVideoLoop);

		if(fs_find_string(g_var.pflyhal_config_list, "YUV") > 0)
		{
			isPrevYUV = 1;
		}
		lidbg("%s:====isPrevYUV:%d====\n",__func__,isPrevYUV);
		sprintf(temp_cmd, "setprop lidbg.uvccam.isPrevYUV %d", isPrevYUV);
		lidbg_shell_cmd(temp_cmd);
		
		/*Stop recording timer(in ACCOFF scene)*/
		init_timer(&suspend_stoprec_timer);
	    suspend_stoprec_timer.data = 0;
	    suspend_stoprec_timer.expires = 0;
	    suspend_stoprec_timer.function = suspend_stoprec_timer_isr;
			
		init_timer(&set_par_timer);
	    set_par_timer.data = 0;
	    set_par_timer.expires = 0;
	    set_par_timer.function = set_par_timer_isr;

		init_timer(&ui_start_rec_timer);
	    ui_start_rec_timer.data = 0;
	    ui_start_rec_timer.expires = 0;
	    ui_start_rec_timer.function = ui_start_rec_timer_isr;

		init_timer(&em_start_dvr_rec_timer);
	    em_start_dvr_rec_timer.data = 0;
	    em_start_dvr_rec_timer.expires = 0;
	    em_start_dvr_rec_timer.function = start_dvr_rec_timer_isr;

		init_timer(&em_start_rear_rec_timer);
	    em_start_rear_rec_timer.data = 0;
	    em_start_rear_rec_timer.expires = 0;
	    em_start_rear_rec_timer.function = start_rear_rec_timer_isr;
	
		lidbg("%s:====start osd set====\n",__func__);
		lidbg_shell_cmd("setprop lidbg.uvccam.rear.osdset 0");
		lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 6&");
		lidbg_shell_cmd("setprop lidbg.uvccam.dvr.osdset 0");
		lidbg_shell_cmd("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 7&");
#if 0
		INIT_WORK(&work_t_start_rec, work_startRec);
	    INIT_WORK(&work_t_stop_rec, work_stopRec);
#endif 
		//INIT_WORK(&work_t_fixScreenBlurred, work_DVR_fixScreenBlurred);

		/*usb camera first plug in (fix lidbgshell delay issue)*/
		isDVRFirstInit = 1;
		isRearViewFirstInit = 1;
#if 0
		isDVRFirstInit = 1;
		pfly_UsbCamInfo->camStatus = lidbg_checkCam();
		lidbg("********camStatus => %d***********",pfly_UsbCamInfo->camStatus);
		if(pfly_UsbCamInfo->camStatus & FLY_CAM_ISSONIX)
			schedule_delayed_work(&work_t_DVR_fixScreenBlurred,10*HZ);/*at about kernel 30s, lidbgshell it's ready*/
		else isDVRFirstInit = 0;

		isRearViewFirstInit = 1;
		if((pfly_UsbCamInfo->camStatus>>4) & FLY_CAM_ISSONIX)
			schedule_delayed_work(&work_t_RearView_fixScreenBlurred,11*HZ);/*at about kernel 35s, lidbgshell it's ready*/
		else isRearViewFirstInit = 0;
#endif
		schedule_delayed_work(&work_t_DVR_fixScreenBlurred,15*HZ);/*at about kernel 30s, lidbgshell it's ready*/
		schedule_delayed_work(&work_t_RearView_fixScreenBlurred,16*HZ);/*at about kernel 31s, lidbgshell it's ready*/
		
	}
	usb_register_notify(&usb_nb_cam);/*USB notifier:must after isDVRFirstInit&isRearViewFirstInit*/
	lidbg_new_cdev(&flycam_nod_fops, "lidbg_flycam");
    return 0;
}

static __init int lidbg_flycam_init(void)
{
    DUMP_BUILD_TIME;
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


