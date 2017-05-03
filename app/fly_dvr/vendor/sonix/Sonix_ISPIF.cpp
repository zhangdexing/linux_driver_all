#include "Flydvr_ISPIF.h"
#include "Flydvr_Media.h"
#include "Flydvr_Message.h"
#include "Flydvr_General.h"
#include "Sonix_ISPIF.h"
#include "../../../inc/lidbg_flycam_app.h"

#include "fw/BurnerApLib//BurnerApLib.h"
#include "fw/common//usb.h"
#include "fw/common//my_type.h"
#include "fw/common//debug.h"
#include "fw/common//CamEnum.h"
#include "fw/BurnMgr//BurnMgr.h"

#define REC_TIME_SEC 	60
#define EMREC_REQUIRE_TIME_SEC 	60


ISP_IF_VERSION m_Front_IspVer;
ISP_IF_VERSION m_Rear_IspVer;

isp_hardware_t front_hw,rear_hw,front_online_hw;

pthread_t thread_LP_daemon_id;

bool isFrontWriteEnable = false;
bool isRearWriteEnable = false;
bool isFrontOnlineWriteEnable = false;

unsigned int singleFileVRTime = 300;

ISP_IF_VERSION Sonix_ISP_IF_LIB_LibVer_Init(INT32 cam_id)
{
	ISP_IF_VERSION m_isp_if;
	
	CCamEnum	cam_enum;
	struct usb_device* CamArray[MAX_CAM_NUM];
	INT32 		nCamNum;
	INT32 		camchoose;

	if((cam_id == DVR_ID && Sonix_ISP_IF_LIB_CheckFrontCamExist() == FLY_FALSE)
		|| (cam_id == REARVIEW_ID && Sonix_ISP_IF_LIB_CheckRearCamExist() == FLY_FALSE))
	{
		lidbg("None cam is found!\n");
		goto err;
	}

	if (!cam_enum.Enum_Cam(CamArray, nCamNum))
	{
		lidbg("enumerate webcam error!\n");
		goto err;
	}
	if (nCamNum == 0)
	{
		lidbg("NO webcam is found!\n");
		goto err;
	}
	//Print_CamArray(CamArray, nCamNum);
	//lidbg("\n");

	lidbg("Check Camera FW Version!\n");
	camchoose = Choose_CamArray(CamArray, nCamNum, cam_id);
	if(camchoose != -1) 
	{
		struct usb_device_descriptor *des;
		usb_dev_handle *udev;
		udev = usb_open(CamArray[camchoose]);
		if (!udev) lidbg("udev error!\n");
		des = &(CamArray[camchoose]->descriptor);
		m_isp_if.Vid = des->idVendor;
		m_isp_if.Pid = des->idProduct;
		usb_get_string_simple(udev, des->iManufacturer, m_isp_if.szMfg, 64*sizeof(char));
		usb_close(udev);
		get_FW_version(camchoose,CamArray, cam_id, m_isp_if.VerMark);
		m_isp_if.VerMark[10] = '\0';//just 10 char
		lidbg("vid = 0x%.4x, pid = 0x%.4x, Manufacturer = %s, VersionMark= %s\n", 
				m_isp_if.Vid, m_isp_if.Pid, m_isp_if.szMfg, m_isp_if.VerMark);
	}
	else 
	{
		lidbg("Get Camera FW Version Fail : Camera it's not exsit\n");
		goto err;
	}
	return m_isp_if;
err:
	strcpy(m_isp_if.szMfg, "NULL CAM");
	strcpy(m_isp_if.VerMark, "NULL CAM");
	return m_isp_if; 
}

void Sonix_ISP_IF_LIB_Init(void)
{
	int cnt = 10;
	/*In order to display correct Lib version, Wait for Front camera ready (10s)*/
	while(cnt-- > 0)
	{
		if(Sonix_ISP_IF_LIB_CheckFrontCamExist() == FLY_TRUE)
			break;
		sleep(1);
	}
	if(cnt == 0) lidbg("%s: Wait for Front camera timeout! \n", __func__ );	
	
	/*Get Camera Lib Version*/
	m_Front_IspVer = Sonix_ISP_IF_LIB_LibVer_Init(DVR_ID);
	sleep(2);//prevent error
	m_Rear_IspVer = Sonix_ISP_IF_LIB_LibVer_Init(REARVIEW_ID);
}

ISP_IF_VERSION Sonix_ISP_IF_LIB_GetFrontLibVer()
{
	return m_Front_IspVer;
}

ISP_IF_VERSION Sonix_ISP_IF_LIB_GetRearLibVer()
{
	return m_Rear_IspVer;
}

ISP_IF_VERSION Sonix_ISP_IF_LIB_GetSensorName()
{
	ISP_IF_VERSION m_isp_if;
	return m_isp_if;
}


//==========Sonix VR===========//

static bool sonix_check_cam(int cam_id, bool isH264, char* dev_name)
{
	char temp_devname[256], temp_devname2[256],hub_path[256];
	int rc = -1;
	DIR *pDir = NULL;
	struct dirent *ent = NULL;
	int fcnt = 0  ;

    lidbg("%s: E,=======[%d]\n", __func__, cam_id);

	memset(hub_path,0,sizeof(hub_path));  
	memset(temp_devname,0,sizeof(temp_devname));  

	//fix for attenuation hub.find the deepest one.
	unsigned int back_charcnt = 0,front_charcnt = 0;
	pDir=opendir("/sys/bus/usb/drivers/usb/");  
	if(pDir != NULL)
	{
		while((ent=readdir(pDir))!=NULL)  
		{  
				if((!strncmp(ent->d_name,  BACK_NODE , 5)) &&
					(strlen(ent->d_name) >= back_charcnt) && (cam_id == 0))
				{
					back_charcnt = strlen(ent->d_name);
					sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", ent->d_name,ent->d_name);//back cam
				}
				else if((!strncmp(ent->d_name,  FRONT_NODE , 5)) &&
					(strlen(ent->d_name) >= front_charcnt) && (cam_id == 1))
				{
					front_charcnt = strlen(ent->d_name);
					sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", ent->d_name,ent->d_name);//front cam
				} 
		}
		closedir(pDir);
		pDir = NULL;
	}

	if((front_charcnt == 0) && (back_charcnt == 0))
	{
		lidbg("%s: can not found suitable hubpath! \n", __func__ );	
		return false;
	}
	
	lidbg("%s:hubPath:%s\n",__func__ ,hub_path);  

	if(access(hub_path, R_OK) != 0)
	{
		adbg("%s: hub path access wrong!\n ", __func__ );
		return false;
	}
	
	pDir=opendir(hub_path);  
	if(pDir != NULL)
	{
		while((ent=readdir(pDir))!=NULL)  
		{  
				fcnt++;
		        if(ent->d_type & DT_DIR)  
		        {  
		                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (strncmp(ent->d_name, "video", 5)))  
		                        continue;  
						if(fcnt == 3)
							sprintf(temp_devname,"/dev/%s", ent->d_name);  
						if(fcnt == 4)//also save 2nd node name
							sprintf(temp_devname2,"/dev/%s", ent->d_name);  
		        }  
		}
		closedir(pDir);
		pDir = NULL;
	}
	else 
	{
		adbg("%s: openDir error!\n ", __func__ );
		return false;
	}

	lidbg("%s: This Camera [%d] has %d video node.\n", __func__ ,cam_id, fcnt - 2);
	
	if(fcnt == 3)	
	{
		adbg("%s: Camera [%d] does not support Sonix Recording!\n",__func__,cam_id);
		return false;
	}
	else if((fcnt == 0) && (ent == NULL))
	{
		adbg("%s: Hub node is not exist !\n ", __func__);
		return false;
	}
	else if(fcnt < 3)
	{
		adbg("%s: Camera [%d] nothing exist in hub dir!\n",__func__,cam_id);
		return false;
	}	

	lidbg("%s:First node Path:%s, Second node Path:%s\n",__func__ ,temp_devname,temp_devname2);      

	if(isH264 == true)
		strncpy(dev_name, temp_devname2, 256);
	else
		strncpy(dev_name, temp_devname, 256);
	
	return true;
}

FLY_BOOL Sonix_ISP_IF_LIB_CheckFrontCamExist()
{
	bool ret;
	INT8 dev_name[255];
	ret = sonix_check_cam(DVR_ID, true, dev_name);
	if(ret == true)
		return FLY_TRUE;
	return FLY_FALSE;
}

FLY_BOOL Sonix_ISP_IF_LIB_CheckRearCamExist()
{
	bool ret;
	INT8 dev_name[255];
	ret = sonix_check_cam(REARVIEW_ID, true, dev_name);
	if(ret == true)
		return FLY_TRUE;
	return FLY_FALSE;
}

FLY_BOOL Sonix_ISP_IF_LIB_GetFrontCamDevName(INT8* dev_name)
{
	bool ret;
	ret = sonix_check_cam(DVR_ID, true, dev_name);
	if(ret == true)
		return FLY_TRUE;
	return FLY_FALSE;
}

FLY_BOOL Sonix_ISP_IF_LIB_GetRearCamDevName(INT8* dev_name)
{
	bool ret;
	ret = sonix_check_cam(REARVIEW_ID, true, dev_name);
	if(ret == true)
		return FLY_TRUE;
	return FLY_FALSE;
}

static int sonix_video_enable(int dev, int enable)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret;

	ret = ioctl(dev, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		lidbg( "Unable to %s capture: %d.\n",
			enable ? "start" : "stop", errno);
		return ret;
	}

	return 0;
}

static int sonix_video_open(const char *devname)
{
	struct v4l2_capability cap;
	int dev, ret;

	dev = open(devname, O_RDWR);
	if (dev < 0) {
		adbg( "Error opening device %s: %d.\n", devname, errno);
		return dev;
	}

	memset(&cap, 0, sizeof cap);
	ret = ioctl(dev, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		adbg( "Error opening device %s: unable to query device.\n",
			devname);
		close(dev);
		return ret;
	}

#if 0
	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
		lidbg( "Error opening device %s: video capture not supported.\n",
			devname);
		close(dev);
		return -EINVAL;
	}
#endif

	printf( "Device %s opened: %s.\n", devname, cap.card);
	return dev;
}

static int GetFreeRam(unsigned int* freeram)
{
    FILE *meminfo = fopen("/proc/meminfo", "r");
	char line[256];
    if(meminfo == NULL)
	{
		lidbg( "/proc/meminfo can't open\n");
        return 0;
	}
    while(fgets(line, sizeof(line), meminfo))
    {
        if(sscanf(line, "MemFree: %d kB", freeram) == 1)
        {
			*freeram <<= 10;
            fclose(meminfo);
            return 1;
        }
    }
	
    fclose(meminfo);
    return 0;
}

static int GetCurrentTimeString(char *time_string)
{
	time_t timep; 
	struct tm *p; 
	char rtc_cmd[255] = {0};
	time(&timep); 
	p=localtime(&timep); 
    if(time_string)
        sprintf(time_string, "%d-%02d-%02d__%02d.%02d.%02d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday,p->tm_hour , p->tm_min,p->tm_sec);
    return 0;
}

static int sonix_video_reqbufs(int dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	int ret;

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		adbg( "Unable to allocate buffers: %d.\n", errno);
		return ret;
	}

	lidbg("%u buffers allocated.\n", rb.count);
	return rb.count;
}


static int sonix_video_set_format(int dev, unsigned int w, unsigned int h, unsigned int format)
{
	struct v4l2_format fmt;
	int ret;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	ret = ioctl(dev, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		lidbg( "Unable to set format: %d.\n", errno);
		return ret;
	}

	lidbg( "Video format set: width: %u height: %u buffer size: %u\n",
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
	return 0;
}


static int sonix_video_set_framerate(int dev, int framerate, unsigned int *MaxPayloadTransferSize)
{
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		adbg(  "Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	lidbg( "Current frame rate: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);

	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = framerate;

	ret = ioctl(dev, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		adbg(  "Unable to set frame rate: %d.\n", errno);
		return ret;
	}

    //yiling: get MaxPayloadTransferSize from sonix driver
    if(MaxPayloadTransferSize)
        *MaxPayloadTransferSize = parm.parm.capture.reserved[0];

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		adbg(  "Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	lidbg( "Frame rate set: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);
	return 0;
}

static int sonix_unInitV4L2mmap(isp_hardware_t* hw)
    {
        unsigned int i;
        int rc = 0;
        lidbg("%s: E\n", __func__);

        for (i = 0; i < hw->nbufs; i++)
            if (-1 == munmap(hw->mem0[i], hw->buf0.length))
            {
                lidbg("%s: munmap failed for buffer: %d\n", __func__, i);
                rc = -1;
            }

        lidbg("%s: X\n", __func__);
        return rc;
    }

static int sonixStopVRInternal(isp_hardware_t* hw)
{
    int rc = 0;
    lidbg("%s: E\n", __func__);
	struct timespec timeLimit_sp;
	timeLimit_sp.tv_sec = 3;
	timeLimit_sp.tv_nsec = 0;

    if(hw->VREnabledFlag)
    {
        hw->VRCmdPending++;
        hw->VRCmd         = VR_CMD_EXIT;

        /* yield lock while waiting for the preview thread to exit */
        //hw->lock.unlock();
        if(pthread_join(hw->VRThread, NULL))
        {
            lidbg("%s: Error in pthread_join VR thread\n", __func__);
        }
        //hw->lock.lock();

        if(sonix_video_enable(hw->dev, 0))
        {
            lidbg("%s: Error in sonix_video_enable [0]\n", __func__);
            rc = -1;
        }
				
        if(sonix_unInitV4L2mmap(hw))
        {
            lidbg("%s: Error in stopUsbCamCapture\n", __func__);
            rc = -1;
        }
				
        hw->VREnabledFlag = 0;
    }

    lidbg("%s: X, rc: %d\n", __func__, rc);
    return rc;
}

void sonix_stop_VR(isp_hardware_t* hw)
{
        lidbg("%s: E\n", __func__);

        int rc = 0;

        //Mutex::Autolock autoLock(hw->lock);

        rc = sonixStopVRInternal(hw);
        if(rc)
            lidbg("%s: stopPreviewInternal returned error\n", __func__);

		close(hw->dev);//add

        lidbg("%s: X\n", __func__);
        return;
}

void sonix_pause_VR(isp_hardware_t* hw)
{
        lidbg("%s: E\n", __func__);
		hw->VRCmdPending++;
        hw->VRCmd         = VR_CMD_PAUSE;
        lidbg("%s: X\n", __func__);
        return;
}

void sonix_resume_VR(isp_hardware_t* hw)
{
        lidbg("%s: E\n", __func__);
		hw->VRCmdPending++;
        hw->VRCmd         = VR_CMD_RESUME;
        lidbg("%s: X\n", __func__);
        return;
}

static int sonixInitFrontDefaultParameters()
{
        lidbg("%s: E\n", __func__);
        int rc = 0;

        front_hw.VRFormat          = FRONT_VR_FMT;
        front_hw.VRWidth           = FRONT_VR_WIDTH;
        front_hw.VRHeight          = FRONT_VR_HEIGHT;
        front_hw.VREnabledFlag  = 0;
        front_hw.VRCmdPending      = 0;
		front_hw.VRFps = FRONT_VR_FRAMERATE;
		front_hw.nbufs = V4L_BUFFERS_DEFAULT;
		front_hw.VRBitRateMode = FRONT_VR_BIT_RATE_MODE;
		front_hw.VRBitRate = FRONT_VR_BITRATE;
		
        lidbg("%s: X\n", __func__);
        return rc;
 }

static int sonixInitRearDefaultParameters()
{
        lidbg("%s: E\n", __func__);
        int rc = 0;

        rear_hw.VRFormat          = REAR_VR_FMT;
        rear_hw.VRWidth           = REAR_VR_WIDTH;
        rear_hw.VRHeight          = REAR_VR_HEIGHT;
        rear_hw.VREnabledFlag  = 0;
        rear_hw.VRCmdPending      = 0;
		rear_hw.VRFps = REAR_VR_FRAMERATE;
		rear_hw.nbufs = V4L_BUFFERS_DEFAULT;
		rear_hw.VRBitRateMode = REAR_VR_BIT_RATE_MODE;
		rear_hw.VRBitRate = REAR_VR_BITRATE;
		
        lidbg("%s: X\n", __func__);
        return rc;
 }

static int sonixInitFrontOnlineDefaultParameters()
{
        lidbg("%s: E\n", __func__);
        int rc = 0;

        front_online_hw.VRFormat          = FRONT_VR_FMT;
        front_online_hw.VRWidth           = FRONT_ONLINE_VR_WIDTH;
        front_online_hw.VRHeight          = FRONT_ONLINE_VR_HEIGHT;
        front_online_hw.VREnabledFlag  = 0;
        front_online_hw.VRCmdPending      = 0;
		front_online_hw.VRFps = FRONT_VR_FRAMERATE;
		front_online_hw.nbufs = V4L_BUFFERS_DEFAULT;
		front_online_hw.VRBitRateMode = FRONT_ONLINE_VR_BIT_RATE_MODE;
		front_online_hw.VRBitRate = FRONT_VR_BITRATE;
		
        lidbg("%s: X\n", __func__);
        return rc;
 }


/********************/

#define member_of(ptr, type, member) ({ \
  const typeof(((type *)0)->member) *__mptr = (ptr); \
  (type *)((char *)__mptr - offsetof(type,member));})

struct cam_list {
  struct cam_list *next, *prev;
};

static inline void cam_list_init(struct cam_list *ptr)
{
  ptr->next = ptr;
  ptr->prev = ptr;
}

static inline void cam_list_add_tail_node(struct cam_list *item,
  struct cam_list *head)
{
  struct cam_list *prev = head->prev;

  head->prev = item;
  item->next = head;
  item->prev = prev;
  prev->next = item;
}

static inline void cam_list_insert_before_node(struct cam_list *item,
  struct cam_list *node)
{
  item->next = node;
  item->prev = node->prev;
  item->prev->next = item;
  node->prev = item;
}

static inline void cam_list_del_node(struct cam_list *ptr)
{
  struct cam_list *prev = ptr->prev;
  struct cam_list *next = ptr->next;

  next->prev = ptr->prev;
  prev->next = ptr->next;
  ptr->next = ptr;
  ptr->prev = ptr;
}

typedef struct {
    struct cam_list list;
    void* data;
	int length;
	unsigned int msize;
} camera_q_node;

camera_q_node front_mhead,rear_mhead; /* dummy head */
pthread_mutex_t alock;

bool enqueue(void *data,int count, camera_q_node* mhead)
{
	void *tmpData = NULL;
	tmpData = malloc(count);  
	memcpy(tmpData, data, count);
    camera_q_node *node =
        (camera_q_node *)malloc(sizeof(camera_q_node));
    if (NULL == node) {
        lidbg("%s: No memory for camera_q_node\n", __func__);
        return false;
    }

    memset(node, 0, sizeof(camera_q_node));
    node->data = tmpData;
	node->length = count;
    pthread_mutex_lock(&alock);
    cam_list_add_tail_node(&node->list, &mhead->list);
    mhead->msize++;
	//free(tmpData);
    pthread_mutex_unlock(&alock);
    return true;
}

int query_length(camera_q_node* mhead)
{
	int length = 0;
	camera_q_node* node = NULL;
    //void* data = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;

    pthread_mutex_lock(&alock);
    head = &mhead->list;
    pos = head->next;
    if (pos != head) {
        node = member_of(pos, camera_q_node, list);
    }
    pthread_mutex_unlock(&alock);
	
	if (NULL != node)  return node->length;
	else return 0;
}

int dequeue(void* data, camera_q_node* mhead)
{
	int ret = 0;
    camera_q_node* node = NULL;
    //void* data = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;

    pthread_mutex_lock(&alock);
    head = &mhead->list;
    pos = head->next;
    if (pos != head) {
        node = member_of(pos, camera_q_node, list);
        cam_list_del_node(&node->list);
        mhead->msize--;
    }
    pthread_mutex_unlock(&alock);

    if (NULL != node) {
        //data = node->data;
        memcpy(data, node->data, node->length);
		ret =  node->length;
        if (NULL != node->data) free(node->data);
		free(node);
    }

    return ret;
}

int queue_flush(camera_q_node* mhead)
{
	int ret = 0;
    camera_q_node* node = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;

    pthread_mutex_lock(&alock);
    head = &mhead->list;
    pos = head->next;
    if (pos != head) {
        node = member_of(pos, camera_q_node, list);
        cam_list_del_node(&node->list);
        mhead->msize--;
    }
    pthread_mutex_unlock(&alock);

    if (NULL != node) {
        //data = node->data;
		ret =  node->length;
        if (NULL != node->data) free(node->data);
		free(node);
    }

    return ret;
}

void dequeue_to_fp(int count , FILE * rec_fp, bool* isPermitted, camera_q_node* mhead)
{
	FILE *fp1 = NULL;
	FILE *fp2 = NULL;
	unsigned int totalLength = 0,diffMs = 0,kbPerSec = 0;
	struct timespec start_sp, stop_sp ;
	bool needAttachIFrame = false;
	bool isFirstIFrame = false;
	unsigned long filesize = -1;

	if(mhead->msize < count)
	{
		vdbg("%s:===msize:%d, count:%d, not enough for dequeue!!==\n",__func__,mhead->msize,count);
		return;
	}
	
	vdbg("%s:E===totally [%d] , dequeue count :[%d]==\n",__func__,mhead->msize,count);

	clock_gettime(CLOCK_MONOTONIC, &start_sp);

	/*Check whether it is the first frame of file*/
	if(rec_fp > 0)
	{
		fseek(rec_fp, 0L, SEEK_END);  
	    filesize = ftell(rec_fp);  
		vdbg("%s:==filesize: %d==\n",__func__,filesize);
		if(filesize == 0)
			needAttachIFrame = true;
	}
	else
	{
		lidbg("%s:==File des wrong!==\n",__func__);
		return;
	}

	while((count --) > 0)
	{
		void* tempa;
		unsigned int lengtha;
		lengtha = query_length(mhead);
		if(lengtha == 0) 
		{
			//lidbg("%s:===length error!==\n",__func__);
			queue_flush(mhead);
			continue;
		}
		tempa = malloc(lengtha);
		lengtha = dequeue(tempa, mhead);

		if(needAttachIFrame == true)
		{
			if(isFirstIFrame == false)
			{
#if 1		
				unsigned char tmp_val = 0;
				tmp_val = *(unsigned char*)(tempa + 18);
				if(tmp_val == 0x68) 
				{

					/*1280x720*/
					tmp_val = *(unsigned char*)(tempa + 26);
					if(tmp_val == 0x65) 
					{
						isFirstIFrame = true;
					}
				}

				tmp_val = *(unsigned char*)(tempa + 19);
				if(tmp_val == 0x68) 
				{
					/*640x360*/
					tmp_val = *(unsigned char*)(tempa + 27);
					if(tmp_val == 0x65)
					{
						isFirstIFrame = true;
					}
				}
#else
					isFirstIFrame = true;
#endif
			}
		}
		else isFirstIFrame = true;

		if(*isPermitted == false);
			//lidbg("%s:force return!!!count => %d\n",__func__,count);
		
		if((isFirstIFrame == true) && rec_fp > 0)
		{
			fwrite(tempa, lengtha, 1, rec_fp);//write data to the output files
			totalLength += (lengtha/1000);
		}

		if(tempa != NULL) free(tempa);
	}
	clock_gettime(CLOCK_MONOTONIC, &stop_sp);
	diffMs = (stop_sp.tv_sec * 1000 + stop_sp.tv_nsec / 1000000) - (start_sp.tv_sec * 1000 + start_sp.tv_nsec / 1000000);
	kbPerSec = totalLength/diffMs;
	
	vdbg("(Write Speed: %dMBytes/Sec)\n",kbPerSec);
	if(kbPerSec < 5)
		lidbg("%s:Write speed warning![%dMB/s] \n",__func__,kbPerSec);
	
	vdbg("%s:X====\n",__func__,count);
	return;
}

void dequeue_flush(int count , camera_q_node* mhead)
{
	if(mhead->msize < count)
	{
		vdbg("%s:===msize:%d, count:%d, not enough for flush!!==\n",__func__,mhead->msize,count);
		return;
	}
	
	vdbg("%s:E===count => %d==\n",__func__,count);

	while((count --) > 0)
	{
		queue_flush(mhead);
	}
	vdbg("%s:X===count => %d==\n",__func__,count);
	return;
}


/********************/

 static void* sonixFrontVRloop(void *par)
    {
        int                 rc, threadPriority;
        int                 buffer_id   = 0;
        pid_t               tid         = 0;
        int                 msgType     = 0;
		int ret;
		unsigned int i;
		unsigned int delay = 0, nframes = (unsigned int)-1;
		struct timeval start, end, ts;
		//bool b_writeVR;
		char timeString[100] = {0};
		char VR_File_Name[255] = {0};
		char Old_VR_File_Name[255] = {0};
		bool isCreateNewVRFile = false;
		unsigned int originRecSec = 0;
		unsigned int previousdiff_Sec = 1;
		//bool iswritePermitted = false;
		unsigned int SingleFile_VRTime = singleFileVRTime;
		front_hw.iswritePermitted = isFrontWriteEnable;//PreSave

		lidbg("%s: isFrontWriteEnable: %d", __func__, isFrontWriteEnable);

		if(front_hw.iswritePermitted == true)
			Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_RECORD_START);
		else
			Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_RECORD_STOP);

		lidbg("%s: E\n", __func__);

		//if(Flydvr_SDMMC_GetMountState() == SDMMC_IN)
		//	front_hw.iswritePermitted = true;

        static int loop_count = 0;
        struct timespec start_sp ;
        start_sp.tv_sec = start_sp.tv_nsec = 0;
        clock_gettime(CLOCK_MONOTONIC, &start_sp);

        front_hw.VRCmdPending = 0;

        //tid = androidGetTid();
        tid  = gettid();
        androidSetThreadPriority(tid, ANDROID_PRIORITY_DISPLAY);
        prctl(PR_SET_NAME, (unsigned long)"flyaudio VR thread", 0, 0, 0);
        threadPriority = androidGetThreadPriority(tid);

        while(1)
        {
        	int FPS = 0;
			for (i = 0; i < nframes; ++i) {

				/*Counting current framerate*/
				fd_set fds;
	            struct timeval tv;
	            int r = 0;

	            loop_count++;
	            if(loop_count >= 100)//
	            {
	                int diff;
	                struct timespec stop_sp ;
	                stop_sp.tv_sec = stop_sp.tv_nsec = 0;
	                clock_gettime(CLOCK_MONOTONIC, &stop_sp);
	                diff = (stop_sp.tv_sec * 1000 + stop_sp.tv_nsec / 1000000) - (start_sp.tv_sec * 1000 + start_sp.tv_nsec / 1000000);
	                FPS = 1000 * loop_count / diff;
					if(FPS < 25) // warning
	                	lidbg("%s:PRI.%d, FPS.%d,WritePermit.%d,totalFrames.%d,EMHandle.%d\n",
										__func__, threadPriority, FPS, front_hw.iswritePermitted,front_mhead.msize,front_hw.isEMHandling);
	                loop_count = 0;
	                clock_gettime(CLOCK_MONOTONIC, &start_sp);
	                //usleep(670 * 1000);
	                //continue;
	            }

	            FD_ZERO(&fds);
	            FD_SET(front_hw.dev, &fds);

	            tv.tv_sec = 0;
	            tv.tv_usec = 500000;

	            r = select(front_hw.dev + 1, &fds, NULL, NULL, &tv);
	            if (-1 == r)
	            {
	            	lidbg("%s: FDSelect error: %d", __func__, errno);
	                if (EINTR == errno)
	                    continue;
	            }

				//Mutex::Autolock autoLock(front_hw.lock);
	            if(front_hw.VRCmdPending)
	            {
	                front_hw.VRCmdPending--;
	                if(VR_CMD_EXIT== front_hw.VRCmd)
	                {
	                    //front_hw.lock.unlock();
						lidbg("%s: Exiting coz VR_CMD_EXIT\n", __func__);
						if(front_hw.rec_fp > 0) 
							fclose(front_hw.rec_fp);
						front_hw.rec_fp = 0;
						//dequeue_flush(front_mhead.msize, &front_mhead);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_FLUSH, front_mhead.msize);
						Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_EMERGENCY_UNLOCK);
	                    return (void *)0;
	                }
					else if(VR_CMD_PAUSE == front_hw.VRCmd)
					{
						if(front_hw.iswritePermitted == true)
						{
							lidbg("%s: VR_CMD_PAUSE\n", __func__);
							if(front_hw.rec_fp > 0) 
								fclose(front_hw.rec_fp);
							front_hw.iswritePermitted = false;
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_RECORD_STOP);
						}
					}
					else if(VR_CMD_RESUME == front_hw.VRCmd)
					{
						if(front_hw.iswritePermitted == false)
						{
							lidbg("%s: VR_CMD_RESUME\n", __func__);
							if(front_hw.isEMHandling == true) //Not write EM cancel
							{
								lidbg("%s: Not write EM : resume\n", __func__);
								front_hw.isEMHandling = false;
							}
							else //write EM new round
							{
								i = 0;
								originRecSec = front_hw.buf0.timestamp.tv_sec;
							}
							front_hw.iswritePermitted = true;
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_RECORD_START);
						}
					}
					else if(VR_CMD_GSENSOR_CRASH == front_hw.VRCmd)
					{
						if(front_hw.isEMHandling == false)
						{
							lidbg("%s: VR_CMD_GSENSOR_CRASH\n", __func__);
							if(front_hw.iswritePermitted == false) //Not write EM new round
								originRecSec = front_hw.buf0.timestamp.tv_sec;
							front_hw.isEMHandling = true;
						}
					}
					else if(VR_CMD_CHANGE_SINGLE_FILE_TIME == front_hw.VRCmd)
					{
						lidbg("%s: VR_CMD_CHANGE_SINGLE_FILE_TIME\n", __func__);
						if(front_hw.iswritePermitted == true) //write new round
						{
							i = 0;
							originRecSec = front_hw.buf0.timestamp.tv_sec;
						}
						SingleFile_VRTime = singleFileVRTime;
					}
	            }

				if(FPS > 500) 
				{
					usleep(100*1000);
					continue;//prevent bug
				}

				if(Flydvr_SDMMC_GetMountState() == SDMMC_OUT)
				{
					if(front_hw.rec_fp > 0) 
						fclose(front_hw.rec_fp);
					front_hw.iswritePermitted = false;
				}
				else
				{
					front_hw.iswritePermitted = isFrontWriteEnable;
				}

				/* Dequeue a buffer. */
				memset(&(front_hw.buf0), 0, sizeof (front_hw.buf0));
				front_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				front_hw.buf0.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(front_hw.dev, VIDIOC_DQBUF, &(front_hw.buf0));
				if (ret < 0) {
					adbg( "%s: Unable to dequeue buffer0 (%d).\n",__func__, errno);
					//close(front_hw.dev);//notify
					return (void *)0;
				}

				gettimeofday(&ts, NULL);

				//lidbg("Frame[%4u] %u bytes %ld.%06ld %ld.%06ld\n ", i, front_hw.buf0.bytesused, front_hw.buf0.timestamp.tv_sec, front_hw.buf0.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);


				if (i == 0)
					start = ts;
				
				//if (i == 0) originRecSec = front_hw.buf0.timestamp.tv_sec;

				if(i % 150 == 0) 
					Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_OSD_SYNC, 0);

				/*60s*/
				if(front_hw.iswritePermitted == true) /*Writting permmited*/
				{
					if(front_hw.isEMHandling == true)
					{
						if((front_hw.buf0.timestamp.tv_sec - originRecSec) % SingleFile_VRTime > (SingleFile_VRTime - EMREC_REQUIRE_TIME_SEC)	)
						{
							int diffsec = (front_hw.buf0.timestamp.tv_sec - originRecSec) % SingleFile_VRTime - (SingleFile_VRTime - EMREC_REQUIRE_TIME_SEC);
							lidbg("======== diffsec:%d=======\n",diffsec);
							originRecSec += diffsec;
						}
						else lidbg("======== keep recording!=======\n");
						front_hw.isEMHandling = false;
					}

				
				    if((((front_hw.buf0.timestamp.tv_sec - originRecSec) % SingleFile_VRTime == 0) 
							&& (originRecSec != front_hw.buf0.timestamp.tv_sec))|| (i == 0))
				    {
						lidbg("======== CreateNewVRFile!=======\n");
						originRecSec = front_hw.buf0.timestamp.tv_sec;

						if((front_hw.rec_fp > 0) && (i != 0)) //seamless
						{
#if 0							
							fclose(front_hw.rec_fp);						
							strcpy(Old_VR_File_Name, VR_File_Name);
							front_hw.old_rec_fp = fopen(Old_VR_File_Name, "ab+");
#else
							front_hw.old_rec_fp = front_hw.rec_fp;
							front_hw.rec_fp = NULL;
							Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_DEQUEUE_OLDFP, front_mhead.msize);
#endif							
						}
						else
							dequeue_flush(front_mhead.msize, &front_mhead);
						
						GetCurrentTimeString(timeString);
						sprintf(front_hw.currentVRFileName, "F%s.h264", timeString);
						sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, front_hw.currentVRFileName);
						lidbg("=========new VR_File_Name : %s===========\n", VR_File_Name);	
						Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_EMERGENCY_UNLOCK);
						Flydvr_SetFirstDelProtectFile(front_hw.currentVRFileName);
						front_hw.rec_fp = fopen(VR_File_Name, "wb");
					}
				}
				else /*Writting not permmited*/
				{
					if(front_hw.isEMHandling == true)
					{
						if((((front_hw.buf0.timestamp.tv_sec - originRecSec) % EMREC_REQUIRE_TIME_SEC == 0) 
							&& (originRecSec != front_hw.buf0.timestamp.tv_sec)))
						{
							lidbg("======== EM End!(not write)=======\n");
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_EMERGENCY_UNLOCK);
							front_hw.isEMHandling = false;
						}
					}
				}

				if(front_mhead.msize < 900)
				{

				/* Record the H264 video file */
				//if(front_hw.rec_fp > 0)
				//{
#if 0
					fwrite(front_hw.mem0[front_hw.buf0.index], front_hw.buf0.bytesused, 1, front_hw.rec_fp);
#endif
					if(front_hw.buf0.bytesused == 0)
					{
						//lidbg("%s: ======== data's length = 0!skip!=======\n",__func__);
					}
					else
						enqueue(front_hw.mem0[front_hw.buf0.index], front_hw.buf0.bytesused,&front_mhead);
				//}

				}
			

				/* Requeue the buffer. */
				//if (delay > 0)
				//	usleep(delay * 1000);
				if(front_hw.iswritePermitted == true)
				{
					if((front_mhead.msize >= 600) && (i % 100 == 0)) //dequeue_to_fp(300, front_hw.rec_fp);
					{
						vdbg("Dequeue!.%d\n",front_mhead.msize);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_DEQUEUE, 300);
					}
				}
				else
				{
					if(front_hw.isEMHandling == true)
					{
						if((front_mhead.msize >= 600) && (i % 100 == 0)) //dequeue_to_fp(300, front_hw.rec_fp);
						{
							vdbg("Dequeue!.%d\n",front_mhead.msize);
							Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_DEQUEUE, 300);
						}
					}
					else
					{
						if((front_mhead.msize >= 600) && (i % 100 == 0))  //dequeue_to_fp(300, front_hw.rec_fp);
						{
							vdbg("Flush!.%d\n",front_mhead.msize);
							Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_FLUSH, 300);
						}
					}
				}
				
				ret = ioctl(front_hw.dev, VIDIOC_QBUF, &(front_hw.buf0));
				if (ret < 0) {
					adbg( "%s: Unable to requeue buffer0 (%d).\n",__func__, errno);
					//close(front_hw.dev);//notify
					return (void *)0;
				}

			}

			//fflush(stdout);
		}
		lidbg("%s: X\n", __func__);
        return (void *)0;
   }

 static void* sonixRearVRloop(void *par)
    {
        int                 rc, threadPriority;
        int                 buffer_id   = 0;
        pid_t               tid         = 0;
        int                 msgType     = 0;
		int ret;
		unsigned int i;
		unsigned int delay = 0, nframes = (unsigned int)-1;
		struct timeval start, end, ts;
		//bool b_writeVR;
		char timeString[100] = {0};
		char VR_File_Name[255] = {0};
		char Old_VR_File_Name[255] = {0};
		bool isCreateNewVRFile = false;
		unsigned int originRecSec = 0;
		unsigned int previousdiff_Sec = 1;
		//bool iswritePermitted = false;
		unsigned int SingleFile_VRTime = singleFileVRTime;
		rear_hw.iswritePermitted = isRearWriteEnable;//PreSave

		lidbg("%s: isRearWriteEnable: %d", __func__, isRearWriteEnable);

		lidbg("%s: E\n", __func__);

		//if(Flydvr_SDMMC_GetMountState() == SDMMC_IN)
		//	rear_hw.iswritePermitted = true;

        static int loop_count = 0;
        struct timespec start_sp ;
        start_sp.tv_sec = start_sp.tv_nsec = 0;
        clock_gettime(CLOCK_MONOTONIC, &start_sp);

        rear_hw.VRCmdPending = 0;

        //tid = androidGetTid();
        tid  = gettid();
        androidSetThreadPriority(tid, ANDROID_PRIORITY_DISPLAY);
        prctl(PR_SET_NAME, (unsigned long)"flyaudio VR thread", 0, 0, 0);
        threadPriority = androidGetThreadPriority(tid);

        while(1)
        {
        	int FPS = 0;
			for (i = 0; i < nframes; ++i) {

				/*Counting current framerate*/
				fd_set fds;
	            struct timeval tv;
	            int r = 0;

	            loop_count++;
	            if(loop_count >= 100)//
	            {
	                int diff;
	                struct timespec stop_sp ;
	                stop_sp.tv_sec = stop_sp.tv_nsec = 0;
	                clock_gettime(CLOCK_MONOTONIC, &stop_sp);
	                diff = (stop_sp.tv_sec * 1000 + stop_sp.tv_nsec / 1000000) - (start_sp.tv_sec * 1000 + start_sp.tv_nsec / 1000000);
	                FPS = 1000 * loop_count / diff;
					if(FPS < 25)// warning
	               		lidbg("%s:PRI.%d, FPS.%d,WritePermit.%d,totalFrames.%d,EMHandle.%d\n",
										__func__, threadPriority, FPS, rear_hw.iswritePermitted,rear_mhead.msize,rear_hw.isEMHandling);
	                loop_count = 0;
	                clock_gettime(CLOCK_MONOTONIC, &start_sp);
	                //usleep(670 * 1000);
	                //continue;
	            }

	            FD_ZERO(&fds);
	            FD_SET(rear_hw.dev, &fds);

	            tv.tv_sec = 0;
	            tv.tv_usec = 500000;

	            r = select(rear_hw.dev + 1, &fds, NULL, NULL, &tv);
	            if (-1 == r)
	            {
	            	lidbg("%s: FDSelect error: %d", __func__, errno);
	                if (EINTR == errno)
	                    continue;
	            }

				//Mutex::Autolock autoLock(front_hw.lock);
	            if(rear_hw.VRCmdPending)
	            {
	                rear_hw.VRCmdPending--;
	                if(VR_CMD_EXIT== rear_hw.VRCmd)
	                {
	                    //front_hw.lock.unlock();
						lidbg("%s: Exiting coz VR_CMD_EXIT\n", __func__);
						if(rear_hw.rec_fp > 0) 
							fclose(rear_hw.rec_fp);
						rear_hw.rec_fp = 0;
						//dequeue_flush(rear_mhead.msize, &rear_mhead);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_FLUSH, rear_mhead.msize);
	                    return (void *)0;
	                }
					else if(VR_CMD_PAUSE == rear_hw.VRCmd)
					{
						if(rear_hw.iswritePermitted == true)
						{
							lidbg("%s: VR_CMD_PAUSE\n", __func__);
							if(rear_hw.rec_fp > 0) 
								fclose(rear_hw.rec_fp);
							rear_hw.iswritePermitted = false;
						}
					}
					else if(VR_CMD_RESUME == rear_hw.VRCmd)
					{
						if(rear_hw.iswritePermitted == false)
						{
							lidbg("%s: VR_CMD_RESUME\n", __func__);
							if(rear_hw.isEMHandling == true) //Not write EM cancel
							{
								lidbg("%s: Not write EM : resume\n", __func__);
								rear_hw.isEMHandling = false;
							}
							else //write EM new round
							{
								i = 0;
								originRecSec = rear_hw.buf0.timestamp.tv_sec;
							}
							rear_hw.iswritePermitted = true;
						}
					}
					else if(VR_CMD_GSENSOR_CRASH == rear_hw.VRCmd)
					{
						if(rear_hw.isEMHandling == false)
						{
							lidbg("%s: VR_CMD_GSENSOR_CRASH\n", __func__);
							if(rear_hw.iswritePermitted == false) //Not write EM new round
								originRecSec = rear_hw.buf0.timestamp.tv_sec;
							rear_hw.isEMHandling = true;
						}
					}
					else if(VR_CMD_CHANGE_SINGLE_FILE_TIME == rear_hw.VRCmd)
					{
						lidbg("%s: VR_CMD_CHANGE_SINGLE_FILE_TIME\n", __func__);
						if(rear_hw.iswritePermitted == true) //write EM new round
						{
							i = 0;
							originRecSec = rear_hw.buf0.timestamp.tv_sec;
						}
						SingleFile_VRTime = singleFileVRTime;
					}
	            }

				if(FPS > 500) 
				{
					usleep(100*1000);
					continue;//prevent bug
				}

				if(Flydvr_SDMMC_GetMountState() == SDMMC_OUT)
				{
					if(rear_hw.rec_fp > 0) 
						fclose(rear_hw.rec_fp);
					rear_hw.iswritePermitted = false;
				}
				else
				{
					rear_hw.iswritePermitted = isRearWriteEnable;
				}

				/* Dequeue a buffer. */
				memset(&(rear_hw.buf0), 0, sizeof (rear_hw.buf0));
				rear_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				rear_hw.buf0.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(rear_hw.dev, VIDIOC_DQBUF, &(rear_hw.buf0));
				if (ret < 0) {
					adbg( "%s: Unable to dequeue buffer0 (%d).\n",__func__, errno);
					//close(front_hw.dev);//notify
					return (void *)0;
				}

				gettimeofday(&ts, NULL);

				//lidbg("Frame[%4u] %u bytes %ld.%06ld %ld.%06ld\n ", i, front_hw.buf0.bytesused, front_hw.buf0.timestamp.tv_sec, front_hw.buf0.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);


				if (i == 0)
					start = ts;
				
				//if (i == 0) originRecSec = front_hw.buf0.timestamp.tv_sec;

				if(i % 150 == 0) 
					Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_OSD_SYNC, 0);

				/*60s*/
				if(rear_hw.iswritePermitted == true)
				{
					if(rear_hw.isEMHandling == true)
					{
						if((rear_hw.buf0.timestamp.tv_sec - originRecSec) % SingleFile_VRTime > (SingleFile_VRTime - EMREC_REQUIRE_TIME_SEC)	)
						{
							int diffsec = (rear_hw.buf0.timestamp.tv_sec - originRecSec) % SingleFile_VRTime - (SingleFile_VRTime - EMREC_REQUIRE_TIME_SEC);
							lidbg("%s: ======== diffsec:%d=======\n",__func__,diffsec);
							originRecSec += diffsec;
						}
						else lidbg("%s: ======== keep recording!=======\n",__func__);
						rear_hw.isEMHandling = false;
					}

				
				    if((((rear_hw.buf0.timestamp.tv_sec - originRecSec) % SingleFile_VRTime == 0) 
							&& (originRecSec != rear_hw.buf0.timestamp.tv_sec))|| (i == 0))
				    {
						lidbg("======== CreateNewVRFile!=======\n");
						originRecSec = rear_hw.buf0.timestamp.tv_sec;

						if((rear_hw.rec_fp > 0) && (i != 0)) //seamless
						{
#if 0						
							fclose(rear_hw.rec_fp);
							strcpy(Old_VR_File_Name, VR_File_Name);
							rear_hw.old_rec_fp = fopen(Old_VR_File_Name, "ab+");
#else
							rear_hw.old_rec_fp = rear_hw.rec_fp;
							rear_hw.rec_fp = NULL;
							Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_DEQUEUE_OLDFP, rear_mhead.msize);
#endif							
						}
						else
							dequeue_flush(rear_mhead.msize, &rear_mhead);
						
						GetCurrentTimeString(timeString);
						sprintf(rear_hw.currentVRFileName, "R%s.h264", timeString);
						sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
						lidbg("=========new VR_File_Name : %s===========\n", VR_File_Name);	
						Flydvr_SetSecondDelProtectFile(rear_hw.currentVRFileName);
						rear_hw.rec_fp = fopen(VR_File_Name, "wb");
					}
				}
				else /*Writting not permmited*/
				{
					if(rear_hw.isEMHandling == true)
					{
						if((((rear_hw.buf0.timestamp.tv_sec - originRecSec) % EMREC_REQUIRE_TIME_SEC == 0) 
							&& (originRecSec != rear_hw.buf0.timestamp.tv_sec)))
						{
							lidbg("======== EM End!(not write)=======\n");
							rear_hw.isEMHandling = false;
						}
					}
				}


				if(rear_mhead.msize < 900)
				{
					/* Record the H264 video file */
					//if(rear_hw.rec_fp > 0)
					//{
#if 0
						fwrite(front_hw.mem0[front_hw.buf0.index], front_hw.buf0.bytesused, 1, front_hw.rec_fp);
#endif
						if(rear_hw.buf0.bytesused == 0)
						{
							//lidbg("%s: ======== data's length = 0!skip!=======\n",__func__);
						}
						else 
							enqueue(rear_hw.mem0[rear_hw.buf0.index], rear_hw.buf0.bytesused,&rear_mhead);
					//}
				}
			

				/* Requeue the buffer. */
				//if (delay > 0)
				//	usleep(delay * 1000);
				if(rear_hw.iswritePermitted == true)
				{
					if((rear_mhead.msize >= 600) && (i % 100 == 0))  //dequeue_to_fp(300, front_hw.rec_fp);
					{
						vdbg("Dequeue!.%d\n",rear_mhead.msize);
						Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_DEQUEUE, 300);
					}
				}
				else
				{
					if(rear_hw.isEMHandling == true)
					{
						if((rear_mhead.msize >= 600) && (i % 100 == 0)) //dequeue_to_fp(300, front_hw.rec_fp);
						{
							vdbg("Dequeue!.%d\n",rear_mhead.msize);
							Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_DEQUEUE, 300);
						}
					}
					else
					{
						if((rear_mhead.msize >= 600) && (i % 100 == 0))  //dequeue_to_fp(300, front_hw.rec_fp);
						{
							vdbg("Flush!.%d\n",rear_mhead.msize);
							Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_FLUSH, 300);
						}
					}
				}
				
				ret = ioctl(rear_hw.dev, VIDIOC_QBUF, &(rear_hw.buf0));
				if (ret < 0) {
					adbg( "%s: Unable to requeue buffer0 (%d).\n",__func__, errno);
					//close(front_hw.dev);//notify
					return (void *)0;
				}
			}

			//fflush(stdout);
		}
		lidbg("%s: X\n", __func__);
        return (void *)0;
   }

  static void* sonixFrontOnlineVRloop(void *par)
    {
        int                 rc, threadPriority;
        int                 buffer_id   = 0;
        pid_t               tid         = 0;
        int                 msgType     = 0;
		int ret;
		unsigned int i;
		unsigned int delay = 0, nframes = (unsigned int)-1;
		struct timeval start, end, ts;
		//bool b_writeVR;
		char timeString[100] = {0};
		char VR_File_Name[255] = {0};
		char Old_VR_File_Name[255] = {0};
		bool iswritePermitted = false;
		unsigned char online_head[11] = {0};
		unsigned char tmp_val = 0;
		bool isFirstIFrame = false;
		
		lidbg("%s: E\n", __func__);

		//if(Flydvr_SDMMC_GetMountState() == SDMMC_IN)
		//	iswritePermitted = true;

		if(Flydvr_CheckOnlineVRPath()== FLY_FALSE)
		{
			lidbg("%s: ======VR Path Not Found!Make New One!======\n", __func__);
			Flydvr_MkOnlineVRPath();
		}
		
        static int loop_count = 0;
        struct timespec start_sp ;
        start_sp.tv_sec = start_sp.tv_nsec = 0;
        clock_gettime(CLOCK_MONOTONIC, &start_sp);

		if(front_online_hw.rec_fp > 0) 
			fclose(front_online_hw.rec_fp);
		front_online_hw.rec_fp = 0;

        front_online_hw.VRCmdPending = 0;

        //tid = androidGetTid();
        tid  = gettid();
        androidSetThreadPriority(tid, ANDROID_PRIORITY_NORMAL);
        prctl(PR_SET_NAME, (unsigned long)"flyaudio VR thread", 0, 0, 0);
        threadPriority = androidGetThreadPriority(tid);

        while(1)
        {
        	int FPS = 0;
			for (i = 0; i < nframes; ++i) {

				/*Counting current framerate*/
				fd_set fds;
	            struct timeval tv;
	            int r = 0;

	            loop_count++;
	            if(loop_count >= 100)//
	            {
	                int diff;
	                struct timespec stop_sp ;
	                stop_sp.tv_sec = stop_sp.tv_nsec = 0;
	                clock_gettime(CLOCK_MONOTONIC, &stop_sp);
	                diff = (stop_sp.tv_sec * 1000 + stop_sp.tv_nsec / 1000000) - (start_sp.tv_sec * 1000 + start_sp.tv_nsec / 1000000);
	                FPS = 1000 * loop_count / diff;
	                lidbg("%s.%d: FPS.%d,[%d,%d ms]\n", __func__, threadPriority, FPS, loop_count, diff);
	                loop_count = 0;
	                clock_gettime(CLOCK_MONOTONIC, &start_sp);
	                //usleep(670 * 1000);
	                //continue;
	            }

	            FD_ZERO(&fds);
	            FD_SET(front_online_hw.dev, &fds);

	            tv.tv_sec = 0;
	            tv.tv_usec = 500000;

	            r = select(front_online_hw.dev + 1, &fds, NULL, NULL, &tv);
	            if (-1 == r)
	            {
	            	lidbg("%s: FDSelect error: %d", __func__, errno);
	                if (EINTR == errno)
	                    continue;
	            }

				//Mutex::Autolock autoLock(front_hw.lock);
	            if(front_online_hw.VRCmdPending)
	            {
	                front_online_hw.VRCmdPending--;
	                if(VR_CMD_EXIT== front_online_hw.VRCmd)
	                {
	                    //front_hw.lock.unlock();
						lidbg("%s: Exiting coz VR_CMD_EXIT\n", __func__);
						Flydvr_ISP_IF_LIB_CloseOnlineFileAccess();
						if(front_online_hw.rec_fp > 0) 
							fclose(front_online_hw.rec_fp);
						front_online_hw.rec_fp = 0;
	                    return (void *)0;
	                }
	            }

				if(FPS > 500) 
				{
					usleep(100*1000);
					continue;//prevent bug
				}

				if(front_online_hw.rec_fp <= 0) 
				{
					sprintf(front_online_hw.currentVRFileName, "tmp0.h264");
					sprintf(VR_File_Name, "%s/%s", MMC0_ONLINE_VR_PATH, front_online_hw.currentVRFileName);
					lidbg("=========online VR_File_Name : %s===========\n", VR_File_Name);	
					front_online_hw.rec_fp = fopen(VR_File_Name, "wb");
					chmod(VR_File_Name,0777);
					Flydvr_ISP_IF_LIB_OpenOnlineFileAccess();
				}	
				iswritePermitted = true;

				/* Dequeue a buffer. */
				memset(&(front_online_hw.buf0), 0, sizeof (front_online_hw.buf0));
				front_online_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				front_online_hw.buf0.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(front_online_hw.dev, VIDIOC_DQBUF, &(front_online_hw.buf0));
				if (ret < 0) {
					lidbg( "Unable to dequeue buffer0 (%d).\n", errno);
					//close(front_hw.dev);//notify
					return (void *)0;
				}

				gettimeofday(&ts, NULL);

				//lidbg("Frame[%4u] %u bytes %ld.%06ld %ld.%06ld\n ", i, front_hw.buf0.bytesused, front_hw.buf0.timestamp.tv_sec, front_hw.buf0.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);


				if (i == 0)
					start = ts;
				
				//if (i == 0) originRecSec = front_hw.buf0.timestamp.tv_sec;

				if(i % 150 == 0) 
					Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_ONLINE_CAM_OSD_SYNC, 0);
				
				if(iswritePermitted == true)
				{			
					/*Compose the head of frame*/
					online_head[0] = front_online_hw.buf0.bytesused >> 24;
					online_head[1] = front_online_hw.buf0.bytesused >> 16;
					online_head[2] = front_online_hw.buf0.bytesused >> 8;
					online_head[3] = front_online_hw.buf0.bytesused;

					online_head[4] = 0;
					online_head[5] = 0;
					online_head[6] = 0;
					online_head[7] = 0x4e;

					online_head[8] = 0;

					tmp_val = *(unsigned char*)(front_online_hw.mem0[front_online_hw.buf0.index] + 17);
					if(tmp_val == 0x68) 
					{
						tmp_val = *(unsigned char*)(front_online_hw.mem0[front_online_hw.buf0.index] + 25);
						if(tmp_val == 0x65) 
						{
							online_head[8] = 0x01;
							isFirstIFrame = true;
						}
					}

					tmp_val = *(unsigned char*)(front_online_hw.mem0[front_online_hw.buf0.index] + 18);
					if(tmp_val == 0x68) 
					{

						/*1280x720*/
						tmp_val = *(unsigned char*)(front_online_hw.mem0[front_online_hw.buf0.index] + 26);
						if(tmp_val == 0x65) 
						{
							online_head[8] = 0x01;
							isFirstIFrame = true;
						}
					}

					tmp_val = *(unsigned char*)(front_online_hw.mem0[front_online_hw.buf0.index] + 19);
					if(tmp_val == 0x68) 
					{
						/*640x360*/
						tmp_val = *(unsigned char*)(front_online_hw.mem0[front_online_hw.buf0.index] + 27);
						if(tmp_val == 0x65)
						{
							online_head[8] = 0x01;
							isFirstIFrame = true;
						}
					}
					if((isFirstIFrame == true) &&  front_online_hw.rec_fp > 0)
					{
#if FRONT_ONLINE_NEED_FRAME_COMPOSE						
						fwrite(online_head, 12 , 1, front_online_hw.rec_fp);//Frame Head
#endif	
						fwrite(front_online_hw.mem0[front_online_hw.buf0.index], front_online_hw.buf0.bytesused, 1, front_online_hw.rec_fp);
					}
				}
				
				ret = ioctl(front_online_hw.dev, VIDIOC_QBUF, &(front_online_hw.buf0));
				if (ret < 0) {
					lidbg("Unable to requeue buffer0 (%d).\n", errno);
					//close(front_hw.dev);	//notify
					return (void *)0;
				}

			}

			//fflush(stdout);
		}
		lidbg("%s: X\n", __func__);
        return (void *)0;
   }

 static void* LPDaemonloop(void *par)
 {
	while(1)
    {
        UINT32 uiMsgId, uiParam1, uiParam2;
		UINT16 usCount;
		UINT8 osdFrontFailTime = 0,osdRearFailTime = 0;
        while (1) {
		    if (Flydvr_GetMessage_LP( &uiMsgId, &uiParam1, &uiParam2) == FLY_FALSE) {
  			    continue;
		    }
       		break;
        }
		//lidbg("=======LPMSG:%d,%d,%d=======",uiMsgId,uiParam1,uiParam2);//tmp for debug

		switch( uiParam1 )
	    {
			case EVENT_FRONT_CAM_DEQUEUE:
				vdbg("=======EVENT_FRONT_CAM_DEQUEUE=======\n");
				dequeue_to_fp(uiParam2, front_hw.rec_fp,&front_hw.iswritePermitted,&front_mhead);
	        break;
			case EVENT_FRONT_CAM_DEQUEUE_OLDFP:
				vdbg("=======EVENT_FRONT_CAM_DEQUEUE_OLDFP=======\n");
				dequeue_to_fp(uiParam2, front_hw.old_rec_fp,&front_hw.iswritePermitted, &front_mhead);
				if(front_hw.old_rec_fp > 0)
				{
					fflush(front_hw.old_rec_fp);
					fclose(front_hw.old_rec_fp);
				}
	        break;
			case EVENT_FRONT_CAM_FLUSH:
				vdbg("=======EVENT_FRONT_CAM_FLUSH=======\n");
				dequeue_flush(uiParam2, &front_mhead);
	        break;
					//dequeue_flush
			case EVENT_REAR_CAM_DEQUEUE:
				vdbg("=======EVENT_REAR_CAM_DEQUEUE=======\n");
				dequeue_to_fp(uiParam2, rear_hw.rec_fp,&rear_hw.iswritePermitted,&rear_mhead);
	        break;
			case EVENT_REAR_CAM_DEQUEUE_OLDFP:
				vdbg("=======EVENT_REAR_CAM_DEQUEUE_OLDFP=======\n");
				dequeue_to_fp(uiParam2, rear_hw.old_rec_fp,&rear_hw.iswritePermitted, &rear_mhead);
				if(rear_hw.old_rec_fp > 0)
				{
					fflush(rear_hw.old_rec_fp);
					fclose(rear_hw.old_rec_fp);
				}
	        break;
			case EVENT_REAR_CAM_FLUSH:
				vdbg("=======EVENT_REAR_CAM_FLUSH=======\n");
				dequeue_flush(uiParam2, &rear_mhead);
	        break;
			case EVENT_FRONT_CAM_OSD_SYNC:
				//lidbg("=======EVENT_FRONT_CAM_OSD_SYNC=======\n");
				{
					time_t timep; 
					struct tm *p; 
					time(&timep); 
					p=localtime(&timep); 
					if(XU_OSD_Set_RTC(front_hw.dev, 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec) <0)
					{	
						lidbg( "front_hw  : XU_OSD_Set_RTC Failed\n");
						if(osdFrontFailTime++ == 5)
						{
							adbg("front_hw: OSD set error! front cam reset!\n");
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DISABLE_FRONT_CAM_POWER, NULL);
							usleep(200*1000);
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_ENABLE_FRONT_CAM_POWER, NULL);
							osdFrontFailTime = 0;
						}
					}
					else osdFrontFailTime = 0;
				}
			break;
			case EVENT_REAR_CAM_OSD_SYNC:
				//lidbg("=======EVENT_REAR_CAM_OSD_SYNC=======\n");
				{
					time_t timep; 
					struct tm *p; 
					time(&timep); 
					p=localtime(&timep); 
					if(XU_OSD_Set_RTC(rear_hw.dev, 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec) <0)
					{	
						lidbg( "rear_hw  : XU_OSD_Set_RTC Failed\n");
						if(osdRearFailTime++ == 5)
						{
							adbg("rear_hw: OSD set error! rear cam reset!\n");
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DISABLE_REAR_CAM_POWER, NULL);
							usleep(200*1000);
							Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_ENABLE_REAR_CAM_POWER, NULL);
							osdRearFailTime = 0;
						}
					}
					else osdRearFailTime = 0;
				}
			break;
			case EVENT_FRONT_ONLINE_CAM_OSD_SYNC:
				//lidbg("=======EVENT_FRONT_ONLINE_CAM_OSD_SYNC=======\n");
				{
					time_t timep; 
					struct tm *p; 
					time(&timep); 
					p=localtime(&timep); 
					if(XU_OSD_Set_RTC(front_online_hw.dev, 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec) <0)
					{	
						lidbg( "front_online_hw  : XU_OSD_Set_RTC Failed\n");
					}
				}
			break;
			case EVENT_GSENSOR_CRASH:
				lidbg("=======EVENT_GSENSOR_CRASH=======\n");
				{
					char VR_File_Name[255] = {0};
					char EM_VR_File_Name[255] = {0};
					char tmpCurrent_File_Name[255] = {0};
					if(front_hw.VREnabledFlag)
					{
						Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_EMERGENCY_LOCK);
						if(front_hw.iswritePermitted == true)
						{
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, front_hw.currentVRFileName);
							sprintf(EM_VR_File_Name, "%s/E%s", MMC1_VR_PATH, front_hw.currentVRFileName);
							
							if(strncmp(front_hw.currentVRFileName, "E",1) != 0 && strncmp(front_hw.currentVRFileName, "L",1) != 0)//prevent multi rename
							{
								strcpy(tmpCurrent_File_Name, front_hw.currentVRFileName);
								sprintf(front_hw.currentVRFileName, "E%s", tmpCurrent_File_Name);
							
								front_hw.iswritePermitted = false;
								lidbg("======== EM_VR_File_Name:%s=======\n",EM_VR_File_Name);	
								if(front_hw.rec_fp != NULL) 
									fclose(front_hw.rec_fp);
								if(rename(VR_File_Name, EM_VR_File_Name) < 0)
									lidbg("========rename fail=======\n");			
								front_hw.rec_fp = fopen(EM_VR_File_Name, "a+b");
								front_hw.iswritePermitted = true;
							}
							else lidbg("========Same File! Just Skip and add delay!=======\n");	
							front_hw.VRCmdPending++;
		       				front_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
						}
						else
						{
							char timeString[100] = {0};
							GetCurrentTimeString(timeString);
							sprintf(front_hw.currentVRFileName, "EF%s.h264", timeString);
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, front_hw.currentVRFileName);
							lidbg("=========new EM_File_Name : %s (not write)===========\n", VR_File_Name);	
							front_hw.rec_fp = fopen(VR_File_Name, "wb");
							
							if(front_mhead.msize > 150) //just save pre 5s
								dequeue_flush(front_mhead.msize - 150, &front_mhead);
							dequeue_to_fp(front_mhead.msize, front_hw.rec_fp,&front_hw.iswritePermitted,&front_mhead);
							front_hw.VRCmdPending++;
	       					front_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
							
						}
					}
					if(rear_hw.VREnabledFlag)
					{
						if(rear_hw.iswritePermitted == true)
						{
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
							sprintf(EM_VR_File_Name, "%s/E%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
							
							if(strncmp(rear_hw.currentVRFileName, "E",1) != 0 && strncmp(rear_hw.currentVRFileName, "L",1) != 0)//prevent multi rename
							{
								strcpy(tmpCurrent_File_Name, rear_hw.currentVRFileName);
								sprintf(rear_hw.currentVRFileName, "E%s", tmpCurrent_File_Name);
							
								rear_hw.iswritePermitted = false;
								lidbg("======== EM_VR_File_Name:%s=======\n",EM_VR_File_Name);	
								if(rear_hw.rec_fp != NULL) 
									fclose(rear_hw.rec_fp);
								if(rename(VR_File_Name, EM_VR_File_Name) < 0)
									lidbg("========rename fail=======\n");			
								rear_hw.rec_fp = fopen(EM_VR_File_Name, "a+b");
								rear_hw.iswritePermitted = true;
							}
							else lidbg("========Same File! Just Skip and add delay!=======\n");	
							rear_hw.VRCmdPending++;
		       				rear_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
						}
						else
						{
							char timeString[100] = {0};
							GetCurrentTimeString(timeString);
							sprintf(rear_hw.currentVRFileName, "ER%s.h264", timeString);
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
							lidbg("=========new EM_File_Name : %s (not write)===========\n", VR_File_Name);	
							rear_hw.rec_fp = fopen(VR_File_Name, "wb");
							
							if(rear_mhead.msize > 150) //just save pre 5s
								dequeue_flush(rear_mhead.msize - 150, &rear_mhead);
							dequeue_to_fp(rear_mhead.msize, rear_hw.rec_fp,&rear_hw.iswritePermitted,&rear_mhead);
							rear_hw.VRCmdPending++;
	       					rear_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
							
						}
					}
				}
			break;
			case EVENT_USER_LOCK:
				lidbg("=======EVENT_USER_LOCK=======\n");
				{
					char VR_File_Name[255] = {0};
					char EM_VR_File_Name[255] = {0};
					char tmpCurrent_File_Name[255] = {0};
					if(front_hw.VREnabledFlag)
					{
						Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_EMERGENCY_LOCK);
						if(front_hw.iswritePermitted == true)
						{
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, front_hw.currentVRFileName);
							sprintf(EM_VR_File_Name, "%s/L%s", MMC1_VR_PATH, front_hw.currentVRFileName);
							
							if(strncmp(front_hw.currentVRFileName, "E",1) != 0 || strncmp(front_hw.currentVRFileName, "L",1) != 0)//prevent multi rename
							{
								strcpy(tmpCurrent_File_Name, front_hw.currentVRFileName);
								sprintf(front_hw.currentVRFileName, "L%s", tmpCurrent_File_Name);
							
								front_hw.iswritePermitted = false;
								lidbg("======== UserLock_File_Name:%s=======\n",EM_VR_File_Name);	
								if(front_hw.rec_fp != NULL) 
									fclose(front_hw.rec_fp);
								if(rename(VR_File_Name, EM_VR_File_Name) < 0)
									lidbg("========rename fail=======\n");			
								front_hw.rec_fp = fopen(EM_VR_File_Name, "a+b");
								front_hw.iswritePermitted = true;
							}
							else lidbg("========UserLock Same File! Just Skip and add delay!=======\n");	
							front_hw.VRCmdPending++;
		       				front_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
						}
						else
						{
							char timeString[100] = {0};
							GetCurrentTimeString(timeString);
							sprintf(front_hw.currentVRFileName, "LF%s.h264", timeString);
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, front_hw.currentVRFileName);
							lidbg("=========UserLock_File_Name : %s (not write)===========\n", VR_File_Name);	
							front_hw.rec_fp = fopen(VR_File_Name, "wb");
							
							if(front_mhead.msize > 150) //just save pre 5s
								dequeue_flush(front_mhead.msize - 150, &front_mhead);
							dequeue_to_fp(front_mhead.msize, front_hw.rec_fp,&front_hw.iswritePermitted,&front_mhead);
							front_hw.VRCmdPending++;
	       					front_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
							
						}
					}
					if(rear_hw.VREnabledFlag)
					{
						if(rear_hw.iswritePermitted == true)
						{
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
							sprintf(EM_VR_File_Name, "%s/L%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
							
							if(strncmp(rear_hw.currentVRFileName, "E",1) != 0 || strncmp(rear_hw.currentVRFileName, "L",1) != 0)//prevent multi rename
							{
								strcpy(tmpCurrent_File_Name, rear_hw.currentVRFileName);
								sprintf(rear_hw.currentVRFileName, "L%s", tmpCurrent_File_Name);
							
								rear_hw.iswritePermitted = false;
								lidbg("======== UserLock_File_Name:%s=======\n",EM_VR_File_Name);	
								if(rear_hw.rec_fp != NULL) 
									fclose(rear_hw.rec_fp);
								if(rename(VR_File_Name, EM_VR_File_Name) < 0)
									lidbg("========rename fail=======\n");			
								rear_hw.rec_fp = fopen(EM_VR_File_Name, "a+b");
								rear_hw.iswritePermitted = true;
							}
							else lidbg("========UserLock Same File! Just Skip and add delay!=======\n");	
							rear_hw.VRCmdPending++;
		       				rear_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
						}
						else
						{
							char timeString[100] = {0};
							GetCurrentTimeString(timeString);
							sprintf(rear_hw.currentVRFileName, "LR%s.h264", timeString);
							sprintf(VR_File_Name, "%s/%s", MMC1_VR_PATH, rear_hw.currentVRFileName);
							lidbg("=========UserLock_Name : %s (not write)===========\n", VR_File_Name);	
							rear_hw.rec_fp = fopen(VR_File_Name, "wb");
							
							if(rear_mhead.msize > 150) //just save pre 5s
								dequeue_flush(rear_mhead.msize - 150, &rear_mhead);
							dequeue_to_fp(rear_mhead.msize, rear_hw.rec_fp,&rear_hw.iswritePermitted,&rear_mhead);
							rear_hw.VRCmdPending++;
	       					rear_hw.VRCmd         = VR_CMD_GSENSOR_CRASH;
							
						}
					}
				}
			break;
			case EVENT_FRONT_PAUSE:
				lidbg("=======EVENT_FRONT_PAUSE=======\n");
				isFrontWriteEnable = false;
				front_hw.VRCmdPending++;
       			front_hw.VRCmd         = VR_CMD_PAUSE;
			break;
			case EVENT_FRONT_RESUME:
				lidbg("=======EVENT_FRONT_RESUME=======\n");
				if(Flydvr_CheckVRPath(FLYDVR_MEDIA_MMC1)== FLY_FALSE)
				{
					lidbg("%s: ======VR Path Not Found!Make New One!======\n", __func__);
					Flydvr_MkVRPath(FLYDVR_MEDIA_MMC1);
				}
				isFrontWriteEnable = true;
				front_hw.VRCmdPending++;
       			front_hw.VRCmd         = VR_CMD_RESUME;
			break;
			case EVENT_REAR_PAUSE:
				lidbg("=======EVENT_REAR_PAUSE=======\n");
				isRearWriteEnable = false;
				rear_hw.VRCmdPending++;
       			rear_hw.VRCmd         = VR_CMD_PAUSE;
			break;
			case EVENT_REAR_RESUME:
				lidbg("=======EVENT_REAR_RESUME=======\n");
				if(Flydvr_CheckVRPath(FLYDVR_MEDIA_MMC1)== FLY_FALSE)
				{
					lidbg("%s: ======VR Path Not Found!Make New One!======\n", __func__);
					Flydvr_MkVRPath(FLYDVR_MEDIA_MMC1);
				}
				isRearWriteEnable = true;
				rear_hw.VRCmdPending++;
       			rear_hw.VRCmd         = VR_CMD_RESUME;
			break;
#if 0			
			case EVENT_FRONT_ONLINE_PAUSE:
				lidbg("=======EVENT_FRONT_ONLINE_PAUSE=======\n");
				isFrontOnlineWriteEnable = false;
				front_online_hw.VRCmdPending++;
       			front_online_hw.VRCmd         = VR_CMD_PAUSE;
			break;
			case EVENT_FRONT_ONLINE_RESUME:
				lidbg("=======EVENT_FRONT_ONLINE_RESUME=======\n");
				if(Flydvr_CheckOnlineVRPath()== FLY_FALSE)
				{
					lidbg("%s: ======VR Path Not Found!Make New One!======\n", __func__);
					Flydvr_MkOnlineVRPath();
				}
				isFrontOnlineWriteEnable = true;
				front_online_hw.VRCmdPending++;
       			front_online_hw.VRCmd         = VR_CMD_RESUME;
			break;
#endif			
			case EVENT_VIDEO_KEY_SINGLE_FILE_TIME:
				lidbg("=======EVENT_VIDEO_KEY_SINGLE_FILE_TIME : [%d] secs=======\n",uiParam2 * 60);
				singleFileVRTime = uiParam2 * 60;
				front_hw.VRCmdPending++;
       			front_hw.VRCmd         = VR_CMD_CHANGE_SINGLE_FILE_TIME;
				rear_hw.VRCmdPending++;
       			rear_hw.VRCmd         = VR_CMD_CHANGE_SINGLE_FILE_TIME;
			break;
	    }
    }
	return (void*)0;
 }

 static int sonix_launch_FrontVR_thread()
    {
        int rc = 0;
		lidbg("%s: E\n", __func__);
		
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&(front_hw.VRThread), &attr, sonixFrontVRloop, NULL);

        lidbg("%s: X\n", __func__);
        return rc;
}

  static int sonix_launch_RearVR_thread()
    {
        int rc = 0;
		lidbg("%s: E\n", __func__);
		
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&(rear_hw.VRThread), &attr, sonixRearVRloop, NULL);

        lidbg("%s: X\n", __func__);
        return rc;
}

 static int sonix_launch_FrontOnlineVR_thread()
    {
        int rc = 0;
		lidbg("%s: E\n", __func__);
		
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&(front_online_hw.VRThread), &attr, sonixFrontOnlineVRloop, NULL);

        lidbg("%s: X\n", __func__);
        return rc;
}


INT32 Sonix_ISP_IF_LIB_StartLPDaemon()
{
	int rc = 0;
	lidbg("%s: E\n", __func__);
	
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
      pthread_create(&thread_LP_daemon_id, &attr, LPDaemonloop, NULL);

      lidbg("%s: X\n", __func__);
      return rc;
}

INT32 Sonix_ISP_IF_LIB_StartFrontVR()
{
	unsigned int freeram;
	double m_BitRate;
	
	unsigned int i;
	int ret = 0;
	char dev_name[255];
	int fail_cnt = 0;

	lidbg("%s: E\n", __func__);

	if(front_hw.VREnabledFlag)
	{
		lidbg("%s: Reopen!Return!\n", __func__);
		return 0;
	}

	sonixInitFrontDefaultParameters();

OpenFrontDev:
	/* Open the video device. */
	if(Sonix_ISP_IF_LIB_GetFrontCamDevName(dev_name) == FLY_FALSE)
	{
		adbg("%s: Sonix_ISP_IF_LIB_GetFrontCamDevName Failed\n", __func__);
		return 1;
	}
	
	front_hw.dev = sonix_video_open(dev_name);
	if (front_hw.dev < 0)
	{
		adbg("%s: sonix_video_open Failed\n", __func__);
		/*Protect Procedue*/
		fail_cnt++;
		if(fail_cnt >= 5)
		{
			fail_cnt = 0;
			adbg("%s: Try open timeout!Return!%d\n", __func__,fail_cnt);
			return 1;
		}
		else
		{
			adbg("%s: Try open again!%d\n", __func__,fail_cnt);
			usleep(200*1000);
			goto OpenFrontDev;
		}
	}

	if (sonix_video_set_format(front_hw.dev, front_hw.VRWidth, front_hw.VRHeight, front_hw.VRFormat) < 0) {
			lidbg(" === Set Format Failed : skip for H264 ===  \n");
	}

	/* Set the frame rate. */
	if (sonix_video_set_framerate(front_hw.dev,  front_hw.VRFps , NULL) < 0)		
	{
		adbg("%s: sonix_video_set_framerate Failed\n", __func__);
		close(front_hw.dev);	
		return 1;
	}

	if(XU_Ctrl_ReadChipID(front_hw.dev) < 0)
		lidbg("%s: XU_Ctrl_ReadChipID Failed\n", __func__);

	if(XU_H264_Set_Mode(front_hw.dev, front_hw.VRBitRateMode) < 0)
		lidbg("%s: XU_H264_Set_Mode Failed\n", __func__);
	if(XU_H264_Set_BitRate(front_hw.dev, front_hw.VRBitRate) < 0 )
		lidbg("%s: XU_H264_Set_BitRate Failed\n", __func__);
	XU_H264_Get_BitRate(front_hw.dev, &m_BitRate);
	if(m_BitRate < 0 )
		lidbg("%s: XU_H264_Get_BitRate Failed\n", __func__);
	else 
		lidbg("%s: Current bit rate: %.2f Kbps\n", __func__,m_BitRate);

	if(XU_OSD_Set_Enable(front_hw.dev, 1, 1) <0)
		lidbg( "XU_OSD_Set_Enable Failed\n");	

	if(XU_OSD_Set_CarcamCtrl(front_hw.dev, 0, 0, 0) < 0)
			lidbg( "XU_OSD_Set_CarcamCtrl Failed\n");	

	Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_OSD_SYNC, 0);

	if(GetFreeRam(&freeram) && freeram<1843200*(front_hw.nbufs)+4194304)
	{
		lidbg( "free memory isn't enough(%d),But still continue\n",freeram);		
		//return 1;
	}

	/* Allocate buffers. */
	if ((int)(front_hw.nbufs = sonix_video_reqbufs(front_hw.dev, front_hw.nbufs)) < 0)
	{
		adbg("%s: sonix_video_reqbufs Failed\n", __func__);
		close(front_hw.dev);	
		return 1;
	}

	/* Map the buffers. */
	for (i = 0; i < front_hw.nbufs; ++i) {
		memset(&(front_hw.buf0), 0, sizeof (front_hw.buf0));
		front_hw.buf0.index = i;
		front_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		front_hw.buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(front_hw.dev, VIDIOC_QUERYBUF, &(front_hw.buf0));
		if (ret < 0) {
			adbg( "Unable to query buffer %u (%d).\n", i, errno);
			close(front_hw.dev);		
			return 1;
		}
		lidbg( "length: %u offset: %10u     --  \n", front_hw.buf0.length, front_hw.buf0.m.offset);

		front_hw.mem0[i] = mmap(0, front_hw.buf0.length, PROT_READ, MAP_SHARED, front_hw.dev, front_hw.buf0.m.offset);
		if (front_hw.mem0[i] == MAP_FAILED) {
			adbg( "Unable to map buffer %u (%d)\n", i, errno);
			close(front_hw.dev);		
			return 1;
		}
		lidbg("Buffer %u mapped at address %p.\n", i, front_hw.mem0[i]);
	}

	/* Queue the buffers. */
	for (i = 0; i < front_hw.nbufs; ++i) {
		memset(&(front_hw.buf0), 0, sizeof (front_hw.buf0));
		front_hw.buf0.index = i;
		front_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		front_hw.buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(front_hw.dev, VIDIOC_QBUF, &(front_hw.buf0));
		if (ret < 0) {
			adbg( "Unable to queue buffer0(%d).\n", errno);
			close(front_hw.dev);		
			return 1;
		}
	}

	/* Start streaming. */
	sonix_video_enable(front_hw.dev, 1);

	cam_list_init(&front_mhead.list);
	
	ret = sonix_launch_FrontVR_thread();

    if(!ret)
            front_hw.VREnabledFlag = 1;

	lidbg("%s: X\n", __func__);
	return 0;
}

INT32 Sonix_ISP_IF_LIB_StopFrontVR()
{
	lidbg("%s: E\n", __func__);
	sonix_stop_VR(&front_hw);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_RECORD_STOP);
	lidbg("%s: X\n", __func__);
	return 0;
}

INT32 Sonix_ISP_IF_LIB_ResumeFrontVR()
{
	lidbg("%s: E\n", __func__);
	sonix_resume_VR(&front_hw);
	lidbg("%s: X\n", __func__);
	return 0;
}

INT32 Sonix_ISP_IF_LIB_PauseFrontVR()
{
	lidbg("%s: E\n", __func__);
	sonix_pause_VR(&front_hw);
	lidbg("%s: X\n", __func__);
	return 0;
}


INT32 Sonix_ISP_IF_LIB_StartRearVR()
{
	unsigned int freeram;
	double m_BitRate;
	
	unsigned int i;
	int ret = 0;
	char dev_name[255];
	int fail_cnt = 0;

	lidbg("%s: E\n", __func__);

	if(rear_hw.VREnabledFlag)
	{
		lidbg("%s: Reopen!Return!\n", __func__);
		return 0;
	}

	sonixInitRearDefaultParameters();

OpenRearDev:
	/* Open the video device. */
	if(Sonix_ISP_IF_LIB_GetRearCamDevName(dev_name) == FLY_FALSE)
	{
		adbg("%s: Sonix_ISP_IF_LIB_GetRearCamDevName Failed\n", __func__);
		return 1;
	}
	
	rear_hw.dev = sonix_video_open(dev_name);
	if (rear_hw.dev < 0)
	{
		adbg("%s: sonix_video_open Failed\n", __func__);
		/*Protect Procedue*/
		fail_cnt++;
		if(fail_cnt >= 5)
		{
			fail_cnt = 0;
			adbg("%s: Try open timeout!Return!%d\n", __func__,fail_cnt);
			return 1;
		}
		else
		{
			adbg("%s: Try open again!%d\n", __func__,fail_cnt);
			usleep(200*1000);
			goto OpenRearDev;
		}
	}

	if (sonix_video_set_format(rear_hw.dev, rear_hw.VRWidth, rear_hw.VRHeight, rear_hw.VRFormat) < 0) {
			lidbg(" === Set Format Failed : skip for H264 ===  \n");
	}

	/* Set the frame rate. */
	if (sonix_video_set_framerate(rear_hw.dev,  rear_hw.VRFps , NULL) < 0)
	{
		adbg("%s: sonix_video_set_framerate Failed\n", __func__);
		close(rear_hw.dev);	
		return 1;
	}

	if(XU_Ctrl_ReadChipID(rear_hw.dev) < 0)
		lidbg("%s: XU_Ctrl_ReadChipID Failed\n", __func__);

	if(XU_H264_Set_Mode(rear_hw.dev, rear_hw.VRBitRateMode) < 0)
		lidbg("%s: XU_H264_Set_Mode Failed\n", __func__);
	if(XU_H264_Set_BitRate(rear_hw.dev, rear_hw.VRBitRate) < 0 )
		lidbg("%s: XU_H264_Set_BitRate Failed\n", __func__);
	XU_H264_Get_BitRate(rear_hw.dev, &m_BitRate);
	if(m_BitRate < 0 )
		lidbg("%s: XU_H264_Get_BitRate Failed\n", __func__);
	else 
		lidbg("%s: Current bit rate: %.2f Kbps\n", __func__,m_BitRate);

	if(XU_OSD_Set_Enable(rear_hw.dev, 1, 1) <0)
		lidbg( "XU_OSD_Set_Enable Failed\n");	

	if(XU_OSD_Set_CarcamCtrl(rear_hw.dev, 0, 0, 0) < 0)
			lidbg( "XU_OSD_Set_CarcamCtrl Failed\n");	

	Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_OSD_SYNC, 0);

	if(GetFreeRam(&freeram) && freeram<1843200*(rear_hw.nbufs)+4194304)
	{
		lidbg( "free memory isn't enough(%d),But still continue\n",freeram);		
		//return 1;
	}

	/* Allocate buffers. */
	if ((int)(rear_hw.nbufs = sonix_video_reqbufs(rear_hw.dev, rear_hw.nbufs)) < 0) 
	{
		adbg("%s: sonix_video_reqbufs Failed\n", __func__);
		close(rear_hw.dev);	
		return 1;
	}

	/* Map the buffers. */
	for (i = 0; i < rear_hw.nbufs; ++i) {
		memset(&(rear_hw.buf0), 0, sizeof (rear_hw.buf0));
		rear_hw.buf0.index = i;
		rear_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		rear_hw.buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(rear_hw.dev, VIDIOC_QUERYBUF, &(rear_hw.buf0));
		if (ret < 0) {
			adbg( "Unable to query buffer %u (%d).\n", i, errno);
			close(rear_hw.dev);		
			return 1;
		}
		lidbg( "length: %u offset: %10u     --  \n", rear_hw.buf0.length, rear_hw.buf0.m.offset);

		rear_hw.mem0[i] = mmap(0, rear_hw.buf0.length, PROT_READ, MAP_SHARED, rear_hw.dev, rear_hw.buf0.m.offset);
		if (rear_hw.mem0[i] == MAP_FAILED) {
			adbg( "Unable to map buffer %u (%d)\n", i, errno);
			close(rear_hw.dev);		
			return 1;
		}
		lidbg("Buffer %u mapped at address %p.\n", i, rear_hw.mem0[i]);
	}

	/* Queue the buffers. */
	for (i = 0; i < rear_hw.nbufs; ++i) {
		memset(&(rear_hw.buf0), 0, sizeof (rear_hw.buf0));
		rear_hw.buf0.index = i;
		rear_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		rear_hw.buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(rear_hw.dev, VIDIOC_QBUF, &(rear_hw.buf0));
		if (ret < 0) {
			adbg( "Unable to queue buffer0(%d).\n", errno);
			close(rear_hw.dev);		
			return 1;
		}
	}

	/* Start streaming. */
	sonix_video_enable(rear_hw.dev, 1);

	cam_list_init(&rear_mhead.list);
	
	ret = sonix_launch_RearVR_thread();

    if(!ret)
            rear_hw.VREnabledFlag = 1;

	lidbg("%s: X\n", __func__);
	return 0;
}

INT32 Sonix_ISP_IF_LIB_StopRearVR()
{
	lidbg("%s: E\n", __func__);
	sonix_stop_VR(&rear_hw);
	lidbg("%s: X\n", __func__);
	return 0;
}

INT32 Sonix_ISP_IF_LIB_ResumeRearVR()
{
	lidbg("%s: E\n", __func__);
	sonix_resume_VR(&rear_hw);
	lidbg("%s: X\n", __func__);
	return 0;
}

INT32 Sonix_ISP_IF_LIB_PauseRearVR()
{
	lidbg("%s: E\n", __func__);
	sonix_pause_VR(&rear_hw);
	lidbg("%s: X\n", __func__);
	return 0;
}


INT32 Sonix_ISP_IF_LIB_StartFrontOnlineVR()
{
	unsigned int freeram;
	double m_BitRate;
	
	unsigned int i;
	int ret = 0;
	char dev_name[255];

	lidbg("%s: E\n", __func__);

	if(front_online_hw.VREnabledFlag)
	{
		lidbg("%s: Reopen!Return!\n", __func__);
		return 0;
	}

	sonixInitFrontOnlineDefaultParameters();
	
	/* Open the video device. */
	if(Sonix_ISP_IF_LIB_GetFrontCamDevName(dev_name) == FLY_FALSE)
		return 1;
	
	front_online_hw.dev = sonix_video_open(dev_name);
	if (front_online_hw.dev < 0)
		return 1;

	if (sonix_video_set_format(front_online_hw.dev, front_online_hw.VRWidth, front_online_hw.VRHeight, front_online_hw.VRFormat) < 0) {
			lidbg(" === Set Format Failed : skip for H264 ===  \n");
	}

	/* Set the frame rate. */
	if (sonix_video_set_framerate(front_online_hw.dev,  front_online_hw.VRFps , NULL) < 0) {
		close(front_online_hw.dev);		
		return 1;
	}

	if(XU_Ctrl_ReadChipID(front_online_hw.dev) < 0)
		lidbg("%s: XU_Ctrl_ReadChipID Failed\n", __func__);

	if(XU_H264_Set_Mode(front_online_hw.dev, front_online_hw.VRBitRateMode) < 0)
		lidbg("%s: XU_H264_Set_Mode Failed\n", __func__);
	if(XU_H264_Set_BitRate(front_online_hw.dev, front_online_hw.VRBitRate) < 0 )
		lidbg("%s: XU_H264_Set_BitRate Failed\n", __func__);
	XU_H264_Get_BitRate(front_online_hw.dev, &m_BitRate);
	if(m_BitRate < 0 )
		lidbg("%s: XU_H264_Get_BitRate Failed\n", __func__);
	else 
		lidbg("%s: Current bit rate: %.2f Kbps\n", __func__,m_BitRate);

	if(XU_OSD_Set_Enable(front_online_hw.dev, 1, 1) <0)
		lidbg( "XU_OSD_Set_Enable Failed\n");	

	if(XU_OSD_Set_CarcamCtrl(front_online_hw.dev, 0, 0, 0) < 0)
			lidbg( "XU_OSD_Set_CarcamCtrl Failed\n");	

	Flydvr_SendMessage_LP(FLYM_UI_NOTIFICATION, EVENT_FRONT_ONLINE_CAM_OSD_SYNC, 0);

	if(GetFreeRam(&freeram) && freeram<1843200*(front_online_hw.nbufs)+4194304)
	{
		lidbg( "free memory isn't enough(%d),But still continue\n",freeram);		
		//return 1;
	}

	/* Allocate buffers. */
	if ((int)(front_online_hw.nbufs = sonix_video_reqbufs(front_online_hw.dev, front_online_hw.nbufs)) < 0) {
		close(front_online_hw.dev);
		return 1;
	}

	/* Map the buffers. */
	for (i = 0; i < front_online_hw.nbufs; ++i) {
		memset(&(front_online_hw.buf0), 0, sizeof (front_online_hw.buf0));
		front_online_hw.buf0.index = i;
		front_online_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		front_online_hw.buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(front_online_hw.dev, VIDIOC_QUERYBUF, &(front_online_hw.buf0));
		if (ret < 0) {
			lidbg( "Unable to query buffer %u (%d).\n", i, errno);
			close(front_online_hw.dev);		
			return 1;
		}
		lidbg( "length: %u offset: %10u     --  \n", front_online_hw.buf0.length, front_online_hw.buf0.m.offset);

		front_online_hw.mem0[i] = mmap(0, front_online_hw.buf0.length, PROT_READ, MAP_SHARED, front_online_hw.dev, front_online_hw.buf0.m.offset);
		if (front_online_hw.mem0[i] == MAP_FAILED) {
			lidbg( "Unable to map buffer %u (%d)\n", i, errno);
			close(front_online_hw.dev);		
			return 1;
		}
		lidbg("Buffer %u mapped at address %p.\n", i, front_online_hw.mem0[i]);
	}

	/* Queue the buffers. */
	for (i = 0; i < front_online_hw.nbufs; ++i) {
		memset(&(front_online_hw.buf0), 0, sizeof (front_online_hw.buf0));
		front_online_hw.buf0.index = i;
		front_online_hw.buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		front_online_hw.buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(front_online_hw.dev, VIDIOC_QBUF, &(front_online_hw.buf0));
		if (ret < 0) {
			lidbg( "Unable to queue buffer0(%d).\n", errno);
			close(front_online_hw.dev);		
			return 1;
		}
	}

	/* Start streaming. */
	sonix_video_enable(front_online_hw.dev, 1);

	//cam_list_init(&front_mhead.list);
	
	ret = sonix_launch_FrontOnlineVR_thread();

    if(!ret)
            front_online_hw.VREnabledFlag = 1;

	lidbg("%s: X\n", __func__);
	return 0;
}


INT32 Sonix_ISP_IF_LIB_StopFrontOnlineVR()
{
	lidbg("%s: E\n", __func__);
	sonix_stop_VR(&front_online_hw);
	lidbg("%s: X\n", __func__);
	return 0;
}
