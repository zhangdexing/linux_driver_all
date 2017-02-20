#include "Flydvr_ISPIF.h"

//==========Sonix VR===========//
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

#include "rec/v4l2uvc.h"
#include "rec/sonix_xu_ctrls.h"
#include "rec/nalu.h"
//#include "rec/debug.h"
#include "rec/cap_desc_parser.h"
#include "rec/cap_desc.h"

/*HAL*/
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/resource.h>


#define	FRONT_VR_BITRATE			12000000
#define	REAR_VR_BITRATE			2000000

#define	FRONT_VR_FRAMERATE	30
#define	REAR_VR_FRAMERATE		30

#define FRONT_VR_FMT					V4L2_PIX_FMT_H264
#define REAR_VR_FMT					V4L2_PIX_FMT_H264

#define FRONT_VR_WIDTH					1280
#define FRONT_VR_HEIGHT					720
#define REAR_VR_WIDTH					1280
#define REAR_VR_HEIGHT					720
#define FRONT_ONLINE_VR_WIDTH					480
#define FRONT_ONLINE_VR_HEIGHT				272

#define	FRONT_VR_BIT_RATE_MODE	1	//CBR (CBR:1,VBR:2)
#define	REAR_VR_BIT_RATE_MODE		1	//VBR (CBR:1,VBR:2)
#define	FRONT_ONLINE_VR_BIT_RATE_MODE	2	//CBR (CBR:1,VBR:2)

typedef enum{
    VR_CMD_EXIT = 0,
    VR_CMD_PAUSE,
    VR_CMD_RESUME,
    VR_CMD_GSENSOR_CRASH,
    VR_CMD_CHANGE_SINGLE_FILE_TIME,
}VR_CMD;

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,32)
#define V4L_BUFFERS_DEFAULT	6//6,16
#define V4L_BUFFERS_MAX		16//16,32
#else
#define V4L_BUFFERS_DEFAULT	3
#define V4L_BUFFERS_MAX		3
#endif

typedef struct {
	int dev;
	struct v4l2_buffer buf0;
	void *mem0[V4L_BUFFERS_MAX];
	FILE *rec_fp;
	char currentVRFileName[255];
	FILE *old_rec_fp;
	pthread_t                           VRThread;
    //Mutex                               lock;
	volatile int                        VRCmdPending;
    volatile int                        VRCmd;
    int                                 VREnabledFlag;
	unsigned int 					nbufs;

	bool 								iswritePermitted;
	bool								isEMHandling;

	char devH264Name[255];

    int                                 msgEnabledFlag;

    /* capture related members */
    /* prevFormat is pixel format of preview buffers that are exported */
    int                                 VRFormat;
    int                                 VRFps;
    int                                 VRWidth;
    int                                 VRHeight;
	int									 VRBitRate;
	int									 VRBitRateMode;
} isp_hardware_t;

void Sonix_ISP_IF_LIB_Init(void);
ISP_IF_VERSION Sonix_ISP_IF_LIB_GetFrontLibVer(void);
ISP_IF_VERSION Sonix_ISP_IF_LIB_GetRearLibVer(void);
ISP_IF_VERSION Sonix_ISP_IF_LIB_GetSensorName(void);

INT32 Sonix_ISP_IF_LIB_StartFrontVR(void);
INT32 Sonix_ISP_IF_LIB_StopFrontVR(void);
INT32 Sonix_ISP_IF_LIB_StartFrontOnlineVR(void);
INT32 Sonix_ISP_IF_LIB_StopFrontOnlineVR(void);
INT32 Sonix_ISP_IF_LIB_StartRearVR(void);
INT32 Sonix_ISP_IF_LIB_StopRearVR(void);
INT32 Sonix_ISP_IF_LIB_StartLPDaemon();
FLY_BOOL Sonix_ISP_IF_LIB_CheckFrontCamExist(void);
FLY_BOOL Sonix_ISP_IF_LIB_CheckRearCamExist(void);
INT32 Sonix_ISP_IF_LIB_StartLPDaemon(void);