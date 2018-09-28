#ifndef _LIGDBG_FLYCAM_APP__
#define _LIGDBG_FLYCAM_APP__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils/Log.h>
#include <cutils/uevent.h>
#include <cutils/properties.h>
#include <sys/poll.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <math.h>
#include <cutils/log.h>
#include "lidbg_servicer.h"
#include <linux/videodev2.h>
#include "../../drivers/inc/lidbg_flycam_par.h" /*flycam parameter*/

#define RECORD_MODE		0
#define PREVIEW_MODE		1
#define XU_MODE				2

#define FILENAME_LENGTH     (256)


static inline int lidbg_get_hub_uvc_device(int mode,char *devname,int cam_id,int node_num)
{
	char temp_devname[256], temp_devname2[256],hub_path[256];
    int  ret = 0, fd = -1;
    struct  v4l2_capability     cap;
	DIR *pDir ;  
	struct dirent *ent  ;  
	int fcnt = 0  ;  
	//char camID[PROPERTY_VALUE_MAX];

	char devopen[PROPERTY_VALUE_MAX];
	property_get("lidbg.uvccam.devopen", devopen, "0");
	if((!strncmp(devopen, "/", 1)))
	{
		strcpy(devname, devopen);
		lidbg("%s: futengfei.use node below.=======[%s],return\n", __func__, devname);
		return 0;
	}


#if 0
	if(mode == RECORD_MODE)  cam_id = 1;  //capture or recording force to camid 1
	else
	{
		property_get("fly.uvccam.camid", camID, "0");//according to last preview camid
		cam_id = atoi(camID);
	}

    if(id)
        cam_id = atoi(id);

	property_set("fly.uvccam.camid", id);
#endif

    lidbg("%s: E,=======[%d]\n", __func__, cam_id);
    *devname = '\0';

	memset(hub_path,0,sizeof(hub_path));  
	memset(temp_devname,0,sizeof(temp_devname));  

	//fix for attenuation hub.find the deepest one.
	unsigned int back_charcnt = 0,front_charcnt = 0;
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
	if(pDir != NULL) closedir(pDir);

	if((front_charcnt == 0) && (back_charcnt == 0))
	{
		lidbg("%s: can not found suitable hubpath! \n", __func__ );	
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
					if(fcnt == 3)
						sprintf(temp_devname,"/dev/%s", ent->d_name);  
					if(fcnt == 4)//also save 2nd node name
						sprintf(temp_devname2,"/dev/%s", ent->d_name);  
	        }  
	}

	if(pDir != NULL) closedir(pDir);
	
	lidbg("%s:First node Path:%s\n",__func__ ,temp_devname);      

	lidbg("%s: This Camera [%d] has %d video node.\n", __func__ ,cam_id, fcnt - 2);
	if(fcnt == 3)	
	{
		lidbg("%s: Camera [%d] does not support Sonix Recording!\n",__func__,cam_id);
	}
	
	if((fcnt == 0) && (ent == NULL))
	{
		lidbg("%s: Hub node is not exist !\n ", __func__);
		goto failproc;
	}

	if(node_num == 1) strncpy(temp_devname, temp_devname2, FILENAME_LENGTH);
	
openDev:
	  lidbg("%s: trying open ====[%s]\n", __func__, temp_devname);
      fd = open(temp_devname, O_RDWR  | O_NONBLOCK, 0);
      if(-1 != fd)
      {
          ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
          if((0 == ret) || (ret && (ENOENT == errno)))
          {
          	  //not usb cam node
              if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
              {
					lidbg("%s: This is not video capture device -> %s\n", __func__,temp_devname);
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
			  if((fcnt == 4)&&(mode == PREVIEW_MODE))
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
              lidbg("%s: Found UVC node,OK: ======%s,[camid = %d]\n", __func__, temp_devname, cam_id);
          }
          close(fd);
      }
      else if(2 != errno)
          lidbg("%s: Probing.%s: ret: %d, errno: %d,%s\n", __func__, temp_devname, ret, errno, strerror(errno));
    lidbg("%s: X,with mode[%d] we use %s\n", __func__,mode, devname);
    return 0;

failproc:
	lidbg("%s: Probing fail:%s\n ", __func__, devname);
	strncpy(devname, "/dev/video1", FILENAME_LENGTH);
	return -1;
}

#endif
