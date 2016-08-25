/*
 *      test.c  --  USB Video Class test application
 *
 *      Copyright (C) 2005-2008
 *          Laurent Pinchart (laurent.pinchart@skynet.be)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */

/*
 * WARNING: This is just a test application. Don't fill bug reports, flame me,
 * curse me on 7 generations :-).
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>		//for timestamp incomplete error in kernel 2.6.21
#include <linux/videodev2.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <pthread.h>

#include "v4l2uvc.h"
#include "sonix_xu_ctrls.h"
#include "nalu.h"
#include "debug.h"
#include "cap_desc_parser.h"
#include "cap_desc.h"
#include <cutils/properties.h>
#include "lidbg_servicer.h"
#include<time.h> 

#include <dirent.h>  
#include <sys/stat.h>  
#include <sys/types.h> 
#include <linux/rtc.h>
#include <sys/vfs.h>

#include "../inc/lidbg_flycam_app.h"

static int lidbg_get_current_time(char isXUSet , char *time_string, struct rtc_time *ptm);
static void send_driver_msg(char magic ,char nr,unsigned long arg);
int find_earliest_file(char* Dir,char* minRecName);


#define TESTAP_VERSION		"v1.0.21_SONiX_UVC_TestAP_Multi"

#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif

#ifndef V4L2_PIX_FMT_H264
#define V4L2_PIX_FMT_H264 v4l2_fourcc('H','2','6','4') /* H264 */
#endif
#define V4L2_PIX_FMT_MP2T v4l2_fourcc('M','P','2','T') /* MPEG-2 TS */

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)<<16+(b)<<8+(c))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,32)
#define V4L_BUFFERS_DEFAULT	6//6,16
#define V4L_BUFFERS_MAX		16//16,32
#else
#define V4L_BUFFERS_DEFAULT	3
#define V4L_BUFFERS_MAX		3
#endif
// chris +
#define H264_SIZE_HD				((1280<<16)|720)
#define H264_SIZE_VGA				((640<<16)|480)
#define H264_SIZE_QVGA				((320<<16)|240)
#define H264_SIZE_QQVGA				((160<<16)|112)
#define H264_SIZE_360P				((640<<16)|360)
#define H264_SIZE_180P				((320<<16)|180)

#define MULTI_STREAM_HD_QVGA		0x01
#define MULTI_STREAM_HD_180P		0x02
#define MULTI_STREAM_HD_360P		0x04
#define MULTI_STREAM_HD_VGA			0x08
#define MULTI_STREAM_HD_QVGA_VGA	0x10
#define MULTI_STREAM_QVGA_VGA	    0x20
#define MULTI_STREAM_HD_180P_360P	0x40
#define MULTI_STREAM_360P_180P	    0x80

//eho
#define	NIGHT_GAINVAL					18
#define	NIGHT_CONTRASTVAL			64
#define	NIGHT_SATURATIONVAL		39
#define	NIGHT_BRIGHTVAL				95
#define	NIGHT_EXPOSUREVAL			619

#define	DAY_GAINVAL					0
#define	DAY_CONTRASTVAL			50
#define	DAY_SATURATIONVAL		71
#define	DAY_BRIGHTVAL				53

#if ANDROID_VERSION >= 600
#define EMMC_MOUNT_POINT0  "/storage/emulated/0"
#define EMMC_MOUNT_POINT1  "/storage/emulated/1"
#else
#define EMMC_MOUNT_POINT0  "/storage/sdcard0"
#define EMMC_MOUNT_POINT1  "/storage/sdcard1"
#endif

//flyaudio
#define NONE_HUB_SUPPORT	0

//#define REC_SAVE_DIR	EMMC_MOUNT_POINT0"/camera_rec/"
char Rec_Save_Dir[100] = EMMC_MOUNT_POINT1"/camera_rec/";
char Em_Save_Dir[100] = EMMC_MOUNT_POINT1"/camera_rec/BlackBox/";
char Em_Save_Tmp_Dir[100] = EMMC_MOUNT_POINT1"/camera_rec/BlackBox//.tmp/";
char Capture_Save_Dir[100] = EMMC_MOUNT_POINT0"/preview_cache/";

int Max_Rec_Num = 1;
int Rec_Sec = 300;//s
//int Em_Sec = 10;//s
unsigned int Rec_File_Size = 8192;//MB
unsigned int Rec_Bitrate = 8000000;//b/s
int isDualCam = 0;
int isColdBootRec = 0;
int isBlackBoxTopRec = 0;
int isBlackBoxBottomRec = 0;
int isBlackBoxTopWaitDequeue = 0;
int isDequeue = 0;
int isOldFp = 0;
int isRemainOldFp = 0;
int isVideoLoop = 0;
int isDelDaysFile = 0;
int delDays = 6;
int oldisVideoLoop = 0;
int isThinkNavi = 0;
int isDisableVideoLoop = 0;
int isNewFile = 0;
int isToDel = 0;
int isTranscoding = 0;
int isEmPermit = 0;
int isBrokenIFrame = 0;
int isConvertMP4 = 0;


// chris -

struct H264Format *gH264fmt = NULL;
int Dbg_Param = 0x1f;

//lidbg_parm
char startRecording[PROPERTY_VALUE_MAX];
char Res_String[PROPERTY_VALUE_MAX];
char Rec_Sec_String[PROPERTY_VALUE_MAX];
char Max_Rec_Num_String[PROPERTY_VALUE_MAX];
char Rec_File_Size_String[PROPERTY_VALUE_MAX];
char Rec_Bitrate_String[PROPERTY_VALUE_MAX];
char isDualCam_String[PROPERTY_VALUE_MAX];
char isColdBootRec_String[PROPERTY_VALUE_MAX];
char isBlackBoxRec[PROPERTY_VALUE_MAX];
char Em_Top_Sec_String[PROPERTY_VALUE_MAX];
char Em_Bottom_Sec_String[PROPERTY_VALUE_MAX];
char Wait_Deq_Str[PROPERTY_VALUE_MAX];
char isVideoLoop_Str[PROPERTY_VALUE_MAX];
char isTranscoding_Str[PROPERTY_VALUE_MAX];
char isEmPermit_Str[PROPERTY_VALUE_MAX];
char isDelDaysFile_Str[PROPERTY_VALUE_MAX];
char delDays_Str[PROPERTY_VALUE_MAX];
char startNight[PROPERTY_VALUE_MAX];
char isConvertMP4_Str[PROPERTY_VALUE_MAX];
//char startCapture[PROPERTY_VALUE_MAX];

unsigned char isPreview = 0;
int dev,flycam_fd;
unsigned int originRecsec = 0;
unsigned int oldRecsec = 1;
int isIframe = 0;

unsigned int oldFrameSize = 0;

int cam_id = -1;

FILE *rec_fp1 = NULL;
FILE *old_rec_fp1 = NULL;

unsigned int tmp_count = 0;

char isNormDequeue = 0;
char isTopDequeue = 0;

static unsigned int Emergency_Top_Sec = 5,Emergency_Bottom_Sec = 60;
int BlackBoxBottomCnt = 0;

static unsigned int top_totalFrames ,bottom_totalFrames;
static unsigned int top_lastFrames ,bottom_lastFrames;

void *iFrameData;
int iframe_length;

static int iframe_diff_val,iframe_threshold_val;
struct v4l2_buffer buf0;
struct v4l2_buffer buf1;

//lidbg("CAMID[%d] :",cam_id);
#define camdbg(msg...) do{\
	lidbg(msg);\
}while(0)

struct thread_parameter
{
	struct v4l2_buffer *buf;
	void *mem[16];
	int *dev;
	unsigned int *nframes ;
	unsigned char multi_stream_mjpg_enable;
};

#define member_of(ptr, type, member) ({ \
  const typeof(((type *)0)->member) *__mptr = (ptr); \
  (type *)((char *)__mptr - offsetof(type,member));})

struct cam_list {
  struct cam_list *next, *prev;
};

struct trans_list {
  struct trans_list *next, *prev;
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
} camera_q_node;

typedef struct {
    struct trans_list list;
    char dest[100];
	char src[100];
} trans_q_node;

camera_q_node mhead; /* dummy head */
unsigned int msize;
pthread_mutex_t alock;

trans_q_node trans_head; /* dummy head */
unsigned int trans_size;
pthread_mutex_t trans_lock;

bool pri_enqueue(void *data,int count)
{
	void *tmpData;
	tmpData = malloc(count);  
	memcpy(tmpData, data, count);
    camera_q_node *node =
        (camera_q_node *)malloc(sizeof(camera_q_node));
    if (NULL == node) {
        ALOGE("%s: No memory for camera_q_node", __func__);
        return false;
    }

    memset(node, 0, sizeof(camera_q_node));
    node->data = tmpData;
	node->length = count;

    //pthread_mutex_lock(&mlock);
    struct cam_list *p_next = mhead.list.next;

    mhead.list.next = &node->list;
    p_next->prev = &node->list;
    node->list.next = p_next;
    node->list.prev = &mhead.list;

    msize++;
    //pthread_mutex_unlock(&mlock);
    return true;
}

bool enqueue(void *data,int count)
{
	void *tmpData;
	tmpData = malloc(count);  
	memcpy(tmpData, data, count);
    camera_q_node *node =
        (camera_q_node *)malloc(sizeof(camera_q_node));
    if (NULL == node) {
        lidbg("%s: No memory for camera_q_node", __func__);
        return false;
    }

    memset(node, 0, sizeof(camera_q_node));
    node->data = tmpData;
	node->length = count;

    pthread_mutex_lock(&alock);
    cam_list_add_tail_node(&node->list, &mhead.list);
    msize++;
	if((msize >  (Emergency_Top_Sec * 30 *2)) &&  (msize % 100 == 0)) 
		lidbg("[%d]:=====enqueue => %d======\n",cam_id,msize);
	//free(tmpData);
    pthread_mutex_unlock(&alock);
    return true;
}

int query_length()
{
	int length;
	camera_q_node* node = NULL;
    //void* data = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;

    pthread_mutex_lock(&alock);
    head = &mhead.list;
    pos = head->next;
    if (pos != head) {
        node = member_of(pos, camera_q_node, list);
    }
    pthread_mutex_unlock(&alock);
	
	if (NULL != node)  return node->length;
	else return 0;
}

int dequeue(void* data)
{
	int ret;
    camera_q_node* node = NULL;
    //void* data = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;

    pthread_mutex_lock(&alock);
    head = &mhead.list;
    pos = head->next;
    if (pos != head) {
        node = member_of(pos, camera_q_node, list);
        cam_list_del_node(&node->list);
        msize--;
    }
    pthread_mutex_unlock(&alock);

    if (NULL != node) {
        //data = node->data;
        memcpy(data, node->data, node->length);
		ret =  node->length;
        free(node->data);
		free(node);
    }

    return ret;
}

#if 0
bool trans_enqueue(char* dest,char* src)
{
    trans_q_node *node =
        (trans_q_node *)malloc(sizeof(trans_q_node));
    if (NULL == node) {
        lidbg("%s: No memory for trans_q_node", __func__);
        return false;
    }
	lidbg("==dest:%s,src:%s==\n",dest,src);
    memset(node, 0, sizeof(trans_q_node));
	strcpy(node->dest, dest);	
	strcpy(node->src, src);	

    pthread_mutex_lock(&trans_lock);
    cam_list_add_tail_node(&node->list, &trans_head.list);
    trans_size++;
    pthread_mutex_unlock(&trans_lock);
    return true;
}
#endif

#if 0
static void pantilt(int dev, char *dir, char *length)
{
	struct v4l2_ext_control xctrls[2];
	struct v4l2_ext_controls ctrls;
	unsigned int angle = atoi(length);

	char directions[9][2] = {
		{ -1,  1 },
		{  0,  1 },
		{  1,  1 },
		{ -1,  0 },
		{  0,  0 },
		{  1,  0 },
		{ -1, -1 },
		{  0, -1 },
		{  1, -1 },
	};

	if (dir[0] == '5') {
		xctrls[0].id = V4L2_CID_PANTILT_RESET;
		xctrls[0].value = angle;

		ctrls.count = 1;
		ctrls.controls = xctrls;
	} else {
		xctrls[0].id = V4L2_CID_PAN_RELATIVE;
		xctrls[0].value = directions[dir[0] - '1'][0] * angle;
		xctrls[1].id = V4L2_CID_TILT_RELATIVE;
		xctrls[1].value = directions[dir[0] - '1'][1] * angle;

		ctrls.count = 2;
		ctrls.controls = xctrls;
	}

	ioctl(dev, VIDIOC_S_EXT_CTRLS, &ctrls);
}
#endif

static int GetFreeRam(int* freeram)
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


static int get_vendor_verson(int dev, unsigned char szFlashVendorVer[12])
{
	return XU_SF_Read(dev, 0x154, szFlashVendorVer, 12);
}

static int video_open(const char *devname)
{
	struct v4l2_capability cap;
	int dev, ret;

	dev = open(devname, O_RDWR);
	if (dev < 0) {
		lidbg( "Error opening device %s: %d.\n", devname, errno);
		return dev;
	}

	memset(&cap, 0, sizeof cap);
	ret = ioctl(dev, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		lidbg( "Error opening device %s: unable to query device.\n",
			devname);
		close(dev);
		close(flycam_fd);
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

static void uvc_set_control(int dev, unsigned int id, int value)
{
	struct v4l2_control ctrl;
	int ret;

	ctrl.id = id;
	ctrl.value = value;

	ret = ioctl(dev, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		lidbg( "unable to set gain control: %s (%d).\n",
			strerror(errno), errno);
		return;
	}
}

static int video_set_format(int dev, unsigned int w, unsigned int h, unsigned int format)
{
	struct v4l2_format fmt;
	int ret;

	lidbg("*****Res_String => %s******\n",Res_String);
	//flyaudio
	if(!strncmp(Res_String, "1280x720", 8) || !strncmp(Res_String, "1280*720", 8) )
	{
		lidbg("%s: select 720P!\n",__func__);
		w = 1280;
		h = 720;		
		if(cam_id == REARVIEW_ID)
		{
			w = 640;
			h = 360;
		}		
	}
	else if(!strncmp(Res_String, "1920x1080", 9) || !strncmp(Res_String, "1920*1080", 9))
	{
		lidbg("%s: select 1080P!\n",__func__);
		w = 1920;
		h = 1080;
	}
	else if(!strncmp(Res_String, "640x360", 7) || !strncmp(Res_String, "640*360", 7) || !strncmp(Res_String, "480x272", 7) )
	{
		char tmpCMD[100] = {0};
		lidbg("%s: select 480x272!\n",__func__);
		w = 480;
		h = 272;
		isPreview = 1;
		sprintf(tmpCMD , "rm -f %s/tmp*.h264&",Rec_Save_Dir);
		system(tmpCMD);
	}
	property_set("fly.uvccam.curprevnum", "-1");
	
	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	lidbg("eho1--Video format set: width: %u height: %u buffer size: %u\n",
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);

	ret = ioctl(dev, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		lidbg( "Unable to set format: %d.", errno);
		return ret;
	}

	lidbg("eho2--Video format set: width: %u height: %u buffer size: %u\n",
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
	return 0;
}

static int video_set_still_format(int dev, unsigned int w, unsigned int h, unsigned int format)
{
	struct v4l2_format fmt;
	int ret;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	if(GetKernelVersion()> KERNEL_VERSION (3, 0, 36))
		ret = ioctl(dev, UVCIOC_STILL_S_FMT_KNL3, &fmt);
	else
		ret = ioctl(dev, UVCIOC_STILL_S_FMT_KNL2, &fmt);

	if (ret < 0) {
		lidbg( "Unable to set still format: %d.\n", errno);
		if(errno == EINVAL)
		lidbg( "still function doesn't support?\n", errno);	
		return ret;
	}

	TestAp_Printf(TESTAP_DBG_FLOW, "still format set: width: %u height: %u buffer size: %u\n",
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
	return 0;
}


static int video_set_framerate(int dev, int framerate, unsigned int *MaxPayloadTransferSize)
{
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		lidbg( "Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	TestAp_Printf(TESTAP_DBG_FLOW, "Current frame rate: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);

	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = framerate;

	ret = ioctl(dev, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		lidbg( "Unable to set frame rate: %d.\n", errno);
		return ret;
	}

    //yiling: get MaxPayloadTransferSize from sonix driver
    if(MaxPayloadTransferSize)
        *MaxPayloadTransferSize = parm.parm.capture.reserved[0];

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		lidbg( "Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	lidbg( "Frame rate set: %u/%u,MaxPayloadTransferSize:%d\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator,parm.parm.capture.reserved[0]);
	return 0;
}

int video_get_framerate(int dev, int *framerate)
{
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		lidbg( "Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	TestAp_Printf(TESTAP_DBG_FLOW, "Current frame rate: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);
    *framerate = parm.parm.capture.timeperframe.denominator;
    
	return 0;
}


static int video_reqbufs(int dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	int ret;

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		lidbg( "Unable to allocate buffers: %d.\n", errno);
		return ret;
	}

	TestAp_Printf(TESTAP_DBG_FLOW, "%u buffers allocated.\n", rb.count);
	return rb.count;
}

static int video_req_still_buf(int dev)
{
	struct v4l2_requestbuffers rb;
	int ret;

	memset(&rb, 0, sizeof rb);
	rb.count = 1;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	if(GetKernelVersion()> KERNEL_VERSION (3, 0, 36))
		ret = ioctl(dev, UVCIOC_STILL_REQBUF_KNL3, &rb);
	else
		ret = ioctl(dev, UVCIOC_STILL_REQBUF_KNL2, &rb);

	
	if (ret < 0) {
		lidbg( "Unable to allocate still buffers: %d.\n", errno);
		return ret;
	}

	TestAp_Printf(TESTAP_DBG_FLOW, "still buffers allocated.\n");
	return rb.count;
}

static int video_enable(int dev, int enable)
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

static void video_query_menu(int dev, unsigned int id)
{
	struct v4l2_querymenu menu;
	int ret;

	menu.index = 0;
	while (1) {
		menu.id = id;
		ret = ioctl(dev, VIDIOC_QUERYMENU, &menu);
		if (ret < 0)
			break;

		TestAp_Printf(TESTAP_DBG_FLOW, "  %u: %.32s\n", menu.index, menu.name);
		menu.index++;
	};
}

static void video_list_controls(int dev)
{
	struct v4l2_queryctrl query;
	struct v4l2_control ctrl;
	char value[12];
	int ret;

#ifndef V4L2_CTRL_FLAG_NEXT_CTRL
	unsigned int i;

	for (i = V4L2_CID_BASE; i <= V4L2_CID_LASTP1; ++i) {
		query.id = i;
#else
	query.id = 0;
	while (1) {
		query.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
#endif
		ret = ioctl(dev, VIDIOC_QUERYCTRL, &query);
		if (ret < 0)
			break;

		if (query.flags & V4L2_CTRL_FLAG_DISABLED)
			continue;

		ctrl.id = query.id;
		ret = ioctl(dev, VIDIOC_G_CTRL, &ctrl);
		if (ret < 0)
			strcpy(value, "n/a");
		else
			sprintf(value, "%d", ctrl.value);

		TestAp_Printf(TESTAP_DBG_FLOW, "control 0x%08x %s min %d max %d step %d default %d current %s.\n",
			query.id, query.name, query.minimum, query.maximum,
			query.step, query.default_value, value);

		if (query.type == V4L2_CTRL_TYPE_MENU)
			video_query_menu(dev, query.id);

	}
}

static void video_enum_inputs(int dev)
{
	struct v4l2_input input;
	unsigned int i;
	int ret;

	for (i = 0; ; ++i) {
		memset(&input, 0, sizeof input);
		input.index = i;
		ret = ioctl(dev, VIDIOC_ENUMINPUT, &input);
		if (ret < 0)
			break;

		if (i != input.index)
			TestAp_Printf(TESTAP_DBG_FLOW, "Warning: driver returned wrong input index "
				"%u.\n", input.index);

		TestAp_Printf(TESTAP_DBG_FLOW, "Input %u: %s.\n", i, input.name);
	}
}

static int video_get_input(int dev)
{
	__u32 input;
	int ret;

	ret = ioctl(dev, VIDIOC_G_INPUT, &input);
	if (ret < 0) {
		lidbg( "Unable to get current input: %s.\n", strerror(errno));
		return ret;
	}

	return input;
}

static int video_set_input(int dev, unsigned int input)
{
	__u32 _input = input;
	int ret;

	ret = ioctl(dev, VIDIOC_S_INPUT, &_input);
	if (ret < 0)
		lidbg( "Unable to select input %u: %s.\n", input,
			strerror(errno));

	return ret;
}

static void Enum_MaxPayloadTransSize(char *dev_name)
{
    
    struct CapabiltyBinaryData CapData;
    struct CapabilityDescriptor Cap_Desc;
    struct InterfaceDesc Interface[2], Interface_tmp;

    int i, j,k,l;
    int ret, dev_tmp, dev[2];
    char dev_name_tmp[20];
    unsigned int MaxPayloadTransferSize[2];
    
    Dbg_Param = 0x12;
    
    dev_tmp = video_open(dev_name);
    if (dev_tmp < 0)
        return;

    //get interface
    GetInterface(dev_tmp,&Interface_tmp);    
    strcpy(dev_name_tmp, dev_name);
    if(Interface_tmp.NumFormat == 2)
    {
        //dev_name is path of interface 1
        dev[0] = dev_tmp;
        memcpy(&Interface[0], &Interface_tmp, sizeof(struct InterfaceDesc));  
        //set  interface 2
		dev_name_tmp[10] += 1;	//	for string "/dev/videoX", if X>0, X++
		dev[1]= video_open(dev_name_tmp);
		if (dev_tmp < 0)
            return;
        GetInterface(dev[1],&Interface[1]);
    }    
    else
    {
        //dev_name is path of interface 2
        dev[1] = dev_tmp;
        memcpy(&Interface[1], &Interface_tmp, sizeof(struct InterfaceDesc));
        //set  interface 1
		dev_name_tmp[10] -= 1;	//	for string "/dev/videoX", if X>0, X--
		dev[0]= video_open(dev_name_tmp);
		if (dev[0] < 0)
            return;
        GetInterface(dev[0],&Interface[0]);		
    }
    
    
    //get capability
    GetCapability(dev[1], &CapData);
    ParseCapability(CapData.pbuf, CapData.Lenght, &Cap_Desc);


    //list bandwidth
    for(i = 0; i<Cap_Desc.NumConfigs ;i++)
    {
        struct InterfaceDesc *pInterface;
        struct MultiStreamCap *pCap_tmp, *pCap[2] = {NULL};
        int if_index, fmt_index, frame_index, fmt[2], width, height;
        struct FrameTypeDesc *pFrameDesc[2];

        //get Cap descriptor
        for(j = 0 ; j < Cap_Desc.Cfg_Desc[i].NumStreams ; j++)
        {
            pCap_tmp= &Cap_Desc.Cfg_Desc[i].MS_Cap[j];
            if(pCap[min(pCap_tmp->UVCInterfaceNum,2)-1]==NULL)
                pCap[min(pCap_tmp->UVCInterfaceNum,2)-1] = pCap_tmp;
        }

        //set format, width and height to device
        for(j = 0 ; j < 2 ; j++)
        {
            if(pCap[j]==NULL)
                continue;
            
            //get interface
            if_index = min(pCap[j]->UVCInterfaceNum,2)-1;
            pInterface = &Interface[if_index];

            //get format
            fmt_index = min(pCap[j]->UVCFormatIndex, pInterface->NumFormat) - 1;
            fmt[j] = pInterface->fmt[fmt_index];

            //get frame
            frame_index = min(pCap[j]->UVCFrameIndex, pInterface->NumFrame[fmt_index]) - 1;
            pFrameDesc[j] = &pInterface->frame_info[fmt_index][frame_index];
            width = pInterface->frame_info[fmt_index][frame_index].width;
            height = pInterface->frame_info[fmt_index][frame_index].height;
            video_set_format(dev[j], width, height, fmt[j]);
            
        }

        TestAp_Printf(TESTAP_DBG_BW, "config = %d\n", i+1);
        //set bitrate and get the MaxPayloadTransferSize
        if(pCap[0] && pCap[1]==NULL)
        {
            for(j = 0; j < pFrameDesc[0]->NumFPS; j++)
            {
                video_set_framerate(dev[0], pFrameDesc[0]->FPS[j], &MaxPayloadTransferSize[0]);
                TestAp_Printf(TESTAP_DBG_BW, "fmt = %c%c%c%c, size = %4dx%4d, fps = %2d",
                                    fmt[0]&0xff, (fmt[0]>>8)&0xff, (fmt[0]>>16)&0xff, (fmt[0]>>24)&0xff, 
                                    pFrameDesc[0]->width, pFrameDesc[0]->height, pFrameDesc[0]->FPS[j]);
                TestAp_Printf(TESTAP_DBG_BW, "\tMaxPayloadTransferSize = 0x%x\n", MaxPayloadTransferSize[0]); 

            } 
        }
        else if(pCap[0]==NULL && pCap[1])
        {
            for(j = 0; j < pFrameDesc[1]->NumFPS; j++)
            {
                video_set_framerate(dev[1], pFrameDesc[1]->FPS[j], &MaxPayloadTransferSize[1]);
                TestAp_Printf(TESTAP_DBG_BW, "fmt = %c%c%c%c, size = %4dx%4d, fps = %2d",
                                    fmt[1]&0xff, (fmt[1]>>8)&0xff, (fmt[1]>>16)&0xff, (fmt[1]>>24)&0xff, 
                                    pFrameDesc[1]->width, pFrameDesc[1]->height, pFrameDesc[1]->FPS[j]);
                TestAp_Printf(TESTAP_DBG_BW, "\tMaxPayloadTransferSize = 0x%x\n", MaxPayloadTransferSize[1]); 

            } 
        }                
        else if(pCap[0] && pCap[1])
        {
            for(j = 0; j < pFrameDesc[0]->NumFPS; j++)
            {
                for(k = 0; k < pFrameDesc[1]->NumFPS; k++)
                {
                    video_set_framerate(dev[0], pFrameDesc[0]->FPS[j], &MaxPayloadTransferSize[0]);
                    video_set_framerate(dev[1], pFrameDesc[1]->FPS[k], &MaxPayloadTransferSize[1]);
                    TestAp_Printf(TESTAP_DBG_BW, "(fmt = %c%c%c%c, size = %4dx%4d, fps = %2d), (fmt = %c%c%c%c, size = %4dx%4d, fps = %2d)",
                                    fmt[0]&0xff, (fmt[0]>>8)&0xff, (fmt[0]>>16)&0xff, (fmt[0]>>24)&0xff, 
                                    pFrameDesc[0]->width, pFrameDesc[0]->height, pFrameDesc[0]->FPS[j],
                                    fmt[1]&0xff, (fmt[1]>>8)&0xff, (fmt[1]>>16)&0xff, (fmt[1]>>24)&0xff, 
                                    pFrameDesc[1]->width, pFrameDesc[1]->height, pFrameDesc[1]->FPS[k]);
                    TestAp_Printf(TESTAP_DBG_BW, "\tMaxPayloadTransferSize = 0x%x(%x+%x)\n"
                                        , MaxPayloadTransferSize[0]+MaxPayloadTransferSize[1]
                                        , MaxPayloadTransferSize[0],MaxPayloadTransferSize[1]);
                       
                }
            }
                
        }

    }





#if 0   //dbg
    for(l = 0; l < 2; l ++)
    {
        TestAp_Printf(TESTAP_DBG_BW, "INTERFACE[%d]:\n", l+1);
        TestAp_Printf(TESTAP_DBG_BW, "format num = %d\n", Interface[l].NumFormat);
        for(i = 0; i < Interface[l].NumFormat;i++)
        {
            TestAp_Printf(TESTAP_DBG_BW, "\tformat[%d] = %x\n", i, Interface[l].fmt[i]);
            for(j = 0; j < Interface[l].NumFrame[i];j++)
            {
                TestAp_Printf(TESTAP_DBG_BW, "\t\tframe[%d]: size = %d x %d \n", j, Interface[l].frame_info[i][j].width, Interface[l].frame_info[i][j].height);
                for(k = 0; k<Interface[l].frame_info[i][j].NumFPS; k++)
                    TestAp_Printf(TESTAP_DBG_BW, "\t\t\tfps[%d]: fps = %d\n", k,  Interface[l].frame_info[i][j].FPS[k]);

            }

        }
    }
    
#endif


#if 0   //dbg   
    int index_i, index_j;

    TestAp_Printf(TESTAP_DBG_BW, "num of cfg = %d\n", Cap_Desc.NumConfigs);
    for(index_i = 0; index_i < Cap_Desc.NumConfigs ;index_i++)
    {
        TestAp_Printf(TESTAP_DBG_BW, "\tnum of string = %d\n", Cap_Desc.Cfg_Desc[index_i].NumStreams);    
        for(index_j = 0; index_j < Cap_Desc.Cfg_Desc[index_i].NumStreams;index_j++)
        {
            TestAp_Printf(TESTAP_DBG_BW, "\t\t[%d] %d %d %d %d %d %d %d %d %d %d %d %d \n", index_j, Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].UVCInterfaceNum, 
                    Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].UVCFormatIndex , Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].UVCFrameIndex , Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].DemuxerIndex, 
                    Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].FPSIndex, Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].BRCIndex, Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].OSDIndex, 
                    Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].MDIndex, Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].PTZIIndex, Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].FPSGroup, 
                    Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].BRCGroup, Cap_Desc.Cfg_Desc[index_i].MS_Cap[index_j].OSDGroup);  
            
        }
    }
    TestAp_Printf(TESTAP_DBG_BW, "num of demuxer = %d\n", Cap_Desc.NumDemuxers);
    for(index_i = 0; index_i < Cap_Desc.NumDemuxers;index_i++)
    {
        TestAp_Printf(TESTAP_DBG_BW, "\t[%d] %d %d %d %d\n", index_i, Cap_Desc.demuxer_Desc[index_i].MSCDemuxIndex,
            Cap_Desc.demuxer_Desc[index_i].DemuxID, Cap_Desc.demuxer_Desc[index_i].Width,
            Cap_Desc.demuxer_Desc[index_i].Height);
    }

    TestAp_Printf(TESTAP_DBG_BW, "num of frameinterval = %d\n", Cap_Desc.NumFrameIntervals);
    for(index_i = 0; index_i < Cap_Desc.NumFrameIntervals;index_i++)
    {
        TestAp_Printf(TESTAP_DBG_BW, "\t[%d] %d %d\n", index_i, Cap_Desc.FrameInt_Desc[index_i].FPSIndex,
            Cap_Desc.FrameInt_Desc[index_i].FPSCount);
        for(index_j = 0; index_j < Cap_Desc.FrameInt_Desc[index_i].FPSCount;index_j++)
            TestAp_Printf(TESTAP_DBG_BW, "\t\t[%d] %d\n", index_j, Cap_Desc.FrameInt_Desc[index_i].FPS[index_j]);
            
    }    
    
    TestAp_Printf(TESTAP_DBG_BW, "num of bitrate = %d\n", Cap_Desc.NumBitrate);
    for(index_i = 0; index_i < Cap_Desc.NumBitrate;index_i++)
    {
        TestAp_Printf(TESTAP_DBG_BW, "\t[%d] %d %d\n", index_i, Cap_Desc.Bitrate_Desc[index_i].BRCIndex,
             Cap_Desc.Bitrate_Desc[index_i].BRCMode);
    }
    
#endif
    close(dev[0]);
    close(dev[1]);
    Dbg_Param = 0x1f;    

}


int get_still_image(int dev, unsigned int w, unsigned int h, unsigned int format)
{
	char filename[30];
	struct v4l2_buffer buf; 
	FILE *file = NULL;
	int ret = 0, repeat_setting = 0;
	void *mem;
	static int counter = 0;
	static int still_dev = 0, still_width = 0, still_height = 0, still_format = 0;

	TestAp_Printf(TESTAP_DBG_FLOW, "%s ============>\n",__FUNCTION__);
	if(still_dev == dev && still_width == w && still_height == h && still_format == format)
		repeat_setting = 1;
	

	//set file name
	if(format == V4L2_PIX_FMT_MJPEG)
		sprintf(filename, "still_img-%02u.jpg", counter);
	else if(format == V4L2_PIX_FMT_YUYV)
		sprintf(filename, "still_img-%02u.yuyv", counter);

	TestAp_Printf(TESTAP_DBG_FLOW, "repeat_setting = %d, fname = %s\n",repeat_setting, filename);
		
	if(!repeat_setting)
	{		
		//set format
		video_set_still_format(dev, w, h, format);

		//request still buffer
		ret = video_req_still_buf(dev);
		if (ret < 0) {
			lidbg( "Unable to request still buffer(%d).\n", errno);			
			return ret;
		}
	}
	//mmap
	memset(&buf, 0, sizeof buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if(GetKernelVersion()> KERNEL_VERSION (3, 0, 36))
		ret = ioctl(dev, UVCIOC_STILL_QUERYBUF_KNL3, &buf);
	else
		ret = ioctl(dev, UVCIOC_STILL_QUERYBUF_KNL2, &buf);
	
	if (ret < 0) {
		lidbg( "Unable to query still buffer(%d).\n", errno);			
		return ret;
	}
	TestAp_Printf(TESTAP_DBG_FLOW, "length: %u offset: %10u     --  ", buf.length, buf.m.offset);


	mem = mmap(0, buf.length, PROT_READ, MAP_SHARED, dev, buf.m.offset);

	if (mem == MAP_FAILED) {
		lidbg( "Unable to map still buffer(%d)\n", errno);		
		return -1;
	}
	TestAp_Printf(TESTAP_DBG_FLOW, "still Buffer mapped at address %p.\n", mem);

	//get data
	if(GetKernelVersion()> KERNEL_VERSION (3, 0, 36))
		ret = ioctl(dev, UVCIOC_STILL_GET_FRAME_KNL3, &buf);
	else
		ret = ioctl(dev, UVCIOC_STILL_GET_FRAME_KNL2, &buf);	
	if (ret < 0) {
		lidbg( "Unable to get still image(%d).\n", errno);			
		return ret;
	}
	TestAp_Printf(TESTAP_DBG_FLOW, "buf.bytesused = %d\n", buf.bytesused);
	
	file = fopen(filename, "wb");
	if (file != NULL) 
		fwrite(mem, buf.bytesused, 1, file);

	fclose(file);
	munmap(mem, buf.length);

	counter ++;
	TestAp_Printf(TESTAP_DBG_FLOW, "%s <============\n",__FUNCTION__);
	still_dev = dev; 
	still_width = w;
	still_height = h;
	still_format = format;
	return 0;

}


static void usage(const char *argv0)
{
	TestAp_Printf(TESTAP_DBG_USAGE, "Usage: %s [options] device\n", argv0);
	TestAp_Printf(TESTAP_DBG_USAGE, "Supported options:\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-c, --capture[=nframes]	Capture frames\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-d, --delay		Delay (in ms) before requeuing buffers\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-e,                     enum MaxPayloadTransferSize\n");    
	TestAp_Printf(TESTAP_DBG_USAGE, "-f, --format format	Set the video format (mjpg or yuyv)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-h, --help		Show this help screen\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-i, --input input	Select the video input\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-l, --list-controls	List available controls\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-n, --nbufs n		Set the number of video buffers\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-s, --size WxH		Set the frame size\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "    --fr framerate	Set framerate\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-S, --save		Save captured images to disk\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "    --enum-inputs	Enumerate inputs\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "    --skip n		Skip the first n frames\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-r, --record		Record H264 file\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--still, 			get still image\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--bri-set values	Set brightness values\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--bri-get		Get brightness values\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--shrp-set values	Set sharpness values\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--shrp-get		Get sharpness values\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--dbg value		Set level of debug message(bit0:usage, bit1:error, bit2:flow, bit3:frame)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "--vnd-get		Get vender version\n"); 
	TestAp_Printf(TESTAP_DBG_USAGE, "SONiX XU supported options:\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "-a,  --add-xuctrl			Add Extension Unit Ctrl into Driver\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget id cs datasize d0 d1 ...	XU Get command: xu_id control_selector data_size data_0 data_1 ...\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset id cs datasize d0 d1 ...	XU Set command: xu_id control_selector data_size data_0 data_1 ...\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-chip			Read SONiX Chip ID\n");
#if(0)
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-fmt			List H.264 format\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-fmt fmt-fps		Set H.264 format - fps index\n");
#endif
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-qp				Get H.264 QP values\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-qp val			Set H.264 QP values: val\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-br				Get H.264 bit rate (bps)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-br val			Set H.264 bit rate (bps) \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --asic-r	addr			[Hex] Read register address data\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --asic-w	addr data		[Hex] Write register address data\n");
#if(CARCAM_PROJECT == 0)
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mf val			Set Multi-Stream format:[1]HD+QVGA [2]HD+180p [4]HD+360p [8]HD+VGA [10]HD+QVGA+VGA [20]HD+QVGA [40]HD+180p+360p [80]360p+180p\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mgs				Get Multi-Stream Status. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mgi				Get Multi-Stream Info. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --msqp StreamID QP                 Set Multi-Stream QP. StreamID = 0 ~ 2 \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mgqp StreamID	                Get Multi-Stream QP. StreamID = 0 ~ 2 \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --msbr StreamID Bitrate         	Set Multi-Stream Bitrate (bps). StreamID = 0 ~ 2 \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mgbr StreamID			Get Multi-Stream BitRate (bps). StreamID = 0 ~ 2 \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mscvm StreamID H264Mode          Set Multi-Stream H264 Mode. StreamID = 0 ~ 2(1:CBR 2:VBR) \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mgcvm StreamID	                Get Multi-Stream H264 Mode. StreamID = 0 ~ 2 \n");    
	TestAp_Printf(TESTAP_DBG_USAGE, "     --msfr val                         Set Multi-Stream substream frame rate.\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mgfr                             Get Multi-Stream substream frame rate.\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --msgop val                        Set Multi-Stream substream GOP(suggest GOP = fps-1).\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mggop                            Get Multi-Stream substream GOP.\n");    
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mse Enable		        Set Multi-Stream Enable : [0]Disable [1]H264  [3]H264+Mjpg. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --mge				Get Multi-Stream Enable. \n");
#endif
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-timer Enable		Set OSD Timer Counting  1:enable   0:disable\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-rtc year month day hour min sec	Set OSD RTC\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-rtc 			Get OSD RTC\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-os Line Block 		Set OSD Line and Block Size (0~4)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-os				Get OSD Line and Block Size (0~4)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-oc Font Border            	Set OSD Font and Border Color   0:Black  1:Red  2:Green  3:Blue  4:White\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-oc				Get OSD Font and Border Color   0:Black  1:Red  2:Green  3:Blue  4:White\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-oe Line Block		Set OSD Show  1:enable  0:disable\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-oe				Get OSD Show  1:enable  0:disable\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-oas Line Block		Set OSD Auto Scale  1:enable  0:disable\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-oas			Get OSD Auto Scale  1:enable  0:disable\n");
#if(CARCAM_PROJECT == 0)	
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-oms Stream0 Stream1 Stream2	Set OSD MultiStream Size  (0~4)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-oms			Get OSD MultiStream Size  (0~4)\n");
#endif	
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-osp Type Row Col		Set OSD Start Row and Col (unit:16)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-osp			Get OSD Start Row and Col (unit:16)\n");
#if(CARCAM_PROJECT == 0)
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-ostr Group '.....'		Set OSD 2nd String.Group from 0 to 2.8 words per 1 Group.\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-ostr Group 		Get OSD 2nd String. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-omssp StreamID Row Col	Set OSD Multi stream start row and col. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-omssp			Get OSD Multi stream start raw and col. \n");
#endif
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-mde Enable			Set Motion detect enable\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-mde			Get Motion detect enable\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-mdt Thd			Set Motion detect threshold (0~65535)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-mdt			Get Motion detect threshold\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-mdm  m1 m2 ... m24		Set Motion detect mask\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-mdm			Get Motion detect mask\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-mdr  m1 m2 ... m24		Set Motion detect result\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-mdr			Get Motion detect result\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-mjb Bitrate		Set MJPG Bitrate (bps) \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-mjb			Get MJPG Bitrate (bps) \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-if	nframe			Set H264 reset to IFrame.  nframe : reset per nframe.\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-sei			Set H264 SEI Header Enable.\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-sei			Get H264 SEI Header Enable. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-gop			Set H264 GOP. (1 ~ 4095)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-gop			Get H264 GOP. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-cvm			Set H264 CBR/VBR mode(1:CBR 2:VBR)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-cvm			Get H264 CBR/VBR mode(1:CBR 2:VBR)\n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-mir			Set Image mirror. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-mir			Get Image mirror. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-flip			Set Image flip. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-flip			Get Image flip. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-gpio enable out_value      Set GPIO ctrl(hex). \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-gpio                       Get GPIO ctrl. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-clr			Set Image color. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-clr			Get Image color. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-fde s1 s2			Set Frame drop enable. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-fde			Get Frame drop enable. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-fdc s1 s2			Set Frame drop value. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-fdc			Get Frame drop value. \n");
	
#if(CARCAM_PROJECT == 1)
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-car SpeedEn CoordinateEn CoordinateCtrl		Set Car control . \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-car			Get Car control. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-spd Speed			Set Speed info. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-spd			Get Speed info. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-coor1 Dir v1 v2 v3 v4 v5 v6	Set Coordinate info. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-coor1			Get Coordinate info. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuset-coor2 Dir v1 v2 v3 v4	Set Coordinate info. \n");
	TestAp_Printf(TESTAP_DBG_USAGE, "     --xuget-coor2			Get Coordinate info. \n");
#endif
}

#define OPT_ENUM_INPUTS			256
#define OPT_SKIP_FRAMES			OPT_ENUM_INPUTS + 1
#define OPT_XU_ADD_CTRL			OPT_ENUM_INPUTS + 2
#define OPT_XU_GET_CHIPID		OPT_ENUM_INPUTS + 3
#define OPT_XU_GET_FMT 			OPT_ENUM_INPUTS + 4
#define OPT_XU_SET_FMT 			OPT_ENUM_INPUTS + 5
#define OPT_XU_SET_FPS 			OPT_ENUM_INPUTS + 6
#define OPT_XU_GET_QP			OPT_ENUM_INPUTS + 7
#define OPT_XU_SET_QP			OPT_ENUM_INPUTS + 8
#define OPT_XU_GET_BITRATE		OPT_ENUM_INPUTS + 9
#define OPT_XU_SET_BITRATE		OPT_ENUM_INPUTS + 10
#define OPT_XU_GET				OPT_ENUM_INPUTS + 11
#define OPT_XU_SET				OPT_ENUM_INPUTS + 12
#define OPT_BRIGHTNESS_GET		OPT_ENUM_INPUTS + 13
#define OPT_BRIGHTNESS_SET		OPT_ENUM_INPUTS + 14
#define OPT_SHARPNESS_GET		OPT_ENUM_INPUTS + 15
#define OPT_SHARPNESS_SET		OPT_ENUM_INPUTS + 16
#define OPT_ASIC_READ			OPT_ENUM_INPUTS + 17
#define OPT_ASIC_WRITE			OPT_ENUM_INPUTS + 18
#define OPT_MULTI_FORMAT		OPT_ENUM_INPUTS + 19
#define OPT_MULTI_SET_BITRATE	OPT_ENUM_INPUTS + 20
#define OPT_MULTI_GET_BITRATE	OPT_ENUM_INPUTS + 21
#define OPT_OSD_TIMER_CTRL_SET	OPT_ENUM_INPUTS + 22
#define OPT_OSD_RTC_SET			OPT_ENUM_INPUTS + 23
#define OPT_OSD_RTC_GET			OPT_ENUM_INPUTS + 24
#define OPT_OSD_SIZE_SET		OPT_ENUM_INPUTS + 25
#define OPT_OSD_SIZE_GET		OPT_ENUM_INPUTS + 26
#define OPT_OSD_COLOR_SET		OPT_ENUM_INPUTS + 27
#define OPT_OSD_COLOR_GET		OPT_ENUM_INPUTS + 28
#define OPT_OSD_SHOW_SET		OPT_ENUM_INPUTS + 29
#define OPT_OSD_SHOW_GET		OPT_ENUM_INPUTS + 30
#define OPT_OSD_AUTOSCALE_SET	OPT_ENUM_INPUTS + 31
#define OPT_OSD_AUTOSCALE_GET	OPT_ENUM_INPUTS + 32
#define OPT_OSD_MS_SIZE_SET		OPT_ENUM_INPUTS + 33
#define OPT_OSD_MS_SIZE_GET		OPT_ENUM_INPUTS + 34
#define OPT_OSD_POSITION_SET	OPT_ENUM_INPUTS + 35
#define OPT_OSD_POSITION_GET	OPT_ENUM_INPUTS + 36
#define OPT_MD_MODE_SET			OPT_ENUM_INPUTS + 37
#define OPT_MD_MODE_GET			OPT_ENUM_INPUTS + 38
#define OPT_MD_THRESHOLD_SET	OPT_ENUM_INPUTS + 39
#define OPT_MD_THRESHOLD_GET	OPT_ENUM_INPUTS + 40
#define OPT_MD_MASK_SET			OPT_ENUM_INPUTS + 41
#define OPT_MD_MASK_GET			OPT_ENUM_INPUTS + 42
#define OPT_MD_RESULT_SET		OPT_ENUM_INPUTS + 43
#define OPT_MD_RESULT_GET		OPT_ENUM_INPUTS + 44
#define OPT_MJPG_BITRATE_SET	OPT_ENUM_INPUTS + 45
#define OPT_MJPG_BITRATE_GET	OPT_ENUM_INPUTS + 46
#define OPT_H264_IFRAME_SET		OPT_ENUM_INPUTS + 47
#define OPT_H264_SEI_SET		OPT_ENUM_INPUTS + 48
#define OPT_H264_SEI_GET		OPT_ENUM_INPUTS + 49
#define OPT_IMG_MIRROR_SET		OPT_ENUM_INPUTS + 50
#define OPT_IMG_MIRROR_GET		OPT_ENUM_INPUTS + 51
#define OPT_IMG_FLIP_SET		OPT_ENUM_INPUTS + 52
#define OPT_IMG_FLIP_GET		OPT_ENUM_INPUTS + 53
#define OPT_IMG_COLOR_SET		OPT_ENUM_INPUTS + 54
#define OPT_IMG_COLOR_GET		OPT_ENUM_INPUTS + 55
#define OPT_OSD_STRING_SET		OPT_ENUM_INPUTS + 56
#define OPT_OSD_STRING_GET		OPT_ENUM_INPUTS + 57
#define OPT_MULTI_GET_STATUS	OPT_ENUM_INPUTS + 58
#define OPT_MULTI_GET_INFO		OPT_ENUM_INPUTS + 59
#define OPT_MULTI_SET_ENABLE	OPT_ENUM_INPUTS + 60
#define OPT_MULTI_GET_ENABLE	OPT_ENUM_INPUTS + 61
#define OPT_MULTI_SET_QP		OPT_ENUM_INPUTS + 62
#define OPT_MULTI_GET_QP		OPT_ENUM_INPUTS + 63
#define OPT_MULTI_SET_H264MODE	OPT_ENUM_INPUTS + 64
#define OPT_MULTI_GET_H264MODE	OPT_ENUM_INPUTS + 65
#define OPT_MULTI_SET_SUB_FR	OPT_ENUM_INPUTS + 66
#define OPT_MULTI_GET_SUB_FR	OPT_ENUM_INPUTS + 67
#define OPT_MULTI_SET_SUB_GOP	OPT_ENUM_INPUTS + 68
#define OPT_MULTI_GET_SUB_GOP	OPT_ENUM_INPUTS + 69
#define OPT_H264_GOP_SET		OPT_ENUM_INPUTS + 70
#define OPT_H264_GOP_GET		OPT_ENUM_INPUTS + 71
#define OPT_H264_MODE_SET		OPT_ENUM_INPUTS + 72
#define OPT_H264_MODE_GET		OPT_ENUM_INPUTS + 73
#define OPT_OSD_MS_POSITION_SET	OPT_ENUM_INPUTS + 74
#define OPT_OSD_MS_POSITION_GET	OPT_ENUM_INPUTS + 75
#define OPT_OSD_CARCAM_SET		OPT_ENUM_INPUTS + 76
#define OPT_OSD_CARCAM_GET		OPT_ENUM_INPUTS + 77
#define OPT_OSD_SPEED_SET		OPT_ENUM_INPUTS + 78
#define OPT_OSD_SPEED_GET		OPT_ENUM_INPUTS + 79
#define OPT_OSD_COORDINATE_SET1	OPT_ENUM_INPUTS + 80
#define OPT_OSD_COORDINATE_SET2	OPT_ENUM_INPUTS + 81
#define OPT_OSD_COORDINATE_GET1	OPT_ENUM_INPUTS + 82
#define OPT_OSD_COORDINATE_GET2	OPT_ENUM_INPUTS + 83
#define OPT_GPIO_CTRL_SET   	OPT_ENUM_INPUTS + 84
#define OPT_GPIO_CTRL_GET   	OPT_ENUM_INPUTS + 85
#define OPT_FRAMERATE			OPT_ENUM_INPUTS + 86
#define OPT_FRAME_DROP_EN_SET	OPT_ENUM_INPUTS + 87
#define OPT_FRAME_DROP_EN_GET	OPT_ENUM_INPUTS + 88
#define OPT_FRAME_DROP_CTRL_SET	OPT_ENUM_INPUTS + 89
#define OPT_FRAME_DROP_CTRL_GET	OPT_ENUM_INPUTS + 90
#define OPT_DEBUG_LEVEL			OPT_ENUM_INPUTS + 91
#define OPT_STILL_IMAGE			OPT_ENUM_INPUTS + 92
#define OPT_VENDOR_VERSION_GET	OPT_ENUM_INPUTS + 93
#define OPT_EFFECT_SET	OPT_ENUM_INPUTS + 94


static struct option opts[] = {
	{"capture", 2, 0, 'c'},
	{"delay", 1, 0, 'd'},
	{"enum-inputs", 0, 0, OPT_ENUM_INPUTS},
	{"format", 1, 0, 'f'},
	{"help", 0, 0, 'h'},
	{"input", 1, 0, 'i'},
	{"list-controls", 0, 0, 'l'},
	{"save", 0, 0, 'S'},
	{"still", 0, 0, OPT_STILL_IMAGE},
	{"size", 1, 0, 's'},
	{"fr", 1, 0, OPT_FRAMERATE},
	{"skip", 1, 0, OPT_SKIP_FRAMES},
	{"record", 0, 0, 'r'},
	{"bri-get", 0, 0, OPT_BRIGHTNESS_GET},
	{"bri-set", 1, 0, OPT_BRIGHTNESS_SET},
	{"shrp-get", 0, 0, OPT_SHARPNESS_GET},
	{"shrp-set", 1, 0, OPT_SHARPNESS_SET},
	{"add-xuctrl", 0, 0, OPT_XU_ADD_CTRL},
	{"xuget", 1, 0, OPT_XU_GET},
	{"xuset", 1, 0, OPT_XU_SET},
	{"xuget-chip", 0, 0, OPT_XU_GET_CHIPID},
	{"xuget-fmt", 0, 0, OPT_XU_GET_FMT},
	{"xuset-fmt", 1, 0, OPT_XU_SET_FMT},
	{"xuget-qp", 0, 0, OPT_XU_GET_QP},
	{"xuset-qp", 1, 0, OPT_XU_SET_QP},
	{"xuget-br", 0, 0, OPT_XU_GET_BITRATE},
	{"xuset-br", 1, 0, OPT_XU_SET_BITRATE},
	{"asic-r", 1, 0, OPT_ASIC_READ},
	{"asic-w", 1, 0, OPT_ASIC_WRITE},
	{"msqp", 1, 0, OPT_MULTI_SET_QP},
	{"mgqp", 1, 0, OPT_MULTI_GET_QP},
	{"mscvm", 1, 0, OPT_MULTI_SET_H264MODE},
	{"mgcvm", 1, 0, OPT_MULTI_GET_H264MODE},
	{"msfr", 1, 0, OPT_MULTI_SET_SUB_FR},
	{"mgfr", 0, 0, OPT_MULTI_GET_SUB_FR},
	{"msgop", 1, 0, OPT_MULTI_SET_SUB_GOP},
	{"mggop", 0, 0, OPT_MULTI_GET_SUB_GOP},
	{"mf", 1, 0, OPT_MULTI_FORMAT},
	{"msbr", 1, 0, OPT_MULTI_SET_BITRATE},
	{"mgbr", 1, 0, OPT_MULTI_GET_BITRATE},
	{"mgs", 0, 0, OPT_MULTI_GET_STATUS},
	{"mgi", 0, 0, OPT_MULTI_GET_INFO},
	{"mse", 1, 0, OPT_MULTI_SET_ENABLE},
	{"mge", 0, 0, OPT_MULTI_GET_ENABLE},
	{"xuset-timer", 1, 0, OPT_OSD_TIMER_CTRL_SET},
	{"xuset-rtc", 1, 0, OPT_OSD_RTC_SET},
	{"xuget-rtc", 0, 0, OPT_OSD_RTC_GET},
	{"xuset-os", 1, 0, OPT_OSD_SIZE_SET},
	{"xuget-os", 0, 0, OPT_OSD_SIZE_GET},
	{"xuset-oc", 1, 0, OPT_OSD_COLOR_SET},
	{"xuget-oc", 0, 0, OPT_OSD_COLOR_GET},
	{"xuset-oe", 1, 0, OPT_OSD_SHOW_SET},
	{"xuget-oe", 0, 0, OPT_OSD_SHOW_GET},
	{"xuset-oas", 1, 0, OPT_OSD_AUTOSCALE_SET},
	{"xuget-oas", 0, 0, OPT_OSD_AUTOSCALE_GET},
	{"xuset-oms", 1, 0, OPT_OSD_MS_SIZE_SET},
	{"xuget-oms", 0, 0, OPT_OSD_MS_SIZE_GET},
	{"xuset-ostr", 1, 0, OPT_OSD_STRING_SET},
	{"xuget-ostr", 1, 0, OPT_OSD_STRING_GET},
	{"xuset-osp", 1, 0, OPT_OSD_POSITION_SET},
	{"xuget-osp", 0, 0, OPT_OSD_POSITION_GET},
	{"xuset-omssp", 1, 0, OPT_OSD_MS_POSITION_SET},
	{"xuget-omssp", 0, 0, OPT_OSD_MS_POSITION_GET},
	{"xuset-mde", 1, 0, OPT_MD_MODE_SET},
	{"xuget-mde", 0, 0, OPT_MD_MODE_GET},
	{"xuset-mdt", 1, 0, OPT_MD_THRESHOLD_SET},
	{"xuget-mdt", 0, 0, OPT_MD_THRESHOLD_GET},
	{"xuset-mdm", 1, 0, OPT_MD_MASK_SET},
	{"xuget-mdm", 0, 0, OPT_MD_MASK_GET},
	{"xuset-mdr", 1, 0, OPT_MD_RESULT_SET},
	{"xuget-mdr", 0, 0, OPT_MD_RESULT_GET},
	{"xuset-mjb", 1, 0, OPT_MJPG_BITRATE_SET},
	{"xuget-mjb", 0, 0, OPT_MJPG_BITRATE_GET},
	{"xuset-if", 1, 0, OPT_H264_IFRAME_SET},
	{"xuset-sei", 1, 0, OPT_H264_SEI_SET},
	{"xuget-sei", 0, 0, OPT_H264_SEI_GET},
	{"xuset-gop", 1, 0, OPT_H264_GOP_SET},
	{"xuget-gop", 0, 0, OPT_H264_GOP_GET},
	{"xuset-cvm", 1, 0, OPT_H264_MODE_SET},
	{"xuget-cvm", 0, 0, OPT_H264_MODE_GET},
	{"xuset-mir", 1, 0, OPT_IMG_MIRROR_SET},
	{"xuget-mir", 0, 0, OPT_IMG_MIRROR_GET},
	{"xuset-flip", 1, 0, OPT_IMG_FLIP_SET},
	{"xuget-flip", 0, 0, OPT_IMG_FLIP_GET},
	{"xuset-clr", 1, 0, OPT_IMG_COLOR_SET},
	{"xuget-clr", 0, 0, OPT_IMG_COLOR_GET},
	{"xuset-car", 1, 0, OPT_OSD_CARCAM_SET},
	{"xuget-car", 0, 0, OPT_OSD_CARCAM_GET},
	{"xuset-spd", 1, 0, OPT_OSD_SPEED_SET},
	{"xuget-spd", 0, 0, OPT_OSD_SPEED_GET},
	{"xuset-coor1", 1, 0, OPT_OSD_COORDINATE_SET1},
	{"xuset-coor2", 1, 0, OPT_OSD_COORDINATE_SET2},
	{"xuget-coor1", 0, 0, OPT_OSD_COORDINATE_GET1},
	{"xuget-coor2", 0, 0, OPT_OSD_COORDINATE_GET2},
	{"xuset-gpio", 1, 0, OPT_GPIO_CTRL_SET},
	{"xuget-gpio", 0, 0, OPT_GPIO_CTRL_GET},
	{"xuset-fde", 1, 0, OPT_FRAME_DROP_EN_SET},
	{"xuget-fde", 0, 0, OPT_FRAME_DROP_EN_GET},
	{"xuset-fdc", 1, 0, OPT_FRAME_DROP_CTRL_SET},
	{"xuget-fdc", 0, 0, OPT_FRAME_DROP_CTRL_GET},
	{"dbg", 1, 0, OPT_DEBUG_LEVEL},
	{"vnd-get", 0, 0, OPT_VENDOR_VERSION_GET},
	{"ef-set", 1, 0, OPT_EFFECT_SET},
	{0, 0, 0, 0}
};

char dvr_blackbox_filename[200] = {0};
char rear_blackbox_filename[200] = {0};
char dvr_blackbox_dest_filename[200] = {0};
char rear_blackbox_dest_filename[200] = {0};
char deq_time_buf[100] = {0};

void dequeue_buf(int count , FILE * rec_fp)
{
	FILE *fp1 = NULL;
	FILE *fp2 = NULL;
	int isBeginTopDeq = 0;
	if(isBlackBoxBottomRec) lidbg("***BlackBoxBottomCnt:%d****\n",BlackBoxBottomCnt);
#if 1
	while(1)
	{
		property_get("lidbg.uvccam.isdequeue", Wait_Deq_Str, "0");
		if(strncmp(Wait_Deq_Str, "1", 1)) break;
		usleep(100*1000);
	}
#endif
	//system("setprop lidbg.uvccam.isdequeue 1");
	property_set("lidbg.uvccam.isdequeue", "1");
	
	ALOGE("=====dequeue_buf===count => %d==\n",count);
	if(isBlackBoxTopRec)
	{
		if(cam_id == DVR_ID)
		{
			if(isConvertMP4)
			{
				sprintf(dvr_blackbox_filename, "%s/F%s.h264", Em_Save_Tmp_Dir, deq_time_buf);
				sprintf(dvr_blackbox_dest_filename, "%s/F%s.mp4", Em_Save_Dir, deq_time_buf);
			}
			else
			{
				sprintf(dvr_blackbox_filename, "%s/EF%s.h264", Rec_Save_Dir, deq_time_buf);
			}
			lidbg("=========[%d]:BlackBoxTopRec : %s===========\n", cam_id,dvr_blackbox_filename);
			fp1 = fopen(dvr_blackbox_filename, "ab+");
			//fwrite(iFrameData, iframe_length , 1, fp1);
		}
		else if(cam_id == REARVIEW_ID)
		{
			if(isConvertMP4)
			{
				sprintf(rear_blackbox_filename, "%s/R%s.h264", Em_Save_Tmp_Dir, deq_time_buf);
				sprintf(rear_blackbox_dest_filename, "%s/R%s.mp4", Em_Save_Dir, deq_time_buf);
			}
			else
			{
				sprintf(rear_blackbox_filename, "%s/ER%s.h264", Rec_Save_Dir, deq_time_buf);
			}
			lidbg("=========[%d]:BlackBoxTopRec : %s===========\n", cam_id,rear_blackbox_filename);
			fp1 = fopen(rear_blackbox_filename, "ab+");
			//fwrite(iFrameData, iframe_length , 1, fp1);
		}
		//if(rec_fp != NULL) fclose(rec_fp);
	}
	else if(isBlackBoxBottomRec)
	{
#if 0
		lidbg_get_current_time(0 , time_buf, NULL);
		if(cam_id == DVR_ID)
			sprintf(flyh264_filename, "%s/BlackBox/F%s.h264", Rec_Save_Dir, time_buf);
		else if(cam_id == REARVIEW_ID)
			sprintf(flyh264_filename, "%s/BlackBox/R%s.h264", Rec_Save_Dir, time_buf);
#endif
		if(cam_id == DVR_ID)
		{
			lidbg("=========[%d]:BlackBoxBottomRec : %s===========\n", cam_id,dvr_blackbox_filename);
			fp2 = fopen(dvr_blackbox_filename, "ab+");
		}
		else if(cam_id == REARVIEW_ID)
		{
			lidbg("=========[%d]:BlackBoxBottomRec : %s===========\n", cam_id,rear_blackbox_filename);
			fp2 = fopen(rear_blackbox_filename, "ab+");
		}
		//if(rec_fp != NULL) fclose(rec_fp);
	}
	//else lidbg_get_current_time(0 , deq_time_buf, NULL);
	while((count --) > 0)
	{
		void* tempa;
		int lengtha;
		//tempa = malloc(220000);  
		lengtha = query_length();
		if(lengtha == 0) lengtha = 220000;
		tempa = malloc(lengtha);
		lengtha = dequeue(tempa);
		//lidbg("=====dequeue2===%d===\n",lengtha);
		if( (isBlackBoxTopRec || isNewFile) && !isOldFp && !isRemainOldFp)
		{
			if(!isBeginTopDeq)
			{
#if 1		
				unsigned char tmp_val = 0;
				//lidbg("****[%d]isNewFile*****\n",cam_id);
				tmp_val = *(unsigned char*)(tempa + 18);
				if(tmp_val == 0x68) 
				{

					/*1280x720*/
					tmp_val = *(unsigned char*)(tempa + 26);
					if(tmp_val == 0x65) 
					{
						isBeginTopDeq = 1;
						isNewFile = 0;
					}
				}

				tmp_val = *(unsigned char*)(tempa + 19);
				if(tmp_val == 0x68) 
				{
					/*640x360*/
					tmp_val = *(unsigned char*)(tempa + 27);
					if(tmp_val == 0x65)
					{
						isBeginTopDeq = 1;
						isNewFile = 0;
					}
				}
				
#else
				isBeginTopDeq = 1;
#endif
			}
		}
		else isBeginTopDeq = 1;
		
		if(isVideoLoop > 0)
		{
			if(isBeginTopDeq && rec_fp != NULL) fwrite(tempa, lengtha, 1, rec_fp);//write data to the output files
			//else lidbg("****[%d]throw*****\n",cam_id);
		}
		if(isBlackBoxTopRec) 
		{
			if(isBeginTopDeq && fp1 != NULL) fwrite(tempa, lengtha, 1, fp1);
		}
		else if(isBlackBoxBottomRec)
		{
			if(fp2 != NULL) fwrite(tempa, lengtha, 1, fp2);
		}
		if(tempa != NULL) free(tempa);
	}
	if(fp1 != NULL) 
	{
		fflush(fp1);
		fclose(fp1);
	}
	if(fp2 != NULL)
	{
		fflush(fp2);
		fclose(fp2);
	}
	if(isConvertMP4 && !BlackBoxBottomCnt)
	{
		if(isBlackBoxTopRec == 0 && isBlackBoxBottomRec == 1) 
		{
			char tmp_cmd[500] = {0};
			//trans_enqueue("/storage/sdcard0/em_test/trans.h264",dvr_blackbox_filename);
			if(cam_id == DVR_ID) 
			{
#if 0		
				int length;
				length = strlen(dvr_blackbox_filename);
				dvr_blackbox_filename[length - 5] = '\0';
#endif			
				sprintf(tmp_cmd, "am broadcast -a com.flyaudio.lidbg.H264ToMp4.H264ToMp4Service --ei action 0 --es src %s --es dec %s&",dvr_blackbox_filename,dvr_blackbox_dest_filename);
				system(tmp_cmd);
				//isToDel = 1;
				send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_ONLINE_INVOKE_NOTIFY, RET_EM_ISREC_OFF);
			}
			else if(cam_id == REARVIEW_ID) 
			{
#if 0				
				int length;
				length = strlen(rear_blackbox_filename);
				rear_blackbox_filename[length - 5] = '\0';
#endif			
				sprintf(tmp_cmd, "am broadcast -a com.flyaudio.lidbg.H264ToMp4.H264ToMp4Service --ei action 0 --es src %s --es dec %s&",rear_blackbox_filename,rear_blackbox_dest_filename);
				system(tmp_cmd);
				//isToDel = 1;
			}
		}
	}
	if(isBlackBoxTopRec == 0 && !(BlackBoxBottomCnt--)) isBlackBoxBottomRec = 0;
	isBlackBoxTopRec = 0;
	//system("setprop lidbg.uvccam.isdequeue 0");
	property_set("lidbg.uvccam.isdequeue", "0");
}


void *thread_dequeue(void *par)
{
	//unsigned int count = *(unsigned int*)par;
	unsigned int count = tmp_count;
	while(1)
	{
		if(isNormDequeue)
		{
			//lidbg("[%d]%s: count = %d ,isDequeue:%d\n", cam_id,__func__,tmp_count,isDequeue);
			
			if(isOldFp && old_rec_fp1 != NULL)
			{
				while(1)
				{
					if(!isDequeue) break;
				}
			}
			
			if(!isDequeue)
			{
				isDequeue = 1;
				XU_H264_Set_IFRAME(dev);
				if(isOldFp) 
				{
					//lidbg("****<%d>deq old_rec_fp1****\n",cam_id);
					if(msize > Emergency_Top_Sec * 30)
					{
						isRemainOldFp = 1;
						dequeue_buf(msize -Emergency_Top_Sec * 30 ,old_rec_fp1);
					}
					else dequeue_buf(msize,old_rec_fp1);
				}
				else if(isRemainOldFp)
				{
					dequeue_buf(Emergency_Top_Sec * 30 ,old_rec_fp1);
					if(old_rec_fp1 != NULL) fclose(old_rec_fp1);
					isRemainOldFp = 0;
				}
				else dequeue_buf(tmp_count,rec_fp1);
				isDequeue = 0;
			}
#if 0
			if(isOldFp && old_rec_fp1 != NULL)
			{
				//lidbg("****<%d>fclose old_rec_fp1****\n",cam_id);
				fclose(old_rec_fp1);
				isOldFp = 0;
			}
#else
			isOldFp = 0;
#endif
			isNormDequeue = 0;
		}
		usleep(10*1000);
	}
	return 0;
}

void *thread_top_dequeue(void *par)
{
	//unsigned int count = *(unsigned int*)par;
	unsigned int count;
	while(1)
	{
		if(isTopDequeue)
		{
			//lidbg("%s: E \n", __func__);
			if(!isDequeue)
			{
				lidbg("%s: E dq\n", __func__);
				isDequeue = 1;
				if(top_lastFrames < (Emergency_Top_Sec*30 - 150)) count = Emergency_Top_Sec*30 - 150;
				else if(top_lastFrames > (Emergency_Top_Sec*30 + 30)) count = Emergency_Top_Sec*30 + 30;
				else count = (top_lastFrames/10)*10 + 10;
				XU_H264_Set_IFRAME(dev);
				lidbg_get_current_time(0 , deq_time_buf, NULL);
				if(msize > count)
					dequeue_buf(msize - count,rec_fp1);
				isBlackBoxTopRec = 1;
				isBlackBoxBottomRec = 1;
				if(count < msize)
					dequeue_buf(count,rec_fp1);
				else dequeue_buf(msize,rec_fp1);
				isDequeue = 0;
				isTopDequeue = 0;
			}
		}
		usleep(10*1000);
	}
	return 0;
}

void *thread_top_count_frame(void *par)
{
	while(1)
	{
		sleep(Emergency_Top_Sec);
		//lidbg("%s: [%d] top_totalFrames = %d \n", __func__,cam_id,top_totalFrames);
		top_lastFrames = top_totalFrames;
		top_totalFrames = 0;
	}
}

void *thread_bottom_count_frame(void *par)
{
	while(1)
	{
		sleep(Emergency_Bottom_Sec);
		//lidbg("%s: [%d] bottom_totalFrames = %d \n", __func__,cam_id,bottom_totalFrames);
		bottom_lastFrames = bottom_totalFrames;
		bottom_totalFrames = 0;
	}
}

void *thread_del_tmp_emfile(void *par)
{
	char minRecName[100] = {0};
	int filecnt = 0;
	char minRecPath[200] = {0};
	char tmp_cmd[300] = {0};
	char isFormat_str[PROPERTY_VALUE_MAX];
	int isFormat = 0;
	while(1)
	{
		sleep(10);
		filecnt = find_earliest_file(Em_Save_Tmp_Dir,minRecName);
		if(filecnt < 0) 
		{
			property_get("lidbg.uvccam.isFormat", isFormat_str, "0");
			isFormat = atoi(isFormat_str);
			if(isFormat)
			{
				lidbg("======Fomat process!Stop making dir!======\n");
				continue;
			}
			lidbg("======Em_Save_Tmp_Dir access error!======\n");
			mkdir(Em_Save_Tmp_Dir,S_IRWXU|S_IRWXG|S_IRWXO);
			continue;
		}
		sprintf(minRecPath, "%s/%s",Em_Save_Tmp_Dir,minRecName);//minRecPath
		//lidbg("\n****dvr:%s\n,rear:%s,\nmini:%s\n,filecnt:%d****\n",dvr_blackbox_filename,rear_blackbox_filename,minRecPath,filecnt);
		if(filecnt > 4)
		{
			if(strcmp(minRecPath,dvr_blackbox_filename) != 0)
			{
				sprintf(tmp_cmd, "rm -rf %s/%s&",Em_Save_Tmp_Dir,minRecName);
				system(tmp_cmd);
				lidbg("%s:****del %s/%s****\n",__func__,Em_Save_Tmp_Dir,minRecName);
			}
		}
	}
}

void *thread_capture(void *par)
{

	unsigned int mjpg_resolution = 0;//(width << 16) | (height);
	unsigned int mjpg_width = 0;
	unsigned int mjpg_height = 0;
	unsigned int unknow_size = 0;
	unsigned int skip = 0;
	struct thread_parameter thread_par = * (struct thread_parameter*) par;
	int i = 0;
	int ret;
	FILE *file = NULL;
	char filename[] = "";
/*	
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.dev = %d \n",*thread_par.dev);
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.nframes = %d \n",*thread_par.nframes);
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.multi_stream_mjpg_enable = %d \n",(unsigned int)thread_par.multi_stream_mjpg_enable);
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.mem = 0x%x \n",thread_par.mem[0]);
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.mem = 0x%x \n",thread_par.mem[1]);
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.mem = 0x%x \n",thread_par.mem[2]);
	TestAp_Printf(TESTAP_DBG_FLOW, "  +++ thread_par.mem = 0x%x \n",thread_par.mem[3]);
*/
	if(thread_par.multi_stream_mjpg_enable)
	{
		skip = 6;
	}
	lidbg( "-----------eho-----thread_capture-------.\n");
	for (i = 0; i < *thread_par.nframes; ++i) 
	{
		unknow_size = 0;
		
		/* Dequeue a buffer. */
		memset(thread_par.buf, 0, sizeof *thread_par.buf);
		thread_par.buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		thread_par.buf->memory = V4L2_MEMORY_MMAP;
		ret = ioctl(*thread_par.dev, VIDIOC_DQBUF, thread_par.buf);
		if (ret < 0) {
			lidbg( "Unable to dequeue -thread- buffer (%d).\n", errno);
			close(*thread_par.dev);
			return -1;
		}
		
		/* Save the image. */
		if(thread_par.multi_stream_mjpg_enable)
		{
			//mjpg_width = (unsigned int)((*(unsigned char *)(thread_par.mem[(thread_par.buf->index)]+9)) << 8) | (*(unsigned char *)(thread_par.mem[(thread_par.buf->index)]+10));
			//mjpg_height = (unsigned int)((*(unsigned char *)(thread_par.mem[(thread_par.buf->index)]+7)) << 8) | (*(unsigned char *)(thread_par.mem[(thread_par.buf->index)]+8));
			//mjpg_resolution = (unsigned int)((mjpg_width << 16) | (mjpg_height));
			mjpg_resolution = thread_par.buf->reserved;
			
			if(mjpg_resolution == H264_SIZE_HD)
				sprintf(filename, "[   HD]frame-%06u.jpg", i);
			else if(mjpg_resolution == H264_SIZE_VGA)
				sprintf(filename, "[  VGA]frame-%06u.jpg", i);
			else if(mjpg_resolution == H264_SIZE_QVGA)
				sprintf(filename, "[ QVGA]frame-%06u.jpg", i);
			else if(mjpg_resolution == H264_SIZE_QQVGA)
				sprintf(filename, "[QQVGA]frame-%06u.jpg", i);
            else if(mjpg_resolution == H264_SIZE_360P)
				sprintf(filename, "[360P]frame-%06u.jpg", i);
            else if(mjpg_resolution == H264_SIZE_180P+4)
				sprintf(filename, "[180P]frame-%06u.jpg", i);
			else
			{
				unknow_size = 1;
				//TestAp_Printf(TESTAP_DBG_FRAME, "  ### [frame %4d] mjpg  unknow size  w=%d  h=%d  \n",i , mjpg_width,mjpg_height);
				//sprintf(filename, "[%4d*%4d]frame-%06u.jpg", mjpg_width, mjpg_height , i);
			}
		}
		else
		{
			sprintf(filename, "frame-%06u.jpg", i);		
		}
		
		//TestAp_Printf(TESTAP_DBG_FRAME, "  +++ unknow_size=%d  skip=%d    %x  \n", unknow_size,skip,mjpg_resolution);
		
		if((!unknow_size)&&(!skip))
		{
			file = fopen(filename, "wb");
			if (file != NULL) 
			{
				fwrite(thread_par.mem[thread_par.buf->index], thread_par.buf->bytesused, 1, file);
				fclose(file);
			}
		}
		
		if(skip)
			--skip;
	
		ret = ioctl(*thread_par.dev, VIDIOC_QBUF, thread_par.buf);
		if (ret < 0) {
			lidbg( "Unable to requeue -thread- buffer (%d).\n", errno);
			close(*thread_par.dev);			
			return -1;
		}
	}

	pthread_exit(NULL);
	return 0;
}

void osd_set(int cam_id)
{
	char time_buf[100] = {0};
	char devName[50] = {0};
	char OSDSet_Str[PROPERTY_VALUE_MAX];
	int fail_times = 0;
	int rc = -1;
    lidbg("%s:E\n",__func__); 
    while(1)
    { 
    	if(cam_id == DVR_ID) property_get("lidbg.uvccam.dvr.osdset", OSDSet_Str, "0");
		else if(cam_id == REARVIEW_ID) property_get("lidbg.uvccam.rear.osdset", OSDSet_Str, "0");
		if(!strncmp(OSDSet_Str, "1", 1))
		{
			if((dev == NULL) ||  (lidbg_get_current_time(1,time_buf, NULL) < 0))
			{
				lidbg("%s: ===OSD Open dev===\n", __func__);
		    	rc = lidbg_get_hub_uvc_device(RECORD_MODE,devName,cam_id,1);
			    if((rc == -1)  || (*devName == '\0'))
			    {
			        lidbg("%s: No UVC node found \n", __func__);
			    }
				else
				{
					dev = video_open(devName);
					if(dev != NULL && (3 == ++fail_times) && (XU_Ctrl_ReadChipID(dev) < 0)) 
					{
						send_driver_msg(FLYCAM_STATUS_IOC_MAGIC,NR_STATUS,RET_DVR_OSD_FAIL);
					}
					lidbg("===fail_times:%d==\n",fail_times);
				}
				//XU_OSD_Timer_Ctrl(dev, 0);
			}
			else fail_times = 0;
			sleep(4);
		}
		sleep(1);
    } 
    lidbg("%s:X\n",__func__); 
    return 0;
}

#if 0
void *thread_switch(void *par)
{
    lidbg("-------eho--------%s----start\n",__func__); 
    while(1)
    { 
    	property_get("persist.lidbg.uvccam.recording", startRecording, "0");
	//property_get("persist.lidbg.uvccam.capture", startCapture, "0");
		usleep(500);
		if(!strncmp(startRecording, "0", 1)) break;
    } 
    lidbg("-------eho--------%s----exit\n",__func__);
    return 0;
}
#endif
#if 0
void *thread_nightmode(void *par)
{
	char on = 1;
    lidbg("-------eho--------%s----start\n",__func__); 
    while(1)
    { 
    	property_get("lidbg.uvccam.nightmode", startNight, "1");
		sleep(1);
		if((on != 1) && (!strncmp(startNight, "1", 1)))
		{
			on = 1;
			lidbg("========startNight==========");
			if (v4l2SetControl (dev, V4L2_CID_GAIN, NIGHT_GAINVAL)<0)
				lidbg("----eho---- : do_gain (%d) Failed", NIGHT_GAINVAL);
			if (v4l2SetControl (dev, V4L2_CID_CONTRAST, NIGHT_CONTRASTVAL)<0)
				lidbg("----eho---- : do_contrast (%d) Failed", NIGHT_CONTRASTVAL);
			if (v4l2SetControl (dev, V4L2_CID_SATURATION, NIGHT_SATURATIONVAL)<0)
				lidbg("----eho---- : do_saturation (%d) Failed", NIGHT_SATURATIONVAL);
			if (v4l2SetControl (dev, V4L2_CID_BRIGHTNESS, NIGHT_BRIGHTVAL - 64)<0)
				lidbg("----eho---- : do_bright (%d) Failed", NIGHT_BRIGHTVAL);
			if (v4l2SetControl (dev, V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL)<0)
				lidbg("----eho---- : do_exposure (%d) Failed", NIGHT_EXPOSUREVAL);
			if (v4l2SetControl (dev, V4L2_CID_EXPOSURE_ABSOLUTE, NIGHT_EXPOSUREVAL)<0)
				lidbg("----eho---- : do_exposure (%d) Failed", NIGHT_EXPOSUREVAL);
		}
		else if((on != 0) && (!strncmp(startNight, "0", 1)))
		{
			on = 0;
			lidbg("========startDay==========");
			if (v4l2SetControl (dev, V4L2_CID_GAIN, DAY_GAINVAL)<0)
				lidbg("----eho---- : do_gain (%d) Failed", DAY_GAINVAL);
			if (v4l2SetControl (dev, V4L2_CID_CONTRAST, DAY_CONTRASTVAL)<0)
				lidbg("----eho---- : do_contrast (%d) Failed", DAY_CONTRASTVAL);
			if (v4l2SetControl (dev, V4L2_CID_SATURATION, DAY_SATURATIONVAL)<0)
				lidbg("----eho---- : do_saturation (%d) Failed", DAY_SATURATIONVAL);
			if (v4l2SetControl (dev, V4L2_CID_BRIGHTNESS, DAY_BRIGHTVAL - 64)<0)
				lidbg("----eho---- : do_bright (%d) Failed", DAY_BRIGHTVAL);
			if (v4l2SetControl (dev, V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_AUTO)<0)
				lidbg("----eho---- : do_exposure (%d) Failed", NIGHT_EXPOSUREVAL);
		}
    } 
    lidbg("-------eho--------%s----exit\n",__func__);
    return 0;
}
#endif
int lidbg_token_string(char *buf, char *separator, char **token)
{
    char *token_tmp;
    int pos = 0;
    if(!buf || !separator)
    {
        lidbg("buf||separator NULL?\n");
        return pos;
    }
    while((token_tmp = strsep(&buf, separator)) != NULL )
    {
        *token = token_tmp;
        token++;
        pos++;
    }
    return pos;
}

static int lidbg_get_current_time(char isXUSet , char *time_string, struct rtc_time *ptm)
{
	time_t timep; 
	struct tm *p; 
	char rtc_cmd[100] = {0};
	time(&timep); 
	p=localtime(&timep); 
    if(time_string)
        sprintf(time_string, "%d-%02d-%02d__%02d.%02d.%02d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday,p->tm_hour , p->tm_min,p->tm_sec);
	//sprintf(rtc_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --xuset-rtc %d %d %d %d %d %d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday,p->tm_hour , p->tm_min,p->tm_sec);
	//system(rtc_cmd);
	//lidbg("\n===OSDSETTIME[%d] => %d-%02d-%02d__%02d.%02d.%02d===\n", cam_id ,(1900+p->tm_year), (1+p->tm_mon), p->tm_mday,p->tm_hour , p->tm_min,p->tm_sec);
	if(isXUSet)
	{
		if(XU_OSD_Set_RTC(dev, 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec) <0)
		{	
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_RTC Failed\n");
			return -1;
		}
	}
    return 0;
}

#if 0
  static int get_uvc_device(char *devname,char do_save,char do_record)
    {
        char    temp_devname[256];
        int     i = 0, ret = 0, fd;
        struct  v4l2_capability     cap;

        lidbg("%s: E\n", __func__);
        *devname = '\0';
        while(1)
        {
            sprintf(temp_devname, "/dev/video%d", i);
            fd = open(temp_devname, O_RDWR  | O_NONBLOCK, 0);
            if(-1 != fd)
            {
                ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
                if((0 == ret) || (ret && (ENOENT == errno)))
                {
                	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))//not usb cam node
				    {
					    lidbg("%s: This is not video capture device\n", __func__);
					    i++;
					    continue;
				    }
                    lidbg("%s: Found UVC node: %s\n", __func__, temp_devname);
					if((do_save) && (!do_record)) //capture
					{
						lidbg("----%s:-------capture----------\n",__func__);
						strncpy(devname, temp_devname, 256);
					}
                    else if((!do_save) && (do_record))//recording
                  	{
                  		lidbg("----%s:-------recording----------\n",__func__);
						sprintf(temp_devname, "/dev/video%d", i + 1);
						strncpy(devname, temp_devname, 256);
                  	}
					else
					{
						lidbg("----%s:-------user ctrl----------\n",__func__);
						sprintf(temp_devname, "/dev/video%d", i + 1);
						strncpy(devname, temp_devname, 256);
					}
                    break;
                }   
            }
            else if(2 != errno)
                lidbg("%s.%d: Probing.%s: ret: %d, errno: %d,%s", __func__, i, temp_devname, ret, errno, strerror(errno));
			close(fd);
			
            if(i++ > 1000)
            {
                strncpy(devname, "/dev/video1", 256);
                lidbg("%s.%d: Probing fail:%s \n", __func__, i, devname);
                //break;
                lidbg("%s: X,%s\n", __func__, devname);
				return 1;
            }
        }

        lidbg("%s: X,%s\n", __func__, devname);
        return 0;
    }

static int get_hub_uvc_device(char *devname,char do_save,char do_record)
{
	char temp_devname[256], temp_devname2[256],hub_path[256];
    int     i = 0, ret = 0, fd = -1, cam_id = -1, uvc_count = -1;
    struct  v4l2_capability     cap;
	DIR *pDir ;  
	struct dirent *ent  ;  
	int fcnt = 0  ;  
	char camID[PROPERTY_VALUE_MAX];

	
	if((do_save) || (do_record))  cam_id = 1;  //capture or recording force to camid 1
	else
	{
		property_get("fly.uvccam.camid", camID, "0");//according to last preview camid
		cam_id = atoi(camID);
	}

    lidbg("%s: E,======[%d]\n", __func__, cam_id);
    *devname = '\0';

	memset(hub_path,0,sizeof(hub_path));  
	memset(temp_devname,0,sizeof(temp_devname));  

	//fix for attenuation hub.find the deepest one.
	int back_charcnt = 0,front_charcnt = 0;
	pDir=opendir("/sys/bus/usb/drivers/usb/");  
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

	if((front_charcnt == 0) && (back_charcnt == 0))
	{
		lidbg("%s: can not found suitable hubpath!\n ", __func__ );	
		goto failproc;
	}
	
	lidbg("%s:hubPath:%s\n",__func__ ,hub_path);  
#if 0
	//check Front | Back Cam
	if(cam_id == 1)
		sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", FRONT_NODE,FRONT_NODE);//front cam
	else if(cam_id == 0)
		sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", BACK_NODE,BACK_NODE);//back cam
	else
	{
		lidbg("%s: cam_id wrong!==== %d ", __func__ , cam_id);
		goto failproc;
	}
#endif
	if(access(hub_path, R_OK) != 0)
	{
		lidbg("%s: hub path access wrong!\n ", __func__ );
		goto failproc;
	}
	
	pDir=opendir(hub_path);  
	while((ent=readdir(pDir))!=NULL)  
	{  
			fcnt++;
	        if(ent->d_type & DT_DIR)  
	        {  
	                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (strncmp(ent->d_name, "video", 5)))  
	                        continue;  
					if(fcnt == 4)//also save 2nd node name
					{
						sprintf(temp_devname2,"/dev/%s", ent->d_name);  
						sprintf(temp_devname,"/dev/%s", ent->d_name); 
						break;
					}
	                sprintf(temp_devname,"/dev/%s", ent->d_name);  
	                lidbg("%s:Path:%s\n",__func__ ,temp_devname);  
	        }  
	}
	closedir(pDir);
	lidbg("%s: This Camera has %d video node.\n", __func__ , fcnt - 2);
	if((fcnt == 3) && (cam_id == 1))	
	{
		lidbg("%s: Front Camera does not support Sonix Recording!\n", __func__);
		goto failproc;
	}
	
	if((fcnt == 0) && (ent == NULL))
	{
		lidbg("%s: Hub node is not exist !\n", __func__);
		goto failproc;
	}

	if((do_save) && (!do_record)) //capture
	{
		lidbg("----%s:-------capture----------\n",__func__);
		strncpy(devname, temp_devname, 256);
	}
	else if((!do_save) && (do_record))//recording
	{
		lidbg("----%s:-------recording----------\n",__func__);
		strncpy(devname, temp_devname, 256);
	}
	else
	{
		lidbg("----%s:-------user ctrl----------\n",__func__);
		strncpy(devname, temp_devname, 256);
	}  
	
openDev:
	  lidbg("%s: trying open ====[%s]\n", __func__, devname);
      fd = open(devname, O_RDWR  | O_NONBLOCK, 0);
      if(-1 != fd)
      {
          ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
          if((0 == ret) || (ret && (ENOENT == errno)))
          {
          	  //not usb cam node
              if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
              {
					lidbg("%s: This is not video capture device\n", __func__);
					close(fd);
					goto failproc;
              }      
              lidbg("%s: Found UVC node,OK: ======%s,[camid = %d]\n", __func__, devname, cam_id);
          }
      }
      else if(2 != errno)
          lidbg("%s: Probing.%s: ret: %d, errno: %d,%s\n", __func__, devname, ret, errno, strerror(errno));
	  close(fd);
      lidbg("%s: X,%s\n", __func__, devname);
      return 0;

failproc:
	strncpy(devname, "/dev/video1", 256);
	lidbg("%s: Probing fail:%s , run normal proc\n", __func__, devname);
#if NONE_HUB_SUPPORT
	system("echo flyaudio:touch /dev/log/CameraScan2.txt > /dev/lidbg_misc0");
	return get_uvc_device(devname,do_save,do_record);
#else
	return 1;
#endif
}

#endif

static void switch_scan(void)
{
	if(cam_id == DVR_ID)
	{
		property_get("lidbg.uvccam.dvr.recording", startRecording, "0");
		property_get("lidbg.uvccam.dvr.blackbox", isBlackBoxRec, "0");

		property_get("persist.uvccam.isDVRVideoLoop", isVideoLoop_Str, "0");
		isVideoLoop = atoi(isVideoLoop_Str);
		//lidbg("======== isDVRVideoLoop-> %d=======\n",isVideoLoop);
	}
	else if(cam_id == REARVIEW_ID)
	{
		property_get("lidbg.uvccam.rearview.recording", startRecording, "0");
		property_get("lidbg.uvccam.rear.blackbox", isBlackBoxRec, "0");

		property_get("persist.uvccam.isRearVideoLoop", isVideoLoop_Str, "0");
		isVideoLoop = atoi(isVideoLoop_Str);
		//lidbg("======== isRearVideoLoop-> %d=======\n",isVideoLoop);
	}
	property_get("persist.uvccam.empath", Em_Save_Dir, EMMC_MOUNT_POINT1"/camera_rec/BlackBox/");
	sprintf(Em_Save_Tmp_Dir, EMMC_MOUNT_POINT0"/camera_rec/BlackBox/.tmp/");//Em_Save_Tmp_Dir
	property_get("persist.uvccam.top.emtime", Em_Top_Sec_String, "5");
	property_get("persist.uvccam.bottom.emtime", Em_Bottom_Sec_String, "10");
	Emergency_Top_Sec = atoi(Em_Top_Sec_String);
	Emergency_Bottom_Sec = atoi(Em_Bottom_Sec_String);
	/*Force to 60s bottom*/
	if(!isConvertMP4) Emergency_Bottom_Sec = 60;
	
	//if(isToDel)
	//{
#if 0	
		property_get("lidbg.uvccam.isTranscoding", isTranscoding_Str, "0");
		isTranscoding = atoi(isTranscoding_Str);
		if(isTranscoding) isToDel = 1;
		if(!isTranscoding && isToDel) 
		{
			char tmp_cmd[300] = {0};
			sprintf(tmp_cmd, "rm -rf %s.h264&",dvr_blackbox_filename);
			system(tmp_cmd);
			lidbg("****del %s.h264****\n",dvr_blackbox_filename);
			isToDel = 0;
		}
#endif		
	//}
	return;
}

static void send_driver_msg(char magic ,char nr,unsigned long arg)
{
#if 0
	/*Do not send msg while in RearView mode:still not have dedicate StatusCode*/
	if(cam_id == REARVIEW_ID)
	{
		lidbg("%s:stop send msg.\n",__func__);
		return;
	}
#endif
	if(cam_id == REARVIEW_ID) arg = arg | 0x80;
	if (ioctl(flycam_fd,_IO(magic, nr), arg) < 0)
    {
      	lidbg("%s:nr => %d ioctl fail=======\n",__func__,nr);
	}	
	return;
}

/*EF or ER*/
int lidbg_del_days_file(char* Dir,int days)
{
	char filepath[200] = {0};
	DIR *pDir ;
	struct dirent *ent; 
	unsigned char filecnt = 0;
	int ret;
	char *date_time_key[5] = {NULL};
	char *date_key[3] = {NULL};
	char *time_key[3] = {NULL};
	int cur_date[3] = {0,0,0};
	int cur_time[3] = {0,0,0};
	int min_date[3] = {5000,13,50};
	int min_time[3] = {13,100,100};
	char tmpDName[100] = {0};
	struct tm prevTm,curTm;
	time_t prevtimep = 0,curtimep,currentTime;
	int diffval;

	lidbg("************del_days_file -> %d************\n",days);
	time( &currentTime ); 
	
	pDir=opendir(Dir);  
	if(pDir == NULL) return -1;
	while((ent=readdir(pDir))!=NULL)  
	{  
	        if(!(ent->d_type & DT_DIR))  
	        {  
	                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (ent->d_reclen != 48) ) 
	                        continue;  
					if(strncmp(ent->d_name, "E", 1))
							continue;
						
						filecnt++; 
						strcpy(tmpDName, ent->d_name + 2);
						lidbg_token_string(tmpDName, "__", date_time_key);
						lidbg_token_string(date_time_key[0], "-", date_key);
						lidbg_token_string(date_time_key[2], ".", time_key);
					
						curTm.tm_year = atoi(date_key[0]) -1900;
						curTm.tm_mon = atoi(date_key[1]) -1;
						curTm.tm_mday = atoi(date_key[2]);
						curTm.tm_hour = atoi(time_key[0]);
						curTm.tm_min = atoi(time_key[1]);
						curTm.tm_sec	 = atoi(time_key[2]);	
						curtimep = mktime(&curTm);

    					diffval =  (currentTime - curtimep) /(24*60*60);
						//lidbg("====== diff days:%d.======\n",diffval);
						if(diffval > days)
						{
							sprintf(filepath , "%s/%s",Dir,ent->d_name);
							lidbg("====== oldest EM file will be del:%s.======\n",filepath);
							remove(filepath);  
						}
	        }  
	}
	closedir(pDir);
	return filecnt;

}

int find_earliest_file(char* Dir,char* minRecName)
{
	DIR *pDir ;
	struct dirent *ent; 
	unsigned char filecnt = 0;
	int ret;
	char *date_time_key[5] = {NULL};
	char *date_key[3] = {NULL};
	char *time_key[3] = {NULL};
	int cur_date[3] = {0,0,0};
	int cur_time[3] = {0,0,0};
	int min_date[3] = {5000,13,50};
	int min_time[3] = {13,100,100};
	char tmpDName[100] = {0};
	struct tm prevTm,curTm;
	time_t prevtimep = 0,curtimep;
	
	/*find the earliest rec file and del*/
	pDir=opendir(Dir);  
	if(pDir == NULL) return -1;
	while((ent=readdir(pDir))!=NULL)  
	{  
	        if(!(ent->d_type & DT_DIR))  
	        {  
	                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (ent->d_reclen != 48) ) 
	                        continue;  
					//if((!strncmp(ent->d_name, "F", 1) && (cam_id == DVR_ID)) ||(!strncmp(ent->d_name, "R", 1) && (cam_id == REARVIEW_ID)) )
					//{
					if(strncmp(ent->d_name, "F", 1) && strncmp(ent->d_name, "R", 1))
							continue;
						
						filecnt++;
		                //lidbg("ent->d_name:%s====ent->d_reclen:%d=====\n", ent->d_name,ent->d_reclen); 

						strcpy(tmpDName, ent->d_name + 1);
						lidbg_token_string(tmpDName, "__", date_time_key);
						//lidbg("date_time_key0:%s====date_time_key1:%s=====", date_time_key[0],date_time_key[2]);	
						lidbg_token_string(date_time_key[0], "-", date_key);
						//lidbg("date_key:%s====%s===%s==", date_key[0],date_key[1],date_key[2]);	
						lidbg_token_string(date_time_key[2], ".", time_key);
						//lidbg("time_key:%s====%s===%s==", time_key[0],time_key[1],time_key[2]);	
						
						curTm.tm_year = atoi(date_key[0]) -1900;
						curTm.tm_mon = atoi(date_key[1]) -1;
						curTm.tm_mday = atoi(date_key[2]);
						curTm.tm_hour = atoi(time_key[0]);
						curTm.tm_min = atoi(time_key[1]);
						curTm.tm_sec	 = atoi(time_key[2]);	
						curtimep = mktime(&curTm);
						
						#if 0
						lidbg("prevtimep=======%d========",  prevtimep);
						lidbg("curtimep=======%d========",  curtimep);
						lidbg("difftime=======%d========", difftime(curtimep, prevtimep));
						#endif
						if((curtimep < prevtimep) || (prevtimep == 0))
						{
							prevtimep = curtimep;
							strcpy(minRecName, ent->d_name);
							//lidbg("minRecName---->%s\n",minRecName);
						}
					//}
	        }  
	}
	closedir(pDir);
	return filecnt;
}

static void get_driver_prop(int camID)
{
		/*set each file recording time*/
		if(camID == DVR_ID)
			property_get("fly.uvccam.dvr.rectime", Rec_Sec_String, "120");
		else if(camID == REARVIEW_ID)
			property_get("fly.uvccam.rearview.rectime", Rec_Sec_String, "120");
		Rec_Sec = atoi(Rec_Sec_String);
		lidbg("========set each file recording time-> %d s=======\n",Rec_Sec);
		if(Rec_Sec == 0) 
		{
			lidbg("not allow recording time = 0s !!reset to 300s.\n");
			Rec_Sec = 300;
		}
		
#if 1
		/*set max file num*/
		if(camID == DVR_ID)
			property_get("fly.uvccam.dvr.recnum", Max_Rec_Num_String, "5");
		else if(camID == REARVIEW_ID)
			property_get("fly.uvccam.rearview.recnum", Max_Rec_Num_String, "5");
		Max_Rec_Num = atoi(Max_Rec_Num_String);
		lidbg("====Max_rec_num-> %d===\n",Max_Rec_Num);
		if(Max_Rec_Num == 0) Max_Rec_Num = 5;
#endif

		/*set record file savePath*/
		if(camID == DVR_ID)
			property_get("fly.uvccam.dvr.recpath", Rec_Save_Dir, EMMC_MOUNT_POINT1"/camera_rec/");
		else if(camID == REARVIEW_ID)
			property_get("fly.uvccam.rearview.recpath", Rec_Save_Dir, EMMC_MOUNT_POINT0"/");
		lidbg("==========recording dir -> %s===========\n",Rec_Save_Dir);
		if(!strncmp(Rec_Save_Dir, "/storage/udisk", 14) )
		{
			char tmp_usb_mkdir[100] = "mkdir /storage/udisk/camera_rec/";
			lidbg("======== try create udisk rec dir -> %s =======\n",Rec_Save_Dir);
			sprintf(tmp_usb_mkdir, "mkdir %s", Rec_Save_Dir);
			system(tmp_usb_mkdir);
		}
#if 0
		/*create preview cache dir*/
		if(!strncmp(Rec_Save_Dir, EMMC_MOUNT_POINT0"/preview_cache", 30) )
		{
			/*
			char tmp_preview_mkdir[100] = "mkdir "EMMC_MOUNT_POINT0"/preview_cache";
			lidbg("======== try create preview cache dir -> %s =======",Rec_Save_Dir);
			sprintf(tmp_preview_mkdir, "mkdir %s", Rec_Save_Dir);
			system(tmp_preview_mkdir);
			*/
			isPreview = 1;
		}
#endif

#if 0
		if(access(Rec_Save_Dir, R_OK) != 0)
		{
			lidbg("record file path access wrong!\n" );
			//return 0;
			strcpy(Rec_Save_Dir,  EMMC_MOUNT_POINT0"/camera_rec/");
		}
#endif
		
		
		/*set record file total size*/
		if(camID == DVR_ID)
			property_get("fly.uvccam.dvr.recfilesize", Rec_File_Size_String, "8192");
		else if(camID == REARVIEW_ID)
			property_get("fly.uvccam.rearview.recfilesize", Rec_File_Size_String, "8192");
		Rec_File_Size = atoi(Rec_File_Size_String);
		lidbg("======== video file total size-> %ld MB=======\n",Rec_File_Size);
		if(Rec_File_Size == 0) 
		{
			lidbg("not allow video file size = 0MB !!reset to 8192MB.\n");
			Rec_File_Size = 8192;
		}

		/*set record file bitrate*/
		if(camID == DVR_ID)
			property_get("fly.uvccam.dvr.recbitrate", Rec_Bitrate_String, "8000000");
		else if(camID == REARVIEW_ID)
			property_get("fly.uvccam.rearview.recbitrate", Rec_Bitrate_String, "8000000");
		Rec_Bitrate = atoi(Rec_Bitrate_String);
		lidbg("======== video bitrate-> %ld b/s=======\n",Rec_Bitrate);
		if(Rec_Bitrate == 0) 
		{
			lidbg("not allow video bitrate = 0MB !!reset to 8000000b/s.\n");
			Rec_Bitrate = 8000000;
		}

		if(camID == DVR_ID)
			property_get("fly.uvccam.dvr.res", Res_String, "0");
		else if(camID == REARVIEW_ID)
			property_get("fly.uvccam.rearview.res", Res_String, "0");

		if(camID == DVR_ID) 
		{
			property_get("fly.uvccam.isDualCam", isDualCam_String, "0");
			isDualCam = atoi(isDualCam_String);
			lidbg("======== isDualCam-> %d=======\n",isDualCam);

			property_get("fly.uvccam.coldboot.isRec", isColdBootRec_String, "0");
			isColdBootRec = atoi(isColdBootRec_String);
			lidbg("======== isColdBootRec-> %d=======\n",isColdBootRec);

			property_get("persist.uvccam.isEmNotPermit", isEmPermit_Str, "0");
			isEmPermit = !(atoi(isEmPermit_Str));
			lidbg("======== isEmPermit-> %d=======\n",isEmPermit);
		}

		if(camID == DVR_ID)
		{
			property_get("persist.uvccam.isDVRVideoLoop", isVideoLoop_Str, "0");
			isVideoLoop = atoi(isVideoLoop_Str);
			lidbg("======== isDVRVideoLoop-> %d=======\n",isVideoLoop);
		}
		else if(camID == REARVIEW_ID)
		{
			property_get("persist.uvccam.isRearVideoLoop", isVideoLoop_Str, "0");
			isVideoLoop = atoi(isVideoLoop_Str);
			lidbg("======== isRearVideoLoop-> %d=======\n",isVideoLoop);
		}

		if(camID == DVR_ID)
		{
			property_get("lidbg.uvccam.isDelDaysFile", isDelDaysFile_Str, "0");
			isDelDaysFile = atoi(isDelDaysFile_Str);
			lidbg("======== isDelDaysFile-> %d=======\n",isDelDaysFile);

			property_get("persist.uvccam.delDays", delDays_Str, "6");
			delDays = atoi(delDays_Str);
			lidbg("======== delDays-> %d=======\n",delDays);
		}

		property_get("lidbg.uvccam.isConvertMP4", isConvertMP4_Str, "0");
		isConvertMP4 = atoi(isConvertMP4_Str);
		if(isConvertMP4 > 0) 
			lidbg("======== ConvertMP4!=======\n");
		
#if 0
		property_get("lidbg.uvccam.isDisableVideoLoop", isDisableVideoLoop_Str, "0");
		isDisableVideoLoop = atoi(isDisableVideoLoop_Str);
		lidbg("======== isDisableVideoLoop-> %d=======\n",isDisableVideoLoop);
#endif

		/*
		char i = 10;
		Rec_Bitrate = 512000;
		*/
#if 0
		if(XU_Ctrl_ReadChipID(dev) < 0)
			lidbg( "XU_Ctrl_ReadChipID Failed\n");
		if(XU_H264_Set_BitRate(dev, Rec_Bitrate) < 0 )
			lidbg( "XU_H264_Set_BitRate Failed\n");
		XU_H264_Get_BitRate(dev, &m_BitRate);
		if(m_BitRate < 0 )
			lidbg( "SONiX_UVC_TestAP @main : XU_H264_Get_BitRate Failed\n");
		lidbg("Current bit rate1: %.2f Kbps\n",m_BitRate);
#endif
		/*
		while((i--) && (m_BitRate != Rec_Bitrate))
		{
			if(XU_H264_Set_BitRate(dev, Rec_Bitrate) < 0 )
				lidbg( "XU_H264_Set_BitRate Failed\n");
			usleep(500*1000);
		}
		lidbg("Current bit rate2: %.2f Kbps\n",m_BitRate);
		*/
		return;
}

static int get_path_free_space(char* path)
{
	struct statfs diskInfo;  
	statfs(path, &diskInfo);  
	unsigned long long totalBlocks = diskInfo.f_bsize;  
	unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;  
	size_t mbFreedisk = freeDisk>>20; 
	return mbFreedisk;
}


int main(int argc, char *argv[])
{
	char filename[100] = EMMC_MOUNT_POINT0"/quickcam-0000.jpg";
	char rec_filename[30] = EMMC_MOUNT_POINT0"/RecordH264.h264";			/*"H264.ts"*/
	char rec_filename1[30] = EMMC_MOUNT_POINT0"/RecordH264HD.h264";		/*"H264.ts"*/
	char rec_filename2[30] = EMMC_MOUNT_POINT0"/RecordH264QVGA.h264";	/*"H264.ts"*/
	char rec_filename3[30] = EMMC_MOUNT_POINT0"/RecordH264QQVGA.h264";	/*"H264.ts"*/
	char rec_filename4[30] = EMMC_MOUNT_POINT0"/RecordH264VGA.h264";		/*"H264.ts"*/
/*
	char flyh264_filename[5][100] = {  EMMC_MOUNT_POINT0"/flytmp1.h264",
								EMMC_MOUNT_POINT0"/flytmp2.h264",
								EMMC_MOUNT_POINT0"/flytmp3.h264",
								EMMC_MOUNT_POINT0"/flytmp4.h264",
								EMMC_MOUNT_POINT0"/flytmp5.h264"};
*/						    
	char flyh264_filename[100] = {0};
	char old_flyh264_filename[100] = {0};
	char flypreview_filename[100] = {0};
	char flypreview_prevcnt[20] = {0};
	
	int ret;
	int fake_dev; // chris
	int freeram;

	/* Options parsings */
	char do_save = 0, do_enum_inputs = 0, do_capture = 0, do_get_still_image = 0;
	char do_list_controls = 0, do_set_input = 0;
	char *endptr;
	int c;
	int framerate = 30;
    int bit_num = 0, tmp = 0;

	/* Video buffers */
	void *mem0[V4L_BUFFERS_MAX];
	void *mem1[V4L_BUFFERS_MAX];
	unsigned int pixelformat = V4L2_PIX_FMT_MJPEG;
	unsigned int width = 1920;
	unsigned int height = 1080;
	unsigned int nbufs = V4L_BUFFERS_DEFAULT;
	unsigned int input = 0;
	unsigned int skip = 0;

	/* Capture loop */
	struct timeval start, end, ts;
	unsigned int delay = 0, nframes = (unsigned int)-1;
	FILE *file = NULL;
	//FILE *rec_fp1 = NULL;
	FILE *rec_fp2 = NULL;
	FILE *rec_fp3 = NULL;
	FILE *rec_fp4 = NULL;
	double fps;
/*
	struct v4l2_buffer buf0;
	struct v4l2_buffer buf1;
*/
	unsigned int i;

	char do_record 			= 0;	
	/* Houston 2010/11/23 XU Ctrls */
	char do_add_xu_ctrl 	= 0;
	char do_xu_get_chip 	= 0;
	char do_xu_get_fmt   	= 0;
	char do_xu_set_fmt   	= 0;
	char do_xu_get_qp  		= 0;
	char do_xu_set_qp  		= 0;
	char do_xu_get_br  		= 0;
	char do_xu_set_br  		= 0;
	char do_enum_MaxPayloadTransSize = 0;
	int m_QP_Val = 0;
	double m_BitRate = 0.0;
	struct Cur_H264Format cur_H264fmt;
	char do_xu_get 			= 0;
	char do_xu_set 			= 0;
	unsigned char GetXU_ID = 0;
	unsigned char SetXU_ID = 0;
	unsigned char GetCS = 0;
	unsigned char SetCS = 0;
	unsigned char GetCmdDataNum = 0;
	unsigned char SetCmdDataNum = 0;
	__u8 GetData[11] = {0};
	__u8 SetData[11] = {0};

	char do_vendor_version_get = 0;
	char do_bri_set = 0;
	char do_bri_get = 0;
	int m_bri_val = 0;
	char do_shrp_set = 0;
	char do_shrp_get = 0;
	int m_shrp_val = 0;
	char do_asic_r = 0;
	char do_asic_w = 0;
	unsigned int rAsicAddr = 0x0;
	unsigned char rAsicData = 0x0;
	unsigned int wAsicAddr = 0x0;
	unsigned char wAsicData = 0x0;

	//eho
	char do_ef_set = 0;
	int gainVal = 0;
	int sharpVal = 0;
	int gammaVal = 0;
	int brightVal = 0;
	int vmirrorVal = 0;
	int contrastVal = 0;
	int saturationVal = 0;
	int autogainVal = 0;
	int exposureVal = 0;
	int hueVal = 0;
	
	char do_gain = 0;
	char do_sharp = 0;
	char do_gamma = 0;
	char do_bright= 0;
	char do_vmirror = 0;
	char do_contrast = 0;
	char do_saturation = 0;
	char do_autogain = 0;
	char do_exposure = 0;
	char do_nightthread = 0;
	char do_hue = 0;

	char do_night_thread = 0;

 // chris +
	/* multi-stream */
	unsigned char multi_stream_format = 0;
	unsigned char multi_stream_enable = 0;
	unsigned int multi_stream_width = 0;//1280;
	unsigned int multi_stream_height = 0;//720;
	unsigned int multi_stream_resolution = 0;//(multi_stream_width << 16) | (multi_stream_height);
	unsigned int MS_bitrate = 0;
	unsigned int MS_qp = 0;
	unsigned int MS_H264_mode = 0;
	unsigned int MS_sub_fr = 0;
	unsigned int MS_sub_gop = 0;
	unsigned int streamID_set = 0;
	unsigned int streamID_get = 0;
	char do_multi_stream_set_bitrate = 0;
	char do_multi_stream_get_bitrate = 0;
	char do_multi_stream_set_qp = 0;
	char do_multi_stream_get_qp = 0;
	char do_multi_stream_set_H264Mode = 0;
	char do_multi_stream_get_H264Mode = 0;
	char do_multi_stream_set_sub_fr = 0;
	char do_multi_stream_get_sub_fr = 0;
	char do_multi_stream_set_sub_gop = 0;
	char do_multi_stream_get_sub_gop = 0;    
 // chris -
 //cjc +
	char do_multi_stream_set_type = 0;
	struct Multistream_Info TestInfo;
	char do_multi_stream_get_status = 0;
	char do_multi_stream_get_info = 0;
	char do_multi_stream_set_enable = 0;
	char do_multi_stream_get_enable = 0;
	char do_osd_timer_ctrl_set = 0;
	char do_osd_rtc_set = 0;
	char do_osd_rtc_get = 0;
	char do_osd_size_set = 0;
	char do_osd_size_get = 0;
	char do_osd_color_set = 0;
	char do_osd_color_get = 0;
	char do_osd_show_set = 0;
	char do_osd_show_get = 0;
	char do_osd_autoscale_set = 0;
	char do_osd_autoscale_get = 0;
	char do_osd_ms_size_set = 0;
	char do_osd_ms_size_get = 0;
	char do_osd_position_set = 0;
	char do_osd_position_get = 0;
	char do_osd_ms_position_set = 0;
	char do_osd_ms_position_get = 0;
	char do_md_mode_set = 0;
	char do_md_mode_get = 0;
	char do_md_threshold_set = 0;
	char do_md_threshold_get = 0;
	char do_md_mask_set = 0;
	char do_md_mask_get = 0;
	char do_md_result_set = 0;
	char do_md_result_get = 0;
	char do_mjpg_bitrate_set = 0;
	char do_mjpg_bitrate_get = 0;
	char do_h264_iframe_set = 0;
	char do_h264_sei_set = 0;
	char do_h264_sei_get = 0;
	char do_h264_gop_set = 0;
	char do_h264_gop_get = 0;
	char do_h264_mode_get = 0;
	char do_h264_mode_set = 0;
	char do_img_mirror_set = 0;
	char do_img_mirror_get = 0;
	char do_img_flip_set = 0;
	char do_img_flip_get = 0;
	char do_img_color_set = 0;
	char do_img_color_get = 0;
	char do_osd_string_set = 0;
	char do_osd_string_get = 0;
	char do_osd_carcam_set = 0;
	char do_osd_carcam_get = 0;
	char do_osd_speed_set = 0;
	char do_osd_speed_get = 0;
	char do_osd_coordinate_set = 0;
	char do_osd_coordinate_get = 0;
    char do_gpio_ctrl_set = 0;
    char do_gpio_ctrl_get = 0;
	char do_frame_drop_en_set = 0;
	char do_frame_drop_en_get = 0;
	char do_frame_drop_ctrl_set = 0;
	char do_frame_drop_ctrl_get = 0;	
	unsigned char osd_timer_count = 0;
	unsigned int osd_rtc_year = 0;
	unsigned char osd_rtc_month = 0;
	unsigned char osd_rtc_day = 0;
	unsigned char osd_rtc_hour = 0;
	unsigned char osd_rtc_minute = 0;
	unsigned char osd_rtc_second = 0;
	unsigned char osd_size_line = 0;
	unsigned char osd_size_block = 0;
	unsigned char osd_color_font = 0;
	unsigned char osd_color_border = 0;
	unsigned char osd_show_line = 0;
	unsigned char osd_show_block = 0;
	unsigned char osd_autoscale_line = 0;
	unsigned char osd_autoscale_block = 0;
	unsigned char osd_ms_size_stream0 = 0;
	unsigned char osd_ms_size_stream1 = 0;
	unsigned char osd_ms_size_stream2 = 0;
	unsigned char osd_type = 0;
	unsigned int osd_start_row = 0;
	unsigned int osd_start_col = 0;
	unsigned char osd_ms_position_streamid = 0;
	unsigned char osd_ms_start_row = 0;
	unsigned char osd_ms_start_col = 0;
	unsigned char osd_ms_s0_start_row = 0;
	unsigned char osd_ms_s0_start_col = 0;
	unsigned char osd_ms_s1_start_row = 0;
	unsigned char osd_ms_s1_start_col = 0;
	unsigned char osd_ms_s2_start_row = 0;
	unsigned char osd_ms_s2_start_col = 0;
	unsigned int osd_line_start_row = 0;
	unsigned int osd_line_start_col = 0;
	unsigned int osd_block_start_row = 0;
	unsigned int osd_block_start_col = 0;	
	unsigned char md_mode = 0;
	unsigned int md_threshold = 0;
	unsigned char md_mask[24] = {0};
	unsigned char md_result[24] = {0};
	unsigned int mjpg_bitrate = 0;
	unsigned char h264_sei_en = 0;
	unsigned int h264_gop = 0;
	unsigned int h264_mode = 0;
	unsigned char img_mirror = 0;
	unsigned char img_flip = 0;
	unsigned char img_color = 0;
	unsigned char h264_iframe_reset = 0;
	unsigned char osd_2nd_string_group = 0;
	unsigned char osd_speed_en = 0;
	unsigned char osd_coordinate_en = 0;
	unsigned char osd_coordinate_ctrl = 0;
	unsigned int osd_speed = 0;
	unsigned char osd_coordinate_direction = 0;
	unsigned char osd_direction_value[6] = {0};
	unsigned char osd_direction_value1 = 0;
	unsigned long osd_direction_value2 = 0;
	unsigned char osd_direction_value3 = 0;
	unsigned long osd_direction_value4 = 0;
    unsigned char gpio_ctrl_en = 0;
    unsigned char gpio_ctrl_output_value = 0;
    unsigned char gpio_ctrl_input_value = 0;
	unsigned char stream1_frame_drop_en = 0;
	unsigned char stream2_frame_drop_en = 0;
	unsigned char stream1_frame_drop_ctrl = 0;
	unsigned char stream2_frame_drop_ctrl = 0;
	char osd_string[12] = {"0"};
	pthread_t thread_capture_id;
	pthread_t thread_dequeue_id;
	pthread_t thread_top_dequeue_id;
	pthread_t thread_count_top_frame_dequeue_id;
	pthread_t thread_count_bottom_frame_dequeue_id;
	pthread_t thread_del_tmp_emfile_id;
	//pthread_t thread_switch_id;
	//pthread_t thread_nightMode_id;
	int flytmpcnt = 0,rc;
	char devName[256];
	char time_buf[100] = {0};
	char tmpCMD[100] = {0};
	unsigned char isExceed = 0;
	unsigned long totalSize = 0;
	unsigned char tryopencnt = 20;

	
 //cjc -
#if(CARCAM_PROJECT == 1)
	printf("%s   ******  for Carcam  ******\n",TESTAP_VERSION);
#else
	printf("%s\n",TESTAP_VERSION);	
#endif

	opterr = 0;
	while ((c = getopt_long(argc, argv, "b:c::d:f:hi:ln:s:Srae", opts, NULL)) != -1) {
		
		TestAp_Printf(TESTAP_DBG_FLOW, "optind:%d  optopt:%d\n",optind,optopt);

		switch (c) {
		case 'b':
			if (optarg)
				cam_id = atoi(optarg);
			break;
		case 'c':
			do_capture = 1;
			if (optarg)
				nframes = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'f':
			if (strcmp(optarg, "mjpg") == 0)
				pixelformat = V4L2_PIX_FMT_MJPEG;
			else if (strcmp(optarg, "yuyv") == 0)
				pixelformat = V4L2_PIX_FMT_YUYV;
			else if (strcmp(optarg, "MPEG") == 0)
				pixelformat = V4L2_PIX_FMT_MPEG;
			else if (strcmp(optarg, "H264") == 0)
				pixelformat = V4L2_PIX_FMT_H264;
			else if (strcmp(optarg, "MP2T") == 0)
				pixelformat = V4L2_PIX_FMT_MP2T;
			else {
				lidbg( "Unsupported video format '%s'\n", optarg);
				return 1;
			}
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		case 'i':
			do_set_input = 1;
			input = atoi(optarg);
			break;
		case 'l':
			do_list_controls = 1;
			break;
		case 'n':
			nbufs = atoi(optarg);
			if (nbufs > V4L_BUFFERS_MAX)
				nbufs = V4L_BUFFERS_MAX;
			break;
		case 's':
			width = strtol(optarg, &endptr, 10);
			if (*endptr != 'x' || endptr == optarg) {
				lidbg( "Invalid size '%s'\n", optarg);
				return 1;
			}
			height = strtol(endptr + 1, &endptr, 10);
			if (*endptr != 0) {
				lidbg( "Invalid size '%s'\n", optarg);
				return 1;
			}
			break;
		case 'S':
			do_save = 1;
			break;
		case OPT_STILL_IMAGE:
			do_get_still_image = 1;
			break;
		case OPT_FRAMERATE:
			framerate = strtol(optarg, &endptr, 10);
			break;
		case OPT_ENUM_INPUTS:
			do_enum_inputs = 1;
			break;
		case OPT_SKIP_FRAMES:
			skip = atoi(optarg);
			break;		
		case 'r':			// record H.264 video sequence
			do_record = 1;
			break;
		// SONiX XU Ctrl +++++

		case 'a':
			do_add_xu_ctrl = 1;
			break;

        case 'e':
            do_enum_MaxPayloadTransSize = 1;
            break;

		case OPT_XU_GET_CHIPID:
			do_xu_get_chip = 1;
			break;
		case OPT_XU_GET_FMT:
			do_xu_get_fmt = 1;
			break;
		case OPT_XU_SET_FMT:
			
			do_xu_set_fmt = 1;

			cur_H264fmt.FmtId = strtol(optarg, &endptr, 10) - 1;
			if (*endptr != '-' || endptr == optarg) {
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			cur_H264fmt.FrameRateId = strtol(endptr + 1, &endptr, 10) - 1;
			if (*endptr != 0) {
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			
			break;
		case OPT_XU_GET_QP:
			do_xu_get_qp = 1;
			break;
		case OPT_XU_SET_QP:
			do_xu_set_qp = 1;
			m_QP_Val = atoi(optarg);
			break;
		case OPT_XU_GET_BITRATE:
			do_xu_get_br = 1;
			break;
		case OPT_XU_SET_BITRATE:
			do_xu_set_br = 1;
			m_BitRate = atoi(optarg);
			break;

		case OPT_XU_GET:
			do_xu_get = 1;

	// cjc +
			GetXU_ID = strtol(optarg, &endptr, 16);				// Unit ID Number
	// cjc -
			GetCS = strtol(endptr+1, &endptr, 16);				// Control Selector Number
			GetCmdDataNum = strtol(endptr+1, &endptr, 10);		// Command Data Number
			for(i=0; i<GetCmdDataNum; i++)
			{
				GetData[i] = strtol(endptr+1, &endptr, 16);		// Command Data
			}

			break;
			
		case OPT_XU_SET:
			do_xu_set = 1;

	// cjc +
			SetXU_ID = strtol(optarg, &endptr, 16);				// Unit ID Number
	// cjc -
			SetCS = strtol(endptr+1, &endptr, 16);				// Control Selector Number
			SetCmdDataNum = strtol(endptr+1, &endptr, 10);		// Command Data Number
			for(i=0; i<SetCmdDataNum; i++)
			{
				SetData[i] = strtol(endptr+1, &endptr, 16);		// Command Data
			}
				
			break;
		// SONiX XU Ctrl -----

		case OPT_BRIGHTNESS_GET:
			do_bri_get = 1;
			break;

		case OPT_BRIGHTNESS_SET:
			do_bri_set = 1;
			m_bri_val = atoi(optarg);
			break;

		case OPT_SHARPNESS_GET:
			do_shrp_get = 1;
			break;

		case OPT_SHARPNESS_SET:
			do_shrp_set = 1;
			m_shrp_val = atoi(optarg);
			break;
			
		// Houston 2011/10/14 Asic R/W +++
		
		case OPT_ASIC_READ:
			TestAp_Printf(TESTAP_DBG_FLOW, "== Asic Read: input command ==\n");
			do_asic_r = 1;
			
			rAsicAddr = strtol(optarg, &endptr, 16);		// Asic address
			TestAp_Printf(TESTAP_DBG_FLOW, "  Asic Address = 0x%x \n", rAsicAddr);
			break;
			
		case OPT_ASIC_WRITE:
			TestAp_Printf(TESTAP_DBG_FLOW, "== Asic Write: input command ==\n");
			do_asic_w = 1;
			
			wAsicAddr = strtol(optarg, &endptr, 16);		// Asic Address
			wAsicData = strtol(endptr+1, &endptr, 16);		// Asic Data
			TestAp_Printf(TESTAP_DBG_FLOW, "  Asic Address = 0x%x \n", wAsicAddr);
			TestAp_Printf(TESTAP_DBG_FLOW, "  Data         = 0x%x \n", wAsicData);
				
			break;	
		// Houston 2011/10/14 Asic R/W ---	

// cjc +
#if(CARCAM_PROJECT == 0)
		case OPT_MULTI_FORMAT:
			multi_stream_format = strtol(optarg, &endptr, 16);
            tmp = multi_stream_format;
            while(tmp)
            {
                bit_num += (tmp&1);
                tmp>>=1;
            }
			if(bit_num>=2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_multi_stream_set_type = 1;
			break;

		case OPT_MULTI_SET_BITRATE:
			streamID_set = strtol(optarg, &endptr, 10);
			MS_bitrate = strtol(endptr+1, &endptr, 10);

			if(streamID_set > 2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			
			do_multi_stream_set_bitrate = 1;
			break;
			
		case OPT_MULTI_GET_BITRATE:
			streamID_get = strtol(optarg, &endptr, 10);

			if(streamID_get > 2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}

			do_multi_stream_get_bitrate = 1;
			break;

		case OPT_MULTI_SET_QP:
			streamID_set = strtol(optarg, &endptr, 10);
			MS_qp = strtol(endptr+1, &endptr, 10);

			if(streamID_set > 2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			
			do_multi_stream_set_qp = 1;
			break;
			
		case OPT_MULTI_GET_QP:
			streamID_get = strtol(optarg, &endptr, 10);

			if(streamID_get > 2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}

			do_multi_stream_get_qp = 1;
			break;
            
        case OPT_MULTI_SET_H264MODE:
                streamID_set = strtol(optarg, &endptr, 10);
                MS_H264_mode = strtol(endptr+1, &endptr, 10);
            
                if(streamID_set > 2)
                {
                    lidbg( "Invalid arguments '%s'\n", optarg);
                    return 1;
                }
                
                do_multi_stream_set_H264Mode = 1;
                break;
                
        case OPT_MULTI_GET_H264MODE:
                streamID_get = strtol(optarg, &endptr, 10);
            
                if(streamID_get > 2)
                {
                    lidbg( "Invalid arguments '%s'\n", optarg);
                    return 1;
                }
            
                do_multi_stream_get_H264Mode = 1;
                break;

        case OPT_MULTI_SET_SUB_FR:
                MS_sub_fr = strtol(optarg, &endptr, 10);
                do_multi_stream_set_sub_fr = 1;
                break;

        case OPT_MULTI_GET_SUB_FR:
                do_multi_stream_get_sub_fr = 1;
                break;
                
        case OPT_MULTI_SET_SUB_GOP:
                MS_sub_gop= strtol(optarg, &endptr, 10);
                do_multi_stream_set_sub_gop= 1;
                break;
        
        case OPT_MULTI_GET_SUB_GOP:
                do_multi_stream_get_sub_gop= 1;
                break;

		case OPT_MULTI_GET_STATUS:
			do_multi_stream_get_status = 1;
			break;

		case OPT_MULTI_GET_INFO:
			do_multi_stream_get_info = 1;
			break;

		case OPT_MULTI_SET_ENABLE:
			multi_stream_enable = atoi(optarg);
			if((multi_stream_enable != 0)&&(multi_stream_enable != 1)&&(multi_stream_enable != 3))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}

			do_multi_stream_set_enable = 1;
			break;

		case OPT_MULTI_GET_ENABLE:
			do_multi_stream_get_enable = 1;
			break;
#endif			
		case OPT_OSD_TIMER_CTRL_SET:
			osd_timer_count = atoi(optarg);
			if(osd_timer_count > 1)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_timer_ctrl_set = 1;
			break;
			
		case OPT_OSD_RTC_SET:
			osd_rtc_year = strtol(optarg, &endptr, 10);
			osd_rtc_month = strtol(endptr+1, &endptr, 10);
			osd_rtc_day = strtol(endptr+1, &endptr, 10);
			osd_rtc_hour = strtol(endptr+1, &endptr, 10);
			osd_rtc_minute = strtol(endptr+1, &endptr, 10);
			osd_rtc_second = strtol(endptr+1, &endptr, 10);
			if((osd_rtc_year > 9999)||(osd_rtc_month > 12)||(osd_rtc_day > 31)||(osd_rtc_hour > 24)||(osd_rtc_minute > 59)||(osd_rtc_second > 59))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}			
			do_osd_rtc_set = 1;
			break;
			
		case OPT_OSD_RTC_GET:
			do_osd_rtc_get = 1;
			break;
			
		case OPT_OSD_SIZE_SET:
			osd_size_line = strtol(optarg, &endptr, 10);
			osd_size_block = strtol(endptr+1, &endptr, 10);
			if((osd_size_line > 4)||(osd_size_block > 4))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_size_set = 1;
			break;
			
		case OPT_OSD_SIZE_GET:
			do_osd_size_get = 1;
			break;
			
		case OPT_OSD_COLOR_SET:
			osd_color_font = strtol(optarg, &endptr, 10);
			osd_color_border = strtol(endptr+1, &endptr, 10);
			if((osd_color_font > 4)||(osd_color_border > 4))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_color_set = 1;
			break;
			
		case OPT_OSD_COLOR_GET:
			do_osd_color_get = 1;
			break;
			
		case OPT_OSD_SHOW_SET:
			osd_show_line = strtol(optarg, &endptr, 10);
			osd_show_block = strtol(endptr+1, &endptr, 10);
			if((osd_show_line > 1)||(osd_show_block > 1))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_show_set = 1;
			break;
			
		case OPT_OSD_SHOW_GET:
			do_osd_show_get = 1;
			break;
			
		case OPT_OSD_AUTOSCALE_SET:
			osd_autoscale_line = strtol(optarg, &endptr, 10);
			osd_autoscale_block = strtol(endptr+1, &endptr, 10);
			if((osd_autoscale_line > 1)||(osd_autoscale_block > 1))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_autoscale_set = 1;
			break;
			
		case OPT_OSD_AUTOSCALE_GET:
			do_osd_autoscale_get = 1;
			break;
#if(CARCAM_PROJECT == 0)		
		case OPT_OSD_MS_SIZE_SET:
			osd_ms_size_stream0 = strtol(optarg, &endptr, 10);
			osd_ms_size_stream1 = strtol(endptr+1, &endptr, 10);
			osd_ms_size_stream2 = strtol(endptr+1, &endptr, 10);
			if((osd_ms_size_stream0 > 4)||(osd_ms_size_stream1 > 4)||(osd_ms_size_stream2 > 4))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_ms_size_set = 1;
			break;
			
		case OPT_OSD_MS_SIZE_GET:
			do_osd_ms_size_get = 1;
			break;

		case OPT_OSD_STRING_SET:
			osd_2nd_string_group = strtol(optarg, &endptr, 10);
			
			if(osd_2nd_string_group > 2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			
			for(i=0; i<8; i++)
			{
				osd_string[i] = *(endptr+1+i);
			}

			do_osd_string_set = 1;
			break;

		case OPT_OSD_STRING_GET:
			osd_2nd_string_group = strtol(optarg, &endptr, 10);
			
			do_osd_string_get = 1;
			break;
#endif
		case OPT_OSD_POSITION_SET:
			osd_type = strtol(optarg, &endptr, 10);
			osd_start_row = strtol(endptr+1, &endptr, 10);
			osd_start_col = strtol(endptr+1, &endptr, 10);
			if((osd_type <= 0)|(osd_type > 3))
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_osd_position_set = 1;
			break;
			
		case OPT_OSD_POSITION_GET:
			do_osd_position_get = 1;
			break;
#if(CARCAM_PROJECT == 0)			
		case OPT_OSD_MS_POSITION_SET:
			osd_ms_position_streamid = strtol(optarg, &endptr, 10);
			osd_ms_start_row = strtol(endptr+1, &endptr, 10);
			osd_ms_start_col = strtol(endptr+1, &endptr, 10);
			do_osd_ms_position_set = 1;
			break;
			
		case OPT_OSD_MS_POSITION_GET:
			do_osd_ms_position_get = 1;
			break;			
#endif		
		case OPT_MD_MODE_SET:
			md_mode = atoi(optarg);
			if(md_mode > 1)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_md_mode_set = 1;
			break;
			
		case OPT_MD_MODE_GET:
			do_md_mode_get = 1;
			break;
			
		case OPT_MD_THRESHOLD_SET:
			md_threshold = atoi(optarg);
			if(md_threshold > 65535)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}
			do_md_threshold_set = 1;
			break;
			
		case OPT_MD_THRESHOLD_GET:
			do_md_threshold_get = 1;
			break;
			
		case OPT_MD_MASK_SET:
			md_mask[0] = strtol(optarg, &endptr, 16);

			for(i=1; i<24; i++)
			{
				md_mask[i] = strtol(endptr+1, &endptr, 16);
			}
			do_md_mask_set = 1;
			break;
			
		case OPT_MD_MASK_GET:
			do_md_mask_get = 1;
			break;

		case OPT_MD_RESULT_SET:
			md_result[0] = strtol(optarg, &endptr, 16);

			for(i=1; i<24; i++)
			{
				md_result[i] = strtol(endptr+1, &endptr, 16);
			}
			do_md_result_set = 1;
			break;
			
		case OPT_MD_RESULT_GET:
			do_md_result_get = 1;
			break;
			
		case OPT_MJPG_BITRATE_SET:
			mjpg_bitrate = strtol(optarg, &endptr, 10);
			do_mjpg_bitrate_set = 1;
			break;
			
		case OPT_MJPG_BITRATE_GET:
			do_mjpg_bitrate_get = 1;
			break;

		case OPT_H264_IFRAME_SET:
			h264_iframe_reset = atoi(optarg);
			do_h264_iframe_set = 1;
			break;

		case OPT_H264_SEI_SET:
			h264_sei_en = atoi(optarg);
			do_h264_sei_set = 1;
			break;

		case OPT_H264_SEI_GET:
			do_h264_sei_get = 1;
			break;

		case OPT_H264_GOP_SET:
			h264_gop = atoi(optarg);
			if(h264_gop > 4095)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}			
			do_h264_gop_set = 1;
			break;

		case OPT_H264_GOP_GET:
			do_h264_gop_get = 1;
			break;
			
		case OPT_H264_MODE_SET:
			h264_mode = atoi(optarg);
			if(h264_mode<1 || h264_mode>2)
			{
				lidbg( "Invalid arguments '%s'\n", optarg);
				return 1;
			}	
			do_h264_mode_set = 1;
			break;
		
		case OPT_H264_MODE_GET:
			do_h264_mode_get = 1;
			break;

		case OPT_IMG_MIRROR_SET:
			img_mirror = atoi(optarg);
			do_img_mirror_set = 1;
			break;

		case OPT_IMG_MIRROR_GET:
			do_img_mirror_get = 1;
			break;

		case OPT_IMG_FLIP_SET:
			img_flip = atoi(optarg);
			do_img_flip_set = 1;
			break;

		case OPT_IMG_FLIP_GET:
			do_img_flip_get = 1;
			break;

		case OPT_IMG_COLOR_SET:
			img_color = atoi(optarg);
			do_img_color_set = 1;
			break;

		case OPT_IMG_COLOR_GET:
			do_img_color_get = 1;
			break;

        case OPT_GPIO_CTRL_SET:
            gpio_ctrl_en = strtol(optarg, &endptr, 16);
			gpio_ctrl_output_value = strtol(endptr+1, &endptr, 16);
            do_gpio_ctrl_set = 1;
            break;

        case OPT_GPIO_CTRL_GET:
            do_gpio_ctrl_get = 1;
            break;

		case OPT_FRAME_DROP_EN_SET:
			stream1_frame_drop_en = strtol(optarg, &endptr, 10);
			stream2_frame_drop_en = strtol(endptr+1, &endptr, 10);
			do_frame_drop_en_set = 1;
			break;

		case OPT_FRAME_DROP_EN_GET:
			do_frame_drop_en_get = 1;			
			break;			
			
		case OPT_FRAME_DROP_CTRL_SET:
			stream1_frame_drop_ctrl = strtol(optarg, &endptr, 10);
			stream2_frame_drop_ctrl = strtol(endptr+1, &endptr, 10);		
			do_frame_drop_ctrl_set = 1;
			break;

		case OPT_FRAME_DROP_CTRL_GET:
			do_frame_drop_ctrl_get = 1;
			break;	
			
#if(CARCAM_PROJECT == 1)
		case OPT_OSD_CARCAM_SET:
			osd_speed_en = strtol(optarg, &endptr, 10);
			osd_coordinate_en = strtol(endptr+1, &endptr, 10);
			osd_coordinate_ctrl = strtol(endptr+1, &endptr, 10);			
			do_osd_carcam_set = 1;
			break;

		case OPT_OSD_CARCAM_GET:
			do_osd_carcam_get = 1;
			break;

		case OPT_OSD_SPEED_SET:
			osd_speed = atoi(optarg);
			do_osd_speed_set = 1;
			break;

		case OPT_OSD_SPEED_GET:
			do_osd_speed_get = 1;
			break;

		case OPT_OSD_COORDINATE_SET1:
			osd_coordinate_direction = strtol(optarg, &endptr, 10);
			osd_direction_value[0] = strtol(endptr+1, &endptr, 10);
			osd_direction_value[1] = strtol(endptr+1, &endptr, 10);
			osd_direction_value[2] = strtol(endptr+1, &endptr, 10);
			osd_direction_value[3] = strtol(endptr+1, &endptr, 10);
			osd_direction_value[4] = strtol(endptr+1, &endptr, 10);
			osd_direction_value[5] = strtol(endptr+1, &endptr, 10);
			do_osd_coordinate_set = 1;
			break;

		case OPT_OSD_COORDINATE_GET1:
			do_osd_coordinate_get = 1;	
			break;

		case OPT_OSD_COORDINATE_SET2:
			osd_coordinate_direction = strtol(optarg, &endptr, 10);
			osd_direction_value1 = strtol(endptr+1, &endptr, 10);
			osd_direction_value2 = strtol(endptr+1, &endptr, 10);
			osd_direction_value3 = strtol(endptr+1, &endptr, 10);
			osd_direction_value4 = strtol(endptr+1, &endptr, 10);
			do_osd_coordinate_set = 2;
			break;

		case OPT_OSD_COORDINATE_GET2:
			do_osd_coordinate_get = 2;	
			break;
#endif
// cjc -
		case OPT_DEBUG_LEVEL:
			Dbg_Param = strtol(optarg, &endptr, 16);
			break;
		case OPT_VENDOR_VERSION_GET:
			do_vendor_version_get = 1;
			break;
		case OPT_EFFECT_SET:
			do_ef_set = 1;
			lidbg("OPT_EFFECT_SET111=----E--");
			lidbg("optarg = %s",optarg);
			char *keyval[2] = {NULL};//key-vals
			lidbg_token_string(optarg, "=", keyval) ;
			if (strcmp(keyval[0], "gain") == 0)
			{
				do_gain = 1;
				gainVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "sharp") == 0)
			{
				do_sharp = 1;
				sharpVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "gamma") == 0)
			{
				do_gamma = 1;
				gammaVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "bright") == 0)
			{
				do_bright = 1;
				lidbg("eho------bright2 %s , %s",keyval[0],keyval[1]);
				brightVal = strtol(keyval[1], &endptr, 10);
				lidbg("eho------bright3");
			}	
			else if (strcmp(keyval[0], "vmirror") == 0)
			{
				do_vmirror = 1;
				vmirrorVal = strtol(keyval[1], &endptr, 10);
			}
			else if (strcmp(keyval[0], "contrast") == 0)
			{
				do_contrast = 1;
				contrastVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "saturation") == 0)
			{
				do_saturation = 1;
				saturationVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "autogain") == 0)
			{
				do_autogain = 1;
				autogainVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "exposure") == 0)
			{
				do_exposure = 1;
				exposureVal = strtol(keyval[1], &endptr, 10);
			}	
			else if (strcmp(keyval[0], "hue") == 0)
			{
				do_hue = 1;
				hueVal = strtol(keyval[1], &endptr, 10);
			}
			else if (strcmp(keyval[0], "nightthread") == 0)
			{
				do_nightthread = 1;
			}	
			lidbg("OPT_EFFECT_SET=----X--");
			break;
		default:
			lidbg( "Invalid option -%c\n", c);
			lidbg( "Run %s -h for help.\n", argv[0]);
			return 1;
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
		return 1;
	}

    //yiling ++
    if(do_enum_MaxPayloadTransSize)
    {
        Enum_MaxPayloadTransSize(argv[optind]);
        return 1;      
    }
    //yiling --

	flycam_fd = open("/dev/lidbg_flycam0", O_RDWR);
	if((flycam_fd == 0xfffffffe) || (flycam_fd == 0) || (flycam_fd == 0xffffffff))
	{
	    lidbg("open lidbg_flycam0 fail\n");
		close(flycam_fd);
	    return 0;
	}


	/*
		ACCON Block mode
		W:	return [0-OK;1-Timout]	W[0-Rear;1-DVR] -> Wait for 10s
	*/
	if((cam_id == REAR_BLOCK_ID_MODE) || (cam_id == DVR_BLOCK_ID_MODE))
	{
		int ret_acc;
		lidbg("********ACC FIRST RESUME*********\n");
		ret_acc = ioctl(flycam_fd,_IO(FLYCAM_STATUS_IOC_MAGIC, NR_ACCON_CAM_READY), cam_id -2);
		lidbg("********ACC ret => %d*********\n",ret_acc);
		if (ret_acc != 0)
	    {
	      	lidbg("%s:===Wait ACCON Camera PLUG IN Timeout!!End Wait proc!====\n",__func__);
			close(flycam_fd);
			return 0;
		}	
		cam_id -= 2;
	}

	/*Query Camera supportResolutions */
	if(cam_id == DVR_GET_RES_ID_MODE || cam_id == REAR_GET_RES_ID_MODE)
	{
		cam_id -= 4;
		lidbg("********GET RESOULUTION :cam_id => %d*********\n",cam_id);
		
		rc = lidbg_get_hub_uvc_device(RECORD_MODE,devName,cam_id,0);
	    if((rc == -1)  || (*devName == '\0'))
	    {
	        lidbg("%s: No UVC node found \n", __func__);
			//return 1;
			goto try_open_again; 
	    }
			
		dev = video_open(devName);
	
		if (dev < 0)
			return 1;
		
		struct v4l2_frmsizeenum	res_frmsize;
		int res_i = 0;
		char res_all[100] = {0};
		char res_final[100] = {0};
		char res_each[10] = {0};
		for(res_i = 0; ; res_i++)
        {
        	res_frmsize.pixel_format = V4L2_PIX_FMT_YUYV;
            res_frmsize.index = res_i;
            if (ioctl(dev, VIDIOC_ENUM_FRAMESIZES , &res_frmsize) < 0)
            {
                  //lidbg("%s: VIDIOC_ENUM_FRAMESIZES failed--%d\n", __func__, res_i);
				  break;
            }
			if (res_frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
			{
				  //lidbg("%s: RES => %dx%d\n", __func__, res_frmsize.discrete.width, res_frmsize.discrete.height);
				  sprintf(res_each , "%dx%d,",res_frmsize.discrete.width, res_frmsize.discrete.height);
				  strcat(res_all,res_each);
			}
        }
		 strncpy(res_final ,res_all ,strlen(res_all) - 1);
		 lidbg("%s: RESALL => %s \n", __func__, res_final);
		 if(cam_id == DVR_ID)
			send_driver_msg(FLYCAM_STATUS_IOC_MAGIC,NR_DVR_RES,(unsigned long)res_final);
		else if(cam_id == REARVIEW_ID)
			send_driver_msg(FLYCAM_STATUS_IOC_MAGIC,NR_REAR_RES,(unsigned long)res_final);
		 close(dev);
		 close(flycam_fd);
		 return 0;
	}

	if(cam_id == SET_DVR_OSD_ID_MODE)
	{
		lidbg("%s: SET_OSD_ID_MODE \n", __func__);
		get_driver_prop(DVR_ID);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_TIME), Rec_Sec);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_TOTALSIZE), Rec_File_Size);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_PATH), Rec_Save_Dir);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_ISDUALCAM), isDualCam);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_ISCOLDBOOTREC), isColdBootRec);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_ISEMPERMITTED), isEmPermit);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_ISVIDEOLOOP), isVideoLoop);
		ioctl(flycam_fd,_IO(FLYCAM_FRONT_REC_IOC_MAGIC, NR_DELDAYS), delDays);
		osd_set(DVR_ID);//loop
		return 0;
	}
	else if(cam_id == SET_REAR_OSD_ID_MODE)
	{
		lidbg("%s: SET_REAR_OSD_ID_MODE \n", __func__);
		get_driver_prop(REARVIEW_ID);
		ioctl(flycam_fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_TIME), Rec_Sec);
		ioctl(flycam_fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_TOTALSIZE), Rec_File_Size);
		ioctl(flycam_fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_PATH), Rec_Save_Dir);
		ioctl(flycam_fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_ISVIDEOLOOP), isVideoLoop);
		osd_set(REARVIEW_ID);//loop
		return 0;
	}
	
	cam_list_init(&mhead.list);

	/* Open the video device. */
	//dev = video_open(argv[optind]);
getuvcdevice:
	/*auto find camera device*/
	//rc = get_hub_uvc_device(devName,do_save,do_record);
	lidbg("************cam_id -> %d************\n",cam_id);
	if(cam_id == -1)	cam_id = DVR_ID;
	if(pixelformat == V4L2_PIX_FMT_MJPEG)
		rc = lidbg_get_hub_uvc_device(RECORD_MODE,devName,cam_id,0);
	else
		rc = lidbg_get_hub_uvc_device(RECORD_MODE,devName,cam_id,1);
    if((rc == -1)  || (*devName == '\0'))
    {
        lidbg("%s: No UVC node found \n", __func__);
		//return 1;
		goto try_open_again; 
    }
openfd:
	dev = video_open(devName);
	
	if (dev < 0)
		return 1;

	send_driver_msg(FLYCAM_STATUS_IOC_MAGIC,NR_STATUS,RET_DVR_START);
	
	if((do_save) || (do_record)) 
		get_driver_prop(cam_id);
	
	/*enable OSD*/
	if(XU_OSD_Set_Enable(dev, 1, 1) <0)
			lidbg( "XU_OSD_Set_Enable Failed\n");	
	if(XU_OSD_Set_CarcamCtrl(dev, 0, 0, 0) < 0)
			lidbg( "XU_OSD_Set_CarcamCtrl Failed\n");	

	/*set OSD time*/
	lidbg_get_current_time(1, time_buf, NULL);
	
	if(do_vendor_version_get)
	{
	 	char vendor_version[12];
		get_vendor_verson(dev, vendor_version);
		TestAp_Printf(TESTAP_DBG_FLOW, "Vendor Version : %s\n",vendor_version);
		
	}

	if(do_ef_set)
	{
		lidbg("do_ef_set=----E--");
		if (do_gain)
		{
			if (v4l2SetControl (dev, V4L2_CID_GAIN, gainVal)<0)
			lidbg("----eho---- : do_gain (%d) Failed", gainVal);
		}	
		else if (do_sharp)
		{
			if (v4l2SetControl (dev, V4L2_CID_SHARPNESS, sharpVal)<0)
			lidbg("----eho---- : do_sharp (%d) Failed", sharpVal);
		}	
		else if (do_gamma)
		{
			if (v4l2SetControl (dev, V4L2_CID_GAMMA, gammaVal)<0)
			lidbg("----eho---- : do_gamma (%d) Failed", gammaVal);
		}	
		else if (do_bright)
		{
			if (v4l2SetControl (dev, V4L2_CID_BRIGHTNESS, brightVal - 64)<0)
			lidbg("----eho---- : do_bright (%d) Failed", brightVal);
		}	
		else if (do_vmirror)
		{
			//if (v4l2SetControl (dev, V4L2_CID_VFLIP, vmirrorVal)<0)
			char  temp_devname[256];
			sprintf(temp_devname, "./flysystem/lib/out/lidbg_testuvccam /dev/video0 --xuset-flip %d", vmirrorVal);
			system(temp_devname);
			lidbg("----eho---- : do_vmirror (%d) ", vmirrorVal);
		}	
		else if (do_contrast)
		{
			if (v4l2SetControl (dev, V4L2_CID_CONTRAST, contrastVal)<0)
			lidbg("----eho---- : do_contrast (%d) Failed", contrastVal);
		}	
		else if (do_saturation)
		{
			if (v4l2SetControl (dev, V4L2_CID_SATURATION, saturationVal)<0)
			lidbg("----eho---- : do_saturation (%d) Failed", saturationVal);
		}	
		else if (do_autogain)
		{
			if (v4l2SetControl (dev, V4L2_CID_EXPOSURE_AUTO, autogainVal)<0)
			lidbg("----eho---- : do_autogain (%d) Failed", autogainVal);
		}	
		else if (do_exposure)
		{
			//if (v4l2SetControl (dev, V4L2_CID_EXPOSURE, exposureVal)<0)
			//lidbg("----eho---- : do_exposure (%d) Failed", exposureVal);
			if (v4l2SetControl (dev, V4L2_CID_EXPOSURE_ABSOLUTE, V4L2_EXPOSURE_AUTO)<0)
			lidbg("----eho---- : do_exposure (%d) Failed", exposureVal);
		}
		else if (do_hue)
		{
			if (v4l2SetControl (dev, V4L2_CID_HUE, hueVal -40)<0)
			lidbg("----eho---- : do_hue (%d) Failed", hueVal);
		}
		#if 0
		else if (do_nightthread)
		{
			lidbg("nightthread create ----E----");
			ret = pthread_create(&thread_nightMode_id,NULL,thread_nightmode,NULL);
			if(ret != 0)
			{
				lidbg( "-----eho-----nightthread pthread error!\n");
				return 1;
			}
		}
		#endif
		lidbg("do_ef_set=----X--");
		return 0;
	}
	
	// SONiX XU Ctrl ++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// cjc +
	ret = XU_Ctrl_ReadChipID(dev);
	if(ret<0)
		lidbg( "SONiX_UVC_TestAP @main : XU_Ctrl_ReadChipID Failed\n");
	// cjc -

	/* Add XU ctrls */
	if(do_add_xu_ctrl)
	{
		ret = XU_Init_Ctrl(dev);
		if(ret<0)
		{
			if(ret == -EEXIST)
			{
				lidbg( "SONiX_UVC_TestAP @main : Initial XU Ctrls");
			}
			else
			{
				lidbg( "SONiX_UVC_TestAP @main : Initial XU Ctrls Failed (%i)\n",ret);
			}
			//lidbg( "SONiX_UVC_TestAP @main : ");//Initial XU Ctrls ignored, uvc driver had already supported\n");//\t\t\t No need to Add Extension Unit Ctrls into Driver\n");

		}
	}

	if (do_list_controls)
		video_list_controls(dev);
	
	if(do_xu_get_chip)
	{
		ret = XU_Ctrl_ReadChipID(dev);
		if(ret<0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Ctrl_ReadChipID Failed\n");
	}

	if(do_xu_get_fmt)
	{
		if(!H264_GetFormat(dev))
			lidbg( "SONiX_UVC_TestAP @main : H264 Get Format Failed\n");
	}

	if(do_xu_set_fmt)
	{
		if(XU_H264_SetFormat(dev, cur_H264fmt) < 0)
			lidbg( "SONiX_UVC_TestAP @main : H264 Set Format Failed\n");
	}
	
	//yiling ++ bit rate can only work in CBR mode, QP value can only work in VBR mode
	if(do_h264_mode_get)
	{
		XU_H264_Get_Mode(dev, &h264_mode);
	}

	if(do_h264_mode_set)
	{
		XU_H264_Set_Mode(dev, h264_mode);
	}
	//yiling --

	if(do_xu_set_qp)
	{
		if(XU_H264_Set_QP(dev, m_QP_Val) < 0 )
			lidbg( "SONiX_UVC_TestAP @main : XU_H264_Set_QP Failed\n");
	}

	if(do_xu_get_qp)
	{
		if(XU_H264_Get_QP(dev, &m_QP_Val))
			lidbg( "SONiX_UVC_TestAP @main : XU_H264_Get_QP Failed\n");
	}

	if(do_xu_get_br)
	{
		XU_H264_Get_BitRate(dev, &m_BitRate);
		if(m_BitRate < 0 )
			lidbg( "SONiX_UVC_TestAP @main : XU_H264_Get_BitRate Failed\n");
		TestAp_Printf(TESTAP_DBG_FLOW, "Current bit rate: %.2f Kbps\n",m_BitRate);
	}

	if(do_xu_set_br)
	{
		if(XU_H264_Set_BitRate(dev, m_BitRate) < 0 )
			lidbg( "SONiX_UVC_TestAP @main : XU_H264_Set_BitRate Failed\n");
	}

	if(do_xu_set)
	{
		TestAp_Printf(TESTAP_DBG_FLOW, "== XU Set: input command ==\n");		
		TestAp_Printf(TESTAP_DBG_FLOW, "  XU ID = 0x%x \n", SetXU_ID);
		TestAp_Printf(TESTAP_DBG_FLOW, "  Control Selector = 0x%x \n", SetCS);
		TestAp_Printf(TESTAP_DBG_FLOW, "  Cmd Data Number  = %d \n", SetCmdDataNum);
			
		if(XU_Set_Cur(dev, SetXU_ID, SetCS, SetCmdDataNum, SetData) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Set Failed\n");
	}

	if(do_xu_get)
	{

		TestAp_Printf(TESTAP_DBG_FLOW, "== XU Get: input command ==\n");
		TestAp_Printf(TESTAP_DBG_FLOW, "  XU ID = 0x%x \n", GetXU_ID);
		TestAp_Printf(TESTAP_DBG_FLOW, "  Control Selector = 0x%x \n", GetCS);
		TestAp_Printf(TESTAP_DBG_FLOW, "  Cmd Data Number  = %d \n", GetCmdDataNum);
		for(i=0; i<GetCmdDataNum; i++)
			TestAp_Printf(TESTAP_DBG_FLOW, "  Cmd Data[%d] = 0x%x\n", i, GetData[i]);		

		if(XU_Get_Cur(dev, GetXU_ID, GetCS, GetCmdDataNum, GetData) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Get Failed\n");
	}

// cjc +	
	if(do_multi_stream_set_type)
	{
	    struct Multistream_Info Multi_Info;
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}

        XU_Multi_Get_Info(dev, &Multi_Info);
        if(multi_stream_format && (Multi_Info.format&multi_stream_format) == 0)
        {
            lidbg( "SONiX_UVC_TestAP @main : Multistream format doesn't support(%x)\n", multi_stream_format);
            return 1;
        }


		// Set multi stream format to device
		if(XU_Multi_Set_Type(dev, multi_stream_format) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_Type Failed\n");
	}

	if(do_multi_stream_set_bitrate)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Set multi stream bitRate
		if(XU_Multi_Set_BitRate(dev, streamID_set, MS_bitrate) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_Bitrate Failed\n");
	}
	
	if(do_multi_stream_get_bitrate)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream bitRate
		if(XU_Multi_Get_BitRate(dev, streamID_get, &MS_bitrate) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_Bitrate Failed\n");
	}

	if(do_multi_stream_set_qp)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Set multi stream QP
		if(XU_Multi_Set_QP(dev, streamID_set, MS_qp) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_QP Failed\n");
	}
	
	if(do_multi_stream_get_qp)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream QP
		if(XU_Multi_Get_QP(dev, streamID_get, &MS_qp) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_QP Failed\n");
	}
    
	if(do_multi_stream_set_H264Mode)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Set multi stream H264 Mode
		if(XU_Multi_Set_H264Mode(dev, streamID_set, MS_H264_mode) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_H264Mode Failed\n");
	}
	
	if(do_multi_stream_get_H264Mode)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream H264 Mode
		if(XU_Multi_Get_H264Mode(dev, streamID_get, &MS_H264_mode) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_H264Mode Failed\n");
	}

	if(do_multi_stream_set_sub_gop)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Set multi stream substream gop
		if(XU_Multi_Set_SubStream_GOP(dev, MS_sub_gop) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_SubStream_GOP Failed\n");
	}
	
	if(do_multi_stream_get_sub_gop)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream substream gop
		if(XU_Multi_Get_SubStream_GOP(dev, &MS_sub_gop) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_SubStream_GOP Failed\n");
	}


	if(do_multi_stream_get_status)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream status
		if(XU_Multi_Get_status(dev, &TestInfo) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_status Failed\n");
	}

	if(do_multi_stream_get_info)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream info
		if(XU_Multi_Get_Info(dev, &TestInfo) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_Info Failed\n");
	}

	if(do_multi_stream_get_enable)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream enable
		if(XU_Multi_Get_Enable(dev, &multi_stream_enable) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_Enable Failed\n");
	}
	
	if(do_multi_stream_set_enable)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Set multi stream enable
		if(XU_Multi_Set_Enable(dev, multi_stream_enable) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_Enable Failed\n");
	}
	else
	{
		// Set multi stream disable
		//if(XU_Multi_Set_Enable(dev, 0) < 0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_Enable Failed\n");
		if(XU_Multi_Get_Enable(dev, &multi_stream_enable) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_Enable Failed\n");			
	}
	
	if(do_osd_timer_ctrl_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Timer_Ctrl(dev, osd_timer_count) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Timer_Ctrl Failed\n");
	}

	if(do_osd_rtc_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		lidbg_get_current_time(1,time_buf, NULL);
		//if(XU_OSD_Set_RTC(dev, osd_rtc_year, osd_rtc_month, osd_rtc_day, osd_rtc_hour, osd_rtc_minute, osd_rtc_second) <0)
		//	lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_RTC Failed\n");
	}

	if(do_osd_rtc_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_RTC(dev, &osd_rtc_year, &osd_rtc_month, &osd_rtc_day, &osd_rtc_hour, &osd_rtc_minute, &osd_rtc_second) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_RTC Failed\n");
	}

	if(do_osd_size_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Set_Size(dev, osd_size_line, osd_size_block) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Size Failed\n");
	}

	if(do_osd_size_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_Size(dev, &osd_size_line, &osd_size_block) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Size Failed\n");
	}

	if(do_osd_color_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Set_Color(dev, osd_color_font, osd_color_border) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Color Failed\n");
	}

	if(do_osd_color_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_Color(dev, &osd_color_font, &osd_color_border) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Color Failed\n");
	}

	if(do_osd_show_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Set_Enable(dev, osd_show_line, osd_show_block) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Enable Failed\n");	
	}

	if(do_osd_show_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_Enable(dev, &osd_show_line, &osd_show_block) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Enable Failed\n");
	}

	if(do_osd_autoscale_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Set_AutoScale(dev, osd_autoscale_line, osd_autoscale_block) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_AutoScale Failed\n");
	}

	if(do_osd_autoscale_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_AutoScale(dev, &osd_autoscale_line, &osd_autoscale_block) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_AutoScale Failed\n");
	}

	if(do_osd_ms_size_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Set_Multi_Size(dev, osd_ms_size_stream0, osd_ms_size_stream1, osd_ms_size_stream2) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Multi_Size Failed\n");
	}

	if(do_osd_ms_size_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_Multi_Size(dev, &osd_ms_size_stream0, &osd_ms_size_stream1, &osd_ms_size_stream2) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Multi_Size Failed\n");
	}

	if(do_osd_position_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Set_Start_Position(dev, osd_type, osd_start_row, osd_start_col) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Start_Position Failed\n");
	}

	if(do_osd_position_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_Start_Position(dev, &osd_line_start_row, &osd_line_start_col, &osd_block_start_row, &osd_block_start_col) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Start_Position Failed\n");
	}

	if((do_osd_ms_position_set)&&(!do_capture))
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}

		if(XU_OSD_Set_MS_Start_Position(dev, osd_ms_position_streamid, osd_ms_start_row, osd_ms_start_col) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Start_Position Failed\n");
	}

	if(do_osd_ms_position_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_OSD_Get_MS_Start_Position(dev, &osd_ms_s0_start_row, &osd_ms_s0_start_col, &osd_ms_s1_start_row, &osd_ms_s1_start_col, &osd_ms_s2_start_row, &osd_ms_s2_start_col) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Start_Position Failed\n");
	}

	if(do_md_mode_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Set_Mode(dev, md_mode) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Set_Mode Failed\n");
	}
	
	if(do_md_mode_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Get_Mode(dev, &md_mode) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Get_Mode Failed\n");
	}

	if(do_md_threshold_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Set_Threshold(dev, md_threshold) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Set_Threshold Failed\n");
	}

	if(do_md_threshold_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Get_Threshold(dev, &md_threshold) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Get_Threshold Failed\n");
	}

	if(do_md_mask_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Set_Mask(dev, md_mask) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Set_Mask Failed\n");
	}

	if(do_md_mask_get)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Get_Mask(dev, md_mask) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Get_Mask Failed\n");
	}

	if(do_md_result_set)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		if(XU_MD_Set_RESULT(dev, md_mask) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MD_Set_RESULT Failed\n");
	}

	if(do_mjpg_bitrate_set)
	{
		if(XU_MJPG_Set_Bitrate(dev, mjpg_bitrate) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MJPG_Set_Bitrate Failed\n");
	}

	if(do_mjpg_bitrate_get)
	{
		if(XU_MJPG_Get_Bitrate(dev, &mjpg_bitrate) <0)
			lidbg( "SONiX_UVC_TestAP @main : XU_MJPG_Get_Bitrate Failed\n");
	}
	
	if(do_h264_sei_set)
	{
		XU_H264_Set_SEI(dev, h264_sei_en);
	}

	if(do_h264_sei_get)
	{
		XU_H264_Get_SEI(dev, &h264_sei_en);
	}
	
	if(do_h264_gop_set)
	{
		XU_H264_Set_GOP(dev, h264_gop);
	}

	if(do_h264_gop_get)
	{
		XU_H264_Get_GOP(dev, &h264_gop);
	}	


	if(do_img_mirror_set)
	{
		XU_IMG_Set_Mirror(dev, img_mirror);
	}

	if(do_img_mirror_get)
	{
		XU_IMG_Get_Mirror(dev, &img_mirror);
	}
	
	if(do_img_flip_set)
	{
		XU_IMG_Set_Flip(dev, img_flip);
	}

	if(do_img_flip_get)
	{
		XU_IMG_Get_Flip(dev, &img_flip);
	}
	
	if(do_img_color_set)
	{
		XU_IMG_Set_Color(dev, img_color);
	}

	if(do_img_color_get)
	{
		XU_IMG_Get_Color(dev, &img_color);
	}

	if(do_osd_string_set)
	{
		XU_OSD_Set_String(dev, osd_2nd_string_group, osd_string);
	}
	
	if(do_osd_string_get)
	{
		XU_OSD_Get_String(dev, osd_2nd_string_group, osd_string);
	}

#if(CARCAM_PROJECT == 1)	
	if(do_osd_carcam_set)
	{
		XU_OSD_Set_CarcamCtrl(dev, osd_speed_en, osd_coordinate_en, osd_coordinate_ctrl);
	}

	if(do_osd_carcam_get)
	{
		XU_OSD_Get_CarcamCtrl(dev, &osd_speed_en, &osd_coordinate_en, &osd_coordinate_ctrl);
	}

	if(do_osd_speed_set)
	{
		XU_OSD_Set_Speed(dev, osd_speed);
	}

	if(do_osd_speed_get)
	{
		XU_OSD_Get_Speed(dev, &osd_speed);
	}

	if(do_osd_coordinate_set == 1)
	{
		XU_OSD_Set_Coordinate1(dev, osd_coordinate_direction, osd_direction_value);
	}

	if(do_osd_coordinate_set == 2)
	{
		XU_OSD_Set_Coordinate2(dev, osd_coordinate_direction, osd_direction_value1, osd_direction_value2, osd_direction_value3, osd_direction_value4);
	}
	
	if(do_osd_coordinate_get == 1)
	{
		XU_OSD_Get_Coordinate1(dev, &osd_coordinate_direction, osd_direction_value);
	}

	if(do_osd_coordinate_get == 2)
	{
		XU_OSD_Get_Coordinate2(dev, &osd_coordinate_direction, &osd_direction_value1, &osd_direction_value2, &osd_direction_value3, &osd_direction_value4);
	}
#endif

    if(do_gpio_ctrl_set == 1)
    {
        XU_GPIO_Ctrl_Set(dev, gpio_ctrl_en, gpio_ctrl_output_value);
    }

    if(do_gpio_ctrl_get == 1)
    {
        XU_GPIO_Ctrl_Get(dev, &gpio_ctrl_en, &gpio_ctrl_output_value, &gpio_ctrl_input_value);
    }
    
	if(do_frame_drop_en_set == 1)
	{
		XU_Frame_Drop_En_Set(dev, stream1_frame_drop_en, stream2_frame_drop_en);
	}
	
	if(do_frame_drop_en_get == 1)
	{
		XU_Frame_Drop_En_Get(dev, &stream1_frame_drop_en, &stream2_frame_drop_en);
	}

	if(do_frame_drop_ctrl_set == 1)
	{
		XU_Frame_Drop_Ctrl_Set(dev, stream1_frame_drop_ctrl, stream2_frame_drop_ctrl);
	}
	
	if(do_frame_drop_ctrl_get == 1)
	{
		XU_Frame_Drop_Ctrl_Get(dev, &stream1_frame_drop_ctrl, &stream2_frame_drop_ctrl);
	}	
// cjc -
	
	// SONiX XU Ctrl -------------------------------------------------------	

	// Standard UVC image properties setting +++++++++++++++++++++++++++++++
	if (do_bri_set) 
	{
	    if (v4l2SetControl (dev, V4L2_CID_BRIGHTNESS, m_bri_val)<0)
		lidbg( "SONiX_UVC_TestAP @main : Set Brightness (%d) Failed\n", m_bri_val);
	} 

	if (do_bri_get) 
	{
		m_bri_val = v4l2GetControl (dev, V4L2_CID_BRIGHTNESS);
		if(m_bri_val < 0)
			lidbg( "SONiX_UVC_TestAP @main : Get Brightness Failed\n");
		TestAp_Printf(TESTAP_DBG_FLOW, "SONiX_UVC_TestAP @main : Get Brightness (%d)\n", m_bri_val);
	}

	if (do_shrp_set) 
	{
	    if (v4l2SetControl (dev, V4L2_CID_SHARPNESS, m_shrp_val)<0)
			lidbg( "SONiX_UVC_TestAP @main : Set Sharpness (%d) Failed\n", m_shrp_val);
	} 

	if (do_shrp_get) 
	{
		m_shrp_val = v4l2GetControl (dev, V4L2_CID_SHARPNESS);
		if(m_shrp_val < 0)
			lidbg( "SONiX_UVC_TestAP @main : Get Sharpness Failed\n");
		TestAp_Printf(TESTAP_DBG_FLOW, "SONiX_UVC_TestAP @main : Get Sharpness (%d)\n", m_shrp_val);
	}
	// Standard UVC image properties setting -------------------------------
	
	// Houston 2011/10/14 Asic R/W +++
	if(do_asic_r)
	{
		if(XU_Asic_Read(dev, rAsicAddr, &rAsicData)<0)		
			lidbg( "SONiX_UVC_TestAP @main : XU_Asic_Read(0x%x) Failed\n", rAsicAddr);
	}

	if(do_asic_w)
	{
		if(XU_Asic_Write(dev, wAsicAddr, wAsicData)<0 )
			lidbg( "SONiX_UVC_TestAP @main : XU_Asic_Write(0x%x) Failed\n",wAsicAddr);
	}
	// Houston 2011/10/14 Asic R/W ---
	

	if (do_enum_inputs)
		video_enum_inputs(dev);

	if (do_set_input)
		video_set_input(dev, input);

	//ret = video_get_input(dev);
	//TestAp_Printf(TESTAP_DBG_FLOW, "Input %d selected\n", ret);

	if (!do_capture) {
		close(dev);	
		close(flycam_fd);
		return 0;
	}

	/* Set the video format. */
    if(multi_stream_enable && (multi_stream_format == MULTI_STREAM_QVGA_VGA))
    {
        width = 640;
        height= 480;
    }
    else if(multi_stream_enable && (multi_stream_format == MULTI_STREAM_360P_180P))
    {
        width = 640;
        height= 360;
    }
	if (video_set_format(dev, width, height, pixelformat) < 0) {
// cjc +		
		if(pixelformat == V4L2_PIX_FMT_H264) {
			lidbg(" === Set Format Failed : skip for H264 === ");
		}
		else {
// cjc -
			close(dev);		
			close(flycam_fd);
			return 1;
		}
	}

	/* Set the frame rate. */
	if (video_set_framerate(dev, framerate, NULL) < 0) {
		close(dev);	
		close(flycam_fd);
		return 1;
	}



    //yiling:should get/set substream framerate after video_set_framerate() 
    //because this two functions need to execute video_get_framerate() to get framerate of main stream(sub<=main)
	if(do_multi_stream_set_sub_fr)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Set multi stream substream frame rate
		if(XU_Multi_Set_SubStream_FrameRate(dev, MS_sub_fr) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Set_SubStream_FrameRate Failed\n");
	}
	
	if(do_multi_stream_get_sub_fr)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		// Get multi stream substream frame rate
		if(XU_Multi_Get_SubStream_FrameRate(dev, &MS_sub_fr) < 0)
			lidbg( "SONiX_UVC_TestAP @main : XU_Multi_Get_H264Mode Failed\n");
	}
    //yiling --

	/*Preview: H264 VBR Mode; DVR: CBR 4Mbps*/
	XU_H264_Set_IFRAME(dev);
	if(isPreview)
		XU_H264_Set_Mode(dev, 2);
	else
	{
		XU_H264_Set_Mode(dev, 1);
		if(XU_Ctrl_ReadChipID(dev) < 0)
			lidbg( "XU_Ctrl_ReadChipID Failed\n");
		if(cam_id == DVR_ID)
		{
			if(XU_H264_Set_BitRate(dev, 12000000) < 0 )
				lidbg( "XU_H264_Set_BitRate Failed\n");
			iframe_diff_val = 70000;
			iframe_threshold_val = 45000;
			if(isDelDaysFile)
			{
				lidbg_del_days_file(Rec_Save_Dir, delDays);
				property_set("lidbg.uvccam.isDelDaysFile", "0");
			}
		}
		else if(cam_id == REARVIEW_ID)
		{
			if(XU_H264_Set_BitRate(dev, 2000000) < 0 )
				lidbg( "XU_H264_Set_BitRate Failed\n");
			iframe_diff_val = 35000;
			iframe_threshold_val = 22500;
		}
		XU_H264_Get_BitRate(dev, &m_BitRate);
		if(m_BitRate < 0 )
			lidbg( "SONiX_UVC_TestAP @main : XU_H264_Get_BitRate Failed\n");
		lidbg("Current bit rate1: %.2f Kbps\n",m_BitRate);
	}
 

	//XU_H264_Set_GOP(dev, 10);
	property_set("lidbg.uvccam.isdequeue", "0");
	
	pthread_create(&thread_top_dequeue_id,NULL,thread_top_dequeue,NULL);
	pthread_create(&thread_dequeue_id,NULL,thread_dequeue,NULL);
	pthread_create(&thread_count_top_frame_dequeue_id,NULL,thread_top_count_frame,NULL);
	pthread_create(&thread_count_bottom_frame_dequeue_id,NULL,thread_bottom_count_frame,NULL);
#if 1
	if(cam_id == DVR_ID)
		pthread_create(&thread_del_tmp_emfile_id,NULL,thread_del_tmp_emfile,NULL);
#endif	

	top_lastFrames = Emergency_Top_Sec * 30;
	bottom_lastFrames = Emergency_Bottom_Sec * 30;
		
	if(GetFreeRam(&freeram) && freeram<1843200*nbufs+4194304)
	{
		lidbg( "free memory isn't enough(%d),But still continue\n",freeram);		
		//return 1;
	}

	/* Allocate buffers. */
	if ((int)(nbufs = video_reqbufs(dev, nbufs)) < 0) {
		close(dev);
		close(flycam_fd);
		return 1;
	}

	/* Map the buffers. */
	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev, VIDIOC_QUERYBUF, &buf0);
		if (ret < 0) {
			lidbg( "Unable to query buffer %u (%d).\n", i, errno);
			close(dev);		
			close(flycam_fd);
			return 1;
		}
		TestAp_Printf(TESTAP_DBG_FLOW, "length: %u offset: %10u     --  ", buf0.length, buf0.m.offset);

		mem0[i] = mmap(0, buf0.length, PROT_READ, MAP_SHARED, dev, buf0.m.offset);
		if (mem0[i] == MAP_FAILED) {
			lidbg( "Unable to map buffer %u (%d)\n", i, errno);
			close(dev);		
			close(flycam_fd);
			return 1;
		}
		TestAp_Printf(TESTAP_DBG_FLOW, "Buffer %u mapped at address %p.\n", i, mem0[i]);
	}

	/* Queue the buffers. */
	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev, VIDIOC_QBUF, &buf0);
		if (ret < 0) {
			lidbg( "Unable to queue buffer0(%d).\n", errno);
			close(dev);		
			close(flycam_fd);
			return 1;
		}
	}

	/* Start streaming. */
	video_enable(dev, 1);


// chris +
	if((multi_stream_enable)&&(pixelformat == V4L2_PIX_FMT_H264))
	{
		char fake_filename[12];
        int MJ_width = 0, MJ_height = 0; 
		strcpy(fake_filename, argv[optind]);
		if(fake_filename[10]>0x30)	//	for string "/dev/videoX", if X>0, X--
			fake_filename[10] -= 1;
		/* Open the video device. */
		fake_dev = video_open(fake_filename);
		if (fake_dev < 0)
			return 1;

        if(multi_stream_format == MULTI_STREAM_HD_180P || multi_stream_format == MULTI_STREAM_HD_360P ||
                multi_stream_format == MULTI_STREAM_HD_180P_360P || multi_stream_format == MULTI_STREAM_360P_180P)
        {
            MJ_width = 640;
            MJ_height= 360;
        }
        else
        {
            MJ_width = 640;
            MJ_height= 480;
        }
		/* Set the video format. */
		if (video_set_format(fake_dev, MJ_width, MJ_height, V4L2_PIX_FMT_MJPEG) < 0) {
			close(fake_dev);
			return 1;
		}

		if(do_save)
		{
			/* Allocate buffers. */
			if(GetFreeRam(&freeram) && freeram<307200*nbufs+4+4194304)
			{
				lidbg( "do_save:free memory isn't enough(%d)\n",freeram);
				close(fake_dev);
				return 1;
			}
				
			if ((int)(nbufs = video_reqbufs(fake_dev, nbufs)) < 0) {
				close(fake_dev);
				return 1;
			}

			/* Map the buffers. */
			for (i = 0; i < nbufs; ++i) {
				memset(&buf1, 0, sizeof buf1);
				buf1.index = i;
				buf1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf1.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(fake_dev, VIDIOC_QUERYBUF, &buf1);
				if (ret < 0) {
					lidbg( "Unable to query buffer %u (%d).\n", i, errno);
					close(dev);
					close(flycam_fd);
					close(fake_dev);
					return 1;
				}
				TestAp_Printf(TESTAP_DBG_FLOW, "length: %u offset: %10u     --  ", buf1.length, buf1.m.offset);

				mem1[i] = mmap(0, buf1.length, PROT_READ, MAP_SHARED, fake_dev, buf1.m.offset);
				if (mem1[i] == MAP_FAILED) {
					lidbg( "Unable to map buffer %u (%d)\n", i, errno);
					close(dev);
					close(flycam_fd);
					close(fake_dev);
					return 1;
				}
				TestAp_Printf(TESTAP_DBG_FLOW, "Buffer %u mapped at address %p.\n", i, mem1[i]);
			}

			/* Queue the buffers. */
			for (i = 0; i < nbufs; ++i) {
				memset(&buf1, 0, sizeof buf1);
				buf1.index = i;
				buf1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf1.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(fake_dev, VIDIOC_QBUF, &buf1);
				if (ret < 0) {
					lidbg( "Unable to queue buffer (%d).\n", errno);
					close(dev);
					close(flycam_fd);
					close(fake_dev);
					return 1;
				}
			}
		}
		else
		{
			//add buffer to avoid vb2_streamon fail when video stream_on in kernel 3.18.0 
			if ((int)(nbufs = video_reqbufs(fake_dev, 6)) < 0) {
				close(fake_dev);
				return 1;
			}
		}
		
		/* Start streaming. */
		video_enable(fake_dev, 1);
	}
// chris -	





	if(multi_stream_enable != 0)
	{
		if((chip_id != CHIP_SNC291B)&&(chip_id != CHIP_SNC292A))
		{
			close(dev);
			close(fake_dev);			
			lidbg( "This command only for 291B & 292'\n");
			return 1;			
		}
		
		//set multi stream osd start position
		//if(XU_OSD_Get_MS_Start_Position(dev, &osd_ms_s0_start_row, &osd_ms_s0_start_col, &osd_ms_s1_start_row, &osd_ms_s1_start_col, &osd_ms_s2_start_row, &osd_ms_s2_start_col) <0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Start_Position Failed\n");

		//if(XU_OSD_Set_MS_Start_Position(dev, 0, osd_ms_s0_start_row, osd_ms_s0_start_col) <0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Start_Position Failed\n");

		//if(XU_OSD_Set_MS_Start_Position(dev, 1, osd_ms_s1_start_row, osd_ms_s1_start_col) <0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Start_Position Failed\n");
			
		//if(XU_OSD_Set_MS_Start_Position(dev, 2, osd_ms_s2_start_row, osd_ms_s2_start_col) <0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Start_Position Failed\n");

		//set multi stream osd size
		//if(XU_OSD_Get_Multi_Size(dev, &osd_ms_size_stream0, &osd_ms_size_stream1, &osd_ms_size_stream2) <0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Get_Multi_Size Failed\n");

		//if(XU_OSD_Set_Multi_Size(dev, osd_ms_size_stream0, osd_ms_size_stream1, osd_ms_size_stream2) <0)
			//lidbg( "SONiX_UVC_TestAP @main : XU_OSD_Set_Multi_Size Failed\n");
	}	

	if((do_record)&&(do_save)&&(multi_stream_enable!=0)&&(pixelformat == V4L2_PIX_FMT_H264))
	{
		struct thread_parameter par;
		par.buf = &buf1;
		for(i=0;i < nbufs;i++)
			par.mem[i] = mem1[i];

		par.dev = &fake_dev;
		par.nframes = &nframes;
		par.multi_stream_mjpg_enable = (multi_stream_enable & 0x02) >> 1;
		
		ret = pthread_create(&thread_capture_id,NULL,thread_capture,(void*)&par);
		if(ret != 0)
		{
			close(dev);
			close(flycam_fd);
			close(fake_dev);
			lidbg( "Create pthread error!\n");
			return 1;
		}
	}
    
    if(multi_stream_format == MULTI_STREAM_HD_180P || multi_stream_format == MULTI_STREAM_HD_360P ||
            multi_stream_format == MULTI_STREAM_HD_180P_360P || multi_stream_format == MULTI_STREAM_360P_180P)
    {
        sprintf(rec_filename2, "RecordH264_180P.h264");
        sprintf(rec_filename4, "RecordH264_360P.h264");
    }
	#if 0
	if(!do_save)//recording
	{
		ret = pthread_create(&thread_switch_id,NULL,thread_switch,NULL);
		if(ret != 0)
		{
			lidbg( "-----eho-----uvc switch pthread error!\n");
			return 1;
		}
	}
	property_get("lidbg.lidbg.uvccam.recording", startRecording, "1");
	#endif
	//property_get("persist.lidbg.uvccam.capture", startCapture, "1");
/*
	ret = pthread_create(&thread_switch_id,NULL,thread_checkdev,NULL);
	if(ret != 0)
	{
		lidbg( "-----eho-----checkdev pthread error!\n");
		return 1;
	}
*/

	for (i = 0; i < nframes; ++i) {

		/*read prop:whether stop recoding*/
    	switch_scan();

		if(!strncmp(isBlackBoxRec, "1", 1))
		{
			if(isVideoLoop)
			{
				char new_flyh264_filename[100] = {0};
				char tmp_cmd[200] = {0};
#if 0				
				while(1)
				{
					if(!isDequeue) break;
				}
#endif				

				if(cam_id == DVR_ID)
					sprintf(new_flyh264_filename, "%sEF%s.h264", Rec_Save_Dir, time_buf);
				else if(cam_id == REARVIEW_ID)
					sprintf(new_flyh264_filename, "%sER%s.h264", Rec_Save_Dir, time_buf);
				
				lidbg("======== new_flyh264_filename:%s=======\n",new_flyh264_filename);	
				if(rec_fp1 != NULL) fclose(rec_fp1);
#if 1
				if(rename(flyh264_filename, new_flyh264_filename) < 0)
					lidbg("========rename fail=======\n");			
#endif
				strcpy(flyh264_filename, new_flyh264_filename);
				rec_fp1 = fopen(flyh264_filename, "a+b");

				if(isRemainOldFp)
				{
					//lidbg("****<%d>new file isRemainOldFp****\n",cam_id);
					dequeue_buf(Emergency_Top_Sec * 30 ,rec_fp1);
					if(old_rec_fp1 != NULL) fclose(old_rec_fp1);
					isRemainOldFp = 0;
				}
				
				system("sync&");
				
				//if(rec_fp1 == NULL) lidbg("======== rec_fp1 null!=======\n");
				//lidbg("======== flyh264_filename 1111  %s=======\n",flyh264_filename);

				if((buf0.timestamp.tv_sec - originRecsec) % Rec_Sec > (Rec_Sec - 60)	)
				{
					int diffsec = (buf0.timestamp.tv_sec - originRecsec) % Rec_Sec - (Rec_Sec - 60);

					lidbg("======== diffsec:%d=======\n",diffsec);
					//lidbg("======== old originRecsec:%d=======\n",originRecsec);
					originRecsec += diffsec;
					//lidbg("======== new originRecsec:%d=======\n",originRecsec);
				}
				else lidbg("======== keep recording!=======\n");
			}
			else
			{
				if((isBlackBoxTopRec == 0) && (isBlackBoxBottomRec == 0))
				{
					int isStorageOK = 0;
					if(access(Em_Save_Dir, R_OK) != 0)
					mkdir(Em_Save_Dir,S_IRWXU|S_IRWXG|S_IRWXO);

					if(access(Em_Save_Dir, R_OK) != 0)
					{
						lidbg("Em_Save_Dir still not OK!\n");
						isStorageOK = 0;
					}
					else if(!strncmp(Em_Save_Dir, EMMC_MOUNT_POINT0, strlen(EMMC_MOUNT_POINT0)) )
					{
						size_t mbFreedisk;
						mbFreedisk = get_path_free_space(EMMC_MOUNT_POINT0);
						if(mbFreedisk < 300)
						{
							lidbg("[SD0] Emergency recording not enough space!!\n");
							send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_EM_SD0_INSUFFICIENT_SPACE);
							isStorageOK = 0;
						}
						else isStorageOK = 1;
					}
					else if(!strncmp(Em_Save_Dir, EMMC_MOUNT_POINT1, strlen(EMMC_MOUNT_POINT1)) )
					{
						size_t mbFreedisk;
						mbFreedisk = get_path_free_space(EMMC_MOUNT_POINT1);
						if(mbFreedisk < 100)
						{
							lidbg("[SD1] Emergency recording not enough space!!\n");
							send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_EM_SD1_INSUFFICIENT_SPACE);
							isStorageOK = 0;
						}
						else isStorageOK = 1;
					}
					else
					{
						lidbg("Emergency recording path ERR!! (ex:/storage/sdcard1.....)\n");
						isStorageOK = 0;
					}
					
					if(isStorageOK)
					{
						if(Emergency_Top_Sec <= Emergency_Bottom_Sec)
							BlackBoxBottomCnt = Emergency_Bottom_Sec / Emergency_Top_Sec; /*top <= bottom,use top as base*/
						else BlackBoxBottomCnt = 1;/*bottom <= top,1 time*/
						
						if(msize <= (Emergency_Top_Sec * 30))
						{
							lidbg("Waiting for msize restoration!\n");
#if 0
							isBlackBoxTopRec = 1;
							isBlackBoxBottomRec = 1;
							tmp_count = msize;
							pthread_create(&thread_dequeue_id,NULL,thread_dequeue,msize);
#endif
							isBlackBoxTopWaitDequeue = 1;
						}
						else
						{
#if 0
							//tmp_count = msize - 300;
							//pthread_create(&thread_dequeue_id,NULL,thread_dequeue,msize - 300);
							//usleep(20 * 1000);
							dequeue_buf(msize - 300,rec_fp1);
							isBlackBoxTopRec = 1;
							isBlackBoxBottomRec = 1;
							tmp_count = 300;
							pthread_create(&thread_dequeue_id,NULL,thread_dequeue,300);
#endif
							//pthread_create(&thread_dequeue_id,NULL,thread_top_dequeue,NULL);
							isTopDequeue = 1;
						}
					}
			}
			}
			
			//else isBlackBoxTopWaitDequeue = 1;
			
			if(cam_id == DVR_ID)
				property_set("lidbg.uvccam.dvr.blackbox", "0");
			else if(cam_id == REARVIEW_ID)
				property_set("lidbg.uvccam.rear.blackbox", "0");
		}

		top_totalFrames++;
		bottom_totalFrames++;

		if((msize > (Emergency_Top_Sec*30)) && isBlackBoxTopWaitDequeue && (isBlackBoxTopRec == 0) && (isBlackBoxBottomRec == 0))
		{
			lidbg("***isBlackBoxTopWaitDequeue***\n");
			//pthread_create(&thread_dequeue_id,NULL,thread_top_dequeue,NULL);
			isTopDequeue = 1;
			isBlackBoxTopWaitDequeue = 0;
		}

		if(msize == (Emergency_Top_Sec*30)) XU_H264_Set_IFRAME(dev);
		
		if((!strncmp(startRecording, "0", 1)) && (!do_save) )
		{
			lidbg("-------eho---------uvccam stop recording -----------\n");
#if 0
			if(isPreview) 
			{
				sprintf(tmpCMD , "rm -f %s&",Rec_Save_Dir);
				system(tmpCMD);
			}
#endif
			//dequeue_buf(msize,rec_fp1);
			//system("echo 'udisk_unrequest' > /dev/flydev0");
			property_set("fly.uvccam.curprevnum", "-1");
			send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_STOP);
			
			if(rec_fp1 != NULL) fclose(rec_fp1);
			close(dev);
			close(flycam_fd);
			return 0;
		}
		
		if(do_get_still_image)
		{
			if(i==nframes*2/3)
				get_still_image(dev, 640, 360, V4L2_PIX_FMT_YUYV);
			else 
				if(i==nframes*1/3)
				get_still_image(dev, 1280, 720, V4L2_PIX_FMT_MJPEG);
		}
		
		if((do_h264_iframe_set) && (i%h264_iframe_reset == 0))
		{
			XU_H264_Set_IFRAME(dev);
		}
		
		/* Dequeue a buffer. */
		memset(&buf0, 0, sizeof buf0);
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev, VIDIOC_DQBUF, &buf0);
		if (ret < 0) {
			lidbg( "Unable to dequeue buffer0 (%d).\n", errno);
			close(dev);
			close(flycam_fd);
			if(multi_stream_enable)
				close(fake_dev);
			return 1;
		}

		if(oldisVideoLoop <= 0 && isVideoLoop > 0)
		{
			lidbg("======== create new file!=======\n");
			originRecsec = buf0.timestamp.tv_sec;
			oldRecsec = 1;
		}
		oldisVideoLoop = isVideoLoop;
		
		if(i == 0) originRecsec = buf0.timestamp.tv_sec;
		gettimeofday(&ts, NULL);


// cjc +
		if(multi_stream_enable)
		{
			h264_decode_seq_parameter_set(mem0[buf0.index]+4, buf0.bytesused, &multi_stream_width, &multi_stream_height);
			
			multi_stream_resolution = (multi_stream_width << 16) | (multi_stream_height);
			if(multi_stream_resolution == H264_SIZE_HD)
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[   HD]  ");		
			}
			else if(multi_stream_resolution == H264_SIZE_VGA)
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[  VGA]  ");
			}
			else if(multi_stream_resolution == H264_SIZE_QVGA)
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[ QVGA]  ");
			}
			else if(multi_stream_resolution == H264_SIZE_QQVGA)
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[QQVGA]  ");
			}
            else if(multi_stream_resolution == H264_SIZE_360P)
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[ 360P]  ");
			}
            else if(multi_stream_resolution == H264_SIZE_180P)
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[ 180P]  ");
			}
			else
			{
				TestAp_Printf(TESTAP_DBG_FRAME, "[unknow size w:%d h:%d ]  ",multi_stream_width, multi_stream_height);
			}

		}
// cjc -

		TestAp_Printf(TESTAP_DBG_FRAME, "Frame[%4u] %u bytes %ld.%06ld %ld.%06ld\n ", i, buf0.bytesused, buf0.timestamp.tv_sec, buf0.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);

		if(do_md_result_get)
		{
			if(XU_MD_Get_RESULT(dev, md_mask) <0)
				lidbg( "SONiX_UVC_TestAP @main : XU_MD_Get_RESULT Failed\n");
		}

		if (i == 0)
			start = ts;

		/* Save the image. */
		if ((do_save && !skip)&&(pixelformat == V4L2_PIX_FMT_MJPEG))
		{
			time_t timep; 
			struct tm *p; 
			time(&timep); 
			p=gmtime(&timep); 
			property_get("persist.uvccam.capturepath", Capture_Save_Dir, EMMC_MOUNT_POINT0"/preview_cache/");
			lidbg("==========[%d]Capture_Save_Dir -> %s===========\n",cam_id,Capture_Save_Dir);
			lidbg_get_current_time(0 , time_buf, NULL);
			if(cam_id == DVR_ID)
				sprintf(filename, "%s/F%s.jpg", Capture_Save_Dir, time_buf);
			else if(cam_id == REARVIEW_ID)
				sprintf(filename, "%s/R%s.jpg", Capture_Save_Dir, time_buf);
			
			//sprintf(filename, "%s/FA%04d%02d%02d%02d%02d%02d.jpg",
			//	(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
			file = fopen(filename, "wb");
			if (file != NULL) {
				fwrite(mem0[buf0.index], buf0.bytesused, 1, file);
				fclose(file);
			}
			return 0;//exit
		}
		if (skip)
			--skip;

		/* Record the H264 video file */
		if(do_record)
		{
			if((multi_stream_enable & 0x01) == 1)
			{
				if(multi_stream_resolution == H264_SIZE_HD)
				{
					if(rec_fp1 == NULL)
						rec_fp1 = fopen(rec_filename1, "a+b");
					
					if(rec_fp1 != NULL)
						fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp1);
				}

				if(multi_stream_resolution == H264_SIZE_VGA || multi_stream_resolution == H264_SIZE_360P)
				{
				    
					if(rec_fp4 == NULL)
						rec_fp4 = fopen(rec_filename4, "a+b");
					
					if(rec_fp4 != NULL)
						fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp4);
				}

				if(multi_stream_resolution == H264_SIZE_QVGA || multi_stream_resolution == H264_SIZE_180P)
				{
					if(rec_fp2 == NULL)
						rec_fp2 = fopen(rec_filename2, "a+b");
					
					if(rec_fp2 != NULL)
						fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp2);
				}

				if(multi_stream_resolution == H264_SIZE_QQVGA)
				{
					if(rec_fp3 == NULL)
						rec_fp3 = fopen(rec_filename3, "a+b");
					
					if(rec_fp3 != NULL)
						fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp3);
				}
			}
			else
			{
				/*
				if(rec_fp1 == NULL)
				{
					lidbg_get_current_time(time_buf, NULL);
					sprintf(flyh264_filename[flytmpcnt], "%s%s_%d.h264", REC_SAVE_DIR, time_buf,flytmpcnt);
					rec_fp1 = fopen(flyh264_filename[flytmpcnt], "wb");
				}
				*/
				/*DVR Check save Dir size:whether change file name*/
				if((i % 30 == 0) && !isPreview && (cam_id == DVR_ID))
				{
					DIR *pDir ;
					struct dirent *ent;
					struct stat buf; 
					char path[100] = {0};
					totalSize = 0;
					pDir=opendir(Rec_Save_Dir);  
					if(pDir != NULL) 
					{
						while((ent=readdir(pDir))!=NULL)  
						{
							 if(!(ent->d_type & DT_DIR))  
					         {  
					                //if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (ent->d_reclen != 48) ) 
					                //        continue; 
									if((!strncmp(ent->d_name, "F", 1))  ||(!strncmp(ent->d_name, "R", 1)) )
									{
										sprintf(path , "%s%s",Rec_Save_Dir,ent->d_name);
										if (stat(path,&buf) == -1)
									    {
										      lidbg ("Get stat on %s Error?%s\n", ent->d_name, strerror (errno));
										      //return (-1);
									    }
										//lidbg("%s -> size = %d , ent->d_reclen = %ld/n",ent->d_name,buf.st_size,ent->d_reclen); 
										totalSize += buf.st_size/1000000;
									}
						 	 }
						}
						if((totalSize%50) == 0 && totalSize > 0) lidbg("total file size = %dMB\n",totalSize); 
						if(totalSize >= Rec_File_Size)	
						{
							//isExceed = 1;
							//send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_EXCEED_UPPER_LIMIT);
						}
						closedir(pDir);
					}
				}
				
				/*reserve for storage*/
				if(i % 30 == 0 && isVideoLoop)
				{
					if(!strncmp(Rec_Save_Dir, EMMC_MOUNT_POINT0, strlen(EMMC_MOUNT_POINT0)) )
					{
#if 0					
						struct statfs diskInfo;  
						statfs(EMMC_MOUNT_POINT0, &diskInfo);  
						unsigned long long totalBlocks = diskInfo.f_bsize;  
						unsigned long long stotalSize = totalBlocks * diskInfo.f_blocks;  
						size_t mbTotalsize = stotalSize>>20;  
						unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;  
						size_t mbFreedisk = freeDisk>>20;  
#endif
						size_t mbFreedisk;
						mbFreedisk = get_path_free_space(EMMC_MOUNT_POINT0);
						//lidbg(EMMC_MOUNT_POINT0"  total=%dMB, free=%dMB\n", mbTotalsize, mbFreedisk);  
						if(!isPreview)
						{
							if(mbFreedisk < 300 && !isOldFp && (cam_id == DVR_ID))
							{
								lidbg("======DVR:EMMC Free space less than 300MB!![%dMB]======\n",mbFreedisk);
								if(i == 0)
								{
									lidbg("======Init Free space less than 300MB!!Force quit![%dMB]======\n",mbFreedisk);
									send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INIT_INSUFFICIENT_SPACE_STOP);
#if 0									
									close(dev);
									close(flycam_fd);
									return 0;
#else
									isVideoLoop = 0;
									property_set("persist.uvccam.isDVRVideoLoop", "0");
									property_set("persist.uvccam.isRearVideoLoop", "0");
#endif
								}
								isExceed = 1;
								send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INSUFFICIENT_SPACE_CIRC);
							}
							if(mbFreedisk < 10 && i > 300)
							{
								lidbg("======Recording Protect![%dMB]======\n",mbFreedisk);
								send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INIT_INSUFFICIENT_SPACE_STOP);
#if 0									
								close(dev);
								close(flycam_fd);
								return 0;
#else
								isVideoLoop = 0;
								property_set("persist.uvccam.isDVRVideoLoop", "0");
								property_set("persist.uvccam.isRearVideoLoop", "0");
#endif
							}
						}
						else
						{
							if(mbFreedisk < 50)
							{
								lidbg("======ONLINE:EMMC Free space less than 50MB!!Force quit!======\n");
								send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_ONLINE_INVOKE_NOTIFY, RET_ONLINE_INSUFFICIENT_SPACE_STOP);
								close(dev);
								close(flycam_fd);
								return 0;
							}
						}
					}
					else if(!strncmp(Rec_Save_Dir, EMMC_MOUNT_POINT1, strlen(EMMC_MOUNT_POINT1)) )
					{
#if 0					
						struct statfs diskInfo;  
						statfs(EMMC_MOUNT_POINT1, &diskInfo);  
						unsigned long long totalBlocks = diskInfo.f_bsize;  
						unsigned long long stotalSize = totalBlocks * diskInfo.f_blocks;  
						size_t mbTotalsize = stotalSize>>20;  
						unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;  
						size_t mbFreedisk = freeDisk>>20;  
#endif
						size_t mbFreedisk;
						mbFreedisk = get_path_free_space(EMMC_MOUNT_POINT1);
						//lidbg(EMMC_MOUNT_POINT1"  total=%dMB, free=%dMB\n", mbTotalsize, mbFreedisk);  
						if(!isPreview)
						{
							if(mbFreedisk < 1024 && !isOldFp && (cam_id == DVR_ID))
							{
								lidbg("======DVR: Free space less than 1024MB!![%dMB]======\n",mbFreedisk);
#if 0								
								if(i == 0)
								{
									lidbg("======Init Free space less than 1024MB!!Force quit![%dMB]======\n",mbFreedisk);
									send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INIT_INSUFFICIENT_SPACE_STOP);
									close(dev);
									close(flycam_fd);
									return 0;
								}
#endif
								isExceed = 1;
								send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INSUFFICIENT_SPACE_CIRC);
							}
							if(mbFreedisk < 10 && i > 300)
							{
								lidbg("======Recording Protect![%dMB]======\n",mbFreedisk);
								send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INIT_INSUFFICIENT_SPACE_STOP);
#if 0									
								close(dev);
								close(flycam_fd);
								return 0;
#else
								isVideoLoop = 0;
								property_set("persist.uvccam.isDVRVideoLoop", "0");
								property_set("persist.uvccam.isRearVideoLoop", "0");
#endif
							}
						}
						else
						{
							if(mbFreedisk < 10)
							{
								lidbg("======ONLINE: Free space less than 10MB!!======\n");
								send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_ONLINE_INVOKE_NOTIFY, RET_ONLINE_INSUFFICIENT_SPACE_STOP);
								close(dev);
								close(flycam_fd);
								return 0;
							}
						}
					}
				}

				//lidbg("****buf0:%d,originRecsec:%d,oldRecsec:%d***\n",buf0.timestamp.tv_sec,originRecsec,oldRecsec);
				/*
					preview :
					not going to change file name.(tmpX.h264)
				*/
				if(isPreview)
				{
					//lidbg("======isPreview======");
					if(rec_fp1 == NULL)
					{
						lidbg_get_current_time(0,time_buf, NULL);
						sprintf(flypreview_filename, "%stmp%d.h264", Rec_Save_Dir,flytmpcnt);
						rec_fp1 = fopen(flypreview_filename, "wb");
						sprintf(flypreview_prevcnt, "%d", flytmpcnt);
						property_set("fly.uvccam.curprevnum", flypreview_prevcnt);
					}	
					if((i % (Rec_Sec*30)  == 0) && (i > 0) && (Max_Rec_Num != 1))//frames = sec * 30f/s
					{
							lidbg("preview change file name to write!");
							if(flytmpcnt < Max_Rec_Num - 1) flytmpcnt++;
							else flytmpcnt = 0;
							lidbg_get_current_time(0,time_buf, NULL);
							sprintf(flypreview_filename, "%stmp%d.h264", Rec_Save_Dir,flytmpcnt);
							rec_fp1 = fopen(flypreview_filename, "wb");
							sprintf(flypreview_prevcnt, "%d", flytmpcnt);
							property_set("fly.uvccam.curprevnum", flypreview_prevcnt);
					}
				}
				/*
					DVR:
					two condition:
					1.exceed [Rec_Sec] time;
					2.total size large than [Rec_File_Size] or sdcard0 less than 300MB.
					change file name.(current time)
				*/
				else if((((buf0.timestamp.tv_sec - originRecsec) % Rec_Sec == 0)
					&&(oldRecsec != (buf0.timestamp.tv_sec - originRecsec)) || isExceed) && isVideoLoop)
				{
					char minRecName[100] = EMMC_MOUNT_POINT0"/camera_rec/1111.mp4";//error for del
					unsigned char filecnt = 0;
					int ret;
#if 0				
					DIR *pDir ;
					struct dirent *ent; 
					int ret;
					char *date_time_key[5] = {NULL};
					char *date_key[3] = {NULL};
					char *time_key[3] = {NULL};
					int cur_date[3] = {0,0,0};
					int cur_time[3] = {0,0,0};
					int min_date[3] = {5000,13,50};
					int min_time[3] = {13,100,100};
					char minRecName[100] = EMMC_MOUNT_POINT0"/camera_rec/1111.h264";//error for del
					char tmpDName[100] = {0};
					unsigned char filecnt = 0;
					unsigned char memcpyFlag = 0;

					struct tm prevTm,curTm;
					time_t prevtimep = 0,curtimep;
#endif
					/*get last timeval: prevent from repetition*/
					if(!isExceed) oldRecsec = buf0.timestamp.tv_sec - originRecsec;
					
					#if 0
					if(i > 0)
					{
						if(flytmpcnt < Max_Rec_Num - 1) flytmpcnt++;
						else flytmpcnt = 0;
					}
					#endif
#if 0			
					/*find the earliest rec file and del*/
					pDir=opendir(Rec_Save_Dir);  
					while((ent=readdir(pDir))!=NULL)  
					{  
					        if(!(ent->d_type & DT_DIR))  
					        {  
					                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (ent->d_reclen != 48) ) 
					                        continue;  
									//if((!strncmp(ent->d_name, "F", 1) && (cam_id == DVR_ID)) ||(!strncmp(ent->d_name, "R", 1) && (cam_id == REARVIEW_ID)) )
									//{
										filecnt++;
						                //lidbg("ent->d_name:%s====ent->d_reclen:%d=====\n", ent->d_name,ent->d_reclen); 

										strcpy(tmpDName, ent->d_name + 1);
										lidbg_token_string(tmpDName, "__", date_time_key);
										//lidbg("date_time_key0:%s====date_time_key1:%s=====", date_time_key[0],date_time_key[2]);	
										lidbg_token_string(date_time_key[0], "-", date_key);
										//lidbg("date_key:%s====%s===%s==", date_key[0],date_key[1],date_key[2]);	
										lidbg_token_string(date_time_key[2], ".", time_key);
										//lidbg("time_key:%s====%s===%s==", time_key[0],time_key[1],time_key[2]);	
										
										curTm.tm_year = atoi(date_key[0]) -1900;
										curTm.tm_mon = atoi(date_key[1]) -1;
										curTm.tm_mday = atoi(date_key[2]);
										curTm.tm_hour = atoi(time_key[0]);
										curTm.tm_min = atoi(time_key[1]);
										curTm.tm_sec	 = atoi(time_key[2]);	
										curtimep = mktime(&curTm);
										
										#if 0
										lidbg("prevtimep=======%d========",  prevtimep);
										lidbg("curtimep=======%d========",  curtimep);
										lidbg("difftime=======%d========", difftime(curtimep, prevtimep));
										#endif
										if((curtimep < prevtimep) || (prevtimep == 0))
										{
											prevtimep = curtimep;
											strcpy(minRecName, ent->d_name);
											//lidbg("minRecName---%d--->%s\n",filecnt,minRecName);
										}
									//}
					        }  
					}
					closedir(pDir);
					//lidbg("minRecName end------>%s\n",minRecName);
#endif
					filecnt = find_earliest_file(Rec_Save_Dir,minRecName);
					/*
						1.storage exceed process:del the oldest rec file,keep writing last file;if filecnt < 2,create new file.
						2.time exceed process:write a new file.
					*/
					struct stat filebuf; 
					char filepath[200] = {0};
					lidbg("====== totally %d files.======\n",filecnt);
					if(isExceed)
					{
						//lidbg("current cnt------>%d\n",filecnt);
						sprintf(filepath , "%s%s",Rec_Save_Dir,minRecName);
						if (stat(filepath,&filebuf) == -1)
					    {
						      lidbg ("Get stat on %s Error?%s\n", filepath, strerror (errno));
					    }
						if(strcmp(filepath,flyh264_filename) == 0 || filecnt == 0)
						{
							lidbg("======Can not del processing file!Stop!======\n");
#if 0
							lidbg_get_current_time(time_buf, NULL);
							sprintf(flyh264_filename, "%s%s.h264", Rec_Save_Dir, time_buf);
							lidbg("=========new flyh264_filename : %s===========\n", flyh264_filename);
							rec_fp1 = fopen(flyh264_filename, "wb");
#endif
							send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_INSUFFICIENT_SPACE_STOP);
#if 0									
							close(dev);
							close(flycam_fd);
							return 0;
#else
							isVideoLoop = 0;
							property_set("persist.uvccam.isDVRVideoLoop", "0");
							property_set("persist.uvccam.isRearVideoLoop", "0");
#endif
						}
						lidbg("====== oldest rec file will be del:%s (%d MB).======\n",minRecName,filebuf.st_size/1000000);
#if 0						
						sprintf(tmpCMD , "rm -f %s&",filepath);
						system(tmpCMD);
#endif				
						remove(filepath);  
						if((totalSize - filebuf.st_size/1000000) > Rec_File_Size)
							lidbg("rec file exceed!still has %d MB .\n",(totalSize - filebuf.st_size/1000000));
					}
					else
					{
						//tmp_count = msize;
						//pthread_create(&thread_dequeue_id,NULL,thread_dequeue,msize);
						//pthread_join(thread_capture_id,NULL);
						
						//while(isDequeue) usleep(100*1000);
						//dequeue_buf(msize,rec_fp1);
#if 1
						if(rec_fp1 != NULL) 
						{
							//lidbg("****<%d>start feeding  %s****\n",cam_id,flyh264_filename);
							fclose(rec_fp1);
							strcpy(old_flyh264_filename, flyh264_filename);
							old_rec_fp1 = fopen(old_flyh264_filename, "ab+");
							isOldFp = 1;
							isNormDequeue = 1;	
						}
#else
						if(rec_fp1 != NULL) fclose(rec_fp1);
#endif
#if 0
						isIframe = 1;
						XU_H264_Set_IFRAME(dev);
#endif
						isNewFile = 1;

						lidbg_get_current_time(0 , time_buf, NULL);

						if(cam_id == DVR_ID)
							sprintf(flyh264_filename, "%sF%s.h264", Rec_Save_Dir, time_buf);
						else if(cam_id == REARVIEW_ID)
							sprintf(flyh264_filename, "%sR%s.h264", Rec_Save_Dir, time_buf);
						
						lidbg("=========new flyh264_filename : %s===========\n", flyh264_filename);		
						rec_fp1 = fopen(flyh264_filename, "wb");
						//if(i > 0) fwrite(iFrameData, iframe_length , 1, rec_fp1);
					}
					#if 0
					/*only if within Rec_File_Size,otherwise del oldest file again.*/
					if(((totalSize - filebuf.st_size) /1000000) < Rec_File_Size)
					{
						lidbg_get_current_time(time_buf, NULL);
						sprintf(flyh264_filename, "%s%s.h264", Rec_Save_Dir, time_buf);
						lidbg("=========new flyh264_filename : %s===========\n", flyh264_filename);
						rec_fp1 = fopen(flyh264_filename, "wb");
					}
					else lidbg("rec file exceed!still has %d MB .\n",((totalSize - filebuf.st_size) /1000000));
					#endif
					
				}
				
				if(isPreview) 
					fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp1);
				else
				{
					//if (isIframe == 1)
					//	isIframe = 0;
#if 0		
					unsigned char tmp_val = 0;

		
					if(i == 0) 
					{
						iframe_length = buf0.bytesused;
						lidbg("=====IFRAME SAVE!length:%d======\n",iframe_length);
						iFrameData = malloc(iframe_length);  
						memcpy(iFrameData, mem0[buf0.index], iframe_length);
					}

					unsigned char a = 0;
					for(a = 0;a < 30;a++){
						tmp_val = *(unsigned char*)(mem0[buf0.index] + a);
						lidbg("======tmp_val[%d]:%d=====\n",a,tmp_val);
						if(tmp_val == 0x65) lidbg("=====****IFRAME detect!!***======\n");
					}


					tmp_val = *(unsigned char*)(mem0[buf0.index] + 26);
					if(tmp_val == 0x65)
						lidbg("=====****IFRAME detect!!***%dBytes======\n",buf0.bytesused);
#endif

#if 0
					/*IFrame filtering */
					tmp_val = *(unsigned char*)(mem0[buf0.index] + 26);
					if(tmp_val == 0x65) 
					{
						//ALOGE("********<%d>=>Frame[%4u] %u bytes %ld.%06ld %ld.%06ld*******\n ",cam_id, i, buf0.bytesused, buf0.timestamp.tv_sec, buf0.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);
						if(((oldFrameSize > buf0.bytesused) && ((oldFrameSize - buf0.bytesused) > iframe_diff_val)) || (buf0.bytesused < iframe_threshold_val))
						{
							//lidbg("=====IFRAME set!Throw!======\n");
							ALOGE("********<%d>=>Frame[%4u] %u bytes %ld.%06ld %ld.%06ld*******\n ",cam_id, i, buf0.bytesused, buf0.timestamp.tv_sec, buf0.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);
							isBrokenIFrame = 1;
							XU_H264_Set_IFRAME(dev);
						}
						else
						{
							isBrokenIFrame = 0;
							if(msize <=  (Emergency_Top_Sec * 30 *2) + 1000)
								enqueue(mem0[buf0.index], buf0.bytesused);
						}
						oldFrameSize = buf0.bytesused;
					}
					else if(!isBrokenIFrame)
					{
						if(msize <=  (Emergency_Top_Sec * 30 *2) + 1000)
							enqueue(mem0[buf0.index], buf0.bytesused);
					}
					//else lidbg("=====other throw!======\n");
#else
					if(msize <=  (Emergency_Top_Sec * 30 *2) + 1000)
							enqueue(mem0[buf0.index], buf0.bytesused);
#endif
						
					
					if(isBlackBoxBottomRec && (isBlackBoxTopRec == 0))
					{
						//lidbg("======Bottom write===lastFrames:%d,Bottom_Sec:%d======\n",bottom_lastFrames,Emergency_Bottom_Sec);
						if(Emergency_Top_Sec > Emergency_Bottom_Sec) /*bottom < top ,1 time*/
						{
							if(msize > (isBlackBoxBottomRec*30)) 
							{
								tmp_count = Emergency_Bottom_Sec*30 + 10;
								isNormDequeue = 1;
							}
						}
						else if(msize > (Emergency_Top_Sec*30))  /*top <= bottom,use top as base*/
						{
							if(top_lastFrames < (Emergency_Top_Sec*30 - 150)) tmp_count = Emergency_Top_Sec*30 - 150;
							else if(top_lastFrames > (Emergency_Top_Sec*30 + 30)) tmp_count = Emergency_Top_Sec*30 + 30;
							else tmp_count = (top_lastFrames/10)*10 + 10;
							isNormDequeue = 1;
						}
					}
					else if(!isBlackBoxBottomRec && (msize % 100 == 0) && (msize >= (Emergency_Top_Sec * 30 *2)))
					{
						tmp_count = Emergency_Top_Sec * 30;
						//pthread_create(&thread_dequeue_id,NULL,thread_dequeue,&tmp_count);
						if(isVideoLoop)
						{
							/*left 5s for subpackage*/
							if((buf0.timestamp.tv_sec - originRecsec) % Rec_Sec < (Rec_Sec - 5))
								isNormDequeue = 1;
						}
						else isNormDequeue = 1;
					}
				}
				isExceed = 0;
			}
		}

		/* Requeue the buffer. */
		if (delay > 0)
			usleep(delay * 1000);

		ret = ioctl(dev, VIDIOC_QBUF, &buf0);
		if (ret < 0) {
			lidbg( "Unable to requeue buffer0 (%d).But try again.\n", errno);
			//goto try_open_again;
#if 1
			close(flycam_fd);
			close(dev);
			if(multi_stream_enable)
				close(fake_dev);		
			property_set("fly.uvccam.curprevnum", "-1");
			//system("echo 'udisk_unrequest' > /dev/flydev0");

			return 1;
#endif
		}

		fflush(stdout);
	}
	gettimeofday(&end, NULL);

	if((do_record)&&(do_save)&&(multi_stream_enable == 1)&&(pixelformat == V4L2_PIX_FMT_H264))
		pthread_join(thread_capture_id,NULL);
	
	/* Stop streaming. */
	video_enable(dev, 0);
	
	if(multi_stream_enable)
		video_enable(fake_dev, 0);

	/* Houston 2010/11/30 */
	if(do_record && rec_fp1 != NULL)
		fclose(rec_fp1);
	
// chris +
	if(do_record && rec_fp2 != NULL)
		fclose(rec_fp2);

	if(do_record && rec_fp3 != NULL)
		fclose(rec_fp3);
// chris -		

	end.tv_sec -= start.tv_sec;
	end.tv_usec -= start.tv_usec;

	if (end.tv_usec < 0) {
		end.tv_sec--;
		end.tv_usec += 1000000;
	}
	fps = (i-1)/(end.tv_usec+1000000.0*end.tv_sec)*1000000.0;

	printf("Captured %u frames in %lu.%06lu seconds (%f fps).\n",
		i-1, end.tv_sec, end.tv_usec, fps);

	if(gH264fmt)
	{
		free(gH264fmt);
		gH264fmt = NULL;
	}

	close(dev);
	close(flycam_fd);
	if(multi_stream_enable)
		close(fake_dev);	

	//system("echo 'udisk_unrequest' > /dev/flydev0");
	return 0;

#if 1
/*when open fail,go to try_open_again (suspend adaptation) */
try_open_again:
		//system("echo 'udisk_request' > /dev/flydev0");
		usleep(500*1000);
		//sleep(2);
		/*check on-off cmd */
		//property_get("lidbg.uvccam.dvr.recording", startRecording, "0");
		switch_scan();
		if((!strncmp(startRecording, "0", 1)) && (!do_save) )
		{
			lidbg("-------eho---------uvccam stop recording! -----------\n");
			//system("echo 'udisk_unrequest' > /dev/flydev0");
			property_set("fly.uvccam.curprevnum", "-1");
			send_driver_msg(FLYCAM_STATUS_IOC_MAGIC, NR_STATUS, RET_DVR_STOP);
			if(rec_fp1 != NULL) fclose(rec_fp1);
			close(dev);
			close(flycam_fd);
			return 0;
		}
		/*exit when timeout*/
		if(!(tryopencnt--))
		{
			lidbg("-------eho---------uvccam try open timeout! -----------\n");
			//system("echo 'udisk_unrequest' > /dev/flydev0");
			property_set("fly.uvccam.curprevnum", "-1");

			return 1;
		}
		/*fix usb R/W error(reverse 8*500ms for initialization)*/
		if(tryopencnt == 8)
		{
			//system("echo 'ws udisk_reset' > /dev/lidbg_pm0");
			lidbg("%s: Can not found camera till 8!!Camera may burst R/W error unexpected!\n", __func__);
		}
		//lidbg("%s: Camera may extract unexpected!try open again!-> %d\n", __func__,tryopencnt);
		lidbg("%s: Camera open fail!try open again!-> %d\n", __func__,tryopencnt);
		//rc = get_hub_uvc_device(devName,do_save,do_record);
		if(pixelformat == V4L2_PIX_FMT_MJPEG)
			rc = lidbg_get_hub_uvc_device(RECORD_MODE,devName,cam_id,0);
		else
			rc = lidbg_get_hub_uvc_device(RECORD_MODE,devName,cam_id,1);
		if((rc == -1) || (*devName == '\0'))
        {
            lidbg("%s: No UVC node found again\n", __func__);
            //return 1;
            goto try_open_again;
        }
		else goto openfd;
#endif
}
