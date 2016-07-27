
#define ALOG_NIDEBUG 1
#define LOG_TAG "lidbgUsbCamera"

#include "lidbg_servicer.h"
#include "LidbgCameraCommon.h"
#include "LidbgCameraUsb.h"
#include <cutils/properties.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <dirent.h>  
#include <sys/stat.h>  
#include <sys/types.h> 
#include "../inc/lidbg_flycam_app.h"

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

//for hub
#define	FRONT_NODE		"1-1.2"	
#define	BACK_NODE		"1-1.3"

static int is_debug = 0;
static int cam_id = -1;

extern "C" {
#include <sys/time.h>
#include "jpeg2yuv/LidbgJpeg2Yuv.c"
}

#undef ALOGE
#define ALOGE(msg...) do{\
	lidbg(msg); \
	ALOGD(msg); \
}while(0)

#ifdef PLATFORM_msm8909
//#undef USE_ION
#endif

camera_device_ops_t usbcam_camera_ops =
{
set_preview_window:
    android::usbcam_set_preview_window,
set_callbacks:
    android::usbcam_set_CallBacks,
enable_msg_type:
    android::usbcam_enable_msg_type,
disable_msg_type:
    android::usbcam_disable_msg_type,
msg_type_enabled:
    android::usbcam_msg_type_enabled,

start_preview:
    android::usbcam_start_preview,
stop_preview:
    android::usbcam_stop_preview,
preview_enabled:
    android::usbcam_preview_enabled,
store_meta_data_in_buffers:
    android::usbcam_store_meta_data_in_buffers,

start_recording:
    android::usbcam_start_recording,
stop_recording:
    android::usbcam_stop_recording,
recording_enabled:
    android::usbcam_recording_enabled,
release_recording_frame:
    android::usbcam_release_recording_frame,

auto_focus:
    android::usbcam_auto_focus,
cancel_auto_focus:
    android::usbcam_cancel_auto_focus,

take_picture:
    android::usbcam_take_picture,
cancel_picture:
    android::usbcam_cancel_picture,

set_parameters:
    android::usbcam_set_parameters,
get_parameters:
    android::usbcam_get_parameters,
put_parameters:
    android::usbcam_put_parameters,
send_command:
    android::usbcam_send_command,

release:
    android::usbcam_release,
dump:
    android::usbcam_dump,
};

#define CAPTURE                 1
#define DISPLAY                 1
#define CALL_BACK               1
#define MEMSET                  0
#define FREAD_JPEG_PICTURE      0
#define JPEG_ON_USB_CAMERA      1
#define FILE_DUMP_CAMERA        0
#define FILE_DUMP_B4_DISP       0

namespace android
{
    static int ioctlLoop(int fd, int ioctlCmd, void *args);

    extern "C" int usbcam_get_number_of_cameras()
    {
        int numCameras = 2;
        ALOGD("%s: futenghfei2.E", __func__);
        return numCameras;
    }

    extern "C" int usbcam_get_camera_info(int camera_id, struct camera_info *info)
    {
        int mcamera_id = camera_id;
        int rc = -1;
        ALOGD("%s___camera_id = %d: E", __func__,camera_id);
        if(info)
        {
        	if(camera_id == 1)//front cam:origin
			{
				ALOGD("%s: front cam:origin", __func__);
				info->facing = CAMERA_FACING_BACK;
	        	info->orientation = 0;
			}
			else if(camera_id == 0)//back cam:mirror
			{
				ALOGD("%s: back cam:mirror", __func__);
				info->facing = CAMERA_FACING_FRONT;
	        	info->orientation = 0;
			}
            rc = 0;
        }
        ALOGD("%s: X", __func__);
        return rc;
    }

    static String8 create_sizes_str(const camera_size_type *sizes, int len)
    {
        String8 str;
        char buffer[32];
        if (len > 0)
        {
            snprintf(buffer, sizeof(buffer), "%dx%d", sizes[0].width, sizes[0].height);
            str.append(buffer);
        }
        for (int i = 1; i < len; i++)
        {
            snprintf(buffer, sizeof(buffer), ",%dx%d", sizes[i].width, sizes[i].height);
            str.append(buffer);
        }
        return str;
    }


    static camera_size_type picture_sizes[] =
    {
        { 1920, 1088}, //HD1080
        { 1280, 720}, //HD720
        { 640, 480}, // VGA
        { 320, 240}, // QVGA
    };

    int usbCamInitDefaultParameters(camera_hardware_t *camHal)
    {
        ALOGD("%s: E", __func__);
        int rc = 0;
        char tempStr[FILENAME_LENGTH];

        camHal->prevFormat          = DEFAULT_USBCAM_PRVW_FMT;
        camHal->prevWidth           = DEFAULT_USBCAM_PRVW_WD;
        camHal->prevHeight          = DEFAULT_USBCAM_PRVW_HT;
        camHal->dispFormat          = camHal->prevFormat;
        camHal->dispWidth           = camHal->prevWidth;
        camHal->dispHeight          = camHal->prevHeight;
        camHal->pictFormat          = DEFAULT_USBCAM_PICT_FMT;
        camHal->pictWidth           = DEFAULT_USBCAM_PICT_WD;
        camHal->pictHeight          = DEFAULT_USBCAM_PICT_HT;
        camHal->pictJpegQlty        = DEFAULT_USBCAM_PICT_QLTY;
        camHal->thumbnailWidth      = DEFAULT_USBCAM_THUMBNAIL_WD;
        camHal->thumbnailHeight     = DEFAULT_USBCAM_THUMBNAIL_HT;
        camHal->thumbnailJpegQlty   = DEFAULT_USBCAM_THUMBNAIL_QLTY;
        camHal->previewEnabledFlag  = 0;
        camHal->prvwStoppedForPicture = 0;
        camHal->prvwCmdPending      = 0;
        camHal->takePictInProgress  = 0;

        camHal->pictSizeValues = create_sizes_str(
                                     picture_sizes, sizeof(picture_sizes) / sizeof(camera_size_type));
        ALOGD("%s: X", __func__);

        return rc;
    }

void lidbg_set_sonix_osd_time()
{
	time_t timep; 
	struct tm *p; 
	char rtc_cmd[100] = {0};
	time(&timep); 
	p=localtime(&timep); 
	sprintf(rtc_cmd, "./flysystem/lib/out/lidbg_testuvccam /dev/video1 --xuset-rtc %d %d %d %d %d %d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday,p->tm_hour , p->tm_min,p->tm_sec);
	system(rtc_cmd);
	return;
}
#if 0
static int get_uvc_device(const char *id,char *devname)
{
    char    temp_devname[FILENAME_LENGTH];
    int     i = 0, ret = 0, fd = -1, uvc_count = -1;
    struct  v4l2_capability     cap;
	cam_id = -1;

    if(id)
        cam_id = atoi(id);


    ALOGD("%s: E,======[%d]", __func__, cam_id);
    *devname = '\0';
    while(1)
    {
        sprintf(temp_devname, "/dev/video%d", i);
        //ALOGI("%s: trying open ==%d==[%s]", __func__, i, temp_devname);        
        fd = open(temp_devname, O_RDWR  | O_NONBLOCK, 0);
		i++;
        if(-1 != fd)
        {
            ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if((0 == ret) || (ret && (ENOENT == errno)))
            {
                if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))//not usb cam node
                {
                    ALOGE("%s: This is not video capture device -> %s\n", __func__,temp_devname);
                    close(fd);
                    continue;
                }
#if 0
                {
                    struct v4l2_fmtdesc fmt;
                    int isH264sup = -1;
                    memset(&fmt, 0, sizeof(fmt));
                    fmt.index = 0;
                    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt)) == 0)
                    {
                        fmt.index++;
                        if(V4L2_PIX_FMT_H264 == fmt.pixelformat)
                        {
                            isH264sup = 1;
                            break;
                        }
                    }
                    if(isH264sup == 1)
                    {
                        ALOGE("%s: V4L2_PIX_FMT_H264 is supported,find next node", __func__ );
                        close(fd);
                        continue;
                    }
                }
                uvc_count++;
                ALOGE("%s: Found UVC node: ======%s,[%d,%d]\n", __func__, temp_devname, cam_id, uvc_count);
                if(cam_id != -1 && cam_id != uvc_count)
                {
                    ALOGE("%s: need to find another======", __func__);
                    close(fd);
                    continue;
                }
#endif
                strncpy(devname, temp_devname, FILENAME_LENGTH);
                ALOGE("%s: Found UVC node,OK: ======%s,[%d,%d]\n", __func__, temp_devname, cam_id, uvc_count);
                break;
            }
            close(fd);
        }
        else if(i < 10)
            ALOGE("%s.%d: Probing.%s: ret: %d, errno: %d,%s", __func__, i, temp_devname, ret, errno, strerror(errno));

        if(i > 1000)
        {
            strncpy(devname, "/dev/video1", FILENAME_LENGTH);
            //*devname = '\0';
            ALOGE("%s.%d: Probing fail:%s \n", __func__, i, devname);
            //break;
            ALOGD("%s: X,%s", __func__, devname);
			return -1;
        }
    }

    ALOGD("%s: X,%s", __func__, devname);
    return 0;
}

static int get_hub_uvc_device(const char *id,char *devname)
{
	char temp_devname[FILENAME_LENGTH], temp_devname2[FILENAME_LENGTH],hub_path[FILENAME_LENGTH];
    int  ret = 0, fd = -1;
	cam_id = -1;
    struct  v4l2_capability     cap;
	DIR *pDir ;  
	struct dirent *ent  ;  
	int fcnt = 0  ;  

    if(id)
        cam_id = atoi(id);

	property_set("fly.uvccam.camid", id);

    ALOGD("%s: E,=======[%d]", __func__, cam_id);
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
		lidbg("%s: can not found suitable hubpath! ", __func__ );	
		goto failproc;
	}
	
	ALOGE("%s:hubPath:%s",__func__ ,hub_path);  
#if 0
	//check Front | Back Cam
	if(cam_id == 1)
		sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", FRONT_NODE,FRONT_NODE);//front cam
	else if(cam_id == 0)
		sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/%s:1.0/video4linux/", BACK_NODE,BACK_NODE);//back cam
	else
	{
		ALOGE("%s: cam_id wrong!==== %d ", __func__ , cam_id);
		goto failproc;
	}
#endif
	if(access(hub_path, R_OK) != 0)
	{
		ALOGE("%s: hub path access wrong! ", __func__ );
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
						break;
					}
	                sprintf(temp_devname,"/dev/%s", ent->d_name);  
	                ALOGD("%s:Path:%s",__func__ ,temp_devname);  
	        }  
	}

	ALOGD("%s: This Camera has %d video node.", __func__ , fcnt - 2);
	if((fcnt == 3) && (cam_id == 1))	
	{
		ALOGE("%s: Front Camera does not support Sonix Recording!", __func__);
	}
	
	if((fcnt == 0) && (ent == NULL))
	{
		ALOGE("%s: Hub node is not exist ! ", __func__);
		goto failproc;
	}
	
openDev:
	  ALOGI("%s: trying open ====[%s]", __func__, temp_devname);
      fd = open(temp_devname, O_RDWR  | O_NONBLOCK, 0);
      if(-1 != fd)
      {
          ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
          if((0 == ret) || (ret && (ENOENT == errno)))
          {
          	  //not usb cam node
              if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
              {
					ALOGE("%s: This is not video capture device -> %s\n", __func__,temp_devname);
					close(fd);
					if(fcnt == 4)	
					{
						//sprintf(temp_devname,temp_devname2); 
						strncpy(temp_devname, temp_devname2, FILENAME_LENGTH);
						fcnt--;
						goto openDev;
					}
					else goto failproc;
              }
			  //for sonix cam:do not preview on H264 recording node
			  if(fcnt == 4)
              {
                  struct v4l2_fmtdesc fmt;
                  int isH264sup = -1;
                  memset(&fmt, 0, sizeof(fmt));
                  fmt.index = 0;
                  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                  while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt)) == 0)
                  {
                      fmt.index++;
                      if(V4L2_PIX_FMT_H264 == fmt.pixelformat)
                      {
                          isH264sup = 1;
                          break;
                      }
                  }
                  if(isH264sup == 1)
                  {
						close(fd);
						ALOGI("%s: V4L2_PIX_FMT_H264 is supported,find next node", __func__ );
						//sprintf(temp_devname,temp_devname2); 
						strncpy(temp_devname, temp_devname2, FILENAME_LENGTH);
						fcnt--;
						goto openDev;
                  }
              }        
              strncpy(devname, temp_devname, FILENAME_LENGTH);
              ALOGD("%s: Found UVC node,OK: ======%s,[camid = %d]\n", __func__, temp_devname, cam_id);
          }
          close(fd);
      }
      else if(2 != errno)
          ALOGE("%s: Probing.%s: ret: %d, errno: %d,%s", __func__, temp_devname, ret, errno, strerror(errno));
    ALOGD("%s: X,%s", __func__, devname);
    return 0;

failproc:
	strncpy(devname, "/dev/video1", FILENAME_LENGTH);
	//*devname = '\0';
	ALOGD("%s: Probing fail:%s , run normal proc", __func__, devname);
#if NONE_HUB_SUPPORT
	system("echo flyaudio:touch /dev/CameraScan1.txt > /dev/lidbg_misc0");
	return get_uvc_device(id , devname);
#else
	return -1;
#endif
}
#endif
    extern "C" int  usbcam_camera_device_open(
        const struct hw_module_t *module, const char *id,
        struct hw_device_t **hw_device)
    {
        const struct hw_module_t *mmodule = module;
        const char *mid = id;
        int rc = -1;
        camera_device       *device = NULL;
        camera_hardware_t   *camHal;
        char                *dev_name;
        *hw_device = NULL;
        ALOGD("%s: E", __func__);

        camHal = new camera_hardware_t();
        if(!camHal)
        {

            ALOGE("%s:  end in no mem", __func__);
            rc = -1;
            goto out_err;
        }

        rc = usbCamInitDefaultParameters(camHal);
        if(0 != rc)
        {
            ALOGE("%s: usbCamInitDefaultParameters error", __func__);
            rc = -1;
            goto out_err;
        }
getuvcdevice:
        dev_name = camHal->dev_name;
        //rc = get_hub_uvc_device(mid,dev_name);
        cam_id = atoi(mid);
        rc = lidbg_get_hub_uvc_device(PREVIEW_MODE,dev_name,cam_id,0);
        if((rc == -1) || (*dev_name == '\0'))
        {
            ALOGE("%s: No UVC node found \n", __func__);
            //goto try_open_again;
            goto out_err;
        }
openfd:
        camHal->fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (camHal->fd <  0)
        {
            ALOGE("%s: Cannot open '%s'", __func__, dev_name);
            rc = -1;
			goto out_err;
        }
        else
        {
            ALOGD("%s:  open.success '%s',%d", __func__, dev_name, camHal->fd);
        }

        device                  = &camHal->hw_dev;
        device->common.close    = usbcam_close_camera_device;
        device->ops             = &usbcam_camera_ops;
        device->priv            = (void *)camHal;
        *hw_device              = &(device->common);

        ALOGD("%s: camHal: %p", __func__, camHal);
        ALOGE("%s: X %d", __func__, rc);

        return 0;
out_err:
        ALOGE("%s: X.out_err  %d", __func__, rc);
        return rc;
#if 0
try_open_again:
		system("echo 'udisk_request' > /dev/flydev0");
		ALOGE("%s: echo 'udisk_request' > /dev/flydev0 success\n", __func__);
		//usleep(500*1000);
		sleep(2);
		rc = get_hub_uvc_device(mid,dev_name);
		if((rc == -1) || (*dev_name == '\0'))
        {
            ALOGE("%s: No UVC node found again\n", __func__);
            goto out_err;
        }
		else goto openfd;
#endif
    }

    extern "C"  int usbcam_close_camera_device( hw_device_t *hw_dev)
    {
        ALOGI("%s: device =%p E", __func__, hw_dev);
        int rc =  -1;
        camera_device_t *device     = (camera_device_t *)hw_dev;

        if(device)
        {
            camera_hardware_t *camHal   = (camera_hardware_t *)device->priv;
            if(camHal)
            {
                rc = close(camHal->fd);
                if(rc < 0)
                {
                    ALOGE("%s: close failed ", __func__);
                }
                camHal->fd = 0;
                delete camHal;
            }
            else
            {
                ALOGE("%s: camHal is NULL pointer ", __func__);
            }
        }
		//system("echo 'udisk_unrequest' > /dev/flydev0");	
        ALOGI("%s: X device =%p, rc = %d", __func__, hw_dev, rc);
        return rc;
    }

    static int initDisplayBuffers(camera_hardware_t *camHal)
    {
        preview_stream_ops    *mPreviewWindow;
        struct ion_fd_data    ion_info_fd;
        int                   numMinUndequeuedBufs = 0;
        int                   rc = 0;
        int                   gralloc_usage = 0;
        int                   err;
        int                   color = 30;

        ALOGD("%s: E", __func__);

        if(camHal == NULL)
        {
            ALOGE("%s: camHal = NULL", __func__);
            return -1;
        }

        mPreviewWindow = camHal->window;
        if(!mPreviewWindow)
        {
            ALOGE("%s: mPreviewWindow = NULL", __func__);
            return -1;
        }

        if(mPreviewWindow->get_min_undequeued_buffer_count)
        {
            rc = mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow, &numMinUndequeuedBufs);
            if (0 != rc)
            {
                ALOGE("%s: get_min_undequeued_buffer_count returned error", __func__);
            }
            else
                ALOGD("%s: get_min_undequeued_buffer_count returned: %d ",
                      __func__, numMinUndequeuedBufs);
        }
        else
            ALOGE("%s: get_min_undequeued_buffer_count is NULL pointer", __func__);

        if(mPreviewWindow->set_buffer_count)
        {
            camHal->previewMem.buffer_count = numMinUndequeuedBufs  + PRVW_DISP_BUF_CNT;
            rc = mPreviewWindow->set_buffer_count(mPreviewWindow, camHal->previewMem.buffer_count);
            if (rc != 0)
            {
                ALOGE("%s: set_buffer_count returned error", __func__);
            }
            else
                ALOGD("%s: set_buffer_count returned success", __func__);
        }
        else
            ALOGE("%s: set_buffer_count is NULL pointer", __func__);

        if(mPreviewWindow->set_buffers_geometry)
        {
            rc = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                    camHal->dispWidth,
                    camHal->dispHeight,
                    camHal->dispFormat);
            if (rc != 0)
            {
                ALOGE("%s: set_buffers_geometry returned error. %s (%d)",
                      __func__, strerror(-rc), -rc);
            }
            else
                ALOGD("%s: set_buffers_geometry returned success", __func__);
        }
        else
            ALOGE("%s: set_buffers_geometry is NULL pointer", __func__);

        gralloc_usage = GRALLOC_USAGE_HW_CAMERA_WRITE | GRALLOC_USAGE_PRIVATE_IOMMU_HEAP;

        if(mPreviewWindow->set_usage)
        {
            rc = mPreviewWindow->set_usage(mPreviewWindow, gralloc_usage);
            if (rc != 0)
            {
                ALOGE("%s: set_usage returned error", __func__);
            }
            else
                ALOGD("%s: set_usage returned success", __func__);
        }
        else
            ALOGE("%s: set_usage is NULL pointer", __func__);

        for (int cnt = 0; cnt < camHal->previewMem.buffer_count; cnt++)
        {
            int stride;
            err = mPreviewWindow->dequeue_buffer(
                      mPreviewWindow,
                      &camHal->previewMem.buffer_handle[cnt],
                      &camHal->previewMem.stride[cnt]);
            if(!err)
            {
                ALOGD("%s: dequeue buf: %p\n",
                      __func__, camHal->previewMem.buffer_handle[cnt]);

                if(mPreviewWindow->lock_buffer)
                {
                    err = mPreviewWindow->lock_buffer(
                              mPreviewWindow,
                              camHal->previewMem.buffer_handle[cnt]);
                    ALOGD("%s: mPreviewWindow->lock_buffer success",
                          __func__);
                }

                camHal->previewMem.local_flag[cnt] = BUFFER_UNLOCKED;
                camHal->previewMem.private_buffer_handle[cnt] =
                    (struct private_handle_t *) (*camHal->previewMem.buffer_handle[cnt]);

                ALOGD("%s: idx = %d, fd = %d, size = %d, offset = %d", __func__,
                      cnt, camHal->previewMem.private_buffer_handle[cnt]->fd,
                      camHal->previewMem.private_buffer_handle[cnt]->size,
                      camHal->previewMem.private_buffer_handle[cnt]->offset);

                camHal->previewMem.camera_memory[cnt] =
                    camHal->get_memory(
                        camHal->previewMem.private_buffer_handle[cnt]->fd,
                        camHal->previewMem.private_buffer_handle[cnt]->size,
                        1, camHal->cb_ctxt);

                ALOGD("%s: data = %p, size = %d, handle = %p", __func__,
                      camHal->previewMem.camera_memory[cnt]->data,
                      camHal->previewMem.camera_memory[cnt]->size,
                      camHal->previewMem.camera_memory[cnt]->handle);
#ifdef USE_ION
                camHal->previewMem.mem_info[cnt].main_ion_fd =
                    open("/dev/ion", O_RDONLY);
                if (camHal->previewMem.mem_info[cnt].main_ion_fd < 0)
                {
                    ALOGE("%s: failed: could not open ion device\n", __func__);
                }
                else
                {
                    memset(&ion_info_fd, 0, sizeof(ion_info_fd));
                    ion_info_fd.fd =
                        camHal->previewMem.private_buffer_handle[cnt]->fd;
                    if (ioctl(camHal->previewMem.mem_info[cnt].main_ion_fd,
                              ION_IOC_IMPORT, &ion_info_fd) < 0)
                    {
                        ALOGE("ION import failed\n");
                    }
                }
                camHal->previewMem.mem_info[cnt].fd =
                    camHal->previewMem.private_buffer_handle[cnt]->fd;
                camHal->previewMem.mem_info[cnt].size =
                    camHal->previewMem.private_buffer_handle[cnt]->size;
                camHal->previewMem.mem_info[cnt].handle = ion_info_fd.handle;
#endif
            }
            else
                ALOGE("%s: dequeue buf %d failed \n", __func__, cnt);
        }

        for (int cnt = 0; cnt < camHal->previewMem.buffer_count; cnt++)
        {
            err = mPreviewWindow->cancel_buffer(mPreviewWindow, (buffer_handle_t *)camHal->previewMem.buffer_handle[cnt]);
            if(!err)
            {
                ALOGD("%s: cancel_buffer successful: %p\n",
                      __func__, camHal->previewMem.buffer_handle[cnt]);
            }
            else
                ALOGE("%s: cancel_buffer failed: %p\n", __func__,
                      camHal->previewMem.buffer_handle[cnt]);
        }

        ALOGD("%s: X", __func__);
        return rc;
    }

    static int deInitDisplayBuffers(camera_hardware_t *camHal)
    {
        int rc = 0;
        preview_stream_ops    *previewWindow;

        ALOGD("%s: E", __func__);

        if(!camHal || !camHal->window)
        {
            ALOGE("%s: camHal = NULL or window = NULL ", __func__);
            return -1;
        }

        previewWindow = camHal->window;

        /************************************************************************/
        /* - Release all buffers that were acquired using get_memory            */
        /* - If using ION memory, free ION related resources                    */
        /* - genUnlock if buffer is genLocked                                   */
        /* - Cancel buffers: queue w/o displaying                               */
        /************************************************************************/

        for (int cnt = 0; cnt < camHal->previewMem.buffer_count; cnt++)
        {

            /* Release all buffers that were acquired using get_memory */
            camHal->previewMem.camera_memory[cnt]->release(
                camHal->previewMem.camera_memory[cnt]);
#ifdef USE_ION
            /* If using ION memory, free ION related resources */
            struct ion_handle_data ion_handle;
            memset(&ion_handle, 0, sizeof(ion_handle));
            ion_handle.handle = camHal->previewMem.mem_info[cnt].handle;
            if (ioctl(camHal->previewMem.mem_info[cnt].main_ion_fd,
                      ION_IOC_FREE, &ion_handle) < 0)
            {
                ALOGE("%s: ion free failed\n", __func__);
            }
            close(camHal->previewMem.mem_info[cnt].main_ion_fd);
#endif
            //note: I do not know why it is caused the system crash,so I drop it temprarily.
            /*
            	        rc = previewWindow->cancel_buffer(previewWindow,
            	            (buffer_handle_t *)camHal->previewMem.buffer_handle[cnt]);
            	        if(!rc) {
            	            ALOGD("%s: cancel_buffer successful: %p\n",
            	                 __func__, camHal->previewMem.buffer_handle[cnt]);
            	        }else
            	            ALOGE("%s: cancel_buffer failed: %p\n", __func__,
            	                 camHal->previewMem.buffer_handle[cnt]);
            */
        }
        memset(&camHal->previewMem, 0, sizeof(camHal->previewMem));

        ALOGD("%s: X", __func__);
        return rc;
    }
    int usbcam_set_preview_window(struct camera_device *device,
                                  struct preview_stream_ops *window)
    {
        ALOGD("%s: E", __func__);

		ALOGD("%s: ================window = %p========", __func__ , window);
		
        int rc = 0;
        camera_hardware_t *camHal;

        VALIDATE_DEVICE_HDL(camHal, device, -1);
        Mutex::Autolock autoLock(camHal->lock);

        /* if window is already set, then de-init previous buffers */
        if(camHal->window)
        {
            rc = deInitDisplayBuffers(camHal);
            if(rc < 0)
            {
                ALOGE("%s: deInitDisplayBuffers returned error", __func__);
            }
        }
        camHal->window = window;

        if(camHal->window)
        {
            rc = initDisplayBuffers(camHal);
            if(rc < 0)
            {
                ALOGE("%s: initDisplayBuffers returned error", __func__);
            }
        }
        ALOGI("%s: X. rc = %d", __func__, rc);
        return rc;
    }

    void usbcam_set_CallBacks(struct camera_device *device,
                              camera_notify_callback notify_cb,
                              camera_data_callback data_cb,
                              camera_data_timestamp_callback data_cb_timestamp,
                              camera_request_memory get_memory,
                              void *user)
    {
        ALOGI("%s: E", __func__);
        camera_hardware_t *camHal;

        if(device && device->priv)
        {
            camHal = (camera_hardware_t *)device->priv;
        }
        else
        {
            ALOGE("%s: Null device or device->priv", __func__);
            return;
        }

        Mutex::Autolock autoLock(camHal->lock);
        camHal->notify_cb           = notify_cb;
        camHal->data_cb             = data_cb;
        camHal->data_cb_timestamp   = data_cb_timestamp;
        camHal->get_memory          = get_memory;
        camHal->cb_ctxt             = user;
        ALOGI("%s: X", __func__);
    }

    void usbcam_enable_msg_type(struct camera_device *device, int32_t msg_type)
    {
        if(is_debug)
            ALOGI("%s: msg_type: %d", __func__, msg_type);

        camera_hardware_t *camHal;

        if(device && device->priv)
        {
            camHal = (camera_hardware_t *)device->priv;
        }
        else
        {
            ALOGE("%s: Null device or device->priv", __func__);
            return;
        }

        Mutex::Autolock autoLock(camHal->lock);
        camHal->msgEnabledFlag |= msg_type;
        if(is_debug)
            ALOGI("%s: X", __func__);
    }

    void usbcam_disable_msg_type(struct camera_device *device, int32_t msg_type)
    {
        if(is_debug)
            ALOGI("%s: msg_type: %d", __func__, msg_type);

        camera_hardware_t *camHal;
        if(device && device->priv)
        {
            camHal = (camera_hardware_t *)device->priv;
        }
        else
        {
            ALOGE("%s: Null device or device->priv", __func__);
            return;
        }

        Mutex::Autolock autoLock(camHal->lock);
        camHal->msgEnabledFlag &= ~msg_type;
        if(is_debug)
            ALOGI("%s: X", __func__);
    }

    int usbcam_msg_type_enabled(struct camera_device *device, int32_t msg_type)
    {
        if(is_debug)
            ALOGI("%s: E", __func__);

        camera_hardware_t *camHal;
        if(device && device->priv)
        {
            camHal = (camera_hardware_t *)device->priv;
        }
        else
        {
            ALOGE("%s: Null device or device->priv", __func__);
            return -1;
        }

        Mutex::Autolock autoLock(camHal->lock);
        if(is_debug)
            ALOGI("%s: X", __func__);
        return (camHal->msgEnabledFlag & msg_type);
    }

    static int ioctlLoop(int fd, int ioctlCmd, void *args)
    {
        int rc = -1;

        while(1)
        {
            rc = ioctl(fd, ioctlCmd, args);
            if(!((-1 == rc) && (EINTR == errno)))
                break;
        }
        return rc;
    }
    static int getPreviewCaptureFmt(camera_hardware_t *camHal)
    {
        int     i = 0, mjpegSupported = -1, h264Supported = -1;
        struct v4l2_fmtdesc fmtdesc;
		struct v4l2_frmsizeenum	frmsize;
		int isPrevYUV = 0;
		char isPrevYUV_Str[PROPERTY_VALUE_MAX];

        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));

		for(i = 0; ; i++)
        {
        	frmsize.pixel_format = V4L2_PIX_FMT_YUYV;
            frmsize.index = i;
            if (-1 == ioctlLoop(camHal->fd, VIDIOC_ENUM_FRAMESIZES , &frmsize))
            {
                  ALOGE("%s: VIDIOC_ENUM_FRAMESIZES failed--%d", __func__, i);
				  break;
            }
			if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
			{
				  ALOGE("%s: ==DISCRETE==%dx%d\n", __func__, frmsize.discrete.width, frmsize.discrete.height);
			}
			/*
			if (frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS)
			{
				  ALOGE("%s: ==CONTINUOUS==%dx%d\n", __func__, frmsize.continuous.width, frmsize.continuous.height);
			}
			*/
			if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
			{
                ALOGE("%s: ==STEPWISE==%dx%d\n", __func__,frmsize.stepwise.max_width,frmsize.stepwise.max_height);
            }
        }

        for(i = 0; ; i++)
        {
            fmtdesc.index = i;
            fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == ioctlLoop(camHal->fd, VIDIOC_ENUM_FMT, &fmtdesc))
            {
                if (EINVAL == errno)
                {
                    ALOGD("%s: Queried all formats till index %d\n", __func__, i);
                    break;
                }
                else
                {
                    ALOGE("%s: VIDIOC_ENUM_FMT failed", __func__);
                }
            }
            if(V4L2_PIX_FMT_MJPEG == fmtdesc.pixelformat)
            {
                mjpegSupported = 0;
                ALOGD("%s: V4L2_PIX_FMT_MJPEG is supported", __func__ );
            }
            if(V4L2_PIX_FMT_H264 == fmtdesc.pixelformat)
            {
                h264Supported = 0;
                ALOGD("%s: V4L2_PIX_FMT_H264 is supported", __func__ );
            }
	 	    ALOGD("%s: Capture format supported: 0x%x.",__func__,fmtdesc.pixelformat);

        }

        camHal->captureFormat = V4L2_PIX_FMT_YUYV;
        if(camHal->prevWidth > 640)
        {
            if(1 == mjpegSupported)
                camHal->captureFormat = V4L2_PIX_FMT_MJPEG;
            else if(1 == h264Supported)
                camHal->captureFormat = V4L2_PIX_FMT_H264;
        }
        ALOGI("%s: Capture format chosen: 0x%x. 0x%x:YUYV. 0x%x:MJPEG. 0x%x: H264",
              __func__, camHal->captureFormat, V4L2_PIX_FMT_YUYV,
              V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_H264);
#if 0
		char chooseMJEPG[PROPERTY_VALUE_MAX];
	    property_get("lidbg.uvccam.mjpeg", chooseMJEPG, "0");
	    if((!strncmp(chooseMJEPG, "1", 1)) && (0 == mjpegSupported))
		{
			camHal->captureFormat = V4L2_PIX_FMT_MJPEG;
			ALOGD("-------uvccam captureFormat use MJEPG -----");
		}
		else
		{
			camHal->captureFormat = V4L2_PIX_FMT_YUYV;
			ALOGD("-------uvccam captureFormat use YUYV -----");
		}
#else
		if(0 == mjpegSupported)
		{
			camHal->captureFormat = V4L2_PIX_FMT_MJPEG;
			ALOGE("-------uvccam captureFormat use MJEPG -----");
		}
		else
		{
			camHal->captureFormat = V4L2_PIX_FMT_YUYV;
			ALOGE("-------uvccam captureFormat use YUYV -----");
		}
#endif

		property_get("lidbg.uvccam.isPrevYUV", isPrevYUV_Str, "0");
		isPrevYUV = atoi(isPrevYUV_Str);
		if(isPrevYUV > 0) 
		{
			lidbg("======== Force to Use YUV!=======\n",isPrevYUV);
			camHal->captureFormat = V4L2_PIX_FMT_YUYV;
		}

		return camHal->captureFormat;
        //return V4L2_PIX_FMT_YUYV;
    }
    static int initV4L2mmap(camera_hardware_t *camHal)
    {
        int rc = -1;
        struct v4l2_requestbuffers  reqBufs;
        struct v4l2_buffer          tempBuf;

        ALOGD("%s: E", __func__);
        memset(&reqBufs, 0, sizeof(v4l2_requestbuffers));
        reqBufs.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqBufs.memory  = V4L2_MEMORY_MMAP;
        reqBufs.count   = PRVW_CAP_BUF_CNT;

        if (-1 == ioctlLoop(camHal->fd, VIDIOC_REQBUFS, &reqBufs))
        {
            if (EINVAL == errno)
            {
                ALOGE("%s: does not support memory mapping\n", __func__);
            }
            else
            {
                ALOGE("%s: VIDIOC_REQBUFS failed", __func__);
            }
        }
        ALOGD("%s: VIDIOC_REQBUFS success", __func__);

        if (reqBufs.count < PRVW_CAP_BUF_CNT)
        {
            ALOGE("%s: Insufficient buffer memory on\n", __func__);
        }

        camHal->buffers = ( bufObj * ) calloc(reqBufs.count, sizeof(bufObj));

        if (!camHal->buffers)
        {
            ALOGE("%s: Out of memory\n", __func__);
        }

        for (camHal->n_buffers = 0;
                camHal->n_buffers < reqBufs.count;
                camHal->n_buffers++)
        {
            memset(&tempBuf, 0, sizeof(tempBuf));
            tempBuf.index       = camHal->n_buffers;
            tempBuf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            tempBuf.memory      = V4L2_MEMORY_MMAP;

            if (-1 == ioctlLoop(camHal->fd, VIDIOC_QUERYBUF, &tempBuf))
                ALOGE("%s: VIDIOC_QUERYBUF failed", __func__);

            ALOGD("%s: VIDIOC_QUERYBUF success", __func__);

            camHal->buffers[camHal->n_buffers].len = tempBuf.length;
            camHal->buffers[camHal->n_buffers].data =
                mmap(NULL , tempBuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camHal->fd, tempBuf.m.offset);
            if (MAP_FAILED == camHal->buffers[camHal->n_buffers].data)
                ALOGE("%s: mmap failed", __func__);
        }
        ALOGD("%s: X", __func__);
        return 0;
    }

    int v4l2_vidio_s_ctrl(int fd, const char *idchar, int id, int value)
    {
        struct v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = id;
        ctrl.value = value;
        if (-1 == ioctlLoop(fd, VIDIOC_S_CTRL, &ctrl))
        {
            ALOGE("%s:  [%s]failed,%s", __func__, idchar, strerror(errno));
            return -1;
        }
        else
            ALOGD("%s:  [%s.%d]success", __func__, idchar, value);
        return 0;
    }
    int v4l2_vidio_g_ctrl(int fd, const char *idchar, int id)
    {
        struct v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = id;
        if (-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl))
        {
            ALOGE("%s: [%s] failed,%s", __func__, idchar, strerror(errno));
            return -1;
        }
        else
            ALOGD("%s: [%s] success,%d", __func__, idchar, ctrl.value);
        return 0;
    }

    static int initUsbCamera(camera_hardware_t *camHal, int width, int height,
                             int pixelFormat)
    {
        int mpixelFormat = pixelFormat;
        int     rc = -1;
        struct  v4l2_capability     cap;
        struct  v4l2_cropcap        cropcap;
        struct  v4l2_crop           crop;
        struct  v4l2_format         v4l2format;
        unsigned int                min;
	char startNight[PROPERTY_VALUE_MAX];
	
        ALOGI("%s: E", __func__);

        if (-1 == ioctlLoop(camHal->fd, VIDIOC_QUERYCAP, &cap))
        {
            if (EINVAL == errno)
            {
                ALOGE( "%s: This is not V4L2 device\n", __func__);
                return -1;
            }
            else
            {
                ALOGE("%s: VIDIOC_QUERYCAP errno: %d", __func__, errno);
            }
        }
        ALOGD("%s: VIDIOC_QUERYCAP success", __func__);

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
            ALOGE("%s: This is not video capture device\n", __func__);
            return -1;
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            ALOGE("%s: This does not support streaming i/o\n", __func__);
            return -1;
        }

        if (cap.capabilities & V4L2_CAP_DEVICE_CAPS)
            ALOGE("%s: support:V4L2_CAP_DEVICE_CAPS\n", __func__);
        else
            ALOGE("%s: not.support:V4L2_CAP_DEVICE_CAPS\n", __func__);

        memset(&cropcap, 0, sizeof(cropcap));
        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 == ioctlLoop(camHal->fd, VIDIOC_CROPCAP, &cropcap))
        {
            crop.c = cropcap.defrect;
            crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            ALOGD("%s: VIDIOC_CROPCAP success", __func__);
            if (-1 == ioctlLoop(camHal->fd, VIDIOC_S_CROP, &crop))
            {
                switch (errno)
                {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
                }
            }
            ALOGD("%s: VIDIOC_S_CROP success", __func__);

        }
        else
        {
            /* Errors ignored. */
            ALOGE("%s: VIDIOC_S_CROP failed", __func__);
        }

        //add  for FPS contrl
        {
            struct v4l2_streamparm Stream_Parm;
            memset(&Stream_Parm, 0, sizeof(struct v4l2_streamparm));
            Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (0 == ioctlLoop(camHal->fd, VIDIOC_G_PARM, &Stream_Parm))
            {
                ALOGD("%s: VIDIOC_G_PARM success,%d,%d,%d,%d", __func__, Stream_Parm.parm.capture.capability, Stream_Parm.parm.capture.capturemode,
                      Stream_Parm.parm.capture.timeperframe.denominator, Stream_Parm.parm.capture.timeperframe.numerator);
                Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                Stream_Parm.parm.capture.timeperframe.denominator = 120;;
                Stream_Parm.parm.capture.timeperframe.numerator = 1;
                if (-1 == ioctlLoop(camHal->fd, VIDIOC_S_PARM, &Stream_Parm))
                {
                    switch (errno)
                    {
                    case EINVAL:
                        /* Cropping not supported. */
                        break;
                    default:
                        /* Errors ignored. */
                        break;
                    }
                }
                ALOGD("%s: VIDIOC_S_PARM success", __func__);
                if (0 == ioctlLoop(camHal->fd, VIDIOC_G_PARM, &Stream_Parm))
                    ALOGD("%s: VIDIOC_G_PARM success,%d,%d,%d,%d", __func__, Stream_Parm.parm.capture.capability, Stream_Parm.parm.capture.capturemode,
                          Stream_Parm.parm.capture.timeperframe.denominator, Stream_Parm.parm.capture.timeperframe.numerator);
            }
            else
            {
                ALOGE("%s: VIDIOC_G_PARM failed", __func__);
            }
        }

		ALOGE("%s:Camera driver: %s   Driver version: %d.%d.%d ", __func__ ,cap.driver,
	        (cap.version>>16) & 0xff,(cap.version>>8) & 0xff);

        //v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_EXPOSURE_AUTO", V4L2_CID_EXPOSURE_AUTO,V4L2_EXPOSURE_AUTO );
        v4l2_vidio_g_ctrl(camHal->fd, "V4L2_CID_EXPOSURE_AUTO_PRIORITY", V4L2_CID_EXPOSURE_AUTO_PRIORITY);
        v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_EXPOSURE_AUTO_PRIORITY", V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0);
        v4l2_vidio_g_ctrl(camHal->fd, "V4L2_CID_EXPOSURE", V4L2_CID_EXPOSURE);
        v4l2_vidio_g_ctrl(camHal->fd, "V4L2_CID_EXPOSURE_ABSOLUTE", V4L2_CID_EXPOSURE_ABSOLUTE);
#if 0
		if(cam_id == 1)
		{
			system("./flysystem/lib/out/lidbg_testuvccam /dev/video0 --xuset-mir 0 ");//front cam
			v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_HFLIP", V4L2_CID_HFLIP, 0);
		}
		else if(cam_id == 0)
		{
			system("./flysystem/lib/out/lidbg_testuvccam /dev/video0 --xuset-mir 1 ");//back cam
			v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_HFLIP", V4L2_CID_HFLIP, 1);
		}
		
		system("./flysystem/lib/out/lidbg_testuvccam /dev/video0 --xuset-flip 0 ");
#endif
	//car record enable time osd
#if 1
	lidbg("*****preview cam_id => %d*******",cam_id);
	if(cam_id == 1)
	{
		system("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-oe 1 1 ");
		system("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 1 --xuset-car 0 0 0");
		//lidbg_set_sonix_osd_time();
	}
	else if(cam_id == 0)
	{
		system("setprop lidbg.uvccam.rear.osdset 0&");
		system("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --xuset-oe 0 0 ");
	}
#else
	system("./flysystem/lib/out/lidbg_testuvccam /dev/video1 --xuset-oe 0 0 ");
#endif
		
		//system("./flysystem/lib/out/lidbg_testuvccam /dev/video1 --ef-set nightthread=1 &");
#if 0
	property_get("lidbg.uvccam.nightmode", startNight, "0");

	if( !strncmp(startNight, "1", 1))
	{
		ALOGE("========startNight==========");
		if (v4l2_vidio_s_ctrl (camHal->fd, "V4L2_CID_GAIN" ,V4L2_CID_GAIN, NIGHT_GAINVAL)<0)
			ALOGE("----eho---- : do_gain (%d) Failed", NIGHT_GAINVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_CONTRAST" ,V4L2_CID_CONTRAST, NIGHT_CONTRASTVAL)<0)
			ALOGE("----eho---- : do_contrast (%d) Failed", NIGHT_CONTRASTVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_SATURATION" ,V4L2_CID_SATURATION, NIGHT_SATURATIONVAL)<0)
			ALOGE("----eho---- : do_saturation (%d) Failed", NIGHT_SATURATIONVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_BRIGHTNESS" ,V4L2_CID_BRIGHTNESS, NIGHT_BRIGHTVAL - 64)<0)
			ALOGE("----eho---- : do_bright (%d) Failed", NIGHT_BRIGHTVAL);
		/*
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_EXPOSURE_AUTO" ,V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL)<0)
			ALOGE("----eho---- : do_exposure (%d) Failed", NIGHT_EXPOSUREVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_EXPOSURE_ABSOLUTE" ,V4L2_CID_EXPOSURE_ABSOLUTE, NIGHT_EXPOSUREVAL)<0)
			ALOGE("----eho---- : do_exposure (%d) Failed", NIGHT_EXPOSUREVAL);
		*/
	}
	else if( !strncmp(startNight, "0", 1))
	{
		ALOGE("========startDay==========");
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_GAIN" ,V4L2_CID_GAIN, DAY_GAINVAL)<0)
			ALOGE("----eho---- : do_gain (%d) Failed", DAY_GAINVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_CONTRAST" ,V4L2_CID_CONTRAST, DAY_CONTRASTVAL)<0)
			ALOGE("----eho---- : do_contrast (%d) Failed", DAY_CONTRASTVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_SATURATION" ,V4L2_CID_SATURATION, DAY_SATURATIONVAL)<0)
			ALOGE("----eho---- : do_saturation (%d) Failed", DAY_SATURATIONVAL);
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_BRIGHTNESS" ,V4L2_CID_BRIGHTNESS, DAY_BRIGHTVAL - 64)<0)
			ALOGE("----eho---- : do_bright (%d) Failed", DAY_BRIGHTVAL);
		/*
		if (v4l2_vidio_s_ctrl (camHal->fd,  "V4L2_CID_EXPOSURE_AUTO" ,V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_AUTO)<0)
			ALOGE("----eho---- : do_exposure (%d) Failed", NIGHT_EXPOSUREVAL);
		*/
	}
#endif
        memset(&v4l2format, 0, sizeof(v4l2format));

        v4l2format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        {
            v4l2format.fmt.pix.field       = V4L2_FIELD_ANY;
            //v4l2format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
			v4l2format.fmt.pix.pixelformat = mpixelFormat;
            v4l2format.fmt.pix.width       = width;
            v4l2format.fmt.pix.height      = height;

			ALOGE("%s: 1--VIDIOC_S_FMT ,width=%d,height=%d,pixelformat=0x%x,buffer size: %u", __func__,
				v4l2format.fmt.pix.width,v4l2format.fmt.pix.height,v4l2format.fmt.pix.pixelformat ,v4l2format.fmt.pix.sizeimage);

            if (-1 == ioctlLoop(camHal->fd, VIDIOC_S_FMT, &v4l2format))
            {
                ALOGE("%s: VIDIOC_S_FMT failed", __func__);
                return -1;
            }
						
			ALOGE("%s: 2--VIDIOC_S_FMT success,width=%d,height=%d,pixelformat=0x%x,buffer size: %u", __func__,
				v4l2format.fmt.pix.width,v4l2format.fmt.pix.height,v4l2format.fmt.pix.pixelformat ,v4l2format.fmt.pix.sizeimage);
        }
				
        rc = initV4L2mmap(camHal);
        ALOGI("%s: X", __func__);
        return rc;
    }



    static int startUsbCamCapture(camera_hardware_t *camHal)
    {
        int         rc = -1;
        unsigned    int i;
        enum        v4l2_buf_type   v4l2BufType;
        ALOGD("%s: E", __func__);

        for (i = 0; i < camHal->n_buffers; ++i)
        {
            struct v4l2_buffer tempBuf;

            memset(&tempBuf, 0, sizeof(tempBuf));
            tempBuf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            tempBuf.memory  = V4L2_MEMORY_MMAP;
            tempBuf.index   = i;

            if (-1 == ioctlLoop(camHal->fd, VIDIOC_QBUF, &tempBuf))
                ALOGE("%s: VIDIOC_QBUF for %d buffer failed", __func__, i);
            else
                ALOGD("%s: VIDIOC_QBUF for %d buffer success", __func__, i);
        }

        v4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctlLoop(camHal->fd, VIDIOC_STREAMON, &v4l2BufType))
            ALOGE("%s: VIDIOC_STREAMON failed", __func__);
        else
        {
            ALOGD("%s: VIDIOC_STREAMON success", __func__);
            rc = 0;
        }

        ALOGD("%s: X", __func__);
        return rc;
    }

    static int get_buf_from_display(camera_hardware_t *camHal, int *buffer_id)
    {
        int                     err = 0;
        preview_stream_ops      *mPreviewWindow = NULL;
        int                     stride = 0, cnt = 0;
        buffer_handle_t         *buffer_handle = NULL;
        struct private_handle_t *private_buffer_handle = NULL;
        if(is_debug)
            ALOGD("%s: E", __func__);

        if (camHal == NULL)
        {
            ALOGE("%s: camHal = NULL", __func__);
            return -1;
        }

        mPreviewWindow = camHal->window;
        if( mPreviewWindow == NULL)
        {
            ALOGE("%s: mPreviewWindow = NULL", __func__);
            return -1;
        }
        err = mPreviewWindow->dequeue_buffer(mPreviewWindow,
                                             &buffer_handle,
                                             &stride);
        if(!err)
        {
            if(is_debug)
                ALOGD("%s: dequeue buf buffer_handle: %p\n", __func__, buffer_handle);

            if(is_debug)
                ALOGD("%s: mPreviewWindow->lock_buffer: %p",
                      __func__, mPreviewWindow->lock_buffer);
            if(mPreviewWindow->lock_buffer)
            {
                err = mPreviewWindow->lock_buffer(mPreviewWindow, buffer_handle);
                if(is_debug)
                    ALOGD("%s: mPreviewWindow->lock_buffer success", __func__);
            }

            private_buffer_handle = (struct private_handle_t *)(*buffer_handle);
            if(is_debug)
                ALOGD("%s: fd = %d, size = %d, offset = %d, stride = %d", __func__, private_buffer_handle->fd,
                      private_buffer_handle->size, private_buffer_handle->offset, stride);

            for(cnt = 0; cnt < camHal->previewMem.buffer_count + 2; cnt++)
            {
                if(private_buffer_handle->fd ==
                        camHal->previewMem.private_buffer_handle[cnt]->fd)
                {
                    *buffer_id = cnt;
                    if(is_debug)
                        ALOGD("%s: deQueued fd = %d, index: %d",
                              __func__, private_buffer_handle->fd, cnt);
                    break;
                }
            }
        }
        else
            ALOGE("%s: dequeue buf failed \n", __func__);
        if(is_debug)
            ALOGD("%s: X", __func__);

        return err;
    }

    static int get_buf_from_cam(camera_hardware_t *camHal)
    {
        int rc = -1;
        if(is_debug)
            ALOGD("%s: E", __func__);
        {
            memset(&camHal->curCaptureBuf, 0, sizeof(camHal->curCaptureBuf));

            camHal->curCaptureBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            camHal->curCaptureBuf.memory = V4L2_MEMORY_MMAP;

            if (-1 == ioctlLoop(camHal->fd, VIDIOC_DQBUF, &camHal->curCaptureBuf))
            {
                switch (errno)
                {
                case EAGAIN:
                    ALOGE("%s: EAGAIN error", __func__);
                    return 1;
                case EIO:
                    /* Could ignore EIO, see spec. */
                default:
                    ALOGE("%s: VIDIOC_DQBUF error", __func__);
                }
            }
            else
            {
                rc = 0;
                if(is_debug)
                    ALOGD("%s: VIDIOC_DQBUF: %d successful, %d bytes",
                          __func__, camHal->curCaptureBuf.index,
                          camHal->curCaptureBuf.bytesused);
            }
        }
        if(is_debug)
            ALOGD("%s: X", __func__);
        return rc;
    }

    static int fileDump(const char *fn, char *data, int length, int *frm_cnt)
    {

        FILE *fp = NULL;
        if (0 == *frm_cnt)
        {
            fp = fopen(fn, "wb");
            if (NULL == fp)
            {
                ALOGE("%s: 1Error in opening %s", __func__, fn);
            }
            fclose(fp);
        }
        fp = fopen(fn, "ab");
        if (NULL == fp)
        {
            ALOGE("%s: 2Error in opening %s", __func__, fn);
        }
        fwrite(data, 1, length, fp);
        fclose(fp);
        (*frm_cnt)++;
        ALOGD("%s: Written %d bytes for frame:%d, in %s",
              __func__, length, *frm_cnt, fn);

        return 0;
    }

    static int convert_YUYV_to_420_NV12(char *in_buf, char *out_buf, int wd, int ht)
    {
        int rc = 0;
        int row, col, uv_row;

        if(in_buf == NULL || out_buf == NULL)
        {
            ALOGD("%s: E.in_buf==NULL||out_buf==NULL,return -1", __func__);
            return -1;
        }
        if(is_debug)
            ALOGD("%s: E", __func__);

        /* Arrange Y */
        for(row = 0; row < ht; row++)
            for(col = 0; col < wd * 2; col += 2)
            {
                out_buf[row * wd + col / 2] = in_buf[row * wd * 2 + col];
            }

        /* Arrange UV */
        for(row = 0, uv_row = ht; row < ht; row += 2, uv_row++)
            for(col = 1; col < wd * 2; col += 4)
            {
                out_buf[uv_row * wd + col / 2] = in_buf[row * wd * 2 + col + 2];
                out_buf[uv_row * wd + col / 2 + 1]  = in_buf[row * wd * 2 + col];
            }
        if(is_debug)
            ALOGD("%s: X", __func__);
        return rc;
    }
    static int convert_data_frm_cam_to_disp(camera_hardware_t *camHal, int buffer_id)
    {
        int rc = -1;

        if(!camHal)
        {
            ALOGE("%s: camHal is NULL", __func__);
            return -1;
        }
        if(is_debug)
            ALOGD("%s: E", __func__);

        if(HAL_PIXEL_FORMAT_YCrCb_420_SP == camHal->dispFormat)
        {
			if(V4L2_PIX_FMT_YUYV == camHal->captureFormat)
			{
				if(is_debug)
					ALOGD("%s: E.convert_YUYV_to_420_NV12", __func__);
				convert_YUYV_to_420_NV12(
					(char *)camHal->buffers[camHal->curCaptureBuf.index].data,
					(char *)camHal->previewMem.camera_memory[buffer_id]->data,
					camHal->prevWidth,
					camHal->prevHeight);

			}
			if(V4L2_PIX_FMT_MJPEG == camHal->captureFormat)
			{
				if(is_debug)
					ALOGD("%s: E.convert_JPEG_to_420_NV12", __func__);
				char *ptmp = (char *)calloc(camHal->buffers[camHal->curCaptureBuf.index].len, sizeof(char));
				memcpy(ptmp, camHal->buffers[camHal->curCaptureBuf.index].data, camHal->buffers[camHal->curCaptureBuf.index].len);
				jpeg_init_decoder(camHal->prevWidth, camHal->prevHeight);
				jpeg_decode(
					(uint8_t *)camHal->buffers[camHal->curCaptureBuf.index].data,
					(uint8_t *)ptmp,
					camHal->curCaptureBuf.bytesused);
				convert_YUYV_to_420_NV12(
					(char *)ptmp,
					(char *)camHal->previewMem.camera_memory[buffer_id]->data,
					camHal->prevWidth,
					camHal->prevHeight);
					free(ptmp);
			}
			if(is_debug)
				ALOGD("%s: Copied %d bytes from camera buffer %d to display buffer: %d",
					__func__, camHal->curCaptureBuf.bytesused,
					camHal->curCaptureBuf.index, buffer_id);
			rc = 0;
		}
        if(is_debug)
            ALOGD("%s: X", __func__);
        return rc;
    }



    int cache_ops1(QCameraHalMemInfo_t *mem_info,  void *buf_ptr, unsigned int cmd)
    {
        struct ion_flush_data cache_inv_data;
        struct ion_custom_data custom_data;
        int ret = MM_CAMERA_OK;

#ifdef USE_ION
        if (NULL == mem_info)
        {
            ALOGE("%s: mem_info is NULL, return here", __func__);
            return -1;
        }
        memset(&cache_inv_data, 0, sizeof(cache_inv_data));
        memset(&custom_data, 0, sizeof(custom_data));
        cache_inv_data.vaddr = buf_ptr;
        cache_inv_data.fd = mem_info->fd;
        cache_inv_data.handle = mem_info->handle;
        cache_inv_data.length = mem_info->size;
        custom_data.cmd = cmd;
        custom_data.arg = (unsigned long)&cache_inv_data;
        if(is_debug)
            ALOGD("%s: addr = %p, fd = %d, handle = %p length = %d, ION Fd = %d",
                  __func__, cache_inv_data.vaddr, cache_inv_data.fd,
                  cache_inv_data.handle, cache_inv_data.length,
                  mem_info->main_ion_fd);
        if(mem_info->main_ion_fd > 0)
        {
            if(ioctl(mem_info->main_ion_fd, ION_IOC_CUSTOM, &custom_data) < 0)
            {
                ALOGE("%s: Cache Invalidate failed\n", __func__);
                ret = -1;
            }
        }
#endif

        return ret;
    }
    static int put_buf_to_display(camera_hardware_t *camHal, int buffer_id)
    {
        int err = 0;
        preview_stream_ops    *mPreviewWindow;
        if(is_debug)
            ALOGD("%s: E", __func__);

        if (camHal == NULL)
        {
            ALOGE("%s: camHal = NULL", __func__);
            return -1;
        }

        mPreviewWindow = camHal->window;
        if( mPreviewWindow == NULL)
        {
            ALOGE("%s: mPreviewWindow = NULL", __func__);
            return -1;
        }

        cache_ops1(&camHal->previewMem.mem_info[buffer_id],
                   (void *)camHal->previewMem.camera_memory[buffer_id]->data,
                   ION_IOC_CLEAN_CACHES);
        err = mPreviewWindow->enqueue_buffer(mPreviewWindow,
                                             (buffer_handle_t *)camHal->previewMem.buffer_handle[buffer_id]);
        if(!err)
        {
            if(is_debug)
                ALOGD("%s: enqueue buf successful: %p\n",
                      __func__, camHal->previewMem.buffer_handle[buffer_id]);
        }
        else
            ALOGE("%s: enqueue buf failed: %p\n",
                  __func__, camHal->previewMem.buffer_handle[buffer_id]);
        if(is_debug)
            ALOGD("%s: X", __func__);

        return err;
    }

    static int put_buf_to_cam(camera_hardware_t *camHal)
    {
        if(is_debug)
            ALOGD("%s: E", __func__);

        camHal->curCaptureBuf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        camHal->curCaptureBuf.memory      = V4L2_MEMORY_MMAP;
        if (-1 == ioctlLoop(camHal->fd, VIDIOC_QBUF, &camHal->curCaptureBuf))
        {
            ALOGE("%s: VIDIOC_QBUF failed ", __func__);
            return 1;
        }
        if(is_debug)
            ALOGD("%s: X", __func__);
        return 0;
    }
    static void *previewloop(void *hcamHal)
    {
        int                 rc, threadPriority;
        int                 buffer_id   = 0;
        pid_t               tid         = 0;
        camera_hardware_t   *camHal     = NULL;
        int                 msgType     = 0;
        camera_memory_t     *data       = NULL;
        camera_frame_metadata_t *metadata = NULL;
        camera_memory_t     *previewMem = NULL;

        static int loop_count = 0;
        struct timespec start ;
        start.tv_sec = start.tv_nsec = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);


        camHal = (camera_hardware_t *)hcamHal;
        if(is_debug)
            ALOGD("%s: E", __func__);

        if(!camHal)
        {
            ALOGE("%s: camHal is NULL", __func__);
            return NULL ;
        }


        //tid = androidGetTid();
        tid  = gettid();
        androidSetThreadPriority(tid, ANDROID_PRIORITY_AUDIO);
        prctl(PR_SET_NAME, (unsigned long)"Camera HAL preview thread", 0, 0, 0);
        threadPriority = androidGetThreadPriority(tid);
        while(1)
        {
            fd_set fds;
            struct timeval tv;
            int r = 0;

            loop_count++;
            if(loop_count >= 100)//
            {
                int diff, FPS;
                struct timespec stop ;
                stop.tv_sec = stop.tv_nsec = 0;
                clock_gettime(CLOCK_MONOTONIC, &stop);
                diff = (stop.tv_sec * 1000 + stop.tv_nsec / 1000000) - (start.tv_sec * 1000 + start.tv_nsec / 1000000);
                FPS = 1000 * loop_count / diff;
                ALOGE("%s.%d: FPS.%d,[%d,%d ms]\n", __func__, threadPriority, FPS, loop_count, diff);
                loop_count = 0;
                clock_gettime(CLOCK_MONOTONIC, &start);
                //usleep(670 * 1000);
                //continue;
            }

            FD_ZERO(&fds);
            FD_SET(camHal->fd, &fds);

            tv.tv_sec = 0;
            tv.tv_usec = 500000;
            if(is_debug)
                ALOGD("%s: b4 select on camHal->fd + 1,fd: %d", __func__, camHal->fd);
            r = select(camHal->fd + 1, &fds, NULL, NULL, &tv);
            if(is_debug)
                ALOGD("%s: after select : %d", __func__, camHal->fd);

            if (-1 == r)
            {
                if (EINTR == errno)
                    continue;
                ALOGE("%s: FDSelect error: %d", __func__, errno);
            }

            if (0 == r)
            {
                ALOGD("%s: select timeout\n", __func__);
            }

            Mutex::Autolock autoLock(camHal->lock);

            if(camHal->prvwCmdPending)
            {
                camHal->prvwCmdPending--;
                if(USB_CAM_PREVIEW_EXIT == camHal->prvwCmd)
                {
                    camHal->lock.unlock();
                    ALOGI("%s: Exiting coz USB_CAM_PREVIEW_EXIT", __func__);
                    return (void *)0;
                }
                else if(USB_CAM_PREVIEW_TAKEPIC == camHal->prvwCmd)
                {
                    // rc = prvwThreadTakePictureInternal(camHal);
                    rc = 1;
                    if(rc)
                        ALOGE("%s: prvwThreadTakePictureInternal returned error",
                              __func__);
                }
            }

            /* Null check on preview window. If null, sleep */
            if(!camHal->window)
            {
                ALOGD("%s: sleeping coz camHal->window = NULL", __func__);
                camHal->lock.unlock();
                sleep(2);
                continue;
            }

            if(0 == get_buf_from_display(camHal, &buffer_id))
            {
                if(is_debug)
                    ALOGD("%s: get_buf_from_display success: %d",
                          __func__, buffer_id);
            }
            else
            {
                ALOGE("%s: get_buf_from_display failed. Skipping the loop",
                      __func__);
                continue;
            }

            if (0 == get_buf_from_cam(camHal))
            {
                if(is_debug)
                    ALOGD("%s: get_buf_from_cam success", __func__);
            }
            else
			{
			    camHal->lock.unlock();
			    camHal->notify_cb(CAMERA_MSG_ERROR, 0, 0, camHal->cb_ctxt);
			    camHal->lock.lock();
				ALOGE("%s: get_buf_from_cam error", __func__);
				return (void *)0;
			}              

            convert_data_frm_cam_to_disp(camHal, buffer_id);
            if(is_debug)
                ALOGD("%s: 2222Copied data to buffer_id: %d", __func__, buffer_id);

            if(0 == put_buf_to_display(camHal, buffer_id))
            {
                if(is_debug)
                    ALOGD("%s: put_buf_to_display success: %d", __func__, buffer_id);
            }
            else
                ALOGE("%s: put_buf_to_display error", __func__);

            if(0 == put_buf_to_cam(camHal))
            {
                if(is_debug)
                    ALOGD("%s: put_buf_to_cam success", __func__);
            }
            else
			//if (camHal->msgEnabledFlag & CAMERA_MSG_SHUTTER)
			{
			    camHal->lock.unlock();
			    camHal->notify_cb(CAMERA_MSG_ERROR, 1, 0, camHal->cb_ctxt);
			    camHal->lock.lock();
				ALOGE("%s: put_buf_to_cam error", __func__);
				return (void *)0;
			}

            /* TBD: change the 1.5 hardcoding to Bytes Per Pixel */
            int previewBufSize = camHal->prevWidth * camHal->prevHeight * 1.5;
            msgType |=  CAMERA_MSG_PREVIEW_FRAME;
            if(previewBufSize !=
                    camHal->previewMem.private_buffer_handle[buffer_id]->size)
            {

                previewMem = camHal->get_memory(
                                 camHal->previewMem.private_buffer_handle[buffer_id]->fd,
                                 previewBufSize,
                                 1,
                                 camHal->cb_ctxt);

                if (!previewMem || !previewMem->data)
                {
                    ALOGE("%s: get_memory failed.\n", __func__);
                }
                else
                {
                    data = previewMem;
                    if(is_debug)
                        ALOGD("%s: GetMemory successful. data = %p",
                              __func__, data);
                    if(is_debug)
                        ALOGD("%s: previewBufSize = %d, priv_buf_size: %d",
                              __func__, previewBufSize,
                              camHal->previewMem.private_buffer_handle[buffer_id]->size);
                }
            }
            else
            {
                data =   camHal->previewMem.camera_memory[buffer_id];
                ALOGD("%s: No GetMemory, no invalid fmt. data = %p, idx=%d",
                      __func__, data, buffer_id);
            }
            /* Unlock and lock around the callback. */
            /* Sometimes 'disable_msg' is issued in the callback context, */
            /* leading to deadlock */
            camHal->lock.unlock();
            if((camHal->msgEnabledFlag & CAMERA_MSG_PREVIEW_FRAME) &&
                    camHal->data_cb)
            {
                if(is_debug)
                    ALOGD("%s: before data callback", __func__);
                camHal->data_cb(msgType, data, 0, metadata, camHal->cb_ctxt);
                if(is_debug)
                    ALOGD("%s: after data callback: %p", __func__, camHal->data_cb);
            }
            camHal->lock.lock();
            if (previewMem)
                previewMem->release(previewMem);
        }
        if(is_debug)
            ALOGD("%s: X", __func__);
        return (void *)0;
    }


    static int launch_preview_thread(camera_hardware_t *camHal)
    {
        ALOGD("%s: E", __func__);
        int rc = 0;

        if(!camHal)
        {
            ALOGE("%s: camHal is NULL", __func__);
            return -1;
        }

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&camHal->previewThread, &attr, previewloop, camHal);

        ALOGD("%s: X", __func__);
        return rc;
    }
    int usbcam_start_preview(struct camera_device *device)
    {
        ALOGI("%s: E", __func__);

        int rc = -1;
        camera_hardware_t *camHal = NULL;
        VALIDATE_DEVICE_HDL(camHal, device, -1);
        Mutex::Autolock autoLock(camHal->lock);

        //return while there is not exsit a uvc camera.
        if(camHal->fd < 0)
        {
            ALOGI("%s: no uvc device:return", __func__);
            return -1;
        }

        /* If preivew is already running, nothing to be done */
        if(camHal->previewEnabledFlag)
        {
            ALOGI("%s: Preview is already running", __func__);
            return -1;
        }

        rc = initUsbCamera(camHal, camHal->prevWidth,
                           camHal->prevHeight, getPreviewCaptureFmt(camHal));
        if(rc < 0)
        {
            ALOGE("%s: Failed to intialize the device", __func__);
        }
        else
        {
            rc = startUsbCamCapture(camHal);
            if(rc < 0)
            {
                ALOGE("%s: Failed to startUsbCamCapture", __func__);
            }
            else
            {
                rc = launch_preview_thread(camHal);
                if(rc < 0)
                {
                    ALOGE("%s: Failed to launch_preview_thread", __func__);
                }
            }
        }

        /* if no errors, then set the flag */
        if(!rc)
            camHal->previewEnabledFlag = 1;

        ALOGD("%s: X", __func__);
        return rc;
    }



    static int stopUsbCamCapture(camera_hardware_t *camHal)
    {
        int         rc = -1;
        unsigned    int i;
        enum        v4l2_buf_type   v4l2BufType;
        ALOGD("%s: E", __func__);

        if(!camHal->fd)
        {
            ALOGE("%s: camHal->fd = NULL ", __func__);
            return -1;
        }
        v4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctlLoop(camHal->fd, VIDIOC_STREAMOFF, &v4l2BufType))
        {
            ALOGE("%s: VIDIOC_STREAMOFF failed", __func__);
            rc = -1;
        }
        else
        {
            ALOGD("%s: VIDIOC_STREAMOFF success", __func__);
            rc = 0;
        }

        ALOGD("%s: X", __func__);
        return rc;
    }
    static int unInitV4L2mmap(camera_hardware_t *camHal)
    {
        unsigned int i;
        int rc = 0;
        ALOGD("%s: E", __func__);

        for (i = 0; i < camHal->n_buffers; i++)
            if (-1 == munmap(camHal->buffers[i].data, camHal->buffers[i].len))
            {
                ALOGE("%s: munmap failed for buffer: %d", __func__, i);
                rc = -1;
            }

        ALOGD("%s: X", __func__);
        return rc;
    }
    static int stopPreviewInternal(camera_hardware_t *camHal)
    {
        int rc = 0;
        ALOGD("%s: E", __func__);

        if(camHal->previewEnabledFlag)
        {
            camHal->prvwCmdPending++;
            camHal->prvwCmd         = USB_CAM_PREVIEW_EXIT;

            /* yield lock while waiting for the preview thread to exit */
            camHal->lock.unlock();
            if(pthread_join(camHal->previewThread, NULL))
            {
                ALOGE("%s: Error in pthread_join preview thread", __func__);
            }
            camHal->lock.lock();

            if(stopUsbCamCapture(camHal))
            {
                ALOGE("%s: Error in stopUsbCamCapture", __func__);
                rc = -1;
            }
            if(unInitV4L2mmap(camHal))
            {
                ALOGE("%s: Error in stopUsbCamCapture", __func__);
                rc = -1;
            }
            camHal->previewEnabledFlag = 0;
        }

        ALOGD("%s: X, rc: %d", __func__, rc);
        return rc;
    }
    void usbcam_stop_preview(struct camera_device *device)
    {
        ALOGD("%s: E", __func__);

        int rc = 0;
        camera_hardware_t *camHal;

        if(device && device->priv)
        {
            camHal = (camera_hardware_t *)device->priv;
        }
        else
        {
            ALOGE("%s: Null device or device->priv", __func__);
            return;
        }

        Mutex::Autolock autoLock(camHal->lock);

        rc = stopPreviewInternal(camHal);
        if(rc)
            ALOGE("%s: stopPreviewInternal returned error", __func__);

        ALOGI("%s: X", __func__);
        return;
    }
    int usbcam_preview_enabled(struct camera_device *device)
    {
        ALOGI("%s: E", __func__);
        camera_hardware_t *camHal;

        if(device && device->priv)
        {
            camHal = (camera_hardware_t *)device->priv;
        }
        else
        {
            ALOGE("%s: Null device or device->priv", __func__);
            return -1;
        }
        Mutex::Autolock autoLock(camHal->lock);

        ALOGI("%s: X", __func__);
        return camHal->previewEnabledFlag;
    }

    int usbcam_store_meta_data_in_buffers(struct camera_device *device, int enable)
    {
        struct camera_device *mdevice = device;
        int menable = enable;
        ALOGD("%s: E", __func__);
        int rc = 0;

        ALOGD("%s: X", __func__);
        return rc;
    }

    int usbcam_start_recording(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        int rc = 0;
        char startRecording[PROPERTY_VALUE_MAX];
        ALOGD("%s: E", __func__);
        property_get("lidbg.uvccam.dvr.recording", startRecording, "0");
        if(!strncmp(startRecording, "1", 1))
        {
            ALOGI("-------uvccam recording -----");
            system("./flysystem/lib/out/lidbg_testuvccam /dev/video2 -b 1 -c -f H264 -r &");
            property_set("lidbg.uvccam.dvr.recording", "1");
        }
        ALOGD("%s: X", __func__);
        return 0;
    }

    void usbcam_stop_recording(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        ALOGD("%s: E", __func__);
        ALOGI("-------uvccam stop_recording -----");
        property_set("lidbg.uvccam.dvr.recording", "0");
        ALOGD("%s: X", __func__);
    }

    int usbcam_recording_enabled(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        int rc = 0;
        ALOGD("%s: E", __func__);

        ALOGD("%s: X", __func__);
        return 0;
    }

    void usbcam_release_recording_frame(struct camera_device *device,
                                        const void *opaque)
    {
        struct camera_device *mdevice = device;
        const void *mopaque = opaque;
        ALOGD("%s: E", __func__);

        ALOGD("%s: X", __func__);
    }

    int usbcam_auto_focus(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        ALOGD("%s: E", __func__);
        int rc = 0;

        ALOGD("%s: X", __func__);
        return rc;
    }

    int usbcam_cancel_auto_focus(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        int rc = 0;
        ALOGD("%s: E", __func__);

        ALOGD("%s: X", __func__);
        return 0;
    }


    static int allocate_ion_memory(QCameraHalMemInfo_t *mem_info, int ion_type)
    {
        int                         rc = 0;
        struct ion_handle_data      handle_data;
        struct ion_allocation_data  alloc;
        struct ion_fd_data          ion_info_fd;
        int                         main_ion_fd = 0;

        main_ion_fd = open("/dev/ion", O_RDONLY);
        if (main_ion_fd <= 0)
        {
            ALOGE("Ion dev open failed %s\n", strerror(errno));
            goto ION_OPEN_FAILED;
        }

        memset(&alloc, 0, sizeof(alloc));
        alloc.len = mem_info->size;
        /* to make it page size aligned */
        alloc.len = (alloc.len + 4095) & (~4095);
        alloc.align = 4096;
        alloc.flags = ION_FLAG_CACHED;
#ifdef PLATFORM_msm8909
        alloc.heap_id_mask = ion_type;
#else
	alloc.heap_mask = ion_type;
#endif
        rc = ioctl(main_ion_fd, ION_IOC_ALLOC, &alloc);
        if (rc < 0)
        {
            ALOGE("ION allocation failed\n");
            goto ION_ALLOC_FAILED;
        }

        memset(&ion_info_fd, 0, sizeof(ion_info_fd));
        ion_info_fd.handle = alloc.handle;
        rc = ioctl(main_ion_fd, ION_IOC_SHARE, &ion_info_fd);
        if (rc < 0)
        {
            ALOGE("ION map failed %s\n", strerror(errno));
            goto ION_MAP_FAILED;
        }

        mem_info->main_ion_fd = main_ion_fd;
        mem_info->fd = ion_info_fd.fd;
        mem_info->handle = ion_info_fd.handle;
        mem_info->size = alloc.len;
        return 0;

ION_MAP_FAILED:
        memset(&handle_data, 0, sizeof(handle_data));
        handle_data.handle = ion_info_fd.handle;
        ioctl(main_ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
        close(main_ion_fd);
ION_OPEN_FAILED:
        return -1;
    }
    static int deallocate_ion_memory(QCameraHalMemInfo_t *mem_info)
    {
        struct ion_handle_data handle_data;
        int rc = 0;

        if (mem_info->fd > 0)
        {
            close(mem_info->fd);
            mem_info->fd = 0;
        }

        if (mem_info->main_ion_fd > 0)
        {
            memset(&handle_data, 0, sizeof(handle_data));
            handle_data.handle = mem_info->handle;
            ioctl(mem_info->main_ion_fd, ION_IOC_FREE, &handle_data);
            close(mem_info->main_ion_fd);
            mem_info->main_ion_fd = 0;
        }
        return rc;
    }
    int encodeJpeg(camera_hardware_t *camHal)
    {
        camera_hardware_t *mcamHal = camHal;
        int                 rc = 0;
#if 0

        mm_jpeg_ops_t       mmJpegOps;
        int                 jpegEncHdl  = 0;
        mm_jpeg_job         mmJpegJob;
        src_image_buffer_info   *srcBuf = NULL;
        QCameraHalMemInfo_t jpegInMemInfo;
        camera_memory_t    *jpegInMem;
        uint32_t            jobId;

        ALOGI("%s: E", __func__);

        /************************************************************************/
        /* - Allocate Jpeg input buffer from ION memory                         */
        /************************************************************************/
        jpegInMemInfo.size = camHal->pictWidth * camHal->pictHeight * 2;
        rc = allocate_ion_memory(&jpegInMemInfo,
                                 ((0x1 << CAMERA_ZSL_ION_HEAP_ID) |
                                  (0x1 << CAMERA_ZSL_ION_FALLBACK_HEAP_ID)));
        ERROR_CHECK_EXIT(rc, "allocate_ion_memory");
        jpegInMem = camHal->get_memory(
                        jpegInMemInfo.fd, jpegInMemInfo.size, 1, camHal->cb_ctxt);
        if(!jpegInMem)
        {
            ALOGE("%s: get_mem failed", __func__);
            return -1;
        }

        rc = convert_YUYV_to_420_NV12(
                 (char *)camHal->buffers[camHal->curCaptureBuf.index].data,
                 (char *)jpegInMem->data, camHal->pictWidth, camHal->pictHeight);
        ERROR_CHECK_EXIT(rc, "convert_YUYV_to_420_NV12");
        /************************************************************************/
        /* - Populate JPEG encoding parameters from the camHal context          */
        /************************************************************************/
        memset(&mmJpegJob, 0, sizeof(mmJpegJob));

        mmJpegJob.job_type              = JPEG_JOB_TYPE_ENCODE;
        mmJpegJob.encode_job.jpeg_cb    = jpegEncodeCb;
        mmJpegJob.encode_job.userdata   = (void *)camHal;
        /* TBD: Rotation to be set from settings sent from app */
        mmJpegJob.encode_job.encode_parm.rotation           = 0;
        mmJpegJob.encode_job.encode_parm.exif_numEntries    = 0;
        mmJpegJob.encode_job.encode_parm.exif_data          = NULL;

        /* TBD: Add thumbnail support */
        mmJpegJob.encode_job.encode_parm.buf_info.src_imgs.src_img_num = 1;
        mmJpegJob.encode_job.encode_parm.buf_info.src_imgs.is_video_frame = 0;

        /* Fill main image information */
        srcBuf = &mmJpegJob.encode_job.encode_parm.buf_info.src_imgs.src_img[0];
        srcBuf->type                = JPEG_SRC_IMAGE_TYPE_MAIN;
        srcBuf->img_fmt             = JPEG_SRC_IMAGE_FMT_YUV;
        /* TBD: convert from YUYV to CRCBH2V2 */
        srcBuf->color_format        = MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2;
        srcBuf->num_bufs            = 1;
        srcBuf->src_image[0].fd        = jpegInMemInfo.fd;
        srcBuf->src_image[0].buf_vaddr = (uint8_t *)jpegInMem->data;
        //srcBuf->src_image[0].offset    = 0;
        srcBuf->src_dim.width       = camHal->pictWidth;
        srcBuf->src_dim.height      = camHal->pictHeight;
        srcBuf->out_dim.width       = camHal->pictWidth;
        srcBuf->out_dim.height      = camHal->pictHeight;
        srcBuf->crop.offset_x       = 0;
        srcBuf->crop.offset_y       = 0;
        srcBuf->crop.width          = srcBuf->src_dim.width;
        srcBuf->crop.height         = srcBuf->src_dim.height;
        srcBuf->quality             = camHal->pictJpegQlty;

        /* TBD:Fill thumbnail image information */

        /* Fill out buf information */
        mmJpegJob.encode_job.encode_parm.buf_info.sink_img.buf_vaddr =
            (uint8_t *)camHal->pictMem.camera_memory[0]->data;
        mmJpegJob.encode_job.encode_parm.buf_info.sink_img.fd = 0;
        /* TBD: hard coded for 1.5 bytes per pixel */
        mmJpegJob.encode_job.encode_parm.buf_info.sink_img.buf_len =
            camHal->pictWidth * camHal->pictHeight * 1.5;

        /************************************************************************/
        /* - Initialize jpeg encoder and call Jpeg encoder start                */
        /************************************************************************/
        memset(&mmJpegOps, 0, sizeof(mm_jpeg_ops_t));
        jpegEncHdl = jpeg_open(&mmJpegOps);
        if(!jpegEncHdl)
        {
            ALOGE("%s: Failed to open Jpeg Encoder instance", __func__);
        }
        else
            ALOGD("%s: jpegEncHdl = %d", __func__, jpegEncHdl);

        camHal->jpegEncInProgress = 1;
        rc = mmJpegOps.start_job(jpegEncHdl, &mmJpegJob, &jobId);

        /************************************************************************/
        /* - Wait for JPEG encoder to complete encoding                         */
        /************************************************************************/
        pthread_mutex_init(&camHal->jpegEncMutex, NULL);
        pthread_cond_init(&camHal->jpegEncCond, NULL);

        pthread_mutex_lock(&camHal->jpegEncMutex);
        while(camHal->jpegEncInProgress)
            pthread_cond_wait(&camHal->jpegEncCond, &camHal->jpegEncMutex);
        pthread_mutex_unlock(&camHal->jpegEncMutex);

        /************************************************************************/
        /* - De-allocate Jpeg input buffer from ION memory                      */
        /************************************************************************/
        if(jpegInMem)
            jpegInMem->release(jpegInMem);

        rc = deallocate_ion_memory(&jpegInMemInfo);
        if(rc)
            ALOGE("%s: ION memory de-allocation failed", __func__);
#endif
        ALOGI("%s: X rc = %d", __func__, rc);
        return rc;
    }
    static void *takePictureThread(void *hcamHal)
    {
        int                 rc = 0;
        int                 buffer_id   = 0;
        pid_t               tid         = 0;
        camera_hardware_t   *camHal     = NULL;
        int                 msgType     = 0;
        int                 jpegLength  = 0;
        QCameraHalMemInfo_t *mem_info   = NULL;

        camHal = (camera_hardware_t *)hcamHal;
        ALOGI("%s: E", __func__);

        if(!camHal)
        {
            ALOGE("%s: camHal is NULL", __func__);
            return NULL ;
        }

        tid  = gettid();
        /* TBR: Set appropriate thread priority */
        androidSetThreadPriority(tid, ANDROID_PRIORITY_NORMAL);
        prctl(PR_SET_NAME, (unsigned long)"Camera HAL preview thread", 0, 0, 0);

        /************************************************************************/
        /* - If requested for shutter notfication, notify                       */
        /* - Initialize USB camera with snapshot parameters                     */
        /* - Time wait (select) on camera fd for camera frame availability      */
        /* - Dequeue capture buffer from USB camera                             */
        /* - Send capture buffer to JPEG encoder for JPEG compression           */
        /* - If jpeg frames callback is requested, callback with jpeg buffers   */
        /* - Enqueue capture buffer back to USB camera                          */
        /* - Free USB camera resources and close camera                         */
        /* - If preview was stopped for taking picture, restart the preview     */
        /************************************************************************/

        Mutex::Autolock autoLock(camHal->lock);
        /************************************************************************/
        /* - If requested for shutter notfication, notify                       */
        /************************************************************************/
#if 0 /* TBD: Temporarily commented out due to an issue. Sometimes it takes */
        /* long time to get back the lock once unlocked and notify callback */
        if (camHal->msgEnabledFlag & CAMERA_MSG_SHUTTER)
        {
            camHal->lock.unlock();
            camHal->notify_cb(CAMERA_MSG_SHUTTER, 0, 0, camHal->cb_ctxt);
            camHal->lock.lock();
        }
#endif
        /************************************************************************/
        /* - Initialize USB camera with snapshot parameters                     */
        /************************************************************************/
        USB_CAM_OPEN(camHal);


        rc = initUsbCamera(camHal, camHal->pictWidth, camHal->pictHeight,
                           V4L2_PIX_FMT_YUYV);

        ERROR_CHECK_EXIT_THREAD(rc, "initUsbCamera");

        rc = startUsbCamCapture(camHal);
        ERROR_CHECK_EXIT_THREAD(rc, "startUsbCamCapture");

        /************************************************************************/
        /* - Time wait (select) on camera fd for camera frame availability      */
        /************************************************************************/
        {
            fd_set fds;
            struct timeval tv;
            int r = 0;

            FD_ZERO(&fds);
            FD_SET(camHal->fd, &fds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            do
            {
                ALOGD("%s: b4 select on camHal->fd : %d", __func__, camHal->fd);
                r = select(camHal->fd + 1, &fds, NULL, NULL, &tv);
                ALOGD("%s: after select", __func__);
            }
            while((0 == r) || ((-1 == r) && (EINTR == errno)));

            if ((-1 == r) && (EINTR != errno))
            {
                ALOGE("%s: FDSelect ret = %d error: %d", __func__, r, errno);
                return (void *) - 1;
            }

        }
        /************************************************************************/
        /* - Dequeue capture buffer from USB camera                             */
        /************************************************************************/
        if (0 == get_buf_from_cam(camHal))
            ALOGD("%s: get_buf_from_cam success", __func__);
        else
            ALOGE("%s: get_buf_from_cam error", __func__);

        /************************************************************************/
        /* - Send capture buffer to JPEG encoder for JPEG compression           */
        /************************************************************************/
        mem_info = &camHal->pictMem.mem_info[0];
        mem_info->size = MAX_JPEG_BUFFER_SIZE;

        rc = allocate_ion_memory(mem_info,
                                 ((0x1 << ION_CP_MM_HEAP_ID) |
                                  (0x1 << ION_IOMMU_HEAP_ID)));

        if(rc)
            ALOGE("%s: ION memory allocation failed", __func__);

        camHal->pictMem.camera_memory[0] = camHal->get_memory(
                                               mem_info->fd, mem_info->size, 1, camHal->cb_ctxt);
        if(!camHal->pictMem.camera_memory[0])
            ALOGE("%s: get_mem failed", __func__);

        rc = encodeJpeg(camHal);

        ERROR_CHECK_EXIT_THREAD(rc, "jpeg_encode");

        if(jpegLength <= 0)
            ALOGI("%s: jpegLength : %d", __func__, jpegLength);

        ALOGD("%s: jpegLength : %d", __func__, jpegLength);
        /************************************************************************/
        /* - If jpeg frames callback is requested, callback with jpeg buffers   */
        /************************************************************************/
        /* TBD: CAMERA_MSG_RAW_IMAGE data call back */

        if ((camHal->msgEnabledFlag & CAMERA_MSG_COMPRESSED_IMAGE) &&
                (camHal->data_cb))
        {
            /* Unlock temporarily, callback might call HAL api in turn */
            camHal->lock.unlock();

            camHal->data_cb(CAMERA_MSG_COMPRESSED_IMAGE,
                            camHal->pictMem.camera_memory[0],
                            0, NULL, camHal->cb_ctxt);
            camHal->lock.lock();
        }

        /* release heap memory after the call back */
        if(camHal->pictMem.camera_memory[0])
            camHal->pictMem.camera_memory[0]->release(
                camHal->pictMem.camera_memory[0]);

        rc = deallocate_ion_memory(mem_info);
        if(rc)
            ALOGE("%s: ION memory de-allocation failed", __func__);

        /************************************************************************/
        /* - Enqueue capture buffer back to USB camera                          */
        /************************************************************************/
        if(0 == put_buf_to_cam(camHal))
        {
            ALOGD("%s: put_buf_to_cam success", __func__);
        }
        else
            ALOGE("%s: put_buf_to_cam error", __func__);

        /************************************************************************/
        /* - Free USB camera resources and close camera                         */
        /************************************************************************/
        rc = stopUsbCamCapture(camHal);
        ERROR_CHECK_EXIT_THREAD(rc, "stopUsbCamCapture");

        rc = unInitV4L2mmap(camHal);
        ERROR_CHECK_EXIT_THREAD(rc, "unInitV4L2mmap");

        USB_CAM_CLOSE(camHal);
        /************************************************************************/
        /* - If preview was stopped for taking picture, restart the preview     */
        /************************************************************************/
        if(camHal->prvwStoppedForPicture)
        {
            struct camera_device    device;
            device.priv = (void *)camHal;

            USB_CAM_OPEN(camHal);
            /* Unlock temporarily coz usbcam_start_preview has a lock */
            camHal->lock.unlock();
            rc = usbcam_start_preview(&device);
            if(rc)
                ALOGE("%s: start_preview error after take picture", __func__);
            camHal->lock.lock();
            camHal->prvwStoppedForPicture = 0;
        }

        /* take picture activity is done */
        camHal->takePictInProgress = 0;

        ALOGI("%s: X", __func__);
        return (void *)0;
    }
    static int launchTakePictureThread(camera_hardware_t *camHal)
    {
        ALOGD("%s: E", __func__);
        int rc = 0;

        if(!camHal)
        {
            ALOGE("%s: camHal is NULL", __func__);
            return -1;
        }

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        /* create the thread in detatched state, when the thread exits all */
        /* memory resources are freed up */
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&camHal->takePictureThread, &attr, takePictureThread, camHal);

        ALOGD("%s: X", __func__);
        return rc;
    }
    int usbcam_take_picture(struct camera_device *device)
    {
        ALOGI("%s: E", __func__);
        int rc = 0;
        camera_hardware_t *camHal;
	char startRecording[PROPERTY_VALUE_MAX];
	
        ALOGI("-------uvccam capture -----");
        system("echo 'captureenable' > /dev/lidbg_drivers_dbg0");
	
        return -1;
        VALIDATE_DEVICE_HDL(camHal, device, -1);

        Mutex::Autolock autoLock(camHal->lock);

        /* If take picture is already in progress, nothing t be done */
        if(camHal->takePictInProgress)
        {
            ALOGI("%s: Take picture already in progress", __func__);
            return 0;
        }

        if(camHal->previewEnabledFlag)
        {
            rc = stopPreviewInternal(camHal);
            if(rc)
            {
                ALOGE("%s: stopPreviewInternal returned error", __func__);
            }
            USB_CAM_CLOSE(camHal);
            camHal->prvwStoppedForPicture = 1;
        }
        /* TBD: Need to handle any dependencies on video recording state */
        rc = launchTakePictureThread(camHal);
        if(rc)
            ALOGE("%s: launchTakePictureThread error", __func__);

        if(!rc)
            camHal->takePictInProgress = 1;

        ALOGD("%s: X", __func__);
        return rc;
    }

    int usbcam_cancel_picture(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        ALOGD("%s: E", __func__);
        int rc = 0;

        ALOGD("%s: X", __func__);
        return -1;
    }

	#define PRINT_PARAM_STR(parms)    {\
        char temp[701] = {0};\
        int n=0;\
        while(1) {\
            strlcpy(temp,parms+n,700);\
            ALOGD("parms = %s", temp);\
            if (strlen(temp) < 700) break;\
            n += 700;\
        }\
    }

    int usbcam_set_parameters(struct camera_device *device, const char *params)
    {
    	ALOGD("%s: E", __func__);
        String8 str = String8(params);
		CameraParameters lParam;
		int width,height;
		int tmpval;
		camera_hardware_t *camHal;
		VALIDATE_DEVICE_HDL(camHal, device, -1);
        Mutex::Autolock autoLock(camHal->lock);
				
        int rc = 0;
		if(params)
   		     PRINT_PARAM_STR(params);
		lParam.unflatten(str);
		lParam.getPreviewSize(&width, &height);
		ALOGE("%s: width -> %d,height -> %d", __func__,width,height);
		if((width > 0) && (height > 0))
		{
			/*
			if((width == 480)&& (height == 270))
			{
				ALOGE("%s: 480x270 ==>set to 320x240", __func__);
				camHal->prevWidth   = 320;
		        camHal->prevHeight  = 240;
		        camHal->dispWidth   = 320;
		        camHal->dispHeight  = 240;
			}
			else
			*/
			{
				camHal->prevWidth   = width;
		        camHal->prevHeight  = height;
		        camHal->dispWidth   = width;
		        camHal->dispHeight  = height;
			}	
		}	
		/*set preview par*/
		tmpval = lParam.getInt("contrast");
		if(tmpval >= 0)
		{
			ALOGE("%s: contrast -> %d\n", __func__,tmpval);
			v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_CONTRAST",V4L2_CID_CONTRAST, tmpval);
		}
		tmpval = lParam.getInt("saturation");
		if(tmpval >= 0)
		{
			ALOGE("%s: saturation -> %d\n", __func__,tmpval);
			v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_SATURATION",V4L2_CID_SATURATION, tmpval);
		}
		tmpval = lParam.getInt("bright");
		if(tmpval >= 0)
		{
			ALOGE("%s: bright -> %d\n", __func__,tmpval);
			v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_BRIGHTNESS", V4L2_CID_BRIGHTNESS,tmpval);
		}
		tmpval = lParam.getInt("hue");
		if(tmpval >= 0)
		{
			ALOGE("%s: hue -> %d\n", __func__,tmpval);
			v4l2_vidio_s_ctrl(camHal->fd, "V4L2_CID_HUE", V4L2_CID_HUE,tmpval);
		}
		
		
        ALOGD("%s: X", __func__);
        return 0;
    }


    char *usbcam_get_parameters(struct camera_device *device)
    {
        char *parms;
        ALOGI("%s: E", __func__);

        camera_hardware_t *camHal;
        VALIDATE_DEVICE_HDL(camHal, device, NULL);
        Mutex::Autolock autoLock(camHal->lock);

        if(false)
        {
            //parms = usbCamGetParameters(camHal);
        }
        else
        {
            String8 params_str8;
            CameraParameters mParams ;
			
            mParams.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,  "1920x1080,1280x720,640x480,320x240,640x360");
            //mParams.setPictureSize(320, 240);
            mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1920x1080,1280x720,640x480,320x240,640x360");
            //mParams.setPreviewSize(320, 240);
            mParams.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
            mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, CameraParameters::PIXEL_FORMAT_YUV420SP);

	        mParams.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,  
	                "320x240");  
	       mParams.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,  
	                "320x240");  
			mParams.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, "auto");  
		    mParams.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);  
		    mParams.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, "auto");  
		    //mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "30");  
			mParams.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "320");  
			mParams.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "240");  
		    mParams.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, "320x240");  
		    mParams.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, "auto");  
		    mParams.set(CameraParameters::KEY_SUPPORTED_EFFECTS, "none");  
		    mParams.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, "auto");  
		    mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(1000,45000)"); //(1000,33000)  
		    mParams.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "1000,45000"); //"1000,33000"  
		    mParams.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "51.2");  
		    mParams.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "39.4");  

            params_str8 = mParams.flatten();
            parms = (char *) malloc(sizeof(char) * (params_str8.length() + 1));
            strcpy(parms, params_str8.string());
        }

        ALOGI("%s: X,%s", __func__, parms);
        return parms;
    }

    void usbcam_put_parameters(struct camera_device *device, char *parm)
    {
        struct camera_device *mdevice = device;
        char *mparm = parm;
        //       ALOGE("%s: E", __func__);
        //      ALOGE("%s: X", __func__);
        return;
    }

    int usbcam_send_command(struct camera_device *device,
                            int32_t cmd, int32_t arg1, int32_t arg2)
    {
        struct camera_device *mdevice = device;
        int32_t mcmd = cmd;
        int32_t marg1 = arg1;
        int32_t marg2 = arg2;

        int rc = 0;
        ALOGD("%s: E", __func__);
        ALOGD("%d", cmd);

        ALOGD("%s: X", __func__);
        return rc;
    }

    void usbcam_release(struct camera_device *device)
    {
        struct camera_device *mdevice = device;
        ALOGD("%s: E", __func__);
		if(cam_id == REARVIEW_ID) 
		{
			system("./flysystem/lib/out/lidbg_testuvccam /dev/video1 -b 0 --xuset-oe 1 1 ");/*open rear osd*/
			system("setprop lidbg.uvccam.rear.osdset 1&");
		}
        ALOGE("%s: X", __func__);
    }

    int usbcam_dump(struct camera_device *device, int fd)
    {
        struct camera_device *mdevice = device;
        int mfd = fd;
        ALOGD("%s: E", __func__);
        int rc = 0;

        ALOGD("%s: X", __func__);
        return rc;
    }

};
